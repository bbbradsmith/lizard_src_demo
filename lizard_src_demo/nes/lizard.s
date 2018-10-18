; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; lizard
;
; lizard update and drawing
;

.feature force_range
.macpack longbranch

.include "ram.s"
.export lizard_init
.export lizard_tick
.export lizard_draw
.export lizard_fall_test

.include "enums.s"
.include "sound_common.s"
.import flag_read
.import password_read
.import checkpoint
.import do_scroll

GRAVITY      = 60
JUMP_SCALE   = 1200
RUN_SCALE    = 500

WET_GRAVITY  = GRAVITY / 2

START_JUMP   = -JUMP_SCALE
LOW_JUMP     = -JUMP_SCALE / 2
WET_JUMP     = -JUMP_SCALE / 2
RELEASE_JUMP = -JUMP_SCALE / 5

RUN_RIGHT    = RUN_SCALE / 10
WALK_RIGHT   = RUN_SCALE / 30
MAX_RIGHT    = RUN_SCALE
MAXW_RIGHT   = RUN_SCALE * 25 / 64
SKID_RIGHT   = RUN_SCALE * 9 / 64
AIR_RIGHT    = RUN_SCALE * 2 / 64
AIRW_RIGHT   = RUN_SCALE * 1 / 128
AIRS_RIGHT   = RUN_SCALE * 2 / 64
DRAG_RIGHT   = RUN_SCALE * 9 / 64
MAX_DOWN     = JUMP_SCALE * 20 / 16
MAX_DOWN_WET = JUMP_SCALE * 8 / 16

RUN_LEFT     = -RUN_RIGHT
WALK_LEFT    = -WALK_RIGHT
MAX_LEFT     = -MAX_RIGHT
MAXW_LEFT    = -MAXW_RIGHT
AIR_LEFT     = -AIR_RIGHT
AIRW_LEFT    = -AIRW_RIGHT
AIRS_LEFT    = -AIRS_RIGHT
DRAG_LEFT    = -DRAG_RIGHT
SKID_LEFT    = -SKID_RIGHT
MAX_UP       = -MAX_DOWN
MAX_UP_WET   = -MAX_DOWN_WET
MAX_UP_END   = -JUMP_SCALE * 4 / 16

FLOW         = RUN_SCALE * 7 / 64
FLOW_MAX     = RUN_SCALE / 2

SWIM_UP      = -JUMP_SCALE * 9 / 16
SWIM_RIGHT   = RUN_SCALE * 10 / 64
SWIM_LEFT    = -SWIM_RIGHT
SWIM_TIMEOUT = 6

SURF_SKIP    = -JUMP_SCALE * 8 / 16
SURF_RIGHT   = RUN_SCALE * 35 / 64
SURF_LEFT    = -SURF_RIGHT
SURF_JUMP    = 500
SURF_GRAVITY = -32
SURF_RELEASE = SURF_JUMP / 5

BOUNCE_PASS  = 4 * GRAVITY
BOUNCE_LOSS  = GRAVITY
BEYOND_FLOAT = -GRAVITY / 2

HITBOX_R =  4
HITBOX_L = -5
HITBOX_T = -14
HITBOX_M = -7
HITBOX_B = -1

RIVER_RIGHT  = 256
RIVER_LEFT   = -RIVER_RIGHT
RIVER_DOWN   = 256
RIVER_UP     = -RIVER_DOWN
RIVER_DRAG_X = 6
RIVER_DRAG_Y = 10

RIVER_HITBOX_R =  5
RIVER_HITBOX_L = -8
RIVER_HITBOX_T = -5
RIVER_HITBOX_B =  0

.include "../assets/export/names.s"
.import sprite_prepare
.import sprite_add
.import sprite_add_flipped
.import sprite_tile_add
.import palette_load
.import collide_tile
.import collide_all
.import collide_all_left
.import collide_all_right
.import collide_all_up
.import collide_all_down
.import prng

sprite2_add         = sprite_add
sprite2_add_flipped = sprite_add_flipped

.segment "CODE"

BHM_OFFSET = DATA_sprite2_lizard_bhm_stand - DATA_sprite2_lizard_stand

.assert (DATA_sprite2_lizard_stand      + BHM_OFFSET = DATA_sprite2_lizard_bhm_stand),      error,"big head sprites out of order!"
.assert (DATA_sprite2_lizard_blink      + BHM_OFFSET = DATA_sprite2_lizard_bhm_blink),      error,"big head sprites out of order!"
.assert (DATA_sprite2_lizard_walk0      + BHM_OFFSET = DATA_sprite2_lizard_bhm_walk0),      error,"big head sprites out of order!"
.assert (DATA_sprite2_lizard_walk1      + BHM_OFFSET = DATA_sprite2_lizard_bhm_walk1),      error,"big head sprites out of order!"
.assert (DATA_sprite2_lizard_walk2      + BHM_OFFSET = DATA_sprite2_lizard_bhm_walk2),      error,"big head sprites out of order!"
.assert (DATA_sprite2_lizard_walk3      + BHM_OFFSET = DATA_sprite2_lizard_bhm_walk3),      error,"big head sprites out of order!"
.assert (DATA_sprite2_lizard_skid       + BHM_OFFSET = DATA_sprite2_lizard_bhm_skid),       error,"big head sprites out of order!"
.assert (DATA_sprite2_lizard_fly        + BHM_OFFSET = DATA_sprite2_lizard_bhm_fly),        error,"big head sprites out of order!"
.assert (DATA_sprite2_lizard_fall       + BHM_OFFSET = DATA_sprite2_lizard_bhm_fall),       error,"big head sprites out of order!"
.assert (DATA_sprite2_lizard_skull      + BHM_OFFSET = DATA_sprite2_lizard_bhm_skull),      error,"big head sprites out of order!"
.assert (DATA_sprite2_lizard_smoke      + BHM_OFFSET = DATA_sprite2_lizard_bhm_smoke),      error,"big head sprites out of order!"
.assert (DATA_sprite2_lizard_surf_stand + BHM_OFFSET = DATA_sprite2_lizard_bhm_surf_stand), error,"big head sprites out of order!"
.assert (DATA_sprite2_lizard_surf_blink + BHM_OFFSET = DATA_sprite2_lizard_bhm_surf_blink), error,"big head sprites out of order!"
.assert (DATA_sprite2_lizard_surf_fly   + BHM_OFFSET = DATA_sprite2_lizard_bhm_surf_fly),   error,"big head sprites out of order!"
.assert (DATA_sprite2_lizard_surf_fall  + BHM_OFFSET = DATA_sprite2_lizard_bhm_surf_fall),  error,"big head sprites out of order!"

; sign extend: (v & $80) ? -1 : 0
.define SIGN8(v) (0-((v>>7)&1))

; ==============
; lizard helpers
; ==============
stat_lizard_helpers:

lizard_fall_test:
	; test if on ground
	ldy lizard_y+0
	ldx lizard_x+0
	lda lizard_xh
	sta ih
	jsr collide_all
	bne @on_floor
	ldy lizard_y+0
	lda lizard_x+0
	clc
	adc #HITBOX_L
	tax
	lda lizard_xh
	adc #SIGN8 HITBOX_L
	sta ih
	jsr collide_all
	bne @on_floor
	ldy lizard_y+0
	lda lizard_x+0
	clc
	adc #HITBOX_R
	tax
	lda lizard_xh
	adc #SIGN8 HITBOX_R
	sta ih
	jsr collide_all
	bne @on_floor
		; not on floor
		lda #1
		sta lizard_fall
		rts
	@on_floor:
		lda current_lizard
		cmp #LIZARD_OF_STONE
		bne :+
		lda lizard_power
		beq :+
		lda lizard_fall
		beq :+
		lda lizard_flow
		bne :+
			PLAY_SOUND SOUND_STONE
		:
		lda #0
		sta lizard_fall
		rts
	;

; ===========
; lizard_init
; ===========
stat_lizard_init:

; i,j,k = x,y,xh door location
lizard_init:
	ldx #0
	lda hold_y
	bne :+
		; door position
		lda k
		sta lizard_xh
		lda i
		sta lizard_x+0
		stx lizard_x+1
		jmp @hold_y_end
	:
		; clamp to edge
		lda dog_type+11
		cmp #DOG_PASS_X
		bne :+
		lda lizard_x+0
		cmp #<13
		lda lizard_xh
		sbc #>13
		bcs :+
			stx lizard_x+1
			lda #<13
			sta lizard_x+0
			lda #>13
			sta lizard_xh
		:
		lda dog_type+12
		cmp #DOG_PASS_X
		bne @clamp_x_end
		lda room_scrolling
		beq :++
			lda lizard_x+0
			cmp #<500
			lda lizard_xh
			sbc #>500
			bcc :+
				stx lizard_x+1
				lda #<409
				sta lizard_x+0
				lda #>409
				sta lizard_xh
			:
			jmp @hold_y_end
		:
			lda lizard_x+0
			cmp #<244
			lda lizard_xh
			sbc #>244
			bcc :+
				stx lizard_x+1
				lda #<243
				sta lizard_x+0
				lda #>243
				sta lizard_xh
			:
		@clamp_x_end:
	@hold_y_end:
	lda hold_x
	bne :+
		; door position
		lda j
		sta lizard_y+0
		stx lizard_y+1
		jmp @hold_x_end
	:
		; clamp to edge
		lda dog_type+13
		cmp #DOG_PASS_Y
		bne :+
		lda lizard_y+0
		cmp #22
		bcs :+
			stx lizard_y+1
			lda #22
			sta lizard_y+0
		:
		lda dog_type+14
		cmp #DOG_PASS_Y
		beq :+
		lda dog_type+15
		cmp #DOG_PASS_Y
		beq :+
			jmp :++
		:
		lda lizard_y+0
		cmp #232
		bcc :+
			stx lizard_y+1
			lda #232
			sta lizard_y+0
		:
	@hold_x_end:
	lda hold_x
	ora hold_y
	bne :+
		stx lizard_vx+0
		stx lizard_vx+1
		stx lizard_vy+0
		stx lizard_vy+1
	:
	stx lizard_z+0
	stx lizard_z+1
	stx lizard_vz+0
	stx lizard_vz+1
	stx lizard_anim+0
	stx lizard_anim+1
	stx lizard_scuttle
	stx lizard_skid
	stx lizard_dismount
	stx lizard_dead
	;stx lizard_power
	stx lizard_moving
	stx lizard_flow
	stx lizard_wet
	stx hold_x
	stx hold_y
	lda #255
	sta smoke_xh
	sta smoke_x
	sta smoke_y
	lda lizard_y+0
	cmp water
	bcc :+
		lda #1
		sta lizard_wet
	:
	;jsr lizard_fall_test
	jmp do_scroll

; ===========
; lizard_tick
; ===========
stat_lizard_tick:

lizard_move:
	; cap horizontal velocity
	; if lizard vx < MAX_LEFT
	lda lizard_vx+1
	cmp #<MAX_LEFT
	lda lizard_vx+0
	sbc #>MAX_LEFT
	bvc :+
	eor #$80
	:
	bpl :+
		lda #<MAX_LEFT
		sta lizard_vx+1
		lda #>MAX_LEFT
		sta lizard_vx+0
		jmp @move_horizontal
	:
	; if lizard vx > MAX_RIGHT
	lda #<MAX_RIGHT
	cmp lizard_vx+1
	lda #>MAX_RIGHT
	sbc lizard_vx+0
	bvc :+
	eor #$80
	:
	bpl :+
		lda #<MAX_RIGHT
		sta lizard_vx+1
		lda #>MAX_RIGHT
		sta lizard_vx+0
	:
@move_horizontal:
	; move horizontally and collide
	lda lizard_x+1
	clc
	adc lizard_vx+1
	sta lizard_x+1
	lda lizard_x+0
	adc lizard_vx+0
	sta lizard_x+0
	; sign-extend velocity add to 24 bits
	lda lizard_vx+0
	bmi :+
		lda lizard_xh
		adc #0
		sta lizard_xh
		jmp :++
	:
		lda lizard_xh
		adc #$FF
		sta lizard_xh
	:
	; check for collision
	lda lizard_vx+0
	bmi @collide_left ; vx < 0
	ora lizard_vx+1
	jeq @collide_x_end ; vx == 0
	@collide_right:
		lda lizard_x+0
		clc
		adc #HITBOX_R
		tax
		lda lizard_xh
		adc #SIGN8 HITBOX_R
		sta ih
		lda lizard_y+0
		clc
		adc #HITBOX_B
		tay
		jsr collide_all_right
		beq :+
			lda lizard_x+0
			sec
			sbc j
			sta lizard_x+0 ; lizard_x -= shift
			lda lizard_xh
			sbc #0
			sta lizard_xh
			lda #$FF
			sta lizard_x+1
			lda #0
			sta lizard_vx+0
			sta lizard_vx+1
		:
		; ih:X = lizard_x + HITBOX_R
		lda lizard_y+0
		clc
		adc #HITBOX_T
		tay
		jsr collide_all_right
		beq :+
			lda lizard_x+0
			sec
			sbc j
			sta lizard_x+0 ; lizard_x -= shift
			lda lizard_xh
			sbc #0
			sta lizard_xh
			lda #$FF
			sta lizard_x+1
			lda #0
			sta lizard_vx+0
			sta lizard_vx+1
		:
		; ih:X = lizard_x + HITBOX_R
		lda lizard_y+0
		clc
		adc #HITBOX_M
		tay
		jsr collide_all_right
		beq :+
			lda lizard_x+0
			sec
			sbc j
			sta lizard_x+0 ; lizard_x -= shift
			lda lizard_xh
			sbc #0
			sta lizard_xh
			lda #$FF
			sta lizard_x+1
			lda #0
			sta lizard_vx+0
			sta lizard_vx+1
		:
		jmp @collide_x_end
	;
	@collide_left:
		lda lizard_x+0
		clc
		adc #HITBOX_L
		tax
		lda lizard_xh
		adc #SIGN8 HITBOX_L
		sta ih
		lda lizard_y+0
		clc
		adc #HITBOX_B
		tay
		jsr collide_all_left
		beq :+
			clc
			adc lizard_x+0
			sta lizard_x+0
			lda lizard_xh
			adc #0
			sta lizard_xh
			lda #0
			sta lizard_x+1
			sta lizard_vx+0
			sta lizard_vx+1
		:
		; ih:X = lizard_x + HITBOX_L
		lda lizard_y+0
		clc
		adc #HITBOX_T
		tay
		jsr collide_all_left
		beq :+
			clc
			adc lizard_x+0
			sta lizard_x+0
			lda lizard_xh
			adc #0
			sta lizard_xh
			lda #0
			sta lizard_x+1
			sta lizard_vx+0
			sta lizard_vx+1
		:
		; ih:X = lizard_x + HITBOX_L
		lda lizard_y+0
		clc
		adc #HITBOX_M
		tay
		jsr collide_all_left
		beq :+
			clc
			adc lizard_x+0
			sta lizard_x+0
			lda lizard_xh
			adc #0
			sta lizard_xh
			lda #0
			sta lizard_x+1
			sta lizard_vx+0
			sta lizard_vx+1
		:
		;jmp @collide_x_end
	;
	@collide_x_end:
	; beyond
	lda current_lizard
	cmp #LIZARD_OF_BEYOND
	bne @beyond_end
	lda lizard_power
	beq @beyond_end
		lda lizard_vy+1
		clc
		adc #<BEYOND_FLOAT
		sta lizard_vy+1
		lda lizard_vy+0
		adc #>BEYOND_FLOAT
		sta lizard_vy+0
		jsr lizard_fall_test
		lda lizard_fall
		bne @beyond_ground_end
			lda lizard_vy+0
			bmi :+
				lda #0
				sta lizard_vy+0
				sta lizard_vy+1
				lda #$FF
				sta lizard_y+1
			:
		@beyond_ground_end:
		jmp @skip_gravity
	@beyond_end:
	; test if falling, apply gravity
	jsr lizard_fall_test
	; check if falling
	lda lizard_fall
	bne @apply_gravity
		; if current_lizard == LIZARD_OF_BOUNCE
		; && lizard.power != 0
		; && lizard.vy > BOUNCE_PASS
		lda current_lizard
		cmp #LIZARD_OF_BOUNCE
		bne @on_ground
		lda lizard_power
		beq @on_ground
		lda #<BOUNCE_PASS
		cmp lizard_vy+1
		lda #>BOUNCE_PASS
		sbc lizard_vy+0
		bvc :+
		eor #$80
		:
		bpl @on_ground
			; vy = -vy + BOUNCE_LOSS
			lda #<BOUNCE_LOSS
			sec
			sbc lizard_vy+1
			sta lizard_vy+1
			lda #>BOUNCE_LOSS
			sbc lizard_vy+0
			sta lizard_vy+0
			lda lizard_wet
			beq :+
				; lizard_vy >>= 1
				lda lizard_vy+0
				rol ; put sign in carry
				ror lizard_vy+0
				ror lizard_vy+1
			:
			lda #1
			sta lizard_fall
			sta bounced
			dec lizard_y+0 ; y -= 256
			PLAY_SOUND SOUND_BOUNCE
			jmp @apply_gravity
		@on_ground:
			lda lizard_vy+0
			bmi :+ ; if lizard_vy >= 0
				lda #0
				sta lizard_vy+1
				sta lizard_vy+0
				lda #$FF
				sta lizard_y+1
				jmp @skip_gravity
			:
		;
	@apply_gravity:
		lda lizard_wet
		beq :+
			lda lizard_vy+1
			clc
			adc #<WET_GRAVITY
			sta lizard_vy+1
			lda lizard_vy+0
			adc #>WET_GRAVITY
			sta lizard_vy+0
			jmp @skip_gravity
		:
			lda lizard_vy+1
			clc
			adc #<GRAVITY
			sta lizard_vy+1
			lda lizard_vy+0
			adc #>GRAVITY
			sta lizard_vy+0
		;
	@skip_gravity:
	; cap vertical velocity
	lda lizard_wet
	bne @vy_cap_wet
	@vy_cap_dry:
		; if lizard vy < MAX_UP
		lda lizard_vy+1
		cmp #<MAX_UP
		lda lizard_vy+0
		sbc #>MAX_UP
		bvc :+
		eor #$80
		:
		bpl :+
			lda #<MAX_UP
			sta lizard_vy+1
			lda #>MAX_UP
			sta lizard_vy+0
			jmp @move_vertical
		:
		; if lizard vy > MAX_DOWN
		lda #<MAX_DOWN
		cmp lizard_vy+1
		lda #>MAX_DOWN
		sbc lizard_vy+0
		bvc :+
		eor #$80
		:
		bpl :+
			lda #<MAX_DOWN
			sta lizard_vy+1
			lda #>MAX_DOWN
			sta lizard_vy+0
		:
		jmp @move_vertical
	@vy_cap_wet:
		; if lizard vy < MAX_UP_WET
		lda lizard_vy+1
		cmp #<MAX_UP_WET
		lda lizard_vy+0
		sbc #>MAX_UP_WET
		bvc :+
		eor #$80
		:
		bpl :+
			lda #<MAX_UP_WET
			sta lizard_vy+1
			lda #>MAX_UP_WET
			sta lizard_vy+0
			jmp @move_vertical
		:
		; if lizard vy > MAX_DOWN_WET
		lda #<MAX_DOWN_WET
		cmp lizard_vy+1
		lda #>MAX_DOWN_WET
		sbc lizard_vy+0
		bvc :+
		eor #$80
		:
		bpl :+
			lda #<MAX_DOWN_WET
			sta lizard_vy+1
			lda #>MAX_DOWN_WET
			sta lizard_vy+0
		:
	;
@move_vertical:
	; move vertically and collide
	lda lizard_y+1
	clc
	adc lizard_vy+1
	sta lizard_y+1
	lda lizard_y+0
	adc lizard_vy+0
	sta lizard_y+0
	lda lizard_vy+0
	jmi @collide_up ; vy < 0
	ora	lizard_vy+1
	bne @collide_down
		jmp @collide_y_end ; by == 0
	@collide_down:
		lda #0
		sta m ; m = landed
		lda lizard_y+0
		clc
		adc #HITBOX_B
		tay
		ldx lizard_x+0
		lda lizard_xh
		sta ih
		jsr collide_all_down
		beq :+
			lda lizard_y+0
			sec
			sbc j
			sta lizard_y+0 ; lizard_y -= shift
			lda #$FF
			sta lizard_y+1
			lda #1
			sta m ; landed = true
		:
		; Y = lizard_y+HITBOX_B
		lda lizard_x+0
		clc
		adc #HITBOX_L
		tax
		lda lizard_xh
		adc #SIGN8 HITBOX_L
		sta ih
		jsr collide_all_down
		beq :+
			lda lizard_y+0
			sec
			sbc j
			sta lizard_y+0 ; lizard_y -= shift
			lda #$FF
			sta lizard_y+1
			lda #1
			sta m ; landed = true
		:
		; Y = lizard_y+HITBOX_B
		lda lizard_x+0
		clc
		adc #HITBOX_R
		tax
		lda lizard_xh
		adc #SIGN8 HITBOX_R
		sta ih
		jsr collide_all_down
		beq :+
			lda lizard_y+0
			sec
			sbc j
			sta lizard_y+0 ; lizard_y -= shift
			lda #$FF
			sta lizard_y+1
			lda #1
			sta m ; landed = true
		:
		; if landed
		lda m
		beq @landed_end
			; if current_lizard==LIZARD_OF_STONE && lizard_power>0 && lizard_fall
			lda current_lizard
			cmp #LIZARD_OF_STONE
			bne :+
			lda lizard_power
			beq :+
			lda lizard_fall
			beq :+
			lda lizard_flow
			bne :+
				PLAY_SOUND SOUND_STONE
			:
			; lizard_fall = 0
			lda #0
			sta lizard_fall
		@landed_end:
		jmp @collide_y_end
	;
	@collide_up:
		lda lizard_y+0
		clc
		adc #HITBOX_T
		tay
		ldx lizard_x+0
		lda lizard_xh
		sta ih
		jsr collide_all_up
		beq :+
			clc
			adc lizard_y+0
			sta lizard_y+0
			lda #0
			sta lizard_y+1
			sta lizard_vy+0
			sta lizard_vy+1
		:
		; Y = lizard_y + HITBOX_T
		lda lizard_x+0
		clc
		adc #HITBOX_L
		tax
		lda lizard_xh
		adc #SIGN8 HITBOX_L
		sta ih
		jsr collide_all_up
		beq :+
			clc
			adc lizard_y+0
			sta lizard_y+0
			lda #0
			sta lizard_y+1
			sta lizard_vy+0
			sta lizard_vy+1
		:
		; Y = lizard_y + HITBOX_T
		lda lizard_x+0
		clc
		adc #HITBOX_R
		tax
		lda lizard_xh
		adc #SIGN8 HITBOX_R
		sta ih
		jsr collide_all_up
		beq :+
			clc
			adc lizard_y+0
			sta lizard_y+0
			lda #0
			sta lizard_y+1
			sta lizard_vy+0
			sta lizard_vy+1
		:
		;jmp @collide_y_end
	;
	@collide_y_end:
	; check for water
	lda water
	cmp #240
	bcs @no_water
	lda lizard_y+0
	cmp water
	bcc @no_water
		lda lizard_wet
		bne @already_wet
			PLAY_SOUND SOUND_WATER
			lda dog_type+SPLASHER_SLOT
			cmp #DOG_SPLASHER
			bne :+
				lda lizard_x+0
				sec
				sbc #4
				sta dog_x+SPLASHER_SLOT
				lda lizard_xh
				sbc #0
				sta dog_xh+SPLASHER_SLOT
				lda water
				sec
				sbc #9
				sta dog_y+SPLASHER_SLOT
				lda #0
				sta dog_data0+SPLASHER_SLOT ; trigger splash animation
			:
			; surf board can skim the water if going fast enoguh
			lda current_lizard
			cmp #LIZARD_OF_SURF
			bne @no_skim
			lda lizard_power
			beq @no_skim
			; if vx>SURF_RIGHT || vx<SURF_LEFT test
			lda #<SURF_RIGHT
			cmp lizard_vx+1
			lda #>SURF_RIGHT
			sbc lizard_vx+0
			bvc :+
			eor #$80
			:
			bmi @skim ; vx > SURF_RIGHT
			lda lizard_vx+1
			cmp #<SURF_LEFT
			lda lizard_vx+0
			sbc #>SURF_LEFT
			bvc :+
			eor #$80
			:
			bmi @skim ; vx < SURF_LEFT
			jmp @no_skim
			@skim:
				lda #<SURF_SKIP
				sta lizard_vy+1
				lda #>SURF_SKIP
				sta lizard_vy+0
				jmp @water_end
			@no_skim:
		@already_wet:
		lda #1
		sta lizard_wet
		jmp @water_end
	@no_water:
		lda lizard_wet
		beq @already_dry
			PLAY_SOUND SOUND_WATER
			lda dog_type+SPLASHER_SLOT
			cmp #DOG_SPLASHER
			bne :+
				lda lizard_x
				sec
				sbc #4
				sta dog_x+SPLASHER_SLOT
				lda lizard_xh
				sbc #0
				sta dog_xh+SPLASHER_SLOT
				lda water
				sec
				sbc #9
				sta dog_y+SPLASHER_SLOT
				lda #0
				sta dog_data0+SPLASHER_SLOT ; trigger splash animation
			:
		@already_dry:
		lda #0
		sta lizard_wet
	@water_end:
	lda #0
	sta lizard_flow
	jmp do_scroll
	;rts

.import lizard_die
; defined in common.s

lizard_jump_finish:
	PLAY_SOUND SOUND_JUMP
	; increment 32-bit jumps counter
	inc metric_jumps+0
	bne :+
	inc metric_jumps+1
	bne :+
	inc metric_jumps+2
	bne :+
	inc metric_jumps+3
	:
lizard_big_head_mode_test:
	; big head mode
	; trigger by jumping 8 times, alternating holding UP and DOWN
	lda lizard_big_head_mode
	and #1
	bne @bhm_odd
	@bhm_even:
		lda gamepad
		and #PAD_U
		beq @bhm_no
		inc lizard_big_head_mode
		jmp @bhm_end
	@bhm_odd:
		lda gamepad
		and #PAD_D
		beq @bhm_no
		inc lizard_big_head_mode
		lda lizard_big_head_mode
		and #$7F
		cmp #8
		bcc @bhm_end
			; toggle big head mode
			lda lizard_big_head_mode
			eor #$80
			and #$80
			sta lizard_big_head_mode
			PLAY_SOUND SOUND_SECRET
			jmp @bhm_end
		;
	@bhm_no:
		lda lizard_big_head_mode
		and #$80
		sta lizard_big_head_mode
		; jmp @bhm_end
	@bhm_end:
	rts

lizard_dead_tick:
	; clear touch boxes
	lda #127
	sta lizard_touch_xh0
	sta lizard_touch_xh1
	lda #255
	sta lizard_touch_x0
	sta lizard_touch_x1
	sta lizard_touch_y0
	sta lizard_touch_y1
	; lizard.x += lizard.vx
	lda lizard_x+1
	clc
	adc lizard_vx+1
	sta lizard_x+1
	lda lizard_x+0
	adc lizard_vx+0
	sta lizard_x+0
	; sign-extend velocity add to 24 bits
	lda lizard_vx+0
	bmi :+
		lda lizard_xh
		adc #0
		sta lizard_xh
		jmp :++
	:
		lda lizard_xh
		adc #$FF
		sta lizard_xh
	:
	; wrap
	lda room_scrolling
	bne :+
		lda lizard_xh
		beq @done_wrap
		lda #0
		sta lizard_xh
		jmp @wrapped
	:
		lda lizard_xh
		cmp #2
		bcc @done_wrap
		and #1
		sta lizard_xh
		;jmp @wrapped
	@wrapped:
		lda #255
		sta lizard_y+0
		lda #0
		sta lizard_y+1
		sta lizard_vy+0
		sta lizard_vy+1
	@done_wrap:
	; calculate projected lizard.y position
	lda lizard_y+1
	clc
	adc lizard_vy+1
	sta j
	lda lizard_y+0
	adc lizard_vy+0
	sta i
	; if lizard.vy < 0 || temp(ij) >= lizard.y
	lda lizard_vy+0
	bmi :+
		; lizard.vy >= 0
		lda i
		cmp lizard_y+0
		bcs @vertical_move
		jmp @vertical_wrap
	:
		; lizard.vy < 0
		lda lizard_y+0
		cmp i
		bcs @vertical_move
		;jmp @vertical_wrap
	@vertical_wrap:
		lda #255
		sta lizard_y+0
		lda #0
		sta lizard_y+1
		sta lizard_vy+0
		sta lizard_vy+1
		jmp @vertical_end
	@vertical_move:
		lda i
		sta lizard_y+0
		lda j
		sta lizard_y+1
		; lizard.vy += GRAVITY
		lda lizard_vy+1
		clc
		adc #<GRAVITY
		sta lizard_vy+1
		lda lizard_vy+0
		adc #>GRAVITY
		sta lizard_vy+0
	@vertical_end:
	lda lizard_power
	cmp #60
	bcs :+
		inc lizard_power
		rts
	:
		; don't allow restart until gamepad is released
		lda gamepad
		bne :+
			lda #61
			sta lizard_power
			rts
		:
		; restart once gamepad has been released AND a suitable button is pressed
		lda lizard_power
		cmp #61
		bcs :+
			rts
		:
		lda gamepad
		and #(PAD_A|PAD_B|PAD_START)
		bne :+
			rts
		:
		jsr password_read
		beq @bad_password
		lda continued
		beq @bad_password
		jsr checkpoint
		bne @good_password
		@bad_password:
			; invalid password, go back to start
			lda #<DATA_room_start
			sta i
			lda #LIZARD_OF_START
			sta j
			lda #>DATA_room_start
			sta k
		@good_password:
		lda i
		sta current_room+0
		lda k
		sta current_room+1
		lda j
		sta next_lizard
		lda #0
		sta lizard_power
		sta current_door
		lda #FLAG_EYESIGHT
		jsr flag_read
		sta t ; t = eye_old
		; restore flags/coins
		ldx #0
		:
			lda coin_save, X
			sta coin, X
			lda flag_save, X
			sta flag, X
			inx
			cpx #16
			bcc :-
		lda piggy_bank_save
		sta piggy_bank
		lda last_lizard_save
		sta last_lizard
		lda #1
		sta coin_saved
		lda #FLAG_EYESIGHT
		jsr flag_read
		cmp t
		beq :+
			lda #$FF
			.repeat 8,I
				sta chr_cache+I
			.endrepeat
		:
		;inc tip_counter ; disabled for demo
		nop
		nop
		nop
		lda #1
		sta room_change
		rts
	;

lizard_tick:
	; river mode
	lda dog_type+0
	cmp #DOG_RIVER
	jeq lizard_tick_river
	; if dead, there is a different tick
	lda lizard_dead
	jne lizard_dead_tick
	; dismount overrides controls
	lda lizard_dismount
	beq @no_dismount
		cmp #1
		bne :+
			; if dismounting make sure there is no scuttle, and prevent other input
			lda #0
			sta lizard_scuttle
			rts
		:
		cmp #2
		bne :+
			; river exit (forced surf)
			lda #0
			sta lizard_dismount
			lda #(PAD_A | PAD_B | PAD_R)
			sta gamepad
		:
		cmp #3
		bne @forced_fly_end
			lda lizard_vy+1
			cmp #<MAX_UP_END
			lda lizard_vy+0
			sbc #>MAX_UP_END
			bvc :+
			eor #$80
			:
			bpl :+
				lda #<MAX_UP_END
				sta lizard_vy+1
				lda #>MAX_UP_END
				sta lizard_vy+0
			:
			lda #(PAD_D | PAD_B)
			sta gamepad
			lda #0
			sta lizard_dismount
		@forced_fly_end:
	@no_dismount:
	; hold select to scuttle lizard
	lda gamepad
	and #PAD_SELECT
	beq :+
		inc lizard_scuttle
		lda lizard_scuttle
		cmp #255
		jeq lizard_die
		jmp :++
	: ; else
		lda #0
		sta lizard_scuttle
	:
	; press start to pause
	lda gamepad
	and #PAD_START
	beq :++
		lda mode_temp
		beq :+
			lda #2
			sta text_select ; pause
			lda #1
			sta game_pause
			rts
		:
		jmp :++
	:
		lda #1
		sta mode_temp
	:
	; stone mode negates any movement controls, immediately halts skid on ground
	lda current_lizard
	cmp #LIZARD_OF_STONE
	bne @no_stone
	lda lizard_power
	beq @no_stone
		lda gamepad
		and #(PAD_B | PAD_SELECT | PAD_START)
		sta gamepad
		lda lizard_fall
		bne :+ ; halt skid if on ground
			lda #0
			sta lizard_vx+0
			sta lizard_vx+1
		:
	@no_stone:
	; handle jump request
	lda gamepad
	and #PAD_A
	bne @want_jump
		; if lizard_jump && lizard_vy < RELEASE_JUMP allow user to release
		lda lizard_jump
		beq :++
		lda lizard_vy+1
		cmp #<RELEASE_JUMP
		lda lizard_vy+0
		sbc #>RELEASE_JUMP
		bvc :+
		eor #$80
		:
		bpl :+
			lda #<RELEASE_JUMP
			sta lizard_vy+1
			lda #>RELEASE_JUMP
			sta lizard_vy+0
		:
		lda #0
		sta lizard_jump ; jump flag is clear, can now jump again (when on ground)
		jmp @jump_end
	@want_jump:
		lda lizard_fall
		bne @jump_end ; can't jump if falling
		lda lizard_jump
		bne @jump_end ; not a new jump request, don't keep jumping
		; don't jump if trying to bounce
		lda current_lizard
		cmp #LIZARD_OF_BOUNCE
		bne :+
		lda lizard_power
		bne @jump_end
		:
		; begin the jump by setting upward velocity
		lda lizard_wet
		beq :+
			lda #>WET_JUMP
			sta lizard_vy+0
			lda #<WET_JUMP
			sta lizard_vy+1
			jmp :++
		:
			lda #>START_JUMP
			sta lizard_vy+0
			lda #<START_JUMP
			sta lizard_vy+1
		:
		; coffee or lounge powers
		lda lizard_power
		cmp #31
		bcc @no_cl_power
			lda current_lizard
			cmp #LIZARD_OF_COFFEE
			bne :+
				lda #>MAX_UP
				sta lizard_vy+0
				lda #<MAX_UP
				sta lizard_vy+1
				jmp @no_cl_power
			:
			cmp #LIZARD_OF_LOUNGE
			bne :+
				lda #>LOW_JUMP
				sta lizard_vy+0
				lda #<LOW_JUMP
				sta lizard_vy+1
			:
		@no_cl_power:
		; set flags for jump
		lda #1
		sta lizard_fall
		sta lizard_jump
		lda #0
		sta lizard_skid
		jsr lizard_jump_finish
	@jump_end:
	; control movement
	lda lizard_fall
	jne @control_air
		; ground movement requests
		lda gamepad
		and #(PAD_L|PAD_R)
		cmp #PAD_R
		bne @end_right
			; accelerate right
			; if !lizard_face or lizard_vx < 0 enter skid
			lda lizard_face
			beq :+
			lda lizard_vx+0
			bpl :+
				lda #1
				sta lizard_skid
				jmp @skid
			:
			; else facing the correct direction, accelerate
			lda #0
			sta lizard_skid
			sta lizard_face
			lda gamepad
			and #PAD_D
			beq @right_full_speed
				; if lizard.vx < MAXW_RIGHT
				lda lizard_vx+1
				cmp #<MAXW_RIGHT
				lda lizard_vx+0
				sbc #>MAXW_RIGHT
				bvc :+
				eor #$80
				:
				bpl :+
					; < MAXW_RIGHT
					lda lizard_vx+1
					clc
					adc #<WALK_RIGHT
					sta lizard_vx+1
					lda lizard_vx+0
					adc #>WALK_RIGHT
					sta lizard_vx+0
					jmp @skid
				:
					; >= MAXW_RIGHT
					lda lizard_vx+1
					clc
					adc #<DRAG_LEFT
					sta lizard_vx+1
					lda lizard_vx+0
					adc #>DRAG_LEFT
					sta lizard_vx+0
					jmp @skid
				;
			@right_full_speed:
				lda lizard_vx+1
				clc
				adc #<RUN_RIGHT
				sta lizard_vx+1
				lda lizard_vx+0
				adc #>RUN_RIGHT
				sta lizard_vx+0
				jmp @skid
			;
		@end_right:
		cmp #PAD_L
		bne @end_left
			; if !lizard_face or lizard_vx > 0 enter skid
			lda lizard_face
			bne :+
			lda lizard_vx+0
			bmi :+
			ora lizard_vx+1
			beq :+
				lda #1
				sta lizard_skid
				jmp @skid
			:
			; else facing the correct direction, accelerate
			lda #0
			sta lizard_skid
			lda #1
			sta lizard_face
			lda gamepad
			and #PAD_D
			beq @left_full_speed
				; if lizard.vx > MAXW_LEFT
				lda #<MAXW_LEFT
				cmp lizard_vx+1
				lda #>MAXW_LEFT
				sbc lizard_vx+0
				bvc :+
				eor #$80
				:
				bpl :+
					; < MAXW_LEFT
					lda lizard_vx+1
					clc
					adc #<WALK_LEFT
					sta lizard_vx+1
					lda lizard_vx+0
					adc #>WALK_LEFT
					sta lizard_vx+0
					jmp @skid
				:
					; >= MAXW_LEFT
					lda lizard_vx+1
					clc
					adc #<DRAG_RIGHT
					sta lizard_vx+1
					lda lizard_vx+0
					adc #>DRAG_RIGHT
					sta lizard_vx+0
					jmp @skid
				;
			@left_full_speed:
				lda lizard_vx+1
				clc
				adc #<RUN_LEFT
				sta lizard_vx+1
				lda lizard_vx+0
				adc #>RUN_LEFT
				sta lizard_vx+0
				jmp @skid
			;
		@end_left:
		; pressing down initiates skid
		lda gamepad
		and #(PAD_U|PAD_D|PAD_L|PAD_R)
		cmp #PAD_D
		bne :+
			lda #1
			sta lizard_skid
		:
		; drag if not skidding
		lda lizard_skid
		bne @skid
			lda lizard_flow
			jne @control_end ; lizard flow cancels drag
			lda lizard_vx+0
			ora lizard_vx+1
			beq @skid ; lizard_vx == 0
			lda lizard_vx+0
			bmi @drag_right
				; lizard_vx > 0
				lda lizard_vx+1
				clc
				adc #<DRAG_LEFT
				sta lizard_vx+1
				lda lizard_vx+0
				adc #>DRAG_LEFT
				sta lizard_vx+0
				bpl :+
					; result lizard_vx <= 0, halt
					lda #0
					sta lizard_vx+0
					sta lizard_vx+1
				:
				jmp @control_end
			@drag_right:
				; lizard_vx < 0
				lda lizard_vx+1
				clc
				adc #<DRAG_RIGHT
				sta lizard_vx+1
				lda lizard_vx+0
				adc #>DRAG_RIGHT
				sta lizard_vx+0
				bmi :+
					; result lizard_vx >= 0, halt
					lda #0
					sta lizard_vx+0
					sta lizard_vx+1
				:
				jmp @control_end
			;
		@skid:
		lda lizard_skid
		jeq @control_end
			; skid
			lda lizard_face
			beq @skid_left
				lda lizard_vx+1
				clc
				adc #<SKID_RIGHT
				sta lizard_vx+1
				lda lizard_vx+0
				adc #>SKID_RIGHT
				sta lizard_vx+0
				bmi :+
				ora lizard_vx+1
				beq :+
					lda #0
					sta lizard_skid
					sta lizard_vx+0
					sta lizard_vx+1
				:
				jmp @control_end
			@skid_left:
				lda lizard_vx+1
				clc
				adc #<SKID_LEFT
				sta lizard_vx+1
				lda lizard_vx+0
				adc #>SKID_LEFT
				sta lizard_vx+0
				bpl :+
					lda #0
					sta lizard_skid
					sta lizard_vx+0
					sta lizard_vx+1
				:
				jmp @control_end
			;
		;
	@control_air:
		; else air control (if not holding down)
		lda gamepad
		and #PAD_D
		bne @air_walk
		;
		lda gamepad
		and #PAD_L
		beq :+
			lda lizard_vx+1
			clc
			adc #<AIR_LEFT
			sta lizard_vx+1
			lda lizard_vx+0
			adc #>AIR_LEFT
			sta lizard_vx+0
		:
		lda gamepad
		and #PAD_R
		beq :+
			lda lizard_vx+1
			clc
			adc #<AIR_RIGHT
			sta lizard_vx+1
			lda lizard_vx+0
			adc #>AIR_RIGHT
			sta lizard_vx+0
		:
		jmp @control_end
		@air_walk:
		lda gamepad
		and #PAD_L
		beq :+
			lda lizard_vx+1
			clc
			adc #<AIRW_LEFT
			sta lizard_vx+1
			lda lizard_vx+0
			adc #>AIRW_LEFT
			sta lizard_vx+0
		:
		lda gamepad
		and #PAD_R
		beq :+
			lda lizard_vx+1
			clc
			adc #<AIRW_RIGHT
			sta lizard_vx+1
			lda lizard_vx+0
			adc #>AIRW_RIGHT
			sta lizard_vx+0
		:
		lda gamepad
		and #(PAD_L|PAD_R)
		bne @air_stop_end
			lda lizard_vx+0
			bmi @air_stop_left
			@air_stop_right:
				lda lizard_vx+1
				clc
				adc #<AIRS_LEFT
				sta lizard_vx+1
				lda lizard_vx+0
				adc #>AIRS_LEFT
				sta lizard_vx+0
				bmi @air_stop_zero
				jmp @air_stop_end
			@air_stop_left:
				lda lizard_vx+1
				clc
				adc #<AIRS_RIGHT
				sta lizard_vx+1
				lda lizard_vx+0
				adc #>AIRS_RIGHT
				sta lizard_vx+0
				bpl @air_stop_zero
				jmp @air_stop_end
			;
			@air_stop_zero:
				lda #0
				sta lizard_vx+0
				sta lizard_vx+1
			;
		@air_stop_end:
	@control_end:
	; apply flow
	lda lizard_flow
	beq @flow_end
		cmp #1
		beq @flow_left
		cmp #2
		beq @flow_right
		jmp @flow_end
		@flow_left:
			lda #<(-FLOW_MAX)
			cmp lizard_vx+1
			lda #>(-FLOW_MAX)
			sbc lizard_vx+0
			bvc :+
			eor #$80
			:
			bpl :+
				; if lizard.vx > -FLOW_MAX
				; lizard.vx -= FLOW
				lda lizard_vx+1
				sec
				sbc #<FLOW
				sta lizard_vx+1
				lda lizard_vx+0
				sbc #>FLOW
				sta lizard_vx+0
			:
			jmp @flow_end
		;
		@flow_right:
			lda lizard_vx+1
			cmp #<FLOW_MAX
			lda lizard_vx+0
			sbc #>FLOW_MAX
			bvc :+
			eor #$80
			:
			bpl :+
				; if lizard.vx < FLOW_MAX
				; lizard.vx += FLOW
				lda lizard_vx+1
				clc
				adc #<FLOW
				sta lizard_vx+1
				lda lizard_vx+0
				adc #>FLOW
				sta lizard_vx+0
			:
		;
	@flow_end:
	; correct facing after landing
	lda lizard_skid
	ora lizard_fall
	bne @face_correct_end
		; if lizard.face && lizard.vx > 0
		lda lizard_face
		beq :+
		lda lizard_vx+0
		bmi :+
		ora lizard_vx+1
		beq :+
			lda #0
			sta lizard_face
			jmp @face_correct_end
		:
		; if !lizard.face && lizard.vx < 0
		lda lizard_face
		bne :+
		lda lizard_vx+0
		bpl :+
			lda #1
			sta lizard_face
		:
	@face_correct_end:
	; touch default
	lda #127
	sta lizard_touch_xh0
	sta lizard_touch_xh1
	lda #255
	sta lizard_touch_x0
	sta lizard_touch_x1
	sta lizard_touch_y0
	sta lizard_touch_y1
	; powers
	jsr lizard_tick_jump
	; move
	jsr lizard_move
	; animate
	lda lizard_vx+0
	bmi @animate_left
	ora lizard_vx+1
	bne @animate_right
	lda lizard_moving
	beq @animate_none
	@animate_moving:
		lda #<MAXW_RIGHT
		clc
		adc lizard_anim+1
		sta lizard_anim+1
		lda #>MAXW_RIGHT
		adc lizard_anim+0
		sta lizard_anim+0
		jmp @animate_end
	@animate_right: ; lizard_vx > 0
		lda lizard_anim+1
		clc
		adc lizard_vx+1
		sta lizard_anim+1
		lda lizard_anim+0
		adc lizard_vx+0
		sta lizard_anim+0
		jmp @animate_end
	@animate_left: ; lizard_vx < 0
		lda lizard_anim+1
		sec
		sbc lizard_vx+1
		sta lizard_anim+1
		lda lizard_anim+0
		sbc lizard_vx+0
		sta lizard_anim+0
		jmp @animate_end
	@animate_none: ; lizard_vx == 0
		inc lizard_anim+1
		bne :+
			inc lizard_anim+0
		:
	@animate_end:
	; update hitbox
	lda lizard_x+0
	clc
	adc #HITBOX_L
	sta lizard_hitbox_x0
	lda lizard_xh
	adc #SIGN8 HITBOX_L
	sta lizard_hitbox_xh0
	lda lizard_x+0
	clc
	adc #HITBOX_R
	sta lizard_hitbox_x1
	lda lizard_xh
	adc #SIGN8 HITBOX_R
	sta lizard_hitbox_xh1
	lda lizard_y+0
	clc
	adc #HITBOX_T
	sta lizard_hitbox_y0
	lda lizard_y+0
	clc
	adc #HITBOX_B
	sta lizard_hitbox_y1
	; update touch if required
	lda lizard_power
	jeq @touch_end
	lda current_lizard
	cmp #LIZARD_OF_HEAT
	bne @not_fire
		lda lizard_face
		beq @fire_right ; if lizard_face
		@fire_left:
			lda lizard_x+0
			sec
			sbc #15
			sta lizard_touch_x0
			lda lizard_xh
			sbc #0
			sta lizard_touch_xh0
			bpl :+
				lda #0
				sta lizard_touch_xh0
				sta lizard_touch_x0
			:
			lda lizard_y
			sec
			sbc #14
			sta lizard_touch_y0
			lda lizard_x+0
			sec
			sbc #0
			sta lizard_touch_x1
			lda lizard_xh
			sbc #0
			sta lizard_touch_xh1
			bpl :+
				lda #0
				sta lizard_touch_xh1
				sta lizard_touch_x1
			:
			lda lizard_y
			sec
			sbc #1
			sta lizard_touch_y1
			jmp @touch_end
		@fire_right: ; else !lizard_face
			lda lizard_x+0
			clc
			adc #0
			sta lizard_touch_x0
			lda lizard_xh
			adc #0
			sta lizard_touch_xh0
			lda lizard_y
			sec
			sbc #14
			sta lizard_touch_y0
			lda lizard_x+0
			clc
			adc #14
			sta lizard_touch_x1
			lda lizard_xh
			adc #0
			sta lizard_touch_xh1
			lda lizard_y
			sec
			sbc #1
			sta lizard_touch_y1
			jmp @touch_end
		;
	@not_fire:
	cmp #LIZARD_OF_DEATH
	bne @touch_end
		; update touch
		lda lizard_face
		beq @death_right ; if lizard_Face
		@death_left:
			lda lizard_x+0
			sec
			sbc #14
			sta lizard_touch_x0
			lda lizard_xh
			sbc #0
			sta lizard_touch_xh0
			bpl :+
				lda #0
				sta lizard_touch_xh0
				sta lizard_touch_x0
			:
			lda lizard_y
			sec
			sbc #11
			sta lizard_touch_y0
			lda lizard_x+0
			sec
			sbc #12
			sta lizard_touch_x1
			lda lizard_xh
			sbc #0
			sta lizard_touch_xh1
			bpl :+
				lda #0
				sta lizard_touch_xh1
				sta lizard_touch_x1
			:
			lda lizard_y
			sec
			sbc #5
			sta lizard_touch_y1
			jmp @touch_end
		@death_right: ; else
			lda lizard_x+0
			clc
			adc #11
			sta lizard_touch_x0
			lda lizard_xh
			adc #0
			sta lizard_touch_xh0
			lda lizard_y
			sec
			sbc #11
			sta lizard_touch_y0
			lda lizard_x+0
			clc
			adc #13
			sta lizard_touch_x1
			lda lizard_xh
			adc #0
			sta lizard_touch_xh1
			lda lizard_y
			sec
			sbc #5
			sta lizard_touch_y1
			;jmp @touch_end
		;
	@touch_end:
	; timeout for smoother pushing or pressing-against moving character
	lda lizard_skid
	beq :+
		lda #0
		sta lizard_moving
		rts
	:
	lda lizard_vx+1
	ora lizard_vx+0
	beq :+
		lda #4
		sta lizard_moving
		rts
	:
	ldx lizard_moving
	beq :+
		dex
		stx lizard_moving
	:
	rts

; ==================
; lizard tick powers
; ==================
stat_lizard_tick_powers:

lizard_tick_jump:
	ldx current_lizard
	lda lizard_tick_table_high, X
	pha
	lda lizard_tick_table_low, X
	pha
	rts

.segment "DATA"
lizard_tick_table_low:
.byte <(lizard_tick_knowledge-1)
.byte <(lizard_tick_bounce   -1)
.byte <(lizard_tick_swim     -1)
.byte <(lizard_tick_heat     -1)
.byte <(lizard_tick_surf     -1)
.byte <(lizard_tick_push     -1)
.byte <(lizard_tick_stone    -1)
.byte <(lizard_tick_coffee   -1)
.byte <(lizard_tick_lounge   -1)
.byte <(lizard_tick_death    -1)
.byte <(lizard_tick_beyond   -1)

lizard_tick_table_high:
.byte >(lizard_tick_knowledge-1)
.byte >(lizard_tick_bounce   -1)
.byte >(lizard_tick_swim     -1)
.byte >(lizard_tick_heat     -1)
.byte >(lizard_tick_surf     -1)
.byte >(lizard_tick_push     -1)
.byte >(lizard_tick_stone    -1)
.byte >(lizard_tick_coffee   -1)
.byte >(lizard_tick_lounge   -1)
.byte >(lizard_tick_death    -1)
.byte >(lizard_tick_beyond   -1)

.segment "CODE"

lizard_tick_knowledge:
lizard_tick_bounce:
lizard_tick_surf: ; out of order
lizard_tick_stone: ; out of order
lizard_tick_beyond: ; out of order
	lda gamepad
	and #PAD_B
	beq :+
		lda #1
		sta lizard_power
		rts
	:
		;lda #0
		sta lizard_power
		rts
	;

lizard_tick_swim:
	lda gamepad
	and #PAD_B
	beq @no_b
		lda lizard_power
		bne @no_swim
			PLAY_SOUND SOUND_SWIM
			lda lizard_wet
			beq @no_swim
			lda lizard_vy+0
			bmi :+
			ora lizard_vy+1
			beq :+
				; lizard_vy > 0
				lda #0
				sta lizard_vy+0
				sta lizard_vy+1
			:
			lda lizard_vy+1
			clc
			adc #<SWIM_UP
			sta lizard_vy+1
			lda lizard_vy+0
			adc #>SWIM_UP
			sta lizard_vy+0
			lda #1
			sta lizard_fall
			lda #0
			sta lizard_skid
			lda gamepad
			and #PAD_L
			beq :+
				lda #1
				sta lizard_face
				lda lizard_vx+1
				clc
				adc #<SWIM_LEFT
				sta lizard_vx+1
				lda lizard_vx+0
				adc #>SWIM_LEFT
				sta lizard_vx+0
			:
			lda gamepad
			and #PAD_R
			beq :+
				lda #0
				sta lizard_face
				lda lizard_vx+1
				clc
				adc #<SWIM_RIGHT
				sta lizard_vx+1
				lda lizard_vx+0
				adc #>SWIM_RIGHT
				sta lizard_vx+0
			:
		@no_swim:
		; increment lizard_power if <255
		lda lizard_power
		cmp #255
		bcs :+
			inc lizard_power
		:
		rts
	@no_b:
		lda lizard_power
		bne :+
			rts
		:
		inc lizard_power
		lda lizard_power
		cmp #(SWIM_TIMEOUT+1)
		bcc :+
			; if lizard_power > SWIM_TIMEOUT
			lda #0
			sta lizard_power
		:
		rts
	;

lizard_tick_heat:
	lda lizard_wet
	beq :+
		lda #0
		sta lizard_power
		rts
	:
	lda gamepad
	and #PAD_B
	beq @no_fire
		lda lizard_power
		bne :+
			lda #1
			sta lizard_power
			PLAY_SOUND SOUND_FIRE
			jmp @end_fire
		:
			inc lizard_power
			lda lizard_power
			cmp #17
			bcc :+
				lda #1
				sta lizard_power
				PLAY_SOUND SOUND_FIRE_CONTINUE
			:
		@end_fire:
		rts
	@no_fire:
		;lda #0
		sta lizard_power
		rts
	;

;lizard_tick_surf:

lizard_tick_push:
	lda gamepad
	and #PAD_B
	bne :+
		lda #0
		sta lizard_power
		rts
	:
	lda lizard_power
	bne @no_power
		lda #1
		sta lizard_power
		PLAY_SOUND SOUND_BLOW
		; smoke setup
		lda lizard_face
		beq :+
			lda lizard_x+0
			sec
			sbc #<17
			tax
			lda lizard_xh
			sbc #>17
			jmp :++
		:
			lda lizard_x+0
			clc
			adc #<10
			tax
			lda lizard_xh
			adc #>10
		:
		stx smoke_x
		sta smoke_xh
		lda lizard_y
		sec
		sbc #16
		sta smoke_y
		clc
		adc #7
		cmp water
		bcc :+
			lda #255
			sta smoke_y
		:
		rts
	@no_power:
		inc lizard_power
		lda lizard_power
		cmp #255
		bcc :+
			lda #254
			sta lizard_power
		:
		rts
	;

;lizard_tick_stone:

lizard_tick_coffee:
lizard_tick_lounge:
	lda lizard_wet
	beq :+
		lda #0
		sta lizard_power
		rts
	:
	;
	lda lizard_power
	beq @no_power
	cmp #255
	bcs @no_power
		inc lizard_power
		lda lizard_power
		cmp #45
		bne :+
			PLAY_SOUND SOUND_SIP
			jmp @no_power
		:
		cmp #60
		bne @not_60
			lda current_lizard
			cmp #LIZARD_OF_LOUNGE
			bne @no_power
			PLAY_SOUND SOUND_BLOW
			; smoke setup
			lda lizard_face
			beq :+
				lda lizard_x+0
				sec
				sbc #<17
				tax
				lda lizard_xh
				sbc #>17
				jmp :++
			:
				lda lizard_x+0
				clc
				adc #<10
				tax
				lda lizard_xh
				adc #>10
			:
			stx smoke_x
			sta smoke_xh
			lda lizard_y
			sec
			sbc #16
			sta smoke_y
			jmp @no_power
		@not_60:
		cmp #255
		bcc :+
			lda #1
			sta lizard_power
		:
	;
	@no_power:
	lda gamepad
	and #PAD_B
	beq @no_b
		lda lizard_power
		bne :+
			lda #1
			sta lizard_power
		:
		rts
	@no_b:
		lda lizard_power
		cmp #30
		bcc :+
		cmp #255
		beq :+
			rts
		:
		lda #0
		sta lizard_power
		rts
	;

lizard_tick_death:
	lda gamepad
	and #PAD_B
	beq @no_death
		lda lizard_power
		bne :+
			PLAY_SOUND SOUND_SIP
		:
		lda #1
		sta lizard_power
		rts
	@no_death:
		;lda #0
		sta lizard_power
		rts
	;

; ===========
; lizard_draw
; ===========
stat_lizard_draw:

rainbow_palette:
	; cycles the palette if using the beyond dog
	lda dog_type + BEYOND_STAR_SLOT
	cmp #DOG_BEYOND_STAR
	bne @no_rainbow
	lda dgd_BEYOND_STAR_DIE + BEYOND_STAR_SLOT
	beq @no_rainbow
	ldx #((4*4)+1)
	@loop:
		inc palette, X
		lda palette, X
		and #$F
		cmp #$D
		bcc :+
			lda palette, X
			sec
			sbc #$C
			sta palette, X
		:
		inx
		cpx #(5*4)
		bcc @loop
	;
@rainbow:
	lda #1
	rts
@no_rainbow:
	lda #0
	rts

lizard_draw:
	; scuttle test
	jsr lizard_scuttle_test
	beq :+ ; if scuttle flicker, replace palette
		lda #4
		ldx #DATA_palette_lizard_flash
		jsr palette_load
		jmp :++
	: ; else use the lizard palette
		jsr rainbow_palette ; rainbow palette override for beyond star transition
		bne :+
		lda current_lizard
		clc
		adc #DATA_palette_lizard0
		tax
		lda #4
		jsr palette_load
		lda human0_pal
		clc
		adc #DATA_palette_human0
		tax
		lda #5
		jsr palette_load
	:
	lda lizard_dismount
	cmp #1
	bne :+
		; lizard_dismounter draws the lizard during dismount
		rts
	:
	lda lizard_dead
	beq @lizard_not_dead
	;lizard_draw_dead:
		ldy lizard_y
		cpy #255 
		bcc :+
			rts
		:
		lda dog_type+RIVER_SLOT
		cmp #DOG_RIVER
		beq @river_or_frob
		lda dog_type+0
		cmp #DOG_FROB
		bne @river_or_frob_not
		@river_or_frob:
			lda lizard_xh
			beq :+
				rts ; offscreen
			:
			ldx lizard_x+0
			jmp @dead_sprite
		@river_or_frob_not:
			; adjust X for scroll
			lda lizard_x+0
			sec
			sbc scroll_x+0
			tax
			lda lizard_xh
			sbc scroll_x+1
			beq :+
				rts ; wrapped (offscreen)
			:
		@dead_sprite:
		txa
		jsr sprite_prepare
		lda lizard_face
		beq :+
			lda #DATA_sprite2_lizard_skull
			jmp sprite2_add_flipped
			;rts
		: ; else
			lda #DATA_sprite2_lizard_skull
			jmp sprite2_add
			; rts
		;
	@lizard_not_dead:
	; river mode
	lda dog_type+0
	cmp #DOG_RIVER
	jeq lizard_draw_river
	; setup sprite/scroll
	lda lizard_x+0
	sec
	sbc scroll_x+0
	sta u ; u = dx
	jsr sprite_prepare
	; basic lizard
	lda lizard_fall
	beq :++
		lda lizard_vy+0
		bmi :+ ; vy < 0
		ora lizard_vy+1
		beq :+ ; vy == 0
			; vy > 0
			lda #DATA_sprite2_lizard_fall
			sta l
			jmp @lizard_draw_special
		:
			; vy < 0
			lda #DATA_sprite2_lizard_fly
			sta l
			jmp @lizard_draw_special
		;
	:
	lda lizard_skid
	beq :+
		lda #DATA_sprite2_lizard_skid
		sta l
		jmp @lizard_draw_special
	:
	lda lizard_moving
	beq @no_walk
	@walk:
		lda lizard_anim+0
		lsr
		lsr
		lsr
		and #3
		clc
		adc #DATA_sprite2_lizard_walk0
		sta l
		jmp @lizard_draw_special
	@no_walk:
	lda lizard_anim+1
	cmp #248
	bcc :+
		lda #DATA_sprite2_lizard_blink
		sta l
		jmp @lizard_draw_special
	:
		lda #DATA_sprite2_lizard_stand
		sta l
	;
@lizard_draw_special:
	lda lizard_power
	beq lizard_draw_end ; if lizard_power=0 skip the power draw
	jsr lizard_draw_jump
	lda current_lizard
	cmp #LIZARD_OF_SURF
	bne lizard_draw_end
	rts
lizard_draw_end:
	; override sprites for big head mode
	lda lizard_big_head_mode
	bpl @bhm_end
		lda l
		clc
		adc #BHM_OFFSET
		sta l
	@bhm_end:
	; chosen lizard sprite in l
	ldx u
	ldy lizard_y
	lda lizard_face
	beq :+
		lda l
		jsr sprite2_add_flipped
		rts
	:
		lda l
		jsr sprite2_add
		rts
	; end of lizard_draw

lizard_draw_jump:
	ldx current_lizard
	lda lizard_draw_table_high, X
	pha
	lda lizard_draw_table_low, X
	pha
	rts

.segment "DATA"
lizard_draw_table_low:
.byte <(lizard_draw_knowledge-1)
.byte <(lizard_draw_bounce   -1)
.byte <(lizard_draw_swim     -1)
.byte <(lizard_draw_heat     -1)
.byte <(lizard_draw_surf     -1)
.byte <(lizard_draw_push     -1)
.byte <(lizard_draw_stone    -1)
.byte <(lizard_draw_coffee   -1)
.byte <(lizard_draw_lounge   -1)
.byte <(lizard_draw_death    -1)
.byte <(lizard_draw_beyond   -1)

lizard_draw_table_high:
.byte >(lizard_draw_knowledge-1)
.byte >(lizard_draw_bounce   -1)
.byte >(lizard_draw_swim     -1)
.byte >(lizard_draw_heat     -1)
.byte >(lizard_draw_surf     -1)
.byte >(lizard_draw_push     -1)
.byte >(lizard_draw_stone    -1)
.byte >(lizard_draw_coffee   -1)
.byte >(lizard_draw_lounge   -1)
.byte >(lizard_draw_death    -1)
.byte >(lizard_draw_beyond   -1)

; =====================
; drawing lizard powers
; =====================
.segment "CODE"
stat_lizard_draw_powers:

; for each of these subroutines:
; l = lizard sprite (read/write)
; u = dx
; lizard_power != 0 (verified before entry)

lizard_draw_knowledge:
	lda u
	sec
	sbc #4
	tax            ; x
	lda lizard_y
	sec
	sbc #26
	tay            ; y
lizard_draw_knowledge_common:
	lda #%00000001
	sta i          ; attribute
	; big head mode offsets tile
	lda lizard_big_head_mode
	bpl :+
		tya
		sec
		sbc #6
		tay
	:
	lda #$25       ; tile
	jmp sprite_tile_add
	;rts

.segment "DATA"
; bounce boots       st  bl  w0  w1  w2  w3  sk  fy  fl
bounce_pos_x0: .byte -2, -2, -3, -1, -1, -2,  1, -2, -1
bounce_pos_y0: .byte -4, -4, -4, -4, -4, -4, -4, -4, -5
bounce_pos_x1: .byte  2,  2,  2,  2,  2,  2,  4,  2,  2
bounce_pos_y1: .byte -4, -4, -4, -4, -4, -5, -4, -4, -4

; flipped x
bounce_pos_f0: .byte  2-8,  2-8,  3-8,  1-8,  1-8,  2-8, -1-8,  2-8,  1-8
bounce_pos_f1: .byte -2-8, -2-8, -2-8, -2-8, -2-8, -2-8, -4-8, -2-8, -2-8

.segment "CODE"

lizard_draw_bounce:
	ldx l ; l in range: DATA_sprite2_lizard_stand ... fall (contiguous)
	cpx #DATA_sprite2_lizard_fly
	bne :+
		lda #$37 ; if flying upward: springs out
		jmp :++
	:
		lda #$36 ; else: springs in
	:
	pha
	stx j
	lda lizard_face
	bne :+
		lda #%00000001
		sta i ; attribute
		lda bounce_pos_y0-DATA_sprite2_lizard_stand, X
		clc
		adc lizard_y
		tay
		lda bounce_pos_x0-DATA_sprite2_lizard_stand, X
		clc
		adc u
		tax
		pla
		pha
		jsr sprite_tile_add
		ldx j
		lda bounce_pos_y1-DATA_sprite2_lizard_stand, X
		clc
		adc lizard_y
		tay
		lda bounce_pos_x1-DATA_sprite2_lizard_stand, X
		clc
		adc u
		tax
		pla
		jmp sprite_tile_add
		;rts
	:
		lda #%01000001
		sta i ; attribute
		lda bounce_pos_y0-DATA_sprite2_lizard_stand, X
		clc
		adc lizard_y
		tay
		lda bounce_pos_f0-DATA_sprite2_lizard_stand, X
		clc
		adc u
		tax
		pla
		pha
		jsr sprite_tile_add
		ldx j
		lda bounce_pos_y1-DATA_sprite2_lizard_stand, X
		clc
		adc lizard_y
		tay
		lda bounce_pos_f1-DATA_sprite2_lizard_stand, X
		clc
		adc u
		tax
		pla
		jmp sprite_tile_add
		;rts
	;

lizard_draw_swim:
	lda lizard_power
	bne :+
		rts
	:
	cmp #3
	bcs :+
		lda #DATA_sprite2_lizard_fall
		sta l
		rts
	:
	cmp #9
	bcs :+
		lda #DATA_sprite2_lizard_fly
		sta l
	:
	rts
	;

lizard_draw_heat:
	lda lizard_power
	lsr
	lsr
	and #1
	clc
	adc #DATA_sprite2_lizard_power_fire0
	pha
	ldx u
	ldy lizard_y
	lda lizard_face
	beq :+
		pla
		jmp sprite2_add
		;rts
	:
		pla
		jmp sprite2_add_flipped
		; rts
	;

lizard_draw_surf:
	jsr lizard_draw_end ; surfboard must go behind the lizard
	lda lizard_wet
	beq :+
		rts
	:
	lda lizard_y
	sec
	sbc #10
	tay
	sty k ; k = y
	lda u
	tax ; x = dx
	pha ; stack = dx
	lda l
	; big head mode
	bit lizard_big_head_mode
	bpl :+
		sec
		sbc #BHM_OFFSET
		inc k ; y += 1
	:
	; A = sprite
	cmp #DATA_sprite2_lizard_fly
	bne :+
		lda #1
		jmp @surf_face_end
	:
		cmp #DATA_sprite2_lizard_fall
		bne :+
			inc k ; y += 1
		:
		lda #0
	@surf_face_end:
	eor lizard_face
	beq :+
		lda #%01000000
		sta i
		lda #$26
		jsr sprite_tile_add
		pla ; dx
		sec
		sbc #8
		tax
		ldy k
		lda #%10000000
		sta i
		lda #$26
		jmp sprite_tile_add
		;rts
	:
		lda #%11000000
		sta i
		lda #$26
		jsr sprite_tile_add
		pla ; dx
		sec
		sbc #8
		tax
		ldy k
		lda #%00000000
		sta i
		lda #$26
		jmp sprite_tile_add
		;rts
	;

lizard_draw_push:
	lda lizard_power
	cmp #20
	bcc :+
		rts
	:
	lda smoke_x
	sec
	sbc scroll_x+0
	tax
	lda smoke_xh
	sbc scroll_x+1
	beq :+
		rts ; offscreen
	:
	ldy smoke_y
	lda lizard_power
	cmp #5
	bcs :+
		lda #%00000001
		sta i
		lda #$22
		jmp sprite_tile_add
	:
	cmp #10
	bcs :+
		lda #%00000001
		sta i
		lda #$23
		jmp sprite_tile_add
	:
	cmp #15
	bcs :+
		lda #%01000001
		sta i
		lda #$23
		jmp sprite_tile_add
	:
		lda #%01000001
		sta i
		lda #$22
		jmp sprite_tile_add
	;

lizard_draw_stone:
	lda #5
	ldx #DATA_palette_human_stone
	jsr palette_load
	; stone lizard does not move,always stands
	lda #DATA_sprite2_lizard_stand
	sta l
	rts

lizard_draw_coffee:
	lda lizard_power
	cmp #31
	bcc :+
		cmp #45
		bcc @coffee_sip
	:
	ldx u
	ldy lizard_y
	lda lizard_face
	bne :+
		lda #DATA_sprite2_lizard_power_coffee1
		jmp sprite2_add_flipped
		;rts
	:
		lda #DATA_sprite2_lizard_power_coffee1
		jmp sprite2_add
		;rts
	;
@coffee_sip:
	lda #DATA_sprite2_lizard_smoke
	sta l
	ldx u
	ldy lizard_y
	lda lizard_face
	bne :+
		lda #DATA_sprite2_lizard_power_coffee0
		jmp sprite2_add_flipped
		;rts
	:
		lda #DATA_sprite2_lizard_power_coffee0
		jmp sprite2_add
		;rts
	;

lizard_draw_lounge:
	; cigarette
	lda lizard_y
	sec
	sbc #9
	tay
	lda lizard_power
	cmp #31
	bcc :+
		cmp #45
		bcs :+
		dey
		lda #DATA_sprite2_lizard_smoke
		sta l
	:
	ldx u
	lda lizard_face
	bne :+
		lda #DATA_sprite2_lizard_power_cigarette
		jsr sprite2_add_flipped
		jmp @puff
	:
		lda #DATA_sprite2_lizard_power_cigarette
		jsr sprite2_add
		;jmp @puff
	;
@puff:
	lda lizard_power
	cmp #65
	bcs :+
		rts
	:
	cmp #85
	bcc :+
		rts
	:
	lda smoke_x
	sec
	sbc scroll_x+0
	tax
	lda smoke_xh
	sbc scroll_x+1
	beq :+
		rts ; offscreen
	:
	ldy smoke_y
	; select puff frame
	lda lizard_power
	cmp #70
	bcs :+
		lda #%00000001
		sta i
		lda #$22
		jmp sprite_tile_add
		;rts
	:
	cmp #75
	bcs :+
		lda #%00000001
		sta i
		lda #$23
		jmp sprite_tile_add
		;rts
	:
	cmp #80
	bcs :+
		lda #%01000001
		sta i
		lda #$23
		jmp sprite_tile_add
		;rts
	:
		lda #%01000001
		sta i
		lda #$22
		jmp sprite_tile_add
		;rts
	;

lizard_draw_death:
	lda lizard_y
	sec
	sbc #13
	tay
	lda l
	cmp #DATA_sprite2_lizard_walk1
	bcc :+
		cmp #(DATA_sprite2_lizard_walk3+1)
		bcs :+
		iny
	:
	ldx u
	lda lizard_face
	beq :+
		lda #DATA_sprite2_lizard_power_death
		jmp sprite2_add
		;rts
	:
		lda #DATA_sprite2_lizard_power_death
		jmp sprite2_add_flipped
		;rts
	;

lizard_draw_beyond:
	rts

; returns zero flag true if should flicker scuttle palette
lizard_scuttle_test:
	lda lizard_scuttle
	beq @no_scuttle
	cmp #64
	bcs :+
		and #31
		cmp #30
		bcc @no_scuttle
		rts
	:
	cmp #128
	bcs :+
		and #15
		cmp #14
		bcc @no_scuttle
		rts
	:
	cmp #196
	bcs :+
		and #7
		cmp #6
		bcc @no_scuttle
		rts
	: ; >= 196
		and #3
		cmp #2
		bcc @no_scuttle
		rts
	;
@no_scuttle:
	lda #0
	rts

; =====
; river
; =====
stat_lizard_river:

lizard_tick_river:
	; death
	lda lizard_dead
	jne lizard_dead_tick
	; hold select to scuttle lizard
	lda gamepad
	sta lizard_skid ; store last gamepad for draw
	and #PAD_SELECT
	beq :++
		inc lizard_scuttle
		lda lizard_scuttle
		cmp #255
		bne :+
			jmp lizard_die
		:
		jmp :++
	: ; else
		;lda #0
		sta lizard_scuttle
	:
	; press start to pause
	lda gamepad
	and #PAD_START
	beq :++
		lda mode_temp
		beq :+
			lda #2
			sta text_select ; pause
			lda #1
			sta game_pause
			rts
		:
		jmp :++
	:
		lda #1
		sta mode_temp
	:
	; simplified power (only KNOWLEDGE uses it as an easter egg)
	lda gamepad
	and #PAD_B
	sta lizard_power
	; jumping
	lda gamepad
	and #PAD_A
	beq @jump_a_off
	@jump_a_on:
		lda lizard_jump
		bne @jump_end
		lda lizard_wet
		bne @jump_end
			lda #<SURF_JUMP
			sta lizard_vz+1
			lda #>SURF_JUMP
			sta lizard_vz+0
			lda #1
			sta lizard_jump
			sta lizard_wet
			jsr lizard_jump_finish
			jmp @jump_end
		;
	@jump_a_off:
		lda #0
		sta lizard_jump
		lda lizard_wet
		cmp #1
		bne @jump_end
		lda lizard_vz+1
		cmp #<SURF_RELEASE
		lda lizard_vz+0
		sbc #>SURF_RELEASE
		bvc :+
		eor #$80
		:
		bmi @jump_end
			lda #<SURF_RELEASE
			sta lizard_vz+1
			lda #>SURF_RELEASE
			sta lizard_vz+0
		;
	@jump_end:
	lda lizard_wet
	bne @motion_control_end ; skip controls if in the air
	; motion input x
	lda gamepad
	and #(PAD_L | PAD_R)
	cmp #PAD_L
	beq :+
	cmp #PAD_R
	beq :++
	jmp @horizontal_end
	:
		lda #>RIVER_LEFT
		sta lizard_vx+0
		lda #<RIVER_LEFT
		sta lizard_vx+1
		jmp @horizontal_end
	:
		lda #>RIVER_RIGHT
		sta lizard_vx+0
		lda #<RIVER_RIGHT
		sta lizard_vx+1
	@horizontal_end:
	; motion input y
	lda gamepad
	and #(PAD_U | PAD_D)
	cmp #PAD_U
	beq :+
	cmp #PAD_D
	beq :++
	jmp @vertical_end
	:
		lda #>RIVER_UP
		sta lizard_vy+0
		lda #<RIVER_UP
		sta lizard_vy+1
		jmp @vertical_end
	:
		lda #>RIVER_DOWN
		sta lizard_vy+0
		lda #<RIVER_DOWN
		sta lizard_vy+1
	@vertical_end:
	@motion_control_end:
	; motion move x
	lda lizard_x+1
	clc
	adc lizard_vx+1
	sta lizard_x+1
	lda lizard_x+0
	adc lizard_vx+0
	sta lizard_x+0
	; motion move y
	lda lizard_y+1
	clc
	adc lizard_vy+1
	sta lizard_y+1
	lda lizard_y+0
	adc lizard_vy+0
	sta lizard_y+0
	; motion clamp x
	lda #0
	sta lizard_xh
	lda lizard_x
	cmp #15
	bcs :+
		lda #15
		sta lizard_x
		lda #0
		sta lizard_x+1
	:
	lda #239
	cmp lizard_x
	bcs :+
		lda #239
		sta lizard_x
		lda #0
		sta lizard_x+1
	:
	; motion clamp y
	lda lizard_y
	cmp #104
	bcs :+
		lda #104
		sta lizard_y
		lda #0
		sta lizard_y+1
	:
	lda #188
	cmp lizard_y
	bcs :+
		lda #188
		sta lizard_y
		lda #0
		sta lizard_y+1
	:
	lda lizard_wet
	bne @flying ; skip drag when flying
	; drag x
	lda lizard_vx+0
	bmi @drag_x_minus
	eor lizard_vx+1
	beq @drag_x_end
	@drag_x_plus:
		lda lizard_vx+1
		sec
		sbc #<RIVER_DRAG_X
		sta lizard_vx+1
		lda lizard_vx+0
		sbc #>RIVER_DRAG_X
		sta lizard_vx+0
		bpl :+
			lda #0
			sta lizard_vx+0
			sta lizard_vx+1
		:
		jmp @drag_x_end
	@drag_x_minus:
		lda lizard_vx+1
		clc
		adc #<RIVER_DRAG_X
		sta lizard_vx+1
		lda lizard_vx+0
		adc #>RIVER_DRAG_X
		sta lizard_vx+0
		bmi :+
			lda #0
			sta lizard_vx+0
			sta lizard_vx+1
		:
	@drag_x_end:
	; drag y
	lda lizard_vy+0
	bmi @drag_y_minus
	eor lizard_vy+1
	beq @drag_y_end
	@drag_y_plus:
		lda lizard_vy+1
		sec
		sbc #<RIVER_DRAG_Y
		sta lizard_vy+1
		lda lizard_vy+0
		sbc #>RIVER_DRAG_Y
		sta lizard_vy+0
		bpl :+
			lda #0
			sta lizard_vy+0
			sta lizard_vy+1
		:
		jmp @drag_y_end
	@drag_y_minus:
		lda lizard_vy+1
		clc
		adc #<RIVER_DRAG_Y
		sta lizard_vy+1
		lda lizard_vy+0
		adc #>RIVER_DRAG_Y
		sta lizard_vy+0
		bmi :+
			lda #0
			sta lizard_vy+0
			sta lizard_vy+1
		:
	@drag_y_end:
	jmp @flying_end
	; flying
	@flying:
		lda lizard_z+1
		clc
		adc lizard_vz+1
		sta lizard_z+1
		lda lizard_z+0
		adc lizard_vz+0
		sta lizard_z+0
		bpl @gravity
			lda #0
			sta lizard_z+0
			sta lizard_z+1
			sta lizard_vz+0
			sta lizard_vz+1
			sta lizard_wet
			PLAY_SOUND SOUND_WATER
			jmp @flying_end
		@gravity:
			lda lizard_vz+1
			clc
			adc #<SURF_GRAVITY
			sta lizard_vz+1
			lda lizard_vz+0
			adc #>SURF_GRAVITY
			sta lizard_vz+0
	@flying_end:
	; blink animation
	inc lizard_anim+1
	; update hitbox
	lda #0
	sta lizard_hitbox_xh0
	sta lizard_hitbox_xh1
	lda lizard_x+0
	clc
	adc #RIVER_HITBOX_L
	sta lizard_hitbox_x0
	lda lizard_x+0
	clc
	adc #RIVER_HITBOX_R
	sta lizard_hitbox_x1
	lda lizard_y+0
	clc
	adc #RIVER_HITBOX_T
	sta lizard_hitbox_y0
	lda lizard_y+0
	clc
	adc #RIVER_HITBOX_B
	sta lizard_hitbox_y1
	rts

; X = sprite
lizard_draw_river_sprite:
	lda lizard_big_head_mode
	bpl @bhm_end
		txa
		clc
		adc #BHM_OFFSET
		tax
	@bhm_end:
	txa
	ldx lizard_x
	pha
	lda lizard_y
	sec
	sbc lizard_z
	tay
	pla
	jsr sprite2_add
	; KNOWLEDGE easter egg
	lda current_lizard
	cmp #LIZARD_OF_KNOWLEDGE
	bne :+
	lda lizard_power
	beq :+
		lda lizard_x
		sec
		sbc #4
		tax
		lda lizard_y
		sec
		sbc lizard_z
		sec
		sbc #26
		tay
		jsr lizard_draw_knowledge_common
	:
	rts

lizard_draw_river:
	lda lizard_x
	jsr sprite_prepare
	lda lizard_skid ; lizard_skid holds last gamepad position
	and #PAD_U
	bne @draw_up
	lda lizard_skid
	and #PAD_D
	bne @draw_down
	lda lizard_skid
	and #PAD_L
	bne @draw_up
	;lda lizard_skid
	;and #PAD_R
	;bne @draw_down
	jmp @draw_flat
@draw_up:
	ldx #DATA_sprite2_lizard_surf_fly
	jmp lizard_draw_river_sprite
@draw_down:
	ldx #DATA_sprite2_lizard_surf_fall
	jmp lizard_draw_river_sprite
@draw_flat:
	; select stand or blink and draw lizard
	lda lizard_anim+1
	cmp #248
	ldx #DATA_sprite2_lizard_surf_stand
	bcc :+
		ldx #DATA_sprite2_lizard_surf_blink
	:
	jmp lizard_draw_river_sprite

stat_lizard_end_of_file:
; end of file
