// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include <cstdio>
#include "SDL.h"
#include "lizard_game.h"
#include "lizard_options.h"
#include "lizard_ppu.h"
#include "lizard_audio.h"
#include "lizard_config.h"
#include "lizard_text.h"
#include "enums.h"
#include "assets/export/data.h"

using namespace Game;

//
// Options
//

// implemented in lizard_main
extern uint8 input_poll_joydir(int device, int axis_x, int axis_y, int hat);
extern unsigned int system_input_joy_count();
extern void renderer_scale();
extern bool renderer_init();
extern bool renderer_refresh_fullscreen();
extern bool renderer_rebuild_texture();
extern bool audio_init();
extern void audio_shutdown();
extern void system_debug_text_begin();
extern const char* system_debug_text_line();
extern bool quit;

static bool options_capture = false;
static uint8 options_last_joydir = 0;
static uint8 stored_font_cache;
static bool audio_restart = false;
static bool game_restart = false;
static int fps_enter = 0;


enum OptionType // types of option entries
{
	OT_END = 0,
	OT_BLANK,
	OT_TITLE,
	OT_ACTION,
	OT_TOGGLE,
	OT_JOY_DEVICE,
	OT_OFF_NUM,
	OT_NUM,
	OT_JOY,
	OT_KEY,
	OT_PAL,
	OT_SCALING,
	OT_FILTER,
	OT_LANGUAGE,
};

enum OptionID // IDs for each option
{
	OI_RETURN,
	OI_RESET,
	OI_QUIT,
	OI_BLANK,
	OI_DEBUG,
	OI_GAME_OPTIONS,
	OI_KEY_OPTIONS,
	OI_JOY_OPTIONS,
	OI_VIDEO_OPTIONS,
	OI_RESET_DEFAULT,
	OI_BACK,
	OI_EASY_MODE,
	OI_MUSIC,
	OI_SPEED,
	OI_CLOCK,
	OI_STATS,
	OI_RESUME,
	OI_LOGGING,
	OI_KEY_A,
	OI_KEY_B,
	OI_KEY_SELECT,
	OI_KEY_START,
	OI_KEY_UP,
	OI_KEY_DOWN,
	OI_KEY_LEFT,
	OI_KEY_RIGHT,
	OI_JOY_A,
	OI_JOY_B,
	OI_JOY_SELECT,
	OI_JOY_START,
	OI_JOY_UP,
	OI_JOY_DOWN,
	OI_JOY_LEFT,
	OI_JOY_RIGHT,
	OI_JOY_OPTION,
	OI_JOY_DEVICE,
	OI_JOY_AXIS_X,
	OI_JOY_AXIS_Y,
	OI_JOY_HAT,
	OI_FULLSCREEN,
	OI_SCALING,
	OI_FILTER,
	OI_SPRITE_LIMIT,
	OI_BORDER_R,
	OI_BORDER_G,
	OI_BORDER_B,
	OI_MARGIN,
	OI_AUDIO_BUFFER,
	OI_DEBUG_COIN,
	OI_DEBUG_BOSS,
	OI_DEBUG_BEYOND,
	OI_DEBUG_EYE,
	OI_DEBUG_BHM,
	OI_DEBUG_FLAG,
	OI_DEBUG_REWARD,
	OI_LATENCY,
	OI_SAMPLERATE,
	OI_VOLUME,
	OI_LANGUAGE,
};

struct OptionEntry
{
	OptionID id;
	TextEnum text;
	OptionType type;
	int param;
};

const OptionEntry PAGE_MAIN[] = {
	{ OI_BLANK,         TEXT_OPT_OPTIONS,       OT_TITLE,        0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_TITLE,        0 },
	{ OI_RETURN,        TEXT_OPT_RETURN,        OT_ACTION,       0 },
	{ OI_RESET,         TEXT_OPT_RESET,         OT_ACTION,       0 },
	{ OI_QUIT,          TEXT_OPT_QUIT,          OT_ACTION,       0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_BLANK,        0 },
	{ OI_GAME_OPTIONS,  TEXT_OPT_GAME_OPTIONS,  OT_ACTION,       0 },
	{ OI_KEY_OPTIONS,   TEXT_OPT_KEY_OPTIONS,   OT_ACTION,       0 },
	{ OI_JOY_OPTIONS,   TEXT_OPT_JOY_OPTIONS,   OT_ACTION,       0 },
	{ OI_VIDEO_OPTIONS, TEXT_OPT_VIDEO_OPTIONS, OT_ACTION,       0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_BLANK,        0 },
	{ OI_RESET_DEFAULT, TEXT_OPT_RESET_DEFAULT, OT_ACTION,       0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_END,          0 },
};

const OptionEntry PAGE_MAIN_DEBUG[] = {
	{ OI_BLANK,         TEXT_OPT_OPTIONS,       OT_TITLE,        0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_TITLE,        0 },
	{ OI_RETURN,        TEXT_OPT_RETURN,        OT_ACTION,       0 },
	{ OI_RESET,         TEXT_OPT_RESET,         OT_ACTION,       0 },
	{ OI_QUIT,          TEXT_OPT_QUIT,          OT_ACTION,       0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_BLANK,        0 },
	{ OI_DEBUG,         TEXT_OPT_DEBUG,         OT_ACTION,       0 },
	{ OI_GAME_OPTIONS,  TEXT_OPT_GAME_OPTIONS,  OT_ACTION,       0 },
	{ OI_KEY_OPTIONS,   TEXT_OPT_KEY_OPTIONS,   OT_ACTION,       0 },
	{ OI_JOY_OPTIONS,   TEXT_OPT_JOY_OPTIONS,   OT_ACTION,       0 },
	{ OI_VIDEO_OPTIONS, TEXT_OPT_VIDEO_OPTIONS, OT_ACTION,       0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_BLANK,        0 },
	{ OI_RESET_DEFAULT, TEXT_OPT_RESET_DEFAULT, OT_ACTION,       0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_END,          0 },
};

const OptionEntry PAGE_GAME[] = {
	{ OI_BLANK,         TEXT_OPT_GAME_OPTIONS,  OT_TITLE,        0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_TITLE,        0 },
	{ OI_BACK,          TEXT_OPT_BACK,          OT_ACTION,       0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_BLANK,        0 },
	{ OI_MUSIC,         TEXT_OPT_MUSIC,         OT_TOGGLE,       0 },
	{ OI_EASY_MODE,     TEXT_OPT_EASY_MODE,     OT_TOGGLE,       0 },
	{ OI_SPEED,         TEXT_OPT_SPEED,         OT_PAL,          0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_BLANK,        0 },
	{ OI_CLOCK,         TEXT_OPT_CLOCK,         OT_TOGGLE,       0 },
	{ OI_STATS,         TEXT_OPT_STATS,         OT_TOGGLE,       0 },
	//{ OI_BLANK,         TEXT_OPT_BLANK,         OT_BLANK,        0 },
	//{ OI_RESUME,        TEXT_OPT_RESUME,        OT_TOGGLE,       0 }, // option hidden for demo
	//{ OI_LOGGING,       TEXT_OPT_LOGGING,       OT_TOGGLE,       0 }, // option hidden for demo
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_END,          0 },
};

const OptionEntry PAGE_GAME_LANGUAGE[] = {
	{ OI_BLANK,         TEXT_OPT_GAME_OPTIONS,  OT_TITLE,        0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_TITLE,        0 },
	{ OI_BACK,          TEXT_OPT_BACK,          OT_ACTION,       0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_BLANK,        0 },
	{ OI_MUSIC,         TEXT_OPT_MUSIC,         OT_TOGGLE,       0 },
	{ OI_EASY_MODE,     TEXT_OPT_EASY_MODE,     OT_TOGGLE,       0 },
	{ OI_SPEED,         TEXT_OPT_SPEED,         OT_PAL,          0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_BLANK,        0 },
	{ OI_CLOCK,         TEXT_OPT_CLOCK,         OT_TOGGLE,       0 },
	{ OI_STATS,         TEXT_OPT_STATS,         OT_TOGGLE,       0 },
	//{ OI_BLANK,         TEXT_OPT_BLANK,         OT_BLANK,        0 }, // option hidden for demo
	//{ OI_RESUME,        TEXT_OPT_RESUME,        OT_TOGGLE,       0 }, // option hidden for demo
	//{ OI_LOGGING,       TEXT_OPT_LOGGING,       OT_TOGGLE,       0 }, // option hidden for demo
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_BLANK,        0 },
	{ OI_LANGUAGE,      TEXT_OPT_LANGUAGE,      OT_LANGUAGE,     0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_END,          0 },
};

const OptionEntry PAGE_KEY[] = {
	{ OI_BLANK,         TEXT_OPT_KEY_OPTIONS,   OT_TITLE,        0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_TITLE,        0 },
	{ OI_BACK,          TEXT_OPT_BACK,          OT_ACTION,       0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_BLANK,        0 },
	{ OI_KEY_A,         TEXT_OPT_KEY_A,         OT_KEY,          0 },
	{ OI_KEY_B,         TEXT_OPT_KEY_B,         OT_KEY,          1 },
	{ OI_KEY_SELECT,    TEXT_OPT_KEY_SELECT,    OT_KEY,          2 },
	{ OI_KEY_START,     TEXT_OPT_KEY_START,     OT_KEY,          3 },
	{ OI_KEY_UP,        TEXT_OPT_KEY_UP,        OT_KEY,          4 },
	{ OI_KEY_DOWN,      TEXT_OPT_KEY_DOWN,      OT_KEY,          5 },
	{ OI_KEY_LEFT,      TEXT_OPT_KEY_LEFT,      OT_KEY,          6 },
	{ OI_KEY_RIGHT,     TEXT_OPT_KEY_RIGHT,     OT_KEY,          7 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_END,          0 },
};

const OptionEntry PAGE_JOY[] = {
	{ OI_BLANK,         TEXT_OPT_JOY_OPTIONS,   OT_TITLE,        0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_TITLE,        0 },
	{ OI_BACK,          TEXT_OPT_BACK,          OT_ACTION,       0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_BLANK,        0 },
	{ OI_JOY_A,         TEXT_OPT_JOY_A,         OT_JOY,          0 },
	{ OI_JOY_B,         TEXT_OPT_JOY_B,         OT_JOY,          1 },
	{ OI_JOY_SELECT,    TEXT_OPT_JOY_SELECT,    OT_JOY,          2 },
	{ OI_JOY_START,     TEXT_OPT_JOY_START,     OT_JOY,          3 },
	{ OI_JOY_UP,        TEXT_OPT_JOY_UP,        OT_JOY,          4 },
	{ OI_JOY_DOWN,      TEXT_OPT_JOY_DOWN,      OT_JOY,          5 },
	{ OI_JOY_LEFT,      TEXT_OPT_JOY_LEFT,      OT_JOY,          6 },
	{ OI_JOY_RIGHT,     TEXT_OPT_JOY_RIGHT,     OT_JOY,          7 },
	{ OI_JOY_OPTION,    TEXT_OPT_JOY_OPTION,    OT_JOY,          8 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_BLANK,        0 },
	{ OI_JOY_DEVICE,    TEXT_OPT_JOY_DEVICE,    OT_JOY_DEVICE,   0 },
	{ OI_JOY_AXIS_X,    TEXT_OPT_JOY_AXIS_X,    OT_OFF_NUM,      0 },
	{ OI_JOY_AXIS_Y,    TEXT_OPT_JOY_AXIS_Y,    OT_OFF_NUM,      0 },
	{ OI_JOY_HAT,       TEXT_OPT_JOY_HAT,       OT_OFF_NUM,      0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_END,          0 },
};

const OptionEntry PAGE_VIDEO[] = {
	{ OI_BLANK,         TEXT_OPT_VIDEO_OPTIONS, OT_TITLE,        0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_TITLE,        0 },
	{ OI_BACK,          TEXT_OPT_BACK,          OT_ACTION,       0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_BLANK,        0 },
	{ OI_FULLSCREEN,    TEXT_OPT_FULLSCREEN,    OT_TOGGLE,       0 },
	{ OI_SCALING,       TEXT_OPT_SCALING,       OT_SCALING,      0 },
	{ OI_FILTER,        TEXT_OPT_FILTER,        OT_FILTER,       0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_BLANK,        0 },
	{ OI_SPRITE_LIMIT,  TEXT_OPT_SPRITE_LIMIT,  OT_TOGGLE,       0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_BLANK,        0 },
	{ OI_BORDER_R,      TEXT_OPT_BORDER_R,      OT_NUM,          0 },
	{ OI_BORDER_G,      TEXT_OPT_BORDER_G,      OT_NUM,          0 },
	{ OI_BORDER_B,      TEXT_OPT_BORDER_B,      OT_NUM,          0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_BLANK,        0 },
	{ OI_MARGIN,        TEXT_OPT_MARGIN,        OT_NUM,          0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_BLANK,        0 },
	{ OI_VOLUME,        TEXT_OPT_VOLUME,        OT_NUM,          0 },
	{ OI_LATENCY,       TEXT_OPT_LATENCY,       OT_OFF_NUM,      0 },
	{ OI_SAMPLERATE,    TEXT_OPT_SAMPLERATE,    OT_NUM,          0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_END,          0 },
};

const OptionEntry PAGE_DEBUG[] = {
	{ OI_BLANK,         TEXT_OPT_DEBUG,         OT_TITLE,        0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_TITLE,        0 },
	{ OI_BACK,          TEXT_OPT_BACK,          OT_ACTION,       0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_BLANK,        0 },
	{ OI_DEBUG_BEYOND,  TEXT_OPT_DEBUG_BEYOND,  OT_ACTION,       0 },
	{ OI_DEBUG_COIN,    TEXT_OPT_DEBUG_COIN,    OT_TOGGLE,       0 },
	{ OI_DEBUG_BOSS,    TEXT_OPT_DEBUG_BOSS,    OT_TOGGLE,       0 },
	{ OI_DEBUG_EYE,     TEXT_OPT_DEBUG_EYE,     OT_TOGGLE,       0 },
	{ OI_DEBUG_BHM,     TEXT_OPT_DEBUG_BHM,     OT_TOGGLE,       0 },
	{ OI_DEBUG_FLAG,    TEXT_OPT_DEBUG_FLAG,    OT_ACTION,       0 },
	{ OI_DEBUG_REWARD,  TEXT_OPT_DEBUG_REWARD,  OT_ACTION,       0 },
	{ OI_BLANK,         TEXT_OPT_BLANK,         OT_END,          0 },
};

const int PAGE_COUNT = 6;

const OptionEntry* PAGES[PAGE_COUNT] = {
	PAGE_MAIN,
	PAGE_GAME,
	PAGE_KEY,
	PAGE_JOY,
	PAGE_VIDEO,
	PAGE_DEBUG,
};

static unsigned int page = 0;
static unsigned int page_pos[PAGE_COUNT] = { 2,2,2,2,2,2 };
static unsigned int page_len[PAGE_COUNT] = { 0,0,0,0,0,0 }; // must be initialized
CT_ASSERT(PAGE_COUNT == 6, "PAGE_COUNT changed, must update page_pos/page_len initializers too.");

const uint8 OPTIONS_PAL[16] =
{
	0x01, 0x17, 0x27, 0x30, // plain
	0x01, 0x15, 0x25, 0x38, // entries
	0x01, 0x11, 0x21, 0x11, // highlight
	0x01, 0x15, 0x25, 0x31, // title
};

const OptionEntry& selected_entry()
{
	NES_ASSERT(page < PAGE_COUNT, "invalid options page selected?");
	NES_ASSERT(page_pos[page] < page_len[page], "invalid position on option page?");

	return PAGES[page][page_pos[page]];
}

// value accessors

bool value_debug_boss()
{
	return (flag_read(FLAG_BOSS_0_MOUNTAIN) &&
	        flag_read(FLAG_BOSS_1_RIVER) &&
	        flag_read(FLAG_BOSS_2_WATER) &&
	        flag_read(FLAG_BOSS_3_VOLCANO) &&
	        flag_read(FLAG_BOSS_4_PALACE) &&
	        flag_read(FLAG_BOSS_5_VOID));
}

int value_get(const OptionEntry& e)
{
	switch (e.id)
	{
		case OI_RETURN:        return 0;
		case OI_RESET:         return 0;
		case OI_QUIT:          return 0;
		case OI_BLANK:         return 0;
		case OI_DEBUG:         return 0;
		case OI_GAME_OPTIONS:  return 0;
		case OI_KEY_OPTIONS:   return 0;
		case OI_JOY_OPTIONS:   return 0;
		case OI_VIDEO_OPTIONS: return 0;
		case OI_RESET_DEFAULT: return 0;
		case OI_BACK:          return 0;
		case OI_EASY_MODE:     return config.easy ? 1 : 0;
		case OI_MUSIC:         return get_music_enabled() ? 1 : 0;
		case OI_SPEED:         return config.pal ? 1 : 0;
		case OI_CLOCK:         return config.clock ? 1 : 0;
		case OI_STATS:         return config.stats ? 1 : 0;
		case OI_RESUME:        return config.resume ? 1 : 0;
		case OI_LOGGING:       return config.record ? 1 : 0;
		case OI_KEY_A:         return config.keymap[0];
		case OI_KEY_B:         return config.keymap[1];
		case OI_KEY_SELECT:    return config.keymap[2];
		case OI_KEY_START:     return config.keymap[3];
		case OI_KEY_UP:        return config.keymap[4];
		case OI_KEY_DOWN:      return config.keymap[5];
		case OI_KEY_LEFT:      return config.keymap[6];
		case OI_KEY_RIGHT:     return config.keymap[7];
		case OI_JOY_A:         return config.joymap[0];
		case OI_JOY_B:         return config.joymap[1];
		case OI_JOY_SELECT:    return config.joymap[2];
		case OI_JOY_START:     return config.joymap[3];
		case OI_JOY_UP:        return config.joymap[4];
		case OI_JOY_DOWN:      return config.joymap[5];
		case OI_JOY_LEFT:      return config.joymap[6];
		case OI_JOY_RIGHT:     return config.joymap[7];
		case OI_JOY_OPTION:    return config.joymap[8];
		case OI_JOY_DEVICE:    return config.joy_device;
		case OI_JOY_AXIS_X:    return config.axis_x;
		case OI_JOY_AXIS_Y:    return config.axis_y;
		case OI_JOY_HAT:       return config.hat;
		case OI_FULLSCREEN:    return config.fullscreen ? 1 : 0;
		case OI_SCALING:       return config.scaling;
		case OI_FILTER:        return config.filter;
		case OI_SPRITE_LIMIT:  return config.sprite_limit ? 1 : 0;
		case OI_BORDER_R:      return config.border_r;
		case OI_BORDER_G:      return config.border_g;
		case OI_BORDER_B:      return config.border_b;
		case OI_MARGIN:        return config.margin;
		case OI_DEBUG_COIN:    return (coin_count() >= DATA_COIN_COUNT) ? 1 : 0;
		case OI_DEBUG_BOSS:    return value_debug_boss() ? 1 : 0;
		case OI_DEBUG_BEYOND:  return 0;
		case OI_DEBUG_EYE:     return flag_read(FLAG_EYESIGHT) ? 1 : 0;
		case OI_DEBUG_BHM:     return ((lizard.big_head_mode & 0x80) != 0) ? 1 : 0;
		case OI_DEBUG_FLAG:    return 0;
		case OI_DEBUG_REWARD:  return 0;
		case OI_LATENCY:       return config.audio_latency;
		case OI_SAMPLERATE:    return config.audio_samplerate;
		case OI_VOLUME:        return config.audio_volume;
		case OI_LANGUAGE:      return config.language;
		default:
			break;
	};
	NES_ASSERT(false,"value_get with invalid option entry!");
	return 0;
}

void option_draw_debug_text(int x, int y)
{
	system_debug_text_begin();
	const char* t = system_debug_text_line();
	while (t)
	{
		PPU::meta_text(x,y,t,2);
		++y;
		t = system_debug_text_line();
		if (y >= 30) return;
	}
}

void option_entry_draw(const OptionEntry& e, int x, int y)
{
	static char temp[32];

	uint8 attrib = 0;
	if (e.type == OT_TITLE)
	{
		attrib = 3;
		x -= 2;
	}
	PPU::meta_text(x,y,Game::text_get(e.text),attrib);

	int v = value_get(e);
	switch (e.type)
	{
		case OT_END: break;
		case OT_BLANK: break;
		case OT_TITLE: break;
		case OT_ACTION: break;
		case OT_TOGGLE:
			PPU::meta_text(x+24, y, (v==0) ? text_get(TEXT_OPT_OFF) : text_get(TEXT_OPT_ON), 1);
			break;
		case OT_JOY_DEVICE:
			if (v < 0)
			{
				PPU::meta_text(x+24, y, text_get(TEXT_OPT_ANY), 1);
				goto show_arrows;
			}
			if (v >= 255) v = -1;
			// fall through
		case OT_OFF_NUM:
			if (v < 0)
			{
				PPU::meta_text(x+24, y, text_get(TEXT_OPT_OFF), 1);
				goto show_arrows;
			}
			// fall through
		case OT_NUM:
			if (v < 0) v = 0;
			if (v < 1000)
			{
				sprintf(temp,"%3d",v);
				PPU::meta_text(x+24, y, temp, 1);
			}
			else if (v < 100000)
			{
				sprintf(temp,"%5d",v);
				PPU::meta_text(x+22, y, temp, 1);
			}
			else
			{
				PPU::meta_text(x+22, y, "ERROR", 1);
			}
		show_arrows:
			if (&selected_entry() == &e)
			{
				if (v < 1000) PPU::meta_text(x+23,y,"\xA2",2);
				else          PPU::meta_text(x+21,y,"\xA2",2);
				PPU::meta_text(x+27,y,"\xA3",2);
			}
			break;
		case OT_JOY:
			if (v >= 255)
			{
				PPU::meta_text(x+23, y, text_get(TEXT_OPT_NONE), 1);
				break;
			}
			if (v < 0) v = 0;
			if (v > 255) v = 255;
			sprintf(temp,"%3d",v);
			if (&selected_entry() != &e || !options_capture)
			{
				PPU::meta_text(x+24, y, temp, 1);
			}
			break;
		case OT_KEY:
			{
				NES_ASSERT(e.param < 8, "OT_KEY param out of range!");
				const char* longname = SDL_GetKeyName(SDL_GetKeyFromScancode(config.keymap[e.param]));

				char keyname[17];
				for (int j=0;j<17;++j)
				{
					keyname[j] = *longname;
					if (*longname == 0) break;
					++longname;
				}
				keyname[17-1] = 0;

				if (&selected_entry() != &e || !options_capture)
				{
					sprintf(temp,"%16s",keyname);
					PPU::meta_text(14,y,temp,1);
				}
			}
			break;
		case OT_PAL:
			PPU::meta_text(x+18, y, (v==0) ? text_get(TEXT_OPT_NTSC) : text_get(TEXT_OPT_PAL), 1);
			break;
		case OT_SCALING:
			{
				const char*      t = text_get(TEXT_OPT_SCALING_CLEAN);
				if      (v == 1) t = text_get(TEXT_OPT_SCALING_FIT);
				else if (v == 2) t = text_get(TEXT_OPT_SCALING_TV);
				else if (v == 3) t = text_get(TEXT_OPT_SCALING_STRETCH);
				PPU::meta_text(x+20,y,t,1);
			}
			break;
		case OT_FILTER:
			{
				const char*      t = text_get(TEXT_OPT_FILTER_NONE);
				if      (v == 1) t = text_get(TEXT_OPT_FILTER_LINEAR);
				else if (v == 2) t = text_get(TEXT_OPT_FILTER_TV);
				PPU::meta_text(x+20,y,t,1);
			}
			break;
		case OT_LANGUAGE:
			{
				const char*      t = text_get(TEXT_OPT_CURRENT_LANGUAGE);
				int tx = 30 - string_len(t);
				if (tx < x) tx = x;
				PPU::meta_text(tx,y,t,1);
			}
		default:
			break;
	};
}

void options_draw()
{
	if (!Game::get_option_mode()) return;

	NES_ASSERT(page < PAGE_COUNT, "invalid option page selected?");

	PPU::meta_palette(OPTIONS_PAL);
	PPU::meta_cls();

	int y = 1;
	for (unsigned int i=0; i < 30; ++i)
	{
		const OptionEntry& e = PAGES[page][i];
		if (e.type == OT_END)
		{
			if (page == 5) // debug page
			{
				option_draw_debug_text(3,y+1);
			}
			break;
		}
		NES_ASSERT(i < page_len[page], "invalid option entry to be drawn?");

		option_entry_draw(e,3,y);
		++y;
	}

	// cursor
	if (!options_capture)
	{
		PPU::meta_text(1,page_pos[page]+1,"\xA9",0);
	}
	if (options_capture)
	{
		PPU::meta_text(1,page_pos[page]+1,"\xA9",2);

		if (selected_entry().type == OT_KEY)
		{
			PPU::meta_text(1,27,text_get(TEXT_OPT_MSG_KEY),2);
			PPU::meta_text(1,28,text_get(TEXT_OPT_MSG_CANCEL),2);
		}
		else if (selected_entry().type == OT_JOY)
		{
			PPU::meta_text(1,26,text_get(TEXT_OPT_MSG_JOY0),2);
			PPU::meta_text(1,27,text_get(TEXT_OPT_MSG_JOY1),2);
			PPU::meta_text(1,28,text_get(TEXT_OPT_MSG_CANCEL),2);
		}
	}

	if (page == 3) // PAGE_JOY
	{

		PPU::meta_text(3,y+1,text_get(TEXT_OPT_JOYSTICKS_FOUND),2);
		char temp[12];
		sprintf(temp,"%3d",system_input_joy_count());
		PPU::meta_text(27,y+1,temp,2);
	}
}

void options_on()
{
	if (Game::get_option_mode()) return;
	Game::set_option_mode(true);
	pause_audio();
	audio_restart = false;
	game_restart = false;

	// remember entering FPS
	fps_enter = config.pal ? 50 : 60;
	if (config.easy) fps_enter = 40;

	if (config.debug)
	{
		PAGES[0] = PAGE_MAIN_DEBUG;
	}

	if (text_language_count() > 1)
	{
		PAGES[1] = PAGE_GAME_LANGUAGE;
	}

	for (int i=0; i<PAGE_COUNT; ++i)
	{
		page_len[i] = 0;
		for (int p=0; p<32; ++p)
		{
			if (PAGES[i][p].type == OT_END)
			{
				break;
			}

			if (p > 0 && PAGES[i][p].type == OT_BLANK)
			{
				NES_ASSERT(PAGES[i][p-1].type != OT_TITLE, "OT_BLANK should not follow OT_TITLE. (Would become selectable.)");
			}

			++page_len[i];
		}
		NES_ASSERT(page_len[i] < 32,"options page missing OT_END");
		NES_ASSERT(page_len[i] >= 3, "empty options page?");

		if (page_pos[i] >= page_len[i])
		{
			page_pos[i] = page_len[i]-1;
		}

		NES_ASSERT(PAGES[i][0].type == OT_TITLE, "all option pages must being with TITLE");
	}

	page = 0; // PAGE_MAIN
	page_pos[0] = 2;
	options_capture = false;
	options_last_joydir = input_poll_joydir(-1,0,1,0);

	PPU::push_overlay();
	stored_font_cache = Game::zp.chr_cache[1];
	// force clean load of needed font tiles
	bool eyesight = Game::flag_read(FLAG_EYESIGHT);
	Game::flag_clear(FLAG_EYESIGHT);
	Game::zp.chr_cache[1] = 0xFF;
	Game::zp.chr_cache[3] = 0xFF;
	Game::chr_load(1,DATA_chr_font); // ensure font is available
	Game::chr_load(3,DATA_chr_dog_b2); // some special characters are here
	if (eyesight) Game::flag_set(FLAG_EYESIGHT);
	Game::zp.chr_cache[1] = 0xFF;
	Game::zp.chr_cache[3] = 0xFF;

	PPU::overlay_y(0,false);
	PPU::overlay_scroll(256,0);
	PPU::split_x(0,240);
	PPU::debug_off();
	options_draw();
}

void options_off()
{
	if (!Game::get_option_mode()) return;
	Game::set_option_mode(false);
	PPU::pop_overlay();

	// force reload of font if currently in use
	if (stored_font_cache == DATA_chr_font)
	{
		Game::zp.chr_cache[1] = 0xFF;
		Game::chr_load(1,DATA_chr_font);
	}

	if (config.record)
	{
	}
	else
	{
		// NOT IN DEMO
	}

	if (audio_restart)
	{
		audio_restart = false;
		audio_shutdown();
		audio_init();
	}
	unpause_audio();

	// prevent options and title settings from getting out of sync
	if (Game::get_game_mode() == Game::GAME_TITLE)
	{
		game_restart = true;
	}

	if (game_restart)
	{
		play_init(); // resets pause
		Game::init();
	}

	// convert time if speed changed
	int fps_exit = config.pal ? 50 : 60;
	if (config.easy) fps_exit = 40;
	convert_time(fps_enter, fps_exit);
}

enum OptionActionEnum {
	OPTION_ACTION_UP = 0,
	OPTION_ACTION_DOWN = 1,
	OPTION_ACTION_LEFT = 2,
	OPTION_ACTION_RIGHT = 3,
	OPTION_ACTION_ENTER = 4
};

void option_action_enter(const OptionEntry &e); // forward declaration

void option_action_left(const OptionEntry &e, bool repeat)
{
	if (repeat && e.type != OT_NUM) return;
	if(e.type == OT_TOGGLE) { option_action_enter(e); return; }

	switch(e.id)
	{
		// break for config.changed
		// return for no change
		case OI_SPEED: option_action_enter(e);                            return; // config changed on option_action_enter
		case OI_JOY_DEVICE:
		{
		    if (config.joy_device >= 255)                                 return;
		    if (config.joy_device < 0   ) { config.joy_device = 255;      break; }
		    --config.joy_device;                                          break;
		}
		case OI_JOY_AXIS_X: if (config.axis_x   > -1  ) --config.axis_x;  else return; break;
		case OI_JOY_AXIS_Y: if (config.axis_y   > -1  ) --config.axis_y;  else return; break;
		case OI_JOY_HAT:    if (config.hat      > -1  ) --config.hat;     else return; break;
		case OI_SCALING:
		{
		    if (config.scaling  > 0) --config.scaling;
		    else config.scaling = 3;
			renderer_scale();
		}                                                                 break;
		case OI_FILTER:
		{
			if (config.filter  > 0) --config.filter;
			else config.filter = 2;
			if (!renderer_rebuild_texture()) quit = true;
		}                                                                 break;
		case OI_BORDER_R:   config.border_r = (config.border_r-1) & 0xFF; break;
		case OI_BORDER_G:   config.border_g = (config.border_g-1) & 0xFF; break;
		case OI_BORDER_B:   config.border_b = (config.border_b-1) & 0xFF; break;
		case OI_MARGIN:
		{
			if (config.margin > 0)
			{
				--config.margin;
				renderer_scale();
			} else return;
			
		}                                                                 break;
		case OI_LATENCY:
		{
			if (config.audio_latency > -1)
			{
				--config.audio_latency;
				audio_restart = true;
			} else return;
		}                                                                 break;
		case OI_SAMPLERATE: option_action_enter(e);                       return;
		case OI_VOLUME:
		{
			if (config.audio_volume > 0) --config.audio_volume;
			else return;
			play_volume(config.audio_volume);
		}                                                                 break;
		case OI_LANGUAGE:
		{
			text_language_advance(false);
			config.language = text_language_id();
			Game::zp.chr_cache[1] = 0xFF;
			Game::chr_load(1,DATA_chr_font);
			if (get_game_mode() == Game::GAME_TITLE) game_restart = true;
		}                                                                 break;
		default:
		                                                                  return;
	}
	config.changed = true;
}

void option_action_right(const OptionEntry &e, bool repeat)
{
	if (repeat && e.type != OT_NUM) return;
	if(e.type == OT_TOGGLE) { option_action_enter(e); return; }

	switch(e.id)
	{
		// break for config.changed
		// return for no change
		case OI_SPEED: option_action_enter(e);                            return; // config changed on action_enter
		case OI_JOY_DEVICE:
		{
		    if (config.joy_device == 255) { config.joy_device = -1;       break; }
		    if (config.joy_device < 15  ) { ++config.joy_device;          break; }
		                                                                  return;
		}
		case OI_JOY_AXIS_X: if (config.axis_x   < 15  ) ++config.axis_x;  else return; break;
		case OI_JOY_AXIS_Y: if (config.axis_y   < 15  ) ++config.axis_y;  else return; break;
		case OI_JOY_HAT:    if (config.hat      < 15  ) ++config.hat;     else return; break;
		case OI_SCALING:
		{
		    if (config.scaling  < 3) ++config.scaling;
		    else config.scaling = 0;
		    renderer_scale();
		}                                                                 break;
		case OI_FILTER:
		{
			if (config.filter  < 2) ++config.filter;
			else config.filter = 0;
			if (!renderer_rebuild_texture()) quit = true;
		}                                                                 break;
		case OI_BORDER_R:   config.border_r = (config.border_r+1) & 0xFF; break;
		case OI_BORDER_G:   config.border_g = (config.border_g+1) & 0xFF; break;
		case OI_BORDER_B:   config.border_b = (config.border_b+1) & 0xFF; break;
		case OI_MARGIN:
		{
			if (config.margin < 999)
			{
				++config.margin;
				renderer_scale();
			} else return;
			
		}                                                                 break;
		case OI_LATENCY:
		{
			if (config.audio_latency < 8)
			{
				++config.audio_latency;
				audio_restart = true;
			} else return;
		}                                                                 break;
		case OI_SAMPLERATE: option_action_enter(e);                       return;
		case OI_VOLUME:
		{
			if (config.audio_volume < 100) ++config.audio_volume;
			else return;
			play_volume(config.audio_volume);
		}                                                                 break;
		case OI_LANGUAGE:
		{
			text_language_advance(true);
			config.language = text_language_id();
			Game::zp.chr_cache[1] = 0xFF;
			Game::chr_load(1,DATA_chr_font);
			if (get_game_mode() == Game::GAME_TITLE) game_restart = true;
		}                                                                 break;
		default:
		                                                                  return;
	}
	config.changed = true;
}

void option_action_enter(const OptionEntry &e)
{
	switch(e.id)
	{
		// break for config.changed
		// return for no change
		case OI_RETURN:
			options_off();
			return;
		case OI_RESET:
			play_init(); // resets pause
			resume.valid = false; // invalidate resume
			Game::init();
			return;
		case OI_QUIT:
			quit = true;
			return;
		case OI_DEBUG:
			page = 5;
			return;
		case OI_GAME_OPTIONS:
			page = 1;
			return;
		case OI_KEY_OPTIONS:
			page = 2;
			return;
		case OI_JOY_OPTIONS:
			page = 3;
			return;
		case OI_VIDEO_OPTIONS:
			page = 4;
			return;
		case OI_RESET_DEFAULT:
			config_default();
			if(!renderer_refresh_fullscreen()) quit = true;
			text_language_by_id(config.language);
			zp.chr_cache[1] = 0xFF;
			Game::chr_load(1,DATA_chr_font);
			audio_restart = true;
			enable_music(config.music);
			break;
		case OI_BACK:
			page = 0; // PAGE_MAIN
			return;
		case OI_EASY_MODE:
			config.easy = !config.easy;
			break;
		case OI_MUSIC:
			config.music = !get_music_enabled();
			enable_music(config.music);
			break;
		case OI_SPEED:
			config.pal = !config.pal;
			break;
		case OI_CLOCK:
			config.clock = !config.clock;
			break;
		case OI_STATS:
			config.stats = !config.stats;
			break;
		case OI_RESUME:
			config.resume = !config.resume;
			break;
		case OI_LOGGING:
			config.record = !config.record;
			break;
		case OI_KEY_A:
		case OI_KEY_B:
		case OI_KEY_SELECT:
		case OI_KEY_START:
		case OI_KEY_UP:
		case OI_KEY_DOWN:
		case OI_KEY_LEFT:
		case OI_KEY_RIGHT:
			options_capture = true;
			return; // config.changed on capture end
		case OI_JOY_A:
		case OI_JOY_B:
		case OI_JOY_SELECT:
		case OI_JOY_START:
		case OI_JOY_UP:
		case OI_JOY_DOWN:
		case OI_JOY_LEFT:
		case OI_JOY_RIGHT:
		case OI_JOY_OPTION:
			options_capture = true;
			return; // config.changed on capture end
		case OI_FULLSCREEN:
			config.fullscreen = !config.fullscreen;
			if(!renderer_refresh_fullscreen()) quit = true;
			break;
		case OI_SCALING:
			option_action_right(e,false);
			return; // config.changed by option_action_right
		case OI_FILTER:
			option_action_right(e,false);
			break; // config.changed by option_action_right
		case OI_SPRITE_LIMIT:
			config.sprite_limit = !config.sprite_limit;
			break;
		case OI_DEBUG_COIN:
			if (coin_count() < DATA_COIN_COUNT) for (int i=0; i<128; ++i) coin_take(i);
			else                                for (int i=0; i<128; ++i) coin_return(i);
			return;
		case OI_DEBUG_BOSS:
			if (!value_debug_boss())
			{
				flag_set(FLAG_BOSS_0_MOUNTAIN);
				flag_set(FLAG_BOSS_1_RIVER);
				flag_set(FLAG_BOSS_2_WATER);
				flag_set(FLAG_BOSS_3_VOLCANO);
				flag_set(FLAG_BOSS_4_PALACE);
				flag_set(FLAG_BOSS_5_VOID);
			}
			else
			{
				flag_clear(FLAG_BOSS_0_MOUNTAIN);
				flag_clear(FLAG_BOSS_1_RIVER);
				flag_clear(FLAG_BOSS_2_WATER);
				flag_clear(FLAG_BOSS_3_VOLCANO);
				flag_clear(FLAG_BOSS_4_PALACE);
				flag_clear(FLAG_BOSS_5_VOID);
			}
			return;
		case OI_DEBUG_BEYOND:
			zp.current_lizard = LIZARD_OF_BEYOND;
			zp.next_lizard = LIZARD_OF_BEYOND;
			h.last_lizard = 0xFF;
			return;
		case OI_DEBUG_EYE:
			if (flag_read(FLAG_EYESIGHT)) flag_clear(FLAG_EYESIGHT);
			else                          flag_set(FLAG_EYESIGHT);
			for (int i=0; i<8; ++i) zp.chr_cache[i] = 0xFF;
			if (Game::get_game_mode() != Game::GAME_TITLE)
			{
				zp.room_change = 1;
				zp.current_door = 0;
				//zp.current_room = zp.current_room;
				set_game_mode(GAME_FADE_OUT);
			}
			return;
		case OI_DEBUG_BHM:
			lizard.big_head_mode ^= 0x80;
			return;
		case OI_DEBUG_FLAG:
			for (int i=0; i<128; ++i) flag_clear(i);
			return;
		case OI_DEBUG_REWARD:
			return;
		case OI_SAMPLERATE:
			if (config.audio_samplerate != 48000) config.audio_samplerate = 48000;
			else                                  config.audio_samplerate = 44100;
			audio_restart = true;
			break;
		case OI_LANGUAGE:
			option_action_right(e,false);
			return; // config.changed by option_action_right
		default:
			return;
	}
	config.changed = true;
}

void options_action(int action, bool repeat)
{
	NES_ASSERT(page < PAGE_COUNT, "invalid page selected?");
	switch (action)
	{

	case OPTION_ACTION_UP:
		{
		repeat_up:
			if (page_pos[page] > 0) 
			{
				--page_pos[page];
				if (selected_entry().type == OT_TITLE) // don't select TITLE entries
				{
					++page_pos[page]; // undo
					break;
				}
				if (selected_entry().type == OT_BLANK) // skip blanks
				{
					goto repeat_up;
				}
			}
			else break;
		}
		break;
	case OPTION_ACTION_DOWN:
		{
		repeat_down:
			if ((page_pos[page]+1) < page_len[page])
			{
				++page_pos[page];
				if (selected_entry().type == OT_BLANK) // skip blanks
				{
					goto repeat_down;
				}
			}
			else break;
		}
		break;
	case OPTION_ACTION_LEFT:
		option_action_left(selected_entry(), repeat);
		break;
	case OPTION_ACTION_RIGHT:
		option_action_right(selected_entry(), repeat);
		break;
	case OPTION_ACTION_ENTER:
		if (!repeat) option_action_enter(selected_entry());
		break;
	default:
		break;
	}
	options_draw();
}

void options_capture_key(SDL_Scancode s)
{
	if (!options_capture) return; // this shouldn't hapen

	const OptionEntry& e = selected_entry();
	if (e.type == OT_KEY)
	{
		NES_ASSERT(e.param < 8, "OT_KEY param out of range!");
		config.keymap[e.param] = s;
		config.changed = true;
	}

	options_capture = false;
}

void options_capture_joy(Uint8 b)
{
	if (!options_capture) return; // this shouldn't hapen

	const OptionEntry& e = selected_entry();
	if (e.type == OT_JOY)
	{
		NES_ASSERT(e.param < 9, "OT_JOY param out of range!");
		config.joymap[e.param] = b;
		config.changed = true;
	}

	options_capture = false;
}

void options_input_poll()
{
	NES_ASSERT(get_option_mode(), "options_input_poll without option_mode!");

	if (options_capture) return;

	uint8 new_dir = input_poll_joydir(-1,0,1,0);
	if ((new_dir & Game::PAD_U) && !(options_last_joydir & Game::PAD_U)) options_action(OPTION_ACTION_UP,false);
	if ((new_dir & Game::PAD_D) && !(options_last_joydir & Game::PAD_D)) options_action(OPTION_ACTION_DOWN,false);
	if ((new_dir & Game::PAD_L) && !(options_last_joydir & Game::PAD_L)) options_action(OPTION_ACTION_LEFT,false);
	if ((new_dir & Game::PAD_R) && !(options_last_joydir & Game::PAD_R)) options_action(OPTION_ACTION_RIGHT,false);
	options_last_joydir = new_dir;
}

void options_joydown(Uint8 button)
{
	NES_ASSERT(get_option_mode(), "options_joydown without option_mode!");

	if (options_capture)
	{
		options_capture_joy(button);
		options_draw();
	}
	else if (button < 4)
	{
		options_action(OPTION_ACTION_ENTER,false);
	}
}

void options_keydown(const SDL_Keysym& key, bool repeat)
{
	NES_ASSERT(get_option_mode(), "options_keydown without option_mode!");
	if (options_capture)
	{
		if (repeat) return;

		switch (key.sym)
		{
		case SDLK_ESCAPE:
			options_capture = false;
			options_draw();
			break;
		case SDLK_RETURN:
		case SDLK_RETURN2:
		case SDLK_KP_ENTER:
			if (selected_entry().type == OT_JOY)
			{
				// unassign joystick button
				NES_ASSERT(selected_entry().param < 9, "OT_JOY param out of range!");
				config.joymap[selected_entry().param] = 255;
				config.changed = true;
			}
			// fallthrough
		default:
			options_capture_key(key.scancode);
			options_draw();
		}
	}
	else
	{
		switch (key.sym)
		{
		case SDLK_ESCAPE:
			if (page == 0) { page_pos[0] = 2; }
			page = 0;
			options_draw();
			break;
		case SDLK_UP:
			options_action(OPTION_ACTION_UP, repeat);
			break;
		case SDLK_DOWN:
			options_action(OPTION_ACTION_DOWN, repeat);
			break;
		case SDLK_LEFT:
			options_action(OPTION_ACTION_LEFT, repeat);
			break;
		case SDLK_RIGHT:
			options_action(OPTION_ACTION_RIGHT, repeat);
			break;
		case SDLK_RETURN:
		case SDLK_RETURN2:
		case SDLK_KP_ENTER:
			options_action(OPTION_ACTION_ENTER, repeat);
			break;
		default:
			break;
		}
	}
}


// end of file
