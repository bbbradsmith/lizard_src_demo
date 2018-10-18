// Lizard
// Copyright Brad Smith 2018
// http://lizardnes.com

#include "lizard_game.h"
#include "lizard_lizard.h"
#include "lizard_audio.h"
#include "lizard_dogs.h"
#include "lizard_ppu.h"
#include "assets/export/data.h"
#include "assets/export/data_music.h"
#include "enums.h"

namespace Game
{

const sint16 GRAVITY      = 60;
const sint16 JUMP_SCALE   = 1200;
const sint16 RUN_SCALE    = 500;

const sint16 WET_GRAVITY  = GRAVITY / 2;

const sint16 START_JUMP   = -JUMP_SCALE;
const sint16 LOW_JUMP     = -JUMP_SCALE / 2;
const sint16 WET_JUMP     = -JUMP_SCALE / 2;
const sint16 RELEASE_JUMP = -JUMP_SCALE / 5;

const sint16 RUN_RIGHT    = RUN_SCALE / 10;
const sint16 WALK_RIGHT   = RUN_SCALE / 30;
const sint16 MAX_RIGHT    = RUN_SCALE;
const sint16 MAXW_RIGHT   = RUN_SCALE * 25 / 64;
const sint16 SKID_RIGHT   = RUN_SCALE * 9 / 64;
const sint16 AIR_RIGHT    = RUN_SCALE * 2 / 64;
const sint16 AIRW_RIGHT   = RUN_SCALE * 1 / 128;
const sint16 AIRS_RIGHT   = RUN_SCALE * 2 / 64;
const sint16 DRAG_RIGHT   = RUN_SCALE * 9 / 64;
const sint16 MAX_DOWN     = JUMP_SCALE * 20 / 16;
const sint16 MAX_DOWN_WET = JUMP_SCALE * 8 / 16;

const sint16 RUN_LEFT     = -RUN_RIGHT;
const sint16 WALK_LEFT    = -WALK_RIGHT;
const sint16 MAX_LEFT     = -MAX_RIGHT;
const sint16 MAXW_LEFT    = -MAXW_RIGHT;
const sint16 AIR_LEFT     = -AIR_RIGHT;
const sint16 AIRW_LEFT    = -AIRW_RIGHT;
const sint16 AIRS_LEFT    = -AIRS_RIGHT;
const sint16 DRAG_LEFT    = -DRAG_RIGHT;
const sint16 SKID_LEFT    = -SKID_RIGHT;
const sint16 MAX_UP       = -MAX_DOWN;
const sint16 MAX_UP_WET   = -MAX_DOWN_WET;
const sint16 MAX_UP_END   = -JUMP_SCALE * 4 / 16;

const sint16 FLOW         = RUN_SCALE * 7 / 64;
const sint16 FLOW_MAX     = RUN_SCALE / 2;

const sint16 SWIM_UP      = -JUMP_SCALE * 9 / 16;
const sint16 SWIM_RIGHT   = RUN_SCALE * 10 / 64;
const sint16 SWIM_LEFT    = -SWIM_RIGHT;
const uint8  SWIM_TIMEOUT = 6;

const sint16 SURF_SKIP    = -JUMP_SCALE * 8 / 16;
const sint16 SURF_RIGHT   = RUN_SCALE * 35 / 64;
const sint16 SURF_LEFT    = -SURF_RIGHT;
const sint16 SURF_JUMP    = 500;
const sint16 SURF_GRAVITY = -32;
const sint16 SURF_RELEASE = SURF_JUMP / 5;


const sint16 BOUNCE_PASS  = 4 * GRAVITY;
const sint16 BOUNCE_LOSS  = GRAVITY;
const sint16 BEYOND_FLOAT = -GRAVITY / 2;

const int HITBOX_R =  4;  // right side of hitbox
const int HITBOX_L = -5;  // left side of hitbox
const int HITBOX_T = -14; // top of hitbox
const int HITBOX_M = -7;  // vertical middle of hitbox
const int HITBOX_B = -1;  // bottom of hitbox

const int RIVER_RIGHT  = 256;
const int RIVER_LEFT   = -RIVER_RIGHT;
const int RIVER_DOWN   = 256;
const int RIVER_UP     = -RIVER_DOWN;
const int RIVER_DRAG_X = 6;
const int RIVER_DRAG_Y = 10;

const int RIVER_HITBOX_R =   5;
const int RIVER_HITBOX_L =  -8;
const int RIVER_HITBOX_T =  -5;
const int RIVER_HITBOX_B =   0;

const int BHM_OFFSET = DATA_sprite2_lizard_bhm_stand - DATA_sprite2_lizard_stand;

CT_ASSERT(DATA_sprite2_lizard_stand      + BHM_OFFSET == DATA_sprite2_lizard_bhm_stand,      "big head sprites out of order!");
CT_ASSERT(DATA_sprite2_lizard_blink      + BHM_OFFSET == DATA_sprite2_lizard_bhm_blink,      "big head sprites out of order!");
CT_ASSERT(DATA_sprite2_lizard_walk0      + BHM_OFFSET == DATA_sprite2_lizard_bhm_walk0,      "big head sprites out of order!");
CT_ASSERT(DATA_sprite2_lizard_walk1      + BHM_OFFSET == DATA_sprite2_lizard_bhm_walk1,      "big head sprites out of order!");
CT_ASSERT(DATA_sprite2_lizard_walk2      + BHM_OFFSET == DATA_sprite2_lizard_bhm_walk2,      "big head sprites out of order!");
CT_ASSERT(DATA_sprite2_lizard_walk3      + BHM_OFFSET == DATA_sprite2_lizard_bhm_walk3,      "big head sprites out of order!");
CT_ASSERT(DATA_sprite2_lizard_skid       + BHM_OFFSET == DATA_sprite2_lizard_bhm_skid,       "big head sprites out of order!");
CT_ASSERT(DATA_sprite2_lizard_fly        + BHM_OFFSET == DATA_sprite2_lizard_bhm_fly,        "big head sprites out of order!");
CT_ASSERT(DATA_sprite2_lizard_fall       + BHM_OFFSET == DATA_sprite2_lizard_bhm_fall,       "big head sprites out of order!");
CT_ASSERT(DATA_sprite2_lizard_skull      + BHM_OFFSET == DATA_sprite2_lizard_bhm_skull,      "big head sprites out of order!");
CT_ASSERT(DATA_sprite2_lizard_smoke      + BHM_OFFSET == DATA_sprite2_lizard_bhm_smoke,      "big head sprites out of order!");
CT_ASSERT(DATA_sprite2_lizard_surf_stand + BHM_OFFSET == DATA_sprite2_lizard_bhm_surf_stand, "big head sprites out of order!");
CT_ASSERT(DATA_sprite2_lizard_surf_blink + BHM_OFFSET == DATA_sprite2_lizard_bhm_surf_blink, "big head sprites out of order!");
CT_ASSERT(DATA_sprite2_lizard_surf_fly   + BHM_OFFSET == DATA_sprite2_lizard_bhm_surf_fly,   "big head sprites out of order!");
CT_ASSERT(DATA_sprite2_lizard_surf_fall  + BHM_OFFSET == DATA_sprite2_lizard_bhm_surf_fall,  "big head sprites out of order!");

// forward declarations
void lizard_tick_river();
void lizard_draw_river();

void lizard_fall_test()
{
	// test if on ground
	if ( collide_all(lizard_px + 0       , lizard_py) ||
	     collide_all(lizard_px + HITBOX_L, lizard_py) ||
	     collide_all(lizard_px + HITBOX_R, lizard_py) )
	{
		if (zp.current_lizard == LIZARD_OF_STONE && lizard.power > 0 && lizard.fall == 1 && lizard.flow == 0)
			play_sound(SOUND_STONE);
		lizard.fall = 0;
	}
	else lizard.fall = 1;
}

void do_scroll()
{
	if (zp.room_scrolling == 0)
	{
		lizard.x &= 0x0000FFFF; // wrap
		zp.scroll_x = 0;
	}
	else
	{
		lizard.x &= 0x0001FFFF; // wrap

		if (lizard_px <= 128)
		{
			zp.scroll_x = 0;
		}
		else if(lizard_px >= 384)
		{
			zp.scroll_x = 256;
		}
		else
		{
			zp.scroll_x = lizard_px - 128;
		}
	}
}

void lizard_init(uint16 x, uint8 y)
{
	if (zp.hold_y == 0) lizard.x = x << 8; // door position
	else
	{
		// clamp to edge to prevent undesired diagonal transitions
		if (h.dog_type[11] == DOG_PASS_X && lizard_px < 13)
		{
			lizard.x = 13 << 8;
		}

		if (h.dog_type[12] == DOG_PASS_X)
		{
			if (zp.room_scrolling)
			{
				if (lizard_px >= 500)
				{
					lizard.x = 409 << 8;
				}
			}
			else
			{
				if (lizard_px >= 244)
				{
					lizard.x = 243 << 8;
				}
			}
		}
	}

	if (zp.hold_x == 0) lizard.y = y << 8; // door position
	else
	{
		// clamp to edge to prevent undesired diagonal transitions
		if (h.dog_type[13] == DOG_PASS_Y && lizard_py <   22)
		{
			lizard.y =  22 << 8;
		}
		if ((h.dog_type[14] == DOG_PASS_Y || h.dog_type[15] == DOG_PASS_Y)
			&& lizard_py >= 233)
		{
			// note: dog 15 allowed as alternative for dog 14 because of grog room (needed to relocate for extra even slot)
			lizard.y = 232 << 8;
		}
	}

	if (zp.hold_x == 0 && zp.hold_y == 0)
	{
		lizard.vx = 0;
		lizard.vy = 0;
	}

	lizard.z = 0;
	lizard.vz = 0;

	lizard.anim = 0;
	//lizard.face = ?; always keep previous facing
	//lizard_fall_test(); done after dogs_init
	//lizard.jump = ?; keep jump flag
	lizard.scuttle = 0;
	lizard.skid = 0;
	lizard.dismount = 0;
	lizard.dead = 0;
	//lizard.power = 0; power only reset by change of next_lizard
	lizard.moving = 0;

	lizard.flow = 0;
	lizard.wet = 0;
	if (lizard_py >= zp.water) lizard.wet = 1;

	zp.hold_x = 0;
	zp.hold_y = 0;

	zp.smoke_x = 65535;
	zp.smoke_y = 255;

	do_scroll();
}

void lizard_move()
{
	// cap X motion before applying
	if      (lizard.vx < MAX_LEFT)  lizard.vx = MAX_LEFT;
	else if (lizard.vx > MAX_RIGHT) lizard.vx = MAX_RIGHT;

	lizard.x += lizard.vx;
	if (lizard.vx > 0) // collide right
	{
		uint8 shift;
		shift = collide_all_right(lizard_px + HITBOX_R, lizard_py + HITBOX_B); if (shift) { lizard.x -= (shift << 8); lizard.vx = 0; lizard.x |= 0xFF; }
		shift = collide_all_right(lizard_px + HITBOX_R, lizard_py + HITBOX_T); if (shift) { lizard.x -= (shift << 8); lizard.vx = 0; lizard.x |= 0xFF; }
		shift = collide_all_right(lizard_px + HITBOX_R, lizard_py + HITBOX_M); if (shift) { lizard.x -= (shift << 8); lizard.vx = 0; lizard.x |= 0xFF; }
	}
	else if (lizard.vx < 0) // collide left
	{
		uint8 shift;
		shift = collide_all_left( lizard_px + HITBOX_L, lizard_py + HITBOX_B); if (shift) { lizard.x += (shift << 8); lizard.vx = 0; lizard.x &= ~0xFF; }
		shift = collide_all_left( lizard_px + HITBOX_L, lizard_py + HITBOX_T); if (shift) { lizard.x += (shift << 8); lizard.vx = 0; lizard.x &= ~0xFF; }
		shift = collide_all_left( lizard_px + HITBOX_L, lizard_py + HITBOX_M); if (shift) { lizard.x += (shift << 8); lizard.vx = 0; lizard.x &= ~0xFF; }
	}

	if (zp.current_lizard == LIZARD_OF_BEYOND && lizard.power > 0)
	{
		lizard.vy += BEYOND_FLOAT;
		lizard_fall_test();
		if (!lizard.fall)
		{
			if (lizard.vy >= 0)
			{
				lizard.vy = 0;
				lizard.y |= 0xFF;
			}
		}
		goto skip_gravity;
	}

	// test if falling, apply gravity
	lizard_fall_test();
	if (!lizard.fall)
	{
		if (zp.current_lizard == LIZARD_OF_BOUNCE && lizard.power != 0 && lizard.vy > BOUNCE_PASS)
		{
			lizard.vy = -lizard.vy + BOUNCE_LOSS;

			if (lizard.wet)
				lizard.vy >>= 1;

			lizard.fall = 1;
			h.bounced = 1;
			lizard.y -= 256;
			play_sound(SOUND_BOUNCE);
			goto apply_gravity;
		}
		else if (lizard.vy >= 0)
		{
			lizard.vy = 0;
			lizard.y |= 0xFF;
			goto skip_gravity;
		}
	}

apply_gravity:
	if (lizard.wet)
		lizard.vy += WET_GRAVITY;
	else
		lizard.vy += GRAVITY;
skip_gravity:

	// cap Y motion before applying
	if (lizard.wet)
	{
		if (lizard.vy > MAX_DOWN_WET) lizard.vy = MAX_DOWN_WET;
		if (lizard.vy < MAX_UP_WET  ) lizard.vy = MAX_UP_WET;
	}
	else
	{
		if (lizard.vy > MAX_DOWN) lizard.vy = MAX_DOWN;
		if (lizard.vy < MAX_UP  ) lizard.vy = MAX_UP;
	}

	lizard.y += lizard.vy;
	if (lizard.vy > 0) // collide down
	{
		bool landed = false;
		uint8 shift;
		shift = collide_all_down(lizard_px + 0       , lizard_py + HITBOX_B); if (shift) { lizard.y -= (shift << 8);                lizard.y |= 0xFF; landed = true; }
		shift = collide_all_down(lizard_px + HITBOX_L, lizard_py + HITBOX_B); if (shift) { lizard.y -= (shift << 8);                lizard.y |= 0xFF; landed = true; }
		shift = collide_all_down(lizard_px + HITBOX_R, lizard_py + HITBOX_B); if (shift) { lizard.y -= (shift << 8);                lizard.y |= 0xFF; landed = true; }
		// NOTE: lizard_vy is not zeroed, this is to allow bounce

		if (landed)
		{
			if (zp.current_lizard == LIZARD_OF_STONE && lizard.power > 0 && lizard.fall == 1 && lizard.flow == 0)
				play_sound(SOUND_STONE);
			lizard.fall = 0;
		}
	}
	else if (lizard.vy < 0) // collide up
	{
		uint8 shift;
		shift = collide_all_up(  lizard_px + 0       , lizard_py + HITBOX_T); if (shift) { lizard.y += (shift << 8); lizard.vy = 0; lizard.y &= ~0xFF; }
		shift = collide_all_up(  lizard_px + HITBOX_L, lizard_py + HITBOX_T); if (shift) { lizard.y += (shift << 8); lizard.vy = 0; lizard.y &= ~0xFF; }
		shift = collide_all_up(  lizard_px + HITBOX_R, lizard_py + HITBOX_T); if (shift) { lizard.y += (shift << 8); lizard.vy = 0; lizard.y &= ~0xFF; }
	}

	// check water
	if (zp.water < 240 && lizard_py >= zp.water)
	{
		if (lizard.wet == 0)
		{
			play_sound(SOUND_WATER);
			if (h.dog_type[SPLASHER_SLOT] == DOG_SPLASHER)
			{
				h_dog_x[SPLASHER_SLOT] = lizard_px - 4;
				h.dog_y[SPLASHER_SLOT] = zp.water - 9;
				h.dog_data[0][SPLASHER_SLOT] = 0; // trigger splash animation
			}

			// surf board can skim the water if going fast enough
			if (zp.current_lizard == LIZARD_OF_SURF &&
				lizard.power > 0 &&
				(lizard.vx > SURF_RIGHT || lizard.vx < SURF_LEFT))
			{
				lizard.vy = SURF_SKIP;
			}
			else lizard.wet = 1;
		}
		else lizard.wet = 1;
	}
	else
	{
		if (lizard.wet != 0)
		{
			play_sound(SOUND_WATER);
			if (h.dog_type[SPLASHER_SLOT] == DOG_SPLASHER)
			{
				h_dog_x[SPLASHER_SLOT] = lizard_px - 4;
				h.dog_y[SPLASHER_SLOT] = zp.water - 9;
				h.dog_data[0][SPLASHER_SLOT] = 0; // trigger splash animation
			}
		}
		lizard.wet = 0;
	}

	lizard.flow = 0;
	do_scroll();
}

void lizard_die()
{
	if (lizard.dead != 0) return;

	++h.metric_bones;

	play_sound(SOUND_BONES);
	lizard.dead = 1;
	lizard.power = 0;
	lizard.scuttle = 0;
	lizard.vy = -800;
	lizard.vx = prng() << 1;
	lizard.vx = (prng() & 1) ? -lizard.vx : lizard.vx;
	//lizard.big_head_mode = 0;
}

void lizard_big_head_mode_test()
{
	// big head mode
	// trigger by jumping 8 times, alternating holding UP and DOWN each time
	if ((zp.gamepad & PAD_U) && !(lizard.big_head_mode & 1))
	{
		++lizard.big_head_mode;
	}
	else if ((zp.gamepad & PAD_D) && (lizard.big_head_mode & 1))
	{
		++lizard.big_head_mode;
		if ((lizard.big_head_mode & 0x7F) >= 8)
		{
			lizard.big_head_mode ^= 0x80;
			lizard.big_head_mode &= 0x80;

			// cancels jump sound
			play_sound(SOUND_SECRET);
		}
	}
	else
	{
		lizard.big_head_mode &= 0x80;
	}
}

void lizard_jump_finish()
{
	play_sound(SOUND_JUMP);
	++h.metric_jumps;
	lizard_big_head_mode_test();
}

void lizard_dead_tick()
{
	lizard.touch_x0 = (127 << 8) | 255;
	lizard.touch_y0 = 255;
	lizard.touch_x1 = (127 << 8) | 255;
	lizard.touch_y1 = 255;

	lizard.x += lizard.vx;
	if (!zp.room_scrolling) // wrapping
	{
		uint32 tx = lizard.x & 0x0000FFFF;
		if (tx != lizard.x)
		{
			lizard.x = tx;
			lizard.y = 255 << 8;
			lizard.vy = 0;
		}
	}
	else
	{
		uint32 tx = lizard.x & 0x0001FFFF;
		if (tx != lizard.x)
		{
			lizard.x = tx;
			lizard.y = 255 << 8;
			lizard.vy = 0;
		}
	}

	uint16 temp = lizard.y + lizard.vy;
	if ((lizard.vy >= 0 && (temp & 0xFF00) >= (lizard.y & 0xFF00)) ||
		(lizard.vy <  0 && (temp & 0xFF00) <= (lizard.y & 0xFF00)))
	{
		lizard.y = temp;
		lizard.vy += GRAVITY;
	}
	else
	{
		lizard.y = 255 << 8;
		lizard.vy = 0;
	}

	if (lizard.power < 60) // wait 1s
	{
		++lizard.power;
	}
	else
	{
		if (zp.gamepad == 0) lizard.power = 61; // allow restart after input release
		else if (lizard.power >= 61 && (zp.gamepad & (PAD_A|PAD_B|PAD_START)))
		{
			uint16 rr;
			uint8 rl;
			if (!password_read(&rr, &rl) || !checkpoint(rr) || !zp.continued)
			{
				rr = DATA_room_start;
				rl = LIZARD_OF_START;
			}
			lizard.power = 0;
			zp.next_lizard = rl;
			zp.current_room = rr;
			zp.current_door = 0;

			bool eye_old = flag_read(FLAG_EYESIGHT);

			for (int i=0; i<16; ++i) h.coin[i] = h.coin_save[i];
			for (int i=0; i<16; ++i) h.flag[i] = h.flag_save[i];
			h.piggy_bank = h.piggy_bank_save;
			h.last_lizard = h.last_lizard_save;
			zp.coin_saved = 1;

			if (eye_old != flag_read(FLAG_EYESIGHT))
			{
				for (int i=0; i<8; ++i) zp.chr_cache[i] = 0xFF;
			}

			//++h.tip_counter; // disabled for demo
			zp.room_change = 1; // restart the level

			resume_point(true);
		}
	}
}

void lizard_tick()
{
	if (h.dog_type[RIVER_SLOT] == DOG_RIVER)
	{
		lizard_tick_river();
		return;
	}

	if (lizard.dead != 0)
	{
		lizard_dead_tick();
		return;
	}

	if (lizard.dismount != 0)
	{
		if (lizard.dismount == 1) // dismount
		{
			lizard.scuttle = 0;
			return;
		}
		else if (lizard.dismount == 2) // river exit (forced surf)
		{
			zp.gamepad = PAD_A | PAD_B | PAD_R;
			lizard.dismount = 0;
		}
		else if (lizard.dismount == 3) // ending (forced fly)
		{
			if (lizard.vy < MAX_UP_END) lizard.vy = MAX_UP_END;

			zp.gamepad = PAD_D | PAD_B;
			lizard.dismount = 0;
		}
	}

	// hold select to scuttle lizard
	if (zp.gamepad & PAD_SELECT)
	{
		++lizard.scuttle;
		if (lizard.scuttle == 255)
		{
			lizard_die();
			return;
		}
	}
	else
		lizard.scuttle = 0;

	if (zp.gamepad & PAD_START) // pause requested
	{
		if (zp.mode_temp > 0)
		{
			h.text_select = 2; // pause
			zp.game_pause = 1;
			return;
		}
	}
	else zp.mode_temp = 1;

	// stone mode removes all movement input
	if (zp.current_lizard == LIZARD_OF_STONE && lizard.power > 0)
	{
		zp.gamepad &= PAD_B | PAD_SELECT | PAD_START;
		if (!lizard.fall)
		{
			lizard.vx = 0;
		}
	}

	// handle jump request
	if (!(zp.gamepad & PAD_A))
	{
		if (lizard.jump && lizard.vy < RELEASE_JUMP) // if was jumping let the user release
		{
			lizard.vy = RELEASE_JUMP;
		}
		lizard.jump = 0; // clear jump flag, user can now press A
	}
	else if (!lizard.fall && !lizard.jump && // on ground, ready to jump
		!(zp.current_lizard == LIZARD_OF_BOUNCE && lizard.power > 0) // not trying to bounce
		)
	{
		if (lizard.wet)
			lizard.vy = WET_JUMP;
		else
			lizard.vy = START_JUMP;

		// coffe / lounge "powers"
		if (lizard.power > 30)
		{
			if (zp.current_lizard == LIZARD_OF_COFFEE)
				lizard.vy = MAX_UP;
			else if (zp.current_lizard == LIZARD_OF_LOUNGE)
				lizard.vy = LOW_JUMP;
		}

		lizard.fall = 1;
		lizard.skid = 0;
		lizard.jump = 1;

		lizard_jump_finish();
	}

	// handle ground movement requests
	if (!lizard.fall)
	{
		if (zp.gamepad & PAD_R && !(zp.gamepad & PAD_L))
		{
			if (lizard.face && lizard.vx < 0) // moving left, must begin skid
			{
				lizard.skid = 1;
			}
			else // facing the correct direction or ready to accelerate
			{
				lizard.skid = 0;
				lizard.face = 0;
				if (zp.gamepad & PAD_D)
				{
					if (lizard.vx < MAXW_RIGHT) lizard.vx += WALK_RIGHT;
					else lizard.vx += DRAG_LEFT;
				}
				else lizard.vx += RUN_RIGHT;
			}
		}
		else if (zp.gamepad & PAD_L && !(zp.gamepad & PAD_R))
		{
			if (!lizard.face && lizard.vx > 0)
			{
				lizard.skid = 1;
			}
			else
			{
				lizard.skid = 0;
				lizard.face = 1;
				if (zp.gamepad & PAD_D)
				{
					if (lizard.vx > MAXW_LEFT) lizard.vx += WALK_LEFT;
					else lizard.vx += DRAG_RIGHT;
				}
				else lizard.vx += RUN_LEFT;
			}
		}
		else if ((zp.gamepad & (PAD_U|PAD_D|PAD_L|PAD_R)) == PAD_D)
		{
			lizard.skid = 1;
		}
		else if (!lizard.skid && lizard.flow == 0) // drag
		{
			if (lizard.vx > 0)
			{
				lizard.vx += DRAG_LEFT;
				if (lizard.vx < 0) lizard.vx = 0;
			}
			else if (lizard.vx < 0)
			{
				lizard.vx += DRAG_RIGHT;
				if (lizard.vx > 0) lizard.vx = 0;
			}
		}

		// skidding
		if (lizard.skid)
		{
			if (lizard.face) // skidding left
			{
				lizard.vx += SKID_RIGHT;
				if (lizard.vx > 0)
				{
					lizard.skid = 0;
					lizard.vx = 0;
				}
			}
			else
			{
				lizard.vx += SKID_LEFT;
				if (lizard.vx < 0)
				{
					lizard.skid = 0;
					lizard.vx = 0;
				}
			}
		}
	}
	else // air velocity control
	{
		if (!(zp.gamepad & PAD_D))
		{
			if (zp.gamepad & PAD_L) lizard.vx += AIR_LEFT;
			if (zp.gamepad & PAD_R) lizard.vx += AIR_RIGHT;
		}
		else
		{
			if (zp.gamepad & PAD_L) lizard.vx += AIRW_LEFT;
			if (zp.gamepad & PAD_R) lizard.vx += AIRW_RIGHT;
			// holding just down comes to a stop
			if (!(zp.gamepad & (PAD_L | PAD_R)))
			{
				if (lizard.vx >= 0)
				{
					lizard.vx += AIRS_LEFT;
					if (lizard.vx < 0) lizard.vx = 0;
				}
				else if (lizard.vx < 0)
				{
					lizard.vx += AIRS_RIGHT;
					if (lizard.vx >= 0) lizard.vx = 0;
				}
			}
		}
	}

	if (lizard.flow == 1)
	{
		if (lizard.vx > -FLOW_MAX)
		{
			lizard.vx -= FLOW;
		}
	}
	else if (lizard.flow == 2)
	{
		if (lizard.vx < FLOW_MAX)
		{
			lizard.vx += FLOW;
		}
	}

	// correct facing after landing
	if (!lizard.skid && !lizard.fall)
	{
		if ( lizard.face && lizard.vx > 0) lizard.face = 0;
		else
		if (!lizard.face && lizard.vx < 0) lizard.face = 1;
	}

	// touch (set otherwise by fire/death)
	lizard.touch_x0 = (127 << 8) | 255;
	lizard.touch_y0 = 255;
	lizard.touch_x1 = (127 << 8) | 255;
	lizard.touch_y1 = 255;

	// powers
	switch (zp.current_lizard)
	{
	case LIZARD_OF_KNOWLEDGE:
	case LIZARD_OF_BOUNCE:
		if (zp.gamepad & PAD_B)
			lizard.power = 1;
		else
			lizard.power = 0;
		break;
	case LIZARD_OF_SWIM:
		if (zp.gamepad & PAD_B)
		{
			if (lizard.power == 0)
			{
				play_sound(SOUND_SWIM);
				if (lizard.wet)
				{
					if (lizard.vy > 0) lizard.vy = 0;
					lizard.vy += SWIM_UP;
					lizard.fall = 1;
					lizard.skid = 0;
					if (zp.gamepad & PAD_L)
					{
						lizard.face = 1;
						lizard.vx += SWIM_LEFT;
					}
					if (zp.gamepad & PAD_R)
					{
						lizard.face = 0;
						lizard.vx += SWIM_RIGHT;
					}
				}
			}
			if (lizard.power < 255)
				++lizard.power;
		}
		else
		{
			if (lizard.power > 0)
			{
				++lizard.power;
				if (lizard.power > SWIM_TIMEOUT)
					lizard.power = 0;
			}
		}
		break;
	case LIZARD_OF_HEAT:
		if (lizard.wet)
		{
			lizard.power = 0;
			break;
		}
		if ((zp.gamepad & PAD_B))
		{
			if (lizard.power == 0)
			{
				lizard.power = 1;
				play_sound(SOUND_FIRE);
			}
			else
			{
				++lizard.power;
				if (lizard.power >= 17)
				{
					lizard.power = 1;
					play_sound(SOUND_FIRE_CONTINUE);
				}
			}
		}
		else
		{
			lizard.power = 0;
		}
		break;
	case LIZARD_OF_SURF:
		if (zp.gamepad & PAD_B)
			lizard.power = 1;
		else
			lizard.power = 0;
		break;
	case LIZARD_OF_PUSH:
		if (zp.gamepad & PAD_B)
		{
			if (lizard.power == 0)
			{
				lizard.power = 1;
				play_sound(SOUND_BLOW);
				
				zp.smoke_x = lizard.face ? (lizard_px-17) : (lizard_px+10);
				zp.smoke_y = lizard_py - 16;
				if ((zp.smoke_y+7) > zp.water) zp.smoke_y = 255;
			}
			else
			{
				++lizard.power;
				if (lizard.power >= 255)
					lizard.power = 254;
			}
		}
		else
			lizard.power = 0;
		break;
	case LIZARD_OF_STONE:
		if (zp.gamepad & PAD_B)
			lizard.power = 1;
		else
			lizard.power = 0;
		break;
	case LIZARD_OF_COFFEE:
		// same implementation as lounge
	case LIZARD_OF_LOUNGE:
		if (lizard.wet) { lizard.power = 0; break; }

		if (lizard.power > 0 && lizard.power < 255)
		{
			++lizard.power;
			if (lizard.power == 45) play_sound(SOUND_SIP);
			else if (lizard.power == 60 && zp.current_lizard == LIZARD_OF_LOUNGE)
			{
				play_sound(SOUND_BLOW);
				zp.smoke_x = lizard.face ? (lizard_px-17) : (lizard_px+10);
				zp.smoke_y = lizard_py - 16;
			}
			else if (lizard.power == 255) lizard.power = 1; // keep smoking if held
		}

		if (zp.gamepad & PAD_B)
		{
			if (lizard.power == 0) lizard.power = 1;
		}
		else if (lizard.power < 30 || lizard.power == 255)
		{
			lizard.power = 0;
		}
		break;
	case LIZARD_OF_DEATH:
		if (zp.gamepad & PAD_B)
		{
			if (lizard.power == 0) play_sound(SOUND_SIP);
			lizard.power = 1;
		}
		else
			lizard.power = 0;
		break;
	case LIZARD_OF_BEYOND:
		if (zp.gamepad & PAD_B)
			lizard.power = 1;
		else
			lizard.power = 0;
		break;
	default:
		break;
	}

	// apply motion
	lizard_move();

	// animate
	if      (lizard.vx > 0) lizard.anim += lizard.vx;
	else if (lizard.vx < 0) lizard.anim -= lizard.vx;
	else if (lizard.moving != 0) lizard.anim += MAXW_RIGHT;
	else ++lizard.anim;

	// hitbox and touch updated after motion so what's on-screen matches current state

	// update hitbox
	lizard.hitbox_x0 = lizard_px + HITBOX_L;
	lizard.hitbox_x1 = lizard_px + HITBOX_R;
	lizard.hitbox_y0 = lizard_py + HITBOX_T;
	lizard.hitbox_y1 = lizard_py + HITBOX_B;

	// update touch
	if (lizard.power > 0)
	{
		if (zp.current_lizard == LIZARD_OF_HEAT)
		{
			if (lizard.face)
			{
				lizard.touch_x0 = lizard_px-15;
				lizard.touch_y0 = lizard_py-14;
				lizard.touch_x1 = lizard_px-0;
				lizard.touch_y1 = lizard_py-1;
				if (lizard.touch_x0 >= 32768) lizard.touch_x0 = 0;
				if (lizard.touch_x1 >= 32768) lizard.touch_x1 = 0;
			}
			else
			{
				lizard.touch_x0 = lizard_px+0;
				lizard.touch_y0 = lizard_py-14;
				lizard.touch_x1 = lizard_px+14;
				lizard.touch_y1 = lizard_py-1;
			}
		}
		else if (zp.current_lizard == LIZARD_OF_DEATH)
		{
			if (lizard.face)
			{
				lizard.touch_x0 = lizard_px-14;
				lizard.touch_y0 = lizard_py-11;
				lizard.touch_x1 = lizard_px-12;
				lizard.touch_y1 = lizard_py-5;
				if (lizard.touch_x0 >= 32768) lizard.touch_x0 = 0;
				if (lizard.touch_x1 >= 32768) lizard.touch_x1 = 0;
			}
			else
			{
				lizard.touch_x0 = lizard_px+11;
				lizard.touch_y0 = lizard_py-11;
				lizard.touch_x1 = lizard_px+13;
				lizard.touch_y1 = lizard_py-5;
			}
		}
	}

	// timeout for smoother pushing
	if (lizard.skid)
	{
		lizard.moving = 0;
	}
	else if (lizard.vx != 0)
	{
		lizard.moving = 4;
	}
	else if (lizard.moving > 0)
	{
		--lizard.moving;
	}

	// debug
	PPU::debug_box(lizard.hitbox_x0,lizard.hitbox_y0,lizard.hitbox_x1,lizard.hitbox_y1);
	PPU::debug_box(lizard.touch_x0,lizard.touch_y0,lizard.touch_x1,lizard.touch_y1);
}

bool lizard_scuttle()
{
	if (lizard.scuttle == 0) return false;
	if (lizard.scuttle < 64  && ((lizard.scuttle & 31) < 30)) return false;
	if (lizard.scuttle < 128 && ((lizard.scuttle & 15) < 14)) return false;
	if (lizard.scuttle < 196 && ((lizard.scuttle &  7) <  6)) return false;
	if (                         (lizard.scuttle &  3) <  2 ) return false;
	return true;
}

void lizard_draw_dead()
{
	uint8 dx;
	uint8 dy = lizard_py;

	if (h.dog_type[RIVER_SLOT] == DOG_RIVER || h.dog_type[0] == DOG_FROB)
	{
		if (lizard_px >= 256) return;
		dx = uint8(lizard_px);
	}
	else
	{
		uint16 sdx = lizard_px - zp.scroll_x;
		dx = uint8(sdx);
		if (sdx != uint16(dx)) return;
	}
	sprite_prepare(dx);

	if (lizard_py < 255)
	{
		if (lizard.face) sprite2_add_flipped(dx,dy,DATA_sprite2_lizard_skull);
		else             sprite2_add(        dx,dy,DATA_sprite2_lizard_skull);
	}
}

void lizard_draw()
{
	if (lizard_scuttle())
	{
		palette_load(4,DATA_palette_lizard_flash);
	}
	else
	{
		if (h.dog_type[BEYOND_STAR_SLOT] == DOG_BEYOND_STAR && h.dog_data[BEYOND_STAR_DIE][BEYOND_STAR_SLOT] != 0)
		{
			// rainbow cycle
			for (int c=(4*4)+1; c<(5*4); ++c)
			{
				++h.palette[c];
				if ((h.palette[c] & 0xF) >= 0xD)
				{
					h.palette[c] -= 0xC;
				}
			}
		}
		else
		{
			palette_load(4,DATA_palette_lizard0 + zp.current_lizard);
		}
		palette_load(5,DATA_palette_human0 + zp.human0_pal);
	}

	if (lizard.dismount == 1) return; // lizard_dismounter draws lizard at this point

	if (lizard.dead)
	{
		lizard_draw_dead();
		return;
	}

	if (h.dog_type[0] == DOG_RIVER)
	{
		lizard_draw_river();
		return;
	}

	uint8 dx = lizard_px - zp.scroll_x;
	uint8 dy = lizard_py;
	sprite_prepare(dx);

	uint8 s;
	if (lizard.fall) s = (lizard.vy <= 0) ? DATA_sprite2_lizard_fly : DATA_sprite2_lizard_fall;
	else if (lizard.skid) s = DATA_sprite2_lizard_skid;
	else if (lizard.moving > 0)
	{
		s = DATA_sprite2_lizard_walk0 + ((lizard.anim >> 11) & 3);
	}
	else s = ((lizard.anim & 255) >= 248) ? DATA_sprite2_lizard_blink : DATA_sprite2_lizard_stand;

	if (lizard.power < 1) goto draw;

	switch (zp.current_lizard)
	{
	case LIZARD_OF_KNOWLEDGE:
		{
			uint8 sy = dy - 26;
			if (lizard.big_head_mode & 0x80) sy -= 6;
			sprite_tile_add(dx-4,sy,0x25,0x01);
		}
		break;
	case LIZARD_OF_BOUNCE:
		{
			// make sure these stay in line so that NES implementation does not break
			CT_ASSERT(DATA_sprite2_lizard_blink == DATA_sprite2_lizard_stand+1,"lizard sprites out of order");
			CT_ASSERT(DATA_sprite2_lizard_walk0 == DATA_sprite2_lizard_blink+1,"lizard sprites out of order");
			CT_ASSERT(DATA_sprite2_lizard_walk1 == DATA_sprite2_lizard_walk0+1,"lizard sprites out of order");
			CT_ASSERT(DATA_sprite2_lizard_walk2 == DATA_sprite2_lizard_walk1+1,"lizard sprites out of order");
			CT_ASSERT(DATA_sprite2_lizard_walk3 == DATA_sprite2_lizard_walk2+1,"lizard sprites out of order");
			CT_ASSERT(DATA_sprite2_lizard_skid  == DATA_sprite2_lizard_walk3+1,"lizard sprites out of order");
			CT_ASSERT(DATA_sprite2_lizard_fly   == DATA_sprite2_lizard_skid +1,"lizard sprites out of order");
			CT_ASSERT(DATA_sprite2_lizard_fall  == DATA_sprite2_lizard_fly  +1,"lizard sprites out of order");

			const sint8 BOOTS_POS[8][4] = { // { x0,y0,x1,y1 }
				{ -2, -4, 2, -4 }, // stand/blink
				{ -3, -4, 2, -4 }, // walk0
				{ -1, -4, 2, -4 }, // walk1
				{ -1, -4, 2, -4 }, // walk2
				{ -2, -4, 2, -5 }, // walk3
				{  1, -4, 4, -4 }, // skid
				{ -2, -4, 2, -4 }, // fly
				{ -1, -5, 2, -4 }, // fall
			};
			int boots = -1;
			switch (s)
			{
			case DATA_sprite2_lizard_stand:
			case DATA_sprite2_lizard_blink: boots = 0; break;
			case DATA_sprite2_lizard_walk0: boots = 1; break;
			case DATA_sprite2_lizard_walk1: boots = 2; break;
			case DATA_sprite2_lizard_walk2: boots = 3; break;
			case DATA_sprite2_lizard_walk3: boots = 4; break;
			case DATA_sprite2_lizard_skid:  boots = 5; break;
			case DATA_sprite2_lizard_fly:   boots = 6; break;
			case DATA_sprite2_lizard_fall:  boots = 7; break;
			default: break;
			}
			uint8 tile = (boots == 6) ? 0x37 : 0x36;
			if (boots >= 0)
			{
				if (lizard.face == 0)
				{
					sprite_tile_add(dx+BOOTS_POS[boots][0],dy+BOOTS_POS[boots][1],tile,0x01);
					sprite_tile_add(dx+BOOTS_POS[boots][2],dy+BOOTS_POS[boots][3],tile,0x01);
				}
				else
				{
					sprite_tile_add(dx-(8+BOOTS_POS[boots][0]),dy+BOOTS_POS[boots][1],tile,0x41);
					sprite_tile_add(dx-(8+BOOTS_POS[boots][2]),dy+BOOTS_POS[boots][3],tile,0x41);
				}
			}
		}
		break;
	case LIZARD_OF_SWIM:
		{
			if (lizard.power < 3) { s = DATA_sprite2_lizard_fall; goto draw; }
			if (lizard.power < 9) { s = DATA_sprite2_lizard_fly; goto draw; }
		}
		break;
	case LIZARD_OF_HEAT:
		{
			CT_ASSERT(DATA_sprite2_lizard_power_fire1 == DATA_sprite2_lizard_power_fire0+1,"lizard_power_fire sprites out of order");
			uint8 sprite = DATA_sprite2_lizard_power_fire0 + ((lizard.power >> 2) & 1);
			if (lizard.face)
			{
				sprite2_add(dx,dy,sprite);
			}
			else
			{
				sprite2_add_flipped(dx,dy,sprite);
			}
		}
		break;
	case LIZARD_OF_SURF:
		{
			bool bhm = (lizard.big_head_mode & 0x80) != 0;

			if (bhm)
			{
				s += BHM_OFFSET;
				NES_ASSERT(s >= DATA_sprite2_lizard_bhm_stand && s <= DATA_sprite2_lizard_bhm_smoke, "Big head sprite out of range?")
			}

			// draw lizard first so surfboard goes underneath
			if (lizard.face) sprite2_add_flipped(dx,dy,s);
			else             sprite2_add(        dx,dy,s);

			if (lizard.wet) return;

			uint8 y = dy - 10;
			uint8 flipped = lizard.face;

			if (bhm)
			{
				y += 1;
				s -= BHM_OFFSET;
			}

			if (s == DATA_sprite2_lizard_fly) flipped ^= 1;
			else if (s == DATA_sprite2_lizard_fall) y += 1;

			if (flipped)
			{
				sprite_tile_add(dx-0,y,0x26,0x40);
				sprite_tile_add(dx-8,y,0x26,0x80);
			}
			else
			{
				sprite_tile_add(dx-0,y,0x26,0xC0);
				sprite_tile_add(dx-8,y,0x26,0x00);
			}
		}
		break;
	case LIZARD_OF_PUSH:
		if (lizard.power < 20)
		{
			uint8 sx = zp.smoke_x - zp.scroll_x;
			if (uint16(sx) != uint16(zp.smoke_x - zp.scroll_x)) break; // scrolled offscreen

			if (lizard.power <  5) { sprite_tile_add(sx,zp.smoke_y,0x22,0x01); break; }
			if (lizard.power < 10) { sprite_tile_add(sx,zp.smoke_y,0x23,0x01); break; }
			if (lizard.power < 15) { sprite_tile_add(sx,zp.smoke_y,0x23,0x41); break; }
			if (lizard.power < 20) { sprite_tile_add(sx,zp.smoke_y,0x22,0x41); break; }
		}
		break;
	case LIZARD_OF_STONE:
		{
			// stone skin for human
			palette_load(5,DATA_palette_human_stone);
			// stone lizard does not move, always stands
			s = DATA_sprite2_lizard_stand;
		}
		break;
	case LIZARD_OF_COFFEE:
		{
			if (lizard.power > 30 && lizard.power < 45)
			{
				s = DATA_sprite2_lizard_smoke;
				if (!lizard.face) sprite2_add_flipped(dx,dy,DATA_sprite2_lizard_power_coffee0);
				else              sprite2_add(        dx,dy,DATA_sprite2_lizard_power_coffee0);
			}
			else
			{
				if (!lizard.face) sprite2_add_flipped(dx,dy,DATA_sprite2_lizard_power_coffee1);
				else              sprite2_add(        dx,dy,DATA_sprite2_lizard_power_coffee1);
			}
		}
		break;
	case LIZARD_OF_LOUNGE:
		{
			uint8 y = lizard_py-9;
			if (lizard.power > 30 && lizard.power < 45)
			{
				y = lizard_py-10;
				s = DATA_sprite2_lizard_smoke;
			}
			if (!lizard.face) sprite2_add_flipped(dx,y,DATA_sprite2_lizard_power_cigarette);
			else              sprite2_add(        dx,y,DATA_sprite2_lizard_power_cigarette);

			if (lizard.power >= 65 && lizard.power < 85)
			{
				uint8 sx = zp.smoke_x - zp.scroll_x;
				if (uint16(sx) != uint16(zp.smoke_x - zp.scroll_x)) break; // scrolled offscreen

				if (lizard.power < 70) { sprite_tile_add(sx,zp.smoke_y,0x22,0x01); break; }
				if (lizard.power < 75) { sprite_tile_add(sx,zp.smoke_y,0x23,0x01); break; }
				if (lizard.power < 80) { sprite_tile_add(sx,zp.smoke_y,0x23,0x41); break; }
				if (lizard.power < 85) { sprite_tile_add(sx,zp.smoke_y,0x22,0x41); break; }
			}
		}
		break;
	case LIZARD_OF_DEATH:
		{
			uint8 y = lizard_py-13;
			if ((s >= DATA_sprite2_lizard_walk1) && (s <= DATA_sprite2_lizard_walk3))
			{
				y += 1;
			}
			if (lizard.face) sprite2_add(        dx,y,DATA_sprite2_lizard_power_death);
			else             sprite2_add_flipped(dx,y,DATA_sprite2_lizard_power_death);
		}
		break;
	case LIZARD_OF_BEYOND:
		break;
	default:
		break;
	}

	if (zp.current_lizard == LIZARD_OF_SURF) return;

draw:

	if (lizard.big_head_mode & 0x80)
	{
		s += BHM_OFFSET;
		NES_ASSERT(s >= DATA_sprite2_lizard_bhm_stand && s <= DATA_sprite2_lizard_bhm_smoke, "Big head sprite out of range?")
	}

	if (lizard.face) sprite2_add_flipped(dx,dy,s);
	else             sprite2_add(        dx,dy,s);
}

//
// River
//

void lizard_tick_river()
{
	// death
	if (lizard.dead != 0)
	{
		lizard_dead_tick();
		return;
	}

	lizard.skid = zp.gamepad; // store last gamepad for draw

	// hold select to scuttle lizard
	if (zp.gamepad & PAD_SELECT)
	{
		++lizard.scuttle;
		if (lizard.scuttle == 255)
		{
			lizard_die();
			return;
		}
	}
	else
	{
		lizard.scuttle = 0;
	}

	if (zp.gamepad & PAD_START) // pause requested
	{
		if (zp.mode_temp > 0)
		{
			h.text_select = 2; // pause
			zp.game_pause = 1;
			return;
		}
	}
	else zp.mode_temp = 1;

	lizard.power = zp.gamepad & PAD_B; // powers are simplified, only KNOWLEDGE does anything

	// jumping
	// lizard.wet != 0: in air
	// lizard.wet == 2: in air off ramp (prevent jump release)

	if (zp.gamepad & PAD_A)
	{
		if (!lizard.jump && !lizard.wet)
		{
			lizard.vz = SURF_JUMP;
			lizard.jump = 1;
			lizard.wet = 1;

			lizard_jump_finish();
		}
	}
	else
	{
		lizard.jump = 0;
		if (lizard.wet == 1 && lizard.vz >= SURF_RELEASE)
		{
			lizard.vz = SURF_RELEASE;
		}
	}

	// motion

	if (!lizard.wet)
	{
		if      ((zp.gamepad & (PAD_L | PAD_R)) == PAD_L) lizard.vx = RIVER_LEFT;
		else if ((zp.gamepad & (PAD_L | PAD_R)) == PAD_R) lizard.vx = RIVER_RIGHT;

		if      ((zp.gamepad & (PAD_U | PAD_D)) == PAD_U) lizard.vy = RIVER_UP;
		else if ((zp.gamepad & (PAD_U | PAD_D)) == PAD_D) lizard.vy = RIVER_DOWN;
	}

	lizard.x += lizard.vx;
	lizard.y += lizard.vy;

	lizard.x &= 0x00FFFF; // clear lizard.xh

	if (lizard_px < 15 ) lizard.x = 15  << 8;
	if (lizard_px > 239) lizard.x = 239 << 8;

	if (lizard_py < 104) lizard.y = 104 << 8;
	if (lizard_py > 188) lizard.y = 188 << 8;

	if (!lizard.wet) // no drag when flying
	{
		if (lizard.vx > 0)
		{
			lizard.vx -= RIVER_DRAG_X;
			if (lizard.vx < 0) lizard.vx = 0;
		}
		else if (lizard.vx < 0)
		{
			lizard.vx += RIVER_DRAG_X;
			if (lizard.vx > 0) lizard.vx = 0;
		}

		if (lizard.vy > 0)
		{
			lizard.vy -= RIVER_DRAG_Y;
			if (lizard.vy < 0) lizard.vy = 0;
		}
		else if (lizard.vy < 0)
		{
			lizard.vy += RIVER_DRAG_Y;
			if (lizard.vy > 0) lizard.vy = 0;
		}
		lizard.z = 0;
	}
	else // lizard.wet
	{
		lizard.z += lizard.vz;
		//NES_DEBUG("frame %d > %d + %d\n",zp.nmi_count,lizard.z,lizard.vz);
		if (lizard_pz >= 128)
		{
			lizard.z = 0;
			lizard.vz = 0;
			lizard.wet = 0;
			play_sound(SOUND_WATER);
		}
		else
		{
			lizard.vz += SURF_GRAVITY;
		}
	}

	// for blinking
	++lizard.anim;

	// update hitbox
	lizard.hitbox_x0 = (lizard_px + RIVER_HITBOX_L) & 0x00FF;
	lizard.hitbox_x1 = (lizard_px + RIVER_HITBOX_R) & 0x00FF;
	lizard.hitbox_y0 =  lizard_py + RIVER_HITBOX_T;
	lizard.hitbox_y1 =  lizard_py + RIVER_HITBOX_B;

	// debug
	PPU::debug_box(lizard.hitbox_x0,lizard.hitbox_y0,lizard.hitbox_x1,lizard.hitbox_y1);
}

void lizard_draw_river()
{
	uint8 dx = uint8(lizard_px);
	sprite_prepare(dx);

	// select sprite based on input (stored in lizard.skid)
	uint8 s = DATA_sprite2_lizard_surf_stand;
	if (     (lizard.skid & PAD_U) != 0) s = DATA_sprite2_lizard_surf_fly;
	else if ((lizard.skid & PAD_D) != 0) s = DATA_sprite2_lizard_surf_fall;
	else if ((lizard.skid & PAD_L) != 0) s = DATA_sprite2_lizard_surf_fly;

	if (lizard.wet)
	{
		if (lizard.vz >= 0) s = DATA_sprite2_lizard_surf_fly;
		else                s = DATA_sprite2_lizard_surf_fall;
		// todo if moving left, reverse it?
	}

	// blink
	if (s == DATA_sprite2_lizard_stand && ((lizard.anim & 255) >= 248))
	{
		s = DATA_sprite2_lizard_surf_blink;
	}

	if (lizard.big_head_mode & 0x80)
	{
		s += BHM_OFFSET;
		NES_ASSERT(s >= DATA_sprite2_lizard_bhm_surf_stand && s <= DATA_sprite2_lizard_bhm_surf_fall, "Big head sprite out of range?")
	}

	// draw the lizard
	uint8 dy = lizard_py - lizard_pz;
	sprite2_add(uint8(dx),dy,s);

	if (zp.current_lizard == LIZARD_OF_KNOWLEDGE && lizard.power > 0)
	{
		uint8 sy = dy - 26;
		if (lizard.big_head_mode & 0x80) sy -= 6;
		sprite_tile_add(dx-4,sy,0x25,0x01);
	}
}

} // namespace Game