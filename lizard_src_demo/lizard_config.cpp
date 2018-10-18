// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include "SDL.h"
#include "lizard_game.h"
#include "lizard_config.h"
#include "lizard_platform.h"
#include "enums.h"

//
// Game configuration
//

Config config;

enum ConfigEnum
{
	CONFIG_PAL = 0,
	CONFIG_EASY,
	CONFIG_MUSIC,
	CONFIG_CLOCK,
	CONFIG_STATS,
	CONFIG_RESUME,
	CONFIG_RECORD,
	CONFIG_LANGUAGE,
	CONFIG_WIDTH,
	CONFIG_HEIGHT,
	CONFIG_SCALING,
	CONFIG_FILTER,
	CONFIG_FULLSCREEN,
	CONFIG_DISPLAY,
	CONFIG_SPRITE_LIMIT,
	CONFIG_BORDER_R,
	CONFIG_BORDER_G,
	CONFIG_BORDER_B,
	CONFIG_MARGIN,
	CONFIG_LATENCY,
	CONFIG_SAMPLERATE,
	CONFIG_VOLUME,
	CONFIG_KEY_A,
	CONFIG_KEY_B,
	CONFIG_KEY_SELECT,
	CONFIG_KEY_START,
	CONFIG_KEY_UP,
	CONFIG_KEY_DOWN,
	CONFIG_KEY_LEFT,
	CONFIG_KEY_RIGHT,
	CONFIG_JOY_A,
	CONFIG_JOY_B,
	CONFIG_JOY_SELECT,
	CONFIG_JOY_START,
	CONFIG_JOY_UP,
	CONFIG_JOY_DOWN,
	CONFIG_JOY_LEFT,
	CONFIG_JOY_RIGHT,
	CONFIG_JOY_OPTION,
	CONFIG_JOY_DEVICE,
	CONFIG_JOY_AXIS_X,
	CONFIG_JOY_AXIS_Y,
	CONFIG_JOY_HAT,
	// not saved
	CONFIG_DEBUG,
	CONFIG_PLAYBACK,
	CONFIG_PROTANOPIA,
	CONFIG_DEUTERANOPIA,
	CONFIG_TRITANOPIA,
	CONFIG_MONOCHROME,
	CONFIG_COUNT
};
const int CONFIG_HIDE = CONFIG_DEBUG;

const char* config_token[] = {
	"pal",
	"easy",
	"music",
	"clock",
	"stats",
	"resume",
	"record",
	"language",
	"width",
	"height",
	"scaling",
	"filter",
	"fullscreen",
	"display",
	"sprite_limit",
	"border_r",
	"border_g",
	"border_b",
	"margin",
	"audio_latency",
	"audio_samplerate",
	"audio_volume",
	"key_a",
	"key_b",
	"key_select",
	"key_start",
	"key_up",
	"key_down",
	"key_left",
	"key_right",
	"joy_a",
	"joy_b",
	"joy_select",
	"joy_start",
	"joy_up",
	"joy_down",
	"joy_left",
	"joy_right",
	"joy_option",
	"joy_device",
	"joy_axis_x",
	"joy_axis_y",
	"joy_hat",
	// not saved
	"lizard_of_debug",
	"playback",
	"protanopia",
	"deuteranopia",
	"tritanopia",
	"monochrome",
};
CT_ASSERT(ELEMENTS_OF(config_token)==CONFIG_COUNT,"config_token mismatched!");

int config_read(int c)
{
	switch(c)
	{
	case CONFIG_PAL:          return config.pal          ? 1 : 0;
	case CONFIG_EASY:         return config.easy         ? 1 : 0;
	case CONFIG_MUSIC:        return config.music        ? 1 : 0;
	case CONFIG_CLOCK:        return config.clock        ? 1 : 0;
	case CONFIG_STATS:        return config.stats        ? 1 : 0;
	case CONFIG_RESUME:       return config.resume       ? 1 : 0;
	case CONFIG_RECORD:       return config.record       ? 1 : 0;
	case CONFIG_LANGUAGE:     return config.language;
	case CONFIG_SCALING:      return config.scaling;
	case CONFIG_FILTER:       return config.filter;
	case CONFIG_FULLSCREEN:   return config.fullscreen   ? 1 : 0;
	case CONFIG_DISPLAY:      return config.display;
	case CONFIG_SPRITE_LIMIT: return config.sprite_limit ? 1 : 0;
	case CONFIG_BORDER_R:     return config.border_r;
	case CONFIG_BORDER_G:     return config.border_g;
	case CONFIG_BORDER_B:     return config.border_b;
	case CONFIG_MARGIN:       return config.margin;
	case CONFIG_LATENCY:      return config.audio_latency;
	case CONFIG_SAMPLERATE:   return config.audio_samplerate;
	case CONFIG_VOLUME:       return config.audio_volume;
	case CONFIG_KEY_A:        return int(config.keymap[GP_A     ]);
	case CONFIG_KEY_B:        return int(config.keymap[GP_B     ]);
	case CONFIG_KEY_SELECT:   return int(config.keymap[GP_SELECT]);
	case CONFIG_KEY_START:    return int(config.keymap[GP_START ]);
	case CONFIG_KEY_UP:       return int(config.keymap[GP_UP    ]);
	case CONFIG_KEY_DOWN:     return int(config.keymap[GP_DOWN  ]);
	case CONFIG_KEY_LEFT:     return int(config.keymap[GP_LEFT  ]);
	case CONFIG_KEY_RIGHT:    return int(config.keymap[GP_RIGHT ]);
	case CONFIG_JOY_A:        return config.joymap[GP_A     ];
	case CONFIG_JOY_B:        return config.joymap[GP_B     ];
	case CONFIG_JOY_SELECT:   return config.joymap[GP_SELECT];
	case CONFIG_JOY_START:    return config.joymap[GP_START ];
	case CONFIG_JOY_UP:       return config.joymap[GP_UP    ];
	case CONFIG_JOY_DOWN:     return config.joymap[GP_DOWN  ];
	case CONFIG_JOY_LEFT:     return config.joymap[GP_LEFT  ];
	case CONFIG_JOY_RIGHT:    return config.joymap[GP_RIGHT ];
	case CONFIG_JOY_OPTION:   return config.joymap[GP_OPTION];
	case CONFIG_JOY_DEVICE:   return config.joy_device;
	case CONFIG_JOY_AXIS_X:   return config.axis_x;
	case CONFIG_JOY_AXIS_Y:   return config.axis_y;
	case CONFIG_JOY_HAT:      return config.hat;
	case CONFIG_DEBUG:        return config.debug        ? 1 : 0;
	case CONFIG_PLAYBACK:     return 0;
	case CONFIG_PROTANOPIA:   return config.protanopia   ? 1 : 0;
	case CONFIG_DEUTERANOPIA: return config.deuteranopia ? 1 : 0;
	case CONFIG_TRITANOPIA:   return config.tritanopia   ? 1 : 0;
	case CONFIG_MONOCHROME:   return config.monochrome   ? 1 : 0;
	default: return 0;
	}
}

void config_write(int c, int v)
{
	switch(c)
	{
	case CONFIG_PAL:          config.pal               = (v != 0); break;
	case CONFIG_EASY:         config.easy              = (v != 0); break;
	case CONFIG_MUSIC:        config.music             = (v != 0); break;
	case CONFIG_CLOCK:        config.clock             = (v != 0); break;
	case CONFIG_STATS:        config.stats             = (v != 0); break;
	case CONFIG_RESUME:       config.resume            = (v != 0); break;
	case CONFIG_RECORD:       config.record            = (v != 0); break;
	case CONFIG_LANGUAGE:     config.language          = v; break;
	case CONFIG_SCALING:      config.scaling           = v; break;
	case CONFIG_FILTER:       config.filter            = v; break;
	case CONFIG_FULLSCREEN:   config.fullscreen        = (v != 0); break;
	case CONFIG_DISPLAY:      config.display           = v; break;
	case CONFIG_SPRITE_LIMIT: config.sprite_limit      = (v != 0); break;
	case CONFIG_BORDER_R:     config.border_r          = v; break;
	case CONFIG_BORDER_G:     config.border_g          = v; break;
	case CONFIG_BORDER_B:     config.border_b          = v; break;
	case CONFIG_MARGIN:       config.margin            = v; break;
	case CONFIG_LATENCY:      config.audio_latency     = v; break;
	case CONFIG_SAMPLERATE:   config.audio_samplerate  = v; break;
	case CONFIG_VOLUME:       config.audio_volume      = v; break;
	case CONFIG_KEY_A:        config.keymap[GP_A     ] = SDL_Scancode(v); break;
	case CONFIG_KEY_B:        config.keymap[GP_B     ] = SDL_Scancode(v); break;
	case CONFIG_KEY_SELECT:   config.keymap[GP_SELECT] = SDL_Scancode(v); break;
	case CONFIG_KEY_START:    config.keymap[GP_START ] = SDL_Scancode(v); break;
	case CONFIG_KEY_UP:       config.keymap[GP_UP    ] = SDL_Scancode(v); break;
	case CONFIG_KEY_DOWN:     config.keymap[GP_DOWN  ] = SDL_Scancode(v); break;
	case CONFIG_KEY_LEFT:     config.keymap[GP_LEFT  ] = SDL_Scancode(v); break;
	case CONFIG_KEY_RIGHT:    config.keymap[GP_RIGHT ] = SDL_Scancode(v); break;
	case CONFIG_JOY_A:        config.joymap[GP_A     ] = v; break;
	case CONFIG_JOY_B:        config.joymap[GP_B     ] = v; break;
	case CONFIG_JOY_SELECT:   config.joymap[GP_SELECT] = v; break;
	case CONFIG_JOY_START:    config.joymap[GP_START ] = v; break;
	case CONFIG_JOY_UP:       config.joymap[GP_UP    ] = v; break;
	case CONFIG_JOY_DOWN:     config.joymap[GP_DOWN  ] = v; break;
	case CONFIG_JOY_LEFT:     config.joymap[GP_LEFT  ] = v; break;
	case CONFIG_JOY_RIGHT:    config.joymap[GP_RIGHT ] = v; break;
	case CONFIG_JOY_OPTION:   config.joymap[GP_OPTION] = v; break;
	case CONFIG_JOY_DEVICE:   config.joy_device        = v; break;
	case CONFIG_JOY_AXIS_X:   config.axis_x            = v; break;
	case CONFIG_JOY_AXIS_Y:   config.axis_y            = v; break;
	case CONFIG_JOY_HAT:      config.hat               = v; break;
	case CONFIG_DEBUG:        config.debug             = (v != 0); break;
	case CONFIG_PLAYBACK:     /* see use of set_playback */        break;
	case CONFIG_PROTANOPIA:   config.protanopia        = (v != 0); break;
	case CONFIG_DEUTERANOPIA: config.deuteranopia      = (v != 0); break;
	case CONFIG_TRITANOPIA:   config.tritanopia        = (v != 0); break;
	case CONFIG_MONOCHROME:   config.monochrome        = (v != 0); break;
	default: break;
	}
}

void set_playback(const char* f)
{
	config.playback = true;
	string_cpy(config.playback_file,f,sizeof(config.playback_file));
}

bool config_parse(const char* p)
{
	// null line?
	if (p == NULL) return true;

	// skip leading whitespace
	while (p[0] == ' ' || p[0] == '\t')
	{
		++p;
	}

	// empty line
	if (p[0] == 0) return true;
	if (p[0] == 10 || p[0] == 13) return true;

	// find the =
	const char* split = strchr(p,'=');
	if (split == NULL) return false;

	// gather token and value
	char token[512];
	char value[512]; 

	int i = 0;
	while (p != split && p[0] != ' ' && p[0] != '\t' && i<(sizeof(token)-1))
	{
		token[i] = p[0];
		++p; ++i;
	}
	token[i] = 0;

	// make sure there are no extra characters
	while (p != split)
	{
		if (p[0] != ' ' && p[0] != '\t') return false;
		++p;
	}

	i = 0;
	p = split+1;
	while (p[0] != 0 && p[0] != 10 && p[0] != 13 && i<(sizeof(value)-1))
	{
		value[i] = p[0];
		++p; ++i;
	}
	value[i] = 0;

	// match token
	int ct = -1;
	for (i=0; i<CONFIG_COUNT; ++i)
	{
		if (!string_icmp(token,config_token[i]))
		{
			ct = i;
			break;
		}
	}
	if (ct < 0 || ct >= CONFIG_COUNT) return false; // unknown token

	// read value
	int v = strtol(value,NULL,10);

	// store the configuration value
	config_write(ct,v);

	if (ct == CONFIG_PLAYBACK)
	{
		set_playback(value);
	}

	return true;
}

void config_default()
{
	config.pal = false;
	config.easy = false;
	config.music = true;
	config.clock = false;
	config.stats = false;
	config.resume = true;
	config.record = false;
	config.language = 0;

	NES_DEBUG("Default language: %d\n", config.language);

	// logging by default in BETA
	#if BETA != 0
		config.record = true;
	#endif

	config.scaling = 0;
	config.filter = 0;
	config.fullscreen = false;
	config.display = 0;
	config.sprite_limit = true;
	config.border_r = 0;
	config.border_g = 0;
	config.border_b = 0;
	config.margin = 0;

	config.audio_latency = 5; // 4096 samples
	config.audio_samplerate = 48000;
	config.audio_volume = 100;

	config.keymap[GP_A     ] = SDL_GetScancodeFromKey(SDLK_x);
	config.keymap[GP_B     ] = SDL_GetScancodeFromKey(SDLK_z);
	config.keymap[GP_SELECT] = SDL_GetScancodeFromKey(SDLK_a);
	config.keymap[GP_START ] = SDL_GetScancodeFromKey(SDLK_s);
	config.keymap[GP_UP    ] = SDL_GetScancodeFromKey(SDLK_UP);
	config.keymap[GP_DOWN  ] = SDL_GetScancodeFromKey(SDLK_DOWN);
	config.keymap[GP_LEFT  ] = SDL_GetScancodeFromKey(SDLK_LEFT);
	config.keymap[GP_RIGHT ] = SDL_GetScancodeFromKey(SDLK_RIGHT);

	// this default mapping seems okay for most controllers
	config.joymap[GP_A     ] = 0;
	config.joymap[GP_B     ] = 2;
	config.joymap[GP_SELECT] = 6;
	config.joymap[GP_START ] = 7;
	config.joymap[GP_UP    ] = 255;
	config.joymap[GP_DOWN  ] = 255;
	config.joymap[GP_LEFT  ] = 255;
	config.joymap[GP_RIGHT ] = 255;
	config.joymap[GP_OPTION] = 4;

	config.joy_device = -1;
	config.axis_x = 0;
	config.axis_y = 1;
	config.hat = 0;
}

void config_init(int argc, char** argv)
{
	// default configuration
	config_default();

	config.debug = false;
	config.playback = false;
	memset(config.playback_file,0,sizeof(config.playback_file));

	#if _DEBUG
		config.debug = true;
	#endif

	config.protanopia = false;
	config.deuteranopia = false;
	config.tritanopia = false;
	config.monochrome = false;

	config.changed = false;

	// load lizard.ini configuration

	FILE* f;
	f = fopen(get_ini_file(),"rt");
	if (f != NULL)
	{
		int line = 0;
		char config_line[512];
		while (fgets(config_line,sizeof(config_line),f))
		{
			if (!config_parse(config_line))
			{
				char msg[sizeof(config_line) + 32 + 64];
				sprintf(msg,
					"Could not parse lizard.ini configuration.\n"
					"line %d: %s",line,config_line);
				debug_msg(msg); debug_msg("\n");
				//alert(msg,"Could not parse lizard.ini configuration.");
				break;
			}
			++line;
		}
		fclose(f);
	}
	else
	{
		config_save(); // save default config
	}

	// command line arguments

	for (int i=1; i<argc; ++i)
	{
		if (argv[i][0] == '-')
		{
			if (config_parse(argv[i]+1))
			{
				goto good_arg;
			}
		}
		else
		{
			// may also be a log file
			const char* v = argv[i];
			unsigned int l = string_len(v);
			if ( (l < 4) ||
					(v[l-4] != '.'                 ) ||
					(v[l-3] != 'l' && v[l-3] != 'L') ||
					(v[l-2] != 'i' && v[l-2] != 'I') ||
					(v[l-1] != 'z' && v[l-1] != 'Z') )
			{
				goto bad_arg;
			}

			set_playback(v);
			goto good_arg;
		}

		bad_arg:
			//alert(argv[i],"Could not parse command line configuration.");
			debug_msg("argument rejected: %s\n",argv[i]);
			continue;

		good_arg: ;
			NES_DEBUG("argument parsed: %s\n",argv[i]);
			continue;
	}
}

void config_save()
{
	FILE* f;
	f = fopen(get_ini_file(),"wt");
	if (f != NULL)
	{
		for (int i=0; i<CONFIG_HIDE; ++i)
		{
			fprintf(f,"%s=%d\n",config_token[i],config_read(i));
		}
		fclose(f);
	}
}

// end of file
