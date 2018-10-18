#pragma once
// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include "assets/export/text_set.h"

namespace Game
{

extern void text_language_by_id(uint8 id); // set language by ID, if exists (else default)
extern void text_language_advance(bool forward); // selects next language
extern uint8 text_language_id(); // current language ID
extern unsigned int text_language_count();
extern void text_language_init(); // call before loading language files
extern bool text_language_add(const char* bin, unsigned int size); // pass language file binaries to load, true if successful
extern void text_language_sort(); // sorts langauge list (use after all are loaded)
// NOTE: do not free bin passed to text_language_add
// NOTE: do not init/add/sort languages or sort after the game has begun

extern void text_init(); // called by Game::init()

extern char text_convert(char c);
extern void text_confound(uint8 seed);
extern const uint8* text_get_glyphs();
extern const char* text_get(unsigned int t); // only used by options / PC meta stuff
extern void text_load(uint8 t);
extern void text_load_meta(unsigned int t);
extern void text_start(uint8 t);
extern void text_continue();

} // namespace Game

// end of file
