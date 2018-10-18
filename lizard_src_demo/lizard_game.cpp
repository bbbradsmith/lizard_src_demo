// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include <cstdlib>
#include <cstring>
#include "lizard_game.h"
#include "lizard_ppu.h"
#include "lizard_audio.h"
#include "lizard_lizard.h"
#include "lizard_dogs.h"
#include "lizard_text.h"
#include "lizard_version.h"
#include "assets/export/data.h"
#include "assets/export/data_music.h"
#include "enums.h"

// in lizard_main.cpp
extern bool system_pal();
extern void system_pal_set(bool on);
extern bool system_easy();
extern void system_easy_set(bool on);
extern bool system_music();
extern void system_music_set(bool on);
extern bool system_resume();
extern void resume_save();
extern bool system_playback();

// for log testing
//#define DEBUG_MODE(x) NES_DEBUG("%s(%02X)\n",(x),zp.gamepad);
#define DEBUG_MODE(x) {}

namespace Game
{

//
// NES data
//

struct NES_ZP zp;
struct NES_STACK stack;
struct NES_RAM h;
struct Lizard lizard;

struct Resume resume;

//
// Special variables, not part of NES implementation
//

static bool option_mode;
static unsigned int game_mode;
bool debug = false;
bool debug_dogs = false;
int debug_oam_mark = 0; // oam tiles rendered
int debug_oam_loss = 0; // oam tiles discarded (overflowed limit of 64)
int ntsc_phase = 0; // randomized PPU phase for NTSC colour artifact simulation
bool pending_resume_save = false; // autosave during room transitions to hide load hitch

//
// Misc
//

extern const uint8 hex_to_ascii[16]; // in lizard_dogs.cpp

const uint8 TIP_POINT[TIP_MAX] = { 13,17,19,23,29,31 };

uint8 prng()
{
	// NOTE: a seed of 0 will loop to itself
	NES_ASSERT(zp.seed != 0, "prng in deadlock!");

	if (zp.seed & 0x8000) zp.seed = (zp.seed << 1) ^ 0x00D7;
	else                  zp.seed = (zp.seed << 1);

	return zp.seed & 0xFF;
}

uint8 prng(int count)
{
	NES_ASSERT(count > 0,"prng with count of 0");
	uint8 result = 0;
	while (count)
	{
		result = prng();
		--count;
	}
	return result;
}

bool coin_read(uint8 c)
{
	NES_ASSERT(c < 128,"Coin out of range!");
	uint8 pos = c >> 3;
	uint8 bit = c & 7;

	return (h.coin[pos] & (1<<bit))!=0;
}

void coin_take(uint8 c)
{
	NES_ASSERT(c < 128,"Coin out of range!");
	uint8 pos = c >> 3;
	uint8 bit = c & 7;

	h.coin[pos] |= (1<<bit);
	NES_ASSERT(coin_read(c),"coin_take() failed");

	zp.coin_saved = 0;
}

void coin_return(uint8 c)
{
	NES_ASSERT(c < 128,"Coin out of range!");
	uint8 pos = c >> 3;
	uint8 bit = c & 7;

	h.coin[pos] &= ~uint8(1<<bit);
	NES_ASSERT(!coin_read(c),"coin_read() failed");

	zp.coin_saved = 0;
}

uint8 coin_count()
{
	uint8 count = 0;
	for (int i=0; i<128; ++i)
	{
		if (coin_read(i)) ++count;
	}
	return count;
}

bool flag_read(uint8 f)
{
	NES_ASSERT(f < 128,"Flag out of range!");
	uint8 pos = f >> 3;
	uint8 bit = f & 7;

	return (h.flag[pos] & (1<<bit))!=0;
}

void flag_set(uint8 f)
{
	NES_ASSERT(f < 128,"Flag out of range!");
	uint8 pos = f >> 3;
	uint8 bit = f & 7;

	h.flag[pos] |= (1<<bit);
	NES_ASSERT(flag_read(f),"flag_set() failed");

	zp.coin_saved = 0;
}

void flag_clear(uint8 f)
{
	NES_ASSERT(f < 128,"Flag out of range!");
	uint8 pos = f >> 3;
	uint8 bit = f & 7;

	h.flag[pos] &= ~uint8(1<<bit);
	NES_ASSERT(!flag_read(f),"flag_clear() failed");

	zp.coin_saved = 0;
}

void decimal_clear()
{
	for (int i=0; i<5; ++i) h.decimal[i] = 0;
}

void decimal_add(uint8 x)
{
	while (x > 0)
	{
		int digit = 0;

		if (x >= 100)
		{
			digit = 2;
			x -= 99;
		}
		else if (x >= 10)
		{
			digit = 1;
			x -= 9;
		}

		while (digit < 5)
		{
			++h.decimal[digit];
			if (h.decimal[digit] >= 10)
			{
				h.decimal[digit] = 0;
			}
			else
			{
				break;
			}
			++digit;
		}
		if (digit >= 5)
		{
			for (int i=0; i<5; ++i) h.decimal[i] = 9; // maxed out at 99999
			return;
		}
		--x;
	}
}

void decimal_add32(uint32 x)
{
	while (x >= 256)
	{
		decimal_add(255);
		decimal_add(1);
		x -= 256;
	}
	NES_ASSERT(x < 256, "decimal_add32 failure");
	decimal_add(uint8(x));
}

void decimal_print()
{
	uint8 d = 5;
	do
	{
		--d;
		if (h.decimal[d] != 0) goto finish;
		PPU::write(0x70);
	} while (d > 1);

	do
	{
		--d;
	finish:
		PPU::write(0x60 + h.decimal[d]);
	} while (d > 0);
}

void metric_print()
{
	uint8 d = 5;
	do
	{
		--d;
		if (h.decimal[d] != 0) goto finish;
		PPU::write(0xAB);
	} while (d > 1);

	do
	{
		--d;
	finish:
		PPU::write(0xA0 + h.decimal[d]);
	} while (d > 0);
}

void dogs_cycle()
{
	const int DOG_ADD[8] = { 1, 5, 11, 15, 7, 3, 13, 9 }; // relative primes that will cycle all dogs
	// properties: 1 vs 15 order flips every frame
	//             0 is always at 0
	//             8 is always at 8
	zp.dog_add_select = (zp.dog_add_select + 1) & 7;
	zp.dog_add = DOG_ADD[zp.dog_add_select];
}

void nmi_update_at(uint8 x, uint8 y)
{
	zp.nmi_load = 0x2000 + (32 * y) + x;
}

void ppu_nmi_update_row()
{
	PPU::latch(zp.nmi_load);
	ppu_nmi_write_row();
}

void ppu_nmi_write_row()
{
	for (int i=0; i<32; ++i)
	{
		PPU::write(stack.nmi_update[i]);
	}
}

void ppu_nmi_update_double()
{
	PPU::latch(zp.nmi_load);
	for (int i=0; i<64; ++i) PPU::write(stack.nmi_update[i]);
}

//
// Meta sprites
//

void sprite_0_init()
{
	h.oam[0] = 0xFF; // Y = offscreen
	h.oam[1] = 0x2F; // tile
	h.oam[2] = 0x20; // beneath background
	//h.oam[2] = 0; // for testing
	h.oam[3] = 247; // X position
}

void sprite_begin()
{
	zp.oam_pos = 4;
	debug_oam_mark = 0;
	debug_oam_loss = 0;
}

// forward declarations
static void sprite_add(uint8 x, uint8 y, const uint8* meta, uint8 count);
static void sprite_add_flipped(uint8 x, uint8 y, const uint8* meta, uint8 count);
static void sprite_add_edge(uint8 x, uint8 y, const uint8* meta, uint8 count);
static void sprite_add_flipped_edge(uint8 x, uint8 y, const uint8* meta, uint8 count);

void sprite_prepare(uint8 x)
{
	zp.sprite_center = (x ^ (x >> 1)) & 0xC0;
}

void sprite0_add(uint8 x, uint8 y, uint8 sprite)
{
	NES_ASSERT(sprite < DATA_sprite0_COUNT,"Sprite out of range!");
	uint8 count = data_sprite0_tile_count[sprite];
	zp.att_or = data_sprite0_vpal[sprite] ? (zp.dog_now & 1) : 0;
	NES_ASSERT(count > 0, "Empty sprite not allowed!");
	const uint8* meta = data_sprite0[sprite];

	sprite_add(x,y,meta,count);
}

void sprite1_add(uint8 x, uint8 y, uint8 sprite)
{
	NES_ASSERT(sprite < DATA_sprite1_COUNT,"Sprite out of range!");
	uint8 count = data_sprite1_tile_count[sprite];
	zp.att_or = data_sprite1_vpal[sprite] ? (zp.dog_now & 1) : 0;
	NES_ASSERT(count > 0, "Empty sprite not allowed!");
	const uint8* meta = data_sprite1[sprite];

	sprite_add(x,y,meta,count);
}

void sprite2_add(uint8 x, uint8 y, uint8 sprite)
{
	NES_ASSERT(sprite < DATA_sprite2_COUNT,"Sprite out of range!");
	uint8 count = data_sprite2_tile_count[sprite];
	zp.att_or = data_sprite2_vpal[sprite] ? (zp.dog_now & 1) : 0;
	NES_ASSERT(count > 0, "Empty sprite not allowed!");
	const uint8* meta = data_sprite2[sprite];

	sprite_add(x,y,meta,count);
}

void sprite0_add_flipped(uint8 x, uint8 y, uint8 sprite)
{
	NES_ASSERT(sprite < DATA_sprite0_COUNT,"Sprite out of range!");
	uint8 count = data_sprite0_tile_count[sprite];
	zp.att_or = data_sprite0_vpal[sprite] ? (zp.dog_now & 1) : 0;
	NES_ASSERT(count > 0, "Empty sprite not allowed!");
	const uint8* meta = data_sprite0[sprite];

	sprite_add_flipped(x,y,meta,count);
}

void sprite1_add_flipped(uint8 x, uint8 y, uint8 sprite)
{
	NES_ASSERT(sprite < DATA_sprite1_COUNT,"Sprite out of range!");
	uint8 count = data_sprite1_tile_count[sprite];
	zp.att_or = data_sprite1_vpal[sprite] ? (zp.dog_now & 1) : 0;
	NES_ASSERT(count > 0, "Empty sprite not allowed!");
	const uint8* meta = data_sprite1[sprite];

	sprite_add_flipped(x,y,meta,count);
}

void sprite2_add_flipped(uint8 x, uint8 y, uint8 sprite)
{
	NES_ASSERT(sprite < DATA_sprite2_COUNT,"Sprite out of range!");
	uint8 count = data_sprite2_tile_count[sprite];
	zp.att_or = data_sprite2_vpal[sprite] ? (zp.dog_now & 1) : 0;
	NES_ASSERT(count > 0, "Empty sprite not allowed!");
	const uint8* meta = data_sprite2[sprite];

	sprite_add_flipped(x,y,meta,count);
}

static void sprite_add(uint8 x, uint8 y, const uint8* meta, uint8 count)
{
	NES_ASSERT(meta, "NULL sprite!");

	if (!(zp.sprite_center & 0x40))
	{
		sprite_add_edge(x,y,meta,count);
		return;
	}

	while (count)
	{
		if (zp.oam_pos == 0) { debug_oam_loss += count; return; }
		uint8 yt = meta[0] + y;
		if (yt < 239)
		{
			h.oam[zp.oam_pos+0] = yt;
			h.oam[zp.oam_pos+1] = meta[1];
			h.oam[zp.oam_pos+2] = meta[2] | zp.att_or;
			h.oam[zp.oam_pos+3] = meta[3] + x;
			zp.oam_pos += 4;
		}
		meta += 4;
		--count;
	}
}

static void sprite_add_edge(uint8 x, uint8 y, const uint8* meta, uint8 count)
{
	NES_ASSERT(meta, "NULL sprite!");

	while (count)
	{
		if (zp.oam_pos == 0) { debug_oam_loss += count; return; }
		uint8 yt = meta[0] + y;
		if (yt < 239)
		{
			h.oam[zp.oam_pos+0] = yt;
			h.oam[zp.oam_pos+1] = meta[1];
			h.oam[zp.oam_pos+2] = meta[2] | zp.att_or;
			h.oam[zp.oam_pos+3] = meta[3] + x;

			if (0 != ((h.oam[zp.oam_pos+3] ^ zp.sprite_center) & 0x80)) // if x bit 7 does not match, prevent tile
			{
				h.oam[zp.oam_pos+0] = 255;
			}
			else
			{
				zp.oam_pos += 4;
			}
		}

		meta += 4;
		--count;
	}
}

void sprite_add_flipped(uint8 x, uint8 y, const uint8* meta, uint8 count)
{
	NES_ASSERT(meta, "NULL sprite!");

	if (!(zp.sprite_center & 0x40))
	{
		sprite_add_flipped_edge(x,y,meta,count);
		return;
	}

	while (count)
	{
		if (zp.oam_pos == 0) { debug_oam_loss += count; return; }
		uint8 yt = meta[0] + y;
		if (yt < 239)
		{
			h.oam[zp.oam_pos+0] = yt;
			h.oam[zp.oam_pos+1] = meta[1];
			h.oam[zp.oam_pos+2] = (meta[2] ^ 0x40) | zp.att_or;
			h.oam[zp.oam_pos+3] = (x-8) - meta[3];
			zp.oam_pos += 4;
		}
		meta += 4;
		--count;
	}
}

static void sprite_add_flipped_edge(uint8 x, uint8 y, const uint8* meta, uint8 count)
{
	NES_ASSERT(meta, "NULL sprite!");

	while (count)
	{
		if (zp.oam_pos == 0) { debug_oam_loss += count; return; }
		uint8 yt = meta[0] + y;
		if (yt < 239)
		{
			h.oam[zp.oam_pos+0] = yt;
			h.oam[zp.oam_pos+1] = meta[1];
			h.oam[zp.oam_pos+2] = (meta[2] ^ 0x40) | zp.att_or;
			h.oam[zp.oam_pos+3] = (x-8) - meta[3];

			if (0 != ((h.oam[zp.oam_pos+3] ^ zp.sprite_center) & 0x80)) // if x bit 7 does not match, prevent tile
			{
				h.oam[zp.oam_pos+0] = 255;
			}
			else
			{
				zp.oam_pos += 4;
			}
		}

		meta += 4;
		--count;
	}
}

void sprite_tile_add(uint8 x, uint8 y, uint8 tile, uint8 att)
{
	if (zp.oam_pos == 0) { debug_oam_loss += 1; return; }
	h.oam[zp.oam_pos+0] = y;
	h.oam[zp.oam_pos+1] = tile;
	h.oam[zp.oam_pos+2] = att;
	h.oam[zp.oam_pos+3] = x;
	zp.oam_pos += 4;
}

void sprite_tile_add_clip(uint8 x, uint8 y, uint8 tile, uint8 att)
{
	if (x >= 64 && x < 192 && (zp.sprite_center & 0x40))
	{
		sprite_tile_add(x,y,tile,att);
		return;
	}

	if (((x ^ zp.sprite_center) & 0x80) != 0) return;
	sprite_tile_add(x,y,tile,att);
}

void sprite_finish()
{
	debug_oam_mark = zp.oam_pos / 4;
	if (debug_oam_mark == 0) debug_oam_mark = 64;

	while (zp.oam_pos != 0)
	{
		h.oam[zp.oam_pos] = 0xFF;
		++zp.oam_pos;
	}
}

// copies 0-31 bytes of nmi_update to 32-63
void nmi_update_shift()
{
	for (uint8 x=0; x<32; ++x)
	{
		stack.nmi_update[x+32] = stack.nmi_update[x];
	}
}

void nmi_double_fill(uint8 f)
{
	for (int i=0; i<64; ++i)
	{
		stack.nmi_update[i] = f;
	}
}

void nmi_double_scroll()
{
	uint16 scroll = zp.scroll_x;
	if (h.dog_type[RIVER_SLOT] == DOG_RIVER)
	{
		scroll = ((h.dog_data[3][RIVER_SLOT] & 1) << 8) + h.dog_data[2][RIVER_SLOT];
	}

	uint8 sp = scroll >> 3;
	uint8 pos = sp;
	if (sp < 32) // <32 shift is right to left
	{
		pos += 31;
		for (int i=31; i>=0; --i)
		{
			stack.nmi_update[pos] = stack.nmi_update[i];
			pos = pos - 1;
		}
		pos = sp + 32;
	}
	else // >=32 shift is left to right
	{
		for (int i=0; i<32; ++i)
		{
			stack.nmi_update[pos] = stack.nmi_update[i];
			pos = (pos+1) & 63;
		}
	}

	for (int i=0; i<32; ++i)
	{
		stack.nmi_update[pos] = 0x70;
		pos = (pos+1) & 63;
	}
}

//
// Game modes
//

void set_game_mode(uint8 mode)
{
	zp.mode_temp = 0;
	game_mode = mode;
}

uint8 get_game_mode()
{
	return game_mode;
}

// forward
void tick_time();
void tick_mode_title();
void tick_mode_start();
void tick_mode_play();
void tick_mode_river();
void tick_mode_fade_out();
void tick_mode_load();
void tick_mode_fade_in();
void tick_mode_talk();
void tick_mode_pause_draw(uint8 kill_line);
void tick_mode_pause_in();
void tick_mode_pause();
void tick_mode_pause_out();
void tick_mode_hold();
void load_mode_ending();
void tick_mode_ending();
void tick_mode_book();
void tick_mode_soundtrack();
void tick_mode_crash();

void tick_time()
{
	uint8 fps = system_pal() ? 50 : 60;

	++h.metric_time_f;
	if (h.metric_time_f >= fps)
	{
		h.metric_time_f = 0;
		++h.metric_time_s;
		if (h.metric_time_s >= 60)
		{
			h.metric_time_s = 0;
			++h.metric_time_m;
			if (h.metric_time_m >= 60)
			{
				h.metric_time_m = 0;
				++h.metric_time_h;
				if (h.metric_time_h > 99)
				{
					h.metric_time_h = 99;
					h.metric_time_m = 59;
					h.metric_time_s = 59;
					h.metric_time_f = fps - 1;
				}
			}
		}
	}
}

static bool  mode_title_initial = true;
static bool  mode_title_resume = false;
static bool  mode_title_settings = false;
static int mode_title_select_play = 0;
static int mode_title_select_setting = 0;
static int mode_title_setting_music = 0;
static int mode_title_gamepad_last = 0;

bool resume_apply(); // forward declaration

static void mode_title_arrows(int position, bool invert)
{
	NES_ASSERT(position < 2, "mode_title_select_setting invalid!");
	int apos = position * 32;
	uint8 arrow0 = text_convert('\xAB');
	uint8 arrow1 = text_convert('\xAC');
	stack.nmi_update[apos+ 9] = invert ? arrow1 : arrow0;
	stack.nmi_update[apos+22] = invert ? arrow0 : arrow1;
}

void tick_mode_title()
{
	DEBUG_MODE("title");

	const uint8 TOILET_CODE = (PAD_SELECT | PAD_B);

	if (zp.mode_temp == 0)
	{
		mode_title_settings = false;
		mode_title_select_play = 0;
		mode_title_select_setting = 0;
		mode_title_setting_music = system_music() ? 1 : 0;

		mode_title_resume = (resume.valid & system_resume()) & (!system_playback());

		mode_title_gamepad_last = zp.gamepad;
		zp.mode_temp = 1; // 1 = redraw mode
	}

	if (zp.mode_temp == 1)
	{
		// redraw
		if (!mode_title_settings)
		{
			// don't clear BETA/DEMO text for first display
			if (!mode_title_initial)
			{
				text_load(TEXT_BLANK);
				nmi_update_shift();
				nmi_update_at(0,11);
			}

			ppu_nmi_update_double();
			if (!mode_title_resume)
			{
				// NES cartridge style PUSH_START
				text_load(TEXT_BLANK);
				nmi_update_shift();
				uint8 text_start = ((zp.gamepad&TOILET_CODE)==TOILET_CODE) ? TEXT_PUSH_SHART : TEXT_PUSH_START;
				text_load(text_start);
				nmi_update_at(0,13);
				ppu_nmi_update_double();
			}
			else
			{
				// a RESUME option if you've played before
				text_load_meta(TEXT_META_NEW_GAME);
				nmi_update_shift();
				text_load_meta(TEXT_META_RESUME);
				mode_title_arrows(mode_title_select_play,true);
				nmi_update_at(0,13);
				ppu_nmi_update_double();
			}
		}
		else
		{
			// settings page
			text_load(TEXT_BLANK);
			nmi_update_shift();
			text_load(TEXT_SETTINGS);
			nmi_update_at(0,11);
			ppu_nmi_update_double();
			
			NES_ASSERT(mode_title_setting_music < 3, "mode_title_setting_music invalid");
			const Game::TextEnum LINE_MUSIC[3] = { TEXT_MUSIC_OFF, TEXT_MUSIC_ON, TEXT_MUSIC_SOUNDTRACK };
			uint8 line_music = LINE_MUSIC[mode_title_setting_music];
			uint8 line_difficulty = system_easy() ? TEXT_DIFFICULTY_EASY : TEXT_DIFFICULTY_NORMAL;
			text_load(line_music);
			nmi_update_shift();
			text_load(line_difficulty);
			nmi_update_at(0,13);
			mode_title_arrows(mode_title_select_setting,false);
			ppu_nmi_update_double();
		}
		zp.mode_temp = 2;
	}

	uint8 gamepad_new = (zp.gamepad ^ mode_title_gamepad_last) & zp.gamepad;
	mode_title_gamepad_last = zp.gamepad;
	prng(4); // tick random number generator
	
	if (gamepad_new & PAD_SELECT)
	{
		mode_title_settings = !mode_title_settings;
		mode_title_initial = false;
		zp.mode_temp = 1;
		goto render;
	}

	if (mode_title_settings)
	{
		// settings page
		if (gamepad_new & (PAD_U | PAD_D))
		{
			mode_title_select_setting ^= 1;
			zp.mode_temp = 1;
		}
		else if (gamepad_new & (PAD_L | PAD_R | PAD_A))
		{
			if (mode_title_select_setting == 0)
			{
				system_easy_set(!system_easy());
				zp.mode_temp = 1;
			}
			else if (mode_title_select_setting == 1)
			{
				mode_title_setting_music += (gamepad_new & (PAD_R | PAD_A)) ? 1 : -1;
				if (mode_title_setting_music < 0) mode_title_setting_music = 2;
				if (mode_title_setting_music > 2) mode_title_setting_music = 0;
				system_music_set(mode_title_setting_music > 0);
				zp.mode_temp = 1;
			}
		}
		else if (gamepad_new & (PAD_START | PAD_B))
		{
			if (mode_title_setting_music == 2)
			{
				goto start_soundtrack;
			}
			else
			{
				mode_title_settings = false;
				zp.mode_temp = 1;
			}
		}
	}
	else if (mode_title_resume)
	{
		if (gamepad_new & (PAD_U | PAD_D))
		{
			mode_title_select_play ^= 1;
			zp.mode_temp = 1;
		}
		else if (gamepad_new & PAD_START)
		{
			if (mode_title_select_play == 0)
			{
				goto resume_game;
			}
			else
			{
				goto start_game;
			}
		}
	}
	else
	{
		if (gamepad_new & PAD_START)
		{
			goto start_game;
		}
	}

render:
	PPU::scroll_x(zp.scroll_x);
	PPU::overlay_y(240,false);
	PPU::overlay_scroll(256,0);
	zp.nmi_ready = NMI_READY;
	return;

start_game:
	resume.valid = false;

	if (zp.current_room == DATA_room_start)
	{
		// remove PC only "PUSH ESCAPE FOR OPTIONS" text
		PPU::latch_at(0,27);
		room_load_partial(5); for (int i=32; i<64; ++i) PPU::write(stack.nmi_update[i]);
		room_load_partial(6); for (int i=32; i<64; ++i) PPU::write(stack.nmi_update[i]);
		room_load_partial(7); for (int i=32; i<64; ++i) PPU::write(stack.nmi_update[i]);

		// remove "PUSH START" text
		for (int i=0; i<64; ++i) stack.nmi_update[i] = 0x80;
		nmi_update_at(0,13);
		zp.nmi_ready = NMI_DOUBLE;
	}

	zp.easy = system_easy() ? 0xFF : 0x00;
	enable_music(system_music());

	// randomly select human palette/hair for this play
	h.human0_hair = prng(8);
	h.human1_hair = prng(8);

	redo_human0_pal:
	zp.human0_pal = prng(2) & 3;
	if (zp.human0_pal == 0) goto redo_human0_pal;
	--zp.human0_pal;

	redo_human1_pal:
	h.human1_pal = prng(2) & 3;
	if (h.human1_pal == 0) goto redo_human1_pal;
	--h.human1_pal;

	// all dead human1 set
	h.human1_set[0] = 3;
	h.human1_set[1] = 3;
	h.human1_set[2] = 3;
	h.human1_set[3] = 3;
	h.human1_set[4] = 3;
	h.human1_set[5] = 3;

	// for TAS sync, hold LEFT+RIGHT to fix PRNG
	if ((zp.gamepad & (PAD_L | PAD_R)) == (PAD_L | PAD_R))
	{
		zp.seed = 0x0101;
		zp.password[0] = 0;
		zp.password[1] = 0;
		zp.password[2] = 0;
		zp.password[3] = 0;
		zp.password[4] = 0;
		zp.nmi_count = 0;
	}

	set_game_mode(GAME_START);

	PPU::scroll_x(zp.scroll_x);
	PPU::overlay_y(240,false);
	PPU::overlay_scroll(256,0);
	//zp.nmi_ready = NMI_DOUBLE;
	return;

resume_game:
	if (resume_apply())
	{
		for (int i=0; i<8; ++i) zp.chr_cache[i] = 0xFF; // invalidate cache in case of eyesight change
		zp.dog_add = 1; // required for dogs to appear on first active frame
		set_game_mode(GAME_FADE_OUT);
	}
	else
	{
		// shouldn't happen
		// failsafe if it's an invalid state
		resume.valid = false;
		init();
	}
	return;

start_soundtrack:
	enable_music(true);
	set_game_mode(GAME_SOUNDTRACK);
	tick_mode_soundtrack();
	return;
}

void tick_mode_start()
{
	DEBUG_MODE("start");

	uint8 fade = 0x00;
	if (zp.mode_temp == 0)
	{
		sprite_begin();
		lizard_draw();
		dogs_draw();
		sprite_finish();

		for (unsigned int i=0; i<32; ++i) h.shadow_palette[i] = h.palette[i];
		fade = 0x40;
	}
	else if (zp.mode_temp == 2) fade = 0x30;
	else if (zp.mode_temp == 4) fade = 0x20;
	else if (zp.mode_temp == 6) fade = 0x10;
	else if (zp.mode_temp == 8) fade = 0x00;
	
	if (0 == (zp.mode_temp & 0x1))
	{
		for (unsigned int i=16; i<24; ++i)
		{
			uint8 p = h.shadow_palette[i] - fade;
			if (p >= 0x40) p = 0x0F;
			h.palette[i] = p;
		}
	}

	if (zp.mode_temp >= 8)
	{
		set_game_mode(GAME_PLAY);
	}
	else
		++zp.mode_temp;

	PPU::scroll_x(zp.scroll_x);
	PPU::overlay_y(240,false);
	PPU::overlay_scroll(256,0);
	zp.nmi_ready = NMI_READY;
}

void tick_easy()
{
	// not yet initialized
	if (zp.easy == 0xFF) return;

	// update if changed in options
	if (system_easy() && zp.easy == 0)
	{
		zp.easy = 1;
	}
	else if (!system_easy() && zp.easy != 0)
	{
		zp.easy = 0;
	}

	if (zp.easy ==    0) return;


	const uint8 EASY_FRAMES[2] = {
		4, // NTSC skips 1/3 frames
		6, // PAL skips 1/5 frames
	};

	++zp.easy;
	if (zp.easy >= EASY_FRAMES[system_pal() ? 1 : 0])
	{
		zp.easy = 1;
	}
}

void tick_mode_play()
{
	DEBUG_MODE("play");

	if (zp.room_change != 0)
	{
		set_game_mode(GAME_FADE_OUT);
		tick_mode_fade_out();
		return;
	}

	if (zp.easy == 1) goto skip_frame;
	if (zp.game_pause != 0)
	{
		set_game_mode(GAME_PAUSE_IN);
		tick_mode_pause_in();
		return;
	}

	if (zp.climb_assist_time > 0)
	{
		--zp.climb_assist_time;
		zp.gamepad |= zp.climb_assist;
	}
	else
	{
		zp.climb_assist = 0;
	}

	lizard_tick();
	if (zp.game_pause != 0) // if lizard requests pause, immediately do it
	{
		set_game_mode(GAME_PAUSE_IN);
		tick_mode_pause_in();
		return;
	}

	dogs_tick();

skip_frame:

	sprite_begin();
	lizard_draw();
	dogs_draw();
	sprite_finish();

	if (h.dog_type[0] == DOG_FROB)
	{
		zp.scroll_x = h.dog_data[FROB_SCREEN][0] << 8;
	}

	tick_time();
	tick_easy();

	PPU::scroll_x(zp.scroll_x);
	PPU::overlay_y(240,false);
	PPU::overlay_scroll(256,0);

	zp.nmi_ready = zp.nmi_next;
	zp.nmi_next = NMI_READY;
}

void river_draw()
{
	h.wave_draw_control &= 0x7F;

	if (h.dog_data[RIVER_OVERLAP][RIVER_SLOT] == 0)
	{
		lizard_draw();
		dogs_draw();
	}
	else
	{
		dogs_draw();
		lizard_draw();
	}

	// splash
	uint8 splash = h.dog_data[RIVER_SPLASH_TIME][RIVER_SLOT];
	uint8 flip = h.dog_data[RIVER_SPLASH_FLIP][RIVER_SLOT];
	if (splash < 16)
	{
		uint8 attrib = 0x05 | flip;;
		uint8 tile = 0x38 | (splash >> 3);
		sprite_tile_add(zp.smoke_x & 0xFF,zp.smoke_y,tile,attrib);
	}

	// draw the shadow
	if (lizard.dead != 0) return;
	uint8 dx = uint8(lizard_px);
	if (lizard_pz > 0)
	{
		sprite_tile_add(dx-7,lizard_py-3,0xC6,0x02);
		sprite_tile_add(dx-3,lizard_py-3,0xC6,0x02);
	}
}

void setup_river()
{
	// sprite 0
	h.oam[0] = 95 - 6;

	// clear 6 lines from the top of the sprite
	PPU::latch(0x12F0);
	for (int i=0; i<6; ++i) PPU::write(0);
	zp.chr_cache[4] = 0xFF; // invalidate cache to restore in the next room
}

void tick_mode_river()
{
	DEBUG_MODE("river");

	if (zp.room_change != 0)
	{
		set_game_mode(GAME_FADE_OUT);
		tick_mode_fade_out();
		return;
	}

	if (zp.easy == 1) goto skip_river_frame;
	if (zp.game_pause != 0)
	{
		set_game_mode(GAME_PAUSE_IN);
		tick_mode_pause_in();
		return;
	}

	lizard_tick();
	if (zp.game_pause != 0) // if lizard requests pause, immediately do it
	{
		set_game_mode(GAME_PAUSE_IN);
		tick_mode_pause_in();
		return;
	}

	h.dog_data[RIVER_OVERLAP][RIVER_SLOT] = 0;
	dogs_tick();

skip_river_frame:

	sprite_begin();
	river_draw();
	sprite_finish();

	zp.scroll_x =
		h.dog_data[RIVER_SCROLL_A0][RIVER_SLOT] |
		(h.dog_data[RIVER_SCROLL_A1][RIVER_SLOT] << 8);
	PPU::scroll_x(zp.scroll_x);
	PPU::overlay_y(240,false);
	PPU::overlay_scroll(256,0);

	PPU::split_x(
		h.dog_data[RIVER_SCROLL_B0][RIVER_SLOT] |
		(h.dog_data[RIVER_SCROLL_B1][RIVER_SLOT] << 8),
		96);

	tick_time();
	tick_easy();

	zp.nmi_ready = zp.nmi_next;
	zp.nmi_next = NMI_READY;
}

void tick_mode_fade_out()
{
	DEBUG_MODE("fade_out");

	uint8 fade = 0x00;
	if (zp.mode_temp == 0)
	{
		for (unsigned int i=0; i<32; ++i) h.shadow_palette[i] = h.palette[i];
		fade = 0x10;
	}
	else if (zp.mode_temp == 2) fade = 0x20;
	else if (zp.mode_temp == 4) fade = 0x30;
	else if (zp.mode_temp >= 6) fade = 0x40;

	if (0 == (zp.mode_temp & 0x1))
	{
		for (unsigned int i=0; i<32; ++i)
		{
			uint8 p = h.shadow_palette[i] - fade;
			if (p >= 0x40) p = 0x0F;
			h.palette[i] = p;
		}
	}

	if (zp.mode_temp >= 8)
	{
		set_game_mode(GAME_LOAD);
	}
	else
		++zp.mode_temp;

	// just retain scroll from previous mode
	//PPU::scroll_x(0);
	//PPU::overlay_y(240,false);
	//PPU::overlay_scroll(256,0);

	zp.nmi_ready = NMI_READY;
}

extern const uint8 HAIR_6E[128];
const uint8 HAIR_6E[128] =
{
	0x00,0x00,0x00,0x00,0x00,0x24,0x18,0x20,0x00,0x00,0x18,0x3C,0x3C,0x18,0x00,0x1C,
	0x3A,0x7F,0x6F,0x43,0x43,0x25,0x1A,0x20,0x3A,0x7F,0x7F,0x7F,0x7F,0x19,0x02,0x1C,
	0x3C,0x7E,0x7E,0x42,0x42,0x24,0x18,0x20,0x3C,0x7E,0x7E,0x7E,0x7E,0x18,0x00,0x1C,
	0x18,0x3C,0x66,0x62,0x42,0x66,0x7E,0x62,0x18,0x3C,0x7E,0x7E,0x7E,0x5A,0x66,0x5E,
	0x1E,0x3C,0x24,0x00,0x00,0x24,0x18,0x20,0x1E,0x3C,0x3C,0x3C,0x3C,0x18,0x00,0x1C,
	0x18,0x3C,0x76,0x62,0x42,0x24,0x18,0x20,0x18,0x3C,0x7E,0x7E,0x7E,0x18,0x00,0x1C,
	0x00,0x18,0x24,0x00,0x00,0x24,0x18,0x20,0x00,0x18,0x3C,0x3C,0x3C,0x18,0x00,0x1C,
	0x14,0x7A,0x24,0x42,0x00,0x24,0x18,0x20,0x14,0x7A,0x3C,0x7E,0x3C,0x18,0x00,0x1C,
};

const uint8 HAIR_69[128] =
{
	0x00,0x00,0x00,0x00,0x00,0x24,0x1C,0x30,0x00,0x00,0x18,0x3C,0x3C,0x18,0x02,0x0F,
	0x3A,0x7F,0x5F,0x47,0x43,0x25,0x1C,0x30,0x3A,0x7F,0x7F,0x7F,0x7F,0x19,0x02,0x0F,
	0x1C,0x3E,0x7F,0x43,0x02,0x24,0x1C,0x30,0x1C,0x3E,0x7F,0x7F,0x3E,0x18,0x02,0x0F,
	0x18,0x3C,0x2E,0x02,0x02,0x27,0x3D,0x30,0x18,0x3C,0x3E,0x3E,0x3E,0x1B,0x23,0x0F,
	0x1C,0x38,0x2C,0x04,0x00,0x24,0x1C,0x30,0x1C,0x38,0x3C,0x3C,0x3C,0x18,0x02,0x0F,
	0x18,0x3C,0x76,0x46,0x02,0x24,0x1C,0x30,0x18,0x3C,0x7E,0x7E,0x3E,0x18,0x02,0x0F,
	0x00,0x18,0x2C,0x04,0x00,0x24,0x1C,0x30,0x00,0x18,0x3C,0x3C,0x3C,0x18,0x02,0x0F,
	0x14,0x7A,0x2C,0x46,0x00,0x24,0x1C,0x30,0x14,0x7A,0x3C,0x7E,0x3C,0x18,0x02,0x0F,
};

void hair_override()
{
	// apply hairstyle tiles
	if (zp.chr_cache[5] == DATA_chr_lizard_dismount &&  !flag_read(FLAG_EYESIGHT))
	{
		uint8 hair = (h.human0_hair >> 5) & 7;

		const uint8* tile;
		tile = HAIR_6E + (hair << 4);
		PPU::latch(0x16E0);
		for (int i=0; i<16; ++i) PPU::write(tile[i]);

		tile = HAIR_69 + (hair << 4);
		PPU::latch(0x1690);
		for (int i=0; i<16; ++i) PPU::write(tile[i]);
	}
}

void tick_mode_load()
{
	DEBUG_MODE("load");

	// TIP NOT IN DEMO

	zp.current_lizard = zp.next_lizard;
	room_load();

	hair_override();

	// flip lizard if walked through door
	if (zp.room_change == 2)
	{
		lizard.face ^= 1;
	}
	zp.room_change = 0;
	
	if (zp.ending)
	{
		load_mode_ending();
	}
	else
	{
		sprite_begin();
		if (h.dog_type[HOLD_SLOT] != DOG_HOLD_SCREEN)
		{
			lizard_draw();
		}
		dogs_draw();
		sprite_finish();
	}

	set_game_mode(GAME_FADE_IN);
}

void tick_mode_fade_in()
{
	DEBUG_MODE("fade_in");

	uint8 fade = 0x00;
	if (zp.mode_temp == 0)
	{
		for (unsigned int i=0; i<32; ++i) h.shadow_palette[i] = h.palette[i];
		fade = 0x40;
	}

	else if (zp.mode_temp == 2) fade = 0x30;
	else if (zp.mode_temp == 4) fade = 0x20;
	else if (zp.mode_temp == 6) fade = 0x10;
	else if (zp.mode_temp == 8) fade = 0x00;

	if (0 == (zp.mode_temp & 0x1))
	{
		for (unsigned int i=0; i<32; ++i)
		{
			uint8 p = h.shadow_palette[i] - fade;
			if (p >= 0x40) p = 0x0F;
			h.palette[i] = p;
		}
	}

	if (zp.mode_temp >= 8)
	{
		if (zp.ending)
			set_game_mode(GAME_ENDING);
		else if (h.dog_type[RIVER_SLOT] == DOG_RIVER)
			set_game_mode(GAME_RIVER);
		else if (h.dog_type[HOLD_SLOT] == DOG_HOLD_SCREEN)
			set_game_mode(GAME_HOLD);
		else
			set_game_mode(GAME_PLAY);
	}
	else
		++zp.mode_temp;

	PPU::scroll_x(zp.scroll_x);
	PPU::overlay_y(240,false);
	PPU::overlay_scroll(256,0);
	zp.nmi_ready = NMI_READY;
}

void pause_rain()
{
	// simulate use of prng in pause_rain on the NES (updated in audio thread instead here)
	if (h.text_select == 2) return; // player_paused on NES but should be equivalent (player_paused belongs to audio thread here)
	if (h.dog_type[3] != DOG_RAIN_BOSS) return;
	prng();
}

void tick_mode_talk()
{
	DEBUG_MODE("talk");

	if (zp.mode_temp == 0)
	{
		text_start(zp.game_message);
	}

	bool go_to_pause = false;
	if (zp.mode_temp < (64*3))
	{
		if ((zp.mode_temp & 63) == 0)
		{
			if (h.text_more != 0)
			{
				text_continue();
				for (int x=0; x<32; ++x) stack.scratch[x] = stack.nmi_update[x];
			}
			else
			{
				zp.mode_temp = 192;
				go_to_pause = true;
				goto finish;
			}
		}

		int line = (zp.mode_temp >> 6) & 3;
		nmi_update_at(0,24+line);

		for (int x=0; x<32; ++x) stack.nmi_update[x] = stack.scratch[x];

		if (zp.current_lizard != LIZARD_OF_KNOWLEDGE &&
			lizard.dismount != 3) // prevent confound at final ending
		{
			text_confound(zp.mode_temp);
		}

		const char space = text_convert(' ');

		int s = (zp.mode_temp & 63) >> 1;
		if ((zp.mode_temp & 1) == 0 &&
			stack.nmi_update[s] != space)
		{
			play_sound(SOUND_TALK);
		}
		
		for (int i=s+1; i < 32; ++i)
		{
			stack.nmi_update[i] = space;
		}

		++zp.mode_temp;
	}
	else
	{
	finish:
		nmi_update_at(0,27);
		text_load(TEXT_UNPAUSE);
		go_to_pause = true;
	}

	nmi_double_scroll();
	tick_mode_pause_draw(22*8);
	PPU::scroll_x(zp.scroll_x);

	uint16 overlay_scroll = zp.scroll_x;
	if (h.dog_type[RIVER_SLOT] == DOG_RIVER)
	{
		overlay_scroll = ((h.dog_data[3][RIVER_SLOT] & 1) << 8) + h.dog_data[2][RIVER_SLOT];
	}
	PPU::overlay_scroll(overlay_scroll,22*8);

	zp.nmi_ready = NMI_WIDE;

	if (go_to_pause)
	{
		set_game_mode(GAME_PAUSE);
	}
}

// pause stuff

void tick_mode_pause_draw(uint8 kill_line)
{
	dogs_cycle(); // cycle draw order manually (dogs_tick is skipped)
	sprite_begin(); // initialize sprites for rendering

	// 8 hidden $2F sprites that steal sprite priority at the top of the pause overlay
	for (int i=0; i<8; ++i)
	{
		sprite_tile_add(252,kill_line-1,0x2F,0x21);
	}
	PPU::overlay_y(kill_line,false);
	PPU::overlay_scroll_y(kill_line);

	if (h.dog_type[RIVER_SLOT] == DOG_RIVER)
	{
		river_draw();
	}
	else
	{
		lizard_draw();
		dogs_draw();
	}
	sprite_finish();

	// kill sprites at or below the given line
	for (int i=1; i<64; ++i)
	{
		unsigned char & oamy = h.oam[(i*4)+0];
		if (oamy >= kill_line)
		{
			oamy = 255;
		}
	}

	pause_rain();
}

void tick_mode_pause_black_row(uint8 row, bool mid)
{
	uint8 r = row + 22;
	uint8 k = (r * 8);

	if (mid) k = 22*8;
	tick_mode_pause_draw(k);

	nmi_double_fill(0x70);
	zp.nmi_load = (r << 5) + 0x2000;
	zp.nmi_ready = NMI_WIDE;
}

void tick_mode_pause_restore_row(uint8 row)
{
	uint8 r = row + 22;
	uint8 k = ((r+1) * 8);

	tick_mode_pause_draw(k);

	//if (h.dog_type[0] != DOG_FROB)
	{
		room_load_partial(row);

		// clear valves on unpause in volcano boss
		if (h.dog_type[3] == DOG_RACCOON_LAUNCHER && row < 6)
		{
			uint8 i = 15 - (row / 2);
			NES_ASSERT(h.dog_type[i] == DOG_RACCOON_VALVE,"expected raccoon_valve in slots 13-15");
			if (h.dog_data[RACCOON_VALVE_LOCK][i] > 0)
			{
				uint8 x = ((h_dog_x[i] >> 3) + 31) & 63;
				stack.nmi_update[x+0] = 0x80;
				stack.nmi_update[x+1] = 0x80;
			}
		}
	}
	//else
	{
		// NOT IN DEMO
	}

	zp.nmi_load = (r << 5) + 0x2400;
	zp.nmi_ready = NMI_WIDE;
}

void tick_mode_pause_in()
{
	DEBUG_MODE("pause_in");

	// text_select = 0 talk, text characters display gradually with a sound
	// text_select = 1 message, text is displayed immediately without sound
	// text_select = 2 just pause (music paused, show password)
	// game_message = text or message enum to use

	switch (zp.mode_temp)
	{
		// frame 0-7: black out 8 bottom rows (bottom to top)
	case 0:
		if (h.text_select == 2) // pause
		{
			pause_audio();
		}
		// fall through
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
		tick_mode_pause_black_row(7 - zp.mode_temp, false);
		break;
		// frame 8: black out attributes
	case 8:
		for (int i=0; i<32; ++i)
		{
			stack.nmi_update[i   ] = h.att_mirror[i+32];
			stack.nmi_update[i+32] = h.att_mirror[i+96];
		}
		for (int i=0; i<8; ++i)
		{
			// 23E0/27E0 - no change
			//stack.nmi_update[i+ 0+ 8] &= 0xFF;
			//stack.nmi_update[i+32+ 8] &= 0xFF;
			// 23E8/27E8 - black bottom half
			stack.nmi_update[i+ 0+ 8] &= 0x0F;
			stack.nmi_update[i+32+ 8] &= 0x0F;
			// 23F0/27F0 - black all
			stack.nmi_update[i+ 0+16] = 0;
			stack.nmi_update[i+32+16] = 0;
			// 23F8/27F8 - black all
			stack.nmi_update[i+ 0+24] = 0;
			stack.nmi_update[i+32+24] = 0;
		}
		tick_mode_pause_draw(22*8);
		zp.nmi_load = 0x23E0;
		zp.nmi_ready = NMI_WIDE;
		break;
		// frame 9: solid line at top
	case 9:
		nmi_double_fill(0x6F);
		tick_mode_pause_draw(22*8);
		nmi_update_at(0,22);
		zp.nmi_ready = NMI_WIDE;
		break;
	case 10:
		if (h.text_select == 2) // pause
		{
			text_load(TEXT_MY_LIZARD);
		}
		else if (h.text_select == 1) // message
		{
			text_load(zp.game_message);
		}
		else // talk
		{
			text_load(TEXT_BLANK);
		}
		nmi_double_scroll();
		tick_mode_pause_draw(22*8);
		nmi_update_at(0,22+1);
		zp.nmi_ready = NMI_WIDE;
		break;
	case 11:
		if (h.text_select == 2) // pause
		{
			CT_ASSERT(TEXT_LIZARD_KNOWLEDGE == TEXT_LIZARD_KNOWLEDGE + LIZARD_OF_KNOWLEDGE, "Lizard name text out of order.");
			CT_ASSERT(TEXT_LIZARD_BOUNCE    == TEXT_LIZARD_KNOWLEDGE + LIZARD_OF_BOUNCE   , "Lizard name text out of order.");
			CT_ASSERT(TEXT_LIZARD_SWIM      == TEXT_LIZARD_KNOWLEDGE + LIZARD_OF_SWIM     , "Lizard name text out of order.");
			CT_ASSERT(TEXT_LIZARD_HEAT      == TEXT_LIZARD_KNOWLEDGE + LIZARD_OF_HEAT     , "Lizard name text out of order.");
			CT_ASSERT(TEXT_LIZARD_SURF      == TEXT_LIZARD_KNOWLEDGE + LIZARD_OF_SURF     , "Lizard name text out of order.");
			CT_ASSERT(TEXT_LIZARD_PUSH      == TEXT_LIZARD_KNOWLEDGE + LIZARD_OF_PUSH     , "Lizard name text out of order.");
			CT_ASSERT(TEXT_LIZARD_STONE     == TEXT_LIZARD_KNOWLEDGE + LIZARD_OF_STONE    , "Lizard name text out of order.");
			CT_ASSERT(TEXT_LIZARD_COFFEE    == TEXT_LIZARD_KNOWLEDGE + LIZARD_OF_COFFEE   , "Lizard name text out of order.");
			CT_ASSERT(TEXT_LIZARD_LOUNGE    == TEXT_LIZARD_KNOWLEDGE + LIZARD_OF_LOUNGE   , "Lizard name text out of order.");
			CT_ASSERT(TEXT_LIZARD_DEATH     == TEXT_LIZARD_KNOWLEDGE + LIZARD_OF_DEATH    , "Lizard name text out of order.");
			CT_ASSERT(TEXT_LIZARD_BEYOND    == TEXT_LIZARD_KNOWLEDGE + LIZARD_OF_BEYOND   , "Lizard name text out of order.");

			text_load(TEXT_LIZARD_KNOWLEDGE + zp.current_lizard);
		}
		else if (h.text_select == 1) // message
		{
			text_continue();
		}
		else // talk
		{
			text_load(TEXT_BLANK);
		}
		nmi_double_scroll();
		tick_mode_pause_draw(22*8);
		nmi_update_at(0,22+2);
		zp.nmi_ready = NMI_WIDE;
		break;
	case 12:
		if (h.text_select == 1) // message
		{
			text_continue();
		}
		else // pause/talk
		{
			text_load(TEXT_BLANK);
		}
		nmi_double_scroll();
		tick_mode_pause_draw(22*8);
		nmi_update_at(0,22+3);
		zp.nmi_ready = NMI_WIDE;
		break;
	case 13:
		if (h.text_select == 2) // pause
		{
			text_load(TEXT_PASSWORD);
			for (int i=0;i<sizeof(zp.password);++i) stack.nmi_update[13+i] += zp.password[i];
		}
		else if (h.text_select == 1) // message
		{
			text_continue();
		}
		else // talk
		{
			text_load(TEXT_BLANK);
		}
		nmi_double_scroll();
		tick_mode_pause_draw(22*8);
		nmi_update_at(0,22+4);
		zp.nmi_ready = NMI_WIDE;
		break;
	case 14:
		if (h.text_select == 0) // talk
		{
			text_load(TEXT_BLANK);
		}
		else // pause/message
		{
			text_load(TEXT_UNPAUSE);
		}
		nmi_double_scroll();
		tick_mode_pause_draw(22*8);
		nmi_update_at(0,22+5);
		zp.nmi_ready = NMI_WIDE;
		break;
	default:
		NES_ASSERT(false,"Unknown PAUSE_IN state!");
		break;
	}

	uint16 scroll = zp.scroll_x;
	if (h.dog_type[RIVER_SLOT] == DOG_RIVER)
	{
		scroll = ((h.dog_data[3][RIVER_SLOT] & 1) << 8) + h.dog_data[2][RIVER_SLOT];
	}
	PPU::overlay_scroll_x(scroll);
	//PPU::overlay_scroll_y set by tick_mode_pause_draw
	//PPU::overlay_y set by tick_mode_pause_draw

	++zp.mode_temp;
	if (zp.mode_temp >= 15)
	{
		if (h.text_select == 0) // talk
		{
			set_game_mode(GAME_TALK);
		}
		else // pause/message
		{
			set_game_mode(GAME_PAUSE);
		}
	}
}

uint8 lizard_cheat_code[9] =
{
	0, PAD_R,
	0, PAD_U,
	0, PAD_D,
	0, PAD_D, PAD_D|PAD_SELECT
};

uint8 lizard_diagnostic_code[9] =
{
	0, PAD_D,
	0, PAD_U,
	0, PAD_L,
	0, PAD_L, PAD_L|PAD_SELECT
};

void tick_mode_pause()
{
	DEBUG_MODE("pause");

	if (zp.mode_temp == 0)
	{
		zp.game_pause = 0;
		zp.m = 0; // cheat code index
		zp.n = 0; // diagnostic code index
		if (!(zp.gamepad & PAD_START)) zp.mode_temp = 1; // wait for start release
	}

	tick_mode_pause_draw(22*8);

	if (zp.mode_temp > 0)
	{
		if (zp.gamepad != lizard_cheat_code[zp.m]) // look for next step in 
		{
			++zp.m;
			if (zp.gamepad != lizard_cheat_code[zp.m])
			{
				zp.m = 0;
			}
			else if (zp.m >= 8)
			{
				h.metric_cheater = 1;
				zp.current_lizard += 1;
				if (zp.current_lizard >= (debug ? LIZARD_OF_COUNT : LIZARD_OF_COFFEE)) zp.current_lizard = 0;
				zp.next_lizard = zp.current_lizard;
				lizard.power = 0;
				play_sound(SOUND_SWITCH);
				for (int d=0; d<16; ++d)
				{
					if (h.dog_type[d] == DOG_SAVE_STONE)
					{
						h.dog_data[SAVE_STONE_ON][d] = 0;
					}
				}

				set_game_mode(GAME_PAUSE_OUT);
			}
		}

		if (zp.gamepad != lizard_diagnostic_code[zp.n] && zp.current_room != DATA_room_diagnostic)
		{
			++zp.n;
			if (zp.gamepad != lizard_diagnostic_code[zp.n])
			{
				zp.n = 0;
			}
			else if (zp.n >= 8)
			{
				h.metric_cheater = 1;
				zp.room_change = 2;
				zp.current_door = 0;
				h.diagnostic_room = zp.current_room;
				zp.current_room = DATA_room_diagnostic;
				if (h.text_select == 2) // paused
				{
					unpause_audio();
				}
				set_game_mode(GAME_FADE_OUT);
			}
		}

		if (zp.gamepad & PAD_START) // start button finishes pause
		{
			set_game_mode(GAME_PAUSE_OUT);
		}
	}

	uint16 scroll = zp.scroll_x;
	if (h.dog_type[RIVER_SLOT] == DOG_RIVER)
	{
		scroll = ((h.dog_data[3][RIVER_SLOT] & 1) << 8) + h.dog_data[2][RIVER_SLOT];
	}
	PPU::overlay_scroll_x(scroll);
	//PPU::overlay_scroll_y set by tick_mode_pause_draw
	//PPU::overlay_y set by tick_mode_pause_draw

	zp.nmi_ready = NMI_READY;
}

void tick_mode_pause_out()
{
	DEBUG_MODE("pause_out");

	switch (zp.mode_temp)
	{
		// frame 0-5: black out rows with text
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
		tick_mode_pause_black_row(5 - zp.mode_temp, true);
		break;
	case 5:
		tick_mode_pause_black_row(0, false); // can't use priority sprites anymore
		break;
		// frame 6: restore attributes
	case 6:
		for (int i=0; i<32; ++i)
		{
			stack.nmi_update[i   ] = h.att_mirror[i+32];
			stack.nmi_update[i+32] = h.att_mirror[i+96];
		}
		tick_mode_pause_draw(22*8);
		zp.nmi_load = 0x23E0;
		zp.nmi_ready = NMI_WIDE;
		break;
		// frame 7-14: restore tiles
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
		tick_mode_pause_restore_row(zp.mode_temp - 7);
		break;
	default:
		NES_ASSERT(false,"Unknown PAUSE_OUT state!");
		break;
	}

	uint16 scroll = zp.scroll_x;
	if (h.dog_type[RIVER_SLOT] == DOG_RIVER)
	{
		scroll = ((h.dog_data[3][RIVER_SLOT] & 1) << 8) + h.dog_data[2][RIVER_SLOT];
	}
	PPU::overlay_scroll_x(scroll);
	//PPU::overlay_scroll_y set by tick_mode_pause_draw
	//PPU::overlay_y set by tick_mode_pause_draw

	++zp.mode_temp;
	if (zp.mode_temp >= 15)
	{
		if (h.text_select == 2) // pause
		{
			unpause_audio();
		}
		h.text_select = 0;
		zp.game_message = 0;

		if (h.end_book != 0)
		{
			set_game_mode(GAME_BOOK);
		}
		else if (h.dog_type[RIVER_SLOT] == DOG_RIVER)
		{
			set_game_mode(GAME_RIVER);
		}
		else
		{
			set_game_mode(GAME_PLAY);
		}
	}
}

uint8 mode_hold_frame = 0;
uint8 mode_hold_seconds = 0;
uint8 mode_hold_ready = 0;

void tick_mode_hold()
{
	DEBUG_MODE("hold");

	NES_ASSERT(h.dog_type[HOLD_SLOT] == DOG_HOLD_SCREEN, "GAME_MODE_HOLD without hold_screen?");

	if (zp.mode_temp == 0)
	{
		mode_hold_frame = 0;
		mode_hold_seconds = 0;
		mode_hold_ready = 0;
		zp.mode_temp = 1;
	}

	if (zp.gamepad == 0)
	{
		mode_hold_ready = 1;
	}

	if (mode_hold_ready &&
		(zp.gamepad & (PAD_A | PAD_B | PAD_SELECT | PAD_START)) &&
		h.dog_param[HOLD_SLOT] < 255 &&
		(h.dog_type[TIP_SLOT] != DOG_TIP || zp.gamepad & PAD_START) // tip only responds to START
		)
	{
		zp.room_change = 2;
		zp.current_door = 1;
		zp.current_room = h_door_link[zp.current_door];

		if (h.dog_y[HOLD_SLOT] == 255)
		{
			zp.room_change = 1;
			zp.current_door = h.tip_return_door;
			zp.current_room = h.tip_return_room;
		}

		set_game_mode(GAME_FADE_OUT);
	}
	else if (h.dog_param[HOLD_SLOT])
	{
		++mode_hold_frame;
		if (mode_hold_frame >= 60 || (system_pal() && mode_hold_frame >= 50))
		{
			mode_hold_frame = 0;
			++mode_hold_seconds;
			if (mode_hold_seconds >= h.dog_param[HOLD_SLOT] && h.dog_param[HOLD_SLOT] < 255)
			{
				zp.room_change = 2;
				zp.current_door = 1;
				zp.current_room = h_door_link[zp.current_door];

				if (h.dog_y[HOLD_SLOT] == 255)
				{
					zp.room_change = 1;
					zp.current_door = h.tip_return_door;
					zp.current_room = h.tip_return_room;
				}

				set_game_mode(GAME_FADE_OUT);
			}
		}
	}

	if (zp.easy != 1)
	{
		dogs_tick();
	}
	
	sprite_begin();
	dogs_draw();
	sprite_finish();

	if (h.dog_param[HOLD_SLOT] == 0)
	{
		tick_time();
	}
	tick_easy();

	PPU::scroll_x(zp.scroll_x);
	PPU::overlay_y(240,false);
	PPU::overlay_scroll(256,0);

	zp.nmi_ready = zp.nmi_next;
	zp.nmi_next = NMI_READY;
}

uint8 mode_ending_scroll_sub = 0;
uint8 mode_ending_scroll = 0;
uint8 mode_ending_row = 0;

void load_mode_ending()
{
	// setup message

	mode_ending_row = 0;
	mode_ending_scroll = 0;
	mode_ending_scroll_sub = 0;
	text_start(TEXT_CREDITS);

	PPU::meta_cls();
	PPU::latch(0x2400);
	for (int i=0; i<16; ++i)
	{
		text_continue();
		ppu_nmi_write_row();
	}
	mode_ending_row = 16;

	// setup sprite s
	sprite_0_init();
	h.oam[0] = 120-4;
	sprite_begin();
	dogs_draw();
	sprite_finish();

	// setup palettes

	palette_load(4,DATA_palette_lizard0+zp.current_lizard);
	palette_load(5,DATA_palette_human0+zp.human0_pal);
	// leave 6 as in data
	palette_load(7,DATA_palette_human0+h.human1_pal);
}

void tick_mode_ending()
{
	DEBUG_MODE("ending");

	// turn off ending mode
	zp.ending = 0;

	zp.nmi_ready = NMI_READY;

	if (h.text_more)
	{
		++mode_ending_scroll_sub;
		if (mode_ending_scroll_sub >= 4)
		{
			mode_ending_scroll_sub = 0;

			if (0 == (mode_ending_scroll & 7))
			{
				text_continue();
				zp.nmi_load = 0x2400 + (32 * mode_ending_row);
				zp.nmi_ready = NMI_ROW;

				++mode_ending_row;
				if (mode_ending_row >= 30) mode_ending_row = 0;
			}

			++mode_ending_scroll;
			if (mode_ending_scroll >= 240) mode_ending_scroll = 0;
		}
	}

	PPU::scroll_x(zp.scroll_x);
	PPU::overlay_y(120,false);
	PPU::overlay_scroll(256,mode_ending_scroll);

	if (zp.gamepad == 0)
	{
		zp.mode_temp = 1; // wait for input release
	}
	else if ((zp.gamepad == PAD_START) && (zp.mode_temp != 0))
	{
		h.human1_hair = prng(8);
		redo_human1_pal:
		h.human1_pal = prng(2) & 3;
		if (h.human1_pal == 0) goto redo_human1_pal;
		--h.human1_pal;

		zp.current_lizard = LIZARD_OF_KNOWLEDGE;
		zp.next_lizard = LIZARD_OF_KNOWLEDGE;
		h.last_lizard = 0xFF;
		zp.current_room = DATA_room_start_again;
		zp.current_door = 0;
		lizard.face = 0;
		lizard.power = 0;
		zp.room_change = 1;
		set_game_mode(GAME_FADE_OUT);
	}
}

void tick_mode_book()
{
	DEBUG_MODE("book");
	NES_ASSERT(h.dog_type[BOOK_SLOT] == DOG_BOOK, "book mode without book in BOOK_SLOT?");

	if (zp.easy == 1) goto skip_frame;

	// 1. flip book to verso if needed (fades sky to black)
	// 2. flip verso page to blank bage, begin flipping
	lizard.power = 0;
	if (h.dog_param[BOOK_SLOT] == 0) // recto
	{
		h.dog_param[BOOK_SLOT] = 2; // turn to verso
	}
	else if (h.dog_param[BOOK_SLOT] == 1) // verso
	{
		h.dog_param[BOOK_SLOT] = 4; // turn to blank
		play_sound(SOUND_TWINKLE);
	}
	else if (h.dog_param[BOOK_SLOT] == 5) // flipping
	{
		++zp.mode_temp;
		if (zp.mode_temp == 0)
		{
			// begin the ending after ~20 seconds
			++h.end_book;
			if (h.end_book >= 5)
			{
				zp.room_change = 1;
				zp.current_door = 1;
				//zp.current_room = DATA_room_end_chain0; // NOT IN DEMO
				zp.current_room = DATA_room_start;

				set_game_mode(GAME_FADE_OUT);
				tick_mode_fade_out();
			}
		}
	}

	dogs_tick();
skip_frame:

	sprite_begin();

	const uint8 p = h.dog_data[BOOK_HUMAN][BOOK_SLOT];
	if (p < 64 && (prng() & 63) >= p) // random fade lizard
	{
		lizard_draw();
	}
	dogs_draw();
	sprite_finish();

	// does not tick time
	tick_easy();

	PPU::scroll_x(zp.scroll_x);
	PPU::overlay_y(240,false);
	PPU::overlay_scroll(256,0);

	zp.nmi_ready = zp.nmi_next;
	zp.nmi_next = NMI_READY;

}

uint8 mode_soundtrack_track = 0;

void  tick_mode_soundtrack()
{
	DEBUG_MODE("soundtrack");

	if (zp.mode_temp == 0)
	{
		// wipe palette, prevents 1 frame of bad colour
		PPU::latch(0x3F00);
		for (int i=0; i<32; ++i) PPU::write(0x0F);

		zp.current_room = DATA_room_soundtrack;
		room_load();
		zp.mode_temp = 1;
		mode_soundtrack_track = 0;
	}

	if (zp.gamepad == 0) // wait for release of input
	{
		zp.mode_temp = 2;
	}
	else if (zp.mode_temp == 2) // if input was released
	{
		zp.mode_temp = 1; // wait for release after this
		if (zp.gamepad & PAD_SELECT)
		{
			init();
			return;
		}
		else if (zp.gamepad & PAD_U)
		{
			if (mode_soundtrack_track >= 1)
			{
				--mode_soundtrack_track;
				play_music(mode_soundtrack_track);
			}
		}
		else if (zp.gamepad & PAD_D)
		{
			if (mode_soundtrack_track < 18)
			{
				++mode_soundtrack_track;
				play_music(mode_soundtrack_track);
			}
		}
	}

	sprite_begin();
	sprite_prepare(88);
	sprite2_add(88,64+(mode_soundtrack_track*8),DATA_sprite2_lizard_stand);
	sprite_tile_add(95,55+(mode_soundtrack_track*8),0x67,0x02);
	sprite_finish();

	PPU::scroll_x(zp.scroll_x);
	PPU::overlay_y(240,false);
	PPU::overlay_scroll(256,0);
	zp.nmi_ready = NMI_READY;
}

void crash_text(uint8 x, uint8 y, uint8 t)
{
	text_load(t);
	PPU::latch_at(x,y);
	ppu_nmi_write_row();
}

void crash_fake_hex(uint8 x, uint8 y, const char* s)
{
	PPU::latch_at(x,y);
	while (*s)
	{
		uint8 c = text_convert(*s);
		PPU::write(c);
		++s;
	}
}

void tick_mode_crash()
{
	DEBUG_MODE("crash");

	if (zp.mode_temp == 0)
	{
		// phony BSOD (this is just an imitation of the functional NES version)
		flag_clear(FLAG_EYESIGHT);
		zp.chr_cache[1] = 0xFF; // force load of font
		chr_load(1,DATA_chr_font);
		PPU::latch(0x3F00);
		for (int i=0; i<8; ++i)
		{
			PPU::write(0x01); // dark blue
			PPU::write(0x21); // light blue
			PPU::write(0x31); // lightest blue
			PPU::write(0x30); // white
		}
		PPU::latch_at(0,6);
		for (int y=18;y>0;--y)
			for (int x=32;x>0;--x)
				PPU::write(0x70);

		// these numbers to matched the NES Lizard crash from diagnostics screen at some point
		crash_text(11, 7,TEXT_CRASH_CRASH);
		crash_text( 8, 9,TEXT_CRASH_A);    crash_fake_hex(11, 9,"FE");
		crash_text( 8,10,TEXT_CRASH_X);    crash_fake_hex(11,10,"18");
		crash_text( 8,11,TEXT_CRASH_Y);    crash_fake_hex(11,11,"02");
		crash_text( 8,12,TEXT_CRASH_S);    crash_fake_hex(11,12,"F0");
		crash_text( 8,13,TEXT_CRASH_P);    crash_fake_hex(11,13,"37");
		crash_text( 8,14,TEXT_CRASH_PC);   crash_fake_hex(12,14,"BD41");
		crash_text( 8,15,TEXT_CRASH_BANK); crash_fake_hex(14,15,"0D");
		crash_fake_hex( 8,17,"6D990E03FF02BD9D");
		crash_fake_hex( 8,18,"0FA4D644DA0000");
		crash_text( 6,22,TEXT_CRASH_STOP);

		PPU::scroll_x(0);
		PPU::overlay_y(0,false); // hide sprites with overlay
		PPU::overlay_scroll(0,0);
		zp.nmi_ready = NMI_NONE;
	}

	if (zp.gamepad == 0) // wait for release of input
	{
		zp.mode_temp = 2;
	}
	else if (zp.mode_temp == 2) // if input was released
	{
		if (zp.gamepad == PAD_START)
		{
			// reset game
			init();
		}
	}
}

//
// Public functions
//

Array16 h_dog_x      [16];
Array16 h_blocker_x0 [ 4];
Array16 h_blocker_x1 [ 4];
Array16 h_door_x     [ 8];
Array16 h_door_link  [ 8];

int array16_init()
{
	Array16::generate(h_dog_x,      h.dog_x,      h.dog_xh,      16);
	Array16::generate(h_blocker_x0, h.blocker_x0, h.blocker_xh0,  4);
	Array16::generate(h_blocker_x1, h.blocker_x1, h.blocker_xh1,  4);
	Array16::generate(h_door_x,     h.door_x,     h.door_xh,      8);
	Array16::generate(h_door_link,  h.door_link,  h.door_linkh,   8);
	return 1;
}

static int array16_init_run = array16_init(); // forces initialization at startup

void init()
{
	// these are skipped by init
	uint16  hold_seed   = zp.seed;

	// ZP
	memset(&zp,0,sizeof(zp));
	memset(&lizard,0,sizeof(lizard)); // on ZP

	// Stack
	memset(&stack,0,sizeof(stack));

	// RAM
	memset(&h,0,sizeof(h));

	// Special
	set_option_mode(false);
	set_game_mode(GAME_TITLE);
	text_init();
	
	for (int i=0; i<8; ++i) zp.chr_cache[i] = 0xFF;

	uint16 start_room = DATA_room_start;
	uint8 start_door = 0;
	uint8 start_lizard = LIZARD_OF_START;
	NES_ASSERT(start_lizard < LIZARD_OF_COUNT,"LIZARD_OF_START out of range!");

	zp.current_room = start_room;
	zp.current_door = start_door;
	zp.current_lizard = start_lizard;
	zp.next_lizard = start_lizard;
	zp.easy = 0xFF; // overwritten when PUSH START
	zp.nmi_next = NMI_READY;
	h.last_lizard = 0xFF;
	h.last_lizard_save = 0xFF;
	h.tip_return_room = DATA_room_start;

	// seed it retained over reset
	zp.seed = hold_seed;
	if (zp.seed == 0) zp.seed = 1;

	// fill password from resume
	for (int i=0; i<5; ++i) zp.password[i] = resume.password[i];;
	uint16 nr;
	bool valid_password = password_read(&nr,NULL);
	if (!valid_password || !checkpoint(nr)) // invalid password data
	{
		password_build();
		resume.valid = false; // invalid password = invalid resume
	}

	ntsc_phase = rand() % 6; // randomize colour artifact simulation in TV mode

	pending_resume_save = false;

	// wipe OAM
	memset(h.oam,0xFF,sizeof(h.oam));
	sprite_0_init();

	// wipe nametables
	PPU::latch(0x2000);
	char space = text_convert(' ');
	for (int i=0; i<(1024-64); ++i) PPU::write(space);
	for (int i=0; i<64; ++i) PPU::write(0x00);
	for (int i=0; i<(1024-64); ++i) PPU::write(space);
	for (int i=0; i<64; ++i) PPU::write(0x00);

	// wipe palette
	PPU::latch(0x3F00);
	for (int i=0; i<32; ++i) PPU::write(0x0F);

	zp.current_room = DATA_room_start;
	play_music(MUSIC_SILENT);
	room_load();

	text_load(TEXT_VERSION);
	#if BETA
		stack.nmi_update[18] = hex_to_ascii[VERSION_MAJOR  ];
		stack.nmi_update[20] = hex_to_ascii[VERSION_MINOR  ];
		stack.nmi_update[22] = hex_to_ascii[VERSION_BETA   ];
		stack.nmi_update[24] = hex_to_ascii[VERSION_REVISED];
	#endif
	PPU::latch_at(0,11);
	ppu_nmi_write_row();
	text_load(TEXT_PUSH_START);
	PPU::latch_at(0,13);
	ppu_nmi_write_row();

	static bool first_run = true;
	if (first_run)
	{
		// original 3-line version
		//PPU::meta_text(32+0,27,text_get(TEXT_BLANK),0);
		//PPU::meta_text(32+0,28,text_get(TEXT_META_OPTIONS),0);
		//PPU::meta_text(32+0,29,text_get(TEXT_BLANK),0);

		// more compact 1-line options text
		PPU::meta_text(32+0,29,text_get(TEXT_META_OPTIONS),0);

		// if they've managed to reset the game, they already pressed escape
		// so maybe there's no need to show this again?
		first_run = false;
	}

	zp.nmi_ready = NMI_READY;
}

void tick(uint8 input)
{
	PPU::debug_cls();

	if (option_mode)
	{
		PPU::overlay_y(0,false);
		PPU::split_x(0,240);
		return;
	}

	zp.gamepad = input;

	switch (game_mode)
	{
		case GAME_TITLE:      tick_mode_title();      break;
		case GAME_START:      tick_mode_start();      break;
		case GAME_PLAY:       tick_mode_play();       break;
		case GAME_RIVER:      tick_mode_river();      break;
		case GAME_FADE_OUT:   tick_mode_fade_out();   break;
		case GAME_LOAD:       tick_mode_load();       break;
		case GAME_FADE_IN:    tick_mode_fade_in();    break;
		case GAME_TALK:       tick_mode_talk();       break;
		case GAME_PAUSE_IN:   tick_mode_pause_in();   break;
		case GAME_PAUSE:      tick_mode_pause();      break;
		case GAME_PAUSE_OUT:  tick_mode_pause_out();  break;
		case GAME_HOLD:       tick_mode_hold();       break;
		case GAME_ENDING:     tick_mode_ending();     break;
		case GAME_BOOK:       tick_mode_book();       break;
		case GAME_SOUNDTRACK: tick_mode_soundtrack(); break;
		case GAME_CRASH:      tick_mode_crash();      break;
		default:
			NES_ASSERT(false,"Invalid game mode!");
			init();
			break;
	}

	if (debug_dogs)
	{
		PPU::debug_text("OAM: %2d/%2d",debug_oam_mark,debug_oam_mark+debug_oam_loss);
	}

	++zp.nmi_count;
	if (zp.nmi_ready != NMI_NONE)
	{
		// update palettes
		{
			uint8* p = h.palette;
			PPU::latch(0x3F00);
			for (int i=32; i>0; --i)
			{
				PPU::write(*p);
				++p;
			}
		}

		// update nametable if requested
		if (zp.nmi_ready == NMI_ROW)
		{
			PPU::latch(zp.nmi_load);
			for (int i=0; i<32; ++i) PPU::write(stack.nmi_update[i]);
		}
		else if (zp.nmi_ready == NMI_DOUBLE)
		{
			PPU::latch(zp.nmi_load);
			for (int i=0; i<64; ++i) PPU::write(stack.nmi_update[i]);
		}
		else if (zp.nmi_ready == NMI_WIDE)
		{
			PPU::latch(zp.nmi_load);
			for (int i=0; i<32; ++i) PPU::write(stack.nmi_update[i]);
			PPU::latch(zp.nmi_load ^ 0x0400);
			for (int i=0; i<32; ++i) PPU::write(stack.nmi_update[i+32]);
		}
		else if (zp.nmi_ready == NMI_STREAM)
		{
			uint8 sp = 1;
			uint8 count = stack.nmi_update[0];

			NES_ASSERT(count <= 21, "NMI_STREAM update too long.");
			if (count > 21) count = 21; // failsafe

			for (; count>0; --count)
			{
				uint16 addr = (stack.nmi_update[sp+0] << 8) |
					           stack.nmi_update[sp+1];
				PPU::latch(addr);
				PPU::write(    stack.nmi_update[sp+2]);
				sp += 3;
			}
		}
		else if (zp.nmi_ready == NMI_OFF)
		{
			// turns of rendering on NES
			// needed to elminate write bus conflict while writing lots of data to the PPU
		}

		// upload sprites
		PPU::oam_dma(h.oam);

		zp.nmi_ready = NMI_NONE;
	}
}

void set_option_mode(bool o)
{
	option_mode = o;
}

bool get_option_mode()
{
	return option_mode;
}

bool get_suspended()
{
	return option_mode || (game_mode == GAME_PAUSE) || (game_mode == GAME_TITLE);
}

void set_debug(bool d)
{
	debug = d;
}

void set_debug_dogs(bool d)
{
	debug_dogs = d;
}

void resume_point(bool dead)
{
	// prevents starting a new game but dying before saving from producing a new restore point
	if (!dead)
	{
		resume.valid = true;
	}

	// system state
	resume.pal               = system_pal()  ? 1 : 0;
	resume.easy              = system_easy() ? 1 : 0;

	// ZP state
	resume.continued         = zp.continued;
	resume.seed              = zp.seed;
	for (int i=0; i<5; ++i) resume.password[i] = zp.password[i];
	resume.human0_pal        = zp.human0_pal;

	// RAM state
	for (int i=0; i<16; ++i) resume.coin[i] = h.coin[i];
	for (int i=0; i<16; ++i) resume.flag[i] = h.flag[i];
	resume.piggy_bank        = h.piggy_bank;
	resume.last_lizard       = h.last_lizard;
	resume.human1_pal        = h.human1_pal;
	resume.human0_hair       = h.human0_hair;
	resume.human1_hair       = h.human1_hair;
	resume.moose_text        = h.moose_text;
	resume.moose_text_inc    = h.moose_text_inc;
	resume.heep_text         = h.heep_text;
	for (int i=0; i<6; ++i) resume.human1_set[i] = h.human1_set[i];
	for (int i=0; i<6; ++i) resume.human1_het[i] = h.human1_het[i];
	resume.metric_time_h     = h.metric_time_h;
	resume.metric_time_m     = h.metric_time_m;
	resume.metric_time_s     = h.metric_time_s;
	resume.metric_time_f     = h.metric_time_f;
	resume.metric_bones      = h.metric_bones;
	resume.metric_jumps      = h.metric_jumps;
	resume.metric_continue   = h.metric_continue;
	resume.metric_cheater    = h.metric_cheater;
	resume.frogs_fractioned  = h.frogs_fractioned;
	resume.tip_index         = h.tip_index;
	resume.tip_counter       = h.tip_counter;

	// save to disk at next room transition
	pending_resume_save = true;
}

bool resume_apply()
{
	// system state
	bool pal = (resume.pal != 0);
	bool easy = (resume.easy != 0);

	// use settings for PAL/Easy, don't load them
	//system_pal_set(pal);
	//system_easy_set(easy);
	zp.easy = system_easy() ? 1 : 0;

	if (resume.metric_cheater != 0)
	{
	}

	// ZP state
	zp.continued         = resume.continued;
	zp.seed              = resume.seed;
	for (int i=0; i<5; ++i) zp.password[i] = resume.password[i];
	zp.human0_pal        = resume.human0_pal;

	// RAM state
	for (int i=0; i<16; ++i) h.coin[i] = resume.coin[i];
	for (int i=0; i<16; ++i) h.flag[i] = resume.flag[i];
	h.piggy_bank        = resume.piggy_bank;
	h.last_lizard       = resume.last_lizard;
	h.human1_pal        = resume.human1_pal;
	h.human0_hair       = resume.human0_hair;
	h.human1_hair       = resume.human1_hair;
	h.moose_text        = resume.moose_text;
	h.moose_text_inc    = resume.moose_text_inc;
	h.heep_text         = resume.heep_text;
	for (int i=0; i<6; ++i) h.human1_set[i] = resume.human1_set[i];
	for (int i=0; i<6; ++i) h.human1_het[i] = resume.human1_het[i];
	h.metric_time_h     = resume.metric_time_h;
	h.metric_time_m     = resume.metric_time_m;
	h.metric_time_s     = resume.metric_time_s;
	h.metric_time_f     = resume.metric_time_f;
	h.metric_bones      = resume.metric_bones;
	h.metric_jumps      = resume.metric_jumps;
	h.metric_continue   = resume.metric_continue;
	h.metric_cheater    = resume.metric_cheater;
	h.frogs_fractioned  = resume.frogs_fractioned;
	h.tip_index         = resume.tip_index;
	h.tip_counter       = resume.tip_counter;

	// convert time if FPS/Easy has changed
	int src_fps = resume.pal ? 50 : 60;
	if (resume.easy != 0) src_fps = 40;
	int dest_fps = system_pal() ? 50 : 60;
	if (system_easy()) dest_fps = 40;
	convert_time(src_fps, dest_fps);

	// apply and prepare for room change
	uint16 nr;
	uint8 nl;
	if (password_read(&nr,&nl))
	{
		if (checkpoint(nr))
		{
			zp.current_room = nr;
			zp.current_lizard = nl;
			zp.next_lizard = nl;

			for (int i=0; i<16; ++i) h.coin_save[i] = h.coin[i];
			for (int i=0; i<16; ++i) h.flag_save[i] = h.flag[i];
			h.piggy_bank_save = h.piggy_bank;
			h.last_lizard_save = h.last_lizard;
			zp.coin_saved = 1;
			zp.room_change = 2;

			return true;
		}
	}
	return false;
}

void convert_time(int src_fps, int dest_fps)
{
	NES_ASSERT(src_fps >= 40 && dest_fps >=40, "invalid FPS in time conversion?");
	NES_ASSERT(src_fps <= 60 && dest_fps <=60, "invalid FPS in time conversion?");

	if (src_fps == dest_fps) return;

	uint32 total_frames =
		h.metric_time_h * (60 * 60 * src_fps) +
		h.metric_time_m * (60 * src_fps) +
		h.metric_time_s * src_fps +
		h.metric_time_f;

	h.metric_time_f = total_frames % dest_fps;
	h.metric_time_s = (total_frames / dest_fps) % 60;
	h.metric_time_m = (total_frames / (dest_fps * 60)) % 60;
	h.metric_time_h = (total_frames / (dest_fps * 60 * 60));
	if (h.metric_time_h > 99)
	{
		h.metric_time_h = 99;
		h.metric_time_m = 59;
		h.metric_time_s = 59;
		h.metric_time_f = dest_fps-1;
	}
	else
	{
		NES_ASSERT(total_frames == (
			h.metric_time_h * (60 * 60 * dest_fps) +
			h.metric_time_m * (60 * dest_fps) +
			h.metric_time_s * dest_fps +
			h.metric_time_f), "time conversion broken!");
	}
}

void palette_load(unsigned int slot, unsigned int select)
{
	slot = slot & 7;
	if (select > DATA_palette_COUNT) select = 0;

	const uint8* pal = data_palette[select];
	memcpy(h.palette + (slot<<2), pal, 4);
}

void chr_load(unsigned int slot, unsigned int select)
{
	if (select >= 254) return; // skip

	slot = slot & 7;
	if (select > DATA_chr_COUNT) select = 0;

	if (zp.chr_cache[slot] == select) return; // skip
	zp.chr_cache[slot] = select;
	PPU::latch(slot << 10);

	if (
		!flag_read(FLAG_EYESIGHT) ||
		select == DATA_chr_font ||
		select == DATA_chr_metrics ||
		(select == DATA_chr_dog_b2 && // special case for 2 rooms that stick characters into dog_b2
			(zp.current_room == DATA_room_info ||
			zp.current_room == DATA_room_ice_F_single_moose
			))
		)
	{
		const uint8* chr = data_chr[select];
		for (int i=1024; i>0; --i)
		{
			PPU::write(*chr);
			++chr;
		}

		// apply translation glyphs
		if (select == DATA_chr_font)
		{
			const uint8* glyphs = text_get_glyphs();
			int count = 0;
			while (*glyphs != 0)
			{
				if (count >= 45)
				{
					NES_ASSERT(false,"text_glyphs missing terminating 0!");
					return;
				}

				uint8 tile = *glyphs;
				NES_ASSERT(tile >= 0xC0, "glyph tile out of range.");
				tile = (tile & 0x3F) | 0x40;

				PPU::latch(tile << 4);
				for (int i=1; i<9; ++i) PPU::write(glyphs[i]);
				for (int i=1; i<9; ++i) PPU::write(glyphs[i]);
				glyphs += 9;

				++count;
			}
		}
	}
	else
	{
		const uint8* chr = data_mini + (select*64);
		for (int i=64; i>0; --i)
		{
			// each byte contains 4 pixels of data, corresponding to a 2x2 tile (x2 bitplanes)
			//
			// Packed: ABCDEFGH
			//
			// Unpacked:
			//
			// AE <- bitplane 0
			// BF
			//
			// CG <- bitplane 1
			// DH

			uint8 c = *chr;
			for (int j=4; j>0; --j)
			{
				// select 1 of 4 bit pairs and duplicate bit across nibble
				uint8 cs = c & 0x88;
				cs |= cs >> 1;
				cs |= cs >> 1;
				cs |= cs >> 1;
				// write 4 rows of the same pixels to CHR tile
				PPU::write(cs);
				PPU::write(cs);
				PPU::write(cs);
				PPU::write(cs);
				c <<= 1;
			}
			++chr;
		}

		// fix sprite 0 tile $2F (don't let it be corrupted by eyesight)
		if (select == DATA_chr_lizard)
		{
			PPU::latch(0x12F0);
			for (int i=0;i<8;++i)
			{
				PPU::write(0x03);
			}
		}
	}
}

static void collide_set_row(int nmt, uint8 y, const uint8* row)
{
	uint8 pos = y * 8;

	for (int i=0; i<32; i += 4)
	{
		uint8 c = 0;

		if (nmt == 0)
		{
			if (row[i+0] & 0x80) c |= 0x01;
			if (row[i+1] & 0x80) c |= 0x02;
			if (row[i+2] & 0x80) c |= 0x04;
			if (row[i+3] & 0x80) c |= 0x08;
		}
		else
		{
			if (row[i+0] & 0x80) c |= 0x80;
			if (row[i+1] & 0x80) c |= 0x40;
			if (row[i+2] & 0x80) c |= 0x20;
			if (row[i+3] & 0x80) c |= 0x10;
		}

		h.collision[pos] |= c;
		++pos;
	}
}

// remember the last room for partial restore after pause
const unsigned int UNPACKED_SIZE = 2048 + (5 * 32);
static uint8 unpacked[UNPACKED_SIZE];

void room_load()
{
	if (pending_resume_save)
	{
		if (system_resume())
		{
			resume_save();
		}
		pending_resume_save = false;
	}

	unsigned int select = zp.current_room;
	if (select > DATA_room_COUNT) select = 0;

	const uint8* packed = data_room[select];

	unsigned int packed_pos = 0;
	unsigned int unpacked_pos = 0;
	while (packed_pos < data_room_packed_size[select] && unpacked_pos < UNPACKED_SIZE)
	{
		uint8 v = packed[packed_pos]; ++packed_pos;
		if (v != 255)
		{
			unpacked[unpacked_pos] = v; ++unpacked_pos;
		}
		else // RLE
		{
			if ((packed_pos + 2) > data_room_packed_size[select]) break; // this is an error, unpacked length does not match
			uint8 run = packed[packed_pos]; ++packed_pos;
			v = packed[packed_pos]; ++packed_pos;

			NES_ASSERT(run > 0, "Room data RLE run of 0, probable corrupt data.");

			while (run > 0 && unpacked_pos < UNPACKED_SIZE)
			{
				unpacked[unpacked_pos] = v; ++unpacked_pos;
				--run;
			}
		}
	}
	NES_ASSERT(packed_pos == data_room_packed_size[select],"Room packing size mismatch.");
	NES_ASSERT(unpacked_pos == UNPACKED_SIZE,"Room unpacking size mismatch.");

	// to assist decompression
	#define DEBUG_DECOMPRESS 0
	#if DEBUG_DECOMPRESS
		NES_DEBUG("Debug Room Unpack: %04X\n",select);
		NES_DEBUG("Offset: %02X%02X\n", unpacked[1], unpacked[2]);
		for (int pos=2;pos<UNPACKED_SIZE;pos+=32)
		{
			NES_DEBUG("Row %02X: ",pos/32);
			for (int p=0;p<32;++p)
			{
				NES_DEBUG("%02X ",unpacked[pos+p]);
			}
			NES_DEBUG("\n");
		}
	#endif

	unsigned int pos = 0;

	const uint8* nmt_block_0     = unpacked + pos;
	pos += (64 * 8);

	const uint8* room_palette    = unpacked + pos + 0;
	const uint8* room_chr        = unpacked + pos + 8;
	const uint8  room_water      = unpacked[  pos + 16];
	const uint8  room_music      = unpacked[  pos + 17];
	const uint8  room_scrolling  = unpacked[  pos + 18];
	// 13 bytes of padding       = unpacked   pos + 19;
	pos += 32;

	const uint8* room_door_x0    = unpacked + pos + 0;
	const uint8* room_door_x1    = unpacked + pos + 8;
	const uint8* room_door_link0 = unpacked + pos + 16;
	const uint8* room_door_link1 = unpacked + pos + 24;
	pos += 32;

	const uint8* room_door_y     = unpacked + pos + 0;
	// 8 bytes of padding        = unpacked + pos + 8;
	const uint8* room_dog_type   = unpacked + pos + 16;
	pos += 32;

	const uint8* room_dog_x0     = unpacked + pos + 0;
	const uint8* room_dog_x1     = unpacked + pos + 16;
	pos += 32;

	const uint8* room_dog_y      = unpacked + pos + 0;
	const uint8* room_dog_param  = unpacked + pos + 16;
	pos += 32;

	const uint8* nmt_block_1     = unpacked + pos;
	pos += (32 * 22 * 2);

	// empty collision, solid rows 30/31
	memset(h.collision+0x00,0x00,0xF0);
	memset(h.collision+0xF0,0xFF,0x10);

	// block 0

	// bottom 8 rows of nametable interleaved
	pos = 0;
	for (int j=0;j<8;++j)
	{
		PPU::latch(0x22C0 + (j * 32));
		collide_set_row(0, j+22, nmt_block_0 + pos);
		for (int i=0; i<32; ++i) { PPU::write(nmt_block_0[pos]); ++pos;}

		PPU::latch(0x26C0 + (j * 32));
		collide_set_row(1, j+22, nmt_block_0 + pos);
		for (int i=0; i<32; ++i) { PPU::write(nmt_block_0[pos]); ++pos;}
	}

	// block 1

	// left nametable, top 22 rows
	pos = 0;
	PPU::latch(0x2000);
	for (int y=0; y<22; ++y)
	{
		collide_set_row(0, y, nmt_block_1 + pos);
		for (int x=0; x<32; ++x)
		{
			PPU::write(nmt_block_1[pos]);
			++pos;
		}
	}

	// right nametable, top 22 rows
	PPU::latch(0x2400);
	for (int y=0; y<22; ++y)
	{
		collide_set_row(1, y, nmt_block_1 + pos);
		for (int x=0; x<32; ++x)
		{
			PPU::write(nmt_block_1[pos]);
			++pos;
		}
	}

	// attributes
	uint8 att_pos = 0;

	PPU::latch(0x23C0);
	for (int i=0; i<64; ++i)
	{
		PPU::write(nmt_block_1[pos]);
		h.att_mirror[att_pos] = nmt_block_1[pos];
		++att_pos;
		++pos;
	}

	PPU::latch(0x27C0);
	for (int i=0; i<64; ++i)
	{
		PPU::write(nmt_block_1[pos]);
		h.att_mirror[att_pos] = nmt_block_1[pos];
		++att_pos;
		++pos;
	}

	sprite_0_init();

	play_music(room_music);

	// reset scroll parameters
	PPU::scroll_x(zp.scroll_x);
	PPU::overlay_y(240,false);
	PPU::overlay_scroll(256,0);

	// load other GPU data
	for (int i=0;i<8;++i) palette_load(i,room_palette[i]);
	for (int i=0;i<8;++i) chr_load(i,room_chr[i]);
	
	zp.water = room_water;
	zp.room_scrolling = room_scrolling;

	// doors
	for (int d=0; d<8; ++d)
	{
		h_door_x[d] = (room_door_x0[d] << 8) | room_door_x1[d];
		h.door_y[d] = room_door_y[d];
		h_door_link[d] = (room_door_link0[d] << 8 ) | room_door_link1[d];
	}
	lizard_init(h_door_x[zp.current_door],h.door_y[zp.current_door]);

	// dogs
	// setup all dog params/types before init (some try to correlate thing in init)
	for (int d=0; d<16; ++d)
	{
		h_dog_x[d] = (room_dog_x0[d] << 8) | room_dog_x1[d];
		h.dog_y[d] = room_dog_y[d];
		h.dog_param[d] = room_dog_param[d];
		h.dog_type[d] = room_dog_type[d];
		for (int i=0; i<14; ++i) h.dog_data[i][d] = 0;
	}

	// clear blockers before dog_init (some dogs will use the blockers)
	for (int i=0; i<4; ++i)
	{
		empty_blocker(i);
	}

	zp.nmi_next = NMI_READY; // cancel any pending NMI updates
	h.boss_talk = 0; // less code to do it here than with every boss
	h.text_select = 0;

	play_bg_noise(0,0);

	// initialize dogs
	for (int d=0; d<16; ++d)
	{
		dog_init(d);
	}
	lizard_fall_test(); // dogs could set up blockers, so fall test is after

	if (zp.seed == 0)
	{
		// should never happen, but a fallback just in case to keep the PRNG alive
		zp.seed = zp.nmi_count | 0x0100;
	}

	if (h.dog_type[RIVER_SLOT] == DOG_RIVER)
	{
		setup_river();
	}

	// disable split, only mode_river/ending will re-enable it
	PPU::split_x(0,240);
}

void room_load_partial(uint8 row)
{
	NES_ASSERT(row < 8,"room_load_partial row out of range!");

	unsigned int offset = 0;
	offset += row * 64;

	for (int i=0; i<32; ++i)
	{
		stack.nmi_update[i+32] = unpacked[offset+i+ 0];
		stack.nmi_update[i+ 0] = unpacked[offset+i+32];
	}
}

//
// Collision
//

void collide_set_tile(uint8 x, uint8 y)
{
	NES_ASSERT(y < 32, "collide_set_tile off bottom of screen!");
	unsigned int cidx = (y << 3) | ((x & 31) >> 2);
	if (x < 32)
	{
		uint8 bit = 1 << (x & 3);
		h.collision[cidx & 255] |= bit;
	}
	else if (x < 64)
	{
		uint8 bit = 0x80 >> (x & 3);
		h.collision[cidx & 255] |= bit;
	}
}

void collide_clear_tile(uint8 x, uint8 y)
{
	NES_ASSERT(y < 32, "collide_clear_tile off bottom of screen!");
	unsigned int cidx = (y << 3) | ((x & 31) >> 2);
	if (x < 32)
	{
		uint8 bit = 1 << (x & 3);
		h.collision[cidx & 255] &= (bit ^ 0xFF);
	}
	else if (x < 64)
	{
		uint8 bit = 0x80 >> (x & 3);
		h.collision[cidx & 255] &= (bit ^ 0xFF);
	}
}

bool collide_tile(uint16 x, uint8 y)
{
	uint8 tx = (x & 255) >> 3;
	uint8 ty = y >> 3;
	unsigned int cidx = (ty << 3) | (tx >> 2);

	bool r = true;
	if (x & 0x100)
	{
		switch (tx & 3)
		{
		case 0: r = 0 != (h.collision[cidx&255] & 0x80); break;
		case 1: r = 0 != (h.collision[cidx&255] & 0x40); break;
		case 2: r = 0 != (h.collision[cidx&255] & 0x20); break;
		case 3: r = 0 != (h.collision[cidx&255] & 0x10); break;
		default:                                         break;
		}
	}
	else
	{
		switch (tx & 3)
		{
		case 0: r = 0 != (h.collision[cidx&255] & 0x01); break;
		case 1: r = 0 != (h.collision[cidx&255] & 0x02); break;
		case 2: r = 0 != (h.collision[cidx&255] & 0x04); break;
		case 3: r = 0 != (h.collision[cidx&255] & 0x08); break;
		default:                                         break;
		}
	}

	//NES_DEBUG("collide_tile(%d, %d) = %d\n",x,y,r);
	return r;
}

bool collide_blocker(uint8 i, uint16 x, uint8 y)
{
	bool r = false;
	if ( x >= h_blocker_x0[i] &&
	     x <= h_blocker_x1[i] &&
	     y >= h.blocker_y0[i] &&
	     y <= h.blocker_y1[i] )
	{
		r = true;
	}

	//NES_DEBUG("collide_blocker(%d, %d, %d) = %d\n",i,x,y,r);
	return r;
}

bool collide_all(uint16 x, uint8 y)
{
	if (collide_tile(x,y)) return true;
	if (collide_blocker(0,x,y)) return true;
	if (collide_blocker(1,x,y)) return true;
	if (collide_blocker(2,x,y)) return true;
	if (collide_blocker(3,x,y)) return true;
	return false;
}

uint8 collide_tile_left(uint16 x, uint8 y)
{
	uint8 shift = 0;
	if (collide_tile(x,y))
	{
		uint16 new_x = (x & ~0x7) + 8;
		shift += (new_x - x);
		x = new_x;
	}
	return shift;
}

uint8 collide_tile_right(uint16 x, uint8 y)
{
	uint8 shift = 0;
	if (collide_tile(x,y))
	{
		uint16 new_x = (x & ~0x7) - 1;
		shift += (x - new_x);
		x = new_x;
	}
	return shift;
}

uint8 collide_tile_up(uint16 x, uint8 y)
{
	uint8 shift = 0;
	if (collide_tile(x,y))
	{
		uint8 new_y = (y & ~0x7) + 8;
		shift += (new_y - y);
		y = new_y;
	}
	return shift;
}

uint8 collide_tile_down(uint16 x, uint8 y)
{
	uint8 shift = 0;
	if (collide_tile(x,y))
	{
		uint8 new_y = (y & ~0x7) - 1;
		shift += (y - new_y);
		y = new_y;
	}
	return shift;
}

uint8 collide_all_left(uint16 x, uint8 y)
{
	uint8 shift = 0;
	for (int i=0; i<4; ++i)
	{
		if (collide_blocker(i,x,y))
		{
			uint16 new_x = h_blocker_x1[i] + 1;
			shift += (new_x - x);
			x = new_x;
		}
	}
	if (collide_tile(x,y))
	{
		uint16 new_x = (x & ~0x7) + 8;
		shift += (new_x - x);
		x = new_x;
	}
	return shift;
}

uint8 collide_all_right(uint16 x, uint8 y)
{
	uint8 shift = 0;
	for (int i=0; i<4; ++i)
	{
		if (collide_blocker(i,x,y))
		{
			uint16 new_x = h_blocker_x0[i] - 1;
			shift += (x - new_x);
			x = new_x;
		}
	}
	if (collide_tile(x,y))
	{
		uint16 new_x = (x & ~0x7) - 1;
		shift += (x - new_x);
		x = new_x;
	}
	return shift;
}

uint8 collide_all_up(uint16 x, uint8 y)
{
	uint8 shift = 0;
	for (int i=0; i<4; ++i)
	{
		if (collide_blocker(i,x,y))
		{
			uint8 new_y = h.blocker_y1[i] + 1;
			shift += (new_y - y);
			y = new_y;
		}
	}
	if (collide_tile(x,y))
	{
		uint8 new_y = (y & ~0x7) + 8;
		shift += (new_y - y);
		y = new_y;
	}
	return shift;
}

uint8 collide_all_down(uint16 x, uint8 y)
{
	uint8 shift = 0;
	for (int i=0; i<4; ++i)
	{
		if (collide_blocker(i,x,y))
		{
			uint8 new_y = h.blocker_y0[i] - 1;
			shift += (y - new_y);
			y = new_y;
		}
	}
	if (collide_tile(x,y))
	{
		uint8 new_y = (y & ~0x7) - 1;
		shift += (y - new_y);
		y = new_y;
	}
	return shift;
}

//
// Password stuff
//

void password_scramble()
{
	zp.password[0] ^= 0x2;
	zp.password[1] ^= 0x1;
	zp.password[3] ^= 0x6;
	zp.password[4] ^= 0x5;
}

void password_build()
{
	// password is combination of:
	//   current_room   : 9 bits
	//   current_lizard : 4 bits
	//   parity         : 2 bits
	// packed as 5 3-bit values

	//                 bit 0                      bit 1                      bit 2
	zp.password[4] = ((zp.current_room   )&1) | ((zp.current_room>>3)&2) | ((zp.current_lizard<<2)&4) ;
	zp.password[3] = ((zp.current_room>>1)&1) | ((zp.current_room>>4)&2) | ((zp.current_lizard<<1)&4) ;
	zp.password[1] = ((zp.current_room>>2)&1) | ((zp.current_room>>5)&2) | ((zp.current_lizard   )&4) ;
	zp.password[0] = ((zp.current_room>>3)&1) | ((zp.current_room>>6)&2) | ((zp.current_lizard>>1)&4) ;
	zp.password[2] = ((zp.current_room>>8)&1)                                                         ;
	zp.password[2] |= (zp.password[4] ^ zp.password[3] ^ zp.password[1] ^ zp.password[0]) & 0x6; // parity

	password_scramble(); // scramble

	#ifdef _DEBUG
	{
		uint16 rr;
		uint8 rl;
		NES_ASSERT(password_read(&rr,&rl),"Built password is invalid.");
		NES_ASSERT(rr==zp.current_room && rl==zp.current_lizard,"Built password is corrupt.");
	}
	#endif
}

bool password_read(uint16* read_room, uint8* read_lizard)
{
	password_scramble(); // unscramble

	// check parity
	uint8 parity = (zp.password[4] ^ zp.password[3] ^ zp.password[1] ^ zp.password[0]) & 0x6;
	if ((zp.password[2] & 0x6) != parity)
	{
		password_scramble(); // rescramble
		return false;
	}

	uint16 rr;
	uint8 rl;

	rr = ((zp.password[4]&1)   ) |
	     ((zp.password[3]&1)<<1) |
	     ((zp.password[1]&1)<<2) |
	     ((zp.password[0]&1)<<3) |
	     ((zp.password[4]&2)<<3) |
	     ((zp.password[3]&2)<<4) |
	     ((zp.password[1]&2)<<5) |
	     ((zp.password[0]&2)<<6) |
	     ((zp.password[2]&1)<<8) ;

	rl = ((zp.password[4]&4)>>2) |
	     ((zp.password[3]&4)>>1) |
	     ((zp.password[1]&4)   ) |
	     ((zp.password[0]&4)<<1) ;

	password_scramble(); // rescramble

	// check ranges

	if (rr >= DATA_room_COUNT) return false; // invalid room
	if (rl >= LIZARD_OF_COUNT) return false; // invalid lizard

	if (read_room) *read_room = rr;
	if (read_lizard) *read_lizard = rl;

	return true;
}

bool checkpoint(uint16 room)
{
	for (int c=0; c < DATA_checkpoints_COUNT; ++c)
	{
		if (room == data_checkpoints[c])
		{
			return true;
		}
	}
	return false;
}

} // namespace Game

// C++ string helpers

char* string_cpy(char* dest, const char* src, unsigned int len)
{
	dest[len-1] = 0;
	return ::strncpy(dest,src,len-1);
}

char* string_cat(char* dest, const char* src, unsigned int len)
{
	dest[len-1] = 0;
	return ::strncat(dest,src,len-1);
}

char char_lower(char x)
{
	return (x >= 'A' && x <= 'Z') ?
		x - ('A' - 'a') :
		x;
}

bool string_less(const char* a, const char* b)
{
	char ca = char_lower(*a);
	char cb = char_lower(*b);
	while (ca == cb)
	{
		if (ca == 0) return  false;
		++a;
		++b;
		ca = char_lower(*a);
		cb = char_lower(*b);
	}
	return ca < cb;
}

unsigned int string_len(const char* src)
{
	return ::strlen(src);
}

int string_icmp(const char* s1, const char* s2)
{
	#ifdef __GNUC__
		return strcasecmp(s1,s2);
	#else
		return stricmp(s1,s2);
	#endif
}


// end of file
