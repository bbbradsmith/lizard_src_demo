// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include "lizard_game.h"
#include "lizard_dogs.h"
#include "lizard_audio.h"
#include "lizard_lizard.h"
#include "lizard_text.h"
#include "lizard_ppu.h"
#include "lizard_version.h"
#include "assets/export/data.h"
#include "assets/export/data_music.h"
#include "enums.h"

extern bool system_pal(); // in lizard_main.cpp
extern bool system_easy(); // in lizard_main.cpp

// bound checking for dog data access
inline uint8& dog_data(int x, uint8 d)
{
	NES_ASSERT(x >= 0 && x < 14, "dog_data index out of bounds!");
	NES_ASSERT(d < 16, "dog_data dog out of bounds!");
	return (Game::h.dog_data[x][d]);
}
inline Array16& dog_x(uint8 d)     { NES_ASSERT(d<16, "dog_x out of bounds!");     return Game::h_dog_x[d];     }
inline uint8&   dog_y(uint8 d)     { NES_ASSERT(d<16, "dog_y out of bounds!");     return Game::h.dog_y[d];     }
inline uint8&   dog_param(uint8 d) { NES_ASSERT(d<16, "dog_param out of bounds!"); return Game::h.dog_param[d]; }
inline uint8&   dog_type(uint8 d)  { NES_ASSERT(d<16, "dog_type out of bounds!");  return Game::h.dog_type[d];  }

// shorthand for accessing dog data
#define dgx dog_x(d)
#define dgy dog_y(d)
#define dgp dog_param(d)
#define dgt dog_type(d)
#define dgd(a) dog_data(a,d)
#define dgd_get16(ah,al) (uint16(dog_data(ah,d)<<8) | dog_data(al,d))
#define dgd_put16(v,ah,al) {dog_data(ah,d)=((v)>>8)&0xFF; dog_data(al,d)=((v)&0xFF);}
#define dgd_get24x(a) (uint32(dgx<<8) | dog_data(a,d))
#define dgd_get16x(a) (uint16(dgx<<8) | dog_data(a,d))
#define dgd_get16y(a) (uint16(dog_y(d)<<8) | dog_data(a,d))
#define dgd_put24x(v,a) {dgx=((v)>>8)&0xFFFF; dog_data(a,d)=((v)&0xFFFF);}
#define dgd_put16x(v,a) {dgx=((v)>>8)&0xFF; dog_data(a,d)=((v)&0xFF);}
#define dgd_put16y(v,a) {dog_y(d)=((v)>>8)&0xFF; dog_data(a,d)=((v)&0xFF);}

// creates variable dx, scrolled screen position, returns if offscreen
#define dx_scroll()                      uint8 dx; if (dx_scroll_(dx,dgx)) return;
#define dx_scroll_offset(offset_)        uint8 dx; if (dx_scroll_(dx,dgx + (offset_))) return;
#define dx_scroll_edge()                 uint8 dx; if (dx_scroll_edge_(dx,dgx)) return;
#define dx_scroll_river()                uint8 dx; if (dx_scroll_river_(dx,dgx)) return;
#define dx_screen()                      uint8 dx;     dx_screen_(dx,dgx);
#define play_sound_scroll(x)             play_sound_scroll_(d,x)

namespace Game
{

//
// Dog helpers
//

void dx_screen_(uint8 &dx, uint16 x)
{
	dx = uint8(x);
	zp.sprite_center = (dx ^ (dx >> 1)) & 0xC0;
}

bool dx_scroll_(uint8 &dx, uint16 x)
{
	dx = x - zp.scroll_x; // low byte
	uint8 dxh = (x - zp.scroll_x) >> 8; // high byte

	if (dxh == 0)
	{
		zp.sprite_center = (dx ^ (dx >> 1)) & 0xC0;
		return false;
	}
	else
	{
		return true;
	}
}

bool dx_scroll_edge_(uint8 &dx, uint16 x)
{
	dx = x - zp.scroll_x; // low byte
	uint8 dxh = (x - zp.scroll_x) >> 8; // high byte

	if (dxh == 0)
	{
		zp.sprite_center = (dx ^ (dx >> 1)) & 0xC0;
		return false;
	}
	else
	{
		if (dxh & 0x80) // scrolled off on left
		{
			if ((dx & 0xC0) != 0xC0) return true;
			// on right quarter of imaginary screen to left
		}
		else // scrolled off on right
		{
			if ((dx & 0xC0) != 0x00) return true;
			// on left quarter of imaginary screen to right
		}
		zp.sprite_center = (dx ^ 0x80) & 0x80;
		return false;
	}
}

bool dx_scroll_river_(uint8 &dx, uint16 x)
{
	dx = x & 0xFF; // low byte
	uint8 dxh = x >> 8; // high byte

	if (dxh == 0)
	{
		zp.sprite_center = (dx ^ (dx >> 1)) & 0xC0;
		return false;
	}
	else
	{
		if (dxh & 0x80) // scrolled off on left
		{
			if ((dx & 0xC0) != 0xC0) return true;
		}
		else // scrolled off on right
		{
			if ((dx & 0xC0) != 0x00) return true;
		}
		zp.sprite_center = (dx ^ 0x80) & 0x80;
		return false;
	}
}

bool lizard_overlap(uint16 x0, uint8 y0, uint16 x1, uint8 y1)
{
	if (x0 >= (128<<8)) x0 = 0;
	if (x1 >= (128<<8)) x1 = 0;

	PPU::debug_box(x0,y0,x1,y1);

	if (lizard.hitbox_x1 < x0) return false;
	if (lizard.hitbox_y1 < y0) return false;
	if (lizard.hitbox_x0 > x1) return false;
	if (lizard.hitbox_y0 > y1) return false;
	if (lizard.dead) return false;
	return true;
}

bool lizard_overlap_no_stone(uint16 x0, uint8 y0, uint16 x1, uint8 y1)
{
	if (zp.current_lizard == LIZARD_OF_STONE && lizard.power > 0) return false;
	return lizard_overlap(x0,y0,x1,y1);
}

bool lizard_touch(uint16 x0, uint8 y0, uint16 x1, uint8 y1)
{
	if (x0 >= (128<<8)) x0 = 0;
	if (x1 >= (128<<8)) x1 = 0;

	PPU::debug_box(x0,y0,x1,y1);

	if (lizard.touch_x1 < x0) return false;
	if (lizard.touch_y1 < y0) return false;
	if (lizard.touch_x0 > x1) return false;
	if (lizard.touch_y0 > y1) return false;
	return true;
}

void lizard_x_inc()
{
	lizard.x += 1 << 8;
	do_scroll();
}

void lizard_x_dec()
{
	lizard.x -= 1 << 8;
	do_scroll();
}

bool bound_overlap(uint8 d, sint16 x0, sint8 y0, sint16 x1, sint8 y1)
{
	return lizard_overlap(dgx+x0,dgy+y0,dgx+x1,dgy+y1);
}

bool bound_overlap_no_stone(uint8 d, sint16 x0, sint8 y0, sint16 x1, sint8 y1)
{
	return lizard_overlap_no_stone(dgx+x0,dgy+y0,dgx+x1,dgy+y1);
}

bool bound_touch(uint8 d, sint16 x0, sint8 y0, sint16 x1, sint8 y1)
{
	if (lizard.power < 1) return false;
	return lizard_touch(dgx+x0,dgy+y0,dgx+x1,dgy+y1);
}

bool bound_touch_death(uint8 d, sint16 x0, sint8 y0, sint16 x1, sint8 y1)
{
	if (zp.current_lizard != LIZARD_OF_DEATH) return false;
	return bound_touch(d,x0,y0,x1,y1);
}

void dog_blocker(uint8 d, sint8 x0, sint8 y0, sint8 x1, sint8 y1)
{
	h_blocker_x0[d&3] = dgx + x0;
	h_blocker_x1[d&3] = dgx + x1;
	h.blocker_y0[d&3] = dgy + y0;
	h.blocker_y1[d&3] = dgy + y1;
	NES_ASSERT(h_blocker_x0[d&3] < (128 << 8),"Blocker off left side of room!");
}

void empty_blocker(uint8 d)
{
	h_blocker_x0[d&3] = (127 << 8) | 255;
	h.blocker_y0[d&3] = 255;
	h_blocker_x1[d&3] = (127 << 8) | 255;;
	h.blocker_y1[d&3] = 255;
}

void empty_dog(uint8 d)
{
	dgt = DOG_NONE;
}

void zero_dog_data(uint8 d)
{
	for (uint8 x=0; x <14; ++x)
	{
		dog_data(x,d) = 0;
	}
}

void play_sound_scroll_(uint8 d, uint8 sound)
{
	dx_scroll();
	play_sound(sound);
}

void text_row_write(uint8 text)
{
	text_load(text);
	ppu_nmi_write_row();
}

const sint8 circle32[256] = {
	   0,   1,   2,   2,   3,   4,   5,   5,   6,   7,   8,   8,   9,  10,  11,  11,
	  12,  13,  13,  14,  15,  16,  16,  17,  18,  18,  19,  19,  20,  21,  21,  22,
	  22,  23,  23,  24,  24,  25,  25,  26,  26,  27,  27,  27,  28,  28,  29,  29,
	  29,  29,  30,  30,  30,  30,  31,  31,  31,  31,  31,  31,  31,  31,  32,  32,
	  32,  32,  32,  31,  31,  31,  31,  31,  31,  31,  31,  30,  30,  30,  30,  29,
	  29,  29,  29,  28,  28,  27,  27,  27,  26,  26,  25,  25,  24,  24,  23,  23,
	  22,  22,  21,  21,  20,  19,  19,  18,  18,  17,  16,  16,  15,  14,  13,  13,
	  12,  11,  11,  10,   9,   8,   8,   7,   6,   5,   5,   4,   3,   2,   2,   1,
	   0,  -1,  -2,  -2,  -3,  -4,  -5,  -5,  -6,  -7,  -8,  -8,  -9, -10, -11, -11,
	 -12, -13, -13, -14, -15, -16, -16, -17, -18, -18, -19, -19, -20, -21, -21, -22,
	 -22, -23, -23, -24, -24, -25, -25, -26, -26, -27, -27, -27, -28, -28, -29, -29,
	 -29, -29, -30, -30, -30, -30, -31, -31, -31, -31, -31, -31, -31, -31, -32, -32,
	 -32, -32, -32, -31, -31, -31, -31, -31, -31, -31, -31, -30, -30, -30, -30, -29,
	 -29, -29, -29, -28, -28, -27, -27, -27, -26, -26, -25, -25, -24, -24, -23, -23,
	 -22, -22, -21, -21, -20, -19, -19, -18, -18, -17, -16, -16, -15, -14, -13, -13,
	 -12, -11, -11, -10,  -9,  -8,  -8,  -7,  -6,  -5,  -5,  -4,  -3,  -2,  -2,  -1,
};

const sint8 circle108[256] = {
	   0,   3,   5,   8,  11,  13,  16,  18,  21,  24,  26,  29,  31,  34,  36,  39,
	  41,  44,  46,  48,  51,  53,  55,  58,  60,  62,  64,  66,  68,  70,  72,  74,
	  76,  78,  80,  81,  83,  85,  86,  88,  89,  91,  92,  94,  95,  96,  97,  98,
	  99, 100, 101, 102, 103, 104, 104, 105, 105, 106, 106, 107, 107, 107, 107, 108,
	 108, 108, 107, 107, 107, 107, 106, 106, 105, 105, 104, 104, 103, 102, 101, 100,
	  99,  98,  97,  96,  95,  94,  92,  91,  89,  88,  86,  85,  83,  81,  80,  78,
	  76,  74,  72,  70,  68,  66,  64,  62,  60,  58,  55,  53,  51,  48,  46,  44,
	  41,  39,  36,  34,  31,  29,  26,  24,  21,  18,  16,  13,  11,   8,   5,   3,
	   0,  -3,  -5,  -8, -11, -13, -16, -18, -21, -24, -26, -29, -31, -34, -36, -39,
	 -41, -44, -46, -48, -51, -53, -55, -58, -60, -62, -64, -66, -68, -70, -72, -74,
	 -76, -78, -80, -81, -83, -85, -86, -88, -89, -91, -92, -94, -95, -96, -97, -98,
	 -99,-100,-101,-102,-103,-104,-104,-105,-105,-106,-106,-107,-107,-107,-107,-108,
	-108,-108,-107,-107,-107,-107,-106,-106,-105,-105,-104,-104,-103,-102,-101,-100,
	 -99, -98, -97, -96, -95, -94, -92, -91, -89, -88, -86, -85, -83, -81, -80, -78,
	 -76, -74, -72, -70, -68, -66, -64, -62, -60, -58, -55, -53, -51, -48, -46, -44,
	 -41, -39, -36, -34, -31, -29, -26, -24, -21, -18, -16, -13, -11,  -8,  -5,  -3,
};

const sint8 circle4[256] = {
	   0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
	   1,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   3,   3,
	   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
	   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,
	   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,
	   4,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
	   3,   3,   3,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,
	   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0,   0,   0,   0,   0,
	   0,   0,   0,   0,   0,   0,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
	  -1,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -3,  -3,
	  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,
	  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,
	  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,
	  -4,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,
	  -3,  -3,  -3,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,
	  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   0,   0,   0,   0,   0,
};

extern const uint8 hex_to_ascii[16] = { 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46 };

//
// Dogs
//

// many dogs can convert to DOG_BONES, these are the important data slots when converting:
const int BONES_INIT = 0; // data slot for bones, 0 = not initialized
const int BONES_FLIP = 1; // data slot for bones, 1 = flip sprite
// the dog param is the bones sprite
void dog_tick_bones(uint8 d); // forward declaration

void bones_convert_silent(uint8 d, uint8 sprite, uint8 flip)
{
	dgt = DOG_BONES;
	dgp = sprite;
	dgd(BONES_FLIP) = flip;
	dgd(BONES_INIT) = 0;
	dog_tick_bones(d);
}

void bones_convert(uint8 d, uint8 sprite, uint8 flip)
{
	bones_convert_silent(d,sprite,flip);
	play_sound(SOUND_BONES);
	++h.metric_bones;
}

// push helpers

// generic fall test, tests 3 points on bottom of bounding box + lizard
bool do_fall(uint8 d, sint8 x0, sint8 y0, sint8 x1, sint8 y1)
{
	if (dgy+y1+1 >= 240) return true; // bottom of screen

	uint16 mid_x = (dgx + dgx + x0 + x1) >> 1;
	if (
		!bound_overlap(d,x0,y0+1,x1,y1+1) &&
		!collide_all(dgx+x0,dgy+y1+1) &&
		!collide_all(dgx+x1,dgy+y1+1) &&
		!collide_all(mid_x,dgy+y1+1))
	{
		return true;
	}
	return false;
}

// generic push test, tests for lizard push, then tests 3 points on pushed side
uint8 do_push(uint8 d, sint8 x0, sint8 y0, sint8 x1, sint8 y1)
{
	if (zp.current_lizard != LIZARD_OF_PUSH) return 0;
	if (lizard.power < 1) return 0;

	uint8 mid_y = (((dgy+y0)&0xFF)>>1) + (((dgy+y1)&0xFF)>>1);
	if (zp.gamepad & PAD_R)
	{
		if ((!zp.room_scrolling && (dgx+x1) >= 255) || ((dgx+x1) >= 511)) return 0;

		if (bound_overlap(d,x0-1,y0,x0-1,y1) &&
			!collide_all(dgx+x1+1,dgy+y0) &&
			!collide_all(dgx+x1+1,dgy+y1) &&
			!collide_all(dgx+x1+1,mid_y))
		{
			play_sound(SOUND_PUSH);
			return 1;
		}
	}
	else if (zp.gamepad & PAD_L)
	{
		if ((dgx+x0) < 2) return 0;

		if (bound_overlap(d,x1+1,y0,x1+1,y1) &&
			!collide_all(dgx+x0-1,dgy+y0) &&
			!collide_all(dgx+x0-1,dgy+y1) &&
			!collide_all(dgx+x0-1,mid_y))
		{
			play_sound(SOUND_PUSH);
			return 2;
		}
	}
	return 0;
}

// movement helper
// params: current bounding box, dx/dy
// returns: revised dx/dy and bitfield if collision happened %0000udlr
// note: if dog is a blocker, empty_blocker before calling this (otherwise self-collision will result)
// note: if dog is blocker and should not push lizard, swap lizard hitbox into blocker pos first
uint8 move_dog(uint8 d, sint8 x0, sint8 y0, sint8 x1, sint8 y1, sint8* pdx, sint8* pdy)
{
	NES_ASSERT(pdx && pdy, "move_dog with NULL parameters!");

	uint8 collide = 0;

	sint8 dx = *pdx;
	sint8 dy = *pdy;

	uint16 xe0 = dgx + x0;
	uint16 xe1 = dgx + x1;
	y0 += dgy;
	y1 += dgy;

	uint16 xem = (xe0 + xe1) >> 1;
	uint8  ym  = ((y0 & 0xFF)>>1) + ((y1 & 0xFF)>>1);

	// NOTE: if a wrap over 0 is caused by pdx/pdy,
	//       ym might be centre screen,
	//       xem might be way out of bounds
	// am intentionally ignoring this case, preventing it upstream instead
	// (voidball is the only move_dog that has a problem with this)

	if (dx > 0)
	{
		uint8 shift;
		shift = collide_all_right(xe1+dx,y0); if (shift) { dx -= shift; collide |= 1; }
		shift = collide_all_right(xe1+dx,y1); if (shift) { dx -= shift; collide |= 1; }
		shift = collide_all_right(xe1+dx,ym); if (shift) { dx -= shift; collide |= 1; }
	}
	else if (dx < 0)
	{
		uint8 shift;
		shift = collide_all_left(xe0+dx,y0); if (shift) { dx += shift; collide |= 2; }
		shift = collide_all_left(xe0+dx,y1); if (shift) { dx += shift; collide |= 2; }
		shift = collide_all_left(xe0+dx,ym); if (shift) { dx += shift; collide |= 2; }
	}

	xe0 += dx;
	xem += dx;
	xe1 += dx;

	if (dy > 0)
	{
		uint8 shift;
		shift = collide_all_down(xe0,y1+dy); if (shift) { dy -= shift; collide |= 4; }
		shift = collide_all_down(xe1,y1+dy); if (shift) { dy -= shift; collide |= 4; }
		shift = collide_all_down(xem,y1+dy); if (shift) { dy -= shift; collide |= 4; }
	}
	else if (dy < 0)
	{
		uint8 shift;
		shift = collide_all_up(xe0,y0+dy); if (shift) { dy += shift; collide |= 8; }
		shift = collide_all_up(xe1,y0+dy); if (shift) { dy += shift; collide |= 8; }
		shift = collide_all_up(xem,y0+dy); if (shift) { dy += shift; collide |= 8; }
	}

	*pdx = dx;
	*pdy = dy;
	return collide;
}

void dog_init_boss_door_out_partial(bool verso)
{
	if (h.boss_rush)
	{
		// NOT IN DEMO
	}
}

void dog_init_boss_door(uint8 d); // forward declaration

void dog_init_boss_door_out(uint8 d, bool verso)
{
	dog_init_boss_door_out_partial(verso);
	dog_param(d) = 6;
	dog_init_boss_door(d);
}

//
// BANK0 ($E)
//

// DOG_NONE

void dog_init_none(uint8 d)
{
}
void dog_tick_none(uint8 d)
{
}
void dog_draw_none(uint8 d)
{
}

// DOG_DOOR

const int DOOR_ON = 0;
void dog_init_door(uint8 d) {}
void dog_tick_door(uint8 d)
{
	if ((zp.gamepad & PAD_U) == 0) // up is released
	{
		dgd(DOOR_ON) = 1;
	}
	if (dgd(DOOR_ON) == 0) return; // do not work until released

	if (zp.gamepad & PAD_U) // want to enter door
	{
		if (bound_overlap(d,0,0,15,15))
		{
			zp.room_change = 2;
			zp.current_door = dgp & 7;
			zp.current_room = h_door_link[zp.current_door];
		}
	}
}
void dog_draw_door(uint8 d) {}

// DOG_PASS

void dog_init_pass(uint8 d) {}
void dog_tick_pass(uint8 d)
{
	if (bound_overlap(d,0,0,15,15))
	{
		zp.current_door = dgp & 7;
		zp.current_room = h_door_link[zp.current_door];
		zp.room_change = 1;
	}
}
void dog_draw_pass(uint8 d) {}

// DOG_PASS_X

void dog_init_pass_x(uint8 d)
{
	dgy = 0;
}
void dog_tick_pass_x(uint8 d)
{
	if (lizard_overlap(dgx+0,0,dgx+7,255))
	{
		zp.current_door = dgp & 7;
		zp.current_room = h_door_link[zp.current_door];
		zp.hold_x = 1;
		zp.room_change = 1;
	}
}
void dog_draw_pass_x(uint8 d) {}

// DOG_PASS_Y

void dog_init_pass_y(uint8 d)
{
	dgx = 0;
}
void dog_tick_pass_y(uint8 d)
{
	if (lizard_overlap(0,dgy+0,511,dgy+7))
	{
		zp.current_door = dgp & 7;
		zp.current_room = h_door_link[zp.current_door];
		zp.hold_y = 1;
		zp.room_change = 1;

		// upward transition will force holding a direction and A across the screen transition to assist in the jump
		if (lizard_py < 128)
		{
			zp.climb_assist_time = 9;
			zp.climb_assist = zp.gamepad & (PAD_L | PAD_R | PAD_A);
		}
	}
}
void dog_draw_pass_y(uint8 d) {}

// DOG_PASSWORD_DOOR

const int PASSWORD_DOOR_ON = 0;
void dog_init_password_door(uint8 d)
{
	PPU::latch_at(3,5);
	text_load(TEXT_CONTINUE);

	for (int i=3; i<32; ++i)
	{
		PPU::write(stack.nmi_update[i]);
	}
}
void dog_tick_password_door(uint8 d)
{
	if ((zp.gamepad & PAD_U) == 0) // up is released
	{
		dgd(PASSWORD_DOOR_ON) = 1;
	}
	if (dgd(PASSWORD_DOOR_ON) == 0) return; // do not work until released

	if (zp.gamepad & PAD_U) // want to enter door
	{
		if (bound_overlap(d,0,0,15,15))
		{
			zp.current_door = 0;

			// store and gather password
			for (uint8 x=0; x<5; ++x)
			{
				stack.scratch[x] = zp.password[x];
			}
			for (uint8 x=0; x<16; ++x)
			{
				if (dog_type(x) == DOG_PASSWORD && dog_param(x) < 5)
				{
					zp.password[dog_param(x)] = dog_data(PASSWORD_VALUE,x);
				}
			}

			uint16 nr;
			uint8 nl;
			if (password_read(&nr,&nl))
			{
				if (checkpoint(nr) || // SELECT+A bypasses checkpoint test
					((zp.gamepad & (PAD_SELECT|PAD_A)) == (PAD_SELECT|PAD_A)))
				{
					// can't continue with LIZARD_OF_BEYOND unless you have the coins
					if (nl >= LIZARD_OF_BEYOND && coin_count() < PIGGY_BURST)
					{
						nl = LIZARD_OF_KNOWLEDGE;
					}

					zp.current_room = nr;
					zp.next_lizard = nl;
					zp.continued = 1;

					if (!checkpoint(nr))
					{
						h.metric_cheater = 1;
					}
					++h.metric_continue;

					for (int i=0; i<16; ++i) h.coin_save[i] = h.coin[i];
					for (int i=0; i<16; ++i) h.flag_save[i] = h.flag[i];
					h.piggy_bank_save = h.piggy_bank;
					h.last_lizard_save = h.last_lizard;
					zp.coin_saved = 1;
					h.tip_counter = 0;
					zp.room_change = 2;

					// can't switch current_lizard until we've faded out
					// (draw is going to change palette)
					uint8 temp = zp.current_lizard;
					zp.current_lizard = zp.next_lizard;
					password_build();
					zp.current_lizard = temp;

					resume_point();

					return;
				}
				else
				{
					zp.room_change = 1;
					play_sound(SOUND_NO);
				}
			}
			else
			{
				zp.room_change = 1;
				play_sound(SOUND_NO);
			}

			// restore password
			for (uint8 x=0; x<5; ++x)
			{
				zp.password[x] = stack.scratch[x];
			}
		}
	}
}
void dog_draw_password_door(uint8 d) {}

// DOG_LIZARD_EMPTY_LEFT

const uint8 LIZARD_SWAP_SPEED = 5;
const int EMPTY_ON = 0;
const int EMPTY_SPEED = 1;
const int DISMOUNT_ANIM = 0;
const int DISMOUNT_FLIP = 1;
const int DISMOUNT_SLID = 2;

CT_ASSERT(((13-4)*LIZARD_SWAP_SPEED) > 42,"Dismount time not long enough to scroll.");

void dog_dismounter_locate()
{
	dog_x(DISMOUNT_SLOT) = lizard_px;
	dog_y(DISMOUNT_SLOT) = lizard_py;
}
void dog_dismounter_start(uint8 d)
{
	if (lizard.dead != 0) return;
	lizard.dismount = 1;
	lizard.vy = 0;
	lizard.vx = 0;
	lizard.skid = 0;
	lizard.fall = 0;
	dog_data(DISMOUNT_ANIM,DISMOUNT_SLOT) = 0; // reset dismounter animation
	dog_data(DISMOUNT_SLID,DISMOUNT_SLOT) = 0;
	dog_data(DISMOUNT_FLIP,DISMOUNT_SLOT) = lizard.face;
	dgd(EMPTY_SPEED) = 0;
	dog_dismounter_locate();
}
void dog_dismounter_flip(uint8 d)
{
	uint8 temp = dgp;
	palette_load(4,DATA_palette_lizard0 + dgp);
	palette_load(6,DATA_palette_lizard0 + zp.current_lizard);

	if (dgp == h.last_lizard)
	{
		// when the last lizard is recovered, make sure it doesn't appear anywhere else
		h.last_lizard = 0xFF;
	}
	else
	{
		h.last_lizard = zp.current_lizard;
	}
	dgp = zp.current_lizard;
	zp.current_lizard = temp;
	zp.next_lizard = temp;

	lizard.power = 0;
	dog_dismounter_locate();
}

void dog_init_lizard_empty_left(uint8 d)
{
	if (dgp == zp.current_lizard)
	{
		if (h.last_lizard == 0xFF)
		{
			empty_dog(d);
			return;
		}
		else
		{
			dgp = h.last_lizard;
			dgt = DOG_LIZARD_EMPTY_RIGHT;
			dgx -= 32;
		}
	}

	NES_ASSERT(dog_type(DISMOUNT_SLOT)==DOG_LIZARD_DISMOUNTER,"lizard_empty_left requires dismounter in DISMOUNT_SLOT");
	palette_load(6,DATA_palette_lizard0 + dgp);
	dgd(EMPTY_ON   ) = 1; // player on lizard, suppress dismount
	dgd(EMPTY_SPEED) = 0; // speed counter
}
void dog_tick_lizard_empty_left(uint8 d)
{
	if (lizard.dismount == 0)
	{
		if (bound_touch_death(d,-12,-5,9,-1))
		{
			if (dgp == h.last_lizard)
			{
				h.last_lizard = 0xFF;
			}
			dgy += 5;
			bones_convert(d,DATA_sprite0_lizard_skull_dismount,1);
			return;
		}

		if (lizard.face == 0 &&
		    lizard_py == dgy &&
		   (lizard_px >= uint16(dgx - 37) && lizard_px < uint16(dgx-25)))
		{
			if (dgd(EMPTY_ON) == 0)
			{
				lizard.face = 0;
				dog_dismounter_start(d);
			}
		}
		else dgd(EMPTY_ON) = 0;
	}
	else
	{
		if (dog_data(DISMOUNT_ANIM,DISMOUNT_SLOT) >= 4 &&
			dog_data(DISMOUNT_SLID,DISMOUNT_SLOT) < 42)
		{
			lizard_x_inc();
			++dog_data(DISMOUNT_SLID,DISMOUNT_SLOT);
		}

		++dgd(EMPTY_SPEED);
		if (dgd(EMPTY_SPEED) < LIZARD_SWAP_SPEED) return;
		dgd(EMPTY_SPEED) = 0;

		if (dog_data(DISMOUNT_ANIM,DISMOUNT_SLOT) < 4)
		{
			if (lizard_px > uint16(dgx-37))
			{
				lizard_x_dec();
				dog_dismounter_locate();

				++dog_data(DISMOUNT_ANIM,DISMOUNT_SLOT);
				dog_data(DISMOUNT_ANIM,DISMOUNT_SLOT) &= 3;
			}
			else dog_data(DISMOUNT_ANIM,DISMOUNT_SLOT) = 4;
		}
		else
		{
			++dog_data(DISMOUNT_ANIM,DISMOUNT_SLOT);
			if (dog_data(DISMOUNT_ANIM,DISMOUNT_SLOT) == 13) // middle of dismount, do swap
			{
				dog_dismounter_flip(d);
				dgt = DOG_LIZARD_EMPTY_RIGHT;
				dgx -= 32;
				dog_data(DISMOUNT_FLIP,DISMOUNT_SLOT) = 1; // flip

			}
			else if (dog_data(DISMOUNT_ANIM,DISMOUNT_SLOT) >= 21) // end of dismount (note type of dog has changed)
			{
				lizard.dismount = 0;
				lizard.face = 0;
				dgd(EMPTY_ON) = 1; // suppress remount
			}
		}
	}
}
void dog_draw_lizard_empty_left(uint8 d)
{
	dx_scroll_edge();
	sprite0_add(dx, dgy, DATA_sprite0_lizard_empty_left);
}

// DOG_LIZARD_EMPTY_RIGHT

void dog_init_lizard_empty_right(uint8 d)
{
	if (dgp == zp.current_lizard)
	{
		if (h.last_lizard == 0xFF)
		{
			empty_dog(d);
			return;
		}
		else
		{
			dgp = h.last_lizard;
			dgt = DOG_LIZARD_EMPTY_LEFT;
			dgx += 32;
		}
	}

	NES_ASSERT(dog_type(DISMOUNT_SLOT)==DOG_LIZARD_DISMOUNTER,"lizard_empty_right requires dismounter in DISMOUNT_SLOT");
	palette_load(6,DATA_palette_lizard0 + dgp);
	dgd(EMPTY_ON   ) = 1; // player on lizard, suppress dismount
	dgd(EMPTY_SPEED) = 0; // speed counter
}
void dog_tick_lizard_empty_right(uint8 d)
{
	if (lizard.dismount == 0)
	{
		if (bound_touch_death(d,-10,-5,11,-1))
		{
			if (dgp == h.last_lizard)
			{
				h.last_lizard = 0xFF;
			}
			dgy += 5;
			bones_convert(d,DATA_sprite0_lizard_skull_dismount,0);
			return;
		}

		if (lizard.face == 1 &&
		    lizard_py == dgy &&
		   (lizard_px <= uint16(dgx + 37) && lizard_px > uint16(dgx+25)))
		{
			if (dgd(EMPTY_ON) == 0)
			{
				lizard.face = 1;
				dog_dismounter_start(d);
			}
		}
		else dgd(EMPTY_ON) = 0;
	}
	else
	{
		if (dog_data(DISMOUNT_ANIM,DISMOUNT_SLOT) >= 4 &&
			dog_data(DISMOUNT_SLID,DISMOUNT_SLOT) < 42)
		{
			lizard_x_dec();
			++dog_data(DISMOUNT_SLID,DISMOUNT_SLOT);
		}

		++dgd(EMPTY_SPEED);
		if (dgd(EMPTY_SPEED) < LIZARD_SWAP_SPEED) return;
		dgd(EMPTY_SPEED) = 0;

		if (dog_data(DISMOUNT_ANIM,DISMOUNT_SLOT) < 4)
		{
			if (lizard_px < uint16(dgx + 37))
			{
				lizard_x_inc();
				dog_dismounter_locate();

				++dog_data(DISMOUNT_ANIM,DISMOUNT_SLOT);
				dog_data(DISMOUNT_ANIM,DISMOUNT_SLOT) &= 3;
			}
			else dog_data(DISMOUNT_ANIM,DISMOUNT_SLOT) = 4;
		}
		else
		{
			++dog_data(DISMOUNT_ANIM,DISMOUNT_SLOT);
			if (dog_data(DISMOUNT_ANIM,DISMOUNT_SLOT) == 13) // middle of dismount, do swap
			{
				dog_dismounter_flip(d);
				dgt = DOG_LIZARD_EMPTY_LEFT;
				dgx += 32;
				dog_data(DISMOUNT_FLIP,DISMOUNT_SLOT) = 0; // no flip
			}
			else if (dog_data(DISMOUNT_ANIM,DISMOUNT_SLOT) >= 21) // end of dismount (note type of dog has changed)
			{
				lizard.dismount = 0;
				lizard.face = 1;
				dgd(EMPTY_ON) = 1; // suppress remount
			}
		}
	}
}
void dog_draw_lizard_empty_right(uint8 d)
{
	dx_scroll_edge();
	sprite0_add(dx, dgy, DATA_sprite0_lizard_empty_right);
}

// DOG_LIZARD_DISMOUNTER

const uint8 HAIR_COLOR[64] = {
	// light skin hair
	0x16,0x16,0x06,0x17,0x17,0x07,0x07,0x28,0x28,0x18,0x01,0x01,0x00,0x29,0x21,0x25,
	0x16,0x16,0x06,0x17,0x17,0x07,0x07,0x28,0x28,0x18,0x01,0x01,0x00,0x00,0x18,0x06,
	// dark skin hair
	0x06,0x06,0x07,0x07,0x07,0x27,0x27,0x28,0x28,0x05,0x04,0x00,0x00,0x29,0x21,0x25,
	0x06,0x06,0x07,0x07,0x07,0x27,0x27,0x28,0x28,0x05,0x04,0x00,0x00,0x06,0x06,0x05,
};

uint8 get_hair_color()
{
	uint8 hair = (h.human0_hair & 31) | ((zp.human0_pal & 1) << 5);
	return HAIR_COLOR[hair];
}

void dog_init_lizard_dismounter(uint8 d)
{
	NES_ASSERT(d==DISMOUNT_SLOT,"lizard_dismounter must be in slot DISMOUNT_SLOT");
	for (int i=0; i<16; ++i)
	{
		if (dog_type(i) == DOG_DIAGNOSTIC)
		{
			palette_load(6,DATA_palette_human0 + zp.human0_pal);
			h.palette[27] = get_hair_color();
			return;
		}
	}
}
void dog_tick_lizard_dismounter(uint8 d) {}
void dog_draw_lizard_dismounter(uint8 d)
{
	// dismounter is responsible for drawing character during dismount
	// so that it is not prioritized and can flicker properly

	if (lizard.dismount == 0) return; // not active

	dx_scroll_edge();

	uint8 sprite = DATA_sprite0_lizard_stand_dismount;
	switch (dgd(DISMOUNT_ANIM))
	{
	case  0: sprite = DATA_sprite0_lizard_walk0_dismount; break;
	case  1: sprite = DATA_sprite0_lizard_walk1_dismount; break;
	case  2: sprite = DATA_sprite0_lizard_walk2_dismount; break;
	case  3: sprite = DATA_sprite0_lizard_walk3_dismount; break;
	case  4: sprite = DATA_sprite0_lizard_dismount0;      break;
	case  5: sprite = DATA_sprite0_lizard_dismount1;      break;
	case  6: sprite = DATA_sprite0_lizard_dismount2;      break;
	case  7: sprite = DATA_sprite0_lizard_dismount3;      break;
	case  8: sprite = DATA_sprite0_lizard_dismount2;      break;
	case  9: sprite = DATA_sprite0_lizard_dismount4;      break;
	case 10: sprite = DATA_sprite0_lizard_dismount5;      break;
	case 11: sprite = DATA_sprite0_lizard_dismount6;      break;
	case 12: sprite = DATA_sprite0_lizard_dismount7;      break;
	case 13: sprite = DATA_sprite0_lizard_dismount7;      break;
	case 14: sprite = DATA_sprite0_lizard_dismount6;      break;
	case 15: sprite = DATA_sprite0_lizard_dismount5;      break;
	case 16: sprite = DATA_sprite0_lizard_dismount4;      break;
	case 17: sprite = DATA_sprite0_lizard_dismount3;      break;
	case 18: sprite = DATA_sprite0_lizard_dismount2;      break;
	case 19: sprite = DATA_sprite0_lizard_dismount1;      break;
	case 20: sprite = DATA_sprite0_lizard_dismount0;      break;
	default: break;
	}

	if (sprite == DATA_sprite0_lizard_dismount6 || sprite == DATA_sprite0_lizard_dismount7)
	{
		h.palette[23] = get_hair_color();
	}

	if (dgd(DISMOUNT_FLIP) == 0)
		sprite0_add(dx,dgy,sprite);
	else
		sprite0_add_flipped(dx,dgy,sprite);
}

// DOG_SPLASHER

const int SPLASHER_ANIM     = 0;
const int SPLASHER_BUBBLE   = 1;
const int SPLASHER_BUBBLE_X = 2;
const int SPLASHER_BUBBLE_Y = 3;
const int SPLASHER_SCROLL   = 4;
void dog_init_splasher(uint8 d)
{
	NES_ASSERT(d == SPLASHER_SLOT,"splasher must be in slot SPLASHER_SLOT");
	dgd(SPLASHER_ANIM) = 11;
	dgd(SPLASHER_SCROLL) = uint8(zp.scroll_x);
}
void dog_tick_splasher(uint8 d)
{
	// splash
	if (dgd(SPLASHER_ANIM)<11) ++dgd(SPLASHER_ANIM);

	// bubble
	if (dgd(SPLASHER_BUBBLE) == 0)
	{
		if (lizard.wet != 0 && (prng() < 2) && lizard.dead == 0)
		{
			dgd(SPLASHER_BUBBLE) = 1;
			dgd(SPLASHER_BUBBLE_X) = uint8(lizard_px - zp.scroll_x) - 4;
			dgd(SPLASHER_BUBBLE_Y) = lizard_py - 18;
			dgd(SPLASHER_SCROLL)   = uint8(zp.scroll_x);
		}
		else
		{
			return;
		}
	}

	uint8 ox = dgd(SPLASHER_BUBBLE_X);

	prng(); // extra entropy
	switch (prng() & 7)
	{
		case 0: ++dgd(SPLASHER_BUBBLE_X); break;
		case 1: --dgd(SPLASHER_BUBBLE_X); break;
		case 2:
		case 3:
		default:
			break;
	}
	uint8 so = dgd(SPLASHER_SCROLL) - uint8(zp.scroll_x);
	if (so != 0)
	{
		dgd(SPLASHER_BUBBLE_X) = ox + so;
		dgd(SPLASHER_SCROLL) = uint8(zp.scroll_x);
	}

	uint8 nx = dgd(SPLASHER_BUBBLE_X);
	if (0 != ((ox ^ nx) & 0x80) && // wrapped, or crossed middle
	    0 == ((ox ^ (ox << 1)) & 0x80)) // in quadrant 0 or 3
	{
		// kill wrapped bubble
		dgd(SPLASHER_BUBBLE) = 0;
	}

	--dgd(SPLASHER_BUBBLE_Y);
	if ((dgd(SPLASHER_BUBBLE_Y)+6) < zp.water || // note: if water==0, collide will still pop at 255
		collide_tile(uint16(dgd(SPLASHER_BUBBLE_X)+4) + zp.scroll_x,dgd(SPLASHER_BUBBLE_Y)+6))
	{
		dgd(SPLASHER_BUBBLE) = 0;
	}
}
void dog_draw_splasher(uint8 d)
{
	// bubble
	if (dgd(SPLASHER_BUBBLE))
	{
		sprite_tile_add(dgd(SPLASHER_BUBBLE_X),dgd(SPLASHER_BUBBLE_Y),0x3E,0x01);
	}

	// splash
	dx_scroll();
	if (dgd(SPLASHER_ANIM)<11)
	{
		if (dgd(SPLASHER_ANIM)<8) sprite_tile_add(dx,dgy,0x38,0x01);
		else                      sprite_tile_add(dx,dgy,0x39,0x01);
	}

}

// DOG_DISCO

const int DISCO_ANIM_BALL  = 0;
const int DISCO_ANIM_FLOOR = 1;
const int DISCO_BALL_PAL   = 2;
const int DISCO_MELT_TIMER = 3;
const int DISCO_MELT_INDEX = 4;

void dog_melt_iceblock(uint8 d); // forward declaration

void dog_init_disco(uint8 d)
{
	// NOT IN DEMO
}
void dog_tick_disco(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_disco(uint8 d)
{
	// NOT IN DEMO
}

// DOG_WATER_PALETTE

const int WATER_PALETTE_FRAME = 0;
const int WATER_PALETTE_CYCLE = 1;
void dog_init_water_palette(uint8 d)
{
	play_bg_noise(0xA,0x2);
}
void dog_tick_water_palette(uint8 d)
{
	++dgd(WATER_PALETTE_FRAME);
	if (dgd(WATER_PALETTE_FRAME) >= 3)
	{
		dgd(WATER_PALETTE_FRAME) = 0;
		++dgd(WATER_PALETTE_CYCLE);
		if (dgd(WATER_PALETTE_CYCLE) >= 3)
			dgd(WATER_PALETTE_CYCLE) = 0;
		palette_load(1,DATA_palette_waterfall0 + dgd(WATER_PALETTE_CYCLE));
	}
}
void dog_draw_water_palette(uint8 d) {}

// DOG_GRATE

void dog_init_grate(uint8 d)
{
	dog_blocker(d,0,0,15,13);
}
void dog_tick_grate(uint8 d) {}
void dog_draw_grate(uint8 d)
{
	dx_scroll_edge();
	sprite0_add(dx,dgy,DATA_sprite0_grate);
}

// DOG_GRATE90

void dog_init_grate90(uint8 d)
{
	dog_blocker(d,0,0,13,15);
}
void dog_tick_grate90(uint8 d) {}
void dog_draw_grate90(uint8 d)
{
	dx_scroll_edge();
	sprite0_add(dx,dgy,DATA_sprite0_grate90);
}

// DOG_WATER_FLOW

void dog_init_water_flow(uint8 d) {}
void dog_tick_water_flow(uint8 d)
{
	if (bound_overlap(d,0,0,32,7))
	{
		lizard.flow = 1; // flow left
	}
}
void dog_draw_water_flow(uint8 d) {}

//  DOG_RAINBOW_PALETTE

const int RAINBOW_PALETTE_TIME = 0;
void dog_init_rainbow_palette(uint8 d) {}
void dog_tick_rainbow_palette(uint8 d)
{
	++dgd(RAINBOW_PALETTE_TIME);

	if (dgp <  4 && (dgd(RAINBOW_PALETTE_TIME) & 7) != 0) return;
	if (dgp >= 4 && (dgd(RAINBOW_PALETTE_TIME) & 3) != 0) return; // sprites cycle faster

	uint8 p = (dgp & 7) * 4;
	for (int i=0; i<4; ++i)
	{
		uint8 ov = h.palette[p+i];
		uint8 oh = ov &0xF;
		if (oh >= 0x1 && oh <= 0xC)
		{
			++oh;
			if (oh > 0xC) oh = 0x1;
			ov = (ov & 0xF0) | oh;
			h.palette[p+i] = ov;
		}
	}
}
void dog_draw_rainbow_palette(uint8 d) {}

// DOG_PUMP

void dog_init_pump(uint8 d)
{
	dgp = prng(8);
}
void dog_tick_pump(uint8 d)
{
	dgp += 2;
}
void dog_draw_pump(uint8 d)
{
	sint8 a = circle4[dgp];
	dx_scroll_offset(a);
	sprite_tile_add(dx,dgy-1,0xF9,0x03);
}

// DOG_SECRET_STEAM

const int SECRET_STEAM_SOUND = 2;
void dog_init_secret_steam(uint8 d)
{
	NES_ASSERT(zp.room_scrolling == 0, "DOG_SECRET_STEAM does not support scrolling!");
	play_bg_noise(0xD,2);
}
void dog_tick_secret_steam(uint8 d)
{
	dgd(0) = prng();
	dgd(1) = prng();
}
void dog_draw_secret_steam(uint8 d)
{
	dx_screen();
	sprite_tile_add(dx  ,dgy-1,0x23,(dgd(0) & 0x40) | 0x01);
	sprite_tile_add(dx+8,dgy-1,0x23,(dgd(1) & 0x40) | 0x01);
}

// DOG_CEILING_FREE

void dog_init_ceiling_free(uint8 d)
{
	for (int i=0; i<16; ++i)
	{
		h.collision[240+i] = 0;
	}
}
void dog_tick_ceiling_free(uint8 d)
{
}
void dog_draw_ceiling_free(uint8 d)
{
}

// DOG_BLOCK_COLUMN

void dog_init_block_column(uint8 d)
{
	dog_blocker(d,0,0,3,sint8(255));
}
void dog_tick_block_column(uint8 d) {}
void dog_draw_block_column(uint8 d) {}

// DOG_SAVE_STONE

//const int SAVE_STONE_ON = 0; // in enums.h
const int SAVE_STONE_ANIM = 1;
void dog_init_save_stone(uint8 d)
{
	//dgd(SAVE_STONE_ON  ) = 0;
	//dgd(SAVE_STONE_ANIM) = 0;

	uint16 rr = 0;
	uint8 rl = 0;
	password_read(&rr, &rl);
	if (rr == zp.current_room &&
		rl == zp.current_lizard &&
		h.last_lizard == h.last_lizard_save &&
		zp.continued &&
		zp.coin_saved)
	{
		dgd(SAVE_STONE_ON) = 1;
	}
}
void dog_tick_save_stone(uint8 d)
{
	if ((prng() & 3) == 0) dgd(SAVE_STONE_ANIM) ^= 1;

	if (dgd(SAVE_STONE_ON) != 0) return;
	if (bound_overlap(d,-4,4,3,15))
	{
		dgd(SAVE_STONE_ON) = 1;
		play_sound(SOUND_FIRE);
		password_build();
		zp.continued = 1;

		for (int i=0; i<16; ++i) h.coin_save[i] = h.coin[i];
		for (int i=0; i<16; ++i) h.flag_save[i] = h.flag[i];
		h.piggy_bank_save = h.piggy_bank;
		h.last_lizard_save = h.last_lizard;
		zp.coin_saved = 1;
		h.tip_counter = 0;

		resume_point();
	}
}
void dog_draw_save_stone(uint8 d)
{
	unsigned char p = 6 | (d & 1);

	if (dgd(SAVE_STONE_ON) == 0) palette_load(p,DATA_palette_save_stone0);
	else
	{
		if (dgd(SAVE_STONE_ANIM)==0) palette_load(p,DATA_palette_save_stone1);
		else                         palette_load(p,DATA_palette_save_stone2);
	}

	dx_scroll_edge();
	sprite0_add(dx,dgy,DATA_sprite0_save_stone);
}

// DOG_COIN

const int COIN_ON = 0;
const int COIN_ANIM = 1;
void dog_init_coin(uint8 d)
{
	dgd(COIN_ON) = 1;

	if (coin_read(dgp))
	{
		dgd(COIN_ON) = 0;
	}
	else
	{
		dgd(COIN_ANIM) = prng(8);
	}
}
void dog_tick_coin(uint8 d)
{
	if (dgd(COIN_ON) == 0) return;

	if (bound_overlap(d,3,0,4,7))
	{
		dgd(COIN_ON) = 0;
		coin_take(dgp);
		play_sound(SOUND_COIN);
	}
	++dgd(COIN_ANIM);
}
void dog_draw_coin(uint8 d)
{
	if (dgd(COIN_ON) == 0) return;

	dx_scroll();

	static const uint8 COIN_CYCLE_TILE[8] = { 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE5, 0xE4, 0xE3 };
	static const uint8 COIN_CYCLE_ATT[ 8] = { 0x03, 0x03, 0x03, 0x43, 0x03, 0x03, 0x43, 0x43 };

	uint8 a = (dgd(COIN_ANIM) >> 3) & 7;
	uint8 tile = COIN_CYCLE_TILE[a];
	uint8 att  = COIN_CYCLE_ATT[ a];

	sprite_tile_add(dx,dgy-1,tile,att);
}

// DOG_MONOLITH

void dog_init_monolith(uint8 d)
{
	NES_ASSERT(dgp < 14,"monolith parameter out of range.");
}
void dog_tick_monolith(uint8 d)
{
	if (lizard.dead) return;

	if (lizard.power > 0 && zp.current_lizard == LIZARD_OF_KNOWLEDGE)
	{
		if (
			(lizard.face == 0 && lizard_px <= uint16(dgx + 12)) || // facing right (or on)
			(lizard.face != 0 && lizard_px >= uint16(dgx - 12))    // facing left (or on)
			)
		{
			if (lizard_overlap(dgx-50,0,dgx+49,255))
			{
				CT_ASSERT(TEXT_MONOLITH1  == TEXT_MONOLITH0 + 1 , "Monolith text out of order.");
				CT_ASSERT(TEXT_MONOLITH2  == TEXT_MONOLITH0 + 2 , "Monolith text out of order.");
				CT_ASSERT(TEXT_MONOLITH3  == TEXT_MONOLITH0 + 3 , "Monolith text out of order.");
				CT_ASSERT(TEXT_MONOLITH4  == TEXT_MONOLITH0 + 4 , "Monolith text out of order.");
				CT_ASSERT(TEXT_MONOLITH5  == TEXT_MONOLITH0 + 5 , "Monolith text out of order.");
				CT_ASSERT(TEXT_MONOLITH6  == TEXT_MONOLITH0 + 6 , "Monolith text out of order.");
				CT_ASSERT(TEXT_MONOLITH7  == TEXT_MONOLITH0 + 7 , "Monolith text out of order.");
				CT_ASSERT(TEXT_MONOLITH8  == TEXT_MONOLITH0 + 8 , "Monolith text out of order.");
				CT_ASSERT(TEXT_MONOLITH9  == TEXT_MONOLITH0 + 9 , "Monolith text out of order.");
				CT_ASSERT(TEXT_MONOLITH10 == TEXT_MONOLITH0 + 10, "Monolith text out of order.");
				CT_ASSERT(TEXT_MONOLITH11 == TEXT_MONOLITH0 + 11, "Monolith text out of order.");
				CT_ASSERT(TEXT_MONOLITH12 == TEXT_MONOLITH0 + 12, "Monolith text out of order.");
				CT_ASSERT(TEXT_MONOLITH13 == TEXT_MONOLITH0 + 13, "Monolith text out of order.");

				h.text_select = 1; // message mode, not talk
				zp.game_message = dgp + TEXT_MONOLITH0;
				zp.game_pause = 1;
			}
		}
	}
}
void dog_draw_monolith(uint8 d) {}

// DOG_ICEBLOCK

const int ICEBLOCK_ANIM = 0;

void dog_melt_iceblock(uint8 d)
{
	if (dgt != DOG_ICEBLOCK) return;
	if (dgd(ICEBLOCK_ANIM) != 0) return;

	dgd(ICEBLOCK_ANIM) = 1;

	uint8 tx = (dgx >> 3) - 1;;
	uint8 ty = (dgy >> 3);
	collide_clear_tile(tx-0,ty+0);
	collide_clear_tile(tx+1,ty+0);
	collide_clear_tile(tx-0,ty+1);
	collide_clear_tile(tx+1,ty+1);

	flag_set(dgp);
}

void dog_init_iceblock(uint8 d)
{
	CT_ASSERT(DATA_sprite0_melt0 == (DATA_sprite0_iceblock+1),"iceblock sprites out of order");
	CT_ASSERT(DATA_sprite0_melt1 == (DATA_sprite0_melt0+1),   "iceblock sprites out of order");
	CT_ASSERT(DATA_sprite0_melt2 == (DATA_sprite0_melt1+1),   "iceblock sprites out of order");
	CT_ASSERT(DATA_sprite0_melt3 == (DATA_sprite0_melt2+1),   "iceblock sprites out of order");
	CT_ASSERT(DATA_sprite0_melt4 == (DATA_sprite0_melt3+1),   "iceblock sprites out of order");
	CT_ASSERT(DATA_sprite0_melt5 == (DATA_sprite0_melt4+1),   "iceblock sprites out of order");

	if (flag_read(dgp))
	{
		empty_dog(d);
		return;
	}

	uint8 tx = (dgx >> 3) - 1;
	uint8 ty = (dgy >> 3);
	collide_set_tile(tx+0,ty+0);
	collide_set_tile(tx+1,ty+0);
	collide_set_tile(tx+0,ty+1);
	collide_set_tile(tx+1,ty+1);
}
void dog_tick_iceblock(uint8 d)
{
	if (dgd(ICEBLOCK_ANIM) > 0)
	{
		++dgd(ICEBLOCK_ANIM);
		if (dgd(ICEBLOCK_ANIM) >= (7<<2))
		{
			empty_dog(d);
			return;
		}
	}
	else
	{
		if (zp.current_lizard != LIZARD_OF_HEAT) return;

		if (bound_touch(d,-8,0,7,15))
		{
			dog_melt_iceblock(d);
		}
	}
}
void dog_draw_iceblock(uint8 d)
{
	dx_scroll_edge();
	sprite0_add(dx,dgy,(dgd(ICEBLOCK_ANIM)>>2) + DATA_sprite0_iceblock);
}

// DOG_VATOR

const int VATOR_ANGLE = 0;
void dog_tick_vator_blocker(uint8 d)
{
	dog_blocker(d,-12,0,11,7);
}
void dog_init_vator(uint8 d)
{
	if (dgp == 0)
	{
		dgd(VATOR_ANGLE) = prng(8);
		dgx += circle32[dgd(VATOR_ANGLE)];
	}
	else if (dgp == 1)
	{
		dgd(VATOR_ANGLE) = prng(8);
		dgy += circle32[dgd(VATOR_ANGLE)];
	}
	// the top and bottom half-circle vators are hard-wired to specific startup spots
	else if (dgp == 4)
	{
		if (dgy < 120)
		{
			dgd(VATOR_ANGLE) = 128 + 16;
			dgx = 396 + circle32[(128+16+64) & 255];
			dgy = 120 + circle32[(128+16+0 ) & 255];
		}
		else
		{
			dgd(VATOR_ANGLE) = 16;
		}
	}
	else if (dgp == 5)
	{
		if (dgy >= 120)
		{
			dgd(VATOR_ANGLE) = 128 + 32;
			dgx = 396 + circle32[(128+32+192) & 255];
			dgy = 120 + circle32[(128+32+128) & 255];
		}
		else
		{
			dgd(VATOR_ANGLE) = 32;
		}
	}

	dog_tick_vator_blocker(d);
}
void dog_tick_vator(uint8 d)
{
	sint8 dx, dy;
	dx = 0;
	dy = 0;
	switch (dgp)
	{
	case 0: // sine X
		{
			sint8 a = circle32[ dgd(VATOR_ANGLE)         ];
			sint8 b = circle32[(dgd(VATOR_ANGLE)+1) & 255];
			dx = (b - a);
		}
		break;
	case 1: // sine Y
		{
			sint8 a = circle32[ dgd(VATOR_ANGLE)         ];
			sint8 b = circle32[(dgd(VATOR_ANGLE)+1) & 255];
			dy = (b - a);
		}
		break;
	case 2: // up
		dy = -1;
		break;
	case 3: // down
		dy = 1;
		break;
	case 4:
		if (dgd(VATOR_ANGLE) < 128)
		{
			if (dgx >= 396)
			{
				dy = 1;
				if (dgd(VATOR_ANGLE) >= 127)
				{
					dx = 364 - dgx;
					dy = 248 - dgy;
					dgd(VATOR_ANGLE) = 255;
				}
			}
			else
			{
				dy = -1;
			}
		}
		else
		{

			sint8 a0 = circle32[(dgd(VATOR_ANGLE)+64 ) & 255];
			sint8 b0 = circle32[(dgd(VATOR_ANGLE)+65 ) & 255];
			sint8 a1 = circle32[(dgd(VATOR_ANGLE)+0  ) & 255];
			sint8 b1 = circle32[(dgd(VATOR_ANGLE)+1  ) & 255];
			dx = b0-a0;
			dy = b1-a1;
		}
		break;
	case 5:
		if (dgd(VATOR_ANGLE) < 128)
		{
			if (dgx < 396)
			{
				dy = -1;
				if (dgd(VATOR_ANGLE) >= 127)
				{
					dx = 172 - dgx;
					dy =  -8 - dgy;
					dgd(VATOR_ANGLE) = 255;
				}
			}
			else
			{
				dy = 1;
			}
		}
		else
		{

			sint8 a0 = circle32[(dgd(VATOR_ANGLE)+192) & 255];
			sint8 b0 = circle32[(dgd(VATOR_ANGLE)+193) & 255];
			sint8 a1 = circle32[(dgd(VATOR_ANGLE)+128) & 255];
			sint8 b1 = circle32[(dgd(VATOR_ANGLE)+129) & 255];
			dx = b0-a0;
			dy = b1-a1;
		}
		break;
	default:
		NES_ASSERT(false,"Invalid vator parameter.");
		break;
	}

	if (bound_overlap(d,-12,-1,11,-1))
	{
		// lizard is riding it
		lizard.x += 256 * dx;
		lizard.y += 256 * dy;
		do_scroll();
		dgx += dx;
		dgy += dy;
		++dgd(VATOR_ANGLE);
		dog_tick_vator_blocker(d); // update blocker
	}
	else if (!bound_overlap(d,dx-12,dy,dx+11,dy+7))
	{
		// lizard is not blocking it
		dgx += dx;
		dgy += dy;
		++dgd(VATOR_ANGLE);
		dog_tick_vator_blocker(d); // update blocker
	}
}
void dog_draw_vator(uint8 d)
{
	dx_scroll_edge();
	sprite0_add(dx,dgy,DATA_sprite0_vator);
}

// DOG_NOISE

void dog_init_noise(uint8 d)
{
	// dgx / dgy are noise parameters freq / volume
	NES_ASSERT((dgx & 15) != 0xE, "dog_noise 13 reserved for rain");
	play_bg_noise(dgx & 15, dgy & 15);
}
void dog_tick_noise(uint8 d) {}
void dog_draw_noise(uint8 d) {}

// DOG_SNOW

const int DOG_SNOW_X = 0;
const int DOG_SNOW_Y = 6;

void dog_tick_snow(uint8 d); // forward

void dog_init_snow(uint8 d)
{
	dgx = zp.scroll_x;

	for (int i=0;i<6;++i)
	{
		dgd(DOG_SNOW_X+i) = prng(2);
		dgd(DOG_SNOW_Y+i) = prng(2);
	}

	dgp = 0x01; // default attribute
	if (dog_type(DISMOUNT_SLOT) == DOG_LIZARD_DISMOUNTER)
	{
		dgp = 0x03; // override for snowy dismount
	}

	dog_tick_snow(d); // clear any first-frame colliders
}
void dog_tick_snow(uint8 d)
{
	if ((dgx & 0xFF) != (zp.scroll_x & 0xFF))
	{
		uint8 s = dgx - zp.scroll_x;
		for (int i=0; i<6; ++i)
		{
			dgd(DOG_SNOW_X+i) += s;
		}
		dgx = zp.scroll_x;
	}

	for (int i=0;i<6;++i)
	{
		// move
		if (prng()&1) dgd(DOG_SNOW_X+i) += 2;
		dgd(DOG_SNOW_Y+i) += 2;

		// respawn
		if (//dgd(DOG_SNOW_Y+i) >= 240 || // collide makes this unnecessary
			//dgd(DOG_SNOW_X+i) >= 254 ||
			collide_tile(zp.scroll_x + dgd(DOG_SNOW_X+i),dgd(DOG_SNOW_Y+i)))
		{
			if (prng()&1)
			{
				dgd(DOG_SNOW_Y+i) = prng();
				dgd(DOG_SNOW_X+i) = 0;

				// avoid visual glitching if spawned on the side (just spawn on top, don't care about glitches there)
				if (collide_tile(zp.scroll_x + dgd(DOG_SNOW_X+i),dgd(DOG_SNOW_Y+i)))
				{
					dgd(DOG_SNOW_X+i) = prng();
					dgd(DOG_SNOW_Y+i) = 0;
				}
			}
			else
			{
				dgd(DOG_SNOW_X+i) = prng();
				dgd(DOG_SNOW_Y+i) = 0;
			}
		}
	}
}
void dog_draw_snow_particles(uint8 d, uint8 tile, uint8 attrib)
{
	for (int i=0;i<6;++i)
	{
		// note: sprite is 1 Y line too low, but this is acceptable
		// (saves 2 cycles, but also doesn't matter since colliding surfaces are generally white on the mountain)
		sprite_tile_add(dgd(DOG_SNOW_X+i),dgd(DOG_SNOW_Y+i),tile,attrib);
	}
}
void dog_draw_snow(uint8 d)
{
	dog_draw_snow_particles(d,0xC6,dgp);
}

// DOG_RAIN

void dog_init_rain(uint8 d)
{
	play_bg_noise(0xE,3);
	dog_init_snow(d);
	dgp = 0x03; // default attribute
}
void dog_tick_rain(uint8 d)
{
	dog_tick_snow(d);
	dog_tick_snow(d);
}
void dog_draw_rain(uint8 d)
{
	dog_draw_snow_particles(d,0xFC,dgp);
}

// DOG_RAIN_BOSS


void dog_init_rain_boss(uint8 d)
{
	dog_init_rain(d);
	dgp = 0x02; // change palette attribute
}
void dog_tick_rain_boss(uint8 d) { dog_tick_rain(d); }
void dog_draw_rain_boss(uint8 d) { dog_draw_rain(d); }

// DOG_DRIP

const int DRIP_ANIM = 0;
const int DRIP_X0   = 1;
const int DRIP_X1   = 2;
const int DRIP_Y    = 3;

void dog_init_drip(uint8 d)
{
	dgd_put16(dgx,DRIP_X0,DRIP_X1);
	dgd(DRIP_Y) = dgy;
	dgd(DRIP_ANIM) = prng(2);
}
void dog_tick_drip(uint8 d)
{
	if (dgd(DRIP_ANIM) > 0)
	{
		--dgd(DRIP_ANIM);
		if (dgd(DRIP_ANIM) == 0)
		{
			dgy = dgd(DRIP_Y);
			uint16 tx = dgd_get16(DRIP_X0,DRIP_X1) + (prng() & 7);
			dgx = tx;
		}
		return;
	}

	dgy += 4;
	if (dgy >= 240 || collide_tile(dgx,dgy+2))
	{
		dgd(DRIP_ANIM) = 192 + (prng(2) & 63);
		return;
	}
	if ((dgy+2) >= zp.water)
	{
		play_sound_scroll(SOUND_DRIP);
		if (dog_type(SPLASHER_SLOT) == DOG_SPLASHER)
		{
			dog_x(SPLASHER_SLOT) = dgx - 4;
			dog_y(SPLASHER_SLOT) = zp.water - 9;
			dog_data(0,SPLASHER_SLOT) = 0; // trigger splash animation
		}

		dgd(DRIP_ANIM) = 192 + (prng(2) & 63);
		return;
	}
}
void dog_draw_drip(uint8 d)
{
	if (dgd(DRIP_ANIM) != 0) return;
	dx_scroll();
	sprite_tile_add(dx,dgy,0xFC,0x03);
}

// DOG_HOLD_SCREEN

void dog_init_hold_screen(uint8 d)
{
	NES_ASSERT(d == HOLD_SLOT,"hold_screen in wrong slot!");
}
void dog_tick_hold_screen(uint8 d)
{
}
void dog_draw_hold_screen(uint8 d)
{
}

// DOG_BOSS_DOOR

//const int RAINBOW_PALETTE_TIME = 0;
const int BOSS_DOOR_ANIM =   1;
const int BOSS_DOOR_WOBBLE = 2;

void dog_tick_boss_door_wobble(uint8 d)
{
	uint8 a = circle4[dgd(BOSS_DOOR_WOBBLE)];
	++dgd(BOSS_DOOR_WOBBLE);
	uint8 b = circle4[dgd(BOSS_DOOR_WOBBLE)];

	dgy += (b-a);
}

void dog_init_boss_door_common(uint8 d)
{
	if (dgp < 6)
	{
		if (flag_read(dgp + FLAG_BOSS_0_MOUNTAIN))
		{
			h_door_link[1] = DATA_room_start_again;
		}
	}

	dgp = 6; // RAINBOW_PALETTE parameter
	dgd(BOSS_DOOR_WOBBLE) = 128; // up first
	dgd(BOSS_DOOR_ANIM) = 0;
	play_sound(SOUND_BOSS_DOOR_OPEN);
}

void dog_init_boss_door_palette()
{
	palette_load(6,DATA_palette_boss_door);
	uint8 s = prng() & 0x10;
	uint8 r = prng(4) & 31;
	for (uint8 x = 25; x <27; ++x)
	{
		uint8 c = h.palette[x] & 0x0F;
		c += r;
		while (c >= 0xD) c -= 0xC;
		h.palette[x] = ((h.palette[x] & 0xF0) | c) + s;
	}
}

void dog_init_boss_door(uint8 d)
{
	dog_init_boss_door_palette();
	dog_init_boss_door_common(d);
}

void dog_tick_boss_door(uint8 d)
{
	if (dgd(BOSS_DOOR_ANIM) >= 22) // fully open
	{
		dgd(BOSS_DOOR_ANIM) = 22;
		if((zp.gamepad & PAD_U) && bound_overlap(d,-8,0,7,15))
		{
			if (dgt != DOG_BOSS_RUSH)
			{
				zp.current_door = 1;
			}
			else
			{
				NES_ASSERT(d < 4, "boss_rush in unexpected slot?");
				zp.current_door = d;
				if (h.boss_rush == 0)
				{
					h.boss_rush = 1;
					// reset metrics on first boss rush entry
					h.metric_time_h = 0;
					h.metric_time_m = 0;
					h.metric_time_s = 0;
					h.metric_time_f = 0;
					h.metric_bones = 0;
					h.metric_jumps = 0;
					h.metric_continue = 0;

					// restore any bosses that were beaten before starting the boss rush
					for (uint8 i=FLAG_BOSS_0_MOUNTAIN; i<=FLAG_BOSS_5_VOID; ++i)
					{
						flag_clear(i);
					}
				}
			}

			zp.current_room = h_door_link[zp.current_door];
			zp.room_change = 1;
		}
	}

	dog_tick_boss_door_wobble(d);

	if (dgt != DOG_BOSS_RUSH)
	{
		if ((dgd(RAINBOW_PALETTE_TIME) & 1) == 0)
			++dgd(BOSS_DOOR_ANIM);
		dog_tick_rainbow_palette(d);
	}
}

void dog_draw_boss_door(uint8 d)
{
	uint8 dx;
	if (dog_type(0) == DOG_FROB)
	{
		dx_screen_(dx,dgx);
	}
	else
	{
		if (dx_scroll_edge_(dx,dgx)) return;
	}

	uint8 anim = dgd(BOSS_DOOR_ANIM); // for quick use

	// 4 corners
	uint8 corner_offset = 0;
	uint8 top_offset = 0;
	if (anim < 5)
	{
		corner_offset = 5 - anim;
		top_offset = 16;
	}
	else if (anim < 21)
	{
		top_offset = 21-anim;
	}
	sprite_tile_add_clip((dx-16)+corner_offset,dgy+15,0xC8,0x02);
	sprite_tile_add_clip((dx+ 8)-corner_offset,dgy+15,0xC8,0x42);
	sprite_tile_add_clip((dx-16)+corner_offset,(dgy-9)+top_offset,0xC8,0x82);
	sprite_tile_add_clip((dx+ 8)-corner_offset,(dgy-9)+top_offset,0xC8,0xC2);

	// top/bottom
	uint8 bottom_offset = 0;
	if (anim < 3) bottom_offset = 2;
	sprite_tile_add_clip((dx-8)+bottom_offset,dgy+15,0xC9,0x02);
	sprite_tile_add_clip((dx+0)-bottom_offset,dgy+15,0xC9,0x02);
	sprite_tile_add_clip((dx-8)+bottom_offset,(dgy-9)+top_offset,0xC9,0x82);
	sprite_tile_add_clip((dx+0)-bottom_offset,(dgy-9)+top_offset,0xC9,0x82);

	// lower-middle
	uint8 mt = 0xD8;
	if (anim < 6) return;
	uint8 y = dgy+7;
	if (anim < 10)
	{
		y += 4;
		mt = 0xF7;
	}
	sprite_tile_add_clip(dx- 8,y,0xD9,0x02);
	sprite_tile_add_clip(dx+ 0,y,0xD9,0x02);
	sprite_tile_add_clip(dx-16,y,  mt,0x02);
	sprite_tile_add_clip(dx+ 8,y,  mt,0x42);

	// upper-middle
	if (anim < 14) return;
	y = dgy-1;
	if (anim < 18) y += 4;
	sprite_tile_add_clip(dx- 8,y,0xD9,0x02);
	sprite_tile_add_clip(dx+ 0,y,0xD9,0x02);
	sprite_tile_add_clip(dx-16,y,0xD8,0x02);
	sprite_tile_add_clip(dx+ 8,y,0xD8,0x42);
}

// DOG_BOSS_DOOR_RAIN

void dog_init_boss_door_rain(uint8 d)
{
	NES_ASSERT(d == BOSS_DOOR_RAIN_SLOT,"boss_door_rain not in BOSS_DOOR_RAIN_SLOT.");
	dog_init_boss_door(d);
}
void dog_tick_boss_door_rain(uint8 d)
{
	dog_tick_boss_door(d);

	// undo rainbow palette on last entry
	h.palette[27] = 0x12;
}
void dog_draw_boss_door_rain(uint8 d)
{
	dog_draw_boss_door(d);
}

// DOG_BOSS_DOOR_EXIT

void dog_init_boss_door_exit_common(uint8 d)
{
	play_sound(SOUND_BOSS_DOOR_CLOSE);
	dgd(BOSS_DOOR_ANIM) = 22;
}

void dog_init_boss_door_exit(uint8 d)
{
	dog_init_boss_door(d);
	dog_init_boss_door_exit_common(d);
}

void dog_tick_boss_door_exit_common(uint8 d)
{
	if (dgd(BOSS_DOOR_ANIM) < 1)
	{
		empty_dog(d);
		return;
	}

	dog_tick_boss_door_wobble(d);

	if ((dgd(RAINBOW_PALETTE_TIME) & 1) == 0)
		--dgd(BOSS_DOOR_ANIM);
}

void dog_tick_boss_door_exit(uint8 d)
{
	dog_tick_boss_door_exit_common(d);
	dog_tick_rainbow_palette(d);
}

void dog_draw_boss_door_exit(uint8 d)
{
	dog_draw_boss_door(d);
}

// DOG_BOSS_DOOR_EXEUNT

// This is like DOG_BOSS_DOOR_EXIT except it does not set or cycle the palette,
// used on ending screensh where we don't have enough palettes to accomodate the door.
// The "exeunt" terminology suggests that we are leaving together with the other lizard, maybe.

void dog_init_boss_door_exeunt(uint8 d)
{
	// the "common" versions skip the palette setting
	dog_init_boss_door_common(d);
	dog_init_boss_door_exit_common(d);
}
void dog_tick_boss_door_exeunt(uint8 d)
{
	dog_tick_boss_door_exit_common(d);
	// rainbow_palette is not executed, so manually increment it
	++dgd(RAINBOW_PALETTE_TIME);
}
void dog_draw_boss_door_exeunt(uint8 d)
{
	dog_draw_boss_door(d);
}

// DOG_BOSS_RUSH

void dog_init_boss_rush(uint8 d)
{
	NES_ASSERT(d >= 1 && d <= 3, "boss_rush in unexpected slot?");
	NES_ASSERT(dgp < 6, "boss_rush invalid parameter");

	if (flag_read(dgp + FLAG_BOSS_0_MOUNTAIN))
	{
		empty_dog(d);
		return;
	}

	dgd(BOSS_DOOR_ANIM) = 22; // skip opening phase
	dgd(BOSS_DOOR_WOBBLE) = prng(8);
	dgy += circle4[dgd(BOSS_DOOR_WOBBLE)];
	dog_init_boss_door_palette();
}

void dog_tick_boss_rush(uint8 d)
{
	dog_tick_boss_door(d);
}

void dog_draw_boss_rush(uint8 d)
{
	dog_draw_boss_door(d);
}

// DOG_OTHER

const int OTHER_ANIM = 0;
const int OTHER_FACE = 1;
const int OTHER_SPRITE = 2;
const int OTHER_BLINK = 3;
const int OTHER_PALETTE = 4;
// slightly wider than lizard hitbox
const int OTHER_HITBOX_L = -(5+4);
const int OTHER_HITBOX_R = (4+4);
const int OTHER_HITBOX_T = -14;
const int OTHER_HITBOX_B = -1;
const int OTHER_BLINK_POINT = 223;

void dog_init_other(uint8 d)
{
	NES_ASSERT(zp.room_scrolling == 0, "DOG_OTHER does not support scrolling!");

	dgd(OTHER_ANIM) = 0;
	dgd(OTHER_FACE) = 0;
	dgd(OTHER_SPRITE) = DATA_sprite0_other_stand;
	dgd(OTHER_BLINK) = prng();
	dgd(OTHER_PALETTE) = 0;

	if (uint8(lizard_px) >= uint8(dgx))
	{
		dgd(OTHER_FACE) = 1;
	}

	h_blocker_x0[0] = dgx + OTHER_HITBOX_L;
	h_blocker_x1[0] = dgx + OTHER_HITBOX_R;
	h.blocker_y0[0] = dgy + OTHER_HITBOX_T;
	h.blocker_y1[0] = dgy + OTHER_HITBOX_B;

	CT_ASSERT(DATA_palette_human1_black == DATA_palette_human0_black+1,"palette_human_black out of order");
	CT_ASSERT(DATA_palette_human2_black == DATA_palette_human0_black+2,"palette_human_black out of order");

	palette_load(6,DATA_palette_human0_black + h.human1_pal);
	palette_load(7,DATA_palette_lizard10);

	if (dog_type(3) == DOG_RAIN_BOSS) // special case for end_palace
	{
		h.palette[(6*4)+3] = 0x12;
	}
}

void dog_tick_other_talk_in()
{
	CT_ASSERT(TEXT_END_KNOWLEDGE + LIZARD_OF_KNOWLEDGE == TEXT_END_KNOWLEDGE, "Ending text out of order.");
	CT_ASSERT(TEXT_END_KNOWLEDGE + LIZARD_OF_BOUNCE    == TEXT_END_BOUNCE   , "Ending text out of order.");
	CT_ASSERT(TEXT_END_KNOWLEDGE + LIZARD_OF_SWIM      == TEXT_END_SWIM     , "Ending text out of order.");
	CT_ASSERT(TEXT_END_KNOWLEDGE + LIZARD_OF_HEAT      == TEXT_END_HEAT     , "Ending text out of order.");
	CT_ASSERT(TEXT_END_KNOWLEDGE + LIZARD_OF_SURF      == TEXT_END_SURF     , "Ending text out of order.");
	CT_ASSERT(TEXT_END_KNOWLEDGE + LIZARD_OF_PUSH      == TEXT_END_PUSH     , "Ending text out of order.");
	CT_ASSERT(TEXT_END_KNOWLEDGE + LIZARD_OF_STONE     == TEXT_END_STONE    , "Ending text out of order.");
	CT_ASSERT(TEXT_END_KNOWLEDGE + LIZARD_OF_COFFEE    == TEXT_END_COFFEE   , "Ending text out of order.");
	CT_ASSERT(TEXT_END_KNOWLEDGE + LIZARD_OF_LOUNGE    == TEXT_END_LOUNGE   , "Ending text out of order.");
	CT_ASSERT(TEXT_END_KNOWLEDGE + LIZARD_OF_DEATH     == TEXT_END_DEATH    , "Ending text out of order.");
	CT_ASSERT(TEXT_END_KNOWLEDGE + LIZARD_OF_BEYOND    == TEXT_END_BEYOND   , "Ending text out of order.");

	zp.game_message = (TEXT_END_KNOWLEDGE + zp.current_lizard);
	zp.game_pause = 1;
}

void dog_tick_other_kill_doors()
{
	for (int i=0; i<15; ++i)
	{
		switch (dog_type(i))
		{
		case DOG_DOOR:
		case DOG_PASS:
		case DOG_PASS_X:
		case DOG_PASS_Y:
			dog_type(i) = DOG_NONE;
			break;
		default:
			break;
		}
	}
}

void dog_tick_other_ending()
{
	zp.current_door = 0;
	zp.current_room = DATA_room_ending0 + zp.current_lizard;
	zp.room_change = 1;
}

void dog_tick_other_idle(uint8 d)
{
	if (lizard_py < 240)
	{
		if      ((uint8(lizard_px) >= uint8(dgx)) &&
			     (uint8(lizard_px - dgx) >= 8))
		{
			dgd(OTHER_FACE) = 1;
		}
		else if ((uint8(dgx) >= uint8(lizard_px)) &&
			     (uint8(dgx - lizard_px) >= 8))
		{
			dgd(OTHER_FACE) = 0;
		}
	}

	if (dgd(OTHER_BLINK) >= (OTHER_BLINK_POINT+8)) dgd(OTHER_BLINK) = 0;
	else ++dgd(OTHER_BLINK);
}

void dog_tick_other_simple(uint8 d)
{
	if (dgd(OTHER_ANIM) > 0)
	{
		if (dgd(OTHER_ANIM) >= 64)
		{
			dog_tick_other_ending();
		}
		++dgd(OTHER_ANIM);
		return;
	}

	dog_tick_other_idle(d);
	if (lizard.power > 0 && bound_overlap(d,-20,-5,20,-1))
	{
		dgd(OTHER_ANIM) = 1;
		dog_tick_other_kill_doors();
		lizard.vx = 0;

		dog_tick_other_talk_in();
	}
}

void dog_tick_other_push(uint8 d)
{
	if (dgd(OTHER_ANIM) > 0)
	{
		if (dgd(OTHER_ANIM) < 64)
		{
			dgd(OTHER_SPRITE) = DATA_sprite0_other_skid;
			if ((dgd(OTHER_ANIM) & 1) == 1)
			{
				if (dgd(OTHER_FACE) == 0)
				{
					++dgx;
					++h_blocker_x0[0];
					++h_blocker_x1[0];
				}
				else
				{
					--dgx;
					--h_blocker_x0[0];
					--h_blocker_x1[0];
				}
			}
		}
		else if (dgd(OTHER_ANIM) == 64)
		{
			dgd(OTHER_SPRITE) = DATA_sprite0_other_stand;
			dog_tick_other_talk_in();
		}
		else if (dgd(OTHER_ANIM) >= 128)
		{
			dog_tick_other_ending();
		}

		++dgd(OTHER_ANIM);
		return;
	}

	dog_tick_other_idle(d);
	if (lizard.power > 0)
	{
		if ((zp.gamepad & PAD_R) && bound_overlap(d,OTHER_HITBOX_L-1,OTHER_HITBOX_T,OTHER_HITBOX_L-1,OTHER_HITBOX_B))
		{
			play_sound(SOUND_PUSH);
			dgd(OTHER_ANIM) = 1;
			dog_tick_other_kill_doors();
		}
		if ((zp.gamepad & PAD_L) && bound_overlap(d,OTHER_HITBOX_R+1,OTHER_HITBOX_T,OTHER_HITBOX_R+1,OTHER_HITBOX_B))
		{
			play_sound(SOUND_PUSH);
			dgd(OTHER_ANIM) = 1;
			dog_tick_other_kill_doors();
		}
	}
}

void dog_tick_other_heat(uint8 d)
{
	if (dgd(OTHER_ANIM) == 0)
	{
		dog_tick_other_idle(d);
		if ((lizard.power > 0) && bound_touch(d,OTHER_HITBOX_L,OTHER_HITBOX_T,OTHER_HITBOX_R,OTHER_HITBOX_B))
		{
			dgd(OTHER_PALETTE) = 0;

			dgd(OTHER_ANIM) = 1;
			dgd(OTHER_BLINK) = 0; // using blink for palette cycle
			dog_tick_other_kill_doors();
		}
		return;
	}

	if (dgd(OTHER_ANIM) < 80)
	{
		play_sound(SOUND_FIRE_CONTINUE);

		if (dgd(OTHER_BLINK) >= 3)
		{
			dgd(OTHER_BLINK) = 0;
			++dgd(OTHER_PALETTE);
		}
		else ++dgd(OTHER_BLINK);

		if (dgd(OTHER_PALETTE) >= 3) dgd(OTHER_PALETTE) = 0;

		palette_load(7,DATA_palette_lava0 + dgd(OTHER_PALETTE));
		dgd(OTHER_SPRITE) = DATA_sprite0_other_blink;
	}
	else if (dgd(OTHER_ANIM) <= 160)
	{
		palette_load(7,DATA_palette_lizard9); // DEATH palette = charred
		if ((dgd(OTHER_ANIM) & 31) >= 23) dgd(OTHER_SPRITE) = DATA_sprite0_other_stand;
		else                              dgd(OTHER_SPRITE) = DATA_sprite0_other_blink;

		if (dgd(OTHER_ANIM) == 160)
		{
			dgd(OTHER_SPRITE) = DATA_sprite0_other_stand;
			dog_tick_other_talk_in();
		}
	}
	else if (dgd(OTHER_ANIM) < 254)
	{
		dgd(OTHER_SPRITE) = DATA_sprite0_other_stand;
	}
	else
	{
		dog_tick_other_ending();
	}
	++dgd(OTHER_ANIM);
}

void dog_tick_other_death(uint8 d)
{
	if (dgd(OTHER_ANIM) == 0)
	{
		dog_tick_other_idle(d);
		if ((lizard.power > 0) && bound_touch(d,OTHER_HITBOX_L,OTHER_HITBOX_T,OTHER_HITBOX_R,OTHER_HITBOX_B))
		{
			NES_ASSERT(dog_type(OTHER_BONES_SLOT) == DOG_NONE, "DOG_NONE expected in slot 15!");
			dog_type(OTHER_BONES_SLOT) = DOG_BONES;
			dog_x(OTHER_BONES_SLOT) = dgx;
			dog_y(OTHER_BONES_SLOT) = dgy;
			dog_param(OTHER_BONES_SLOT) = DATA_sprite0_other_bones;
			dog_data(BONES_INIT,OTHER_BONES_SLOT) = 0;
			dog_data(BONES_FLIP,OTHER_BONES_SLOT) = dgd(OTHER_FACE);

			empty_blocker(0);

			dgd(OTHER_ANIM) = 1;
			dgd(OTHER_SPRITE) = DATA_sprite0_none0;
			play_sound(SOUND_BONES);
			++h.metric_bones;
			dog_tick_other_kill_doors();
		}
		return;
	}

	if (dgd(OTHER_ANIM) >= 250)
	{
		dog_tick_other_ending();
	}
	++dgd(OTHER_ANIM);
}

void dog_tick_other_substance(uint8 d)
{
	if (dgd(OTHER_ANIM) > 0)
	{
		dog_tick_other_simple(d);
		return;
	}

	dog_tick_other_idle(d);
	if (lizard.power > 30 && bound_overlap(d,-20,-5,20,-1))
	{
		dgd(OTHER_ANIM) = 1;
		dog_tick_other_kill_doors();
		lizard.vx = 0;
		dog_tick_other_talk_in();
	}
}

void dog_tick_other(uint8 d)
{
	switch (zp.current_lizard)
	{
	case LIZARD_OF_KNOWLEDGE: dog_tick_other_simple(d);    break;
	case LIZARD_OF_BOUNCE:    dog_tick_other_simple(d);    break;
	case LIZARD_OF_SWIM:      dog_tick_other_simple(d);    break;
	case LIZARD_OF_HEAT:      dog_tick_other_heat(d);      break;
	case LIZARD_OF_SURF:      dog_tick_other_simple(d);    break;
	case LIZARD_OF_PUSH:      dog_tick_other_push(d);      break;
	case LIZARD_OF_STONE:     dog_tick_other_simple(d);    break;
	case LIZARD_OF_COFFEE:    dog_tick_other_substance(d); break;
	case LIZARD_OF_LOUNGE:    dog_tick_other_substance(d); break;
	case LIZARD_OF_DEATH:     dog_tick_other_death(d);     break;
	case LIZARD_OF_BEYOND:    dog_tick_other_simple(d);    break;
	default:                  dog_tick_other_simple(d);    break;
	}
}

void dog_draw_other(uint8 d)
{
	uint8 sprite = dgd(OTHER_SPRITE);
	if (sprite == DATA_sprite0_none0) return;
	if (sprite == DATA_sprite0_other_stand && dgd(OTHER_BLINK) >= OTHER_BLINK_POINT)
	{
		sprite = DATA_sprite0_other_blink;
	}

	dx_screen();
	if (dgd(OTHER_FACE) == 0) sprite0_add(        dx,dgy,sprite);
	else                      sprite0_add_flipped(dx,dgy,sprite);
}

// DOG_ENDING

void dog_init_ending(uint8 d)
{
	NES_ASSERT(h.human1_next < 6,"human_next out of range!");

	if (h.human1_next < 6)
	{
		uint8 p = h.human1_pal;
		if (zp.current_lizard == LIZARD_OF_DEATH) p = 3;
		h.human1_set[h.human1_next] = p;
		h.human1_het[h.human1_next] = h.human1_hair;
	}

	zp.ending = 1;
}
void dog_tick_ending(uint8 d) {}
void dog_draw_ending(uint8 d) {}

// DOG_RIVER_EXIT

void dog_init_river_exit(uint8 d)
{
	if (zp.current_lizard != LIZARD_OF_SURF)
	{
		zp.current_lizard = LIZARD_OF_SURF;
		zp.next_lizard = LIZARD_OF_SURF;
		h.last_lizard = 0xFF;
	}
	lizard.vx = 999; // > MAX_RIGHT
	lizard.dismount = 2;
	lizard.face = 0; // face right
	lizard.wet = 0; // out of water
}
void dog_tick_river_exit(uint8 d)
{
	if (lizard.dead != 0) return;

	if (lizard_px < 92)
	{
		lizard.dismount = 2; // force surf to right
	}
	if (lizard_py > zp.water)
	{
		dog_init_river_exit(d);
		lizard.y = (zp.water-1) << 8;
	}
}
void dog_draw_river_exit(uint8 d) {}

// DOG_BONES
//const int BONES_INIT = 0; // defined above
//const int BONES_FLIP = 1; // defined above
const int BONES_VX0 = 2;
const int BONES_VX1 = 3;
const int BONES_VY0 = 4;
const int BONES_VY1 = 5;
const int BONES_X1 = 6;
const int BONES_Y1 = 7;

void dog_init_bones(uint8 d)
{
	dgd(BONES_INIT) = 255;
}
void dog_tick_bones(uint8 d)
{
	if (dgd(BONES_INIT) == 0)
	{
		uint16 bvy = (uint16)(-800);
		uint16 bvx = prng() << 1;
		if (prng() & 1) bvx = -bvx;

		dgd_put16(bvx,BONES_VX0,BONES_VX1);
		dgd_put16(bvy,BONES_VY0,BONES_VY1);

		dgd(BONES_INIT) = 1;
		return;
	}
	else if (dgd(BONES_INIT) == 255) return;

	// 16 bit velocity add
	uint32 bx = dgd_get24x(BONES_X1);
	uint16 by = dgd_get16y(BONES_Y1);
	uint16 by_old = by;

	bx += sint16(dgd_get16(BONES_VX0,BONES_VX1));
	by += dgd_get16(BONES_VY0,BONES_VY1);

	dgd_put24x(bx,BONES_X1);

	if (
		(!zp.room_scrolling && dgx >= 256) ||
		(zp.room_scrolling && dgx >= 512)
		)
	{
		// kill on wrap
		dgd(BONES_INIT) = 255;
	}
	else if ((dgd(BONES_VY0) >= 0x80) || (by >= by_old))
	{
		dgd_put16y(by,BONES_Y1);

		uint16 bvy = dgd_get16(BONES_VY0,BONES_VY1);
		uint16 GRAVITY = 100;
		bvy += GRAVITY;
		dgd_put16(bvy,BONES_VY0,BONES_VY1);
	}
	else
	{
		dgd(BONES_INIT) = 255;
	}
}
void dog_draw_bones(uint8 d)
{
	if (dgd(BONES_INIT) == 0) return;
	if (dgd(BONES_INIT) == 255) return;

	dx_scroll_edge();
	if (dgd(BONES_FLIP)) sprite0_add_flipped(dx,dgy,dgp);
	else                 sprite0_add(        dx,dgy,dgp);
}

// DOG_EASY

const int EASY_Y     = 0;
const int EASY_ANGLE = 1;

const uint8 EASY_SPEED = 5;
const uint8 EASY_WAVE  = 50;

void dog_init_easy(uint8 d)
{
	NES_ASSERT(zp.room_scrolling == 0, "DOG_EASY does not support scrolling!");

	if (zp.easy == 0xFF)
	{
		dgd(EASY_Y) = 4;
	}
	else
	{
		dgd(EASY_Y) = dgy;
	}
}
void dog_tick_easy(uint8 d)
{
	if (zp.easy == 0) return;
	else if (zp.easy == 0xFF) zp.easy = 1;

	if (dgd(EASY_Y) < dgy) ++dgd(EASY_Y);
	dgd(EASY_ANGLE) += EASY_SPEED;
}
void dog_draw_easy(uint8 d)
{
	if (zp.easy == 0) return;
	if (zp.easy == 0xFF) return;

	dx_screen();

	uint8 angle = dgd(EASY_ANGLE);
	uint8 dy = dgd(EASY_Y);

	sprite_tile_add(dx-16,dy + circle4[angle],0x80,0x03); angle += EASY_WAVE;
	sprite_tile_add(dx- 8,dy + circle4[angle],0x81,0x03); angle += EASY_WAVE;
	sprite_tile_add(dx   ,dy + circle4[angle],0x82,0x03); angle += EASY_WAVE;
	sprite_tile_add(dx+ 8,dy + circle4[angle],0x83,0x03);
}

// DOG_SPRITE0

void dog_init_sprite0(uint8 d) { }
void dog_tick_sprite0(uint8 d) { }
void dog_draw_sprite0(uint8 d)
{
	dx_scroll_edge();
	sprite0_add(dx,dgy,dgp);
}

// DOG_SPRITE2

void dog_init_sprite2(uint8 d) { }
void dog_tick_sprite2(uint8 d) { }
void dog_draw_sprite2(uint8 d)
{
	dx_scroll_edge();
	sprite2_add(dx,dgy,dgp);
}

// DOG_HINTD
const int HINT_ON = 0;

void dog_draw_hint_common(uint8 d, uint8 tile, uint8 attrib)
{
	if (dgd(HINT_ON) == 0) return;
	dx_scroll();
	sprite_tile_add(dx,dgy,tile,attrib);
}

void dog_init_hintd(uint8 d) { }
void dog_tick_hintd(uint8 d)
{
	dgd(HINT_ON) = 0;

	if (lizard.dead) return;
	if (lizard.power == 0) return;
	if (zp.current_lizard != LIZARD_OF_KNOWLEDGE) return;
	if (bound_overlap(d,-8,-7,16,17)) return;

	dgd(HINT_ON) = 1;
}
void dog_draw_hintd(uint8 d)
{
	dog_draw_hint_common(d,0xFD,0x01);
}

// DOG_HINTU

void dog_init_hintu(uint8 d) { dog_init_hintd(d); }
void dog_tick_hintu(uint8 d) { dog_tick_hintd(d); }
void dog_draw_hintu(uint8 d)
{
	dog_draw_hint_common(d,0xFD,0x81);
}

// DOG_HINTL

void dog_init_hintl(uint8 d) { dog_init_hintd(d); }
void dog_tick_hintl(uint8 d) { dog_tick_hintd(d); }
void dog_draw_hintl(uint8 d)
{
	dog_draw_hint_common(d,0xFE,0x41);
}

// DOG_HINTR

void dog_init_hintr(uint8 d) { dog_init_hintd(d); }
void dog_tick_hintr(uint8 d) { dog_tick_hintd(d); }
void dog_draw_hintr(uint8 d)
{
	dog_draw_hint_common(d,0xFE,0x01);
}

// DOG_HINT_PENGUIN

void dog_hint_penguin_locate(uint8 d)
{
	dgx = dog_x(d-1) - 4;
	dgy = dog_y(d-1) - 32;
}
void dog_init_hint_penguin(uint8 d)
{
	NES_ASSERT(d > 0 && dog_type(d-1) == DOG_PENGUIN, "DOG_HINT_PENGUIN requires penguin in previous slot!")
	if (d == 0) { empty_dog(d); return; }
	dog_hint_penguin_locate(d);
}
void dog_tick_hint_penguin(uint8 d)
{
	dog_hint_penguin_locate(d);
	dog_tick_hintd(d);
}
void dog_draw_hint_penguin(uint8 d) { dog_draw_hintd(d); }

// DOG_BIRD

const int BIRD_FACE  = 0;
const int BIRD_ANIM  = 1;
const int BIRD_FRAME = 2;

const uint8 BIRD_FLAP[4] = { 0x91, 0x92, 0x93, 0x92 };
const int BIRD_RANGE = 16;

void dog_init_bird(uint8 d)
{
	dgd(BIRD_FACE) = prng() & 1;
	dgd(BIRD_FRAME) = 0x90;
}
void dog_tick_bird(uint8 d)
{
	if (bound_touch(d,-2,-6,1,-2))
	{
		bones_convert(d,DATA_sprite0_bird_skull,dgd(BIRD_FACE));
		return;
	}

	if (dgd(BIRD_FRAME) == 0x90) // sitting
	{
		if (bound_overlap(d,-BIRD_RANGE,-(BIRD_RANGE+4),BIRD_RANGE-1,BIRD_RANGE-4))
		{
			dgd(BIRD_FRAME) = BIRD_FLAP[0]; // initiate flight
			dgd(BIRD_FACE) = (dgx < lizard_px) ? 1 : 0;
		}
		else return;
	}

	// fly away
	if (!dgd(BIRD_FACE)) 
	{
		dgx += 2;
		if ((zp.room_scrolling &&  dgx >= 510) ||
			(!zp.room_scrolling && dgx >= 254))
		{
			empty_dog(d);
			return;
		}
	}
	else
	{
		dgx -= 2;
		if (dgx < 5)
		{
			empty_dog(d);
			return;
		}
	}

	dgy -= (prng() & 1) + 1;
	if (dgy < 10)
	{
		empty_dog(d);
		return;
	}

	++dgd(BIRD_ANIM);
	dgd(BIRD_FRAME) = BIRD_FLAP[(dgd(BIRD_ANIM) >> 2) & 3];
}
void dog_draw_bird(uint8 d)
{
	dx_scroll_offset(-4);
	uint8 attrib = dgd(BIRD_FACE) ? 0x41 : 0x01;
	sprite_tile_add(dx,dgy-9,dgd(BIRD_FRAME),attrib);
}

// DOG_FROG

const int FROG_FACE  = 0;
const int FROG_VY0   = 1; // msb
const int FROG_VY1   = 2; // lsb
const int FROG_Y1    = 3; // lsb of dog_y msb
const int FROG_ANIM  = 4;
const int FROG_FLY   = 5;

const int FROG_RANGE = 12;
const sint16 FROG_JUMP = -500;
const sint16 FROG_GRAVITY = 60;

void dog_init_frog(uint8 d)
{
	dgd(FROG_ANIM) = prng();
	dgd(FROG_FACE) = prng() & 1;

}
void dog_tick_frog(uint8 d)
{
	// remove if now offscreen
	if (dgx >= 509 || dgx < 5 || dgy >= 240)
	{
		empty_dog(d);
		return;
	}

	// talk
	if (zp.current_lizard == LIZARD_OF_KNOWLEDGE &&
		lizard.power > 0 &&
		h.frogs_fractioned == FROG_FRACTION_COUNT &&
		bound_overlap(d,-FROG_RANGE,-(FROG_RANGE+1),FROG_RANGE-1,FROG_RANGE-4)
		)
	{
		h.frogs_fractioned = 255;
		zp.game_message = TEXT_FROG;
		zp.game_pause = 1;
		return;
	}

	// touched by fire/death
	if (bound_touch(d,-3,-4,2,-1))
	{
		if (h.frogs_fractioned < FROG_FRACTION_COUNT)
		{
			++h.frogs_fractioned;
		}

		uint8 s = DATA_sprite0_frog_skull;
		if (dgt == DOG_GROG)
		{
			s = DATA_sprite0_grog_skull;
		}
		bones_convert(d,s,dgd(FROG_FACE));
		return;
	}

	// not flying, croak or maybe jump
	if (!dgd(FROG_FLY))
	{
		bool do_jump = false;

		// random croak
		if (dgd(FROG_ANIM) == 0)
		{
			dgd(FROG_ANIM) = prng(2);
			if ((prng(3) & 15) == 0)
			{
				do_jump = true;
			}
		}
		else --dgd(FROG_ANIM);

		// scared by lizard
		if (bound_overlap(d,-FROG_RANGE,-(FROG_RANGE+1),FROG_RANGE-1,FROG_RANGE-4))
		{
			do_jump = true;
		}

		if (do_jump)
		{
			dgd(FROG_FLY) = 1;
			dgd(FROG_FACE) = (dgx < lizard_px) ? 1 : 0;

			uint16 vy = FROG_JUMP - prng();
			dgd_put16(vy,FROG_VY0,FROG_VY1);
		}

		return;
	}

	// flying

	uint16 vy16 = dgd_get16(FROG_VY0,FROG_VY1);
	uint16 y16 = dgd_get16y(FROG_Y1);

	y16 += vy16;
	vy16 += FROG_GRAVITY;
	dgd_put16(vy16,FROG_VY0,FROG_VY1);
	if (dgd(FROG_VY0) < 128 && dgd(FROG_VY0) >= 6) dgd(FROG_VY0) = 5; // cap to <6 pixels/frame

	sint8 dx = !dgd(FROG_FACE) ? 1 : -1;
	sint8 dy = (y16 >> 8) - dgy;
	uint8 old_dgy = dgy;

	uint8 landed = move_dog(d,-3,-4,2,-1,&dx,&dy);
	dgx += dx;
	dgy += dy;

	if (landed & 4)
	{
		dgd(FROG_Y1) = 0;
		dgd(FROG_FACE) = prng() & 1;
		if (prng() & 1) // jump again
		{
			vy16 = FROG_JUMP- prng();
			dgd_put16(vy16,FROG_VY0,FROG_VY1);
		}
		else
		{
			dgd(FROG_FLY) = 0;
		}
	}
	else
	{
		dgd(FROG_Y1) = (y16 & 0xFF);
	}

	// splash
	if ((old_dgy < zp.water) != (dgy < zp.water))
	{
		play_sound_scroll(SOUND_SPLASH_SMALL);
		if (dog_type(SPLASHER_SLOT) == DOG_SPLASHER)
		{
			dog_x(SPLASHER_SLOT) = dgx - 4;
			dog_y(SPLASHER_SLOT) = zp.water - 9;
			dog_data(0,SPLASHER_SLOT) = 0; // trigger splash animation
		}
	}
}
void dog_draw_frog_common(uint8 d, uint8 attribute, uint8 tile)
{
	dx_scroll_offset(-4);
	attribute |= (dgd(FROG_FACE) ? 0x40 : 0x00);
	if (dgd(FROG_FLY))
	{
		sprite_tile_add(dx,dgy-9,tile | 0x02,attribute);
	}
	else
	{
		tile |= (dgd(FROG_ANIM) < 6) ? 0x01 : 0x00;
		sprite_tile_add(dx,dgy-9,tile,attribute);
	}
}
void dog_draw_frog(uint8 d)
{
	dog_draw_frog_common(d,2,0x80);
}

// DOG_GROG

void dog_init_grog(uint8 d)
{
	dog_init_frog(d);
}
void dog_tick_grog(uint8 d)
{
	dog_tick_frog(d);

	if (bound_overlap_no_stone(d,-3,-4,2,-1))
	{
		lizard_die();
	}
}
void dog_draw_grog(uint8 d)
{
	dog_draw_frog_common(d,2|(d&1),0x70);
}

// DOG_PANDA

const int PANDA_MODE = 0;
const int PANDA_TIME = 1;
const int PANDA_TALK = 2;
//const int FIRE_PAL = 10;
//const int FIRE_ANIM = 11;

void dog_init_panda(uint8 d)
{
	dog_blocker(d,-8,-26,7,-19);
}
void dog_tick_panda_fire(uint8 d); // forward
void dog_tick_panda(uint8 d)
{
	if (bound_touch(d,-14,-20,13,-1))
	{
		if (zp.current_lizard == LIZARD_OF_DEATH)
		{
			empty_blocker(d);
			bones_convert(d,DATA_sprite0_panda_skull,(dgd(PANDA_MODE) == 2) ? 1 : 0);
			return;
		}
		if(zp.current_lizard == LIZARD_OF_HEAT && dgt != DOG_PANDA_FIRE)
		{
			dgt = DOG_PANDA_FIRE;
			dog_tick_panda_fire(d);
			return;
		}
	}

	if ((dgd(PANDA_MODE) == 1 && bound_overlap_no_stone(d,-15, -26, -10, -8)) ||
		(dgd(PANDA_MODE) == 2 && bound_overlap_no_stone(d,  9, -26, +14, -8)))
	{
		lizard_die();
	}

	if (!dgd(PANDA_TALK) &&
		!lizard.dead &&
		lizard.power != 0 &&
		zp.current_lizard == LIZARD_OF_KNOWLEDGE &&
		bound_overlap(d,-64,-64,64,32))
	{
		zp.game_message = TEXT_PANDA_TALK;
		if (dgd(PANDA_MODE) == 3)
		{
			zp.game_message = TEXT_PANDA_HEAD;
		}
		else
		{
			dgd(PANDA_TALK) = 1;
		}
		zp.game_pause = 1;
		return;
	}

	if (bound_overlap(d, -8, -27, 7, -27))
	{
		if (dgd(PANDA_MODE) != 3)
		{
			play_sound(SOUND_PANDA_SIGH);
			dgd(PANDA_MODE) = 3; // step
			dgd(PANDA_TALK) = 0;
		}
		dgd(PANDA_TIME) = 15;
	}
	else if (dgd(PANDA_TIME) == 0)
	{
		if (bound_overlap(d, -30, -50, -15, -1))
		{
			if (dgd(PANDA_MODE) != 1) play_sound(SOUND_PANDA_SWIPE);
			dgd(PANDA_MODE) = 1;
			dgd(PANDA_TIME) = 15;
		}
		else if (bound_overlap(d, 14, -50, 29, -1))
		{
			if (dgd(PANDA_MODE) != 2) play_sound(SOUND_PANDA_SWIPE);
			dgd(PANDA_MODE) = 2;
			dgd(PANDA_TIME) = 15;
		}
		else dgd(PANDA_MODE) = 0;
	}
	else
	{
		--dgd(PANDA_TIME);
	}

}
void dog_draw_panda(uint8 d)
{
	dx_scroll_edge();

	switch (dgd(PANDA_MODE))
	{
	default: NES_ASSERT(false,"Invalid panda mode!"); // fallthrough
	case 0: sprite0_add(        dx,dgy,DATA_sprite0_panda     ); break;
	case 1: sprite0_add(        dx,dgy,DATA_sprite0_panda_mad ); break;
	case 2: sprite0_add_flipped(dx,dgy,DATA_sprite0_panda_mad ); break;
	case 3: sprite0_add(        dx,dgy,DATA_sprite0_panda_step); break;
	}
}

// DOG_GOAT

const int GOAT_MODE = 0; // 0=stand, 1=to-jump, 2=fly, 3=splat
const int GOAT_FLIP = 1;
const int GOAT_GUAT = 2; // vertical flip
const int GOAT_ANIM = 3;
const int GOAT_TIME = 4; // time until flip
const int GOAT_FACE = 5; // 0=left, 1=right, 2=blink
const int GOAT_LOOK = 6; // time for face
//const int FIRE_PAL = 10;
//const int FIRE_ANIM = 11;

const uint8 GUAT_SHIFT = DATA_sprite0_guat - DATA_sprite0_goat;

void dog_init_goat(uint8 d)
{
	CT_ASSERT(DATA_sprite0_goat        + GUAT_SHIFT == DATA_sprite0_guat,        "goat/guat sprites misaligned");
	CT_ASSERT(DATA_sprite0_goat_look   + GUAT_SHIFT == DATA_sprite0_guat_look,   "goat/guat sprites misaligned");
	CT_ASSERT(DATA_sprite0_goat_blink  + GUAT_SHIFT == DATA_sprite0_guat_blink,  "goat/guat sprites misaligned");
	CT_ASSERT(DATA_sprite0_goat_crouch + GUAT_SHIFT == DATA_sprite0_guat_crouch, "goat/guat sprites misaligned");
	CT_ASSERT(DATA_sprite0_goat_fly0   + GUAT_SHIFT == DATA_sprite0_guat_fly0,   "goat/guat sprites misaligned");
	CT_ASSERT(DATA_sprite0_goat_fly1   + GUAT_SHIFT == DATA_sprite0_guat_fly1,   "goat/guat sprites misaligned");
	CT_ASSERT(DATA_sprite0_goat_splat  + GUAT_SHIFT == DATA_sprite0_guat_splat,  "goat/guat sprites misaligned");
	CT_ASSERT(DATA_sprite0_goat_skull  + GUAT_SHIFT == DATA_sprite0_guat_skull,  "goat/guat sprites misaligned");

	CT_ASSERT(DATA_sprite0_goat_look  == DATA_sprite0_goat      + 1, "goat sprites misaligned");
	CT_ASSERT(DATA_sprite0_goat_blink == DATA_sprite0_goat_look + 1, "goat sprites misaligned");

	dgd(GOAT_TIME) = prng(3);
	dgd(GOAT_FLIP) = prng() & 1;
}
void dog_tick_goat(uint8 d)
{
	if (bound_touch(d,-8,-8,7,-1))
	{
		if (zp.current_lizard == LIZARD_OF_DEATH)
		{
			uint8 s = DATA_sprite0_goat_skull;
			if (dgd(GOAT_GUAT)) s += GUAT_SHIFT;
			bones_convert(d,s,dgd(GOAT_FLIP));
			return;
		}
		else if (zp.current_lizard == LIZARD_OF_HEAT)
		{
			// force a jump immediatel if standing
			if (dgd(GOAT_MODE) == 0)
			{
				dgd(GOAT_ANIM) = dgd(GOAT_TIME);
			}
		}
	}

	++dgd(GOAT_ANIM);

	switch (dgd(GOAT_MODE))
	{
	case 0: // stand
		// animate face
		if (dgd(GOAT_LOOK) == 0)
		{
			dgd(GOAT_FACE) = prng() & 3; // 0 left, 1 right, 2 blink
			if (dgd(GOAT_FACE) == 3) // 3 invalid > double chances of left/right
				dgd(GOAT_FACE) = prng() & 1;

			dgd(GOAT_LOOK) = prng(3) & 127;
			if (dgd(GOAT_FACE) == 2)
				dgd(GOAT_LOOK) &= 15; // blink is shortened
			dgd(GOAT_LOOK) += 5;
		}
		else --dgd(GOAT_LOOK);
		// begin jump
		if (dgd(GOAT_ANIM) >= dgd(GOAT_TIME))
		{
			dgd(GOAT_ANIM) = 0;
			dgd(GOAT_MODE) = 1;
			play_sound_scroll(SOUND_GOAT_JUMP);
		}
		break;
	case 1: // to jump
		if (dgd(GOAT_ANIM) < 32)
			break;
		if (dgd(GOAT_GUAT)) dgy += 1;
		else                dgy -= 1;
		dgd(GOAT_MODE) = 2; // fly
		// fallthrough
	case 2: // fly
		if (dgd(GOAT_GUAT) == 0)
		{
			dgy -= 3;
			uint8 shift = collide_tile_up(dgx,dgy-8);
			if (shift)
			{
				dgy += shift;
				dgd(GOAT_ANIM) = 0;
				dgd(GOAT_MODE) = 3;
				play_sound_scroll(SOUND_GOAT_SPLAT);
			}
		}
		else
		{
			dgy += 3;
			uint8 shift = collide_tile_down(dgx,dgy-1);
			if (shift)
			{
				dgy -= shift;
				dgd(GOAT_ANIM) = 0;
				dgd(GOAT_MODE) = 3;
				play_sound(SOUND_GOAT_SPLAT);
			}
		}
		break;
	case 3: // splat
		if (dgd(GOAT_ANIM) == 4)
		{
			dgd(GOAT_GUAT) ^= 1;
		}
		else if (dgd(GOAT_ANIM) >= 12)
		{
			dgd(GOAT_ANIM) = 0;
			dgd(GOAT_MODE) = 0;
			dgd(GOAT_TIME) = 8 + (prng(3) & 127);
		}
		break;
	default:
		NES_ASSERT(false,"invalid dog_goat mode");
	}

	if (bound_overlap_no_stone(d,-8,-8,7,-1))
	{
		lizard_die();
	}
}
void dog_draw_goat(uint8 d)
{
	dx_scroll_edge();

	uint8 s = DATA_sprite0_goat;

	switch(dgd(GOAT_MODE))
	{
	case 0:
		s += dgd(GOAT_FACE);
		break;
	case 1:
		s = DATA_sprite0_goat_crouch;
		break;
	case 2:
		s = DATA_sprite0_goat_fly0 + (prng() & 1);
		break;
	case 3:
		s = DATA_sprite0_goat_splat;
		if (dgd(GOAT_ANIM) >= 8)
			s = DATA_sprite0_goat_crouch;
		break;
	}

	if (dgd(GOAT_GUAT))
		s += GUAT_SHIFT;

	if (dgd(GOAT_FLIP)) sprite0_add_flipped(dx,dgy,s);
	else                sprite0_add(        dx,dgy,s);
}

// DOG_DOG
// abbreviated DDOG to differentiate from DOG

const int DDOG_ANIM = 0;
const int DDOG_TIME = 1; // time to mode switch, or anim cycle for run
const int DDOG_MODE = 2; // 0=sit, 1=lick, 2=turn, 3=walk, 4=run, 5=leap
const int DDOG_FLIP = 3;
const int DDOG_Y1   = 4; // 16 bit Y for leap only
const int DDOG_VY0  = 5;
const int DDOG_VY1  = 6;
const int DDOG_LAND = 7; // starting point of jump
//const int FIRE_PAL = 10;
//const int FIRE_ANIM = 11;

const sint16 DDOG_JUMP = -940;
const sint16 DDOG_GRAV = 60;

void dog_dog_mode(uint8 d, uint8 mode)
{
	dgd(DDOG_MODE) = mode;
	dgd(DDOG_ANIM) = 0;

	dgd(DDOG_TIME) = 64 + (prng(3) & 127);
}
void dog_dog_patrol(uint8 d)
{
	sint8 vdiff = dgy - lizard_py;
	if (vdiff > 10 || vdiff < -10) return;

	if (dgd(DDOG_FLIP) == 0)
	{
		//if (lizard_px >= dgx || ((dgx-lizard_px) >= 100)) return;
		if (lizard_px >= dgx) return;
	}
	else
	{
		//if (lizard_px <= dgx || ((lizard_px-dgx) >= 100)) return;
		if (lizard_px <= dgx) return;
	}

	dog_dog_mode(d,4); // run
}
void dog_dog_leap(uint8 d)
{
	if ((prng(3) & 7) != 0) return;

	if (dgd(DDOG_FLIP) == 0)
	{
		if (lizard_px >= dgx) return; // only jump toward lizard
		if (dgx < 80) return; // too close to edge
		if (!collide_tile(dgx-72,dgy) ||
			!collide_tile(dgx-56,dgy)) return; // no place to land
		if (collide_tile(dgx-72,dgy-1)) return; // landing is blocked
		if (!bound_overlap(d,-78,-8,-70,-1) &&
			!bound_overlap(d,-60,-40,-16,-16)
			) return; // no lizard to jump on
	}
	else
	{
		if (lizard_px <= dgx) return;
		if (dgx >= (511-80)) return;
		if (!collide_tile(dgx+72,dgy) ||
			!collide_tile(dgx+56,dgy)) return;
		if (collide_tile(dgx+72,dgy-1)) return;
		if (!bound_overlap(d,69,-8,77,-1) &&
			!bound_overlap(d,15,-40,59,-16)
			) return;
	}

	dgd(DDOG_LAND) = dgy;
	dgd(DDOG_Y1) = 0;
	dgd_put16(DDOG_JUMP,DDOG_VY0,DDOG_VY1);
	dog_dog_mode(d,5); // leap
	play_sound(SOUND_BARK);
}
void dog_dog_move(uint8 d)
{
	if (dgd(DDOG_MODE) == 2) return; // turning

	if (dgd(DDOG_FLIP) == 0)
	{
		if (collide_tile(dgx-10,dgy-8) || !collide_tile(dgx-11,dgy) || dgx < 18)
			dog_dog_mode(d,2); // turn
		else
			--dgx;
	}
	else
	{
		if (collide_tile(dgx+9,dgy-8) || !collide_tile(dgx+10,dgy) || dgx >= 495)
			dog_dog_mode(d,2); // turn
		else
			++dgx;
	}
}
void dog_dog_bound_body(uint8 d, sint16& x0, sint8& y0, sint16& x1, sint8& y1)
{
	if (dgd(DDOG_MODE) <= 1) // sit/lick
	{
		x0 = -6;
		y0 = -10;
		x1 = 5;
		y1 = -1;
		return;
	}

	x0 = -8;
	y0 = -11;
	x1 = 7;
	y1 = -4;
	return;
}
void dog_dog_bound_head(uint8 d, sint16& x0, sint8& y0, sint16& x1, sint8& y1)
{
	switch (dgd(DDOG_MODE))
	{
	case 0: // sit
		x0 = -10;
		y0 = -12;
		x1 = -3;
		y1 = -11;
		break;
	default:
	case 1: // lick
	case 2: // turn
		// hide head inside body
		x0 = 0;
		y0 = -8;
		x1 = 0;
		y1 = -8;
		break;
	case 3: // walk
	case 4: // run
		x0 = -16;
		y0 = -12;
		x1 = -9;
		y1 = -11;
		break;
	case 5: // leap
		x0 = -16;
		y0 = -10; // Y lowered 2 pixels
		x1 = -9;
		y1 = -9;
		break;
	}

	if (dgd(DDOG_FLIP) != 0)
	{
		sint16 temp = x0;
		x0 = (-x1) - 1;
		x1 = (-temp) - 1;
	}
}

void dog_init_dog(uint8 d)
{
	dgd(DDOG_FLIP) = prng() & 1;
	dgd(DDOG_TIME) = 16;
}
void dog_tick_dog(uint8 d)
{
	sint16 bx0,bx1,hx0,hx1;
	sint8  by0,by1,hy0,hy1;
	dog_dog_bound_body(d,bx0,by0,bx1,by1);
	dog_dog_bound_head(d,hx0,hy0,hx1,hy1);
	if (bound_touch(d,bx0,by0,bx1,by1) ||
		bound_touch(d,hx0,hy0,hx1,hy1))
	{
		if (zp.current_lizard == LIZARD_OF_DEATH)
		{
			bones_convert(d,DATA_sprite0_dog_skull,dgd(DDOG_FLIP));
			return;
		}
		else if (dgt != DOG_DOG_FIRE && zp.current_lizard == LIZARD_OF_HEAT)
		{
			// if not leaping or licking, force lick
			if (dgd(DDOG_MODE) != 5 && dgd(DDOG_MODE) != 1)
			{
				dog_dog_mode(d,1);
				dgd(DDOG_TIME) |= 128; // last at least 2 seconds
			}
			else if (dgd(DDOG_MODE) == 1)
			{
				dgd(DDOG_ANIM) &= 31;
				dgd(DDOG_TIME) |= 128;
			}
		}
	}

	++dgd(DDOG_ANIM);

	switch(dgd(DDOG_MODE))
	{
	case 0: // sit
		{
			if (dgd(DDOG_ANIM) >= dgd(DDOG_TIME))
			{
				if ((prng(3) & 7) == 0)
					dog_dog_mode(d,1); // lick
				else
					dog_dog_mode(d,3); // walk
			}
			dog_dog_patrol(d); // patrol overrides anything else that might happen
		} break;
	case 1: // lick
		{
			if (dgd(DDOG_ANIM) >= dgd(DDOG_TIME))
			{
				dog_dog_mode(d,0); // sit
			}
		} break;
	case 2: // turn
		{
			if (dgd(DDOG_ANIM) ==  8) dgd(DDOG_FLIP) ^= 1;
			if (dgd(DDOG_ANIM) >= 16)
			{
				dog_dog_mode(d,3); // walk
			}
		} break;
	case 3: // walk
		{
			if ((dgd(DDOG_ANIM) & 1) == 0)
			{
				dog_dog_move(d);
			}

			if (dgd(DDOG_ANIM) >= dgd(DDOG_TIME))
			{
				//if ((prng(2) & 3) == 0)
				//	dog_dog_mode(d,2); // turn
				//else
					dog_dog_mode(d,0); // sit
			}
			dog_dog_patrol(d);
		} break;
	case 4: // run
		{
			// animate
			if (dgd(DDOG_ANIM) >= 5)
			{
				++dgd(DDOG_TIME);
				dgd(DDOG_ANIM) = 0;
			}

			dog_dog_move(d);
			dog_dog_move(d);
			dog_dog_leap(d); // possibly enter leap
		} break;
	case 5: // leap
		{
			if (dgd(DDOG_FLIP) == 0) { if (!collide_tile(dgx-10,dgy-8)) dgx -= 2; }
			else                     { if (!collide_tile(dgx+ 9,dgy-8)) dgx += 2; }

			uint16 y16 = dgd_get16y(DDOG_Y1);
			sint16 vy16 = dgd_get16(DDOG_VY0,DDOG_VY1);

			y16 += vy16;
			dgd_put16y(y16,DDOG_Y1);

			vy16 += DDOG_GRAV;
			dgd_put16(vy16,DDOG_VY0,DDOG_VY1);

			if (dgy >= dgd(DDOG_LAND))
			{
				dgy = dgd(DDOG_LAND);
				dog_dog_mode(d,4); // run
			}
		} break;
	}

	dog_dog_bound_body(d,bx0,by0,bx1,by1);
	dog_dog_bound_head(d,hx0,hy0,hx1,hy1);
	if (bound_overlap_no_stone(d,bx0,by0,bx1,by1) ||
		bound_overlap_no_stone(d,hx0,hy0,hx1,hy1))
	{
		lizard_die();
	}
}
void dog_draw_dog(uint8 d)
{
	dx_scroll_edge();

	uint8 s = DATA_sprite0_dog;

	switch(dgd(DDOG_MODE))
	{
	case 0: // sit
		break;
	case 1: // lick
		s = DATA_sprite0_dog_lick0 + ((dgd(DDOG_ANIM) >> 4) & 1);
		break;
	case 2: // turn
		if (dgd(DDOG_ANIM) < 8)
			s = DATA_sprite0_dog_turn;
		// else is DATA_sprite0_dog
		break;
	case 3: // walk
		s = DATA_sprite0_dog_walk0 + ((dgd(DDOG_ANIM) >> 3) & 3);
		break;
	case 4: // run
		s = DATA_sprite0_dog_run0  + (dgd(DDOG_TIME) & 3);
		break;
	case 5: // leap
		s = DATA_sprite0_dog_leap;
		break;
	default:
		NES_ASSERT(false,"invalid dog_dog mode!");
	}

	if (dgd(DDOG_FLIP)) sprite0_add_flipped(dx,dgy,s);
	else                sprite0_add(        dx,dgy,s);
}

// DOG_WOLF

void dog_init_wolf(uint8 d) { dog_init_dog(d); }
void dog_tick_wolf(uint8 d) { dog_tick_dog(d); }
void dog_draw_wolf(uint8 d) { dog_draw_dog(d); }

// DOG_OWL

const int OWL_ANIM = 0;
const int OWL_MODE = 1; // 0 = flap, 1 = dive
const int OWL_Y1   = 2;
const int OWL_VY0  = 3;
const int OWL_VY1  = 4;
const int OWL_DIR  = 5;
const int OWL_FLAP = 6; // controls flap sound
//const int FIRE_PAL = 10;
//const int FIRE_ANIM = 11;

const sint16 OWL_ACCEL = 60;
const uint8 OWL_HANG_TIME = 20; // must be divisible by 4
const uint8 OWL_DIVE_TIME = 68; // must be divisible by 4
const uint8 OWL_FLAP_TIME = 8;  // must be power of 2
const uint8 OWL_PEAK_TIME = OWL_HANG_TIME / 4;
const uint8 OWL_DIVE_A = OWL_DIVE_TIME / 4;
const uint8 OWL_DIVE_B = 3 * OWL_DIVE_A;

CT_ASSERT((OWL_HANG_TIME & 3) == 0,"OWL_HANG_TIME must be divisible by 4");
CT_ASSERT((OWL_DIVE_TIME & 3) == 0,"OWL_DIVE_TIME must be divisible by 4");
CT_ASSERT(IS_POWER_OF_TWO(OWL_FLAP_TIME),"OWL_FLAP_TIME must be power of 2");

void dog_blocker_owl(uint8 d)
{
	if (dgt == DOG_OWL_FIRE)
	{
		empty_blocker(d);
		return;
	}
	dog_blocker(d,-5,-14,3,-6);
}

void dog_init_owl(uint8 d)
{
	dgp &= 1; // force to 0 or 1, param controls dive direction, flips after each dive
	dgd(OWL_ANIM) = OWL_PEAK_TIME; // VY = 0 at peak/antipeak
	dog_blocker_owl(d);
}
void dog_tick_owl(uint8 d)
{
	if (bound_touch_death(d,-6,-12,4,-1))
	{
		empty_blocker(d);
		bones_convert(d,DATA_sprite0_owl_skull,0);
		return;
	}

	sint8 dx = 0;
	sint8 dy = 0;
	uint8 old_anim = dgd(OWL_ANIM);
	uint8 old_mode = dgd(OWL_MODE);
	uint8 old_dir  = dgd(OWL_DIR);

	if (dgd(OWL_MODE) == 0) // hang
	{
		if (dgd(OWL_ANIM) < (OWL_HANG_TIME/2)) dgd(OWL_DIR) = 0;
		else                                   dgd(OWL_DIR) = 1;

		if (dgd(OWL_ANIM) == (OWL_HANG_TIME/2))
		{
			play_sound_scroll(SOUND_OWL_FLAP);
		}

		++dgd(OWL_ANIM);
		if (dgd(OWL_ANIM) == OWL_PEAK_TIME)
		{
			// 1 in 8 chance of flying for no reason
			uint8 r = prng(2) & 7;

			// 50/50 chance of flying if baited
			if (dgp == 0 && bound_overlap_no_stone(d,-64,0, 8,72)) r |= 6;
			if (dgp != 0 && bound_overlap_no_stone(d, -8,0,64,72)) r |= 6;

			if (r == 7)
			{
				dgd(OWL_MODE) = 1;
				dgd(OWL_ANIM) = 0;
				dgd(OWL_FLAP) = 0;
			}
		}
		else if (dgd(OWL_ANIM) >= OWL_HANG_TIME)
		{
			dgd(OWL_ANIM) = 0;
		}
	}
	else // dive
	{
		// acceleration control
		if (dgd(OWL_ANIM) < OWL_DIVE_A || dgd(OWL_ANIM) >= OWL_DIVE_B)
			dgd(OWL_DIR) = 0;
		else
		{
			if ((dgd(OWL_FLAP) & (OWL_FLAP_TIME-1)) == 0)
			{
				play_sound_scroll(SOUND_OWL_FLAP);
			}
			dgd(OWL_DIR) = 1;
		}
		++dgd(OWL_FLAP);

		// move horizontally
		if (dgp == 0)
		{
			dx = -1;
		}
		else
		{
			dx = 1;
		}

		++dgd(OWL_ANIM);
		if (dgd(OWL_ANIM) >= OWL_DIVE_TIME)
		{
			dgd(OWL_ANIM) = OWL_PEAK_TIME; // resume hang at peak
			dgd(OWL_MODE) = 0;
			dgp ^= 1;
		}
	}

	// move vertically

	uint16 y16 = dgd_get16y(OWL_Y1);
	sint16 vy16 = dgd_get16(OWL_VY0,OWL_VY1);

	y16 += vy16;
	dy = (y16 >> 8) - dgy;

	bool allow_motion = false;
	if (bound_overlap(d,-5,-15,3,-15)) // riding
	{
		lizard.x += 256 * dx;
		lizard.y += 256 * dy;
		do_scroll();
		allow_motion = true;
	}
	else if (!bound_overlap(d,dx-5,dy-14,dx+3,dy-6)) // not blocked
	{
		allow_motion = true;
	}

	if (allow_motion)
	{
		dgx += dx;
		if (dgd(OWL_DIR) == 0) vy16 += OWL_ACCEL;
		else                   vy16 -= OWL_ACCEL;
		dgd_put16y(y16,OWL_Y1);
		dgd_put16(vy16,OWL_VY0,OWL_VY1);
	}
	else
	{
		dgd(OWL_ANIM) = old_anim;
		dgd(OWL_MODE) = old_mode;
		dgd(OWL_DIR)  = old_dir;
	}

	dog_blocker_owl(d);

	if (bound_overlap_no_stone(d,-6,-10,4,-1))
	{
		lizard_die();
	}
}
void dog_draw_owl(uint8 d)
{
	dx_scroll_edge();

	uint8 s = DATA_sprite0_owl;
	if (dgd(OWL_DIR) == 1)
	{
		if (dgd(OWL_MODE) == 0 || (dgd(OWL_FLAP) & (OWL_FLAP_TIME-1)) < (OWL_FLAP_TIME/2))
		{
			s = DATA_sprite0_owl_flap;
		}
	}
	sprite0_add(dx,dgy,s);
}

// DOG_ARMADILLO

const int ARMADILLO_ANIM = 0;
const int ARMADILLO_MODE = 1; // 0=walk, 1=turn, 2=roll
const int ARMADILLO_FLIP = 2;
const int ARMADILLO_ROLL = 3;
const int ARMADILLO_SHOW = 4;

const uint8 ARMADILLO_ROLL_TIME = 8;

const uint8 ARMADILLO_TURN_SPRITE[4] = {
	DATA_sprite0_armadillo_roll,
	DATA_sprite0_armadillo_ball0,
	DATA_sprite0_armadillo_ball0,
	DATA_sprite0_armadillo_roll
};

void dog_bound_armadillo(uint8 d, sint8& x0, sint8& y0, sint8& x1, sint8& y1)
{
	if (dgd(ARMADILLO_MODE) != 2)
	{
		x0 = -6;
		y0 = -8;
		x1 =  5;
		y1 = -1;
	}
	else // roll
	{
		x0 = -5;
		y0 = -10;
		x1 =  4;
		y1 = -2;
	}
}
bool dog_stop_armadillo_left(uint8 d)
{
	if (dgx < 17) return true;
	if (collide_all(dgx-6,dgy-3)) return true;
	if (!collide_all(dgx-7,dgy)) return true;
	return false;
}
bool dog_stop_armadillo_right(uint8 d)
{
	if (dgx >= 501) return true;
	if (collide_all(dgx+5,dgy-3)) return true;
	if (!collide_all(dgx+6,dgy)) return true;
	return false;
}
bool dog_move_armadillo(uint8 d)
{
	bool stop = false;
	if (dgd(ARMADILLO_FLIP) == 0)
	{
		--dgx;
		stop = dog_stop_armadillo_left(d);
	}
	else
	{
		++dgx;
		stop = dog_stop_armadillo_right(d);
	}

	return stop;
}

void dog_init_armadillo(uint8 d)
{
	CT_ASSERT(DATA_sprite0_armadillo      +1 == DATA_sprite0_armadillo_walk1,"armadillo sprites out of order");
	CT_ASSERT(DATA_sprite0_armadillo_walk1+1 == DATA_sprite0_armadillo_walk2,"armadillo sprites out of order");
	CT_ASSERT(DATA_sprite0_armadillo_ball0+1 == DATA_sprite0_armadillo_ball1,"armadillo sprites out of order");

	dgd(ARMADILLO_ROLL) = prng(4) | 64;
	dgd(ARMADILLO_FLIP) = prng() & 1;
}
void dog_tick_armadillo(uint8 d)
{
	sint8 bx0,by0,bx1,by1;
	
	// touch
	dog_bound_armadillo(d,bx0,by0,bx1,by1);
	if (bound_touch(d,bx0,by0,bx1,by1))
	{
		if (zp.current_lizard == LIZARD_OF_DEATH)
		{
			bones_convert(d,DATA_sprite0_armadillo_skull,dgd(ARMADILLO_FLIP));
			return;
		}
		else if (zp.current_lizard == LIZARD_OF_HEAT)
		{
			// fire triggers roll
			if (dgd(ARMADILLO_ROLL) >= ARMADILLO_ROLL_TIME)
			{
				dgd(ARMADILLO_ROLL) = ARMADILLO_ROLL_TIME;
			}
		}
	}

	// mode
	switch (dgd(ARMADILLO_MODE))
	{
	case 0: // walk
		if (dgd(ARMADILLO_ROLL) < ARMADILLO_ROLL_TIME)
		{
			if (dgd(ARMADILLO_ROLL) == 0)
			{
				play_sound_scroll(SOUND_ARMADILLO);
				dgd(ARMADILLO_MODE) = 2;
			}
			else
				--dgd(ARMADILLO_ROLL);
		}
		else if (dgd(ARMADILLO_ANIM) & 1)
		{
			--dgd(ARMADILLO_ROLL);

			if ((dgd(ARMADILLO_ANIM) & 3) == 3)
			{
				++dgd(ARMADILLO_SHOW);
				if (dgd(ARMADILLO_SHOW) > 2)
					dgd(ARMADILLO_SHOW) = 0;
			}

			++dgd(ARMADILLO_ANIM);
			if (dog_move_armadillo(d))
			{
				dgd(ARMADILLO_MODE) = 1;
				dgd(ARMADILLO_ANIM) = 0;
				dgd(ARMADILLO_SHOW) = 0;
			}
		}
		else
			++dgd(ARMADILLO_ANIM);
		break;
	case 1: // turn
		if (dgd(ARMADILLO_ANIM) >= ARMADILLO_ROLL_TIME)
		{
			dgd(ARMADILLO_ANIM) = 0;
			++dgd(ARMADILLO_SHOW);
			if (dgd(ARMADILLO_SHOW) >= 4) // end of turn
			{
				dgd(ARMADILLO_MODE) = 0;
				//dgd(ARMADILLO_ANIM) = 0;
				dgd(ARMADILLO_SHOW) = 0;
			}
			else if (dgd(ARMADILLO_SHOW) == 2) // turn
			{
				dgd(ARMADILLO_FLIP) ^= 1;
				if (dgd(ARMADILLO_ROLL) < ARMADILLO_ROLL_TIME) // enter roll immediately
				{
					dgd(ARMADILLO_ROLL) = 0;
					dgd(ARMADILLO_MODE) = 2;
				}
				else
					++dgd(ARMADILLO_ANIM);
			}
		}
		else
			++dgd(ARMADILLO_ANIM);
		break;
	case 2: // roll
		if (dog_move_armadillo(d))
		{
			dgd(ARMADILLO_MODE) = 1;
			dgd(ARMADILLO_ANIM) = ARMADILLO_ROLL_TIME;
			dgd(ARMADILLO_SHOW) = 1;
			dgd(ARMADILLO_ROLL) = prng(4) | 64;
		}
		break;
	default:
		NES_ASSERT(false,"unknown armadillo mode");
		break;
	}

	// overlap
	dog_bound_armadillo(d,bx0,by0,bx1,by1);
	if (bound_overlap_no_stone(d,bx0,by0,bx1,by1))
	{
		lizard_die();
	}
}
void dog_draw_armadillo(uint8 d)
{
	dx_scroll_edge();

	uint8 s = DATA_sprite0_armadillo;

	switch(dgd(ARMADILLO_MODE))
	{
	case 0: // walk
		if (dgd(ARMADILLO_ROLL) < ARMADILLO_ROLL_TIME)
		{
			s = DATA_sprite0_armadillo_roll;
		}
		else
		{
			s = DATA_sprite0_armadillo + dgd(ARMADILLO_SHOW);
		}
		break;
	case 1: // turn
		s = ARMADILLO_TURN_SPRITE[dgd(ARMADILLO_SHOW)];
		break;
	case 2: // roll
		s = DATA_sprite0_armadillo_ball0 + ((dgx >> 3) & 1);
		break;
	default:
		NES_ASSERT(false,"unknown armadillo mode");
		break;
	}

	if (dgd(ARMADILLO_FLIP) == 0) sprite0_add(        dx,dgy,s);
	else                          sprite0_add_flipped(dx,dgy,s);
}

// DOG_BEETLE

const int BEETLE_ANIM = 0;
const int BEETLE_MODE = 1; // 0 hide, 1 wake, 2 walk left, 3 walk right, 4 fall
const int BEETLE_PASS = 2;

const int BEETLE_RANGE = 16;

void dog_beetle_block(uint8 d) /// update blocker
{
	if (dgt == DOG_SKEETLE) return;
	dog_blocker(d,-7,-12,6,-1);
}
bool dog_beetle_stop_left(uint8 d)
{
	if (dgx < 9) return true;
	if (!collide_all(dgx-8,dgy)) return true; // no ground
	if (collide_all(dgx-8,dgy-8)) return true; // wall
	return false;
}
bool dog_beetle_stop_right(uint8 d)
{
	if (zp.room_scrolling  && dgx >= (511-9)) return true;
	if (!zp.room_scrolling && dgx >= (255-9)) return true;
	if (!collide_all(dgx+7,dgy)) return true; // no ground
	if (collide_all(dgx+7,dgy-8)) return true; // wall
	return false;
}

void dog_tick_beetle_mid(uint8 d); // forward
void dog_init_beetle(uint8 d)
{
	dog_beetle_block(d);

	// mode 0 (hide) start with random time, advance 8 frames to skip "duck"
	dgd(BEETLE_ANIM) = 8;
	dgd(BEETLE_PASS) = (prng() & 127) + 10;
}
void dog_tick_beetle(uint8 d)
{
	if (bound_touch_death(d,-7,-12,6,-1))
	{
		empty_blocker(d);
		bones_convert(d,DATA_sprite0_beetle_skull,0);
		return;
	}

	if (bound_overlap(d,-BEETLE_RANGE,-(BEETLE_RANGE+8),BEETLE_RANGE,BEETLE_RANGE-8))
	{
		if (dgd(BEETLE_MODE) != 0)
		{
			dgd(BEETLE_ANIM) = 0;
			dgd(BEETLE_MODE) = 0; // trigger hide
		}
		else if (dgd(BEETLE_ANIM) >= 7)
		{
			dgd(BEETLE_ANIM) = 7;
		}

		uint8 result = do_push(d,-7,-12,6,-1);
		if (result == 1)
		{
			++dgx;
			dog_beetle_block(d);
		}
		else if (result == 2)
		{
			--dgx;
			dog_beetle_block(d);
		}
	}

	if (do_fall(d,-7,-12,6,-1))
	{
		dgd(BEETLE_MODE) = 4;
		++dgy;
		if (dgy >= 255) // fell off screen
		{
			empty_blocker(d);
			empty_dog(d);
			return;
		}
		else if (dgy == zp.water)
		{
			play_sound_scroll(SOUND_SPLASH_SMALL);
			if (dog_type(SPLASHER_SLOT) == DOG_SPLASHER)
			{
				dog_x(SPLASHER_SLOT) = dgx - 4;
				dog_y(SPLASHER_SLOT) = zp.water - 9;
				dog_data(0,SPLASHER_SLOT) = 0; // trigger splash animation
			}
		}

		dog_beetle_block(d);
	}
	else if(dgd(BEETLE_MODE) == 4)
	{
		dgd(BEETLE_MODE) = 1; // wake
		dgd(BEETLE_ANIM) = 0;
	}

	dog_tick_beetle_mid(d);
}
void dog_tick_beetle_mid(uint8 d)
{
	switch(dgd(BEETLE_MODE))
	{
	case 0: // hide
		{
			if (dgd(BEETLE_ANIM) == 0)
			{
				dgd(BEETLE_PASS) = (prng() & 127) + 10; // random stop time
				++dgd(BEETLE_ANIM);
			}
			else if (dgd(BEETLE_ANIM) >= 8 && dgd(BEETLE_ANIM) >= dgd(BEETLE_PASS))
			{
				dgd(BEETLE_ANIM) = 0;
				dgd(BEETLE_MODE) = 1;  // wake
			}
			else if (dgd(BEETLE_ANIM) < 255)
			{
				++dgd(BEETLE_ANIM);
			}
		} break;
	case 1: // wake
		{
			if (dgd(BEETLE_ANIM) >= 8)
			{
				if (dog_beetle_stop_right(d))
				{
					dgd(BEETLE_MODE) = 2; // walk left
				}
				else if (dog_beetle_stop_left(d))
				{
					dgd(BEETLE_MODE) = 3; // walk right
				}
				else dgd(BEETLE_MODE) = 2 + (prng() & 1); // random direction

				dgd(BEETLE_ANIM) = prng();
				if (dgd(BEETLE_ANIM) >= 192) dgd(BEETLE_ANIM) &= 191;
				dgd(BEETLE_ANIM) += 25;
			}
			else ++dgd(BEETLE_ANIM);
		} break;
	case 2: // walk_left
		{
			if (dgd(BEETLE_ANIM) <= 0 || dog_beetle_stop_left(d)) // hide
			{
				dgd(BEETLE_ANIM) = 0;
				dgd(BEETLE_MODE) = 0;
			}
			else
			{
				--dgd(BEETLE_ANIM);
				if ((dgd(BEETLE_ANIM) & 3) == 0)
				{
					--dgx;
					dog_beetle_block(d);
				}
			}
		} break;
	case 3: // walk_right
		{
			if (dgd(BEETLE_ANIM) <= 0 || dog_beetle_stop_right(d)) // hide
			{
				dgd(BEETLE_ANIM) = 0;
				dgd(BEETLE_MODE) = 0;
			}
			else
			{
				--dgd(BEETLE_ANIM);
				if ((dgd(BEETLE_ANIM) & 3) == 0)
				{
					++dgx;
					dog_beetle_block(d);
				}
			}
		} break;
	case 4:
		break;
	default:
		NES_ASSERT(false,"invalid beetle mode!");
	}
}
void dog_draw_beetle(uint8 d)
{
	dx_scroll_edge();

	uint8 sprite = DATA_sprite0_beetle;

	switch(dgd(BEETLE_MODE))
	{
	case 0:
	case 1:
		if (dgd(BEETLE_ANIM) < 8) sprite = DATA_sprite0_beetle_hide;
		// else sprite = DATA_sprite0_beetle;
		break;
	case 2:
	case 3:
		sprite = DATA_sprite0_beetle_walk;
		break;
	case 4:
		//sprite = DATA_sprite0_beetle;
		break;
	default:
		NES_ASSERT(false,"invalid beetle mode!");
	}

	if (dgx & 1)
		sprite0_add_flipped(dx,dgy,sprite);
	else
		sprite0_add(        dx,dgy,sprite);
}

// DOG_SKEETLE

void dog_init_skeetle(uint8 d)
{
	dog_init_beetle(d);
}
void dog_tick_skeetle(uint8 d)
{
	if (bound_touch_death(d,-7,-12,6,-1))
	{
		bones_convert(d,DATA_sprite0_beetle_skull,0);
		return;
	}

	if (bound_overlap_no_stone(d,-BEETLE_RANGE,-(BEETLE_RANGE+8),BEETLE_RANGE,BEETLE_RANGE-8))
	{
		if (dgd(BEETLE_MODE) != 0)
		{
			dgd(BEETLE_ANIM) = 0;
			dgd(BEETLE_MODE) = 0; // trigger hide
		}
		else if (dgd(BEETLE_ANIM) >= 7)
		{
			dgd(BEETLE_ANIM) = 7;
		}
	}

	dog_tick_beetle_mid(d);

	if (bound_overlap_no_stone(d,-7,-11,6,-1))
	{
		lizard_die();
	}
}
void dog_draw_skeetle(uint8 d)
{
	dog_draw_beetle(d);
}

// DOG_SEEKER_FISH

const int SEEKER_FISH_TURN = 0;
const int SEEKER_FISH_FLIP = 1;
const int SEEKER_FISH_ANIM = 2;
const int SEEKER_FISH_SWIM = 3;
const int SEEKER_FISH_PASS = 4;
const int SEEKER_FISH_DIR  = 5; // %000SUDLR - S is seek
const int SEEKER_FISH_X1   = 6;
const int SEEKER_FISH_Y1   = 7;
const int SEEKER_FISH_VX0  = 8;
const int SEEKER_FISH_VX1  = 9;
const int SEEKER_FISH_VY0  = 10;
const int SEEKER_FISH_VY1  = 11;

const int SEEKER_FISH_TURN_TIME = 12;
const int SEEKER_FISH_MOVE = 20;
const int SEEKER_FISH_MAX = 250;

void dog_init_seeker_fish(uint8 d)
{
	dgd(SEEKER_FISH_DIR) = prng();
	dgd(SEEKER_FISH_FLIP) = prng() & 1;
}
void dog_tick_seeker_fish(uint8 d)
{
	if (bound_touch_death(d,-6,-11,6,-3))
	{
		bones_convert(d,DATA_sprite0_seeker_fish_skull,dgd(SEEKER_FISH_FLIP));
		return;
	}

	if (dgd(SEEKER_FISH_TURN))
	{
		--(dgd(SEEKER_FISH_TURN));
	}

	if (dgd(SEEKER_FISH_ANIM) == 0) // randomly chose to seek or not
	{
		dgd(SEEKER_FISH_PASS) = (prng() & 31) + SEEKER_FISH_TURN_TIME;
		dgd(SEEKER_FISH_DIR) = prng();
		dgd(SEEKER_FISH_DIR) |= prng() & 0x10; // increase odds of seeking

		if (lizard.dead)
		{
			// lizard dead, go random
			dgd(SEEKER_FISH_DIR) &= 0x0F;
		}
	}
	++dgd(SEEKER_FISH_ANIM);
	if (dgd(SEEKER_FISH_ANIM) >= dgd(SEEKER_FISH_PASS))
	{
		dgd(SEEKER_FISH_ANIM) = 0;
	}

	// update direction if seeking
	if (dgd(SEEKER_FISH_DIR) & 0x10)
	{
		dgd(SEEKER_FISH_DIR) &= 0x10; // clear direction bits
		// reform direction bits
		if      (dgx < lizard_px) dgd(SEEKER_FISH_DIR) |= 0x01;
		else if (dgx > lizard_px) dgd(SEEKER_FISH_DIR) |= 0x02;
		if      (dgy < lizard_py) dgd(SEEKER_FISH_DIR) |= 0x04;
		else if (dgy > lizard_py) dgd(SEEKER_FISH_DIR) |= 0x08;
	}

	// turn if direction changes
	if (dgd(SEEKER_FISH_VX0) & 0x80) // moving left
	{
		if (dgd(SEEKER_FISH_FLIP)) // pointing right
		{
			if (!dgd(SEEKER_FISH_TURN)) dgd(SEEKER_FISH_TURN) = SEEKER_FISH_TURN_TIME;
			dgd(SEEKER_FISH_FLIP) = 0;
		}
	}
	else if (dgd(SEEKER_FISH_VX0) != 0 || dgd(SEEKER_FISH_VX1) != 0) // moving right
	{
		if (!dgd(SEEKER_FISH_FLIP)) // pointing left
		{
			if (!dgd(SEEKER_FISH_TURN)) dgd(SEEKER_FISH_TURN) = SEEKER_FISH_TURN_TIME;
			dgd(SEEKER_FISH_FLIP) = 1;
		}
	}

	++dgd(SEEKER_FISH_SWIM);

	// move

	sint16 vx16 = dgd_get16(SEEKER_FISH_VX0,SEEKER_FISH_VX1);
	sint16 vy16 = dgd_get16(SEEKER_FISH_VY0,SEEKER_FISH_VY1);
	uint32 x24 = dgd_get24x(SEEKER_FISH_X1);
	uint16 y16 = dgd_get16y(SEEKER_FISH_Y1);

	if (dgd(SEEKER_FISH_DIR) & 0x01) { if (vx16 <   SEEKER_FISH_MAX) vx16 += SEEKER_FISH_MOVE; }
	if (dgd(SEEKER_FISH_DIR) & 0x02) { if (vx16 >= -SEEKER_FISH_MAX) vx16 -= SEEKER_FISH_MOVE; }
	if (dgd(SEEKER_FISH_DIR) & 0x04) { if (vy16 <   SEEKER_FISH_MAX) vy16 += SEEKER_FISH_MOVE; }
	if (dgd(SEEKER_FISH_DIR) & 0x08) { if (vy16 >= -SEEKER_FISH_MAX) vy16 -= SEEKER_FISH_MOVE; }

	x24 += vx16;
	y16 += vy16;

	sint8 dx = (x24>>8) - dgx;
	sint8 dy = (y16>>8) - dgy;

	move_dog(d,-6,-11,6,-3,&dx,&dy);
	
	x24 = ((dgx+dx) << 8) | (x24 & 0xFF);
	y16 = ((dgy+dy) << 8) | (y16 & 0xFF);
	dgd_put24x(x24,SEEKER_FISH_X1);
	dgd_put16y(y16,SEEKER_FISH_Y1);
	dgd_put16(vx16,SEEKER_FISH_VX0,SEEKER_FISH_VX1);
	dgd_put16(vy16,SEEKER_FISH_VY0,SEEKER_FISH_VY1);

	if (dgy < (zp.water + 17)) dgy = (zp.water + 17);
	if (dgx < 9) ++dgx;
	if (dgx >= 504) -- dgx;

	if (bound_overlap_no_stone(d,-6,-11,6,-3))
	{
		lizard_die();
	}
}
void dog_draw_seeker_fish(uint8 d)
{
	dx_scroll_edge();

	uint8 flip = dgd(SEEKER_FISH_FLIP);
	if (dgd(SEEKER_FISH_TURN) >= (SEEKER_FISH_TURN_TIME/2)) flip ^= 1;

	const uint8 ST[4] = { DATA_sprite0_seeker_fish, DATA_sprite0_seeker_fish_walk0, DATA_sprite0_seeker_fish, DATA_sprite0_seeker_fish_walk1 };
	uint8 sprite = ST[(dgd(SEEKER_FISH_SWIM) >> 3) & 3];

	if (dgd(SEEKER_FISH_TURN)) sprite = DATA_sprite0_seeker_fish_turn;

	if (flip) sprite0_add_flipped(dx,dgy,sprite);
	else      sprite0_add(        dx,dgy,sprite);
}

// DOG_MANOWAR

const int MANOWAR_ANIM = 0;
const int MANOWAR_FLIP = 1;
const int MANOWAR_MODE = 2; // 0 = float, 1 = turn, 2 = puff, 
const int MANOWAR_PASS = 3;
const int MANOWAR_X1   = 4;
const int MANOWAR_Y1   = 5;
const int MANOWAR_VX0  = 6;
const int MANOWAR_VX1  = 7;
const int MANOWAR_VY0  = 8;
const int MANOWAR_VY1  = 9;

const sint16 MANOWAR_DRAG       = 8;
const sint16 MANOWAR_PUFF_RIGHT = 320;
const sint16 MANOWAR_PUFF_LEFT  = -MANOWAR_PUFF_RIGHT;
const sint16 MANOWAR_PUFF_UP    = -350;
const sint16 MANOWAR_GRAVITY    = 13;
const sint16 MANOWAR_MAX_DOWN   = 90;

void dog_init_manowar(uint8 d)
{
	dgd(MANOWAR_FLIP) = prng() & 1;
}
void dog_tick_manowar(uint8 d)
{
	if (bound_touch_death(d,-6,-14,5,-8))
	{
		bones_convert(d,DATA_sprite0_manowar_skull,dgd(MANOWAR_FLIP));
		return;
	}

	uint32 x24 = dgd_get24x(MANOWAR_X1);
	uint16 y16 = dgd_get16y(MANOWAR_Y1);
	sint16 vx16 = dgd_get16(MANOWAR_VX0,MANOWAR_VX1);
	sint16 vy16 = dgd_get16(MANOWAR_VY0,MANOWAR_VY1);

	switch(dgd(MANOWAR_MODE))
	{
	case 0:
		{
			if (dgd(MANOWAR_ANIM) == 0)
			{
				dgd(MANOWAR_PASS) = prng() & 63;
			}
			++dgd(MANOWAR_ANIM);
			if (dgd(MANOWAR_ANIM) > 8 && dgd(MANOWAR_ANIM) >= dgd(MANOWAR_PASS))
			{
				dgd(MANOWAR_ANIM) = 0;
				if ((prng() & 1) == 0)
				{
					// 50% chance to seek lizard vertically (turn = down, puff = up)
					uint8 m = 0;
					if (dgy >= lizard_py) m = 1;
					dgd(MANOWAR_MODE) = (m & 1) + 1;
				}
				else
				{
					// 50% chance to seek lizard horizontally (turn toward, puff toward)
					uint8 m = 0;
					if (lizard_px >= dgx) m = 1;
					m ^= dgd(MANOWAR_FLIP);
					dgd(MANOWAR_MODE) = (m & 1) + 1;
				}
			}
		} break;
	case 1:
		{
			++dgd(MANOWAR_ANIM);
			if (dgd(MANOWAR_ANIM) > 12)
			{
				// 50% chance to puff
				dgd(MANOWAR_MODE) = prng() & 2;
				dgd(MANOWAR_FLIP) ^= 1;
				dgd(MANOWAR_ANIM) = 0;
			}
		} break;
	case 2:
		{
			if (dgd(MANOWAR_ANIM) == 0)
			{
				if (dgd(MANOWAR_FLIP)) vx16 = (MANOWAR_PUFF_LEFT  + (prng() & 31));
				else                   vx16 = (MANOWAR_PUFF_RIGHT - (prng() & 31));
				vy16 = MANOWAR_PUFF_UP;
			}
			++dgd(MANOWAR_ANIM);
			if (dgd(MANOWAR_ANIM) > 32)
			{
				dgd(MANOWAR_MODE) = 0;
				dgd(MANOWAR_ANIM) = 0;
			}
		} break;
	default:
		NES_ASSERT(false,"invalid manowar mode!");
	}

	x24 += vx16;
	y16 += vy16;

	vy16 += MANOWAR_GRAVITY;
	if (vy16 > MANOWAR_MAX_DOWN) vy16 = MANOWAR_MAX_DOWN;

	if (vx16 > 0)
	{
		vx16 -= MANOWAR_DRAG;
		if (vx16 < 0) vx16 = 0;
	}
	else if (vx16 < 0)
	{
		vx16 += MANOWAR_DRAG;
		if (vx16 >= 0) vx16 = 0;
	}

	uint16 tx = (x24 >> 8) & 0xFFFF;
	uint8 ty = (y16 >> 8) & 0xFF;

	sint8 dx = tx - dgx;
	sint8 dy = ty - dgy;

	move_dog(d,-6,-14,5,-8,&dx,&dy);
	tx = dgx + dx;
	ty = dgy + dy;
	if (tx < 9) tx = 9;
	if (tx >= (511-8)) tx = (511-9);
	if (ty < (zp.water+16)) ty = zp.water+16;

	x24 = (tx << 8) | (x24 & 0xFF);
	y16 = (ty << 8) | (y16 & 0xFF);

	dgd_put24x(x24,MANOWAR_X1);
	dgd_put16y(y16,MANOWAR_Y1);
	dgd_put16(vx16,MANOWAR_VX0,MANOWAR_VX1);
	dgd_put16(vy16,MANOWAR_VY0,MANOWAR_VY1);

	if (bound_overlap_no_stone(d,-6,-14,5,-8))
	{
		lizard_die();
	}
}
void dog_draw_manowar(uint8 d)
{
	dx_scroll_edge();

	uint8 sprite = DATA_sprite0_manowar;
	if (dgd(MANOWAR_MODE) == 1) sprite = DATA_sprite0_manowar_turn;
	else if (dgd(MANOWAR_MODE) == 2)
	{
		if (dgd(MANOWAR_ANIM) & 8) sprite = DATA_sprite0_manowar_puff1;
		else                       sprite = DATA_sprite0_manowar_puff0;
	}

	if (dgd(MANOWAR_FLIP)) sprite0_add_flipped(dx,dgy,sprite);
	else                   sprite0_add(        dx,dgy,sprite);
}

// DOG_SNAIL

const int SNAIL_ANIM = 0;
const int SNAIL_FLIP = 1;
const int SNAIL_START = 2; // true if just rotated

// SNAIL_FLIP:
//   %00000abc : a=tile, b=v, c=h
//   0 - floor right
//   1 - floor left
//   2 - ceiling right
//   3 - ceiling left
//   4 - r-wall up
//   5 - l-wall up
//   6 - r-wall down
//   7 - l-wall down

// 4 points to test for the floor
//   if none work, snail should fall
const sint8 SNAIL_FLOOR_X[4] = { -3,  3, -3,  3 };
const sint8 SNAIL_FLOOR_Y[4] = { -3, -3,  3,  3 };

// point to test if you should rotate
const sint8 SNAIL_TRAIL_X[8] = { -2,  2, -2,  2,  3, -3,  3, -3 };
const sint8 SNAIL_TRAIL_Y[8] = {  3,  3, -3, -3,  2,  2, -2, -2 };
const uint8 SNAIL_ROTATE[8] = { 7, 6, 5, 4, 0, 1, 2, 3 };

// wall test, if collide is found at wall test, follow the flip transition
const sint8 SNAIL_WALL_X[8] = {  3, -3,  3, -3,  0,  0,  0,  0 };
const sint8 SNAIL_WALL_Y[8] = {  0,  0,  0,  0, -3, -3,  3,  3 };
const sint8 SNAIL_MOVE_X[8] = {  1, -1,  1, -1,  0,  0,  0,  0 };
const sint8 SNAIL_MOVE_Y[8] = {  0,  0,  0,  0, -1, -1,  1,  1 };
const uint8 SNAIL_FLIP_WALL[8] = { 4, 5, 6, 7, 3, 2, 1, 0 };

// snail drawing
const uint8 SNAIL_SPRITE[8] = { 0xA6, 0xA6, 0xA6, 0xA6, 0xB6, 0xB6, 0xB6, 0xB6 };
const sint8 SNAIL_DX[8] = { -3, -4, -3, -4, -5, -2, -5, -2 };
const sint8 SNAIL_DY[8] = { -6, -6, -3, -3, -5, -5, -4, -4 };

void dog_snail_blocker(uint8 d)
{
	// increase vertical height to 6 pixels to prevent snail from slipping into lizard's gap
	if (((dgd(SNAIL_FLIP)) & 0x6) == 0x2) dog_blocker(d,-2,-4,2,2);
	else                                  dog_blocker(d,-2,-2,2,4);
}

void dog_init_snail(uint8 d)
{
	dgd(SNAIL_ANIM) = prng();
	dgd(SNAIL_FLIP) = prng() & 1; // floor, random H flip
	dog_snail_blocker(d);
}
void dog_tick_snail(uint8 d)
{
	if (bound_touch_death(d,-2,-2,2,2))
	{
		empty_blocker(d);
		bones_convert(d,DATA_sprite0_snail_skull,dgd(SNAIL_FLIP) & 1);
		return;
	}

	uint8 push = do_push(d,-2,-2,2,2);
	if (push != 0)
	{
		if (push == 1)
		{
			++dgx;
			if (dgx >= 509) { empty_dog(d); return; }
			dgd(SNAIL_FLIP) = 0;
			dgd(SNAIL_START) = 1;
			dog_snail_blocker(d);
			return;
		}
		else if (push == 2)
		{
			--dgx;
			if (dgx < 5) { empty_dog(d); return; }
			dgd(SNAIL_FLIP) = 1;
			dgd(SNAIL_START) = 1;
			dog_snail_blocker(d);
			return;
		}
	}

	empty_blocker(d);
	bool test = false;
	for (int i=0; i<4; ++i)
	{
		// was originally collide all, but this allowed a pair of snails to climb eachother into the sky
		// collide_tile only here causes blockers to be unclimbable (this is the test for "floor" to stick to)
		if (collide_tile(dgx+SNAIL_FLOOR_X[i],dgy+SNAIL_FLOOR_Y[i]))
		{
			test = true;
			break;
		}
	}
	if (!test)
	{
		if (bound_overlap(d,-2,3,2,3))
		{
			dog_snail_blocker(d);
			return; // don't fall on lizard's head
		}
		dgd(SNAIL_FLIP) &= 1; // orient to floor
		++dgy;
		if (dgy >= 237) { empty_dog(d); return; }
		dog_snail_blocker(d);
		return;
	}


	uint8 r = dgd(SNAIL_FLIP) & 7;

	if (!collide_all(dgx+SNAIL_TRAIL_X[r],dgy+SNAIL_TRAIL_Y[r]))
	{
		if (!dgd(SNAIL_START))
		{
			dgd(SNAIL_FLIP) = SNAIL_ROTATE[r];
			r = dgd(SNAIL_FLIP) & 7;
			dgd(SNAIL_START) = 1; // just rotated, don't rotate again
		}
	}
	else
	{
		dgd(SNAIL_START) = 0; // successfully on new block, ready to rotate again
	}

	++dgd(SNAIL_ANIM);
	if ((dgd(SNAIL_ANIM) & 63) != 0)
	{
		dog_snail_blocker(d);
		return;
	}

	if (bound_overlap(d,-3,-5,3,5))
	{
		// don't move if near lizard
		dog_snail_blocker(d);
		return;
	}

	// move
	if (collide_all(dgx+SNAIL_WALL_X[r],dgy+SNAIL_WALL_Y[r]))
	{
		dgd(SNAIL_FLIP) = SNAIL_FLIP_WALL[r];
	}
	else
	{
		dgx += SNAIL_MOVE_X[r];
		dgy += SNAIL_MOVE_Y[r];

		if (
			(dgx >= 509) ||
			(dgx <  5  ) ||
			(dgy >= 237) ||
			(dgy <  5  ) )
		{ 
			empty_dog(d);
			return;
		}
	}
	dog_snail_blocker(d);
}
void dog_draw_snail(uint8 d)
{
	uint8 r = dgd(SNAIL_FLIP) & 7;
	dx_scroll_offset(SNAIL_DX[r]);

	uint8 sprite = SNAIL_SPRITE[r];
	uint8 y = dgy + SNAIL_DY[r];
	if (dgd(SNAIL_ANIM) & 32) ++sprite;

	uint8 attrib = ((dgd(SNAIL_FLIP) & 3) << 6) | 3;

	sprite_tile_add(dx,y,sprite,attrib);
}

// DOG_SNAPPER

const int SNAPPER_ANIM  = 0;
const int SNAPPER_MODE  = 1; // 0 = left, 1 = right, 2 = fly
const int SNAPPER_VY0   = 2;
const int SNAPPER_VY1   = 3;
const int SNAPPER_Y1    = 4;
const int SNAPPER_FLOOR = 5;
const int SNAPPER_WAIT  = 6;

sint16 SNAPPER_JUMP  = -860;
sint16 SNAPPER_GRAV  = 30;
sint16 SNAPPER_MAX   = -SNAPPER_JUMP;
uint8  SNAPPER_PAUSE = 32;

void dog_init_snapper(uint8 d)
{
	dgd(SNAPPER_MODE) = prng() & 1; // random flip
	dgd(SNAPPER_FLOOR) = dgy;
}
void dog_tick_snapper(uint8 d)
{
	++dgd(SNAPPER_ANIM);

	if (bound_touch(d,-7,-15,6,-4))
	{
		if (zp.current_lizard == LIZARD_OF_DEATH)
		{
			bones_convert(d,DATA_sprite0_snapper_skull,0);
			return;
		}
		else if (zp.current_lizard == LIZARD_OF_HEAT)
		{
			dgd(SNAPPER_WAIT) = 0;
		}
	}

	if (dgd(SNAPPER_WAIT))
	{
		--dgd(SNAPPER_WAIT);
	}
	else
	{
		if (dgd(SNAPPER_MODE) != 2)
		{
			if (bound_overlap_no_stone(d,-20,-72,19,0))
			{
				if (((prng()&3) == 0) ||
					bound_overlap_no_stone(d,-8,-72,7,0))
				{
					play_sound(SOUND_SNAPPER_JUMP);
					dgd(SNAPPER_MODE) = 2;
					dgd_put16(SNAPPER_JUMP,SNAPPER_VY0,SNAPPER_VY1);
					dgd(SNAPPER_Y1) = 0;
				}
			}
		}
	}

	switch (dgd(SNAPPER_MODE))
	{
	case 0:
		{
			if (collide_tile(dgx-8,dgy-6) ||
			   !collide_tile(dgx-7,dgy  ))
			{
				dgd(SNAPPER_MODE) = 1;
			}
			else
			{
				--dgx;
			}
		} break;
	case 1:
		{
			if (collide_tile(dgx+7,dgy-6) ||
			   !collide_tile(dgx+6,dgy  ))
			{
				dgd(SNAPPER_MODE) = 0;
			}
			else
			{
				++dgx;
			}
		} break;
	case 2:
		{
			sint16 y16 = dgd_get16y(SNAPPER_Y1);
			sint16 vy16 = dgd_get16(SNAPPER_VY0,SNAPPER_VY1);

			y16 += vy16;
			dgd_put16y(y16,SNAPPER_Y1);

			vy16 += SNAPPER_GRAV;
			if (vy16 >= SNAPPER_MAX) vy16 = SNAPPER_MAX;
			dgd_put16(vy16,SNAPPER_VY0,SNAPPER_VY1);

			if (dgy > dgd(SNAPPER_FLOOR))
			{
				dgy = dgd(SNAPPER_FLOOR);
				dog_init_snapper(d); // back to left or right move
				dgd(SNAPPER_WAIT) = SNAPPER_PAUSE + (prng() & 15);
			}
		} break;
	default:
		NES_ASSERT(false,"Invalid snapper mode!");
	}

	if (bound_overlap_no_stone(d,-7,-15,6,-4))
	{
		lizard_die();
	}
}
void dog_draw_snapper(uint8 d)
{
	dx_scroll_edge();

	uint8 s = DATA_sprite0_snapper;
	if ((dgd(SNAPPER_MODE) == 2) && (dgd(SNAPPER_VY0) & 0x80)) s = DATA_sprite0_snapper_jump;

	if ((dgd(SNAPPER_ANIM) >> 2) & 1) sprite0_add_flipped(dx,dgy,s);
	else                              sprite0_add(        dx,dgy,s);
}

// DOG_VOIDBALL

const int VOIDBALL_DIR = 0;

const sint8 VOIDBALL_DIR_X[4] = {  1, -1,  1, -1 };
const sint8 VOIDBALL_DIR_Y[4] = {  1,  1, -1, -1 };

void dog_init_voidball(uint8 d)
{
	NES_ASSERT(zp.room_scrolling == 0, "DOG_VOIDBALL does not support scrolling!");
	dgd(VOIDBALL_DIR) = prng() & 3;
}
void dog_tick_voidball(uint8 d)
{
	NES_ASSERT(dgd(VOIDBALL_DIR)<4,"VOIDBALL_DIR corrupt!");
	sint8 dx = VOIDBALL_DIR_X[dgd(VOIDBALL_DIR)];
	sint8 dy = VOIDBALL_DIR_Y[dgd(VOIDBALL_DIR)];

	// break cycles with randomness when not in view
	if (dgy >= 243)
	{
		dx += (prng(2) & 7) - 3;
	}

	uint8 reflect = move_dog(d,-3,-3,2,2,&dx,&dy);
	if ((reflect &  3) &&
		(dgy >= 3) && // filter out wrap-average errors in move_dog
		(dgy < 254))
	{
		dgd(VOIDBALL_DIR) ^= 1; // horizontal bounce
	}
	if ((reflect & 12) &&
		((dgx & 255) >= 3) && // filter out wrap-average errors in move_dog
		((dgx & 255) < 254))
	{
		dgd(VOIDBALL_DIR) ^= 2; // vertical bounce
	}
	dgx += dx;
	dgy += dy;

	if (bound_overlap_no_stone(d,-3,-3,2,2))
	{
		lizard_die();
	}
}
void dog_draw_voidball(uint8 d)
{
	sprite_tile_add(dgx-4,dgy-5,0x60,0x03);
}

// DOG_BALLSNAKE

const int BALLSNAKE_DIR0 = 0; // 0,1,2,3 = L U R D
const int BALLSNAKE_DIR1 = 1;
const int BALLSNAKE_DIR2 = 2;
const int BALLSNAKE_DIR3 = 3;
const int BALLSNAKE_DIR4 = 4;
const int BALLSNAKE_DIR5 = 5;
const int BALLSNAKE_DIR6 = 6;
const int BALLSNAKE_DIR7 = 7;
const int BALLSNAKE_ANIM = 8;

const sint8 BALLSNAKE_DIR_X[4] =  {   -1,    0,    1,    0 };
const sint8 BALLSNAKE_DIR_Y[4] =  {    0,   -1,    0,    1 };
const uint8 BALLSNAKE_ATTRIB[4] = { 0x02, 0x02, 0x42, 0x82 };
const sint8 BALLSNAKE_TRY[4] =    {    0,   -1,    1,    2 }; // directions to try if blocked
const sint8 BALLSNAKE_TRY_X[4] =  {   -1,    4,    8,    4 };
const sint8 BALLSNAKE_TRY_Y[4] =  {    4,   -1,    4,    8 };
const uint8 BALLSNAKE_DRAW[4]  =  {    1,    7,    3,    5 }; // randomized draw order

bool dog_ballsnake_try_dir(uint8 td, uint8 d)
{
	td &= 3;
	uint8 tx = uint8(dgx + BALLSNAKE_TRY_X[td]);
	uint8 ty = dgy + BALLSNAKE_TRY_Y[td];

	if (collide_tile(tx,ty))
	{
		return false;
	}

	dgd(BALLSNAKE_DIR0) = td;
	return true;
}
void dog_ballsnake_build(uint8 d)
{
	uint8 tx = uint8(dgx);
	uint8 ty = dgy;
	uint8 dir = dog_data(BALLSNAKE_DIR0,d);

	uint8 move_b = dgd(BALLSNAKE_ANIM);
	uint8 move_a = 8 - move_b;

	int i = 0;

	do
	{

		stack.scratch[0 +i] = tx;
		stack.scratch[8 +i] = ty;
		stack.scratch[16+i] = BALLSNAKE_ATTRIB[dir];
		stack.scratch[24+i] = 0x40 | (dir & 1) | ((i == 0) ? 0x00 : 0x10); // 0x40->0x50 for body

		// move_a
		switch (dir) // L U R D
		{
		case 0: tx += move_a; break;
		case 1: ty += move_a; break;
		case 2: tx -= move_a; break;
		case 3: ty -= move_a; break;
		default: NES_ASSERT(false,"Invalid BALLSNAKE_DIR!");
		}

		++i;
		if (i >= 8) break;
		dir = dog_data(BALLSNAKE_DIR0+i,d);

		// move_b
		switch (dir)
		{
		case 0: tx += move_b; break;
		case 1: ty += move_b; break;
		case 2: tx -= move_b; break;
		case 3: ty -= move_b; break;
		default: NES_ASSERT(false,"Invalid BALLSNAKE_DIR!");
		}

	} while (true);
}
void dog_init_ballsnake(uint8 d)
{
	NES_ASSERT(zp.room_scrolling == 0, "DOG_BALLSNAKE does not support scrolling!");
	NES_ASSERT((dgx & 7) == 0, "ballsnake not X grid aligned!");
	NES_ASSERT((dgy & 7) == 0, "ballsnake not Y grid aligned!");
}
void dog_tick_ballsnake(uint8 d)
{
	if (dgd(BALLSNAKE_ANIM) == 0)
	{
		// propagate directions
		for (int i=7; i>0; --i)
		{
			dog_data(BALLSNAKE_DIR0+i,d) = dog_data(BALLSNAKE_DIR0+i-1,d);
		}

		uint8 td = dgd(BALLSNAKE_DIR0);
		uint8 r = prng(2) & 15;
		if      (r == 0) --td;
		else if (r == 1) ++td;
		
		// choose a new direction, making sure not to collide
		if (!dog_ballsnake_try_dir(td,d))
		{
			for (int i=0; i<4; ++i)
			{
				td = dgd(BALLSNAKE_DIR0) + BALLSNAKE_TRY[i];
				if (dog_ballsnake_try_dir(td,d)) break;
			}
			// if no good direction was found it will continue on straight
		}
		dgd(BALLSNAKE_ANIM) = 8;
	}
	--dgd(BALLSNAKE_ANIM);

	dgx = dgx + BALLSNAKE_DIR_X[dgd(BALLSNAKE_DIR0)];
	dgy += BALLSNAKE_DIR_Y[dgd(BALLSNAKE_DIR0)];
	dgx &= 255;

	dog_ballsnake_build(d);
	for (int i=0; i<8; ++i)
	{
		uint8 tx = stack.scratch[0+i];
		uint8 ty = stack.scratch[8+i];

		if (lizard_overlap_no_stone(tx+1,ty+1,tx+6,ty+6))
		{
			lizard_die();
			return;
		}
	}
}
void dog_draw_ballsnake(uint8 d)
{
	// $40,41 = head left, up
	// $50,51 = body left, up
	// tile |= (dir & 1)
	// attrib = lookup

	dog_ballsnake_build(d);

	uint8 i = 0;
	uint8 i_add = BALLSNAKE_DRAW[dgd(BALLSNAKE_ANIM) & 3]; // changing draw order

	do
	{
		uint8 tx =     stack.scratch[0 +i];
		uint8 ty =     stack.scratch[8 +i] - 1;
		uint8 attrib = stack.scratch[16+i];
		uint8 tile =   stack.scratch[24+i];

		sprite_tile_add(tx,ty,tile,attrib);

		i = (i + i_add) & 7;
	} while (i != 0);
}

// DOG_MEDUSA

const int MEDUSA_DIVE  = 0;
const int MEDUSA_FLIP  = 1;
const int MEDUSA_SPAWN = 2;
const int MEDUSA_TIME  = 3;
const int MEDUSA_VY0   = 4;
const int MEDUSA_VY1   = 5;
const int MEDUSA_Y1    = 6;
//const int FIRE_PAL = 10;
//const int FIRE_ANIM = 11;

const uint8 MEDUSA_FLAP_CYCLE[4] = {
	DATA_sprite0_medusa0,
	DATA_sprite0_medusa1,
	DATA_sprite0_medusa2,
	DATA_sprite0_medusa1,
};

const sint16 MEDUSA_MIN   = -300;
const sint16 MEDUSA_MAX   = 400;
const sint16 MEDUSA_ACCEL = 10;

void dog_spawn_medusa(uint8 d)
{
	dgd(MEDUSA_DIVE) = 1;

	if (!(lizard_px & 0x80)) // always spawn at edge away from player, if scrolling milddle 256 range flips
	{
		dgd(MEDUSA_FLIP) = 0;
		dgx = 247;
		if (zp.room_scrolling) dgx += 256;
	}
	else
	{
		dgd(MEDUSA_FLIP) = 1;
		dgx = 8;
	}

	dgy = prng(4);
	while (dgy > dgp)
	{
		dgy >>= 1;
	}

	dgd_put16(MEDUSA_MAX,MEDUSA_VY0,MEDUSA_VY1);
	dgd(MEDUSA_Y1) = 0;
}
void dog_time_medusa(uint8 d)
{
	dgd(MEDUSA_TIME) = (prng(2) & 31) | 16;
}

void dog_init_medusa(uint8 d)
{
	dgd(MEDUSA_SPAWN) = (prng(4) | 1) & 127;

	dog_time_medusa(d);
}
void dog_tick_medusa(uint8 d)
{
	// spawn timer
	if (dgd(MEDUSA_SPAWN))
	{
		--dgd(MEDUSA_SPAWN);
		if (dgd(MEDUSA_SPAWN) != 0) return;
		dog_spawn_medusa(d);
	}

	// touch
	if (bound_touch_death(d,-8,-4,7,-1))
	{
		bones_convert(d,DATA_sprite0_medusa_skull,dgd(MEDUSA_FLIP));
		return;
	}

	// horizontal motion
	if (dgd(MEDUSA_FLIP) == 0)
	{
		--dgx;
		if (dgx < 8)
		{
			dog_init_medusa(d);
			return;
		}
	}
	else
	{
		++dgx;
		if ((!zp.room_scrolling && dgx > 247) ||
			(zp.room_scrolling && dgx > 503))
		{
			dog_init_medusa(d);
			return;
		}
	}

	// flap sound on begin of downward wings sprite (sprite 0)
	if ((dgd(MEDUSA_VY0) & 0x80) && ((dgx & 15) == 0))
	{
		play_sound_scroll(SOUND_OWL_FLAP);
	}

	// dive control
	if (dgy >= dgp)
	{
		dgd(MEDUSA_DIVE) = 0;
		dog_time_medusa(d);
	}
	else if (dgy < 32)
	{
		dgd(MEDUSA_DIVE) = 1;
		dog_time_medusa(d);
	}
	else if (dgd(MEDUSA_TIME) == 0)
	{
		dgd(MEDUSA_DIVE) = prng() & 1;
		dog_time_medusa(d);
	}
	else
	{
		--dgd(MEDUSA_TIME);
	}

	// vertical motion

	sint16 vy16 = dgd_get16(MEDUSA_VY0,MEDUSA_VY1);
	uint16 y16 = dgd_get16y(MEDUSA_Y1);

	if (dgd(MEDUSA_DIVE) == 0)
	{
		vy16 -= MEDUSA_ACCEL;
		if (vy16 < MEDUSA_MIN)
			vy16 = MEDUSA_MIN;
	}
	else
	{
		vy16 += MEDUSA_ACCEL;
		if (vy16 >= MEDUSA_MAX)
			vy16 = MEDUSA_MAX;
	}

	dgd_put16(vy16,MEDUSA_VY0,MEDUSA_VY1);
	y16 += vy16;
	dgd_put16y(y16,MEDUSA_Y1);

	// overlap
	if (bound_overlap_no_stone(d,-8,-4,7,-1))
	{
		lizard_die();
	}
}
void dog_draw_medusa(uint8 d)
{
	if (dgd(MEDUSA_SPAWN) > 0) return;

	dx_scroll_edge();

	uint8 s = DATA_sprite0_medusa;
	if (dgd(MEDUSA_VY0) & 0x80)
	{
		s = MEDUSA_FLAP_CYCLE[(dgx>>2) & 3];
	}

	if (dgd(MEDUSA_FLIP) == 0) sprite0_add(        dx,dgy,s);
	else                       sprite0_add_flipped(dx,dgy,s);
}

// DOG_PENGUIN

const int PENGUIN_MODE = 0;
const int PENGUIN_ANIM = 1;
const int PENGUIN_FLIP = 2;

const uint8 PENGUIN_MODE_TURN = 0;
const uint8 PENGUIN_MODE_WALK = 1;
const uint8 PENGUIN_MODE_FEAR = 2;
const uint8 PENGUIN_MODE_HATE = 3;

const uint8 PENGUIN_WALK_SPRITE[4] = {
	DATA_sprite0_penguin_walk2,
	DATA_sprite0_penguin_walk1,
	DATA_sprite0_penguin_walk0,
	DATA_sprite0_penguin_walk1,
};

CT_ASSERT(DATA_sprite0_penguin_fear1 == (DATA_sprite0_penguin_fear0 + 1), "penguin sprites out of order!");
CT_ASSERT(DATA_sprite0_penguin_hate1 == (DATA_sprite0_penguin_hate0 + 1), "penguin sprites out of order!");

void dog_penguin_turn(uint8 d)
{
	dgd(PENGUIN_ANIM) = 16 + (prng() & 15);
	dgd(PENGUIN_MODE) = PENGUIN_MODE_TURN;
}

void dog_penguin_fear(uint8 d)
{
	if (dgd(PENGUIN_MODE) == PENGUIN_MODE_FEAR)
	{
		return;
	}

	if (dgd(PENGUIN_MODE) == PENGUIN_MODE_HATE)
	{
		dgd(PENGUIN_ANIM) &= 15; // reset hate timeout
		return;
	}

	// otherwise switch mode
	dgd(PENGUIN_ANIM) = 0;
	dgd(PENGUIN_MODE) = PENGUIN_MODE_FEAR;
	play_sound_scroll(SOUND_PENGUIN_FEAR);
}

void dog_penguin_blocker(uint8 d)
{
	dog_blocker(d,-6,-14,5,-1);
}

void dog_init_penguin(uint8 d)
{
	dgd(PENGUIN_FLIP) = prng() & 1;
	dog_penguin_turn(d);
	dog_penguin_blocker(d);
}

void dog_tick_penguin(uint8 d)
{
	if (bound_touch(d,-6,-14,5,-1))
	{
		if (zp.current_lizard == LIZARD_OF_DEATH)
		{
			empty_blocker(d);
			bones_convert(d,DATA_sprite0_penguin_skull,dgd(PENGUIN_FLIP));
			return;
		}
		else if (zp.current_lizard == LIZARD_OF_HEAT)
		{
			dog_penguin_fear(d);
		}
	}

	uint8 result = do_push(d,-6,-14,5,-1);
	if (result == 1) // push right
	{
		dgx += 1;
		dog_penguin_fear(d);
		dog_penguin_blocker(d);
	}
	else if (result == 2) // push left
	{
		dgx -= 1;
		dog_penguin_fear(d);
		dog_penguin_blocker(d);
	}

	if (do_fall(d,-6,-14,5,-1))
	{
		if (dgy >= 255)
		{
			empty_blocker(d);
			empty_dog(d);
			return;
		}

		dgy += 1;
		dog_penguin_fear(d);
		dog_penguin_blocker(d);
	}

	// stand on penguin
	if (bound_overlap(d,-6,-15,5,-15))
	{
		dog_penguin_fear(d);
	}

	switch (dgd(PENGUIN_MODE))
	{
	case PENGUIN_MODE_TURN:
		if (dgd(PENGUIN_ANIM) == 0)
		{
			dgd(PENGUIN_MODE) = PENGUIN_MODE_WALK;
			dgd(PENGUIN_FLIP) ^= 1;
		}
		else
		{
			--dgd(PENGUIN_ANIM);
			return;
		}
		// fall through
	case PENGUIN_MODE_WALK:
		{
			if ((dgd(PENGUIN_ANIM) & 3) == 0)
			{
				if (!dgd(PENGUIN_FLIP))
				{
					if (bound_overlap(d,-7,-14,5,-1) ||
						!collide_all(dgx-7,dgy) ||
						collide_all(dgx-7,dgy-1) ||
						collide_all(dgx-7,dgy-14) ||
						(dgx < 9)
						)
					{
						dog_penguin_turn(d);
						return;
					}
					--dgx;
				}
				else
				{
					if (bound_overlap(d,-6,-14,6,-1) ||
						!collide_all(dgx+6,dgy) ||
						collide_all(dgx+6,dgy-1) ||
						collide_all(dgx+6,dgy-14) ||
						((!zp.room_scrolling && dgx >= 248) || (dgx >= 504))
						)
					{
						dog_penguin_turn(d);
						return;
					}
					++dgx;
				}
				dog_penguin_blocker(d);
			}
			++dgd(PENGUIN_ANIM);
		}
		return;
	case PENGUIN_MODE_FEAR:
		{
			++dgd(PENGUIN_ANIM);
			if (dgd(PENGUIN_ANIM) >= 60)
			{
				dgd(PENGUIN_ANIM) = 0;
				dgd(PENGUIN_MODE) = PENGUIN_MODE_HATE;
			}
			else
			{
				return;
			}
		}
		// fall through
	case PENGUIN_MODE_HATE:
		{
			if ((dgd(PENGUIN_ANIM) & 15) == 0)
			{
				play_sound_scroll(SOUND_PENGUIN_HATE);
			}

			++dgd(PENGUIN_ANIM);
			if (dgd(PENGUIN_ANIM) >= 47)
			{
				dog_penguin_turn(d);
				return;
			}

			if (bound_overlap_no_stone(d,-3,-19,2,-15) ||
				bound_overlap_no_stone(d,-10,-15,9,-11))
			{
				lizard_die();
				return;
			}
		}
		return;
	default:
		NES_ASSERT(false,"unknown penguin mode!");
		dog_penguin_turn(d);
		return;
	}
}

void dog_draw_penguin(uint8 d)
{
	dx_scroll_edge();

	uint8 s = DATA_sprite0_penguin;
	uint8 flip = dgd(PENGUIN_FLIP);

	switch (dgd(PENGUIN_MODE))
	{
	case PENGUIN_MODE_TURN:
		if (dgd(PENGUIN_ANIM) >= 4 && dgd(PENGUIN_ANIM) < 10)
		{
			s = DATA_sprite0_penguin_blink;
		}
		break;
	case PENGUIN_MODE_WALK:
		s = PENGUIN_WALK_SPRITE[(dgd(PENGUIN_ANIM) >> 3) & 3];
		break;
	case PENGUIN_MODE_FEAR:
		s = DATA_sprite0_penguin_fear0 + ((dgd(PENGUIN_ANIM) >> 3) & 1);
		flip ^= (dgd(PENGUIN_ANIM) >> 4) & 1;
		break;
	case PENGUIN_MODE_HATE:
		s = DATA_sprite0_penguin_hate0 + ((dgd(PENGUIN_ANIM) >> 3) & 1);
		break;
	default:
		NES_ASSERT(false,"unknown penguin mode!");
	}

	if (!flip)         sprite0_add(dx,dgy,s);
	else       sprite0_add_flipped(dx,dgy,s);
}

// DOG_MAGE

const int MAGE_ANIM  = 0;
const int MAGE_BLINK = 1;
const int MAGE_FLIP  = 2;

//const int MAGE_BALL_MODE = 0; // 0 = off, 1 = on stationary, 2 = on chasing, 3 = chase ty 4, chase 128,ty, 5 = on leaving

void dog_init_mage(uint8 d)
{
	NES_ASSERT(d < 14 && dog_type(d+2) == DOG_MAGE_BALL, "mage without mage_ball pair.");
	dgd(MAGE_BLINK) = prng(8);
	dgd(MAGE_FLIP) = prng() & 1;
}

void dog_tick_mage(uint8 d)
{
	// blink
	if (dgd(MAGE_BLINK) == 0)
	{
		dgd(MAGE_BLINK) = prng() | 0x80;
	}
	else
	{
		--dgd(MAGE_BLINK);
	}

	if (dgd(MAGE_ANIM) == 0)
	{
		bool wake = false;

		const int MAGE_WAKE_WIDTH = 100;
		if (lizard_px < dgx)
		{
			if (dgx >= MAGE_WAKE_WIDTH)
			{
				if (lizard_px >= (dgx - MAGE_WAKE_WIDTH)) wake = true;
			}
			else wake = true;
		}
		else
		{
			if ((lizard_px - dgx) < (MAGE_WAKE_WIDTH + 1)) wake = true;
		}

		if (zp.current_lizard == LIZARD_OF_STONE && lizard.power > 0)
		{
			wake = false;
		}

		if (wake)
		{
			dgd(MAGE_ANIM) = 1;
		}
		return;
	}
	else
	{
		++dgd(MAGE_ANIM);

		if (dgd(MAGE_ANIM) >= 32)
		{
			dgd(MAGE_FLIP) = 0;
			if (lizard_px >= dgx) dgd(MAGE_FLIP) = 1;
		}

		if (dgd(MAGE_ANIM) == 31) // create the ball
		{
			play_sound(SOUND_TWINKLE);
			dog_x(d+2) = dgx;
			dog_y(d+2) = dgy - 29;
			dog_data(MAGE_BALL_MODE,d+2) = 1;
		}
		else if (dgd(MAGE_ANIM) >= 32 && dgd(MAGE_ANIM) < 63) // flicker fade-in ball
		{
			uint8 p = dgd(MAGE_ANIM) << 5 | 0x1F;
			if (p >= prng()) dog_data(MAGE_BALL_MODE,d+2) = 1; // show
			else             dog_data(MAGE_BALL_MODE,d+2) = 0; // hide
		}
		else if (dgd(MAGE_ANIM) == 64) // activate ball
		{
			dog_data(MAGE_BALL_MODE,d+2) = 2;
		}

		// loop in casting
		if (dgd(MAGE_ANIM) >= 80)
		{
			dgd(MAGE_ANIM) = 64;
		}

		if (dgd(MAGE_ANIM) >= 24)
		{
			if (bound_overlap_no_stone(d,-4,-15,3,-1))
			{
				lizard_die();
			}

			if (bound_touch(d,-4,-15,3,-1))
			{
				// set mage ball to escape
				if (dog_data(MAGE_BALL_MODE,d+2) != 0)
				{
					dog_data(MAGE_BALL_MODE,d+2) = 5;
				}

				if (zp.current_lizard == LIZARD_OF_DEATH)
				{
					bones_convert(d,DATA_sprite0_mage_skull,dgd(MAGE_FLIP));
					return;
				}
				else if (zp.current_lizard == LIZARD_OF_HEAT)
				{
					dgd(MAGE_BLINK) = 10;
				}
			}
		}
	}
}

const uint8 MAGE_FRAMES[10] = {
	DATA_sprite0_mage_wake0, //  0 -  7
	DATA_sprite0_mage_wake1, //  8 - 15
	DATA_sprite0_mage_wake2, // 16 - 23
	DATA_sprite0_mage_cast0, // 24 - 31
	DATA_sprite0_mage_cast1, // 32 - 39
	DATA_sprite0_mage_cast0, // 40 - 47
	DATA_sprite0_mage_cast1, // 48 - 55
	DATA_sprite0_mage_cast0, // 56 - 63
	DATA_sprite0_mage_cast1, // 64 - 71
	DATA_sprite0_mage_cast0, // 72 - 79
};


void dog_draw_mage(uint8 d)
{
	dx_scroll_edge();

	uint8 frame = dgd(MAGE_ANIM) >> 3;
	NES_ASSERT(frame < 10, "mage ANIM out of range!");
	uint8 sprite = MAGE_FRAMES[frame];

	if (dgd(MAGE_BLINK) < 11)
	{
		if      (sprite == DATA_sprite0_mage_cast0) sprite = DATA_sprite0_mage_blink0;
		else if (sprite == DATA_sprite0_mage_cast1) sprite = DATA_sprite0_mage_blink1;
	}

	if (!dgd(MAGE_FLIP)) sprite0_add(        dx,dgy,sprite);
	else                 sprite0_add_flipped(dx,dgy,sprite);
}

// DOG_MAGE_BALL

//const int MAGE_BALL_MODE = 0; // 0 = off, 1 = on stationary, 2 = on chasing, 3 = chase ty 4, chase 128,ty, 5 = on leaving
const int MAGE_BALL_ANIM  = 1;
const int MAGE_BALL_FRAME = 2;
const int MAGE_BALL_X1    = 3;
const int MAGE_BALL_Y1    = 4;
const int MAGE_BALL_VX0   = 5;
const int MAGE_BALL_VX1   = 6;
const int MAGE_BALL_VY0   = 7;
const int MAGE_BALL_VY1   = 8;
const int MAGE_BALL_TY    = 9; // target Y (for frob_fly)
//const int FROB_FLY_LOWER   = 10; // set to 1 to start lowering toward target

const sint16 MAGE_BALL_ACCEL = 23;
const sint16 MAGE_BALL_CLAMP = 27 * MAGE_BALL_ACCEL;

void dog_init_mage_ball(uint8 d)
{
	// dormant, mage_ball_mode = 0
}

void dog_tick_mage_ball(uint8 d)
{
	if (dgd(MAGE_BALL_MODE) == 0) return;

	// spin

	++dgd(MAGE_BALL_ANIM);
	if (dgd(MAGE_BALL_ANIM) >= 5)
	{
		dgd(MAGE_BALL_ANIM) = 0;
		++dgd(MAGE_BALL_FRAME);
		if (dgd(MAGE_BALL_FRAME) >= 3)
		{
			dgd(MAGE_BALL_FRAME) = 0;
		}
	}

	if (dgd(MAGE_BALL_MODE) < 2) return;

	// move

	uint32 x = dgd_get24x(MAGE_BALL_X1);
	uint16 y = dgd_get16y(MAGE_BALL_Y1);
	sint16 vx = dgd_get16(MAGE_BALL_VX0, MAGE_BALL_VX1);
	sint16 vy = dgd_get16(MAGE_BALL_VY0, MAGE_BALL_VY1);
	uint32 nx = x + vx;
	uint16 ny = y + vy;

	bool bound = false;
	if (nx >= 0x800000) // i.e. high bit = negative, wrapped left
	{
		nx = 0;
		bound = true;
	}
	else if (nx >= 0x020000) // off right side
	{
		nx = 0x01FFFF;
		bound = true;
	}
	else if (!zp.room_scrolling && nx >= 0x010000)
	{
		nx = 0x00FFFF;
		bound = true;
	}

	if (ny >= (252 << 8)) // wrapped across top
	{
		ny = 0;
		bound = true;
	}
	else if (ny >= (244 << 8)) // hit bottom
	{
		ny = (243 << 8) | 0xFF;
		bound = true;
	}

	if (bound && dgd(MAGE_BALL_MODE) >= 5) // ball wants to die
	{
		dgd(MAGE_BALL_MODE) = 0;
		return;
	}

	dgd_put24x(nx,MAGE_BALL_X1);
	dgd_put16y(ny,MAGE_BALL_Y1);

	// hurt player
	if (bound_overlap_no_stone(d,-3,-2,3,4))
	{
		lizard_die();
	}

	// steering

	bool dx = false;
	bool dy = false;

	if (dgd(MAGE_BALL_MODE) >= 5 || lizard.dead) // escape
	{
		dgd(MAGE_BALL_MODE) = 5;
		if (vx >= 0) dx = true;
		if (vy >= 0) dy = true;
	}
	else // seek
	{
		uint16 tx = lizard_px;
		uint8 ty = lizard_py - 7;

		if (dgd(MAGE_BALL_MODE) >= 3)
		{
			ty = dgd(MAGE_BALL_TY);
			if (dgd(MAGE_BALL_MODE) >= 4)
			{
				tx = 128;
			}
		}

		if (dgx < tx) dx = true;
		if (dgy < ty) dy = true;
		//PPU::debug_text("mage_ball %d seek: %c,%c",d,dx?'X':'-',dy?'Y':'-');
	}

	if (dx) { vx += MAGE_BALL_ACCEL; if (vx >= MAGE_BALL_CLAMP) vx =  MAGE_BALL_CLAMP; }
	else    { vx -= MAGE_BALL_ACCEL; if (vx < -MAGE_BALL_CLAMP) vx = -MAGE_BALL_CLAMP; }
	if (dy) { vy += MAGE_BALL_ACCEL; if (vy >= MAGE_BALL_CLAMP) vy =  MAGE_BALL_CLAMP; }
	else    { vy -= MAGE_BALL_ACCEL; if (vy < -MAGE_BALL_CLAMP) vy = -MAGE_BALL_CLAMP; }

	dgd_put16(vx,MAGE_BALL_VX0,MAGE_BALL_VX1);
	dgd_put16(vy,MAGE_BALL_VY0,MAGE_BALL_VY1);
}

void dog_draw_mage_ball(uint8 d)
{
	if (dgd(MAGE_BALL_MODE) == 0) return;
	NES_ASSERT(dgd(MAGE_BALL_FRAME) < 3, "invalid mage_ball frame.");
	dx_scroll_offset(-4);
	sprite_tile_add(dx, dgy-4, 0x8B + dgd(MAGE_BALL_FRAME), 0x2 | (d & 1));
}

// DOG_GHOST

const int GHOST_ANIM  = 0;
const int GHOST_MODE  = 1; // 0=wait, 1=fade in, 2=fly/fade out, 3=fly/hit
const int GHOST_VX0   = 2;
const int GHOST_VX1   = 3;
const int GHOST_VY0   = 4;
const int GHOST_VY1   = 5;
const int GHOST_X1    = 6;
const int GHOST_Y1    = 7;
const int GHOST_FRAME = 8;
const int GHOST_TICK  = 9;
const int GHOST_FLOAT = 10;
const int GHOST_FLIP  = 11;
const int GHOST_BOSS  = 12; // 1 = wait is externally controlled

// if GHOST_BOSS == 1:
// to start ghost:
//   set GHOST_ANIM = 0
//   set GHOST_MODE = 0
//   set dgx = start x
//   set dgy = start y

void dog_init_ghost(uint8 d)
{
	dgd(GHOST_ANIM) = prng(8);
}

void dog_tick_ghost(uint8 d)
{
	if (dgd(GHOST_MODE) == 0) // wait
	{
		if (dgd(GHOST_ANIM) > 0)
		{
			if (dgd(GHOST_BOSS) == 0)
			{
				--dgd(GHOST_ANIM);
			}
			return;
		}
		else
		{
			dgd(GHOST_MODE) = 1;

			play_sound(SOUND_TWINKLE);

			// pick a location
			if (dgd(GHOST_BOSS) == 0)
			{
				uint8 dir = prng() & 1;
				if (lizard_px < 128)
				{
					dir = 0;
				}
				else if (!zp.room_scrolling || lizard_px >= (256+128))
				{
					dir = 1; // >= 128
				}

				dgd(GHOST_FLIP) = dir;

				dgy = (prng(3) & 127) + 56;

				if (dir == 0)
				{
					dgx = lizard_px + 116;
				}
				else
				{
					dgx = lizard_px - 116;
				}
			}
			else
			{
				if (dgx < lizard_px) dgd(GHOST_FLIP) = 0;
				else                 dgd(GHOST_FLIP) = 1;
			}
		}
	}

	// animate/float
	dgd(GHOST_FLOAT) += 4;
	++dgd(GHOST_TICK);
	if (dgd(GHOST_TICK) >= 6)
	{
		dgd(GHOST_TICK) = 0;
		dgd(GHOST_FRAME) = (dgd(GHOST_FRAME)+1) & 3;
	}

	if (dgd(GHOST_MODE) == 1) // fade in
	{
		++dgd(GHOST_ANIM);
		if (dgd(GHOST_ANIM) < 64)
		{
			return;
		}
		dgd(GHOST_MODE) = 3; // fly
		dgd(GHOST_ANIM) = 0;

		play_sound(SOUND_BOO);

		// calculate velocity to reach target in 64 frames
		uint16 tx16 = lizard_px << 2; // << 2 = * 256 / 64
		uint16 ty16 = (lizard_py - 8) << 2;
		uint16 sx16 = dgx << 2;
		uint16 sy16 = dgy << 2;

		uint16 dx16 = tx16 - sx16;
		uint16 dy16 = ty16 - sy16;

		//sint16 dx16s = dx16;
		//sint16 dy16s = dy16;
		//NES_DEBUG("ghost V: %d, %d (%d)\n",dx16s,dy16s,int(sqrt(dx16s*dx16s+dy16s*dy16s)));

		dgd_put16(dx16,GHOST_VX0,GHOST_VX1);
		dgd_put16(dy16,GHOST_VY0,GHOST_VY1);

		if (dx16 & 0x8000) dgd(GHOST_FLIP) = 0;
		else               dgd(GHOST_FLIP) = 1;
	}

	// move (mode 2 or 3)

	uint32 x = dgd_get24x(GHOST_X1);
	uint16 y = dgd_get16y(GHOST_Y1);

	sint16 dx = dgd_get16(GHOST_VX0,GHOST_VX1);
	sint16 dy = dgd_get16(GHOST_VY0,GHOST_VY1);

	x += dx;
	y += dy;

	dgd_put24x(x,GHOST_X1);
	dgd_put16y(y,GHOST_Y1);

	// stop at edges
	if (dgy >= 248) goto wait;
	if (!zp.room_scrolling)
	{
		if (dgx >= 256) goto wait;
	}
	else
	{
		if (dgx >= 512) goto wait;
	}

	if (dgd(GHOST_MODE) == 2)
	{
		if (dgd(GHOST_ANIM) > 0)
		{
			--dgd(GHOST_ANIM);
			return;
		}
		else goto wait;
	}

	if (dgd(GHOST_MODE) == 3) // hit
	{
		++dgd(GHOST_ANIM);
		if (dgd(GHOST_ANIM) >= 96)
		{
			dgd(GHOST_ANIM) = 64;
			dgd(GHOST_MODE) = 2;
			return;
		}

		sint8 f = circle4[dgd(GHOST_FLOAT)];
		if (bound_overlap_no_stone(d,-6,f-6,5,f+5))
		{
			lizard_die();
		}
	}

	NES_ASSERT(dgd(GHOST_MODE) < 4, "Invalid ghost mode!");
	return;

wait:
	dgd(GHOST_ANIM) = 128 | (prng(3) & 127);
	dgd(GHOST_MODE) = 0;
	return;
}

void dog_draw_ghost(uint8 d)
{
	CT_ASSERT(DATA_sprite0_ghost1 == DATA_sprite0_ghost0+1, "Ghost sprite out of order.");
	CT_ASSERT(DATA_sprite0_ghost2 == DATA_sprite0_ghost0+2, "Ghost sprite out of order.");
	CT_ASSERT(DATA_sprite0_ghost3 == DATA_sprite0_ghost0+3, "Ghost sprite out of order.");

	if (dgd(GHOST_MODE) == 0) return;

	if (dgd(GHOST_MODE) < 3) // fading
	{
		if ((prng() & 63) >= dgd(GHOST_ANIM)) return;
	}

	dx_scroll_edge();

	NES_ASSERT(dgd(GHOST_FRAME) < 4, "Ghost frame out of range.");
	uint8 s = DATA_sprite0_ghost0 + dgd(GHOST_FRAME);
	uint8 dy = dgy + circle4[dgd(GHOST_FLOAT)];

	if (!dgd(GHOST_FLIP)) sprite0_add(dx,dy,s);
	else          sprite0_add_flipped(dx,dy,s);
}

// DOG_PIGGY

const int PIGGY_MODE      = 0; // 0 = idle, 1 = coin drop, 2 = fade out
const int PIGGY_COIN_Y    = 1;
const int PIGGY_BLINK     = 2;
const int PIGGY_NUM_TIME  = 3;
const int PIGGY_FADE      = 4;
const int PIGGY_COUNT     = 5;
const int PIGGY_TEXT      = 6;

//CT_ASSERT(PIGGY_BURST == DATA_COIN_COUNT - 3, "PIGGY_BURST is supposed to be 3 less than all coins.");
CT_ASSERT(PIGGY_COIN_Y == COIN_ANIM, "PIGGY_COIN_Y is used by dog_draw_coin as an ersatz COIN_ANIM");

const uint8 PIGGY_TEXT_LIST[5] = { TEXT_PIGGY0, TEXT_PIGGY1, TEXT_PIGGY2, TEXT_PIGGY3, TEXT_PIGGY4 };
const uint8 PIGGY_TEXT_RANGE[5] = { 0, 25, 50, 75, 100 };

void dog_init_beyond_star(uint8 d); // forward declaration
void dog_init_particle(uint8 d); // forward declaration

void dog_piggy_spawn_beyond_star()
{
	if (zp.current_lizard != LIZARD_OF_BEYOND)
	{
		// hide any drips (that need palette 7)
		for (uint8 x=0; x<16; ++x)
		{
			if (dog_type(x) == DOG_DRIP)
			{
				empty_dog(x);
			}
		}
		palette_load(7,DATA_palette_lizard10);
		dog_type(BEYOND_STAR_SLOT) = DOG_BEYOND_STAR;
		//NOTE: init_beyond_star does nothing, the NES version isn't able to call it anyway due to banking issues
		dog_init_beyond_star(BEYOND_STAR_SLOT);
	}
}

void dog_piggy_range(uint8 d) // moves PIGGY_TEXT into current range
{
	if (dgd(PIGGY_TEXT) >= 4) return;

	while (PIGGY_TEXT_RANGE[dgd(PIGGY_TEXT) + 1] <= h.piggy_bank)
	{
		++dgd(PIGGY_TEXT);
		if (dgd(PIGGY_TEXT) >= 4) break;
	}
}

void dog_init_piggy(uint8 d)
{
	if (h.piggy_bank >= PIGGY_BURST)
	{
		empty_dog(d);
		dog_piggy_spawn_beyond_star();
		return;
	}

	dgd(PIGGY_COUNT) = coin_count();
	dog_blocker(d,-8,-13,10,-1);
	dgd(PIGGY_BLINK) = prng() & 127;
	dgd(PIGGY_FADE) = 127;

	dog_piggy_range(d);

	decimal_clear();
	decimal_add(h.piggy_bank);
}
void dog_tick_piggy(uint8 d)
{
	// blink, counter timer independent of everything else
	++dgd(PIGGY_BLINK);
	if (dgd(PIGGY_BLINK) >= 159)
	{
		dgd(PIGGY_BLINK) = prng() & 31;
	}
	if (dgd(PIGGY_NUM_TIME))
	{
		--dgd(PIGGY_NUM_TIME);
	}

	// fade mode, return if fading
	if (dgd(PIGGY_MODE) == 2)
	{
		if (dgd(PIGGY_FADE) == 0)
		{
			play_sound(SOUND_TWINKLE);

			// fade in beyond star
			dog_piggy_spawn_beyond_star();
			dog_data(BEYOND_STAR_FADE,BEYOND_STAR_SLOT) = 128;

			// kill particles
			dog_type(6) = DOG_NONE;
			dog_type(7) = DOG_NONE;
			dog_type(8) = DOG_NONE;

			empty_dog(d);
			return;
		}
		else
		{
			--dgd(PIGGY_FADE);
		}
		return;
	}

	// death
	if (bound_touch(d,-8,-13,10,-1))
	{
		if (zp.current_lizard == LIZARD_OF_DEATH)
		{
			empty_blocker(d);
			bones_convert(d,DATA_sprite0_piggy_skull,0);
			return;
		}
		else if (zp.current_lizard == LIZARD_OF_HEAT)
		{
			dgd(PIGGY_BLINK) = 153;
		}
	}

	// talk
	if (!lizard.dead &&
		lizard.power != 0 &&
		zp.current_lizard == LIZARD_OF_KNOWLEDGE &&
		bound_overlap(d,-40,-10,32,-1) &&
		dgd(PIGGY_TEXT) < 5 &&
		h.piggy_bank >= PIGGY_TEXT_RANGE[dgd(PIGGY_TEXT)])
	{
		dog_piggy_range(d);
		
		zp.game_message = PIGGY_TEXT_LIST[dgd(PIGGY_TEXT)];
		++dgd(PIGGY_TEXT);

		zp.game_pause = 1;
		return;
	}

	if (dgd(PIGGY_MODE) == 1) // coin mode
	{
		dgd(PIGGY_COIN_Y) += 7;
		if (dgd(PIGGY_COIN_Y) >= (dgy - 12))
		{
			++h.piggy_bank;
			dgd(PIGGY_MODE) = 0; // idle
			zp.coin_saved = 0;
			decimal_add(1);
			play_sound(SOUND_COIN);
		}
	}
	else
	{
		if (h.piggy_bank >= PIGGY_BURST)
		{
			// spawn particles
			for (int i=6; i<=8; ++i)
			{
				dog_type(i) = DOG_PARTICLE;
				dog_init_particle(i);
			}

			dgd(PIGGY_MODE) = 2; // fade
			empty_blocker(d);
			dgd(PIGGY_FADE) = 128;
			return;
		}

		if (bound_overlap(d,-34,-29,36,-1))
		{
			if (h.piggy_bank < dgd(PIGGY_COUNT))
			{
				dgd(PIGGY_MODE) = 1;
				dgd(PIGGY_COIN_Y) = 1;
			}
			dgd(PIGGY_NUM_TIME) = 128;
		}
	}
}

void dog_draw_piggy_body(uint8 d, uint8 dx)
{
	// randomized fadeout
	if (dgd(PIGGY_MODE) == 2)
	{
		uint8 r = prng(4) & 127;
		if (r >= dgd(PIGGY_FADE)) return;
	}

	uint8 sprite = DATA_sprite0_piggy_stand;
	if (dgd(PIGGY_BLINK) >= 153) sprite = DATA_sprite0_piggy_blink;
	sprite0_add(dx,dgy,sprite);
}
void dog_draw_piggy_coin(uint8 d)
{
	uint8 ty = dgy;
	dgy = dgd(PIGGY_COIN_Y);
	dog_draw_coin(d);
	dgy = ty;
}
void dog_draw_piggy_number(uint8 d, uint8 dx, uint8 num, sint8 offset)
{
	uint8 x = dx + offset;
	sprite_tile_add_clip(x,dgy-31,0xA0+num,0x01);
}
void dog_draw_piggy(uint8 d)
{
	dx_scroll_edge();
	dog_draw_piggy_body(d,dx);

	if (dgd(PIGGY_MODE) == 1) // coin mode
	{
		dog_draw_piggy_coin(d);
	}

	if (dgd(PIGGY_NUM_TIME) > 0)
	{
		if (h.decimal[2] > 0)
		{
			dog_draw_piggy_number(d,dx,h.decimal[2],-11);
			dog_draw_piggy_number(d,dx,h.decimal[1],-3 );
			dog_draw_piggy_number(d,dx,h.decimal[0], 5 );
		}
		else if (h.decimal[1] > 0)
		{
			dog_draw_piggy_number(d,dx,h.decimal[1],-7 );
			dog_draw_piggy_number(d,dx,h.decimal[0], 1 );
		}
		else
		{
			dog_draw_piggy_number(d,dx,h.decimal[0],-3 );
		}
	}
}

// =================
// common fire stuff
// =================

const int FIRE_PAL = 10;
const int FIRE_ANIM = 11;
void dog_tick_fire_common(uint8 d)
{
	if (dgd(FIRE_ANIM) == 0)
	{
		++dgd(FIRE_PAL);
		if (dgd(FIRE_PAL) >= 3)
			dgd(FIRE_PAL) = 0;
		palette_load(6|(d&1),DATA_palette_lava0 + dgd(FIRE_PAL));
		dgd(FIRE_ANIM) = 5;
	}
	else --dgd(FIRE_ANIM);
}

// DOG_PANDA_FIRE

void dog_init_panda_fire(uint8 d)
{
	dog_init_panda(d);
}
void dog_tick_panda_fire(uint8 d)
{
	dog_tick_fire_common(d);
	if (bound_overlap_no_stone(d,-8,-28,7,-16))
	{
		lizard_die();
	}
	dog_tick_panda(d);
}
void dog_draw_panda_fire(uint8 d)
{
	dog_draw_panda(d);
}

// DOG_GOAT_FIRE

void dog_init_goat_fire(uint8 d)
{
	dog_init_goat(d);
}
void dog_tick_goat_fire(uint8 d)
{
	dog_tick_fire_common(d);
	dog_tick_goat(d);
}
void dog_draw_goat_fire(uint8 d)
{
	dog_draw_goat(d);
}

// DOG_DOG_FIRE

void dog_init_dog_fire(uint8 d)
{
	dog_init_dog(d);
}
void dog_tick_dog_fire(uint8 d)
{
	dog_tick_fire_common(d);
	dog_tick_dog(d);
}
void dog_draw_dog_fire(uint8 d)
{
	dog_draw_dog(d);
}

// DOG_OWL_FIRE

void dog_init_owl_fire(uint8 d)
{
	dog_init_owl(d);
}
void dog_tick_owl_fire(uint8 d)
{
	dog_tick_fire_common(d);
	dog_tick_owl(d);
}
void dog_draw_owl_fire(uint8 d)
{
	dog_draw_owl(d);
}

// DOG_MEDUSA_FIRE

void dog_init_medusa_fire(uint8 d)
{
	dog_init_medusa(d);
}
void dog_tick_medusa_fire(uint8 d)
{
	dog_tick_fire_common(d);
	dog_tick_medusa(d);
}
void dog_draw_medusa_fire(uint8 d)
{
	dog_draw_medusa(d);
}

// DOG_ARROW_LEFT

const int ARROW_ANIM = 0;
const int ARROW_FLASH = 1;

void dog_init_arrow_left(uint8 d)
{
	dgp = 0;
	dgd(ARROW_ANIM) = 1 + (prng(8) & 63);
	dgd(ARROW_FLASH) = 170 + (prng(4) & 15);
}
void dog_tick_arrow_left(uint8 d)
{
	if (dgp == 0)
	{
		--dgd(ARROW_ANIM);
		if (dgd(ARROW_ANIM) == 0)
		{
			dgd(ARROW_ANIM) = dgd(ARROW_FLASH);
		}

		if (((dgy+4) >= (lizard_py-14)) && // based on lizard HITBOX
		    ((dgy+3) <= (lizard_py-1)) &&
			((dgx + 8) >= lizard_px))
		{
			dgp = 1;
			play_sound(SOUND_ARROW_FIRE);
		}
		return;
	}

	dgx -= 8;

	if (collide_tile(dgx-1,dgy+3))
	{
		dgx += 8;
		bones_convert_silent(d,DATA_sprite0_arrow_bone,1);
		play_sound(SOUND_ARROW_HIT);
		return;
	}

	if (bound_overlap_no_stone(d,0,3,7,4))
	{
		lizard_die();
	}
}
void dog_draw_arrow_common(uint8 d, uint8 attrib)

{
	dx_scroll();

	uint8 tile = 0xE8;
	if (dgp != 0)
	{
		tile = 0xE9;
	}
	else
	{
		if (dgd(ARROW_ANIM) < 20)
		{
			tile = 0xEE; // flash
			if (dgd(ARROW_ANIM) < 10)
			{
				attrib |= 0x80; // animate flash
			}
		}
	}

	sprite_tile_add(dx,dgy-1,tile,attrib);
}
void dog_draw_arrow_left(uint8 d)
{
	dog_draw_arrow_common(d,0x43);
}

// DOG_ARROW_RIGHT

void dog_init_arrow_right(uint8 d)
{
	dog_init_arrow_left(d);
}
void dog_tick_arrow_right(uint8 d)
{
	if (dgp == 0)
	{
		--dgd(ARROW_ANIM);
		if (dgd(ARROW_ANIM) == 0)
		{
			dgd(ARROW_ANIM) = dgd(ARROW_FLASH);
		}

		if (((dgy+4) >= (lizard_py-14)) && // based on lizard HITBOX
		    ((dgy+3) <= (lizard_py-1)) &&
			(dgx < lizard_px))
		{
			dgp = 1;
			play_sound(SOUND_ARROW_FIRE);
		}
		return;
	}

	dgx += 8;

	if (collide_tile(dgx+8,dgy+3))
	{
		bones_convert_silent(d,DATA_sprite0_arrow_bone,0);
		play_sound(SOUND_ARROW_HIT);
		return;
	}

	if (bound_overlap_no_stone(d,0,3,7,4))
	{
		lizard_die();
	}
}
void dog_draw_arrow_right(uint8 d)
{
	dog_draw_arrow_common(d,0x03);
}

// DOG_SAW

const int SAW_ANIM  = 0;
const int SAW_FRAME = 1;

void dog_init_saw(uint8 d)
{
	dgd(SAW_ANIM) = prng(3) & 7;
	dgd(SAW_FRAME) = prng(2);
}

void dog_tick_saw(uint8 d)
{
	if (bound_overlap(d,-8,-3,7,-1))
	{
		if (lizard.power > 0 && zp.current_lizard == LIZARD_OF_STONE)
		{
			return; // clogged the gears
		}
		else
		{
			lizard_die();
		}
	}

	++dgd(SAW_ANIM);
	if (dgd(SAW_ANIM) >= 6)
	{
		++dgd(SAW_FRAME);
		dgd(SAW_ANIM) = 0;
	}
}

void dog_draw_saw(uint8 d)
{
	CT_ASSERT(DATA_sprite0_saw1 == DATA_sprite0_saw0+1, "Saw sprites out of order!");
	CT_ASSERT(DATA_sprite0_saw2 == DATA_sprite0_saw0+2, "Saw sprites out of order!");
	CT_ASSERT(DATA_sprite0_saw3 == DATA_sprite0_saw0+3, "Saw sprites out of order!");

	dx_scroll_edge();

	uint8 s = DATA_sprite0_saw0 + (dgd(SAW_FRAME) & 3);

	// flip if an odd slot
	if ((d & 1) == 0) sprite0_add(        dx,dgy,s);
	else              sprite0_add_flipped(dx,dgy,s);
}

// DOG_STEAM

const int STEAM_ANIM  = 0; // counts 6 frames
const int STEAM_FRAME = 1; // incremented by ANIM counter
const int STEAM_FADE  = 2; // counts 0,1,2 if == 1 flicker hide this frame
const int STEAM_MODE  = 3; // mode
const int STEAM_TICK  = 4; // time in mode

const uint8 STEAM_SPRITE[4*8] = {
	0,                          0,                          0,                          0,
	DATA_sprite0_steam_pre0,     DATA_sprite0_steam_pre1,     DATA_sprite0_steam_pre2,     DATA_sprite0_steam_pre3,
	DATA_sprite0_steam_low0,     DATA_sprite0_steam_low1,     DATA_sprite0_steam_low2,     DATA_sprite0_steam_low3,
	DATA_sprite0_steam_mid0,     DATA_sprite0_steam_mid1,     DATA_sprite0_steam_mid2,     DATA_sprite0_steam_mid3,
	DATA_sprite0_steam_high0,    DATA_sprite0_steam_high1,    DATA_sprite0_steam_high2,    DATA_sprite0_steam_high3,
	DATA_sprite0_steam_release0, DATA_sprite0_steam_release1, DATA_sprite0_steam_release2, DATA_sprite0_steam_release3,
	DATA_sprite0_steam_out0,     DATA_sprite0_steam_out1,     DATA_sprite0_steam_out2,     DATA_sprite0_steam_out3,
	DATA_sprite0_steam_off0,     DATA_sprite0_steam_off1,     DATA_sprite0_steam_off2,     DATA_sprite0_steam_off3,
};

// hitbox Y-offset
const sint8 STEAM_OFF0[8] = {   0,   0, -15, -23, -31, -31, -31,   0 };
const sint8 STEAM_OFF1[8] = {   0,   0,  -1,  -1,  -1,  -9, -17,   0 };
const uint8 STEAM_TIME[8] = {   0,  60,   6,   6,  60,   6,   6,   6 };

void dog_init_steam(uint8 d)
{
	redo_anim:
	dgd(STEAM_ANIM) = prng(3) & 7;
	if (dgd(STEAM_ANIM) >= 6) goto redo_anim;

	dgd(STEAM_FRAME) = prng(3);

	redo_fade:
	dgd(STEAM_FADE) = prng(2) & 3;
	if (dgd(STEAM_FADE) >= 3) goto redo_fade;

	redo_tick:
	dgd(STEAM_TICK) = prng(8);
	if (dgd(STEAM_TICK) > dgp) goto redo_tick;
}

void dog_tick_steam(uint8 d)
{
	NES_ASSERT(dgd(STEAM_MODE) < 8,"STEAM_MODE invalid!");

	// constant animation
	++dgd(STEAM_ANIM);
	if (dgd(STEAM_ANIM) >= 6)
	{
		dgd(STEAM_ANIM) = 0;
		++dgd(STEAM_FRAME);
	}

	++dgd(STEAM_TICK);
	if (dgd(STEAM_MODE) == 0)
	{
		if (dgd(STEAM_TICK) < dgp) goto tick_done;
	}
	else
	{
		if (dgd(STEAM_TICK) < STEAM_TIME[dgd(STEAM_MODE)]) goto tick_done;
	}
	dgd(STEAM_TICK) = 0;
	dgd(STEAM_MODE) = (dgd(STEAM_MODE) + 1) & 7;
	if (dgd(STEAM_MODE) == 2)
	{
		play_sound_scroll(SOUND_STEAM);
	}
	tick_done:

	sint8 hb0 = STEAM_OFF0[dgd(STEAM_MODE)];
	sint8 hb1 = STEAM_OFF1[dgd(STEAM_MODE)];

	if (hb0 == 0) return; // no hitbox this frame

	if(bound_overlap_no_stone(d,-5,hb0,4,hb1))
	{
		lizard_die();
	}
}

void dog_draw_steam(uint8 d)
{
	++dgd(STEAM_FADE);
	if (dgd(STEAM_FADE) >= 3)
	{
		dgd(STEAM_FADE) = 0;
		return;
	}

	if (dgd(STEAM_MODE) == 0) return;
	

	dx_scroll_edge();
	
	uint8 sx = (dgd(STEAM_MODE) << 2) | (dgd(STEAM_FRAME) & 3);
	NES_ASSERT(sx < (4*8), "steam sprite out of range!");
	sprite0_add(dx,dgy,STEAM_SPRITE[sx]);
}

// DOG_SPARKD

const int SPARK_LEN = 0;

void dog_init_spark(uint8 d)
{
	dgd(SPARK_LEN) = ((dgp & 7) + 1) << 3;
	dgp &= 0x38; // param is phase, 0-63 (in starting increments of 8)
}

void dog_hit_spark(uint8 d, sint16 sx, sint8 sy)
{
	if (dgp >= dgd(SPARK_LEN)) return;
	if (bound_overlap_no_stone(d,sx+2,sy+2,sx+5,sy+5))
	{
		lizard_die();
	}
}

void dog_draw_spark(uint8 d, sint16 sx, sint8 sy)
{
	if (dgp >= dgd(SPARK_LEN)) return;

	uint16 sdx = dgx + sx;
	if (sdx >= zp.scroll_x && ((sdx - zp.scroll_x) < 256))
	{
		uint8 dx = (sdx - zp.scroll_x) & 0xFF;
		uint8 dy = dgy + sy;
		uint8 blink = (zp.nmi_count << 2) & 0x30;

		sprite_tile_add(dx,dy-1,0x46 | blink,0x23); // background attribute
	}
}

void dog_init_sparkd(uint8 d) { dog_init_spark(d); }
void dog_tick_sparkd(uint8 d) { dgp = (dgp+1)&63; dog_hit_spark(d,0,dgp); }
void dog_draw_sparkd(uint8 d) { dog_draw_spark(d,0,dgp); }

// DOG_SPARKU

void dog_init_sparku(uint8 d) { dog_init_spark(d); }
void dog_tick_sparku(uint8 d) { dgp = (dgp+1)&63; dog_hit_spark(d,0,-dgp); }
void dog_draw_sparku(uint8 d) { dog_draw_spark(d,0,-dgp); }

// DOG_SPARKL

void dog_init_sparkl(uint8 d) { dog_init_spark(d); }
void dog_tick_sparkl(uint8 d) { dgp = (dgp+1)&63; dog_hit_spark(d,-dgp,0); }
void dog_draw_sparkl(uint8 d) { dog_draw_spark(d,-dgp,0); }

// DOG_SPARKR

void dog_init_sparkr(uint8 d) { dog_init_spark(d); }
void dog_tick_sparkr(uint8 d) { dgp = (dgp+1)&63; dog_hit_spark(d,dgp,0); }
void dog_draw_sparkr(uint8 d) { dog_draw_spark(d,dgp,0); }

// DOG_FROB_FLY

void dog_init_frob_fly(uint8 d)
{
	// NOT IN DEMO
}
void dog_tick_frob_fly(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_frob_fly(uint8 d)
{
	// NOT IN DEMO
}

//
// BANK1 ($D)
//

// DOG_PASSWORD

const int PASSWORD_ON    = 0;
//const int PASSWORD_VALUE = 1;
void dog_init_password(uint8 d)
{
	NES_ASSERT(zp.room_scrolling == 0, "DOG_PASSWORD does not support scrolling!");

	if (dgp == 255)
	{
		return;
	}

	NES_ASSERT(dgp < 5, "DOG_PASSWORD index out of bounds?");
	if (dgp < 5)
	{
		dgd(PASSWORD_VALUE) = zp.password[dgp];
	}
}
void dog_tick_password(uint8 d)
{
	if (bound_overlap(d,0,0,7,3))
	{
		if (dgd(PASSWORD_ON) == 0)
		{
			dgd(PASSWORD_ON) = 1; // stepped on
			if (dgp != 255)
			{
				dgd(PASSWORD_VALUE) = (dgd(PASSWORD_VALUE) + 1) & 7;
			}
			else
			{
				zp.current_lizard += 1;
				if (zp.current_lizard >= (debug ? LIZARD_OF_COUNT : LIZARD_OF_COFFEE)) zp.current_lizard = 0;
				zp.next_lizard = zp.current_lizard;
				lizard.power = 0;
				for (int d=0; d<16; ++d)
				{
					if (dog_type(d) == DOG_SAVE_STONE)
					{
						dog_data(SAVE_STONE_ON,d) = 0;
					}
				}
			}
			play_sound(SOUND_SWITCH);
		}
	}
	else
	{
		dgd(PASSWORD_ON) = 0; // stepped off
	}
}
void dog_draw_password(uint8 d)
{
	dx_screen();

	if (dgd(PASSWORD_ON) == 0)
		sprite1_add(dx,dgy,DATA_sprite1_password);
	else
		sprite1_add(dx,dgy,DATA_sprite1_password_down);

	if (dgp != 255)
	{
		sprite_tile_add(dx,dgy-48,0x61 + dgd(PASSWORD_VALUE),1);
	}
}

// DOG_LAVA_PALETTE

const int LAVA_PALETTE_FRAME = 0;
const int LAVA_PALETTE_CYCLE = 1;
void dog_init_lava_palette(uint8 d) {}
void dog_tick_lava_palette(uint8 d)
{
	++dgd(LAVA_PALETTE_FRAME);
	if (dgd(LAVA_PALETTE_FRAME) >= 9)
	{
		dgd(LAVA_PALETTE_FRAME) = 0;
		++dgd(LAVA_PALETTE_CYCLE);
		if (dgd(LAVA_PALETTE_CYCLE) >= 3)
			dgd(LAVA_PALETTE_CYCLE) = 0;
		palette_load(3,DATA_palette_lava0 + dgd(LAVA_PALETTE_CYCLE));
	}
}
void dog_draw_lava_palette(uint8 d) {}

// DOG_WATER_SPLIT

void dog_tick_water_split(uint8 d); // forward

void dog_init_water_split(uint8 d)
{
	dgp = zp.water;
	dog_tick_water_split(d);
}
void dog_tick_water_split(uint8 d)
{
	if (lizard_px < 256)
	{
		zp.water = 255;
		lizard.wet = 0;
	}
	else
	{
		zp.water = dgp;
	}
}
void dog_draw_water_split(uint8 d) {}

// DOG_BLOCK

void dog_init_block(uint8 d)
{
	dog_blocker(d,-8,0,7,15);
}

void dog_block_stack(uint8 d, sint8 u); // forward declaration

void dog_block_shift(uint8 d, sint8 u)
{
	sint8 pdx = u;
	sint8 pdy = 0;

	empty_blocker(d);
	if (!move_dog(d,-8,0,7,15,&pdx,&pdy))
	{
		dgx += pdx;
		if (dgx < 9) { dgx = 9; pdx = 0; }
		if (!zp.room_scrolling && dgx >= 248) { dgx = 248; pdx = 0; }
		if (zp.room_scrolling && dgx >= 504) { dgx = 504; pdx = 0; }

		dog_init_block(d);
		if (pdx)
		{
			dog_block_stack(d,pdx);
		}
	}
	else
	{
		dog_init_block(d);
	}
}

void dog_block_stack(uint8 d, sint8 u)
{
	if (u == 0) return;

	uint16 bx = dgx - u;
	uint8 by = dgy - 16;

	for (uint8 d2 = 0; d2 < 16; ++d2)
	{
		if (d2 == d) continue;
		if (dog_type(d2) != DOG_BLOCK &&
		    dog_type(d2) != DOG_BLOCK_ON) continue;

		if (by != dog_y(d2)) continue;
		if ((dog_x(d2) + 11) < bx) continue;
		if ((bx + 11) < dog_x(d2)) continue;

		dog_block_shift(d2,u);
	}
}

void dog_tick_block(uint8 d)
{
	if (do_fall(d,-8,0,7,15))
	{
		// falling
		dgy += 1;
		if (dgy >= 240)
		{
			empty_blocker(d);
			empty_dog(d);
			return;
		}
		dog_init_block(d);
	}

	uint8 result = do_push(d,-8,0,7,15);
	if (result == 1) // push right
	{
		dgx += 1;
		dog_init_block(d); // update blocker position
		dog_block_stack(d,1); // shift any stacked blocks
	}
	else if (result == 2) // push left
	{
		dgx -= 1;
		dog_init_block(d);
		dog_block_stack(d,-1);
	}
	else return;
}

void dog_draw_block(uint8 d)
{
	dx_scroll_edge();

	if (dog_type(DISMOUNT_SLOT) == DOG_LIZARD_DISMOUNTER && zp.current_lizard == LIZARD_OF_PUSH)
	{
		// override for the room where you mount lizard of push;
		// mounting the lizard changed palette 3
		// so reuse the player's palette 0 which is now the push palette
		sprite1_add(dx,dgy,DATA_sprite1_block_alt);
	}
	else
	{
		sprite1_add(dx,dgy,DATA_sprite1_block);
	}
}

// DOG_BLOCK_ON

void dog_init_block_on(uint8 d)
{
	if (flag_read(dgp))
	{
		empty_dog(d);
		return;
	}
	dog_init_block(d);
}
void dog_tick_block_on(uint8 d)
{
	if (do_fall(d,-8,0,7,15))
	{
		flag_set(dgp);
		dgt = DOG_BLOCK;
	}
	dog_tick_block(d);
}
void dog_draw_block_on(uint8 d)
{
	dog_draw_block(d);
}

// DOG_BLOCK_OFF

void dog_init_block_off(uint8 d)
{
	if (!flag_read(dgp))
	{
		empty_dog(d);
		return;
	}
	dgt = DOG_BLOCK;
	dog_init_block(d);
}
void dog_tick_block_off(uint8 d)
{
	dog_tick_block(d);
}
void dog_draw_block_off(uint8 d)
{
	dog_draw_block(d);
}

// DOG_DRAWBRIDGE

const int DRAWBRIDGE_BUTTON      = 0;
const int DRAWBRIDGE_ANIM        = 1;
const int DRAWBRIDGE_TILE_OFFSET = 2;

const uint8 DRAWBRIDGE_CHAIN[32 * 2] = {
	0x02,0x02,0x02,0x03,0x01,0x00,0x02,0x02,0x04,0x04,0x04,0x00,0x00,0x02,0x04,0x04,
	0x40,0x40,0x40,0x00,0x80,0x00,0x40,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	// 2-bit
	0x0F,0x0F,0x0F,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0x0F,0x0F,0x0F,
	0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


void dog_drawbridge_test(uint8 d)
{
	dgd(DRAWBRIDGE_BUTTON) = 0;
	if (collide_all(184,119))
	{
		dgd(DRAWBRIDGE_BUTTON) = 2;
	}
	else if (lizard_overlap(180,117,187,119))
	{
		dgd(DRAWBRIDGE_BUTTON) = 1;
	}
}

void dog_drawbridge_block(uint8 d)
{
	dog_blocker(d,-8,0,7,63);
}

void dog_drawbridge_move(uint8 d)
{
	dog_drawbridge_block(d);
	if (dgy >= 144)
	{
		play_sound(SOUND_STONE);
	}
	else
	{
		play_sound(SOUND_DRAWBRIDGE);
	}

	// draw chain (tiles CA/CB, 32 bytes)
	zp.nmi_load = 0x0CA0;
	zp.nmi_next = NMI_ROW;

	uint8 s = ((0 - dgy) & 7);
	s += dgd(DRAWBRIDGE_TILE_OFFSET);

	uint8 y;
	uint8 x = 0;
	const uint8* tile = DRAWBRIDGE_CHAIN;

	for (y=s; (y&31)<8; ++y)
	{
		stack.nmi_update[x+ 0] = tile[y+ 0];
		stack.nmi_update[x+ 8] = tile[y+ 8];
		stack.nmi_update[x+16] = tile[y+16];
		stack.nmi_update[x+24] = tile[y+24];
		++x;
	}
	for (y-=8; y<s;  ++y)
	{
		stack.nmi_update[x+ 0] = tile[y+ 0];
		stack.nmi_update[x+ 8] = tile[y+ 8];
		stack.nmi_update[x+16] = tile[y+16];
		stack.nmi_update[x+24] = tile[y+24];
		++x;
	}
}

void dog_init_drawbridge(uint8 d)
{
	if (flag_read(FLAG_EYESIGHT))
	{
		dgd(DRAWBRIDGE_TILE_OFFSET) = 32;
	}

	dog_drawbridge_test(d);
	dog_drawbridge_block(d);
}

void dog_tick_drawbridge(uint8 d)
{
	dog_drawbridge_test(d);

	++dgd(DRAWBRIDGE_ANIM);
	if ((dgd(DRAWBRIDGE_ANIM) & 3) > 0) return;

	if (dgd(DRAWBRIDGE_BUTTON) == 0)
	{
		if (bound_overlap(d,-8,0,7,64)) return; // don't move
		if (dgy < 144)
		{
			++dgy;
			dog_drawbridge_move(d);
		}
	}
	else
	{
		if (dgy >= 94)
		{
			--dgy;
			dog_drawbridge_move(d);
		}
	}
}

void dog_draw_drawbridge_door(uint8 d)
{
	dx_scroll_edge();
	sprite1_add(dx,dgy,DATA_sprite1_drawbridge_door);
}
void dog_draw_drawbridge_button(uint8 d)
{
	if (dgd(DRAWBRIDGE_BUTTON) >= 2) return;

	dx_scroll_offset(92);
	uint8 dy = 111;

	uint8 attrib = 0x83;
	uint8 tile = 0xE0 | dgd(DRAWBRIDGE_BUTTON);
	sprite_tile_add(dx,dy,tile,attrib);
}

void dog_draw_drawbridge(uint8 d)
{
	dog_draw_drawbridge_door(d);
	dog_draw_drawbridge_button(d);
}

// DOG_ROPE

const int ROPE_TILT = 0;
const int ROPE_BURN = 1;

void dog_init_rope(uint8 d)
{
	if (flag_read(dgp))
	{
		empty_dog(d);
		return;
	}

	dog_init_block(d); // blocker -8,0,7,15
}
void dog_tick_rope(uint8 d)
{
	if (bound_overlap(d,-8,-1,-8,-1) ||
		bound_overlap(d, 7,16, 7,16))
	{
		dgd(ROPE_TILT) = 1;
	}
	else if (bound_overlap(d,7,-1,7,-1) ||
		     bound_overlap(d,-8,16,-8,16))
	{
		dgd(ROPE_TILT) = 2;
	}
	else
	{
		dgd(ROPE_TILT) = 0;
	}

	if (dgd(ROPE_BURN) > 0)
	{
		++dgd(ROPE_BURN);
		if (dgd(ROPE_BURN) >= 128)
		{
			flag_set(dgp);
			// turn to block
			dgt = DOG_BLOCK;
			return;
		}
	}
	else
	{
		if (zp.current_lizard == LIZARD_OF_HEAT && bound_touch(d,-1,-64,0,-1))
		{
			dgd(ROPE_BURN) = 1;
		}
	}
}
void dog_draw_rope_block(uint8 d)
{
	dx_scroll_edge();
	uint8 s = DATA_sprite1_block;
	if      (dgd(ROPE_TILT) == 1) s = DATA_sprite1_block_tilt_left;
	else if (dgd(ROPE_TILT) == 2) s = DATA_sprite1_block_tilt_right;
	sprite1_add(dx,dgy,s);
}
void dog_draw_rope_cord(uint8 d)
{
	dx_scroll_offset(-4);
	uint8 dy = dgy - 65;

	uint8 attrib = 0x01;
	uint8 tile   = 0xDF;
	if (dgd(ROPE_BURN) > 0)
	{
		tile = 0xCE | ((dgd(ROPE_BURN) >> 2) & 1);
		attrib = 0x00;
	}

	for (int i=0; i<8; ++i)
	{
		sprite_tile_add(dx,dy,tile,attrib);
		dy += 8;
	}
}

void dog_draw_rope(uint8 d)
{
	dog_draw_rope_block(d);
	dog_draw_rope_cord(d);
}

// DOG_BOSS_FLAME

const int BOSS_FLAME_ANIM   = 0;
const int BOSS_FLAME_ATTRIB = 1;

void dog_init_boss_flame(uint8 d)
{
	dgd(BOSS_FLAME_ATTRIB) = (prng() & 0x40) | 0x03;
	dgd(BOSS_FLAME_ANIM) = prng(8);

	NES_ASSERT(dgp < 6, "boss_flame param out of range");

	if (!flag_read(dgp + FLAG_BOSS_0_MOUNTAIN))
	{
		empty_dog(d);
		return;
	}

	if (dog_type(1) == DOG_NONE)
	{
		for (uint8 i=FLAG_BOSS_0_MOUNTAIN; i<= FLAG_BOSS_5_VOID; ++i)
		{
			if (!flag_read(i)) return;
		}
		// disabled for demo
		//dog_type(1) = DOG_BOSS_DOOR;
		//dog_param(1) = 6;
		//dog_init_boss_door(1);
	}
}

void dog_tick_boss_flame(uint8 d)
{
	++dgd(BOSS_FLAME_ANIM);
	if (bound_overlap_no_stone(d,-2,-5,1,-1))
	{
		lizard_die();
	}
}

void dog_draw_boss_flame(uint8 d)
{
	dx_scroll_offset(-4);
	uint8 dy = dgy - 9;
	uint8 t = ((dgd(BOSS_FLAME_ANIM) >> 2) & 0x7) + 0x84;
	uint8 att = dgd(BOSS_FLAME_ATTRIB);

	sprite_tile_add(dx,dy,t,att);
}

// DOG_RIVER

//const int RIVER_SCROLL_A0   = 0;
//const int RIVER_SCROLL_A1   = 1;
//const int RIVER_SCROLL_B0   = 2;
//const int RIVER_SCROLL_B1   = 3;
//const int RIVER_SPLASH_TIME = 4;
//const int RIVER_SPLASH_FLIP = 5;
//const int RIVER_OVERLAP     = 6;
const int RIVER_HP          = 7;
const int RIVER_SEQ0        = 8;
const int RIVER_SEQ1        = 9;
const int RIVER_LOOP0       = 10;
const int RIVER_LOOP1       = 11;
const int RIVER_SEQ_TIME    = 12;
const int RIVER_SNEK_Y      = 13;
// dog_param is RIVER_SNEK_SEQ

const int RIVER_HP_TOTAL = 5;

enum RiverEvent
{
	RE_LOOP = 0,
	RE_ROCK0,
	RE_ROCK1,
	RE_ROCK2,
	RE_ROCK3,
	RE_ROCK4,
	RE_ROCK5,
	RE_ROCK6,
	RE_ROCK7,
	RE_LOG,
	RE_DUCK,
	RE_RAMP,
	RE_RIVER_SEEKER,
	RE_BARREL,
	RE_WAVE,
	RE_SNEK_LOOP,
	RE_SNEK_HEAD,
	RE_SNEK_TAIL,
	RE_COUNT,
};

const uint8 RIVER_SEQUENCE[] =
{
	//  stream of 3 byte entries:
	// 1. time to event (frames)
	// 2. RiverEvent
	// 3. Y location (or parameter)
	// final byte is time before entering the next loop

	240, RE_ROCK0,        165,
	190, RE_LOOP,           1,
	 70, RE_LOOP,           0,
	 60
};

//                    Y hits     (Y seen   )  [time]  pal  comment
//   RE_ROCK0         111 - 184  (113 - 188)  [ 70]     6  block
//   RE_ROCK1         108 - 184  (105 - 188)  [ 70]     6  thin
//   RE_ROCK2         107 - 184  (104 - 188)  [ 70]     6  thin
//   RE_ROCK3         126 - 184  (121 - 188)  [ 70]     6  cluster
//   RE_ROCK4         105 - 184  (104 - 188)  [ 70]     6  small, peaky
//   RE_ROCK5         107 - 184  (104 - 188)  [ 70]     6  small
//   RE_ROCK6         108 - 184  (104 - 188)  [ 70]     6  small, fat
//   RE_ROCK7         110 - 184  (105 - 188)  [ 70]     6  small, pair
//   RE_LOG           123 - 161  (122 - 165)  [140]     7
//   RE_DUCK          114 - 186  (121 - 189)  [280]   6/7  adds 100 to the next delay
//   RE_RAMP          120 - 184  (121 - 189)  [120]     7  50 preview arrow, 70 onscreen
//   RE_RIVER_SEEKER  106 - 188  (   N/A   )  [280]     7
//   RE_BARREL          ? -   ?  (  ? -   ?)  [ 70]     6
//   RE_WAVE            ? -   ?  (  ? -   ?)  [~70]     7
//   RE_SNEK_LOOP     108 - 184  (   N/A   )  [150]     7
//   RE_SNEK_HEAD       ? -   ?  (  ? -   ?)  [  ?]     7
//   RE_SNEK_TAIL       ? -   ?  (  ? -   ?)  [  ?]     6

const uint8 RIVER_EVENT_TYPE[] =
{
	DOG_RIVER_LOOP,
	DOG_ROCK, DOG_ROCK, DOG_ROCK, DOG_ROCK, DOG_ROCK, DOG_ROCK, DOG_ROCK, DOG_ROCK,
	DOG_LOG,
	DOG_DUCK,
	DOG_RAMP,
	DOG_RIVER_SEEKER,
	DOG_BARREL,
	DOG_WAVE,
	DOG_SNEK_LOOP,
	DOG_SNEK_HEAD,
	DOG_SNEK_TAIL,
};
CT_ASSERT(ELEMENTS_OF(RIVER_EVENT_TYPE)==RE_COUNT,"RIVER_EVENT_TYPE entry mismatch.");

uint8 dog_river_spawn(uint8 seq_type, uint8 seq_y)
{
	uint8 dy = 1;
	for (; dy < 15; ++dy)
	{
		if (dog_type(dy) == DOG_NONE)
		{
			break;
		}
	}
	NES_ASSERT(dog_type(dy) == DOG_NONE,"No empty dogs for river event!");
	// fallback just uses dog 15

	dog_type(dy) =  RIVER_EVENT_TYPE[seq_type];
	dog_param(dy) = seq_type;
	dog_y(dy) = seq_y;
	dog_x(dy) = 0;

	dog_init(dy);
	return dy;
}

void dog_river_event(uint8 d)
{
	uint16 seq_pos = dgd_get16(RIVER_SEQ1,RIVER_SEQ0);
	uint8 seq_type = RIVER_SEQUENCE[seq_pos+0];
	uint8 seq_y    = RIVER_SEQUENCE[seq_pos+1];
	uint8 seq_time = RIVER_SEQUENCE[seq_pos+2];
	seq_pos += 3;
	dgd_put16(seq_pos,RIVER_SEQ1,RIVER_SEQ0);

	dgd(RIVER_SEQ_TIME) = seq_time;
	dog_river_spawn(seq_type, seq_y);
}

void dog_init_river(uint8 d)
{
	NES_ASSERT(d == RIVER_SLOT, "river dog may only be used in slot 0");
	dgp = 0;

	// time until first event
	dgd(RIVER_SEQ_TIME) = RIVER_SEQUENCE[0];
	dgd_put16(1,RIVER_SEQ1,RIVER_SEQ0);
}
void dog_tick_river(uint8 d)
{
	// scroll

	unsigned int sx0 = dgd_get16(RIVER_SCROLL_A1, RIVER_SCROLL_A0);
	unsigned int sx1 = dgd_get16(RIVER_SCROLL_B1, RIVER_SCROLL_B0);
	sx0 += 2;
	sx1 += 4;
	dgd_put16(sx0,RIVER_SCROLL_A1,RIVER_SCROLL_A0);
	dgd_put16(sx1,RIVER_SCROLL_B1,RIVER_SCROLL_B0);

	// splash
	if (dgd(RIVER_SPLASH_TIME) < 16)
	{
		++dgd(RIVER_SPLASH_TIME);
		if (zp.smoke_x >= 4)
		{
			zp.smoke_x -= 4;
		}
		else
		{
			zp.smoke_y = 255;
		}

		// random flip
		dgd(RIVER_SPLASH_FLIP) =  prng() & 0x40;
	}

	// sequence
	if (dgd(RIVER_SEQ_TIME) > 0)
	{
		--dgd(RIVER_SEQ_TIME);
	}
	else
	{
		dog_river_event(d);
	}

	if (lizard.dead) return;

	// splash new
	if (dgd(RIVER_SPLASH_TIME) >= 16)
	{
		if (lizard.wet)
		{
			dgd(RIVER_SPLASH_TIME) = 17;
		}
		else if (prng() >= 196 || dgd(RIVER_SPLASH_TIME) == 17)
		{
			zp.smoke_x = lizard_px - 11;
			zp.smoke_y = lizard_py - 8;
			if (zp.smoke_x < lizard_px && zp.smoke_y < lizard_py)
			{
				dgd(RIVER_SPLASH_TIME) = 0;
			}
		}
	}
}
void dog_draw_river(uint8 d) {}

// DOG_RIVER_ENTER

void dog_init_river_enter(uint8 d)
{
	if (flag_read(FLAG_BOSS_1_RIVER))
	{
		dgp = 5;
		h_door_link[5] = DATA_room_end_river_again;
	}
}
void dog_tick_river_enter(uint8 d)
{
	if (lizard_py > zp.water &&
	    (lizard_py - zp.water) > 8)
	{
		lizard_die();
		return;
	}

	if (!lizard.dead &&
		zp.current_lizard == LIZARD_OF_SURF &&
		lizard_px >= 488)
	{
		zp.current_door = dgp & 7;
		zp.current_room = h_door_link[zp.current_door];
		zp.room_change = 1;
	}
}
void dog_draw_river_enter(uint8 d) {}

// DOG_SPRITE1

void dog_init_sprite1(uint8 d) { }
void dog_tick_sprite1(uint8 d) { }
void dog_draw_sprite1(uint8 d)
{
	dx_scroll_edge();
	sprite1_add(dx,dgy,dgp);
}

// DOG_BEYOND_STAR

const int BEYOND_STAR_ANIM   = 0;
const int BEYOND_STAR_FRAME = 1;
//const int BEYOND_STAR_FADE  = 2;
//const int BEYOND_STAR_DIE   = 3;

void dog_init_beyond_star(uint8 d)
{
	NES_ASSERT(d == BEYOND_STAR_SLOT,"beyond_star belongs in BEYOND_STAR_SLOT");
}
void dog_tick_beyond_star(uint8 d)
{
	// rotate
	++dgd(BEYOND_STAR_ANIM);
	if (dgd(BEYOND_STAR_ANIM) >= 5)
	{
		dgd(BEYOND_STAR_ANIM) = 0;
		++dgd(BEYOND_STAR_FRAME);
		if (dgd(BEYOND_STAR_FRAME) >= 3)
		{
			dgd(BEYOND_STAR_FRAME) = 0;
		}
	}

	// fade
	if (dgd(BEYOND_STAR_FADE))
	{
		if (dgd(BEYOND_STAR_DIE))
		{
			// note that BEYOND_STAR_DIE is hard coded to rainbow-cycle the lizard palette

			++dgd(BEYOND_STAR_FADE);
			if (dgd(BEYOND_STAR_FADE) >= 128)
			{
				empty_dog(d);
				return;
			}
		}
		else
		{
			--dgd(BEYOND_STAR_FADE);
		}
		return;
	}

	// touch
	if (bound_overlap(d,-7,-13,6,-4))
	{
		play_sound(SOUND_SECRET);
		palette_load(4,DATA_palette_lizard10);
		zp.current_lizard = LIZARD_OF_BEYOND;
		zp.next_lizard = LIZARD_OF_BEYOND;
		h.last_lizard = 0xFF;
		dgd(BEYOND_STAR_FADE) = 1;
		dgd(BEYOND_STAR_DIE) = 1;
	}
}
void dog_draw_beyond_star(uint8 d)
{
	if (dgd(BEYOND_STAR_FADE))
	{
		uint8 r = prng() & 127;
		if (r < dgd(BEYOND_STAR_FADE)) return;
	}

	CT_ASSERT(DATA_sprite1_beyond_star2 == DATA_sprite1_beyond_star+1, "beyond_star sprites out of order.");
	CT_ASSERT(DATA_sprite1_beyond_star3 == DATA_sprite1_beyond_star+2, "beyond_star sprites out of order.");
	NES_ASSERT(dgd(BEYOND_STAR_FRAME) < 3, "BEYOND_STAR_FRAME out of bounds.");

	dx_scroll_edge();
	sprite1_add(dx,dgy,DATA_sprite1_beyond_star + dgd(BEYOND_STAR_FRAME));
}

// DOG_BEYOND_END

const int BEYOND_END_TIMEOUT = 0;
const int BEYOND_END_DONE    = 1;

void dog_init_beyond_end(uint8 d)
{
	CT_ASSERT(DATA_palette_human1 == DATA_palette_human0+1,"palette_human out of order");
	CT_ASSERT(DATA_palette_human2 == DATA_palette_human0+2,"palette_human out of order");
	CT_ASSERT(DATA_palette_human1_sky == DATA_palette_human0_sky+1,"palette_human_sky out of order");
	CT_ASSERT(DATA_palette_human2_sky == DATA_palette_human0_sky+2,"palette_human_sky out of order");
	CT_ASSERT(DATA_palette_human1_black == DATA_palette_human0_black+1,"palette_human_black out of order");
	CT_ASSERT(DATA_palette_human2_black == DATA_palette_human0_black+2,"palette_human_black out of order");

	uint8 p1 = zp.human0_pal+1;
	uint8 p2 = zp.human0_pal+2;
	if (p1 >= 3) p1 -= 3;
	if (p2 >= 3) p2 -= 3;
	palette_load(6,p1+DATA_palette_human0_black);
	palette_load(3,p2+DATA_palette_human0_sky);

	h.palette[0x0F] = h.palette[0x07]; // replace sky colour (for verso version)
}
void dog_tick_beyond_end(uint8 d)
{
	if (dgd(BEYOND_END_DONE))
	{
		zp.room_change = 1;
		zp.current_door = 1;
		zp.current_room = h_door_link[1];
		return;
	}

	if (dog_type(BEYOND_STAR_SLOT) == DOG_NONE)
	{
		lizard.dismount = 3; // forced fly
		++dgd(BEYOND_END_TIMEOUT);
		if (lizard_py < 40 || dgd(BEYOND_END_TIMEOUT) >= 250)
		{
			lizard.vy = 0; // prevent motion on last frame before fadeout
			dgd(BEYOND_END_DONE) = 1;
			zp.game_message = TEXT_BEYOND;
			zp.game_pause = 1;
			return;
		}
	}
}
void dog_draw_beyond_end(uint8 d)
{
}

// DOG_OTHER_END_LEFT

const int OTHER_END_ANIM = 0;

void dog_blocker_other_end(uint8 d)
{
	if (lizard_px >= 256)
	{
		if (dgx < 256) return;
	}
	else
	{
		if (dgx >= 256) return;
	}
	dog_blocker(d,-8,-14,7,-1);
}

uint16 dog_nmt_other_end(uint8 d)
{
	uint16 addr =
		(((dgx >> 3) - 0x01) & 0x001F) | // (X / 8)
		(((dgy << 2) - 0x40) & 0x03E0) | // (Y / 8) * 32
		( (dgx << 2)         & 0x0400) | // nametable
		0x2000;
	return addr;
}
void dog_nmt_clear_other_end(uint8 d)
{
	uint16 addr = dog_nmt_other_end(d);
	PPU::latch(addr);
	PPU::write(0x91);
	PPU::write(0x91);
}

void dog_init_other_end_left(uint8 d)
{
	dgd(OTHER_END_ANIM) = prng(8);

	NES_ASSERT(dgp < 6, "other_end parameter out of range");
	sint8 p = h.human1_set[dgp];

	if (p >= 3 || p < 0)
	{
		dog_nmt_clear_other_end(d);
		empty_dog(d);
		return;
	}

	p -= zp.human0_pal;
	if (p < 0) p += 3;
	dgp = p;

	if (p < 2)
	{
		dog_nmt_clear_other_end(d);
	}

	dog_blocker_other_end(d);
}
void dog_tick_other_end_left(uint8 d)
{
	if(bound_touch_death(d,-8,-14,7,-1))
	{
		stack.nmi_update[0] = 2;
		uint16 addr = dog_nmt_other_end(d);
		stack.nmi_update[1] = (addr >> 8);
		stack.nmi_update[2] = (addr & 0xFF);
		stack.nmi_update[3] = 0x91;
		++addr;
		stack.nmi_update[4] = (addr >> 8);
		stack.nmi_update[5] = (addr & 0xFF);
		stack.nmi_update[6] = 0x91;
		zp.nmi_next = NMI_STREAM;

		empty_blocker(d);
		bones_convert(d,DATA_sprite0_lizard_skull_dismount, (dgt == DOG_OTHER_END_LEFT) ? 0 : 1);
		return;
	}

	dog_blocker_other_end(d);

	if (dgd(OTHER_END_ANIM) > 0)
	{
		--dgd(OTHER_END_ANIM);
	}
	else
	{
		dgd(OTHER_END_ANIM) = prng(4) | 0x80;
	}

	if (
		lizard.power > 0 &&
		zp.current_lizard == LIZARD_OF_KNOWLEDGE &&
		!lizard.dead &&
		h.boss_talk == 0 &&
		bound_overlap(d,-20,-31,19,32))
	{
		h.boss_talk = 1;
		zp.game_message = TEXT_OTHER_END;
		zp.game_pause = 1;
	}
}

void dog_draw_other_end_left(uint8 d)
{
	dx_scroll_edge();

	uint8 s = DATA_sprite1_other_end0;
	if (dgd(OTHER_END_ANIM) < 8)
	{
		s = DATA_sprite1_other_end_blink0;
	}

	s += dgp;

	if (dgt == DOG_OTHER_END_LEFT)
	{
		sprite1_add(dx,dgy,s);
	}
	else
	{
		sprite1_add_flipped(dx,dgy,s);
	}
}

// DOG_OTHER_END_RIGHT

void dog_init_other_end_right(uint8 d) { dog_init_other_end_left(d); }
void dog_tick_other_end_right(uint8 d) { dog_tick_other_end_left(d); }
void dog_draw_other_end_right(uint8 d) { dog_draw_other_end_left(d); }

// DOG_PARTICLE

const int PARTICLE_XH   = 0;
const int PARTICLE_X    = 1;
const int PARTICLE_Y    = 2;
const int PARTICLE_WAIT = 3;
const int PARTICLE_TIME = 4;

void dog_init_particle(uint8 d)
{
	dgd_put16(dgx,PARTICLE_XH,PARTICLE_X);
	dgd(PARTICLE_Y) = dgy;
	dgd(PARTICLE_WAIT) = prng(3) & 7;
}
void dog_tick_particle(uint8 d)
{
	if (dgd(PARTICLE_TIME) > 0)
	{
		// particle rises
		--dgy;
		--dgd(PARTICLE_TIME);
		if (dgd(PARTICLE_TIME) == 0)
		{
			// particle is done, begin wait
			dgd(PARTICLE_WAIT) = prng(2) & 7;
		}
		return;
	}

	if (dgd(PARTICLE_WAIT) > 0)
	{
		--dgd(PARTICLE_WAIT);
		return;
	}
	else
	{
		// spawn particle
		dgx = dgd_get16(PARTICLE_XH,PARTICLE_X) + (prng(2) & 7);
		dgy = dgd(PARTICLE_Y);
		dgd(PARTICLE_TIME) = dgp + (prng(2) & 7);
	}
}
void dog_draw_particle(uint8 d)
{
	if (dgd(PARTICLE_TIME) == 0) return;
	dx_scroll();
	sprite_tile_add(dx,dgy,0xC6,0x01);
}

// DOG_INFO

void dog_init_info(uint8 d)
{
	zp.chr_cache[3] = 0xFF; // invalidate dog_b2 in case of eyesight

	PPU::latch_at(0,4);
	text_row_write(TEXT_INFO0);
	text_row_write(TEXT_INFO1);

	PPU::latch_at(0,4+32);
	text_row_write(TEXT_INFO5);

	PPU::latch_at(0,6+32);
	for (int i=6; i<13; ++i)
	{
		text_row_write(TEXT_INFO0+i);
	}

	CT_ASSERT(TEXT_INFO5  == TEXT_INFO0+5 ,"TEXT_INFO out of order!");
	CT_ASSERT(TEXT_INFO6  == TEXT_INFO0+6 ,"TEXT_INFO out of order!");
	CT_ASSERT(TEXT_INFO7  == TEXT_INFO0+7 ,"TEXT_INFO out of order!");
	CT_ASSERT(TEXT_INFO8  == TEXT_INFO0+8 ,"TEXT_INFO out of order!");
	CT_ASSERT(TEXT_INFO9  == TEXT_INFO0+9 ,"TEXT_INFO out of order!");
	CT_ASSERT(TEXT_INFO10 == TEXT_INFO0+10,"TEXT_INFO out of order!");
	CT_ASSERT(TEXT_INFO11 == TEXT_INFO0+11,"TEXT_INFO out of order!");
	CT_ASSERT(TEXT_INFO12 == TEXT_INFO0+12,"TEXT_INFO out of order!");
}
void dog_tick_info(uint8 d) {}
void dog_draw_info(uint8 d)
{
	dx_scroll();
	sprite_tile_add(dx,207,0x60 + GAME_VERSION,0x01);
}

// DOG_DIAGNOSTIC

const int DIAGNOSTIC_TOGGLE_ON   = 0;
// 1 was removed, didn't feel like reshuffling
const int DIAGNOSTIC_SELECT_COIN = 2;
const int DIAGNOSTIC_SELECT_FLAG = 3;
const int DIAGNOSTIC_SELECT      = 4;
const int DIAGNOSTIC_HOLD        = 5;
const int DIAGNOSTIC_COIN_0      = 6;
const int DIAGNOSTIC_COIN_1      = 7;
const int DIAGNOSTIC_COIN_2      = 8;
const int DIAGNOSTIC_JUMP_0      = 9;
const int DIAGNOSTIC_JUMP_1      = 10;
const int DIAGNOSTIC_JUMP_2      = 11;
const int DIAGNOSTIC_JUMP_3      = 12;
const int DIAGNOSTIC_JUMP_4      = 13;

void dog_nmt_diagnostic_common()
{
	// blank
	for (int i=32; i<64; ++i) stack.nmi_update[i] = 0x70;
	// columns
	stack.nmi_update[ 0] = 0xB8;
	stack.nmi_update[30] = 0xB8;
	stack.nmi_update[32] = 0xB8;
	stack.nmi_update[62] = 0xB8;
	stack.nmi_update[ 1] = 0xB9;
	stack.nmi_update[31] = 0xB9;
	stack.nmi_update[33] = 0xB9;
	stack.nmi_update[63] = 0xB9;
}

void dog_nmt_diagnostic_time()
{
	text_load(TEXT_DIAGNOSTIC_PLAYTIME);
	dog_nmt_diagnostic_common();
	decimal_clear();
	decimal_add(h.metric_time_h);
	stack.nmi_update[15] = h.decimal[1] | 0x60;
	stack.nmi_update[16] = h.decimal[0] | 0x60;
	stack.nmi_update[17] = 0x5E;
	decimal_clear();
	decimal_add(h.metric_time_m);
	stack.nmi_update[18] = h.decimal[1] | 0x60;
	stack.nmi_update[19] = h.decimal[0] | 0x60;
	stack.nmi_update[20] = 0x5E;
	decimal_clear();
	decimal_add(h.metric_time_s);
	stack.nmi_update[21] = h.decimal[1] | 0x60;
	stack.nmi_update[22] = h.decimal[0] | 0x60;
	stack.nmi_update[23] = 0x5C;
	decimal_clear();
	decimal_add(h.metric_time_f);
	stack.nmi_update[24] = h.decimal[1] | 0x60;
	stack.nmi_update[25] = h.decimal[0] | 0x60;
	nmi_update_at(0,6);
}

void dog_nmt_diagnostic_jumps()
{
	text_load(TEXT_DIAGNOSTIC_JUMPS);
	dog_nmt_diagnostic_common();

	stack.nmi_update[11] = h.decimal[4] | 0x60;
	stack.nmi_update[12] = h.decimal[3] | 0x60;
	stack.nmi_update[13] = h.decimal[2] | 0x60;
	stack.nmi_update[14] = h.decimal[1] | 0x60;
	stack.nmi_update[15] = h.decimal[0] | 0x60;

	for (int i=0; i<4; ++i)
	{
		if (stack.nmi_update[i+11] != 0x60) break;
		stack.nmi_update[i+11] = 0x70;
	}
	nmi_update_at(0,8);
}

void dog_nmt_diagnostic_coins(uint8 d)
{
	decimal_clear();
	decimal_add(coin_count());
	dgd(DIAGNOSTIC_COIN_0) = h.decimal[0];
	dgd(DIAGNOSTIC_COIN_1) = h.decimal[1];
	dgd(DIAGNOSTIC_COIN_2) = h.decimal[2];

	text_load(TEXT_DIAGNOSTIC_COINS);
	dog_nmt_diagnostic_common();
	for (int i=0; i<8; ++i)
	{
		uint8 v = h.coin[i];
		stack.nmi_update[ 0 + 12 + (i*2) + 0] = hex_to_ascii[v >>   4];
		stack.nmi_update[ 0 + 12 + (i*2) + 1] = hex_to_ascii[v & 0x0F];

		v = h.coin[i+8];
		stack.nmi_update[32 + 12 + (i*2) + 0] = hex_to_ascii[v >>   4];
		stack.nmi_update[32 + 12 + (i*2) + 1] = hex_to_ascii[v & 0x0F];
	}
	nmi_update_at(0,14);
}

void dog_nmt_diagnostic_flags()
{
	text_load(TEXT_DIAGNOSTIC_FLAGS);
	dog_nmt_diagnostic_common();
	for (int i=0; i<8; ++i)
	{
		uint8 v = h.flag[i];
		stack.nmi_update[ 0 + 12 + (i*2) + 0] = hex_to_ascii[v >>   4];
		stack.nmi_update[ 0 + 12 + (i*2) + 1] = hex_to_ascii[v & 0x0F];

		v = h.flag[i+8];
		stack.nmi_update[32 + 12 + (i*2) + 0] = hex_to_ascii[v >>   4];
		stack.nmi_update[32 + 12 + (i*2) + 1] = hex_to_ascii[v & 0x0F];
	}
	nmi_update_at(0,16);
}

void dog_init_diagnostic(uint8 d)
{
	NES_ASSERT(zp.room_scrolling == 0, "DOG_DIAGNOSTIC does not support scrolling!");
	NES_ASSERT(dog_type(DISMOUNT_SLOT) == DOG_LIZARD_DISMOUNTER,"dismounter required in DISMOUNT_SLOT!");
	dgd(DIAGNOSTIC_HOLD) = 1;

	h_door_link[0] = h.diagnostic_room; // return to room left

	dog_nmt_diagnostic_time();
	ppu_nmi_update_row();

	PPU::latch_at(11,7);
	decimal_clear();
	decimal_add32(h.metric_bones);
	decimal_print();

	PPU::latch_at(11,8);
	decimal_clear();
	decimal_add32(h.metric_jumps);
	dgd(DIAGNOSTIC_JUMP_0) = h.decimal[0];
	dgd(DIAGNOSTIC_JUMP_1) = h.decimal[1];
	dgd(DIAGNOSTIC_JUMP_2) = h.decimal[2];
	dgd(DIAGNOSTIC_JUMP_3) = h.decimal[3];
	dgd(DIAGNOSTIC_JUMP_4) = h.decimal[4];
	dgp = h.metric_jumps & 0xFF;
	decimal_print();

	PPU::latch_at(11,10);
	decimal_clear();
	decimal_add32(h.metric_continue);
	decimal_print();

	PPU::latch_at(10,12);
	text_load(system_pal() ? TEXT_DIAGNOSTIC_PAL : TEXT_DIAGNOSTIC_NTSC);
	for (int i=0; i<4; ++i) PPU::write(stack.nmi_update[i]);

	dog_nmt_diagnostic_coins(d);
	ppu_nmi_update_double();

	dog_nmt_diagnostic_flags();
	ppu_nmi_update_double();
}

void dog_tick_diagnostic(uint8 d)
{
	// invalidate cache in case flags demand a rebuild
	for (int i=0; i<8; ++i) zp.chr_cache[i] = 0xFF;

	uint8 uds = zp.gamepad & (PAD_U | PAD_D | PAD_SELECT);
	if (uds == 0)
	{
		dgd(DIAGNOSTIC_HOLD) = 0;
	}
	else if (dgd(DIAGNOSTIC_HOLD) == 0)
	{
		if (uds == PAD_D)
		{
			if (dgd(DIAGNOSTIC_SELECT) == 0) --dgd(DIAGNOSTIC_SELECT_COIN);
			else                             --dgd(DIAGNOSTIC_SELECT_FLAG);
			dgd(DIAGNOSTIC_SELECT_COIN) &= 127;
			dgd(DIAGNOSTIC_SELECT_FLAG) &= 127;
		}
		else if (uds == PAD_U)
		{
			if (dgd(DIAGNOSTIC_SELECT) == 0) ++dgd(DIAGNOSTIC_SELECT_COIN);
			else                             ++dgd(DIAGNOSTIC_SELECT_FLAG);
			dgd(DIAGNOSTIC_SELECT_COIN) &= 127;
			dgd(DIAGNOSTIC_SELECT_FLAG) &= 127;
		}
		else if (uds == PAD_SELECT)
		{
			dgd(DIAGNOSTIC_SELECT) ^= 1;
		}
		dgd(DIAGNOSTIC_HOLD) = 1;
	}

	NES_ASSERT(zp.nmi_next == NMI_READY, "nmi_next conflict!");
	dog_nmt_diagnostic_time();
	zp.nmi_next = NMI_ROW;

	if (dgp != (h.metric_jumps & 0xFF))
	{
		h.decimal[0] = dgd(DIAGNOSTIC_JUMP_0);
		h.decimal[1] = dgd(DIAGNOSTIC_JUMP_1);
		h.decimal[2] = dgd(DIAGNOSTIC_JUMP_2);
		h.decimal[3] = dgd(DIAGNOSTIC_JUMP_3);
		h.decimal[4] = dgd(DIAGNOSTIC_JUMP_4);
		decimal_add(1);
		dgd(DIAGNOSTIC_JUMP_0) = h.decimal[0];
		dgd(DIAGNOSTIC_JUMP_1) = h.decimal[1];
		dgd(DIAGNOSTIC_JUMP_2) = h.decimal[2];
		dgd(DIAGNOSTIC_JUMP_3) = h.decimal[3];
		dgd(DIAGNOSTIC_JUMP_4) = h.decimal[4];

		dgp = h.metric_jumps & 0xFF;
		dog_nmt_diagnostic_jumps();
		zp.nmi_next = NMI_ROW;
		// jump this frame prevents toggle (so nmi update is never missed)
	}
	else if (lizard_overlap(24,184,24+7,184+3))
	{
		if (dgd(DIAGNOSTIC_TOGGLE_ON) == 0)
		{
			dgd(DIAGNOSTIC_TOGGLE_ON) = 1; // stepped on
			play_sound(SOUND_SWITCH);

			if (dgd(DIAGNOSTIC_SELECT) == 0)
			{
				if (coin_read(dgd(DIAGNOSTIC_SELECT_COIN))) coin_return(dgd(DIAGNOSTIC_SELECT_COIN));
				else                                        coin_take(  dgd(DIAGNOSTIC_SELECT_COIN));

				dog_nmt_diagnostic_coins(d);
			}
			else
			{
				if (flag_read(dgd(DIAGNOSTIC_SELECT_FLAG))) flag_clear(dgd(DIAGNOSTIC_SELECT_FLAG));
				else                                        flag_set(  dgd(DIAGNOSTIC_SELECT_FLAG));

				dog_nmt_diagnostic_flags();
			}

			zp.nmi_next = NMI_DOUBLE;
		}
	}
	else
	{
		dgd(DIAGNOSTIC_TOGGLE_ON) = 0; // stepped off
	}

	if (lizard.scuttle >= 254) // test of BSOD
	{
		set_game_mode(GAME_CRASH);
		return;
	}

	if (zp.gamepad == (PAD_SELECT | PAD_D))
	{
		h.human0_hair = prng(8);

		redo_human0_pal:
		zp.human0_pal = prng(2) & 3;
		if (zp.human0_pal == 0) goto redo_human0_pal;
		--zp.human0_pal;

		zp.room_change = 1;
		zp.current_door = 1;

		// doesn't really affect save on NES
		// but if using the resume feature, does need to be resaved
		zp.coin_saved = 0;
	}
}

void dog_draw_diagnostic(uint8 d)
{
	sprite_prepare(128);

	if (dgd(DIAGNOSTIC_TOGGLE_ON) == 0)
		sprite1_add(24,184,DATA_sprite1_password);
	else
		sprite1_add(24,184,DATA_sprite1_password_down);

	if (zp.gamepad & PAD_U     ) sprite_tile_add(128,87,0xE7,0x00);
	if (zp.gamepad & PAD_D     ) sprite_tile_add(136,87,0xE7,0x00);
	if (zp.gamepad & PAD_L     ) sprite_tile_add(144,87,0xE7,0x00);
	if (zp.gamepad & PAD_R     ) sprite_tile_add(152,87,0xE7,0x00);
	if (zp.gamepad & PAD_B     ) sprite_tile_add(160,87,0xE7,0x00);
	if (zp.gamepad & PAD_A     ) sprite_tile_add(168,87,0xE7,0x00);
	if (zp.gamepad & PAD_SELECT) sprite_tile_add(184,87,0xE7,0x00);
	if (zp.gamepad & PAD_START ) sprite_tile_add(208,87,0xE7,0x00);

	sprite_tile_add(152,151+(dgd(DIAGNOSTIC_SELECT)*8),0xFE,0x43);

	uint8 v = dgd(DIAGNOSTIC_SELECT_COIN);
	sprite_tile_add(104,151,hex_to_ascii[v & 0x0F] ^ 0xC0,0x00);
	sprite_tile_add( 96,151,hex_to_ascii[v >>   4] ^ 0xC0,0x00);
	sprite_tile_add(136,151,0xA0 | (coin_read(v) ? 1 : 0), 0x00);

	v = dgd(DIAGNOSTIC_SELECT_FLAG);
	sprite_tile_add(104,159,hex_to_ascii[v & 0x0F] ^ 0xC0,0x00);
	sprite_tile_add( 96,159,hex_to_ascii[v >>   4] ^ 0xC0,0x00);
	sprite_tile_add(136,159,0xA0 | (flag_read(v) ? 1 : 0), 0x00);

	if (dgd(DIAGNOSTIC_COIN_2) != 0)                                sprite_tile_add(104,71,0xA0 | dgd(DIAGNOSTIC_COIN_2),0x01);
	if (dgd(DIAGNOSTIC_COIN_2) != 0 || dgd(DIAGNOSTIC_COIN_1) != 0) sprite_tile_add(112,71,0xA0 | dgd(DIAGNOSTIC_COIN_1),0x01);
	                                                                sprite_tile_add(120,71,0xA0 | dgd(DIAGNOSTIC_COIN_0),0x01);

	sprite_tile_add( 88+ 0,223,0xA0 | VERSION_MAJOR  ,0x01);
	sprite_tile_add( 88+16,223,0xA0 | VERSION_MINOR  ,0x01);
	sprite_tile_add( 88+32,223,0xA0 | VERSION_BETA   ,0x01);
	sprite_tile_add( 88+48,223,0xA0 | VERSION_REVISED,0x01);

	// human
	sprite_tile_add(216,63,0x6E,0x02);
	sprite_tile_add(215,71,0x6F,0x02);
}

// DOG_METRICS

void dog_metrics_time(uint8 d)
{
	decimal_clear();
	decimal_add(h.metric_time_h);
	dgd(0) = h.decimal[1] | 0x60;
	dgd(1) = h.decimal[0] | 0x60;
	decimal_clear();
	decimal_add(h.metric_time_m);
	dgd(2) = h.decimal[1] | 0x60;
	dgd(3) = h.decimal[0] | 0x60;
	decimal_clear();
	decimal_add(h.metric_time_s);
	dgd(4) = h.decimal[1] | 0x60;
	dgd(5) = h.decimal[0] | 0x60;
	decimal_clear();
	decimal_add(h.metric_time_f);
	dgd(6) = h.decimal[1] | 0x60;
	dgd(7) = h.decimal[0] | 0x60;
}

// minutes for boss rush reward (NTSC, PAL, EASY)
const uint8 RUSH_TIME[3] = { 40, 48, 60 };

void dog_init_metrics(uint8 d)
{
	NES_ASSERT(dog_type(HOLD_SLOT) == DOG_HOLD_SCREEN,"metrics requires hold screen");

	dog_metrics_time(d);

	PPU::latch_at(17,9);
	decimal_clear();
	decimal_add32(h.metric_bones);
	metric_print();

	PPU::latch_at(17,11);
	decimal_clear();
	decimal_add32(h.metric_jumps);
	metric_print();

	PPU::latch_at(17,13);
	decimal_clear();
	decimal_add(coin_count());
	metric_print();

	if (dog_param(HOLD_SLOT) >= 255) // ending holds forever
	{
		PPU::latch_at(8,21);
		if (h.boss_rush)                   text_row_write(TEXT_END_BOSS_RUSH);
		if (h.end_book)                    text_row_write(TEXT_END_BOOK);
		else                               text_row_write(TEXT_END_LOOP);
		if (h.metric_continue > 0)         text_row_write(TEXT_END_CONTINUED);
		if (h.metric_cheater)              text_row_write(TEXT_END_CHEATED);

		uint8 rush_index = system_pal() ? 1 : 0;
		if (system_easy()) rush_index = 2;
		if (!h.boss_rush && h.metric_time_h < 1 && h.metric_time_m < RUSH_TIME[rush_index])
		{
			if (h.end_verso == 0)          text_row_write(TEXT_END_SPEEDY_RECTO);
			else                           text_row_write(TEXT_END_SPEEDY_VERSO);
		}

		if (lizard.big_head_mode & 0x80)   text_row_write(TEXT_END_BIG_HEAD);
		if (!system_pal())                 text_row_write(TEXT_END_NTSC);
		else                               text_row_write(TEXT_END_PAL);
	}
}
void dog_tick_metrics(uint8 d)
{
	dog_metrics_time(d);
}
void dog_draw_metrics(uint8 d)
{
	uint8 dx = 84;
	uint8 dy = 135;
	uint8 att = 0x02;
	sprite_tile_add(dx,dy,dgd(0),att); dx += 8;
	sprite_tile_add(dx,dy,dgd(1),att); dx += 16;
	sprite_tile_add(dx,dy,dgd(2),att); dx += 8;
	sprite_tile_add(dx,dy,dgd(3),att); dx += 16;
	sprite_tile_add(dx,dy,dgd(4),att); dx += 8;
	sprite_tile_add(dx,dy,dgd(5),att); dx += 16;
	sprite_tile_add(dx,dy,dgd(6),att); dx += 8;
	sprite_tile_add(dx,dy,dgd(7),att);
}

// DOG_SUPER_MOOSE

const int SUPER_MOOSE_TICK  = 0;
const int SUPER_MOOSE_FRAME = 1;

const uint8 SUPER_MOOSE_ANIMS[0x12] =
{
	/* 00 */ DATA_sprite1_super_moose,
	/* 01 */ 0,
	/* 02 */ DATA_sprite1_super_moose_blink,
	/* 03 */ DATA_sprite1_super_moose,
	/* 04 */ DATA_sprite1_super_moose_blink,
	/* 05 */ 0,
	/* 06 */ DATA_sprite1_super_moose_step,
	/* 07 */ DATA_sprite1_super_moose_step,
	/* 08 */ DATA_sprite1_super_moose,
	/* 09 */ DATA_sprite1_super_moose_low,
	/* 0A */ 0
};

const uint8 SUPER_MOOSE_TASKS[8] = {
	0x04, // blink
	0x04, // blink
	0x04, // blink
	0x04, // blink
	0x04, // blink
	0x02, // blink 2
	0x06, // step
	0x06, // step
};

const uint8 SUPER_MOOSE_TEXT_LIST[] = {
	TEXT_MOOSE0,  TEXT_MOOSE1,  TEXT_MOOSE2,  TEXT_MOOSE3,  TEXT_MOOSE4,
	TEXT_MOOSE5,  TEXT_MOOSE6,  TEXT_MOOSE7,  TEXT_MOOSE8,  TEXT_MOOSE9,
	TEXT_MOOSE10, TEXT_MOOSE11, TEXT_MOOSE12, TEXT_MOOSE13, TEXT_MOOSE14,
	TEXT_MOOSE15, TEXT_MOOSE16, TEXT_MOOSE17, TEXT_MOOSE18, TEXT_MOOSE19,
	TEXT_MOOSE20, TEXT_MOOSE21, TEXT_MOOSE22, TEXT_MOOSE23, TEXT_MOOSE24,
	TEXT_MOOSE25, TEXT_MOOSE26, TEXT_MOOSE27, TEXT_MOOSE28
};
const uint8 SUPER_MOOSE_TEXT_COUNT = ELEMENTS_OF(SUPER_MOOSE_TEXT_LIST);
CT_ASSERT(SUPER_MOOSE_TEXT_COUNT == 29, "ensure SUPER_MOOSE_TEXT_LIST and COUNT matches NES version, and has prime length");

void dog_init_super_moose(uint8 d)
{
	zp.chr_cache[3] = 0xFF; // invalidate dog_b2 in case of eyesight

	dgd(SUPER_MOOSE_TICK) = prng();
	dog_blocker(d,-9,-16,6,-1);
}
void dog_tick_super_moose(uint8 d)
{
	// death
	if (bound_touch(d,-7,-16,4,-1) ||
		bound_touch(d,4,-23,13,-17))
	{
		if (zp.current_lizard == LIZARD_OF_DEATH)
		{
			empty_blocker(d);
			bones_convert(d,DATA_sprite0_super_moose_skull,0);
			return;
		}
		else
		{
			dgd(SUPER_MOOSE_FRAME) = 0x02; // hold on blink 2
			dgd(SUPER_MOOSE_TICK) = 5;
		}
	}

	// talk
	if (!lizard.dead &&
		lizard.power != 0 &&
		zp.current_lizard == LIZARD_OF_KNOWLEDGE &&
		bound_overlap(d,7,-24,40,-1))
	{
		NES_ASSERT(h.moose_text < SUPER_MOOSE_TEXT_COUNT,"moose_text out of range!");
		if (h.moose_text >= SUPER_MOOSE_TEXT_COUNT) h.moose_text = 0; // safety

		zp.game_message = SUPER_MOOSE_TEXT_LIST[h.moose_text];
		
		if (h.moose_text_inc == 0)
		{
			CT_ASSERT(SUPER_MOOSE_TEXT_COUNT > 17, "super moose text increment too large for range")
			h.moose_text_inc = (prng(4) & 15) + 5; // 5-21 is relatively prime to 29
			//h.moose_text_inc = 1; // for testing
		}
		h.moose_text += h.moose_text_inc;
		while (h.moose_text >= SUPER_MOOSE_TEXT_COUNT) h.moose_text -= SUPER_MOOSE_TEXT_COUNT;

		zp.game_pause = 1;
		return;
	}

	// don't do anything until TICK reaches 0
	if (dgd(SUPER_MOOSE_TICK) > 0)
	{
		--dgd(SUPER_MOOSE_TICK);
		return;
	}

	// advance 1 frame
	++dgd(SUPER_MOOSE_FRAME);
	dgd(SUPER_MOOSE_TICK) = 5;
	if (SUPER_MOOSE_ANIMS[dgd(SUPER_MOOSE_FRAME)] != 0)
	{
		return;
	}

	// end of task reached
	if (dgd(SUPER_MOOSE_FRAME) == 1) // have been idle, begin new task
	{
		uint8 task = prng(3) & 7;
		dgd(SUPER_MOOSE_FRAME) = SUPER_MOOSE_TASKS[task];
	}
	else // go back to idle
	{
		dgd(SUPER_MOOSE_FRAME) = 0; // idle
		dgd(SUPER_MOOSE_TICK) = (prng(4) & 127) + 90;
	}

}
void dog_draw_super_moose(uint8 d)
{
	dx_scroll_edge();
	uint8 sprite = SUPER_MOOSE_ANIMS[dgd(SUPER_MOOSE_FRAME)];
	NES_ASSERT(dgd(SUPER_MOOSE_FRAME) < 0x0A, "super_moose invalid frame");
	NES_ASSERT(sprite != 0, "super_moose invalid sprite");
	sprite1_add(dx,dgy,sprite);
}

// DOG_BRAD_DUNGEON

void dog_init_brad_dungeon(uint8 d)
{
	// NOT IN DEMO
}
void dog_tick_brad_dungeon(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_brad_dungeon(uint8 d)
{
	// NOT IN DEMO
}

// DOG_BRAD

void dog_init_brad(uint8 d)
{
	// NOT IN DEMO
}
void dog_tick_brad(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_brad(uint8 d)
{
	// NOT IN DEMO
}

// DOG_HEEP_HEAD

void dog_init_heep_head(uint8 d)
{
	// NOT IN DEMO
}
void dog_tick_heep_head(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_heep_head(uint8 d)
{
	// NOT IN DEMO
}

// DOG_HEEP

void dog_init_heep(uint8 d)
{
	// NOT IN DEMO
}
void dog_tick_heep(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_heep(uint8 d)
{
	// NOT IN DEMO
}

// DOG_HEEP_TAIL

void dog_init_heep_tail(uint8 d)
{
	// NOT IN DEMO
}
void dog_tick_heep_tail(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_heep_tail(uint8 d)
{
	// NOT IN DEMO
}

// DOG_LAVA_LEFT

void dog_init_lava_left(uint8 d) {}
void dog_tick_lava_left(uint8 d)
{
	if (bound_overlap(d,0,0,31,7))
	{
		lizard.flow = 1; // flow left
		if (zp.current_lizard != LIZARD_OF_STONE || lizard.power == 0)
		{
			lizard_die();
		}
	}
}
void dog_draw_lava_left(uint8 d) {}

// DOG_LAVA_RIGHT

void dog_init_lava_right(uint8 d) {}
void dog_tick_lava_right(uint8 d)
{
	if (bound_overlap(d,0,0,31,7))
	{
		lizard.flow = 2; // flow right
		if (zp.current_lizard != LIZARD_OF_STONE || lizard.power == 0)
		{
			lizard_die();
		}
	}
}
void dog_draw_lava_right(uint8 d) {}

// DOG_LAVA_LEFT_WIDE

void dog_init_lava_left_wide(uint8 d) {}
void dog_tick_lava_left_wide(uint8 d)
{
	if (bound_overlap(d,0,0,71,7))
	{
		lizard.flow = 1; // flow left
		if (zp.current_lizard != LIZARD_OF_STONE || lizard.power == 0)
		{
			lizard_die();
		}
	}
}
void dog_draw_lava_left_wide(uint8 d) {}

// DOG_LAVA_RIGHT_WIDE

void dog_init_lava_right_wide(uint8 d) {}
void dog_tick_lava_right_wide(uint8 d)
{
	if (bound_overlap(d,0,0,71,7))
	{
		lizard.flow = 2; // flow right
		if (zp.current_lizard != LIZARD_OF_STONE || lizard.power == 0)
		{
			lizard_die();
		}
	}
}
void dog_draw_lava_right_wide(uint8 d) {}

// DOG_LAVA_LEFT_WIDER

void dog_init_lava_left_wider(uint8 d) {}
void dog_tick_lava_left_wider(uint8 d)
{
	if (bound_overlap(d,0,0,119,7))
	{
		lizard.flow = 1; // flow left
		if (zp.current_lizard != LIZARD_OF_STONE || lizard.power == 0)
		{
			lizard_die();
		}
	}
}
void dog_draw_lava_left_wider(uint8 d) {}

// DOG_LAVA_RIGHT_WIDER

void dog_init_lava_right_wider(uint8 d) {}
void dog_tick_lava_right_wider(uint8 d)
{
	if (bound_overlap(d,0,0,119,7))
	{
		lizard.flow = 2; // flow right
		if (zp.current_lizard != LIZARD_OF_STONE || lizard.power == 0)
		{
			lizard_die();
		}
	}
}
void dog_draw_lava_right_wider(uint8 d) {}

// DOG_LAVA_DOWN

void dog_init_lava_down(uint8 d) {}
void dog_tick_lava_down(uint8 d)
{
	if (bound_overlap(d,0,0,7,23))
	{
		if (zp.current_lizard != LIZARD_OF_STONE || lizard.power == 0)
		{
			lizard_die();
			return;
		}
		else if (bound_overlap(d,3,0,4,23))
		{
			lizard.vx = 0;
			return;
		}
	}
}
void dog_draw_lava_down(uint8 d) {}

// DOG_LAVA_POOP

const int LAVA_POOP_FRAME = 0;
const int LAVA_POOP_CYCLE = 1;
const int LAVA_POOP_WAIT = 2;
void dog_init_lava_poop(uint8 d)
{
}
void dog_tick_lava_poop(uint8 d)
{
	// palette
	++dgd(LAVA_POOP_FRAME);
	if (dgd(LAVA_POOP_FRAME) >= 5)
	{
		dgd(LAVA_POOP_FRAME) = 0;
		++dgd(LAVA_POOP_CYCLE);
		if (dgd(LAVA_POOP_CYCLE) >= 3)
			dgd(LAVA_POOP_CYCLE) = 0;
		palette_load(7,DATA_palette_lava0 + dgd(LAVA_POOP_CYCLE));
	}

	if (dgd(LAVA_POOP_WAIT) > 0)
	{
		--dgd(LAVA_POOP_WAIT);
		return;
	}

	dgy += 4;
	if (dgy >= 144 && dgy < 240)
	{
		dgy = 240;
		dgd(LAVA_POOP_WAIT) = prng() & 31;
		return;
	}

	if (dgy > 240) return;

	if (bound_overlap_no_stone(d,-3,3,2,23))
	{
		lizard_die();
	}
}
void dog_draw_lava_poop(uint8 d)
{
	if (dgd(LAVA_POOP_WAIT) > 0) return;

	dx_scroll();

	// splash
	if (dgy >= 121 && dgy < 240)
	{
		sprite1_add(dx,0,DATA_sprite1_lava_poop_splash);
	}

	if (dx < 4) return;
	dx -= 4;

	sprite_tile_add(dx,dgy,0xD5,0x23);
	if (dgy < 136 || dgy >= 240) sprite_tile_add(dx,dgy+8,0xD6,0x23);
	if (dgy < 128 || dgy >= 240) sprite_tile_add(dx,dgy+16,0xD7,0x23);
}

// DOG_LAVA_MOUTH

const int MAX_DOWN_LAVA_MOUTH = 255;
const int MIN_DOWN_LAVA_MOUTH = -60;
const int ACCEL_LAVA_MOUTH = 65;

void dog_init_lava_mouth(uint8 d) {}
void dog_tick_lava_mouth(uint8 d)
{
	if (bound_overlap(d,0,0,31,(sint8)143))
	{
		if (lizard.power == 0 || zp.current_lizard != LIZARD_OF_STONE)
		{
			lizard_die();
			return;
		}

		if (lizard.vy >= MIN_DOWN_LAVA_MOUTH)
		{
			lizard.vy -= ACCEL_LAVA_MOUTH;
			if (lizard.vy >= MAX_DOWN_LAVA_MOUTH)
			{
				lizard.vy = MAX_DOWN_LAVA_MOUTH;
			}
		}
	}
}
void dog_draw_lava_mouth(uint8 d) {}

// ======
// BOSSES
// ======

const uint8 BOSS_TALK_TEXT[13] = {
	/*  0 */ TEXT_BUNNY0,
	/*  1 */ TEXT_BUNNY1,
	/*  2 */ TEXT_RACCOON0,
	/*  3 */ TEXT_RACCOON1,
	/*  4 */ TEXT_RACCOON2,
	/*  5 */ TEXT_RACCOON3,
	/*  6 */ TEXT_FROB0,
	/*  7 */ TEXT_BOSSTOPUS0,
	/*  8 */ TEXT_BOSSTOPUS1,
	/*  9 */ TEXT_CAT0,
	/* 10 */ TEXT_SNEK0,
	/* 11 */ TEXT_SNEK1,
};

const uint8 BOSS_TALK_OFFSET[6] = {
	 0, // BUNNY
	10, // SNEK
	 7, // BOSSTOPUS
	 2, // RACCOON
	 6, // FROB
	 9, // CAT
};

const uint8 BOSS_TALK_COUNT[6] = {
	2, // BUNNY
	2, // SNEK
	2, // BOSSTOPUS
	4, // RACCOON
	1, // FROB
	1, // CAT
};

bool dog_talk_test()
{
	return
		lizard.power != 0 &&
		zp.current_lizard == LIZARD_OF_KNOWLEDGE &&
		!lizard.dead;
}

void dog_boss_talk_execute(uint8 b)
{
	NES_ASSERT(b < 6, "dog_boss_talk_common boss index out of range.");

	uint8 offset = BOSS_TALK_OFFSET[b];
	uint8 count = BOSS_TALK_COUNT[b];

	if (h.boss_talk >= count) return;
	uint8 text = BOSS_TALK_TEXT[offset + h.boss_talk];
	++h.boss_talk;

	zp.game_message = text;
	zp.game_pause = 1;
	return;
}

void dog_boss_talk_common(uint8 b)
{
	if (dog_talk_test())
	{
		dog_boss_talk_execute(b);
	}
}

// DOG_BOSSTOPUS

void dog_init_bosstopus(uint8 d)
{
	// NOT IN DEMO
}
void dog_tick_bosstopus(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_bosstopus(uint8 d)
{
	// NOT IN DEMO
}

// DOG_BOSSTOPUS_EGG

void dog_init_bosstopus_egg(uint8 d)
{
	// NOT IN DEMO
}
void dog_tick_bosstopus_egg(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_bosstopus_egg(uint8 d)
{
	// NOT IN DEMO
}

// DOG_CAT

void dog_init_cat(uint8 d)
{
	// NOT IN DEMO
}
void dog_tick_cat(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_cat(uint8 d)
{
	// NOT IN DEMO
}

// DOG_CAT_SMILE

void dog_init_cat_smile(uint8 d)
{
	// NOT IN DEMO
}
void dog_tick_cat_smile(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_cat_smile(uint8 d)
{
	// NOT IN DEMO
}

// DOG_CAT_SPARKLE

void dog_init_cat_sparkle(uint8 d)
{
	// NOT IN DEMO
}
void dog_tick_cat_sparkle(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_cat_sparkle(uint8 d)
{
	// NOT IN DEMO
}

// DOG_CAT_STAR

void dog_init_cat_star(uint8 d)
{
	// NOT IN DEMO
}
void dog_tick_cat_star(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_cat_star(uint8 d)
{
	// NOT IN DEMO
}

// DOG_RACCOON

void dog_init_raccoon(uint8 d)
{
	// NOT IN DEMO
}
void dog_tick_raccoon(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_raccoon(uint8 d)
{
	// NOT IN DEMO
}

// DOG_RACCOON_LAUNCHER

void dog_init_raccoon_launcher(uint8 d)
{
	// NOT IN DEMO
}

void dog_tick_raccoon_launcher(uint8 d)
{
	// NOT IN DEMO
}

void dog_draw_raccoon_launcher(uint8 d)
{
	// NOT IN DEMO
}

// DOG_RACCOON_LAVABALL

void dog_init_raccoon_lavaball(uint8 d)
{
	// NOT IN DEMO
}

void dog_tick_raccoon_lavaball(uint8 d)
{
	// NOT IN DEMO
}

void dog_draw_raccoon_lavaball(uint8 d)
{
	// NOT IN DEMO
}

// DOG_RACCOON_VALVE

void dog_init_raccoon_valve(uint8 d)
{
	// NOT IN DEMO
}

void dog_tick_raccoon_valve(uint8 d)
{
	// NOT IN DEMO
}

void dog_draw_raccoon_valve(uint8 d)
{
	// NOT IN DEMO
}

// DOG_FROB

void dog_init_frob(uint8 d)
{
	// NOT IN DEMO
}

void dog_tick_frob(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_frob(uint8 d)
{
	// NOT IN DEMO
}

// DOG_FROB_HAND_LEFT
// DOG_FROB_HAND_RIGHT

void dog_init_frob_hand_left(uint8 d)
{
	// NOT IN DEMO
}
void dog_tick_frob_hand_left(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_frob_hand_left(uint8 d)
{
	// NOT IN DEMO
}

void dog_init_frob_hand_right(uint8 d)
{
	// NOT IN DEMO
}
void dog_tick_frob_hand_right(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_frob_hand_right(uint8 d)
{
	// NOT IN DEMO
}

// DOG_FROB_ZAP

void dog_init_frob_zap(uint8 d)
{
	// NOT IN DEMO
}

void dog_tick_frob_zap(uint8 d)
{
	// NOT IN DEMO
}

void dog_draw_frob_zap(uint8 d)
{
	// NOT IN DEMO
}

// DOG_FROB_TONGUE

void dog_init_frob_tongue(uint8 d)
{
	// NOT IN DEMO
}
void dog_tick_frob_tongue(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_frob_tongue(uint8 d)
{
	// NOT IN DEMO
}

// DOG_FROB_BLOCK

void dog_init_frob_block(uint8 d)
{
	// NOT IN DEMO
}
void dog_tick_frob_block(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_frob_block(uint8 d)
{
	// NOT IN DEMO
}

// DOG_FROB_PLATFORM

void dog_init_frob_platform(uint8 d)
{
	// NOT IN DEMO
}
void dog_tick_frob_platform(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_frob_platform(uint8 d)
{
	// NOT IN DEMO
}

// DOG_QUEEN

void dog_init_queen(uint8 d)
{
	// NOT IN DEMO
}
void dog_tick_queen(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_queen(uint8 d)
{
	// NOT IN DEMO
}

// DOG_HARE

const int HARE_ANIM   = 0;
const int HARE_MODE   = 1; // moment to moment mode
const int HARE_MOOD   = 2; // higher level behaviour control (IDLE state sequence)
const int HARE_Y1     = 3;
const int HARE_VY0    = 4;
const int HARE_VY1    = 5;
const int HARE_VX     = 6;
const int HARE_HEAT   = 7; // current accumulated heat
const int HARE_SPRITE = 8;
const int HARE_FLIP   = 9;
const int HARE_HP     = 10;
const int HARE_SIT    = 11; // time spent sitting
const int HARE_HOPS   = 12; // chase hop count

enum {
	HARE_MODE_IDLE      = 0,
	HARE_MODE_HOP       = 1,
	HARE_MODE_FLY       = 2,
	HARE_MODE_SKID      = 3,
	HARE_MODE_TURN      = 4,
	HARE_MODE_STUN      = 5,
	HARE_MODE_SIT       = 6,
	HARE_MODE_GET_UP    = 7,
	HARE_MODE_HIDE      = 8,
};

enum {
	HARE_MOOD_ENTER  = 0,
	HARE_MOOD_CHASE  = 1,
	HARE_MOOD_ESCAPE = 2,
	HARE_MOOD_ASCEND = 3,
	HARE_MOOD_STOMPL = 4,
	HARE_MOOD_STOMPR = 5,
};

const uint8 HARE_HP_MAX        = 4;
const uint8 HARE_FIRE_STRENGTH = 25;
const uint8 HARE_HEAT_STRENGTH = 8;

// hop types
// (current jump type stored in dog_param/dgp)
// 0 short hop (32 pixels)
// 1 long hop (77 pixels, 61+16 skid)
// 2 ascend hop (for reaching ledge during escape)
// 3 ascend followup (for jumping offscreen)
// 4 enter throw (falling from peak of arc on entrance)
// 5 faster short hop (for ice stomping or escape)

const int HARE_HOP_COUNT = 6;
const sint16 HARE_GRAVITY = 50;
const sint16 HARE_HOP_VY[HARE_HOP_COUNT] = {  -345,  -900, -1290,  -575,     0,  -345 }; // initial Y velocity
const sint8  HARE_HOP_VX[HARE_HOP_COUNT] = {     2,     2,     2,     2,     2,     2 }; // X speed (pixels per second)
const uint8  HARE_HOP_SK[HARE_HOP_COUNT] = {     0,    10,    16,     0,    10,     0 }; // frames of post-jump skid

const uint8  HARE_HOP_S0[HARE_HOP_COUNT] = {   255,     5,     8,   255,   255,   255 }; // frames of slight squat
const uint8  HARE_HOP_S1[HARE_HOP_COUNT] = {   255,    37,    40,   255,   255,   255 }; // frames of deep squat
const uint8  HARE_HOP_JP[HARE_HOP_COUNT] = {    28,    42,    48,     8,     8,     4 }; // frame of jump

const uint8 HARE_IDLE = 6; // frames to wait in idle
const uint8 HARE_TURN = 8; // frames in turn
const uint8 HARE_HIDE = 60; // frames in hide
const uint8 HARE_CHASE_TIMEOUT = 25; // hops before switching to escape mood

// range for the "escape" jump out (note: has to be less than length of small hop = 32)
const uint16 HARE_ESCAPE_L = 385;
const uint16 HARE_ESCAPE_R = 421;
const uint16 HARE_ESCAPE_M = (HARE_ESCAPE_L + HARE_ESCAPE_R) / 2;

// split point between short and long chasing hops (which are 32 and 95 pixels wide)
const uint16 HARE_SPLIT_L = 57+2;
const uint16 HARE_SPLIT_H = 95-2;
const uint16 HARE_SPLUT_L = (0xFFFF - HARE_SPLIT_L) + 1;
const uint16 HARE_SPLUT_H = (0xFFFF - HARE_SPLIT_H) + 1;

const uint16 HAREBURN_X = 68;
const uint8  HAREBURN_Y = 144;
const sint16 HAREBURN_W = 39;
const sint8  HAREBURN_H = 23;

void dog_harecicle_spawn(uint16 sx); // forward

void dog_init_hare(uint8 d)
{
	NES_ASSERT(d == HARE_SLOT, "hare may only be used in HARE_SLOT.");
	dgd(HARE_MODE) = HARE_MODE_HIDE;
	dgd(HARE_MOOD) = HARE_MOOD_ENTER;
}

bool dog_fly_hare(uint8 d) // returns true if collided with ground
{
	uint16 y16  = dgd_get16y(HARE_Y1);
	uint16 vy16 = dgd_get16(HARE_VY0, HARE_VY1);
	const uint16 yt16 = y16 + vy16;
	const uint8 yt8 = yt16 >> 8;

	vy16 += HARE_GRAVITY;
	dgd_put16(vy16,HARE_VY0,HARE_VY1);

	sint8 u = dgd(HARE_VX);
	sint8 v = yt8 - dgy;

	if (dgx >= 32 && dgx < 488)
	{
		move_dog(d,-14,-29,13,-1,&u,&v);
	}
	else
	{
		// just use X=511 for a one point Y collision test
		if (v > 0)
		{
			uint8 c = collide_all_down(511,yt8);
			v -= c;
		}
	}

	dgx += u;
	dgy += v;
	dgd(HARE_Y1) = (yt16 & 0xFF);

	if(dgy < yt8)
	{
		dgd(HARE_SPRITE) = DATA_sprite2_hare;
		return true;
	}

	// select sprite
	uint8 s = (dgd(HARE_ANIM) >> 2) & 1;
	if (dgd(HARE_MODE) == HARE_MODE_STUN)
	{
		CT_ASSERT(DATA_sprite2_hare_back1 == DATA_sprite2_hare_back0 + 1, "hare_back sprites out of order!");
		s += DATA_sprite2_hare_back0;
	}
	else if (dgd(HARE_VY0) >= 0x80)
	{
		CT_ASSERT(DATA_sprite2_hare_leap1 == DATA_sprite2_hare_leap0 + 1, "hare_leap sprites out of order!");
		s += DATA_sprite2_hare_leap0;
	}
	else
	{
		CT_ASSERT(DATA_sprite2_hare_fall1 == DATA_sprite2_hare_fall0 + 1, "hare_fall sprites out of order!");
		s += DATA_sprite2_hare_fall0;
	}
	dgd(HARE_SPRITE) = s;
	return false; // still flying
}

void dog_warm_hare(uint8 d, uint8 warm)
{
	uint8 w = dgd(HARE_HEAT) + warm;
	if (w < dgd(HARE_HEAT)) w = 255;
	dgd(HARE_HEAT) = w;
}

void dog_hit_hare(uint8 d)
{
	if (dgd(HARE_HP) < HARE_HP_MAX)
	{
		play_sound(SOUND_SAD);
		++dgd(HARE_HP);
	}
	if (dgd(HARE_HP) >= HARE_HP_MAX)
	{
		play_music(MUSIC_SILENT);
		dgd(HARE_HP) = HARE_HP_MAX;
	}
}

void dog_throw_hare(uint8 d, uint8 p)
{
	dgp = p;
	dgd(HARE_ANIM) = 0;
	dgd(HARE_MODE) = HARE_MODE_FLY;
	dgd(HARE_VX) = -2 + (dgd(HARE_FLIP) * 4); // according to flip dir
	dgd(HARE_Y1) = 0; // reset Y1
	uint16 vy = HARE_HOP_VY[p];
	if (dgd(HARE_MOOD) == HARE_MOOD_STOMPL || dgd(HARE_MOOD) == HARE_MOOD_STOMPR) // randomize roof jumps
	{
		vy -= prng() & 127;
	}
	dgd_put16(vy,HARE_VY0,HARE_VY1);
}

void dog_tick_hare(uint8 d)
{
	// diagnostic
	//PPU::debug_text("HARE: %3d,%3d M%d,%d HP%d:%3d A%d",int(dgx),int(dgy),int(dgd(HARE_MODE)),int(dgd(HARE_MOOD)),int(dgd(HARE_HP)),int(dgd(HARE_HEAT)),int(dgd(HARE_ANIM)));
	//PPU::debug_text("DTX: %d",int(::abs(dgx-lizard_px)));

	// heat normally dissipates
	if (dgd(HARE_HEAT))
	{
		--dgd(HARE_HEAT);
	}

	if (dgx < (HAREBURN_X + HAREBURN_W + 14 + 1) && dgy >= 144)
	{
		dog_warm_hare(d,HARE_FIRE_STRENGTH);
	}
	if (bound_touch(d,-14,-29,13,-1))
	{
		if (zp.current_lizard == LIZARD_OF_HEAT)
		{
			dog_warm_hare(d,HARE_HEAT_STRENGTH);
		}
		else if (zp.current_lizard == LIZARD_OF_DEATH)
		{
			play_music(MUSIC_SILENT);;
			bones_convert(d,DATA_sprite0_hare_skull,dgd(HARE_FLIP));
			return;
		}
	}
	if (bound_overlap_no_stone(d,-14,-29,13,-1))
	{
		lizard_die();
	}

	if (dgy >= 128 && dgd(HARE_MODE) != HARE_MODE_HIDE)
	{
		dog_boss_talk_common(0);
	}

	++dgd(HARE_ANIM);

	switch (dgd(HARE_MODE))
	{
	case HARE_MODE_IDLE:
		{
			// sit down if heat limit reached
			if (dgd(HARE_HEAT) >= 255)
			{
				dgd(HARE_ANIM) = 0;
				dgd(HARE_SIT) = 0; // reset sit timer
				dgd(HARE_MODE) = HARE_MODE_SIT;
				dog_hit_hare(d);
				goto mode_sit;
			}

			if (dgd(HARE_ANIM) < 2)
			{
				// on first frame of IDLE, realize the current MOOD

				dgd(HARE_ANIM) = 1; // don't do this twice if we somehow get here with 0

				// on first frame, set sprite, randomly as blink
				dgd(HARE_SPRITE) = DATA_sprite2_hare;
				if ((prng() & 3) == 0)
				{
					dgd(HARE_SPRITE) = DATA_sprite2_hare_blink;
				}

				// choose new target
				uint16 tx = 0;
				uint8 hop = 0;
				switch (dgd(HARE_MOOD))
				{
				default:
				case HARE_MOOD_ENTER:
					tx = 120;
					hop = 1;
					if (dgy < 176)
					{
						break; // run across top area
					}
					else
					{
						dgd(HARE_MOOD) = HARE_MOOD_CHASE;
						dgd(HARE_HOPS) = 0;
						// fall through
					}
				case HARE_MOOD_CHASE:
					if (dgd(HARE_HOPS) < HARE_CHASE_TIMEOUT)
					{
						++dgd(HARE_HOPS);
						tx = lizard_px;
						if (lizard.dead && tx < 103) // make it less common for hare to "suicide" when lizard is dead
						{
							tx = 103;
						}

						hop = 0;
						// use long hop in specific range
						{
							uint16 dtx = tx - dgx;
							if (
								(tx >= dgx && dtx >= HARE_SPLIT_L && dtx <  HARE_SPLIT_H) ||
								(tx <  dgx && dtx <  HARE_SPLUT_L && dtx >= HARE_SPLUT_H)
								)
							{
								hop = 1;
							}
						}
						break;
					}
					else
					{
						dgd(HARE_MOOD) = HARE_MOOD_ESCAPE;
						// fall through
					}
				case HARE_MOOD_ESCAPE:
					tx = HARE_ESCAPE_M;
					hop = 5;
					if (dgx < HARE_ESCAPE_L || dgx >= HARE_ESCAPE_R)
					{
						break;
					}
					else
					{
						dgd(HARE_MOOD) = HARE_MOOD_ASCEND;
						// fall through
					}
				case HARE_MOOD_ASCEND:
					tx = 512 + 100;
					hop = 2;
					if (dgy <= 120)
					{
						hop = 3;
					}
					break;
				case HARE_MOOD_STOMPL:
					tx = 120;
					hop = 5;
					if (dgx >= 120)
					{
						break;
					}
					else
					{
						dgd(HARE_MOOD) = HARE_MOOD_STOMPR;
						// fall through
					}
				case HARE_MOOD_STOMPR:
					tx = 512 + 100;
					hop = 5;
					break;
				}

				dgp = hop;

				// turn to face target
				if (
					(dgx <  tx && dgd(HARE_FLIP) == 0) ||
					(dgx >= tx && dgd(HARE_FLIP) != 0))
				{
					// enter turn immediately
					dgd(HARE_ANIM) = 0;
					dgd(HARE_MODE) = HARE_MODE_TURN;
					goto mode_turn;
				}
				return;

			}
			else if (dgd(HARE_ANIM) < HARE_IDLE)
			{
				// idle for a moment before doing anything
				return;
			}

			// IDLE proceeds to hop unless interrupted by MOOD above
			dgd(HARE_ANIM) = 0;
			dgd(HARE_MODE) = HARE_MODE_HOP;

			return;
		}
	case HARE_MODE_HOP:
		// sit down if heat limit reached
		if (dgd(HARE_HEAT) >= 255)
		{
			dgd(HARE_ANIM) = 0;
			dgd(HARE_SIT) = 0; // reset sit timer
			dgd(HARE_MODE) = HARE_MODE_SIT;
			dog_hit_hare(d);
			goto mode_sit;
		}

		NES_ASSERT(dgp < HARE_HOP_COUNT, "Invalid hop type for HARE_MODE_HOP?");
		if (dgd(HARE_ANIM) >= HARE_HOP_JP[dgp])
		{
			dog_throw_hare(d,dgp);
			play_sound(SOUND_HARE);
			return;
		}
		if (dgd(HARE_ANIM) < HARE_HOP_S0[dgp])
		{
			dgd(HARE_SPRITE) = DATA_sprite2_hare_squat0;
		}
		else if (dgd(HARE_ANIM) < HARE_HOP_S1[dgp])
		{
			// sound of paws touching ground
			if (dgd(HARE_ANIM) == HARE_HOP_S0[dgp])
			{
				play_sound(SOUND_GOAT_JUMP);
			}
			dgd(HARE_SPRITE) = DATA_sprite2_hare_squat1;
		}
		else
		{
			dgd(HARE_SPRITE) = DATA_sprite2_hare_squat0;
		}
		return;
	case HARE_MODE_FLY:
	mode_fly:
		if (dog_fly_hare(d))
		{
			dgd(HARE_MODE) = HARE_MODE_SKID;
			dgd(HARE_ANIM) = 0;

			if (dgy < 72)
			{
				dog_harecicle_spawn(dgx);
			}
		}
	evaluate_hide:
		if (dgx >= 16 && dgx < 40)
		{
			dgd(HARE_MODE) = HARE_MODE_HIDE;
			dgd(HARE_ANIM) = 0;
			if (dgd(HARE_MOOD) == HARE_MOOD_ASCEND)
			{
				dgd(HARE_MOOD) = HARE_MOOD_STOMPL;
			}
			else
			{
				dgd(HARE_MOOD) = HARE_MOOD_ENTER;
			}
		}
		return;
	case HARE_MODE_SKID:
		NES_ASSERT(dgp < HARE_HOP_COUNT, "hare dog_param invalid for HARE_MODE_SKID?");
		if (dgd(HARE_ANIM) >= HARE_HOP_SK[dgp])
		{
			dgd(HARE_ANIM) = 0;
			dgd(HARE_MODE) = HARE_MODE_IDLE;
		}
		else
		{
			sint8 u = 0;
			sint8 v = 0;
			if (dgd(HARE_FLIP) > 0) u =  1;
			else                    u = -1;
			move_dog(d,-14,-29,13,-1,&u,&v);
			dgx += u;
			dgy += v;

			CT_ASSERT(DATA_sprite2_hare_skid1 == DATA_sprite2_hare_skid0 + 1, "hare_skid sprites out of order!");
			uint8 s = (dgd(HARE_ANIM) >> 2) & 1;
			dgd(HARE_SPRITE) = s + DATA_sprite2_hare_skid0;

			goto evaluate_hide;
		}
		return;
	case HARE_MODE_TURN:
	mode_turn:
		// sit down if heat limit reached
		if (dgd(HARE_HEAT) >= 255)
		{
			dgd(HARE_ANIM) = 0;
			dgd(HARE_SIT) = 0; // reset sit timer
			dgd(HARE_MODE) = HARE_MODE_SIT;
			dog_hit_hare(d);
			goto mode_sit;
		}

		dgd(HARE_SPRITE) = DATA_sprite2_hare_turn;
		if (dgd(HARE_ANIM) == HARE_TURN)
		{
			dgd(HARE_FLIP) ^= 1;
		}
		else if (dgd(HARE_ANIM) >= (HARE_TURN*2))
		{
			dgd(HARE_ANIM) = 0;
			dgd(HARE_MODE) = HARE_MODE_IDLE;
		}
		return;
	case HARE_MODE_STUN:
		if (dog_fly_hare(d))
		{
			dgd(HARE_MODE) = HARE_MODE_SIT;
			dgd(HARE_ANIM) = 0;
			goto mode_sit;
		}
		return;
	case HARE_MODE_SIT:
	mode_sit:
		// don't sit in the fire
		if (dgx < (HAREBURN_X + HAREBURN_W + 14 + 1))
		{
			dog_throw_hare(d,0); // small hop trajectory
			dgd(HARE_FLIP) = 0; // face left
			dgd(HARE_VX) = 2; // throw right
			dgd(HARE_MODE) = HARE_MODE_STUN;
		}

		// chose sprite
		if (dgd(HARE_HP) >= HARE_HP_MAX)
		{
			// when finished heat stays permanent, use alternate sprite to free up a palette
			dgd(HARE_HEAT) = 255;
			dgd(HARE_SPRITE) = DATA_sprite2_hare_sit_final;
		}
		else
		{
			dgd(HARE_SPRITE) = DATA_sprite2_hare_sit;
		}

		if (dgd(HARE_HEAT) < 1)
		{
			dgd(HARE_ANIM) = 0;
			dgd(HARE_MODE) = HARE_MODE_GET_UP;
			dgd(HARE_MOOD) = HARE_MOOD_ESCAPE;
			return;
		}

		// has been held in heat for 4+ seconds? give it another hit
		if (dgd(HARE_HEAT) >= 196)
		{
			++dgd(HARE_SIT);
			if (dgd(HARE_SIT) == 0)
			{
				dog_hit_hare(d);
			}
		}
		else
		{
			dgd(HARE_SIT) = 0;
		}

		// select sprite
		if      (dgd(HARE_ANIM) <    2) dgd(HARE_SPRITE) = DATA_sprite2_hare_back0;
		else if (dgd(HARE_ANIM) <    4) dgd(HARE_SPRITE) = DATA_sprite2_hare_back1;
		else if (dgd(HARE_ANIM) ==   4) dgd(HARE_ANIM) = 4 + (prng() & 31); // randomize blink
		else if (dgd(HARE_ANIM) >= 167) dgd(HARE_ANIM) = 3;
		else if (dgd(HARE_ANIM) >= 160) dgd(HARE_SPRITE) = DATA_sprite2_hare_sit_blink;
		return;
	case HARE_MODE_GET_UP:
		if      (dgd(HARE_ANIM) <  5) dgd(HARE_SPRITE) = DATA_sprite2_hare_squat1;
		else if (dgd(HARE_ANIM) < 10) dgd(HARE_SPRITE) = DATA_sprite2_hare_squat0;
		else
		{
			dgd(HARE_ANIM) = 0;
			dgd(HARE_MODE) = HARE_MODE_IDLE;
		}
		return;
	case HARE_MODE_HIDE:
		if (dgd(HARE_ANIM) < HARE_HIDE) return;
		// jump in from the right side
		if (dgd(HARE_MOOD) == HARE_MOOD_STOMPL)
		{
			dgy = 56-18;
		}
		else
		{
			dgy = 120-18;
		}
		dgx = 512+15;
		dgd(HARE_FLIP) = 0;
		dog_throw_hare(d,4);
		goto mode_fly;
		return;
	default:
		NES_ASSERT(false, "Invalid HARE_MODE!");
		dgd(HARE_MODE) = HARE_MODE_HIDE;
		return;
	}
}

void dog_draw_hare(uint8 d)
{
	// shifting scrolling ooordinates by 16 allows the hare to slide off the right of by that much
	uint8 dx;
	uint16 dgx_adjust = (dgx - 16) & 0x1FF;
	if (dx_scroll_edge_(dx,dgx_adjust)) return;
	dx += 16;

	const uint8 m = dgd(HARE_MODE);
	if (m == HARE_MODE_HIDE)
	{
		// do not set palette, used by boss door
		return;
	}
	else
	{
		CT_ASSERT(DATA_palette_hare0 + 1 == DATA_palette_hare1, "hare palettes out of order!");
		CT_ASSERT(DATA_palette_hare0 + 2 == DATA_palette_hare2, "hare palettes out of order!");
		CT_ASSERT(DATA_palette_hare0 + 3 == DATA_palette_hare3, "hare palettes out of order!");
		CT_ASSERT(DATA_palette_hare0_eye + 1 == DATA_palette_hare1_eye, "hare palettes out of order!");
		CT_ASSERT(DATA_palette_hare0_eye + 2 == DATA_palette_hare2_eye, "hare palettes out of order!");
		CT_ASSERT(DATA_palette_hare0_eye + 3 == DATA_palette_hare3_eye, "hare palettes out of order!");

		const uint8 heat = dgd(HARE_HEAT) >> 6;
		NES_ASSERT(heat < 4,"out of range HARE_HEAT");

		if (dgd(HARE_HP) < HARE_HP_MAX) // if palette not needed by boss door
		{
			palette_load(6,DATA_palette_hare0_eye + heat);
		}

		palette_load(7,DATA_palette_hare0 + heat);
	}

	uint8 dy = dgy;

	if (dgd(HARE_FLIP) == 0) sprite2_add(        dx,dy,dgd(HARE_SPRITE));
	else                     sprite2_add_flipped(dx,dy,dgd(HARE_SPRITE));
}

// DOG_HARECICLE

const int HARECICLE_ANIM = 0;
const int HARECICLE_FLIP = 1; // random flip + palette attribute for sprite tile
// dgp = mode 0 = falling, 1 = collided

void dog_init_harecicle(uint8 d); // forward

void dog_harecicle_spawn(uint16 sx)
{
	const int MIN_HARECICLE_SLOT = 2;
	const int MAX_HARECICLE_SLOT = 10;
	
	uint8 d;
	for (d=MIN_HARECICLE_SLOT; d<MAX_HARECICLE_SLOT; ++d)
	{
		if (dog_type(d) == DOG_NONE) break;
	}
	if (d >= MAX_HARECICLE_SLOT)
	{
		NES_ASSERT(false,"out of slots for harecicle spawn!");
		return;
	}

	dgx = sx;
	// hare can be overlapped past the edge
	if (sx < 40) return;
	if (sx >= 512) return;

	dog_init_harecicle(d);
}

void dog_init_harecicle(uint8 d)
{
	dgt = DOG_HARECICLE;
	dgy = 66;
	dgd(HARECICLE_ANIM) = 0;
	dgd(HARECICLE_FLIP) = (prng() & 0x40) | 0x01;
	dgp = 0; // falling

	play_sound(SOUND_ICEFALL);
}

void dog_tick_harecicle(uint8 d)
{
	if (dgp == 0) // falling
	{
		dgy += 3;
		++dgd(HARECICLE_ANIM);

		if(bound_overlap_no_stone(d,-3,0,2,4))
		{
			lizard_die();
		}

		if (collide_tile(dgx,dgy+8))
		{
			dgp = 1;
			dgd(HARECICLE_ANIM) = 10;
		}
	}
	else // fallen
	{
		--dgd(HARECICLE_ANIM);
		if (dgd(HARECICLE_ANIM) < 1)
		{
			empty_dog(d);
		}
	}
}

void dog_draw_harecicle(uint8 d)
{
	dx_scroll_edge();

	uint8                             s = 0x7B;
	if      (dgd(HARECICLE_ANIM) <  5) s = 0x39;
	else if (dgd(HARECICLE_ANIM) < 10) s = 0x38;
	
	sprite_tile_add_clip(dx-4,dgy-1,s,dgd(HARECICLE_FLIP));
}

// DOG_HAREBURN

const int HAREBURN_ANIM  = 0;
const int HAREBURN_FRAME = 1;
const int HAREBURN_WAIT  = 2;

uint8 HAREBURN_PALETTE[6*2] = {
	DATA_palette_hare_fire0, DATA_palette_hare_fire1, DATA_palette_hare_fire2,
	DATA_palette_hare_fire0, DATA_palette_hare_fire1, DATA_palette_hare_fire2,
	DATA_palette_hare_fire_low0, DATA_palette_hare_fire_low1,
	DATA_palette_hare_fire_low0, DATA_palette_hare_fire_low1,
	DATA_palette_hare_fire_low0, DATA_palette_hare_fire_low1,
};

void dog_init_hareburn(uint8 d)
{
	NES_ASSERT(dog_type(HARE_SLOT) == DOG_HARE, "hareburn expects hare in HARE_SLOT");
	NES_ASSERT(dgx == HAREBURN_X && dgy == HAREBURN_Y, "hareburn location has moved? revise HAREBURN_X/Y constants.");

	dgd(HAREBURN_ANIM) = prng() & 7;
}
void dog_tick_hareburn(uint8 d)
{
	++dgd(HAREBURN_ANIM);
	if (dgd(HAREBURN_ANIM) >= 8)
	{
		dgd(HAREBURN_ANIM) = 0;
		++dgd(HAREBURN_FRAME);
		if (dgd(HAREBURN_FRAME) >= 6)
		{
			dgd(HAREBURN_FRAME) = 0;
		}
	}
	palette_load(3, HAREBURN_PALETTE[dgd(HAREBURN_FRAME)+0]);
	palette_load(2, HAREBURN_PALETTE[dgd(HAREBURN_FRAME)+6]);

	if (bound_overlap_no_stone(d,0,0,HAREBURN_W,HAREBURN_H))
	{
		lizard_die();
	}

	if (dog_type(HARE_SLOT) != DOG_HARE || dog_data(HARE_HP,HARE_SLOT) >= HARE_HP_MAX || FASTBOSS)
	{
		++dgd(HAREBURN_WAIT);
		if (dgd(HAREBURN_WAIT) >= 100)
		{
			flag_set(FLAG_BOSS_0_MOUNTAIN);
			h.human1_next = 0;
			if (dog_type(14) != DOG_BOSS_DOOR)
			{
				dog_type(14) = DOG_BOSS_DOOR;
				dog_init_boss_door_out(14,false);
			}
		}
	}
}
void dog_draw_hareburn(uint8 d)
{
}

//
// river dogs
//

//	RE_LOOP,
//	RE_ROCK0,
//	RE_ROCK1,
//	RE_ROCK2,
//	RE_ROCK3,
//	RE_ROCK4,
//	RE_ROCK5,
//	RE_ROCK6,
//	RE_ROCK7,
//	RE_LOG,
//	RE_DUCK,
//	RE_RAMP,
//	RE_RIVER_SEEKER,
//	RE_BARREL,
//	RE_WAVE,
//	RE_SNEK_LOOP,
//	RE_SNEK_HEAD,

const int RDC = 18;

//                                 LP,R0,R1,R2,R3,R4,R5,R6,R7,LG,DK,RP,SK,BR,WV,SL,SH,ST
const uint8 RIVER_EVENT_SP[RDC] = { 0, 4, 4, 4, 4, 4, 4, 4, 4, 2, 1, 4, 1, 4, 4, 0, 0, 0};
const uint8 RIVER_BOUND_X0[RDC] = { 0,16,16,15,16, 8, 8, 8, 6, 8,23,16,16,16,11,18,18, 8};
const uint8 RIVER_BOUND_Y0[RDC] = { 0, 7, 4, 3,22, 3, 3, 4, 6,19,10,16, 4, 6, 4, 4, 4, 4};
const uint8 RIVER_BOUND_X1[RDC] = { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,10,10, 1};
const uint8 RIVER_BOUND_Y1[RDC] = { 0, 1, 1, 1, 1, 1, 1, 1, 1, 9, 3, 1, 1, 1, 1, 1, 1, 1};
const uint8 RIVER_COVER_X0[RDC] = { 0,16,16,15,16, 8, 8, 8, 6, 8,23,16,16,16,11,18,18, 8};
const uint8 RIVER_COVER_Y0[RDC] = { 0,17, 9, 7,25, 8, 7, 8, 9,26,25,25,10,15,17,25,25,14};
const uint8 RIVER_COVER_X1[RDC] = { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 3};
const uint8 RIVER_COVER_Y1[RDC] = { 0, 8, 5, 4,23, 4, 4, 5, 7,20,11,17, 5, 7, 5, 5, 5, 5};
const uint8 RIVER_BOUND_ZH[RDC] = { 0, 8, 3, 2, 1, 3, 2, 2, 1, 4,13, 7, 4, 9, 6, 8, 8,10};
const uint8 RIVER_PAL_SLOT[RDC] = { 0, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 6, 7, 7, 7, 6};
const uint8 RIVER_PAL_USED[RDC] =
{
	0,
	DATA_palette_rock,
	DATA_palette_rock,
	DATA_palette_rock,
	DATA_palette_rock,
	DATA_palette_rock,
	DATA_palette_rock,
	DATA_palette_rock,
	DATA_palette_rock,
	DATA_palette_river_wood,
	DATA_palette_duck, // also needs DATA_palette_duck_eye in 6
	DATA_palette_duck,
	DATA_palette_coin,
	DATA_palette_river_wood,
	DATA_palette_wave,
	DATA_palette_snek,
	DATA_palette_snek,
	DATA_palette_snek_tail,
};
// SH needs adjustment
// LG needs to be applied 3x (with 2 adjustments)
// SL has a second part
// WV needs a bunch of adjustments

void dog_init_river_common(uint8 d)
{
	zero_dog_data(d);

	dgx = 256 + 24;

	uint8 ps = RIVER_PAL_SLOT[dgp];
	if (ps != 0)
	{
		palette_load(ps,RIVER_PAL_USED[dgp]);
	}
}

bool dog_overlap_river_common(uint8 d)
{
	uint16 x0 = dgx - RIVER_BOUND_X0[dgp];
	uint16 x1 = dgx - RIVER_BOUND_X1[dgp];
	if (x0 >= 0x8000) x0 = 0;
	uint8 y0 = uint8(dgy) - RIVER_BOUND_Y0[dgp];
	uint8 y1 = uint8(dgy) - RIVER_BOUND_Y1[dgp];
	return lizard_overlap(x0,y0,x1,y1);
}

bool dog_cover_river_common(uint8 d)
{
	const uint8 dy0 = RIVER_COVER_Y0[dgp];
	const uint8 dy1 = RIVER_COVER_Y1[dgp];
	uint8 dz = (dy0 - dy1);
	if (dz < lizard_pz)
	{
		return false;
	}

	uint16 x0 = dgx - RIVER_COVER_X0[dgp];
	uint16 x1 = dgx - RIVER_COVER_X1[dgp];
	if (x0 >= 0x8000) x0 = 0;
	uint8 y0 = uint8(dgy) - dy0;
	uint8 y1 = uint8(dgy) - dy1;
	return lizard_overlap(x0,y0,x1,y1);
}

void dog_tick_river_common(uint8 d)
{
	uint8 speed = RIVER_EVENT_SP[dgp];
	if (dgx < speed)
	{
		dgx -= speed;
		empty_dog(d);
		return;
	}
	dgx -= speed;

	if (lizard_pz < RIVER_BOUND_ZH[dgp] &&
		dog_overlap_river_common(d))
	{
		lizard_die();
	}
	else if (lizard_py < dgy &&
		dog_cover_river_common(d))
	{
		dog_data(RIVER_OVERLAP,RIVER_SLOT) = 1;
	}
}

void dog_draw_river_common(uint8 d, uint8 sprite)
{
	dx_scroll_river();
	sprite1_add(dx,dgy,sprite);
}

void dog_init_rock(uint8 d) { dog_init_river_common(d); }
void dog_tick_rock(uint8 d) { dog_tick_river_common(d); }
void dog_draw_rock(uint8 d)
{
	CT_ASSERT(DATA_sprite1_rock1 == DATA_sprite1_rock0 + 1, "Rock sprites out of order.");
	CT_ASSERT(DATA_sprite1_rock2 == DATA_sprite1_rock0 + 2, "Rock sprites out of order.");
	CT_ASSERT(DATA_sprite1_rock3 == DATA_sprite1_rock0 + 3, "Rock sprites out of order.");
	CT_ASSERT(DATA_sprite1_rock4 == DATA_sprite1_rock0 + 4, "Rock sprites out of order.");
	CT_ASSERT(DATA_sprite1_rock5 == DATA_sprite1_rock0 + 5, "Rock sprites out of order.");
	CT_ASSERT(DATA_sprite1_rock6 == DATA_sprite1_rock0 + 6, "Rock sprites out of order.");
	CT_ASSERT(DATA_sprite1_rock7 == DATA_sprite1_rock0 + 7, "Rock sprites out of order.");

	dog_draw_river_common(d,DATA_sprite1_rock0 + (dgp - RE_ROCK0));
}

// DOG_LOG

const int LOG_ANIM  = 0;
const int LOG_FRAME = 1;

void dog_init_log(uint8 d)
{
	// NOT IN DEMO
}

void dog_tick_log(uint8 d)
{
	// NOT IN DEMO
}

void dog_draw_log(uint8 d)
{
	// NOT IN DEMO
}

// DOG_DUCK

void dog_init_duck(uint8 d)
{
	// NOT IN DEMO
}

void dog_tick_duck(uint8 d)
{
	// NOT IN DEMO
}

void dog_draw_duck(uint8 d)
{
	// NOT IN DEMO
}

// DOG_RAMP

void dog_init_ramp(uint8 d)
{
	// NOT IN DEMO
}
void dog_tick_ramp(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_ramp(uint8 d)
{
	// NOT IN DEMO
}

// DOG_RIVER_SEEKER

void dog_init_river_seeker(uint8 d)
{
	// NOT IN DEMO
}
void dog_tick_river_seeker(uint8 d)
{
	// NOT IN DEMO
}
void dog_draw_river_seeker(uint8 d)
{
	// NOT IN DEMO
}

// DOG_BARREL

void dog_init_barrel(uint8 d)
{
	// NOT IN DEMO
}

void dog_tick_barrel(uint8 d)
{
	// NOT IN DEMO
}

void dog_draw_barrel(uint8 d)
{
	// NOT IN DEMO
}

// DOG_WAVE

void dog_init_wave(uint8 d)
{
	// NOT IN DEMO
}

void dog_tick_wave(uint8 d)
{
	// NOT IN DEMO
}

void dog_draw_wave(uint8 d)
{
	// NOT IN DEMO
}

// DOG_SNEK_LOOP

void dog_init_snek_loop(uint8 d)
{
	// NOT IN DEMO
}

void dog_tick_snek_loop(uint8 d)
{
	// NOT IN DEMO
}

void dog_draw_snek_loop(uint8 d)
{
	// NOT IN DEMO
}

// DOG_SNEK_HEAD

void dog_init_snek_head(uint8 d)
{
	// NOT IN DEMO
}

void dog_tick_snek_head(uint8 d)
{
	// NOT IN DEMO
}

void dog_draw_snek_head(uint8 d)
{
	// NOT IN DEMO
}

// DOG_SNEK_TAIL

void dog_init_snek_tail(uint8 d)
{
	// NOT IN DEMO
}

void dog_tick_snek_tail(uint8 d)
{
	// NOT IN DEMO
}

void dog_draw_snek_tail(uint8 d)
{
	// NOT IN DEMO
}

// DOG_RIVER_LOOP

void dog_init_river_loop(uint8 d)
{
	// exit on first loop for demo
	dgy = 2;

	dog_param(RIVER_SLOT) = 0; // reset SNEK sequence
	if (dgy == 0)
	{
		// execute loop point
		dog_data(RIVER_SEQ0,RIVER_SLOT) = dog_data(RIVER_LOOP0,RIVER_SLOT);
		dog_data(RIVER_SEQ1,RIVER_SLOT) = dog_data(RIVER_LOOP1,RIVER_SLOT);
	}
	else if (dgy == 2)
	{
		// boss has been defeated
		play_music(MUSIC_SILENT);
		play_bg_noise(0xA,0x2);
		dog_data(RIVER_SEQ_TIME,RIVER_SLOT) = 255;
		dgp = 0; // parameter used for timeout
	}
	else // (dgy == 1)
	{
		// set loop point to start boss round
		play_music(MUSIC_BOSS);
		dog_data(RIVER_LOOP0,RIVER_SLOT) = dog_data(RIVER_SEQ0,RIVER_SLOT);
		dog_data(RIVER_LOOP1,RIVER_SLOT) = dog_data(RIVER_SEQ1,RIVER_SLOT);
	}
}
void dog_tick_river_loop(uint8 d)
{
	if (dgy != 2)
	{
		empty_dog(d);
		return;
	}

	dog_data(RIVER_SEQ_TIME,RIVER_SLOT) = 255; // prevent further events
	++dgp;
	if (dgp >= 200)
	{
		dgp = 200;
		if (lizard.dead == 0)
		{
			//flag_set(FLAG_BOSS_1_RIVER); // disabled for dinner
			h.human1_next = 1;
			zp.room_change = 1;
			zp.current_door = 1;
			dog_init_boss_door_out_partial(false);
			zp.current_room = h_door_link[zp.current_door];
		}
	}
}
void dog_draw_river_loop(uint8 d) {}

// DOG_WATT

void dog_init_watt(uint8 d)
{
	// NOT IN DEMO
}

void dog_tick_watt(uint8 d)
{
	// NOT IN DEMO
}

void dog_draw_watt(uint8 d)
{
	// NOT IN DEMO
}

// DOG_WATERFALL

void dog_init_waterfall(uint8 d)
{
	// empty
}

void dog_tick_waterfall(uint8 d)
{
	// swimming up a waterfall
	if (zp.current_lizard != LIZARD_OF_SWIM) return;
	if (lizard.power != 1) return;

	// left and top edge
	if (lizard_px < dgx) return;
	if (lizard_py < dgy) return;

	if (dgp != 0) // if param > 0 it has a right and bottom edge as well
	{
		if (lizard_py >= zp.water) return;
		if ((lizard_px - dgx) >= 25) return;
	}

	lizard.vy = -550;
}

void dog_draw_waterfall(uint8 d)
{
	// empty
}

//
// BANK2 ($F)
//

// DOG_TIP

void dog_init_tip(uint8 d)
{
	NES_ASSERT(d == TIP_SLOT, "tip not in TIP_SLOT!");
	NES_ASSERT(h.tip_index < TIP_MAX + 1, "tip out of range");
	CT_ASSERT(TEXT_TIP0 + 1 == TEXT_TIP1, "TEXT_TIP1 out of order");
	CT_ASSERT(TEXT_TIP0 + 2 == TEXT_TIP2, "TEXT_TIP2 out of order");
	CT_ASSERT(TEXT_TIP0 + 3 == TEXT_TIP3, "TEXT_TIP3 out of order");
	CT_ASSERT(TEXT_TIP0 + 4 == TEXT_TIP4, "TEXT_TIP4 out of order");
	CT_ASSERT(TEXT_TIP0 + 5 == TEXT_TIP5, "TEXT_TIP5 out of order");
	CT_ASSERT(TEXT_TIP0 + 6 == TEXT_TIP6, "TEXT_TIP6 out of order");
	CT_ASSERT(TIP_MAX <= 6, "extra TEXT_TIPs have been added?");
	if (h.tip_index >= (TIP_MAX+1)) h.tip_index = 0; // safety

	uint8 line_count = 0;

	if (dgp != 0)
	{
		h.tip_index = TIP_MAX; // param=1 forces ending text
		h.tip_return_door = 1;
		h.tip_return_room = h_door_link[1];
	}
	else // ending text skips header
	{
		text_start(TEXT_TIP_HEADER);
		while (h.text_more)
		{
			++line_count;
			text_continue();
		}
	}

	text_start(TEXT_TIP0 + h.tip_index);
	while (h.text_more)
	{
		++line_count;
		text_continue();
	}

	// centre
	PPU::latch_at(0,15 - (line_count >> 1));

	// draw text
	if (dgp == 0) // ending text skips header
	{
		text_start(TEXT_TIP_HEADER);
		while (h.text_more)
		{
			text_continue();
			ppu_nmi_write_row();
		}
	}

	text_start(TEXT_TIP0 + h.tip_index);
	while (h.text_more)
	{
		text_continue();
		ppu_nmi_write_row();
	}

	text_load(TEXT_BLANK);
	ppu_nmi_write_row();
	text_load(TEXT_UNPAUSE);
	ppu_nmi_write_row();

	++h.tip_index;
}

void dog_tick_tip(uint8 d)
{
}

void dog_draw_tip(uint8 d)
{
}

// DOG_WIQUENCE

void dog_init_wiquence(uint8 d)
{
	// NOT IN DEMO
}

void dog_tick_wiquence(uint8 d)
{
	// NOT IN DEMO
}

void dog_draw_wiquence(uint8 d)
{
	// NOT IN DEMO
}

// DOG_WITCH

void dog_init_witch(uint8 d)
{
	// NOT IN DEMO
}

void dog_tick_witch(uint8 d)
{
	// NOT IN DEMO
}

void dog_draw_witch(uint8 d)
{
	// NOT IN DEMO
}

// DOG_BOOK

void dog_init_book(uint8 d)
{
	// NOT IN DEMO
}

void dog_tick_book(uint8 d)
{
	// NOT IN DEMO
}

void dog_draw_book(uint8 d)
{
	// NOT IN DEMO
}

//
// Public stuff
//

#include "dogs_tables.h" // function pointer tables

void dog_init(uint8 d)
{
	zp.dog_now = d;
	uint8 t = dog_type(d);
	if (t >= DOG_COUNT)
	{
		NES_ASSERT(false,"Dog type out of range!");
		dog_init_none(d);
	}
	else
	{
		dog_init_table[t](d);
	}
}

void dogs_tick()
{
	dogs_cycle();

	uint8 d = 0;
	do
	{
		zp.dog_now = d;
		uint8 t = dog_type(d);
		if (t >= DOG_COUNT)
		{
			NES_ASSERT(false,"Dog type out of range!");
			dog_tick_none(d);
		}
		else
		{
			dog_tick_table[t](d);
		}

		// prevent multiple doors from acting at once (causing conflicting movement)
		if (zp.room_change != 0) break;

		d = (d + zp.dog_add) & 15;
	} while (d != 0);

	// wrap
	for (int i=0; i<16; ++i)
	{
		if (zp.room_scrolling)
		{
			dog_x(i) &= 0x01FF;
		}
		else
		{
			dog_x(i) &= 0x00FF;
		}
	}

	if (debug_dogs)
	{
		for (uint8 d=0; d<16; ++d)
		{
			PPU::debug_text("%2d: %3d  %3d,%3d",d,dog_type(d),uint32(dog_x(d)),uint32(dog_y(d)));
		}
	}
}

void dogs_draw()
{
	uint8 d = 0;
	do
	{
		zp.dog_now = d;

		uint8 t = dog_type(d);
		if (t >= DOG_COUNT)
		{
			NES_ASSERT(false,"Dog type out of range!");
			dog_draw_none(d);
		}
		else
		{
			dog_draw_table[t](d);
		}

		d = (d + zp.dog_add) & 15;
	} while (d != 0);
}

} // namespace Game
