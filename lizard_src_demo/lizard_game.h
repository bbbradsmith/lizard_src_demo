#pragma once
// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#if !defined(__APPLE__)
// presuming C++11 is available on all platforms except Mac
#define CPP_11
#else
// Mac is using GCC 4.2 with C99 extensions on
#define CPP_99
#endif

#if defined(CPP_11)
// C++11 headers
#include <cstdint>
#include <type_traits>
#elif defined(CPP_99)
// C99 headers
#include <stdint.h>
#else
#error C++11 or C99 extensions required.
#endif

#include <cstddef> // for NULL

#include <limits>
typedef uint8_t  uint8;
typedef int8_t   sint8;
typedef uint16_t uint16;
typedef int16_t  sint16;
typedef uint32_t uint32;
typedef int32_t  sint32;

const int PATH_LEN = 1024; // maximum allowed path length

// helper class for packing 16 bit arrays as two 8 bit arrays striped
class Array16
{
private:
	uint8 i;
	uint8* al;
	uint8* ah;

public:
	Array16() : i(0), al(NULL), ah(NULL) {}

	void init(uint8 i_, uint8* al_, uint8* ah_)
	{
		i = i_;
		al = al_;
		ah = ah_;
	}
	uint16 get_value() const
	{
		return uint16(al[i]) | uint16(ah[i] << 8);
	}
	void set_value(uint16 v) {
		al[i] = (v & 0xFF);
		ah[i] = (v >> 8) & 0xFF;
	}

	operator uint16() const { return get_value(); } // fetch value through implicit conversion operator
	Array16& operator =(uint16 v) { set_value(v); return *this; }
	Array16& operator =(Array16 v) { set_value(v.get_value()); return *this; } // prevent copy from sibling
	Array16& operator +=(int v) { set_value(get_value() + v); return *this; }
	Array16& operator -=(int v) { set_value(get_value() - v); return *this; }
	Array16& operator ++() { set_value(get_value() + 1); return *this; }
	Array16& operator --() { set_value(get_value() - 1); return *this; }
	Array16& operator &=(uint16 v) { set_value(get_value() & v); return *this; }
	Array16& operator |=(uint16 v) { set_value(get_value() | v); return *this; }

	static void generate(Array16 a[], uint8* al_, uint8* ah_, uint8 size)
	{
		for (int j=0; j<size; ++j) a[j].init(j,al_,ah_);
	}
};


namespace Game
{

enum ModeEnum
{
	GAME_TITLE,
	GAME_START,
	GAME_PLAY,
	GAME_RIVER,
	GAME_FADE_OUT,
	GAME_LOAD,
	GAME_FADE_IN,
	GAME_TALK,
	GAME_PAUSE_IN,
	GAME_PAUSE,
	GAME_PAUSE_OUT,
	GAME_HOLD,
	GAME_ENDING,
	GAME_BOOK,
	GAME_SOUNDTRACK,
	GAME_CRASH,
};

// system functions
extern void init();
extern void tick(uint8 input);
extern void set_option_mode(bool o);
extern bool get_option_mode();
extern bool get_suspended();
extern void set_debug(bool d);
extern void set_debug_dogs(bool d);
extern void resume_point(bool dead=false);
extern void convert_time(int src_fps, int dest_fps);

// game functions exposed to public
extern void nmi(); // usually called by tick
extern void palette_load(unsigned int slot, unsigned int select);
extern void chr_load(unsigned int slot, unsigned int select);
extern void room_load(); // loads zp.current_room
extern void room_load_partial(uint8 row);

// use dx_scroll_edge, dx_scroll, or dx_screen to set zp.sprite_offscreen before calling sprite add functions
// (sprite_tile_add is exempt, but sprite_tile_add_clip is not)
extern void sprite_prepare(uint8 x);
extern void sprite0_add(uint8 x, uint8 y, uint8 sprite);
extern void sprite0_add_flipped(uint8 x, uint8 y, uint8 sprite);
extern void sprite1_add(uint8 x, uint8 y, uint8 sprite);
extern void sprite1_add_flipped(uint8 x, uint8 y, uint8 sprite);
extern void sprite2_add(uint8 x, uint8 y, uint8 sprite);
extern void sprite2_add_flipped(uint8 x, uint8 y, uint8 sprite);
extern void sprite_tile_add(uint8 x, uint8 y, uint8 tile, uint8 att);
extern void sprite_tile_add_clip(uint8 x, uint8 y, uint8 tile, uint8 att);

extern void nmi_update_text(const char* msg);
extern void set_game_mode(uint8 mode);
extern uint8 get_game_mode();

// collision
extern void collide_set_tile(uint8 x, uint8 y); // y = 0-31, x unbounded (discarded if >=64)
extern void collide_clear_tile(uint8 x, uint8 y);  // y = 0-31, x unbounded (discarded if >=64)
extern bool collide_tile(uint16 x, uint8 y);
extern bool collide_all(uint16 x, uint8 y);
extern uint8 collide_tile_left(uint16 x, uint8 y);
extern uint8 collide_tile_right(uint16 x, uint8 y);
extern uint8 collide_tile_up(uint16 x, uint8 y);
extern uint8 collide_tile_down(uint16 x, uint8 y);
extern uint8 collide_all_left(uint16 x, uint8 y);
extern uint8 collide_all_right(uint16 x, uint8 y);
extern uint8 collide_all_up(uint16 x, uint8 y);
extern uint8 collide_all_down(uint16 x, uint8 y);

// misc
extern uint8 prng();
extern uint8 prng(int count);
extern bool coin_read(uint8 c);
extern void coin_take(uint8 c);
extern void coin_return(uint8 c);
extern uint8 coin_count();
extern bool flag_read(uint8 f);
extern void flag_set(uint8 f);
extern void flag_clear(uint8 f);
extern void decimal_clear();
extern void decimal_add(uint8 x);
extern void decimal_add32(uint32 x);
extern void decimal_print();
extern void metric_print();
extern void dogs_cycle();

extern void nmi_update_at(uint8 x, uint8 y);
extern void ppu_nmi_update_row();
extern void ppu_nmi_update_double();
extern void ppu_nmi_write_row();

// password functions
extern void password_build();
extern bool password_read(uint16* read_room, uint8* read_lizard);
extern bool checkpoint(uint16 room);

// NES RAM data

const int MUSIC_RESERVE = 128;

struct Lizard
{
	uint32 x; // big-endian on NES
	uint16 y; // big-endian on NES
	uint16 z; // big-endian on NES
	sint16 vx; // big-endian on NES
	sint16 vy; // big-endian on NES
	sint16 vz; // big-endian on NES

	uint8 flow; // 0 = none, 1 = slide left, 2 = slide right
	uint8 face; // 0 = right, 1 = left
	uint8 fall;
	uint8 jump;
	uint8 skid;
	uint8 dismount;
	uint16 anim; // big-endian on NES
	uint8 scuttle;
	uint8 dead;
	uint8 power;
	uint8 wet;
	uint8 big_head_mode;
	uint8 moving; // animation timeout for moving

	uint16 hitbox_x0; // hitbox (updated by lizard_tick
	uint8  hitbox_y0;
	uint16 hitbox_x1;
	uint8  hitbox_y1;
	uint16 touch_x0; // touchbox for fire/death
	uint8  touch_y0;
	uint16 touch_x1;
	uint8  touch_y1;
};

#define lizard_px (uint16(lizard.x>>8))
#define lizard_py (uint8(lizard.y>>8))
#define lizard_pz (uint8(lizard.z>>8))

struct NES_ZP
{
	//
	// NES Zero Page (256 bytes)
	//

	uint8  i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,tb;
	uint8  ih,jh,kh,lh;

	uint16 temp_ptr0;
	uint16 temp_ptr1;
	uint16 temp_ptrn; // for NMI

	uint8  nmi_count; // increment every frame
	uint8  nmi_lock; // prevents NMI re-entry
	uint8  nmi_ready; // see NMIEnum
	uint8  nmi_next; // dog tick can set this to an NMIEnum to push a nametable update at the end of the frame
	uint16 nmi_load; // PPU load address for NMI update
	uint16 scroll_x;
	uint8  overlay_scroll_y; // scroll start position for overlay
	uint8  mode_temp; // temporary variable used by game modes
	uint8  oam_pos; // meta sprite write position
	uint8  chr_cache[8]; // cached CHR
	uint8  ending; // (PC only, flags ending)
	uint8  continued; // 1 if password door or save stone has been used since reset
	uint8  coin_saved; // 1 if coin_save/flag_save matches coin/flag (also piggy_bank)
	uint8  room_scrolling;
	uint8  sprite_center; // 0x80 = (dx & 0x80) or flipped if offscreen, 0x40 = true if in center 50% of screen

	uint8  easy; // easy mode (FF = init, 0 = no, otherwise frame counter for easy mode)
	uint8  game_pause; // flag to pause game for message etc.(see text_select below)
	uint8  game_message; // message to use when paused (see text_select below)
	uint8  room_change; // flag to indicate room change 2=change by door (flip), 1=change by pass
	uint16 current_room;
	uint8  current_door;
	uint8  current_lizard;
	uint8  next_lizard;
	uint8  hold_x; // hold y/vx for pass_x
	uint8  hold_y; // hold x/vy for pass_y
	uint8  human0_pal; // randomly assigned on startup

	uint8  dog_add_select;
	uint8  dog_add;
	uint8  dog_now;
	uint8  att_or;

	uint8  water;

	uint8  gamepad; // current input state

	uint16 seed; // prng random seed
	uint8  password[5];

	uint16 smoke_x;
	uint8  smoke_y;

	uint8  climb_assist_time;
	uint8  climb_assist;

	// due to alignment and packing, these spaces don't exactly match anyway
	// the size constraints are maintained via the NES port
	//uint8  lizard_padding[sizeof(Lizard)];
	//uint8  music_padding[MUSIC_RESERVE];

	uint8  zp_padding[2]; // for NES crash diagnostic
};

struct NES_STACK
{
	//
	// NES stack is 256 bytes, but lower portion of it is used for NMI and scratch
	//

	// this data is stored on the stack
	uint8 nmi_update[64];
	uint8 scratch[64];

	// keep this for the true stack
	uint8 stack_pad[128];
};

struct NES_RAM
{
	//
	// NES RAM (256 * 6 bytes = 1536 bytes)
	//

public:
	uint8  oam[256];
	uint8  collision[256];

	uint8  dog_data[14][16]; // 256 byte page aligned on NES (for faster indexing)

	uint8  dog_param[16];
	uint8  dog_type[16];
	uint8  dog_y[16];

	// 16 bit striped arrays protected to prevent accidental access (use Array16)
protected:
	uint8  dog_xh[16];
	uint8  dog_x[16];
	uint8  blocker_xh0[4];
	uint8  blocker_x0[4];
	uint8  blocker_xh1[4];
	uint8  blocker_x1[4];
	uint8  door_xh[8];
	uint8  door_x[8];
	uint8  door_linkh[8];
	uint8  door_link[8];
public:

	uint8  door_y[8];

	uint8  att_mirror[128];
	uint8  palette[32];
	uint8  shadow_palette[32];
	uint8  blocker_y0[4];
	uint8  blocker_y1[4];

	uint8  coin[16];
	uint8  coin_save[16];
	uint8  flag[16];
	uint8  flag_save[16];
	uint8  piggy_bank;
	uint8  piggy_bank_save;
	uint8  last_lizard;
	uint8  last_lizard_save;

	uint8  human1_pal;
	uint8  human0_hair;
	uint8  human1_hair;
	uint8  moose_text;
	uint8  moose_text_inc;
	uint8  heep_text;
	uint8  human1_set[6];
	uint8  human1_het[6]; // hair set
	uint8  human1_next;

	uint8  decimal[5];

	uint8  metric_time_h;
	uint8  metric_time_m;
	uint8  metric_time_s;
	uint8  metric_time_f;
	uint32 metric_bones;
	uint32 metric_jumps;
	uint32 metric_continue; // number of times password continue has been used
	uint8  metric_cheater;

	uint16 diagnostic_room;
	uint8 bounced; // set to 1 whenever lizard bounces (must be cleared by reader)
	uint8 frogs_fractioned;
	uint8 wave_draw_control; // bit 7 = waves drawn this frame, bit 6 = reverse order of waves
	uint8 text_select; // 0=talk, 1=message, 2=pause
	uint8 text_more; // 0=finished, 1=more
	uint16 text_ptr; // NES only (filled with "fake" values here)
	uint8 boss_talk; // talking boss counter
	uint8 tip_index; // tip system counts consecutive bones between saves, displays helpful text
	uint8 tip_counter;
	uint8 tip_return_door;
	uint16 tip_return_room;
	uint8 boss_rush; // flag for boss rush mode
	uint8 end_book; // flag for book ending
	uint8 end_verso; // flag for verso ending

	friend int array16_init();
};

// saved state for resume
struct Resume
{
	bool valid;

	// system state
	uint8 pal;
	uint8 easy;

	// ZP state
	uint8  continued;
	uint16 seed;
	uint8 password[5];
	uint8  human0_pal;

	// RAM state
	uint8  coin[16];
	uint8  flag[16];
	uint8  piggy_bank;
	uint8  last_lizard;
	uint8  human1_pal;
	uint8  human0_hair;
	uint8  human1_hair;
	uint8  moose_text;
	uint8  moose_text_inc;
	uint8  heep_text;
	uint8  human1_set[6];
	uint8  human1_het[6];
	uint8  metric_time_h;
	uint8  metric_time_m;
	uint8  metric_time_s;
	uint8  metric_time_f;
	uint32 metric_bones;
	uint32 metric_jumps;
	uint32 metric_continue;
	uint8  metric_cheater;
	uint8 frogs_fractioned;
	uint8 tip_index;
	uint8 tip_counter;
};

extern struct NES_ZP zp;
extern struct NES_STACK stack;
extern struct NES_RAM h;
extern struct Lizard lizard; // actually part of ZP (accounted for by padding)

extern struct Resume resume;

// 16 bit striped arrays

extern Array16 h_dog_x      [16];
extern Array16 h_blocker_x0 [ 4];
extern Array16 h_blocker_x1 [ 4];
extern Array16 h_door_x     [ 8];
extern Array16 h_door_link  [ 8];

extern int ntsc_phase;

extern bool debug;
extern bool debug_dogs;

}

// common C++ and debugging helpers

// platform specific stuff, see lizard_win32.cpp
extern void alert(const char* text, const char* caption);
extern void debug_break();
extern void debug_msg(const char* msg, ...);

extern char* string_cpy(char* dest, const char* src, unsigned int len);
extern char* string_cat(char* dest, const char* src, unsigned int len);
extern char char_lower(char x);
extern bool string_less(const char* a, const char* b); // case insensitive < operator
extern unsigned int string_len(const char* src);
extern int string_icmp(const char* s1, const char* s2);

#ifdef _DEBUG
#define NES_ASSERT(c,text) { if (!(c)) { alert(text,"Debug assert!"); debug_break(); } }
#define NES_DEBUG(...) { debug_msg(__VA_ARGS__); }
#else
#define NES_ASSERT(c,text) {}
#define NES_DEBUG(...) {}
#endif

#define IS_POWER_OF_TWO(x) (0==((x)&((x)-1)))
#define ELEMENTS_OF(x) (sizeof(x)/sizeof(x[0]))

#if defined(CPP_11)
// C++11 has a nice static assert
#define CT_ASSERT(c,text) static_assert(c,text);
#else
// cryptic workaround for static assert
#define CT_ASSERT(c,text) typedef char static_assert__ [(c)?1:-1];
#endif

// assert that overflow behaviour is defined for signed integer types
#ifndef __GNUC__
// GCC should require -fwrapv for this (however, GCC 5.4 reports !is_modulo regardless)
CT_ASSERT(std::numeric_limits<uint8 >::is_modulo,"uint8 not modulo" );
CT_ASSERT(std::numeric_limits<uint16>::is_modulo,"uint16 not modulo");
CT_ASSERT(std::numeric_limits<uint32>::is_modulo,"uint32 not modulo");
CT_ASSERT(std::numeric_limits<sint8 >::is_modulo,"sint8 not modulo" );
CT_ASSERT(std::numeric_limits<sint16>::is_modulo,"sint16 not modulo");
CT_ASSERT(std::numeric_limits<sint32>::is_modulo,"sint32 not modulo");
#endif

// end of file
