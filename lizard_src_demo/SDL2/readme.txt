Win32:

This folder should have, from SDL2-2.0.7:

include/

The following folders should be created and copied to (copy_libs.bat):

lib\debug\
lib\release\

SDL 2.0.7 should be compiled with:
  1. Visual Studio C++ 2013 (120_xp)
  2. Using /MT, not DLL CRT
  3. Enable SSE2 optimizations (was set to only SSE).
  4. SDL2.lib is built as a static library, not DLL
  5. Modified include/SDL_config_windows.h
    Add just beneath include guards:
      #ifndef _WINDLL
      #define HAVE_LIBC
      #endif
  6. Modified src/render/opengl/SDL_glfuncs.h
    Move two functions to the bottom of the list to prevent a crash bug on OpenGL 1.1 systems:
      SDL_PROC(void, glBlendEquation, (GLenum))
      SDL_PROC(void, glBlendFuncSeparate, (GLenum, GLenum, GLenum, GLenum))


http://www.libsdl.org/

Linux:

This folder should contain SDL2-2.0.7.
Run build_libs_linux.sh to build the necessary static libraries.

dependencies on ubuntu before build_libs_linux:
  sudo apt-get install libsdl2-dev

libsndio has been disabled, since pulseaudio seems to be the working default, and sndio is missing on Fedora.


MacOSX:

This folder should contain SDL2-2.0.7.
Run build_libs_xcode.sh to build the necessary static libraries.
