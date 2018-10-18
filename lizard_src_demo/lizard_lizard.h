#pragma once
// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

namespace Game
{

extern void do_scroll();
extern void lizard_init(uint16 x, uint8 y);
extern void lizard_tick();
extern void lizard_draw();
extern void lizard_die();
extern void lizard_fall_test();

}
