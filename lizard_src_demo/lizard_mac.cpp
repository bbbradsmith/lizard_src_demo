// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include "SDL.h"
#include "lizard_game.h"
#include "lizard_text.h"
#include "lizard_platform.h"

extern "C" void alert_objc(const char* text, const char* caption);

void debug_msg(const char* msg, ...); // forward

// preferences path

static bool prefs_path_init = false;
static char prefs_path[PATH_LEN];

static const char* get_prefs_path()
{
	if (prefs_path_init) return prefs_path;
	
	char * path = SDL_GetPrefPath("lizardnes","lizard_src_demo");
	if (path)
	{
		string_cpy(prefs_path,path,PATH_LEN);
		SDL_free(path);
	}
	else string_cpy(prefs_path,"",PATH_LEN);
	
	NES_DEBUG("get_prefs_path(): %s\n",prefs_path);
	
	prefs_path_init = true;
	return prefs_path;
}

// platform interface

void platform_setup()
{
}

void platform_loop()
{
}

void platform_shutdown()
{
}

void platform_saved()
{
}

int platform_enum()
{
	return PLATFORM_MAC;
}

void alert(const char* text, const char* caption)
{
	#ifdef _DEBUG
		debug_msg("Alert: %s : %s\n",text,caption);
	#endif
	alert_objc(text,caption);
}

void debug_break()
{
	__builtin_trap();
}

void debug_msg(const char* msg, ...)
{
	static char msg_out[256];

	va_list args;
	va_start( args, msg );
	vsnprintf( msg_out, sizeof(msg_out), msg, args );

	printf("%s", msg_out);
}

void priority_raise()
{
	SDL_SetThreadPriority(SDL_THREAD_PRIORITY_NORMAL);
}

void priority_lower()
{
	SDL_SetThreadPriority(SDL_THREAD_PRIORITY_LOW);
}

const char* get_sav_file()
{
	const char* prefs = get_prefs_path();
	static char path[PATH_LEN];
	string_cpy(path,prefs,PATH_LEN);
	string_cat(path,"lizard.sav",PATH_LEN);
	return path;
}

const char* get_ini_file()
{
	const char* prefs = get_prefs_path();
	static char path[PATH_LEN];
	string_cpy(path,prefs,PATH_LEN);
	string_cat(path,"lizard.ini",PATH_LEN);
	return path;
}

// end of file
