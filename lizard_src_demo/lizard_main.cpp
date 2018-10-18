// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include "SDL.h"
#include "lizard_game.h"
#include "lizard_platform.h"
#include "lizard_config.h"
#include "lizard_options.h"
#include "lizard_ppu.h"
#include "lizard_audio.h"
#include "lizard_text.h"
#include "enums.h"
#include "assets/export/icon.h"

//
// Global
//

bool quit = false;
static bool debug_dogs = false;
static bool framerate = false;
static bool halt = false;
static bool frame_advance = false;
static bool slowdown = false;
static bool speedup = false;
static bool screenshot = false;
static bool multishot = false;
static int slowdown_count = 0;

static Uint32 fr_render = 0;
static Uint32 fr_present = 0;

const int SLOWDOWN_RATE = 4; // speed divider
const int SPEEDUP_RATE = 8; // speed multiplier

//
// SDL Renderer
//

static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* screen = NULL;
static SDL_Rect display_rect;
static int edge_x = 0;
static int edge_y = 0;

static int WINDOW_SCALE_DEFAULT = 2;
const int TV_WIDTH = (256 * 8) / 7;

static const unsigned int NES_PALETTE[64] = // RGB data
{
	// palette by shiru
	// 0x0X
	0x656665, 0x001E9C, 0x200DAC, 0x44049C,
	0x6A036F, 0x71021D, 0x651000, 0x461E00,
	0x232D00, 0x003900, 0x003C00, 0x003720,
	0x003266, 0x000000, 0x000000, 0x000000,
	// 0x1X
	0xB0B1B0, 0x0955EB, 0x473DFF, 0x7730FE,
	0xAE2CCE, 0xBC2964, 0xB43900, 0x8F4B00,
	0x635F00, 0x1B7000, 0x007700, 0x00733B,
	0x006D99, 0x000000, 0x000000, 0x110011,
	// 0x2X
	0xFFFFFF, 0x4DADFF, 0x8694FF, 0xB885FF,
	0xF17FFF, 0xFF79D4, 0xFF865F, 0xF19710,
	0xC8AB00, 0x7EBE00, 0x47C81F, 0x2BC86F,
	0x2EC4CC, 0x50514D, 0x000000, 0x220022,
	// 0x3X
	0xFFFFFF, 0xB9E5FF, 0xD0DBFF, 0xE6D5FF,
	0xFDD1FF, 0xFFCEF5, 0xFFD4C5, 0xFFDAA3,
	0xEEE290, 0xD0EB8E, 0xB9EFA5, 0xAEEFC7,
	0xAEEEEE, 0xBABCB9, 0x000000, 0x330033,
};

Uint32 render_palette[64];

Uint32 draw_buffer[256 * 256]; // 256 x 256, bottom 16 lines are not seen (for easy sprite overdraw)
Uint32 long_buffer[256 * 480];
Uint32 aux_buffer[257 * 240];
Uint32 feed_buffer[256 * 240];

void renderer_destroy();
bool renderer_init();

void renderer_scale()
{
	int rw, rh;
	SDL_GetRendererOutputSize(renderer,&rw,&rh);

	int rwm = rw;
	int rhm = rh;
	if (config.fullscreen)
	{
		rwm -= config.margin;
		rhm -= config.margin; 
		if (rwm < 256) rwm = 256;
		if (rhm < 240) rhm = 240;
	}

	// fix bad values
	if (config.scaling > 3) config.scaling = 0;
	if (config.filter > 2) config.filter = 0;

	// scaling:
	// 0 = largest available integer scaling for square pixels
	// 1 = fit to screen square pixels
	// 2 = fit to screen 4:3 aspect
	// 3 = stretch to screen

	// filter:
	// 0 = nearest neighbout
	// 1 = bilinear
	// 2 = TV

	// target width / height
	int tw = 256;
	int th = 240;
	if (config.scaling == 2 || config.filter == 2)
	{
		tw = TV_WIDTH;
		if (rwm < tw) tw = rwm; // clamp down if too small to accomodate
	}

	// determine integer scaling
	int scale = 1;
	while (scale < 32)
	{
		int new_scale = scale + 1;
		if (((tw * new_scale) > rwm) || ((th * new_scale) > rhm))
		{
			if (config.scaling == 0 && config.filter == 2)
			{
				// special case for CLEAN scaling + TV filter
				// (allow square aspect ratio as fallback)
				if (((th * new_scale) > rhm) || (256 * new_scale > rwm))
				{
					break;
				}
			}
			else
			{
				break;
			}
		}
		scale = new_scale;
	}
	int iw = tw * scale;
	int ih = th * scale;

	if (config.scaling != 0) // stretch
	{
		display_rect.x = 0;
		display_rect.y = 0;
		display_rect.w = rwm;
		display_rect.h = rhm;

		if (config.scaling != 3) // only stretch as wide as the aspect ratio allows
		{
			float ax = float(rwm) / float(tw);
			float ay = float(rhm) / float(th);
			if (ax > ay)
			{
				display_rect.w = int(ay * float(tw));
			}
			else
			{
				display_rect.h = int(ax * float(th));
			}
		}
		display_rect.x = (rw - display_rect.w) / 2;
		display_rect.y = (rh - display_rect.h) / 2;
	}
	else // integer scaling
	{
		if (iw > rwm) iw = rwm;
		if (ih > rhm) ih = rhm;

		display_rect.x = (rw - iw) / 2;
		display_rect.y = (rh - ih) / 2;
		display_rect.w = iw;
		display_rect.h = ih;
	}

	if (config.filter != 0)
	{
		// estimate number of corrupted pixels wrapping around the edge
		float sx = float(display_rect.w) / (2.0f * float(256));
		float sy = float(display_rect.h) / (2.0f * float(240));
		edge_x = int(ceilf(sx));
		edge_y = int(ceilf(sy));
	}
	else
	{
		edge_x = 0;
		edge_y = 0;
	}
}

Uint32 colour_transform(
	Uint32 c,
	float r0, float r1, float r2,
	float g0, float g1, float g2,
	float b0, float b1, float b2
	)
{
	Uint8 a = (c >> 24) & 0xFF;
	Uint8 r = (c >> 16) & 0xFF;
	Uint8 g = (c >>  8) & 0xFF;
	Uint8 b = (c >>  0) & 0xFF;

	float rf = float(r) / 255.0f;
	float gf = float(g) / 255.0f;
	float bf = float(b) / 255.0f;

	float mr = (rf * r0) + (gf * r1) + (bf * r2);
	float mg = (rf * g0) + (gf * g1) + (bf * g2);
	float mb = (rf * b0) + (gf * b1) + (bf * b2);

	rf = mr;
	gf = mg;
	bf = mb;

	if (rf < 0.0f) rf = 0.0f;
	if (gf < 0.0f) gf = 0.0f;
	if (bf < 0.0f) bf = 0.0f;

	if (rf > 1.0f) rf = 1.0f;
	if (gf > 1.0f) gf = 1.0f;
	if (bf > 1.0f) bf = 1.0f;

	r = int(rf * 255.0f);
	g = int(gf * 255.0f);
	b = int(bf * 255.0f);

	c = (a << 24) |
		(r << 16) |
		(g <<  8) |
		(b <<  0) ;

	return c;
}

static void find_display() // stores display device and window position before destroying it
{
	if (window != NULL)
	{
		int display = SDL_GetWindowDisplayIndex(window);
		if (display != config.display)
		{
			config.display = display;
			config.changed = true;
		}
	}
}

bool renderer_init_texture();

bool renderer_init()
{
retry:

	::memset(draw_buffer,0,sizeof(draw_buffer));
	::memset(long_buffer,0,sizeof(long_buffer));
	::memset(aux_buffer,0,sizeof(aux_buffer));
	::memset(feed_buffer,0,sizeof(feed_buffer));

	#ifndef _DEBUG
		const char* WINDOW_TITLE = "Lizard Demo";
	#else
		const char* WINDOW_TITLE = "Lizard Demo Debug";
	#endif

	// make sure desired display exists
	if (config.display >= SDL_GetNumVideoDisplays())
	{
		config.display = 0;
		config.changed = true;
	}

	Uint32 window_flags = SDL_WINDOW_RESIZABLE;
	int wx = SDL_WINDOWPOS_CENTERED_DISPLAY(config.display);
	int wy = SDL_WINDOWPOS_CENTERED_DISPLAY(config.display);
	int ww = ((config.scaling == 2 || config.filter == 2) ? TV_WIDTH : 256) * WINDOW_SCALE_DEFAULT;
	int wh = 240 * WINDOW_SCALE_DEFAULT;

	if (config.fullscreen)
	{
		window_flags = SDL_WINDOW_FULLSCREEN;
		
		if (config.display > 0)
		{
			// SDL does not seem to be capable of using true fullscreen on the second desktop, sadly.
			// This is a workaround, using the "FULLSCREEN_DESKTOP" mode, which is mostly OK but
			// occasionally has a stuttering display framerate (even though the game still always runs at 60Hz)?
			// This is very strange, but seems like the best solution I could come up with without fixing SDL's fullscreen setup.
			window_flags = SDL_WINDOW_FULLSCREEN_DESKTOP | SDL_WINDOW_BORDERLESS;
		}

		SDL_Rect r;
		if (0 == SDL_GetDisplayBounds(config.display, &r))
		{
			ww = r.w;
			wh = r.h;
		}
	}

	window = SDL_CreateWindow(WINDOW_TITLE,wx,wy,ww,wh,window_flags);

	if (window == NULL)
	{
		// retry in case fullscreen was the problem
		if (config.fullscreen)
		{
			NES_DEBUG("Fullscreen window creation failed, retrying as windowed.\n");
			config.fullscreen = false;
			goto retry;
		}

		alert(SDL_GetError(),"SDL window creation error!");
		return false;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

	// fallback without VSYNC (permits software rendering with no GPU)
	if (renderer == NULL)
	{
		renderer = SDL_CreateRenderer(window, -1, 0);
	}

	if (renderer == NULL)
	{
		// retry without fullscreen in case this was the problem
		if (config.fullscreen)
		{
			NES_DEBUG("Fullscreen renderer creation failed, retrying as windowed.\n");
			SDL_DestroyWindow(window);
			window = NULL;
			config.fullscreen = false;
			goto retry;
		}

		alert(SDL_GetError(),"SDL renderer creation error!");
		return false;
	}

	SDL_SetWindowMinimumSize(window,256,240);

	// set the window icon
	#ifndef __MACOSX__ // mac version uses higher-res associated .icns file
		SDL_Surface* icon_surface = SDL_CreateRGBSurfaceFrom(const_cast<Uint32*>(icon),16,16,32,16*4,0x00FF0000,0x0000FF00,0x000000FF,0xFF000000);
		SDL_SetWindowIcon(window,icon_surface);
		SDL_FreeSurface(icon_surface);
	#endif

	// hide mouse in fullscreen
	SDL_ShowCursor(config.fullscreen ? SDL_DISABLE : SDL_ENABLE);

	// clear to black
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);
	SDL_RenderPresent(renderer);

	// initialize palette
	for (int i=0; i<64; ++i)
	{
		Uint32 c = NES_PALETTE[i] | 0xFF000000; // RGB > ARGB

		if (config.protanopia)
		{
			c = colour_transform(c,
				 0.152286f,  1.052583f, -0.204868f,
				 0.114503f,  0.786281f,  0.099216f,
				-0.003882f, -0.048116f,  1.051998f );
		}
		if (config.deuteranopia)
		{
			c = colour_transform(c,
				 0.367322f,  0.860646f, -0.227968f,
				 0.280085f,  0.672501f,  0.047413f,
				-0.011820f,  0.042940f,  0.968881f );
		}
		if (config.tritanopia)
		{
			c = colour_transform(c,
				 1.255528f, -0.076749f, -0.178779f,
				-0.078411f,  0.930809f,  0.147602f,
				 0.004733f,  0.691367f,  0.303900f );
		}
		if (config.monochrome)
		{
			c = colour_transform(c,
				 0.333333f,  0.333333f,  0.333333f,
				 0.333333f,  0.333333f,  0.333333f,
				 0.333333f,  0.333333f,  0.333333f );
		}

		render_palette[i] = c;
		// NOTE: if other formats are needed it is easy to swizzle the palette here.
	}

	return renderer_init_texture();
}

bool renderer_refresh_fullscreen()
{
	find_display();
	renderer_destroy();
	return renderer_init();
}

bool renderer_init_texture()
{
	renderer_scale();

	int texw = 256;
	int texh = 240;

	if (config.filter > 0)
	{
		texh = 480; // long_buffer: double high for crisper scaling
	}

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,(config.filter > 0) ? "1" : "0");
	//SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY,"0"); // test disabled filter
	screen = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		texw, texh);
	if (screen == NULL)
	{
		alert(SDL_GetError(),"SDL texture creation error!");
		return false;
	}

	return true;
}

bool renderer_rebuild_texture()
{
	if (screen != NULL) SDL_DestroyTexture(screen);
	screen = NULL;

	return renderer_init_texture();
}

const char* get_multishot();
const char* get_timestamp();
void save_screenshot(const Uint32* buffer, const char* file_prefix);

static inline void fake_ntsc(int i, Uint32 mask)
{
	const Uint32 p  = draw_buffer[i];
	const int i0 = (i & 255) ==   0 ? i : i-1;
	const int i1 = (i & 255) == 255 ? i : i+1;
	const Uint32 p0 = draw_buffer[i0];
	const Uint32 p1 = draw_buffer[i1];

	if (//p  == 0xFFFFFFFF ||
		p0 == 0xFFFFFFFF ||
		p1 == 0xFFFFFFFF ) // exception for white
	{
		aux_buffer[i] = p;
		return;
	}

	#define NTSC_BLEND 50

	#if NTSC_BLEND == 25
	// 25% blend with neighbours (preserving masked colour)
	Uint32 p0d = (p0 >> 3) & 0x1F1F1F;
	Uint32 p1d = (p1 >> 3) & 0x1F1F1F;
	Uint32 ph  = (p  >> 1) & 0x7F7F7F;
	Uint32 pq  = (p  >> 2) & 0x3F3F3F;
	Uint32 ps  = ph + pq;
	Uint32 blend = ps + p0d + p1d;

	#elif NTSC_BLEND == 50
	// 50% blend
	Uint32 p0d = (p0 >> 2) & 0x3F3F3F;
	Uint32 p1d = (p1 >> 2) & 0x3F3F3F;
	Uint32 ph  = (p  >> 1) & 0x7F7F7F;
	Uint32 blend = ph + p0d + p1d;

	#else
	#error NTSC_BLEND must be 25 or 50.
	#endif

	Uint32 result = (mask & p) | (~mask & blend);
	aux_buffer[i] = result;
}

static const Uint32* tv_filter()
{
	// fake NTSC in aux_buffer
	int phase = Game::ntsc_phase % 6;
	
	for (int y=0; y<240; ++y)
	{
		int i = y * 256;
		int x = 0;

		// The NTSC NES produces a repeating pattern of subtle colour alterations.
		// Each pixel only contains 2/3 of a colour's signal, creating a three-phase pattern.
		// I am faking this pattern by blending some of the colours with neighbouring pixels
		// in a similar arrangement.

		switch (phase)
		{
		default:
		case 0: goto phase0;
		case 1: goto phase1;
		case 2: goto phase2;
		case 3: goto phase3;
		case 4: goto phase4;
		case 5: goto phase5;
		}

		do
		{
			phase0: fake_ntsc(i+x,0xFF0000); ++x; // blend cyan
			phase1: fake_ntsc(i+x,0x00FF00); ++x; // blend magenta
			phase2: fake_ntsc(i+x,0x0000FF); ++x; // blend yellow
		} while (x < 256);
		phase = (phase + 2) % 3;
		continue;

		do
		{
			phase3: fake_ntsc(i+x,0x00FFFF); ++x; // blend red
			phase4: fake_ntsc(i+x,0xFF00FF); ++x; // blend green
			phase5: fake_ntsc(i+x,0xFFFF00); ++x; // blend blue
		} while (x < 256);
		phase = ((phase + 2) % 3) + 3;
		continue;
	}

	for (int y=0; y<240; ++y)
	{
		for (int x=0; x<256; ++x)
		{
			Uint32 p = aux_buffer[(y*256)+x];

			Uint32 ph = (p >> 1) & 0x7F7F7F;
			Uint32 pq = (p >> 2) & 0x3F3F3F;

			Uint32 r = p & 0xFF0000;
			Uint32 g = p & 0x00FF00;
			Uint32 b = p & 0x0000FF;

			r = r + ((r >> 1) & 0xFF0000); if (r > 0xFF0000) r = 0xFF0000;
			g = g + ((g >> 1) & 0x00FF00); if (g > 0x00FF00) g = 0x00FF00;
			b = b + ((b >> 1) & 0x0000FF); if (b > 0x0000FF) b = 0x0000FF;

			Uint32 pbright = r | g | b;

			// "scanlines" alternate 150% bright (clamped) with 75% dark
			long_buffer[(((y*2)+0)*256)+x] = (pbright) | 0xFF000000;
			long_buffer[(((y*2)+1)*256)+x] = (ph + pq) | 0xFF000000;
			
			// flat
			//long_buffer[(((y*2)+0)*256)+x] = p | 0xFF000000;
			//long_buffer[(((y*2)+1)*256)+x] = p | 0xFF000000;
		}
	}

	return long_buffer;
}

static const Uint32* crisp_filter()
{
	// 2x nearest neighbour vertical stretch before linear stretching to the rest
	for (int y=0; y<240; ++y)
	{
		for (int x=0; x<256; ++x)
		{
			Uint32 p = draw_buffer[(y*256)+x];
			long_buffer[(((y*2)+0)*256)+x] = p;
			long_buffer[(((y*2)+1)*256)+x] = p;
		}
	}

	return long_buffer;
}

static const Uint32* no_filter()
{
	return draw_buffer;
}

void renderer_draw(const uint8* buffer, int ticks)
{
	Uint32* d = draw_buffer;
	const uint8* b = buffer;

	for (int i=(256*240); i!=0; --i)
	{
		*d = render_palette[*b];
		++d;
		++b;
	}

	if (multishot && ticks > 0)
	{
		save_screenshot(draw_buffer,get_multishot());
	}

	if (screenshot)
	{
		screenshot = false;
		save_screenshot(draw_buffer,get_timestamp());
		for (int i=(256*240); i!=0; --i) // visually mark the screenshot by inverting the color
		{
			draw_buffer[i] ^= 0x00FFFFFF;
		}
	}

	PPU::debug_overlay(draw_buffer);

	const Uint32* blit_buffer = NULL;
	switch (config.filter)
	{
		default:
		case 0:
			blit_buffer = no_filter();
			break;
		case 1:
			blit_buffer = crisp_filter();
			break;
		case 2:
			blit_buffer = tv_filter();
			break;
	}

	fr_render = SDL_GetTicks(); // measure time spent in software render

	// the vsync block either occurs on SDL_UpdateTexture or SDL_RenderPresent
	// (fullscreen mode seems to do it on present, but windowed or fullscreen desktop do it on update texture?)

	SDL_UpdateTexture(screen,NULL,blit_buffer,256 * 4);

	SDL_SetRenderDrawColor(renderer,config.border_r,config.border_g,config.border_b,255);
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer,screen,NULL,&display_rect);

	// SDL does not support texture clamping, so I have to trim edges of the screen if using bilinear filtering
	if (config.filter != 0)
	{
		SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);
		SDL_SetRenderDrawColor(renderer,config.border_r,config.border_g,config.border_b,255);
		//SDL_SetRenderDrawColor(renderer,255,0,0,255); // test

		const int dx0 = display_rect.x;
		const int dy0 = display_rect.y;
		const int dx1 = display_rect.x + display_rect.w;
		const int dy1 = display_rect.y + display_rect.h;
		const int dw = display_rect.w;
		const int dh = display_rect.h;

		if (edge_x)
		{
			const SDL_Rect edge0 = { dx0, dy0, edge_x, dh };
			SDL_RenderFillRect(renderer,&edge0);

			const SDL_Rect edge1 = { dx1 - edge_x, dy0, edge_x, dh };
			SDL_RenderFillRect(renderer,&edge1);
		}

		if (edge_y)
		{
			const SDL_Rect edge0 = { dx0, dy0, dw, edge_y };
			SDL_RenderFillRect(renderer,&edge0);

			const SDL_Rect edge1 = { dx0, dy1 - edge_y, dw, edge_y };
			SDL_RenderFillRect(renderer,&edge1);
		}
	}

	SDL_RenderPresent(renderer);
	fr_present = SDL_GetTicks(); // measure time waiting for present and issuing hardware draw
}

void renderer_destroy()
{
	if (screen   != NULL) SDL_DestroyTexture(screen);
	if (renderer != NULL) SDL_DestroyRenderer(renderer);
	if (window   != NULL) SDL_DestroyWindow(window);
	screen = NULL;
	renderer = NULL;
	window = NULL;
}

//
// Audio
//

#define AUDIO_DUMP 0

#if AUDIO_DUMP
FILE * audio_dump_file = NULL;
uint32 audio_dump_bytes = 0;
#endif

bool audio_enabled = false;

extern void play_render(sint16* buffer, int len); // from lizard_audio.cpp

void sdl_audio_callback(void* userdata, Uint8* stream, int len)
{
	const int samples = len / (AUDIO_CHANNELS * 2);
	play_render((Sint16*)stream,samples);

	#if AUDIO_DUMP
		if (audio_dump_file)
		{
			::fwrite(stream,1,len,audio_dump_file);
			audio_dump_bytes += len;
		}
	#endif
}

bool audio_init()
{
	NES_ASSERT(audio_enabled == false, "audio_init called twice?");

	if (config.audio_latency < 0)
	{
		NES_DEBUG("Audio disabled by config audio_latency<0.\n");
		return false;
	}
	else if (config.audio_latency > 8)
	{
		config.audio_latency = 8;
		config.changed = true;
	}

	if (config.audio_samplerate != 44100 && config.audio_samplerate != 48000)
	{
		config.audio_samplerate = 48000;
		config.changed = true;
	}
	if (config.audio_volume < 0)
	{
		config.audio_volume = 0;
		config.changed = true;
	}
	else if (config.audio_volume > 100)
	{
		config.audio_volume = 100;
		config.changed = true;
	}

	unsigned int buffer_samples = 1 << (config.audio_latency + 7);
	NES_DEBUG("Audio samplerate: %d, buffer: %d, volume: %d\n",config.audio_samplerate,buffer_samples,config.audio_volume);

	play_samplerate(config.audio_samplerate);
	play_volume(config.audio_volume);

	SDL_AudioSpec audio_spec;
	memset(&audio_spec,0,sizeof(audio_spec));
	audio_spec.freq = config.audio_samplerate;
	audio_spec.format = AUDIO_S16;
	audio_spec.channels = AUDIO_CHANNELS;
	audio_spec.silence = 0;
	audio_spec.samples = buffer_samples;
	audio_spec.size = audio_spec.samples * 2 * audio_spec.channels;
	audio_spec.callback = sdl_audio_callback;
	audio_spec.userdata = NULL;

	#if AUDIO_DUMP
		audio_dump_file = ::fopen("audio.wav","wb");
		audio_dump_bytes = 0;
		if (audio_dump_file)
		{
			const uint32 wav_header[11] = {
				('R'<<0)|('I'<<8)|('F'<<16)|('F'<<24),
				36,
				('W'<<0)|('A'<<8)|('V'<<16)|('E'<<24),
				('f'<<0)|('m'<<8)|('t'<<16)|(' '<<24),
				16,
				(1<<0) | (audio_spec.channels<<16),
				audio_spec.freq,
				audio_spec.freq * audio_spec.channels * 2,
				((audio_spec.channels * 2)<<0) | (16<<16),
				('d'<<0)|('a'<<8)|('t'<<16)|('a'<<24),
				0 };
			::fwrite(wav_header,4,11,audio_dump_file);
		}
	#endif

	if ( 0 != SDL_OpenAudio(&audio_spec,NULL))
	{
		NES_DEBUG("SDL audio creation error: ");
		NES_DEBUG(SDL_GetError());
		NES_DEBUG("\n");

		// NOTE: not reporting error, just let user play in silence
		audio_enabled = false;
		return false;
	}

	SDL_PauseAudio(0);
	audio_enabled = true;
	return true;
}

void audio_shutdown()
{
	if (audio_enabled)
	{
		SDL_PauseAudio(1);
		SDL_CloseAudio();
		audio_enabled = false;
	}

	#if AUDIO_DUMP
		if (audio_dump_file)
		{
			::fseek(audio_dump_file,40,SEEK_SET);
			::fwrite(&audio_dump_bytes,4,1,audio_dump_file);
			::fclose(audio_dump_file);
			audio_dump_file = NULL;
		}
	#endif
}

void system_audio_lock()
{
	if (audio_enabled)
	{
		SDL_LockAudio();
	}
}

void system_audio_unlock()
{
	if (audio_enabled)
	{
		SDL_UnlockAudio();
	}
}

//
// Input
//

const unsigned int MAX_JOYS = 16;
unsigned int input_joy_count = 0;
SDL_Joystick* input_joy[MAX_JOYS];

bool input_init()
{
	SDL_JoystickEventState(SDL_ENABLE);
	input_joy_count = 0;
	unsigned int found = SDL_NumJoysticks();
	NES_DEBUG("SDL joysticks found: %d\n", found);

	for (unsigned int i=0; i<found; ++i)
	{
		if (input_joy_count >= MAX_JOYS)
		{
			NES_DEBUG("More joysticks found than mappable. (Max %d)\n",MAX_JOYS);
			break; // can't map any more joysticks!
		}

		SDL_Joystick* joy = SDL_JoystickOpen(i);
		if (joy != NULL)
		{
			input_joy[input_joy_count] = joy;
			++input_joy_count;
		}
		else
		{
			NES_DEBUG("Unable to open SDL joystick %d: %s\n",i,SDL_GetError());
		}
	}

	return true; // NOTE: no error conditions
}

uint8 input_poll_joydir(int device, int axis_x, int axis_y, int hat)
{
	uint8 nes_dir = 0;

	for (unsigned int i=0; i<input_joy_count; ++i)
	{
		if (device >= 0 && (i != device)) continue;

		SDL_Joystick* const joy = input_joy[i];

		if (hat >= 0 && SDL_JoystickNumHats(joy) > hat)
		{
			Uint8 v = SDL_JoystickGetHat(joy,hat);
			if (v & SDL_HAT_UP    ) nes_dir |= Game::PAD_U;
			if (v & SDL_HAT_DOWN  ) nes_dir |= Game::PAD_D;
			if (v & SDL_HAT_LEFT  ) nes_dir |= Game::PAD_L;
			if (v & SDL_HAT_RIGHT ) nes_dir |= Game::PAD_R;
		}

		int axcount = SDL_JoystickNumAxes(joy);

		if (axis_x >= 0 && axcount > axis_x)
		{
			Sint16 ax = SDL_JoystickGetAxis(joy,axis_x);
			if (ax < -16384) nes_dir |= Game::PAD_L;
			if (ax >  16384) nes_dir |= Game::PAD_R;
		}

		if (axis_y >= 0 && axcount > axis_y)
		{
			Sint16 ay = SDL_JoystickGetAxis(joy,axis_y);
			if (ay < -16384) nes_dir |= Game::PAD_U;
			if (ay >  16384) nes_dir |= Game::PAD_D;
		}
	}

	return nes_dir;
}

uint8 input_poll()
{
	uint8 nes_input = 0;

	int num_keys;
	const Uint8* key_state = SDL_GetKeyboardState(&num_keys);

	if (key_state[config.keymap[GP_A     ]]) nes_input |= Game::PAD_A     ;
	if (key_state[config.keymap[GP_B     ]]) nes_input |= Game::PAD_B     ;
	if (key_state[config.keymap[GP_SELECT]]) nes_input |= Game::PAD_SELECT;
	if (key_state[config.keymap[GP_START ]]) nes_input |= Game::PAD_START ;
	if (key_state[config.keymap[GP_UP    ]]) nes_input |= Game::PAD_U     ;
	if (key_state[config.keymap[GP_DOWN  ]]) nes_input |= Game::PAD_D     ;
	if (key_state[config.keymap[GP_LEFT  ]]) nes_input |= Game::PAD_L     ;
	if (key_state[config.keymap[GP_RIGHT ]]) nes_input |= Game::PAD_R     ;

	if (input_joy_count == 0 || config.joy_device == 255) return nes_input; // no joystick

	unsigned int joy_start = 0;
	unsigned int joy_end = input_joy_count;

	if (config.joy_device >= 0)
	{
		joy_start = config.joy_device;
		joy_end   = joy_start + 1;

		if (joy_start >= input_joy_count) return nes_input; // no joystick selected
	}

	for (unsigned int i=0; i<joy_end; ++i)
	{
		SDL_Joystick* const joy = input_joy[i];
		const int num_buttons = SDL_JoystickNumButtons(joy);
		for (unsigned int j=0; j<9; ++j)
		{
			const uint8 INPUT_MAP[9] = {
				Game::PAD_A,
				Game::PAD_B,
				Game::PAD_SELECT,
				Game::PAD_START,
				Game::PAD_U,
				Game::PAD_D,
				Game::PAD_L,
				Game::PAD_R,
				Game::PAD_A | Game::PAD_B | Game::PAD_SELECT | Game::PAD_START,
			};

			Uint8 button = config.joymap[j];
			if (num_buttons > button && SDL_JoystickGetButton(joy,button)) nes_input |= INPUT_MAP[j];
		}
	}

	nes_input |= input_poll_joydir(config.joy_device, config.axis_x, config.axis_y, config.hat);

	return nes_input;
}

void input_shutdown()
{
	SDL_JoystickEventState(SDL_IGNORE);
	for (unsigned int i=0; i<input_joy_count; ++i)
	{
		SDL_JoystickClose(input_joy[i]);
	}
	input_joy_count = 0;
}

void input_refresh()
{
	input_shutdown();
	input_init();
}

void input_keydown(const SDL_Keysym& key, bool repeat)
{
	if (Game::get_option_mode())
	{
		options_keydown(key,repeat);
		return;
	}

	if (repeat)
	{
		return;
	}

	if      (key.sym == SDLK_ESCAPE) options_on();
	else if (key.sym == SDLK_RETURN && (key.mod & KMOD_ALT))
	{
		config.fullscreen = !config.fullscreen;
		config.changed = true;
		if(!renderer_refresh_fullscreen()) quit = true;
	}

	if (config.debug)
	{
		if (key.sym == SDLK_c)     PPU::cycle_debug();
		if (key.sym == SDLK_v)     PPU::cycle_debug_sprites();
		if (key.sym == SDLK_g)     PPU::cycle_debug_collide();
		if (key.sym == SDLK_h)     PPU::cycle_debug_box();
		if (key.sym == SDLK_d)     { debug_dogs = !debug_dogs; Game::set_debug_dogs(debug_dogs); }
		if (key.sym == SDLK_f)     framerate = !framerate; 
		if (key.sym == SDLK_w)     halt = !halt;
		if (key.sym == SDLK_PAUSE) halt = !halt;
		if (key.sym == SDLK_e)     frame_advance = true;
		if (key.sym == SDLK_q)     { slowdown = !slowdown; halt = false; }
		if (key.sym == SDLK_TAB)   { speedup = !speedup; halt = false; }
		if (key.sym == SDLK_F12)   screenshot = true;
		if (key.sym == SDLK_F10)   multishot = !multishot;
		if (key.sym == SDLK_F11)   { screenshot = true; frame_advance = true; }
	}
}

unsigned int system_input_joy_count()
{
	return input_joy_count;
}

//
// Resume
//

const int RESUME_BUFFER_SIZE = 84;
uint8 resume_buffer[RESUME_BUFFER_SIZE] = {0};
uint8 resume_buffer_pos = 0;

const uint8 RESUME_VALID = 0x43; // special value to mark valid resume state

void resume_w8(uint8 v)
{
	resume_buffer[resume_buffer_pos] = v;
	++resume_buffer_pos;
	NES_ASSERT(resume_buffer_pos <= RESUME_BUFFER_SIZE, "RESUME_BUFFER_SIZE too low!");
}

void resume_w16(uint16 v)
{
	resume_buffer[resume_buffer_pos+0] = (v >> 0) & 0xFF;
	resume_buffer[resume_buffer_pos+1] = (v >> 8) & 0xFF;
	resume_buffer_pos += 2;
	NES_ASSERT(resume_buffer_pos <= RESUME_BUFFER_SIZE, "RESUME_BUFFER_SIZE too low!");
}

void resume_w32(uint32 v)
{
	resume_buffer[resume_buffer_pos+0] = (v >>  0) & 0xFF;
	resume_buffer[resume_buffer_pos+1] = (v >>  8) & 0xFF;
	resume_buffer[resume_buffer_pos+2] = (v >> 16) & 0xFF;
	resume_buffer[resume_buffer_pos+3] = (v >> 24) & 0xFF;
	resume_buffer_pos += 4;
	NES_ASSERT(resume_buffer_pos <= RESUME_BUFFER_SIZE, "RESUME_BUFFER_SIZE too low!");
}

void resume_r8(uint8 &v)
{
	v = resume_buffer[resume_buffer_pos];
	++resume_buffer_pos;
	NES_ASSERT(resume_buffer_pos <= RESUME_BUFFER_SIZE, "RESUME_BUFFER_SIZE too low!");
}

void resume_r16(uint16 &v)
{
	v  = resume_buffer[resume_buffer_pos+0] << 0;
	v |= resume_buffer[resume_buffer_pos+1] << 8;
	resume_buffer_pos += 2;
	NES_ASSERT(resume_buffer_pos <= RESUME_BUFFER_SIZE, "RESUME_BUFFER_SIZE too low!");
}

void resume_r32(uint32 &v)
{
	v  = resume_buffer[resume_buffer_pos+0] <<  0;
	v |= resume_buffer[resume_buffer_pos+1] <<  8;
	v |= resume_buffer[resume_buffer_pos+2] << 16;
	v |= resume_buffer[resume_buffer_pos+3] << 24;
	resume_buffer_pos += 4;
	NES_ASSERT(resume_buffer_pos <= RESUME_BUFFER_SIZE, "RESUME_BUFFER_SIZE too low!");
}

void resume_pack()
{
	resume_buffer_pos = 0;
	// password packed first because it is exempt from validity check
	for (int i=0; i<5; ++i) resume_w8(Game::resume.password[i]);
	resume_w8(Game::resume.valid ? RESUME_VALID : 0);
	resume_w8(Game::resume.pal);
	resume_w8(Game::resume.easy);
	resume_w8(Game::resume.continued);
	resume_w16(Game::resume.seed);
	resume_w8(Game::resume.human0_pal);
	for (int i=0; i<16; ++i) resume_w8(Game::resume.coin[i]);
	for (int i=0; i<16; ++i) resume_w8(Game::resume.flag[i]);
	resume_w8(Game::resume.piggy_bank);
	resume_w8(Game::resume.last_lizard);
	resume_w8(Game::resume.human1_pal);
	resume_w8(Game::resume.human0_hair);
	resume_w8(Game::resume.human1_hair);
	resume_w8(Game::resume.moose_text);
	resume_w8(Game::resume.moose_text_inc);
	resume_w8(Game::resume.heep_text);
	for (int i=0; i<6; ++i) resume_w8(Game::resume.human1_set[i]);
	for (int i=0; i<6; ++i) resume_w8(Game::resume.human1_het[i]);
	resume_w8(Game::resume.metric_time_h);
	resume_w8(Game::resume.metric_time_m);
	resume_w8(Game::resume.metric_time_s);
	resume_w8(Game::resume.metric_time_f);
	resume_w32(Game::resume.metric_bones);
	resume_w32(Game::resume.metric_jumps);
	resume_w32(Game::resume.metric_continue);
	resume_w8(Game::resume.metric_cheater);
	resume_w8(Game::resume.frogs_fractioned);
	resume_w8(Game::resume.tip_index);
	resume_w8(Game::resume.tip_counter);
	NES_ASSERT(resume_buffer_pos == RESUME_BUFFER_SIZE, "RESUME_BUFFER_SIZE is wrong");
}

void resume_unpack()
{
	memset(&Game::resume, 0, sizeof(Game::resume));
	uint8 resume_valid = 0;

	resume_buffer_pos = 0;
	for (int i=0; i<5; ++i) resume_r8(Game::resume.password[i]);
	resume_r8(resume_valid);
	resume_r8(Game::resume.pal);
	resume_r8(Game::resume.easy);
	resume_r8(Game::resume.continued);
	resume_r16(Game::resume.seed);
	resume_r8(Game::resume.human0_pal);
	for (int i=0; i<16; ++i) resume_r8(Game::resume.coin[i]);
	for (int i=0; i<16; ++i) resume_r8(Game::resume.flag[i]);
	resume_r8(Game::resume.piggy_bank);
	resume_r8(Game::resume.last_lizard);
	resume_r8(Game::resume.human1_pal);
	resume_r8(Game::resume.human0_hair);
	resume_r8(Game::resume.human1_hair);
	resume_r8(Game::resume.moose_text);
	resume_r8(Game::resume.moose_text_inc);
	resume_r8(Game::resume.heep_text);
	for (int i=0; i<6; ++i) resume_r8(Game::resume.human1_set[i]);
	for (int i=0; i<6; ++i) resume_r8(Game::resume.human1_het[i]);
	resume_r8(Game::resume.metric_time_h);
	resume_r8(Game::resume.metric_time_m);
	resume_r8(Game::resume.metric_time_s);
	resume_r8(Game::resume.metric_time_f);
	resume_r32(Game::resume.metric_bones);
	resume_r32(Game::resume.metric_jumps);
	resume_r32(Game::resume.metric_continue);
	resume_r8(Game::resume.metric_cheater);
	resume_r8(Game::resume.frogs_fractioned);
	resume_r8(Game::resume.tip_index);
	resume_r8(Game::resume.tip_counter);
	NES_ASSERT(resume_buffer_pos == RESUME_BUFFER_SIZE, "RESUME_BUFFER_SIZE is wrong");

	Game::resume.valid = (resume_valid == RESUME_VALID);

	//  correct out of range values that could cause problems
	for (int i=0; i<5; ++i)
	{
		if (Game::resume.password[i] >= 8) Game::resume.password[i] = 0;
	}
	if (Game::resume.last_lizard >= Game::LIZARD_OF_COUNT) Game::resume.last_lizard = 0xFF;
}

void resume_load()
{
	return; // disabled for demo

	#if 0
	memset(&Game::resume, 0, sizeof(Game::resume));

	FILE *f;
	f = fopen(get_sav_file(),"rb");
	if (f == NULL) return;

	memset(resume_buffer, 0, sizeof(resume_buffer));
	fread(resume_buffer, 1, RESUME_BUFFER_SIZE, f);
	fclose(f);

	resume_unpack();
	#endif
}

void resume_save()
{
	return; // disabled for demo

	#if 0
	FILE* f;
	f = fopen(get_sav_file(),"wb");
	if (f != NULL)
	{
		resume_pack();
		fwrite(resume_buffer, 1, RESUME_BUFFER_SIZE, f);
		fclose(f);
		platform_saved();
	}
	#endif
}

//
// Screenshot
//

static char timestamp_filename[256];
static time_t timestamp_last = 0;
static int timestamp_sub = 0;
static int multishot_count = 0;

const char* get_multishot()
{
	::sprintf(timestamp_filename,"%04d",multishot_count);
	++multishot_count;
	return timestamp_filename;
}

const char* get_timestamp()
{
	time_t t = ::time(NULL);
	tm* ut = ::localtime(&t);

	NES_ASSERT(::difftime(1,0) == 1.0,"time_t does not represent seconds.");
	if (t == timestamp_last)
	{
		++timestamp_sub;
	}
	else
	{
		timestamp_last = t;
		timestamp_sub = 0;
	}
	
	::strftime(timestamp_filename,sizeof(timestamp_filename),"%Y_%m_%d__%H_%M_%S__",ut);
	::sprintf(timestamp_filename + string_len(timestamp_filename), "%02d",timestamp_sub);
	return timestamp_filename;
}

void save_screenshot(const Uint32* buffer, const char* file_prefix)
{
	#pragma pack(push,1)
	struct TGAHeader {
		uint8 idlength;
		uint8 colormaptype;
		uint8 datatypecode;
		uint16 colormaporigin;
		uint16 colormaplength;
		uint8 colormapdepth;
		uint16 x_origin;
		uint16 y_origin;
		uint16 width;
		uint16 height;
		uint8 bitsperpixel;
		uint8 imagedescriptor;
	} tga_header;
	#pragma pack(pop)
	CT_ASSERT(sizeof(tga_header) == 18, "TGAHeader should be 18 bytes in size.");
	tga_header.idlength = 0; // no id field
	tga_header.colormaptype = 0; // no palette
	tga_header.datatypecode = 2; // RGB
	tga_header.colormaporigin = SDL_SwapLE16(0);
	tga_header.colormaplength = SDL_SwapLE16(0);
	tga_header.colormapdepth = 0;
	tga_header.x_origin = SDL_SwapLE16(0);
	tga_header.y_origin = SDL_SwapLE16(0);
	tga_header.width = SDL_SwapLE16(256);
	tga_header.height = SDL_SwapLE16(240);
	tga_header.bitsperpixel = 24;
	tga_header.imagedescriptor = 0x20; // top-left origin

	char screenshot_path[PATH_LEN];
	string_cat(screenshot_path,file_prefix,PATH_LEN);
	string_cat(screenshot_path,".tga",PATH_LEN);

	FILE *f;
	f = fopen(screenshot_path,"wb");
	if (f != NULL)
	{
		fwrite(&tga_header,sizeof(tga_header),1,f);
		for (int i=0; i<(256*240); ++i)
		{
			fputc((buffer[i] >> 0 )& 0xFF,f); // B
			fputc((buffer[i] >> 8 )& 0xFF,f); // G
			fputc((buffer[i] >> 16)& 0xFF,f); // R
		}
		fclose(f);
	}
}

//
// Misc needed for game
//

bool system_pal()
{
	return config.pal;
}

void system_pal_set(bool on)
{
	if (config.pal != on)
	{
		config.changed = true;
		config.pal = on;
	}
}

bool system_easy()
{
	return config.easy;
}

void system_easy_set(bool on)
{
	if (config.easy != on)
	{
		config.changed = true;
		config.easy = on;
	}
}

bool system_music()
{
	return config.music;
}

void system_music_set(bool on)
{
	if (config.music != on)
	{
		config.changed = true;
		config.music = on;
	}
	enable_music(config.music);
}

bool system_resume()
{
	//return config.resume;
	return false; // disabled for demo
}

bool system_record()
{
	//return config.record;
	return false; // disabled for demo
}

bool system_playback()
{
	return config.playback;
}

const char* system_playback_file()
{
	return config.playback_file;
}

static int debug_text_line = 0;
static int debug_argc = 0;
static const char * const * debug_argv = NULL;

void system_debug_text_begin()
{
	debug_text_line = 0;
}

const char* system_debug_text_line()
{
	const char* r = NULL;

	static char temp[1024];

	switch (debug_text_line)
	{
	case 0:
		{
			int n = SDL_GetNumVideoDrivers();
			sprintf(temp,"SDL video drivers: %d",n);
			r = temp;
		} break;
	case 1:
		{
			const char *d = SDL_GetCurrentVideoDriver();
			if (d == NULL) d = "(none)";
			r = d;
		} break;
	case 2:
		{
			int n = SDL_GetNumAudioDrivers();
			sprintf(temp,"SDL audio drivers: %d",n);
			r = temp;
		} break;
	case 3:
		{
			const char *d = SDL_GetCurrentAudioDriver();
			if (d == NULL) d = "(none)";
			r = d;
		} break;
	case 4:
		{
			if (audio_enabled) r = "Audio: enabled";
			else               r = "Audio: disabled";
		} break;
	case 5:
		{
			int n = input_joy_count;
			sprintf(temp,"Joysticks open: %d",n);
			r = temp;
		} break;
	default:
		{
			int c = debug_text_line - 6;
			if (c < debug_argc)
			{
				r = debug_argv[c];
			}
		}
		break;
	}
	++debug_text_line;
	return r;
}

//
// Main
//

int main(int argc, char** argv)
{
	platform_setup();

	debug_argc = argc;
	debug_argv = argv;

	if (0 != SDL_Init(
		SDL_INIT_VIDEO |
		SDL_INIT_AUDIO |
		SDL_INIT_TIMER |
		SDL_INIT_JOYSTICK |
		SDL_INIT_GAMECONTROLLER |
		SDL_INIT_EVENTS))
	{
		alert(SDL_GetError(),"SDL initialization error!");
		return -1;
	}
	atexit(SDL_Quit);

	resume_load();
	config_init(argc,argv);

	Game::text_language_init();
	// languages disabled for demo
	//load_all_languages(argv[0]); // platform specific way of enumerating and loading language files
	Game::text_language_sort();
	Game::text_language_by_id(config.language); // set language, if it exists

	if (!renderer_init())
	{
		return -1;
	}

	Game::set_debug(config.debug);

	input_init(); // NOTE: silent failure possible for gamepads
	play_init(); // initialize audio player before opening the audio stream
	audio_init(); // NOTE: silent failure possible for audio

	#if _DEBUG
	{
		for (int i=0; i<SDL_GetNumAudioDrivers(); ++i)
			NES_DEBUG("SDL audio driver %d: %s\n",i,SDL_GetAudioDriver(i));
		for (int i=0; i<SDL_GetNumVideoDrivers(); ++i)
			NES_DEBUG("SDL video driver %d: %s\n",i,SDL_GetVideoDriver(i));

		system_debug_text_begin();
		const char *dt = system_debug_text_line();
		while (dt)
		{
			NES_DEBUG(dt);
			NES_DEBUG("\n");
			dt = system_debug_text_line();
		}
	}
	#endif

	PPU::init();
	Game::init();

	// main loop

	// timer to keep framerate consistent
	Uint32 last_time = SDL_GetTicks();
	double accum_time = 0.0;

	Uint32 last_click = SDL_GetTicks();
	bool clicked = false; // suppress first click, double clicking .exe tends to trigger fullscreen otherwise
	int last_ticks = 0; // estimates delta from previous frame
	
	quit = false;
	framerate = false;

	while(!quit)
	{
		platform_loop();

		// calculate desired speed
		double          frame_time = 1000.0 / 60.0; // NTSC speed (default)
		if (config.pal) frame_time = 1000.0 / 50.0; // PAL speed

		double slowest_time = frame_time * 3.0; // don't allow more than 3 frames to accumulate (don't skip frames to catch up after 2)

		// calculate time since last frame
		Uint32 time = SDL_GetTicks();
		Uint32 delta = time - last_time;
		last_time = time;

		SDL_Event event;
		while (SDL_PollEvent(&event) > 0)
		{
			switch (event.type)
			{
				case SDL_KEYDOWN:
					input_keydown(event.key.keysym, event.key.repeat != 0);
					break;
				case SDL_JOYBUTTONDOWN:
					if (Game::get_option_mode()) options_joydown(event.jbutton.button);
					break;
				case SDL_WINDOWEVENT:
				{
					switch(event.window.event)
					{
						case SDL_WINDOWEVENT_FOCUS_GAINED:
							SDL_DisableScreenSaver();
							priority_raise();
							break;
						case SDL_WINDOWEVENT_FOCUS_LOST:
							priority_lower();
							SDL_EnableScreenSaver();
							break;
						case SDL_WINDOWEVENT_SIZE_CHANGED:
						case SDL_WINDOWEVENT_RESIZED:
							renderer_scale();
							break;
						default:
							break;
					}
				} break;
				case SDL_MOUSEBUTTONDOWN:
				{
					Uint32 click_time = SDL_GetTicks();
					if (clicked && (click_time - last_click) < 300)
					{
						// double click = toggle fullscreen
						config.fullscreen = !config.fullscreen;
						config.changed = true;
						if(!renderer_refresh_fullscreen()) quit = true;
						if(Game::get_option_mode()) options_draw();
					}
					clicked = true;
					last_click = click_time;
				} break;
				case SDL_JOYDEVICEADDED:
				case SDL_JOYDEVICEREMOVED:
					input_refresh();
					break;
				case SDL_QUIT:
					quit = true;
					break;
				default:
					break;
			}
		}
		if (quit) break; // skip any rendering, just proceed to quit

		uint8 game_input = 0;
		if (Game::get_option_mode()) options_input_poll();
		else game_input = input_poll();
		
		const uint8 OPTION_PRESS = Game::PAD_A | Game::PAD_B | Game::PAD_SELECT | Game::PAD_START;
		if ((game_input & OPTION_PRESS) == OPTION_PRESS) options_on();

		int ticks = 0;

		double game_time = double(delta);

		if (multishot) slowest_time = frame_time;
		if (game_time > slowest_time) game_time = slowest_time; // just slow down if too many frames must be skipped

		accum_time += game_time;
		while (accum_time > 0.0)
		{
			if (frame_advance) halt = false;

			if (!halt && (!slowdown || slowdown_count == 0))
			{
				Game::tick(game_input);
				++ticks;
				slowdown_count = SLOWDOWN_RATE;

				if (speedup)
				{
					for(int t=1; t<SPEEDUP_RATE;++t)
					{
						Game::tick(game_input);
						++ticks;
					}
				}
			}
			else
			{
				if (slowdown_count > 0) --slowdown_count;
			}

			if (frame_advance)
			{
				halt = true;
				frame_advance = false;
			}
			
			accum_time -= frame_time;
		}

		if (framerate)
		{
			// Display:
			//    Previous frame:  delta  tick  render  present
			//    This frame:  tick  ticks  frame-clock  pause

			static int fr_clock = 0;
			const char* const FR_CLOCK[16] = {
				"0...............",
				".1..............",
				"..2.............",
				"...3............",
				"....4...........",
				".....5..........",
				"......6.........",
				".......7........",
				"........8.......",
				".........9......",
				"..........A.....",
				"...........B....",
				"............C...",
				".............D..",
				"..............E.",
				"...............F",
			};

			static Uint32 prev_delta   = 0;
			static Uint32 prev_time    = 0;
			static Uint32 prev_now     = 0;
			static Uint32 prev_tick    = 0;
			static Uint32 prev_render  = 0;
			static Uint32 prev_present = 0;

			Uint32 now = SDL_GetTicks();
			Uint32 tick = now - time;

			PPU::debug_text2("%2d : %2d %2d %2d",
				delta, prev_tick, prev_render, prev_present);
			PPU::debug_text2("%2d : %2d %s %s",
				tick, ticks, FR_CLOCK[fr_clock], Game::get_suspended() ? "P" : " ");

			prev_tick    = tick;
			prev_render  = fr_render  - prev_now;
			prev_present = fr_present - fr_render;;

			prev_delta   = delta;
			prev_time    = time;
			prev_now     = now;

			fr_clock = (fr_clock + 1) % 16;
		}

		if (config.clock && !Game::get_option_mode())
		{
			PPU::debug_text2("%02d:%02d:%02d/%02d",
				Game::h.metric_time_h,
				Game::h.metric_time_m,
				Game::h.metric_time_s,
				Game::h.metric_time_f);
		}

		if (config.stats && !Game::get_option_mode())
		{
			PPU::debug_text2("BONES:%5d",Game::h.metric_bones);
			PPU::debug_text2("JUMPS:%5d",Game::h.metric_jumps);
			PPU::debug_text2("COINS:%5d",Game::coin_count());
			if (Game::debug && Game::h.metric_cheater)
			{
				PPU::debug_text2("CHEATED!");
			}
		}

		if (halt)
		{
			PPU::debug_text2("HALT");
		}

		PPU::set_sprite_limit(config.sprite_limit);
		renderer_draw(PPU::render(),ticks);
		PPU::debug_cls2();

		// Allow the system relief if we seem to be spinning through the game loop too fast.
		// Usually vsync or texture locks are already providing this, but there are cases (e.g. minimized)
		// that still need this fallback. This is detected as two consecutive frames with no tick.
		// (Sleeping after only 1 tickless should be avoided, as it can cause oscillating delay.)
		// Wllow paused/static screens to intentionally relieve CPU as well.
		if (
			((ticks < 1) && (last_ticks < 1)) || // getting redundant loops, take a quick break
			Game::get_suspended() // game is paused/static
			)
		{
			SDL_Delay(6);
		}
		last_ticks = ticks;
	}

	// goodbye
	resume_save();
	find_display();
	if (config.changed) config_save();
	input_shutdown();
	audio_shutdown();
	renderer_destroy();
	platform_shutdown();
	return 0;
}

// end of file
