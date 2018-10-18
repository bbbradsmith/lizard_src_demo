#pragma once
// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

extern void options_init();
extern void options_on();
extern void options_off();
extern void options_draw();
extern void options_input_poll();
extern void options_joydown(Uint8 button);
extern void options_keydown(const SDL_Keysym& key, bool repeat);

// end of file
