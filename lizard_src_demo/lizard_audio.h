#pragma once
// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#define AUDIO_SAMPLERATE    48000
#define AUDIO_CHANNELS      2

extern void play_music(uint8 m);
extern void enable_music(bool enable);
extern void play_sound(uint8 s);
extern void play_bg_noise(uint8 n, uint8 v);
extern void pause_audio();
extern void unpause_audio();
extern uint8 get_next_music();
extern bool get_music_enabled();

extern void play_init();
extern void play_samplerate(int samplerate_);
extern void play_volume(int volume_); // volume should be 0 to 100
//extern void play_render(Sint16* buffer, int len); // manually extern in lizard_main.cpp

// end of file
