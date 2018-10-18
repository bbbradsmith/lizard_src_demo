// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include <cstring>
#include "lizard_game.h"
#include "lizard_audio.h"
#include "assets/export/data_music.h"

#if AUDIO_CHANNELS != 1 && AUDIO_CHANNELS != 2
#error AUDIO_CHANNELS must be 1 or 2.
#endif

extern bool system_pal(); // in lizard_main.cpp
extern bool system_music();
extern void system_audio_lock();
extern void system_audio_unlock();

static uint8 next_music_unmasked = MUSIC_SILENT; // used to restore if option changes during play

// this data represents the music engine NES ZP usage
struct NES_PLAYER
{
	uint8 pal;
	uint8 next_sound[2];
	uint8 next_music;
	uint8 pause;
	uint8 current_music;
	uint8 bg_noise_freq;
	uint8 bg_noise_vol;
	uint8 music_mask;

	uint8 speed;
	uint8 pattern_length;
	uint8 row;
	uint8 row_sub;
	uint8 order_frame;

	uint8 row_skip[4];
	uint8 macro_pos[4][4];
	uint8 note[4];
	uint8 vol[4];
	uint8 halt[4];
	sint16 pitch[4];

	uint8 sfx_on[2];
	uint8 sfx_skip[2];

	uint8 vol_out[4];
	uint16 freq_out[4];
	uint8 duty_out[4];
	uint8 apu_high[4]; // used in NES implementation to prevent phase reset

	uint16 rain_seed; // PRNG for rain
};

struct NES_PLAYER_POINTERS
{
	// pointers
	const uint8* const * pattern_table;
	const uint8* order;
	const uint8* pattern[4];
	const sint8* macro[4][4];
	const uint8* sfx_pattern[2];
};

static struct NES_PLAYER player;
static struct NES_PLAYER_POINTERS pointer;

// APU represents the internal NES audio

static bool apu_mute;

static unsigned int apu_vol[4];
static unsigned int apu_freq[4];
static unsigned int apu_duty[4];

const unsigned int MAX_FRAME_SAMPLES = 48000 / 50;
const unsigned int OVERSAMPLE = 4;
const double CPU_NTSC = 1789772.0;
const double CPU_PAL  = 1662607.0;

static int samplerate = 48000;
static double sample_clocks = CPU_NTSC / double(48000);
static unsigned int frame_samples = 48000 / 60;

static signed int apu_oversample[MAX_FRAME_SAMPLES*OVERSAMPLE];
static signed int apu_output[MAX_FRAME_SAMPLES];
static int apu_filled;
static double apu_accum;

static unsigned int apu_noise;
static unsigned int apu_noise_tap;
static unsigned int apu_phase[4];
static unsigned int apu_wave[4];

static unsigned int apu_triangle_halt;

static signed int last_render;

// fixed point volume control

static int volume      = 256;
const int VOLUME_FIXED = 256;

// DC highpass filter

// factor = 1024 / ((2 pi * (1 / 48000) * cutoff) + 1)
// 1016/1024 ~= 60 Hz roughly equivalent to my NES
// 1018/1024 ~= 45 Hz slightly weaker than the NES
// 1020/1024 ~= 30 Hz

// Using slightly weaker filter just because all I really care about is rolling off the DC offset.

const signed int DC_FILTER_FACTOR = 1018;
const signed int DC_FILTER_FIXED  = 1024;

static signed int dc_filter_in;
static signed int dc_filter_out;

static inline signed int dc_filter(signed int render)
{
	dc_filter_out = DC_FILTER_FACTOR * (dc_filter_out + render - dc_filter_in) / DC_FILTER_FIXED;
	dc_filter_in = render;
	return dc_filter_out;
}

// roughly based on analog NES mix levels
const double MASTER_VOL = 790.0; // maximum without clipping should be ~806
const signed int SQUARE_MIX   = int(MASTER_VOL * 0.69);
const signed int TRIANGLE_MIX = int(MASTER_VOL * 0.83);
const signed int NOISE_MIX    = int(MASTER_VOL * 0.50);

static const signed int SQUARE_WAVE[4][32] = {
	{ 0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0 },
	{ 0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0 },
	{ 0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0 },
	{ 1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1 }
};

static const signed int TRIANGLE_WAVE[32] = {
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
};

static const unsigned int NOISE_FREQ[16] = {
	4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

void apu_set_region()
{
	NES_ASSERT(samplerate == 48000 || samplerate == 44100, "Invalid samplerate!");
	frame_samples = samplerate / (system_pal() ? 50 : 60);
	player.pal = system_pal() ? 1 : 0;
	sample_clocks = (system_pal() ? CPU_PAL : CPU_NTSC) / double(samplerate);
	apu_mute = false; // change in buffer size means tick_frame has to re-fill the silent buffer
	apu_filled = 0;
}

void apu_set_vol(int c, uint8 v)
{
	apu_vol[c] = v & 15;
	if (c == 2)
	{
		apu_triangle_halt = apu_wave[2];
	}
}

void apu_set_freq(int c, unsigned int freq)
{
	if (c == 3)
	{
		apu_freq[c] = NOISE_FREQ[freq&15];
	}
	else
	{
		apu_freq[c] = (freq & 0x7FF) + 1;
	}
}

void apu_set_duty(int c, uint8 d)
{
	if (c == 3)
	{
		apu_noise_tap = (d&1) ? (1<<6) : (1<<1);
	}
	else
	{
		apu_duty[c] = d & 3;
	}
}

void apu_render_frame()
{
	if (player.pause)
	{
		// apu_output was already filled via tick_frame
		apu_filled = frame_samples;
		return;
	}

	//for (unsigned int i=0; i<frame_samples; ++i)
	for (unsigned int i=0; i<(frame_samples*OVERSAMPLE); ++i)
	{
		//apu_accum += sample_clocks;
		apu_accum += (sample_clocks / double(OVERSAMPLE));
		int clocks = int(apu_accum);
		apu_accum -= double(clocks);

		for (unsigned int c=0; c<4; ++c)
		{
			apu_phase[c] += clocks;
			while(apu_phase[c] >= apu_freq[c])
			{
				apu_wave[c] = (apu_wave[c]+1) & 31;
				apu_phase[c] -= apu_freq[c];
			}
		}

		if (apu_vol[2] == 0) apu_wave[2] = apu_triangle_halt;

		// average noise
		signed int noise = (apu_noise & 0x4000) ? 1 : 0;
		for (unsigned int n=0; n<apu_wave[3]; ++n)
		{
			unsigned int feedback = (apu_noise & 1) ^ ((apu_noise & apu_noise_tap) ? 1 : 0);
			apu_noise = (apu_noise>>1) | (feedback<<14);
			noise += (apu_noise & 0x4000) ? 1 : 0;
		}

		apu_oversample[i] =
			((
				(SQUARE_WAVE[apu_duty[0]][apu_wave[0]] ? apu_vol[0] : 0) +
				(SQUARE_WAVE[apu_duty[1]][apu_wave[1]] ? apu_vol[1] : 0)
			) * SQUARE_MIX) +
			(TRIANGLE_WAVE[apu_wave[2]] * TRIANGLE_MIX) +
			((noise * apu_vol[3] * NOISE_MIX) / (apu_wave[3]+1));

		apu_wave[3] = 0; // reset noise counter
	}
	apu_filled = frame_samples;

	signed int render = 0;
	for (unsigned int i=0;i<frame_samples;++i)
	{
		signed int accum = 0;
		for (int j=0;j<OVERSAMPLE;++j)
			accum += apu_oversample[(i*OVERSAMPLE)+j];
		render = accum / OVERSAMPLE;

		// The NES also has a lowpass filter, but it seemed unnecessary after simple oversampling.

		apu_output[i] = dc_filter(render);
	}
	last_render = render;

	// NES can reseed its noise generator if it ever hits 0, doing this once per frame of audio
	if (apu_noise == 0)
	{
		apu_noise = 1;
	}
}

void sfx_queue_init(); // forward declaration

void apu_init()
{
	sfx_queue_init();

	memset(apu_vol,  0,sizeof(apu_vol));
	memset(apu_duty, 0,sizeof(apu_duty));
	memset(apu_phase,0,sizeof(apu_phase));
	memset(apu_wave, 0,sizeof(apu_wave));
	apu_accum = 0.0;
	apu_noise = 1;
	apu_noise_tap = 1<<1;
	apu_triangle_halt = 0;
	apu_filled = 0;
	apu_freq[0] = 1;
	apu_freq[1] = 1;
	apu_freq[2] = 1;
	apu_freq[3] = 1;
	dc_filter_in = 0;
	dc_filter_out = 0;
	last_render = 0;

	apu_set_region();
}

// SFX queue

// the audio buffer refills less often than 1 per game frame,
// so creating an artificial queue to buffer sound events
// from subsequent game frames and play them back 1 per tick

// 

const int SFX_QUEUE_DEPTH = 8;
static int   sfx_queue_pos        [2];
static uint8 sfx_queue            [2][SFX_QUEUE_DEPTH];
static uint8 sfx_queue_frame      [2][SFX_QUEUE_DEPTH];
static uint8 sfx_queue_last_frame [2];

void sfx_queue_init()
{
	memset(sfx_queue_pos,       0,sizeof(sfx_queue_pos));
	memset(sfx_queue,           0,sizeof(sfx_queue));
	memset(sfx_queue_frame,     0,sizeof(sfx_queue_frame));
	memset(sfx_queue_last_frame,0,sizeof(sfx_queue_last_frame));
};

void sfx_queue_sound(uint8 s)
{
	NES_ASSERT(s < SFX_COUNT, "out of range sound");
	int c = data_sfx_mode[s];
	NES_ASSERT(c < 2, "out of range SFX channel?");

	const uint8 f = Game::zp.nmi_count;

	int p = sfx_queue_pos[c];
	if (p > 0 && sfx_queue_frame[c][p-1] == f)
	{
		p -= 1; // multiple sounds in the same frame, overwrite
	}
	if (p >= SFX_QUEUE_DEPTH)
	{
		p = SFX_QUEUE_DEPTH - 1; // fallback if the queue is full
	}

	sfx_queue[c][p] = s;
	sfx_queue_frame[c][p] = f; // remember frame for overwrite
	sfx_queue_pos[c] = p+1;
}

void sfx_queue_prepare() // call once per audio buffer block, before using sfx_queue_advance
{
	//NES_DEBUG("sfx_queue_prepare()\n");

	// set the queue's simulated "game frame" to the time of the first event in the queue
	// (allows the queue to catch up to now/ASAP when it's not full)
	for (int c=0; c<2; ++c)
	{
		if (sfx_queue_pos[c] > 0)
		{
			sfx_queue_last_frame[c] = sfx_queue_frame[c][0];
		}
	}
}

void sfx_queue_advance() // call once per tick_frame to load player_next_sound from the queue
{
	//NES_DEBUG("sfx_queue_advance()\n");

	for (int c=0; c<2; ++c)
	{
		if (sfx_queue_pos[c] > 0)
		{
			if (sfx_queue_frame[c][0] == sfx_queue_last_frame[c])
			{
				//NES_DEBUG("next_sound[%d] = %3d (%02X)\n", c,sfx_queue[c][0],sfx_queue_last_frame[c]);

				player.next_sound[c] = sfx_queue[c][0];
				--sfx_queue_pos[c];
				for (int i=0; i < sfx_queue_pos[c]; ++i)
				{
					sfx_queue[c][i]       = sfx_queue[c][i+1];
					sfx_queue_frame[c][i] = sfx_queue_frame[c][i+1];
				}
				++sfx_queue_last_frame[c];
			}
			else
			{
				// skip this simualted "game frame", but play it on the next one
				sfx_queue_last_frame[c] = sfx_queue_frame[c][0];

				// for accurate synchronized timing, would wait as many frames as needed,
				// but we intentionally let the buffer slip/catch-up when the queue isn't full
				// to keep latency a little lower, and prevent locked waits
			}
		}
	}
}

// music playback engine

// reimplementation of prng() from lizard_game.cpp for thread-safety (used here for rain sound)
static uint8 rain_prng()
{
	// NOTE: a seed of 0 will loop to itself
	NES_ASSERT(player.rain_seed != 0, "rain_prng in deadlock!");

	if (player.rain_seed & 0x8000) player.rain_seed = (player.rain_seed << 1) ^ 0x00D7;
	else                           player.rain_seed = (player.rain_seed << 1);

	// return noise value
	const uint8 RAIN_FREQ[8] = { 0xF, 0xF, 0xF, 0xE, 0xE, 0xE, 0xE, 0xD };
	return 0xF - RAIN_FREQ[player.rain_seed & 7];
}

static sint8 tick_macro(int c, int m)
{
	sint8 a;
	do
	{
		a = pointer.macro[c][m][player.macro_pos[c][m]];
		++player.macro_pos[c][m];
		if (a == -128) // loop
		{
			player.macro_pos[c][m] = pointer.macro[c][m][player.macro_pos[c][m]];
		}
	}
	while (a == -128);
	return a;
}

static void player_load_music()
{
	NES_ASSERT((player.next_music & player.music_mask) < MUSIC_COUNT, "Music out of range.");
	player.current_music = player.next_music & player.music_mask;

	player.speed = data_music_speed[player.current_music];
	player.pattern_length = data_music_pattern_length[player.current_music];
	pointer.order = data_music_order[player.current_music];
	pointer.pattern_table = data_music_pattern[player.current_music];

	player.row = 0;
	player.row_sub = 0;
	player.order_frame = 0;
	for (int i=0; i<4; ++i)
	{
		// load first order patterns
		pointer.pattern[i] = pointer.pattern_table[pointer.order[player.order_frame+i]];
		player.row_skip[i] = 0;
		// reset channels
		pointer.macro[i][0] = data_music_macro[0];
		pointer.macro[i][1] = data_music_macro[0];
		pointer.macro[i][2] = data_music_macro[0];
		pointer.macro[i][3] = data_music_macro[0];
		player.macro_pos[i][0] = 0;
		player.macro_pos[i][1] = 0;
		player.macro_pos[i][2] = 0;
		player.macro_pos[i][3] = 0;
		player.vol[i] = 15;
		player.halt[i] = 1;
	}
}

static void load_sfx(uint8 chan)
{
	NES_ASSERT(chan < 2, "load_sfx with incorrect channel?");
	NES_ASSERT(player.next_sound[chan] < SFX_COUNT, "SFX out of range.");
	NES_ASSERT(chan == data_sfx_mode[player.next_sound[chan]], "SFX type mismatch.");

	player.sfx_on[chan] = 1;
	player.sfx_skip[chan] = 0;
	pointer.sfx_pattern[chan] = data_sfx[player.next_sound[chan]];
	player.next_sound[chan] = 0;

	if (chan == 1)
	{
		player.duty_out[3] = 0; // cancel periodic noise
	}
}

static void tick_frame()
{
	next_music_unmasked = player.next_music;
	if (player.current_music != (player.next_music & player.music_mask))
	{
		player_load_music();
	}

	sfx_queue_advance(); // reloads player.next_sound if sounds have been issued faster than we can keep up

	if (player.next_sound[0]) load_sfx(0);
	if (player.next_sound[1]) load_sfx(1);

	if (player.pause)
	{
		if (!apu_mute)
		{
			// render last value through DC filter until we get a completely silent frame
			apu_mute = true;
			for (unsigned int i=0; i<frame_samples; ++i)
			{
				apu_output[i] = dc_filter(last_render);
				if (apu_output[i] != 0) apu_mute = false;
			}
		}
		return;
	}
	else
	{
		apu_mute = false;
	}

pal_repeat:

	// tick pattern
	if (player.row_sub <= 0)
	{
		++player.row;
		for (int i=0; i<4; ++i)
		{
			if (player.row_skip[i] > 0) --player.row_skip[i];
			else
			{
				bool read_loop = true;
				while(read_loop)
				{
					uint8 command = pointer.pattern[i][0];
					++pointer.pattern[i];

					if (command < 0x80) // skip and end row
					{
						player.row_skip[i] = command;
						read_loop = false;
					}
					else if (command == 0x80) // halt
					{
						player.halt[i] = 1;
						read_loop = false;
					}
					else if (command >= 0x81 && command < 0xE0)
					{
						player.halt[i] = 0;
						player.note[i] = command - 0x81;
						player.macro_pos[i][0] = 0;
						player.macro_pos[i][1] = 0;
						player.macro_pos[i][2] = 0;
						player.macro_pos[i][3] = 0;
						player.pitch[i] = 0;
						read_loop = false;
					}
					else if (command >= 0xE0 && command < 0xF0) // volume
					{
						player.vol[i] = command & 0x0F;
					}
					else if (command == 0xF0) // instrument
					{
						uint8 param = pointer.pattern[i][0];
						++pointer.pattern[i];

						const uint8* instrument = data_music_instrument + (param*4);

						player.macro_pos[i][0] = 0;
						player.macro_pos[i][1] = 0;
						player.macro_pos[i][2] = 0;
						player.macro_pos[i][3] = 0;
						
						pointer.macro[i][0] = data_music_macro[instrument[0]];
						pointer.macro[i][1] = data_music_macro[instrument[1]];
						pointer.macro[i][2] = data_music_macro[instrument[2]];
						pointer.macro[i][3] = data_music_macro[instrument[3]];
					}
					else if (command == 0xF1) // BXX
					{
						uint8 p = pointer.pattern[i][0];
						++pointer.pattern[i];
						player.row = player.pattern_length;
						player.order_frame = (p*4)-4;
					}
					else if (command == 0xF2) // D00
					{
						player.row = player.pattern_length;
					}
					else if (command == 0xF3) // FXX
					{
						uint8 s = pointer.pattern[i][0];
						++pointer.pattern[i];
						player.speed = s;
					}
					else
					{
						// unimplemented command
						++pointer.pattern[i];
					}
				}
			}
		}

		if (player.row >= player.pattern_length)
		{
			player.order_frame += 4;
			player.row = 0;
			for (int i=0; i<4; ++i)
			{
				pointer.pattern[i] = pointer.pattern_table[pointer.order[player.order_frame+i]];
				player.row_skip[i] = 0;
			}
		}

		player.row_sub = player.speed;
	}
	--player.row_sub;

	// tick sfx
	for (int i=0; i<2; ++i)
	{
		if (!player.sfx_on[i]) continue;

		const int c = (i == 1) ? 3 : 0;

		if (player.sfx_skip[i])
		{
			--player.sfx_skip[i];
			continue;
		}

		bool read_loop = true;
		while (read_loop)
		{
			uint8 command = pointer.sfx_pattern[i][0];
			++pointer.sfx_pattern[i];

			if (command < 0x80)
			{
				player.sfx_skip[i] = command;
				read_loop = false;
			}
			else if (command == 0x80)
			{
				player.sfx_on[i] = 0;
				//player.vol_out[c] = 0;
				player.halt[c] = 1; // kill until next note
				read_loop = false;
			}
			else if (command >= 0x81 && command < 0xE0)
			{
				uint8 note = command - 0x81;
				player.freq_out[c] = c == 3 ? (0x0F - (note & 0xF)) : data_music_tuning[note];
				read_loop = false;
			}
			else if (command >= 0xE0 && command < 0xF0)
			{
				player.vol_out[c] = command & 0xF;
			}
			else if (command >= 0xF0)
			{
				uint8 v = command - 0xF0;
				player.duty_out[c] = v;
			}
		}
	}
	
	// tick macros to play music
	for (int i=0; i<4; ++i)
	{
		sint8 m_vol = tick_macro(i,0);
		sint8 m_arp = tick_macro(i,1);
		sint8 m_pit = tick_macro(i,2);
		sint8 m_dut = tick_macro(i,3);

		uint8 note = player.note[i] + m_arp;
		if (note >= 96) note = 95;

		player.pitch[i] += m_pit;
		signed int freq = data_music_tuning[note];
		signed int pitch = player.pitch[i] + freq;
		freq = pitch & 0x7FF;

		if (i==0 && player.sfx_on[0]) continue;
		if (i==3)
		{
			if (player.sfx_on[1]) continue;
			if (player.bg_noise_vol > 0)
			{
				player.vol_out[3] = player.bg_noise_vol & 0xF;
				player.freq_out[3] = (0xF - player.bg_noise_freq) & 0xF;
				player.duty_out[3] = 0;

				// 0xE treated as special "rain"
				if (player.bg_noise_freq == 0xE)
				{
					player.freq_out[3] = rain_prng();
				}

				continue;
			}
		}

		player.vol_out[i] = player.halt[i] ? 0 : data_music_multiply[m_vol][player.vol[i]];
		player.freq_out[i] = i == 3 ? (0x0F - (note & 0xF)) : freq;
		player.duty_out[i] = m_dut;
	}

	// send the sound
	for (int i=0; i<4; ++i)
	{
		apu_set_vol(i,player.vol_out[i]);
		apu_set_freq(i,player.freq_out[i]);
		apu_set_duty(i,player.duty_out[i]);
	}

	if (player.pal != 0)
	{
		// PAL mode double-ticks every 5th frame
		if (player.pal == 1)
		{
			player.pal = 6;
			goto pal_repeat;
		}
		else
		{
			--player.pal;
		}
	}
}

static void render_block(sint16* b, int len)
{
	sfx_queue_prepare();

	while (len)
	{
		if (apu_filled < 1)
		{
			if (system_pal() ^ (player.pal != 0)) // PAL setting has been changed at runtime
			{
				apu_set_region();
			}

			tick_frame();
			apu_render_frame();
		}

		signed int *o = apu_output + (frame_samples - apu_filled);
		if (len < apu_filled)
		{
			apu_filled -= len;
			while (len)
			{
				signed int ov = (*o * volume) / VOLUME_FIXED;
				*b = ov;
				#if AUDIO_CHANNELS == 2
					*(b+1) = ov;
				#endif
				b += AUDIO_CHANNELS;
				++o;
				--len;
			}
		}
		else
		{
			while (apu_filled)
			{
				signed int ov = (*o * volume) / VOLUME_FIXED;
				*b = ov;
				#if AUDIO_CHANNELS == 2
					*(b+1) = ov;
				#endif
				b += AUDIO_CHANNELS;
				++o;
				--len;
				--apu_filled;
			}
		}
	}
}

//
// Public stuff
//

void play_music(uint8 m)
{
	system_audio_lock();
	player.next_music = m;
	system_audio_unlock();
}

void enable_music(bool enable)
{
	system_audio_lock();
	player.music_mask = enable ? 0xFF : 0x00;
	if (enable)
	{
		player.next_music = next_music_unmasked;
	}
	system_audio_unlock();
}

void play_sound(uint8 s)
{
	system_audio_lock();
	if (s < SFX_COUNT)
	{
		NES_ASSERT(data_sfx_mode[s] < 2, "SFX mode invalid?");
		sfx_queue_sound(s);
	}
	system_audio_unlock();
}

void play_bg_noise(uint8 n, uint8 v)
{
	NES_ASSERT(n <= 0xF, "invalid play_bg_noise parameter");
	NES_ASSERT(v <= 0xF, "invalid play_bg_noise parameter");

	system_audio_lock();
	player.bg_noise_freq = n;
	player.bg_noise_vol = v;
	system_audio_unlock();
}

void pause_audio()
{
	system_audio_lock();
	++player.pause;
	system_audio_unlock();
}

void unpause_audio()
{
	system_audio_lock();
	--player.pause;
	system_audio_unlock();
}

uint8 get_next_music()
{
	return player.next_music;
}

bool get_music_enabled()
{
	return (player.music_mask != 0);
}

void play_init()
{
	system_audio_lock();

	memset(&player,0,sizeof(player));
	next_music_unmasked = 0;
	player.rain_seed = 1;

	apu_init();
	player.music_mask = system_music() ? 0xFF : 0x00;
	pointer.sfx_pattern[0] = data_sfx[0];
	pointer.sfx_pattern[1] = data_sfx[1];
	player_load_music();

	system_audio_unlock();
}

void play_samplerate(int samplerate_)
{
	system_audio_lock();
	samplerate = samplerate_;
	apu_set_region();
	system_audio_unlock();
}

void play_volume(int volume_)
{
	system_audio_lock();
	if      (volume_ <   0) volume_ = 0;
	else if (volume_ > 100) volume_ = 100;
	volume = (volume_ * VOLUME_FIXED) / 100;
	NES_DEBUG("play_volume(%d) = %d\n",volume_,volume);
	system_audio_unlock();
}

void play_render(sint16* buffer, int len)
{
	render_block(buffer,len);
}

// end of file
