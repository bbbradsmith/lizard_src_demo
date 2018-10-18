#pragma once
// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

enum ConfigControlEnum
{
	GP_A      = 0,
	GP_B      = 1,
	GP_SELECT = 2,
	GP_START  = 3,
	GP_UP     = 4,
	GP_DOWN   = 5,
	GP_LEFT   = 6,
	GP_RIGHT  = 7,
	GP_OPTION = 8,
};

struct Config
{
	// game
	bool pal;
	bool easy;
	bool music;
	bool clock;
	bool stats;
	bool resume;
	bool record;
	unsigned int language; // ID

	// video
	unsigned int scaling; // 0=integer, 1=fit, 2=TV, 3=stretch
	unsigned int filter; // 0=nearest, 1=linear, 2=TV
	bool sprite_limit;
	bool fullscreen;
	int display;
	unsigned int border_r;
	unsigned int border_g;
	unsigned int border_b;
	unsigned int margin;

	// audio
	int audio_latency;
	int audio_samplerate;
	int audio_volume; // 0 to 100

	// input
	SDL_Scancode keymap[8];
	Uint8 joymap[9];
	int joy_device;
	int axis_x;
	int axis_y;
	int hat;

	// NOTE: not saved
	bool debug;
	bool playback;
	char playback_file[PATH_LEN];
	bool protanopia, deuteranopia, tritanopia, monochrome;

	bool changed;
};

extern Config config;
extern void config_default(); // resets to default
extern void config_init(int argc, char** argv);
extern void config_save();

// end of file
