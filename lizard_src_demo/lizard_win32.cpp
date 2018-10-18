// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ShlObj.h> // for SHGetFolderPath
#include <DbgHelp.h> // for minidump
#include <cstdio>
#include <cstdarg>
#include "SDL.h"
#include "lizard_game.h"
#include "lizard_text.h"
#include "lizard_platform.h"

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

// exception handler

LONG WINAPI exception_handler( _In_ struct _EXCEPTION_POINTERS* ExceptionInfo )
{
	HMODULE dll = LoadLibrary("DBGHELP.DLL");
	if (dll == NULL) goto nodump;

	typedef BOOL (WINAPI* MINIDUMPWRITEDUMP)(
		_In_ HANDLE                            hProcess,
		_In_ DWORD                             ProcessId,
		_In_ HANDLE                            hFile,
		_In_ MINIDUMP_TYPE                     DumpType,
		_In_ PMINIDUMP_EXCEPTION_INFORMATION   ExceptionParam,
		_In_ PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
		_In_ PMINIDUMP_CALLBACK_INFORMATION    CallbackParam
	);

	MINIDUMPWRITEDUMP write_dump = (MINIDUMPWRITEDUMP)GetProcAddress(dll, "MiniDumpWriteDump");
	if (write_dump == NULL) goto nodump;

	HANDLE dump_file = CreateFile("LIZARD.DMP", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (dump_file == INVALID_HANDLE_VALUE) goto nodump;

	_MINIDUMP_EXCEPTION_INFORMATION ExInfo;
	ExInfo.ThreadId = GetCurrentThreadId();
	ExInfo.ExceptionPointers = ExceptionInfo;
	ExInfo.ClientPointers = NULL;

	BOOL result = write_dump(GetCurrentProcess(), GetCurrentProcessId(), dump_file, MiniDumpNormal, &ExInfo, NULL, NULL);
	if (!result) goto nodump;

	CloseHandle(dump_file);

	MessageBox(0,
		"An unexpected error has occurred.\n"
		"Please send LIZARD.DMP and a report of this problem to:\n"
		"bugs@lizardnes.com",
		"Unexpected error!",
		MB_ICONEXCLAMATION | MB_OK);

nodump:
	return EXCEPTION_CONTINUE_SEARCH;
}

// platform interface

void platform_setup()
{
	// exception handler for minidumps
	SetUnhandledExceptionFilter(exception_handler);
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
	return PLATFORM_WIN32;
}

void alert(const char* text, const char* caption)
{
	debug_msg("Alert: %s : %s\n",text,caption);
	MessageBox(0, text, caption, MB_ICONEXCLAMATION | MB_OK);
}

void debug_break()
{
	DebugBreak();
}

void debug_msg(const char* msg, ...)
{
	static char msg_out[256];

	va_list args;
	va_start( args, msg );
	vsnprintf( msg_out, sizeof(msg_out), msg, args );

	OutputDebugString(msg_out);
}

void priority_raise()
{
	SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
}

void priority_lower()
{
	SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
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
