#pragma once
// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

//
// Platform specific
//

enum
{
	PLATFORM_UNKNOWN = 0,
	PLATFORM_WIN32 = 1,
	PLATFORM_MAC = 2,
	PLATFORM_LINUX32 = 3,
	PLATFORM_LINUX64 = 4,
};

extern void platform_setup();
extern void platform_loop();
extern void platform_shutdown();
extern void platform_saved();
extern int platform_enum();
extern void alert(const char* text, const char* caption);
extern void debug_break();
extern void debug_msg(const char* msg, ...);
extern void priority_raise();
extern void priority_lower();
extern const char* get_sav_file();
extern const char* get_ini_file();
extern const char* get_timestamp(); // in main.cpp

// end of file
