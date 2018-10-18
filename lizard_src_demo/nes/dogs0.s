; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; dogs (bank E)
;
; dog update and drawing
;

.feature force_range
.macpack longbranch

.include "ram.s"
.export dogs_init
.export dogs_tick
.export dogs_draw

.include "enums.s"
.import bank_call
.import sprite_add
.import sprite_add_flipped
.import sprite_tile_add
.import sprite_tile_add_clip
.import palette_load
.import password_read
.import password_build
.import checkpoint
.import do_scroll

sprite0_add         = sprite_add
sprite0_add_flipped = sprite_add_flipped

.include "dogs_inc.s"
.import dogs_cycle
.import decimal_clear

.include "../assets/export/names.s"
.include "../assets/export/text_set.s"

; ====
; Misc
; ====

.segment "DATA"

.export circle4
circle4:
.byte    0,   0,   0,   0,   0,   0,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1
.byte    1,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   3,   3
.byte    3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3
.byte    4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4
.byte    4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4
.byte    4,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3
.byte    3,   3,   3,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2
.byte    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   0,   0,   0,   0,   0
.byte    0,   0,   0,   0,   0,   0,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1
.byte   -1,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -3,  -3
.byte   -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3
.byte   -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4
.byte   -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4,  -4
.byte   -4,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3,  -3
.byte   -3,  -3,  -3,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2,  -2
.byte   -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,   0,   0,   0,   0,   0

circle32:
.byte    0,   1,   2,   2,   3,   4,   5,   5,   6,   7,   8,   8,   9,  10,  11,  11
.byte   12,  13,  13,  14,  15,  16,  16,  17,  18,  18,  19,  19,  20,  21,  21,  22
.byte   22,  23,  23,  24,  24,  25,  25,  26,  26,  27,  27,  27,  28,  28,  29,  29
.byte   29,  29,  30,  30,  30,  30,  31,  31,  31,  31,  31,  31,  31,  31,  32,  32
.byte   32,  32,  32,  31,  31,  31,  31,  31,  31,  31,  31,  30,  30,  30,  30,  29
.byte   29,  29,  29,  28,  28,  27,  27,  27,  26,  26,  25,  25,  24,  24,  23,  23
.byte   22,  22,  21,  21,  20,  19,  19,  18,  18,  17,  16,  16,  15,  14,  13,  13
.byte   12,  11,  11,  10,   9,   8,   8,   7,   6,   5,   5,   4,   3,   2,   2,   1
.byte    0,  -1,  -2,  -2,  -3,  -4,  -5,  -5,  -6,  -7,  -8,  -8,  -9, -10, -11, -11
.byte  -12, -13, -13, -14, -15, -16, -16, -17, -18, -18, -19, -19, -20, -21, -21, -22
.byte  -22, -23, -23, -24, -24, -25, -25, -26, -26, -27, -27, -27, -28, -28, -29, -29
.byte  -29, -29, -30, -30, -30, -30, -31, -31, -31, -31, -31, -31, -31, -31, -32, -32
.byte  -32, -32, -32, -31, -31, -31, -31, -31, -31, -31, -31, -30, -30, -30, -30, -29
.byte  -29, -29, -29, -28, -28, -27, -27, -27, -26, -26, -25, -25, -24, -24, -23, -23
.byte  -22, -22, -21, -21, -20, -19, -19, -18, -18, -17, -16, -16, -15, -14, -13, -13
.byte  -12, -11, -11, -10,  -9,  -8,  -8,  -7,  -6,  -5,  -5,  -4,  -3,  -2,  -2,  -1

.segment "CODE"

; clobbers A
lizard_x_inc:
	lda lizard_x+0
	clc
	adc #<1
	sta lizard_x+0
	lda lizard_xh
	adc #>1
	sta lizard_xh
	jmp do_scroll

; clobbers A
lizard_x_dec:
	lda lizard_x+0
	sec
	sbc #<1
	sta lizard_x+0
	lda lizard_xh
	sbc #>1
	sta lizard_xh
	jmp do_scroll

; ====
; Dogs
; ====

.segment "CODE"

; DOG_NONE
stat_dog_none:
dog_init_none:
dog_tick_none:
dog_draw_none:
	rts

; DOG_DOOR
stat_dog_door:

dgd_DOOR_ON = dog_data0
dog_init_door = dog_init_none
dog_tick_door:
	lda gamepad
	and #PAD_U
	bne :+
		lda #1
		sta dgd_DOOR_ON, Y
	:
	lda dgd_DOOR_ON, Y
	bne :+
		rts
	:
	lda gamepad
	and #PAD_U
	bne :+
		rts
	:
	DOG_BOUND 0,0,15,15
	jsr lizard_overlap
	beq :+
		lda dog_param, Y
		and #7
		sta current_door
		tax
		lda door_link, X
		sta current_room+0
		lda door_linkh, X
		sta current_room+1
		lda #2
		sta room_change
	:
	rts
dog_draw_door = dog_draw_none

; DOG_PASS
stat_dog_pass:

dog_init_pass = dog_init_none
dog_tick_pass:
	DOG_BOUND 0,0,15,15
	jsr lizard_overlap
	beq :+
		lda dog_param, Y
		and #7
		sta current_door
		tax
		lda door_link, X
		sta current_room+0
		lda door_linkh, X
		sta current_room+1
		lda #1
		sta room_change
	:
	rts
dog_draw_pass = dog_draw_none

; DOG_PASS_X
stat_dog_pass_x:

dog_init_pass_x:
	lda #0
	sta dog_y, Y
	rts
dog_tick_pass_x:
	DOG_BOUND 0,0,7,255
	jsr lizard_overlap
	beq :+
		lda dog_param, Y
		and #7
		sta current_door
		tax
		lda door_link, X
		sta current_room+0
		lda door_linkh, X
		sta current_room+1
		lda #1
		sta hold_x
		sta room_change
	:
	rts
dog_draw_pass_x = dog_draw_none

; DOG_PASS_Y
stat_dog_pass_y:

dog_init_pass_y:
	lda #0
	sta dog_x, Y
	sta dog_xh, Y
	rts
dog_tick_pass_y:
	DOG_BOUND 0,0,511,7
	jsr lizard_overlap
	beq :+
		lda dog_param, Y
		and #7
		sta current_door
		tax
		lda door_link, X
		sta current_room+0
		lda door_linkh, X
		sta current_room+1
		lda #1
		sta hold_y
		sta room_change
		; climb assist
		lda lizard_y+0
		bmi :+
		; lizard_py < 128
		lda #9
		sta climb_assist_time
		lda gamepad
		and #(PAD_L | PAD_R | PAD_A)
		sta climb_assist
	:
	rts
dog_draw_pass_y = dog_draw_none

; DOG_PASSWORD_DOOR
stat_dog_password_door:

set_continued:
	ldx #0
	:
		lda coin, X
		sta coin_save, X
		lda flag, X
		sta flag_save, X
		inx
		cpx #16
		bcc :-
	lda piggy_bank
	sta piggy_bank_save
	lda last_lizard
	sta last_lizard_save
	lda #0
	sta tip_counter
	lda #1
	sta coin_saved
	sta continued
	rts

dgd_PASSWORD_DOOR_ON = dog_data0
dog_init_password_door:
	; banked text_load
	lda #TEXT_CONTINUE
	sta t
	ldx #252
	lda #$F
	jsr bank_call
	PPU_LATCH_AT 3,5
	ldx #0
	@loop:
		lda nmi_update, X
		sta $2007
		inx
		cpx #(32-3)
		bcc @loop
	rts

dog_tick_password_door:
	lda gamepad
	and #PAD_U
	bne :+
		lda #1
		sta dgd_PASSWORD_DOOR_ON, Y
	:
	lda dgd_PASSWORD_DOOR_ON, Y
	bne :+
		rts
	:
	lda gamepad
	and #PAD_U
	bne :+
		rts
	:
	DOG_BOUND 0,0,15,15
	jsr lizard_overlap
	bne :+
		rts
	:
	lda #0
	sta current_door
	; store and load password
	ldx #0
	:
		lda password, X
		sta scratch, X
		inx
		cpx #5
		bcc :-
	ldx #0
	@load_password:
		lda dog_type, X
		cmp #DOG_PASSWORD
		bne :+
		ldy dog_param, X
		cpy #5
		bcs :+
			lda dgd_PASSWORD_VALUE, X
			sta password, Y
		:
		inx
		cpx #16
		bcc @load_password
	ldy dog_now
	; test for invalid password
	jsr password_read
	beq @bad_password
	lda i
	cmp #<DATA_room_COUNT
	lda k
	sbc #>DATA_room_COUNT
	bcs @bad_password
	lda j
	cmp #LIZARD_OF_COUNT
	bcs @bad_password
	; test for checkpoint
	jsr checkpoint
	bne @good_password
	; SELECT+A bypasses checkpoint test
	lda gamepad
	and #(PAD_SELECT|PAD_A)
	cmp #(PAD_SELECT|PAD_A)
	bne @bad_password
	lda #1
	sta metric_cheater
	@good_password:
		lda #2
		sta room_change
		jsr set_continued
		lda i
		sta current_room+0
		lda k
		sta current_room+1
		lda j
		pha
		cmp #LIZARD_OF_BEYOND
		bcc :+
		jsr coin_count
		cmp #PIGGY_BURST
		cmp #3
		bcs :+
			pla
			lda #LIZARD_OF_KNOWLEDGE ; can't continue with LIZARD_OF_BEYOND unless you have the coins
			pha
		:
		pla
		sta next_lizard
		lda #0
		sta lizard_power
		; increment continues
		inc metric_continue+0
		bne :+
		inc metric_continue+1
		bne :+
		inc metric_continue+2
		bne :+
		inc metric_continue+3
		:
		; rebuild the password
		; (preserve current lizard so that it isn't redrawn)
		lda current_lizard
		pha
		lda next_lizard
		sta current_lizard
		jsr password_build
		pla
		sta current_lizard
		rts
	@bad_password:
		lda #1
		sta room_change
		PLAY_SOUND SOUND_NO
		; restore password
		ldx #0
		:
			lda scratch, X
			sta password, X
			inx
			cpx #5
			bcc :-
		rts
	;
dog_draw_password_door = dog_draw_none

; DOG_LIZARD_EMPTY_LEFT
stat_dog_lizard_empty_left:

; common empty lizard and dismounter definitions
LIZARD_SWAP_SPEED = 5
dgd_EMPTY_ON      = dog_data0
dgd_EMPTY_SPEED   = dog_data1
dgd_DISMOUNT_ANIM = dog_data0
dgd_DISMOUNT_FLIP = dog_data1
dgd_DISMOUNT_SLID = dog_data2

dog_dismounter_locate:
	lda lizard_xh
	sta dog_xh+DISMOUNT_SLOT
	lda lizard_x+0
	sta dog_x+DISMOUNT_SLOT
	lda lizard_y+0
	sta dog_y+DISMOUNT_SLOT
	rts

dog_dismounter_start:
	lda lizard_dead
	beq :+
		rts
	:
	lda #1
	sta lizard_dismount
	lda #0
	sta lizard_vx+0
	sta lizard_vx+1
	sta lizard_vy+0
	sta lizard_vy+1
	sta lizard_skid
	sta lizard_fall
	sta dgd_DISMOUNT_ANIM + DISMOUNT_SLOT
	sta dgd_DISMOUNT_SLID + DISMOUNT_SLOT
	sta dgd_EMPTY_SPEED, Y
	lda lizard_face
	sta dgd_DISMOUNT_FLIP + DISMOUNT_SLOT
	jmp dog_dismounter_locate

dog_dismounter_flip:
	tya
	pha
	; set palettes
	lda dog_param, Y
	clc
	adc #DATA_palette_lizard0
	tax
	lda #4
	jsr palette_load
	lda current_lizard
	clc
	adc #DATA_palette_lizard0
	tax
	lda #6
	jsr palette_load
	; switch empty lizards
	pla
	tay
	lda dog_param, Y
	cmp last_lizard
	bne :+
		lda #$FF
		sta last_lizard
		jmp :++
	:
		lda current_lizard
		sta last_lizard
	:
	lda dog_param, Y
	pha
	lda current_lizard
	sta dog_param, Y
	pla
	sta current_lizard
	sta next_lizard
	lda #0
	sta lizard_power
	jmp dog_dismounter_locate
	;rts

dog_init_lizard_empty_left:
	; if current lizard, this empty lizard is gone (or replaced by last)
	lda dog_param, Y
	cmp current_lizard
	bne @setup
		lda last_lizard
		cmp #$FF
		bne :+
			jmp empty_dog
		:
		sta dog_param, Y
		lda #DOG_LIZARD_EMPTY_RIGHT
		sta dog_type, Y
		lda dog_x, Y
		sec
		sbc #<32
		sta dog_x, Y
		lda dog_xh, Y
		sbc #>32
		sta dog_xh, Y
	@setup:
dog_init_lizard_empty_common:
	; setup empty lizard
	lda #1
	sta dgd_EMPTY_ON, Y
	lda dog_param, Y
	clc
	adc #DATA_palette_lizard0
	tax
	lda #6
	jsr palette_load
	rts
dog_tick_lizard_empty_left:
	lda lizard_dismount
	beq :+
	jmp @lizard_dismount_on
	:
		DOG_BOUND -12,-5,9,-1
		jsr lizard_touch_death
		beq @no_kill
			lda dog_param, Y
			cmp last_lizard
			bne :+
				lda #$FF
				sta last_lizard
			:
			lda dog_y, Y
			clc
			adc #5
			sta dog_y, Y
			lda #DATA_sprite0_lizard_skull_dismount
			ldx #1 ; flip
			jmp bones_convert
		@no_kill:
		lda lizard_face
		bne @not_ready ; lizard_face == 0
		lda lizard_y+0
		cmp dog_y, Y
		bne @not_ready ; lizard_py == dgy
		lda dog_x, Y
		sec
		sbc #<37
		sta i
		lda dog_xh, Y
		sbc #>37
		sta ih
		lda lizard_x+0
		cmp i
		lda lizard_xh
		sbc ih
		bcc @not_ready ; lizard_px >= dgx-37
		lda dog_x, Y
		sec
		sbc #<25
		sta i
		lda dog_xh, Y
		sbc #>25
		sta ih
		lda lizard_x+0
		cmp i
		lda lizard_xh
		sbc ih
		bcs @not_ready ; lizard_px < dgx-25
			lda dgd_EMPTY_ON, Y
			bne :+
				lda #0
				sta lizard_face
				jsr dog_dismounter_start
			:
			rts
		@not_ready:
			lda #0
			sta dgd_EMPTY_ON, Y
			rts
		;
	@lizard_dismount_on:
		; slide
		lda dgd_DISMOUNT_ANIM+DISMOUNT_SLOT
		cmp #4
		bcc :+
		lda dgd_DISMOUNT_SLID+DISMOUNT_SLOT
		cmp #42
		bcs :+
			jsr lizard_x_inc
			inc dgd_DISMOUNT_SLID+DISMOUNT_SLOT
		:
		; animate
		lda dgd_EMPTY_SPEED, Y
		clc
		adc #1
		sta dgd_EMPTY_SPEED, Y
		cmp #LIZARD_SWAP_SPEED
		bcs :+
			rts
		:
		lda #0
		sta dgd_EMPTY_SPEED, Y
		; align dismount
		lda dgd_DISMOUNT_ANIM+DISMOUNT_SLOT
		cmp #4
		bcs @aligned
			lda dog_x, Y
			sec
			sbc #<37
			sta i
			lda dog_xh, Y
			sbc #>37
			sta ih
			lda i
			cmp lizard_x+0
			lda ih
			sbc lizard_xh
			bcs :+ ; if lizard_px > dgx-37
				jsr lizard_x_dec
				jsr dog_dismounter_locate
				inc dgd_DISMOUNT_ANIM+DISMOUNT_SLOT
				lda dgd_DISMOUNT_ANIM+DISMOUNT_SLOT
				and #3
				sta dgd_DISMOUNT_ANIM+DISMOUNT_SLOT
				rts
			:
				lda #4
				sta dgd_DISMOUNT_ANIM+DISMOUNT_SLOT
				rts
			;
		@aligned:
			inc dgd_DISMOUNT_ANIM+DISMOUNT_SLOT
			lda dgd_DISMOUNT_ANIM+DISMOUNT_SLOT
			cmp #13
			bne @end_dismount
				; 13 is middle frame of dismount
				jsr dog_dismounter_flip
				lda #DOG_LIZARD_EMPTY_RIGHT
				sta dog_type, Y
				lda dog_x, Y
				sec
				sbc #<32
				sta dog_x, Y
				lda dog_xh, Y
				sbc #>32
				sta dog_xh, Y
				lda #1
				sta dgd_DISMOUNT_FLIP+DISMOUNT_SLOT
				rts
			@end_dismount:
			cmp #21
			bcs :+
				rts
			:
				; 21 is end frame of dismount
				lda #0
				sta lizard_dismount
				sta lizard_face
				lda #1
				sta dgd_EMPTY_ON, Y
				rts
			;
		;
	;
dog_draw_lizard_empty_left:
	DX_SCROLL_EDGE
	lda dog_y, Y
	tay
	lda #DATA_sprite0_lizard_empty_left
	jsr sprite0_add
	rts

; DOG_LIZARD_EMPTY_RIGHT
stat_dog_lizard_empty_right:

dog_init_lizard_empty_right:
	; if current lizard, this empty lizard is gone (or replaced by last)
	lda dog_param, Y
	cmp current_lizard
	bne @setup
		lda last_lizard
		cmp #$FF
		bne :+
			jmp empty_dog
		:
		sta dog_param, Y
		lda #DOG_LIZARD_EMPTY_LEFT
		sta dog_type, Y
		lda dog_x, Y
		clc
		adc #<32
		sta dog_x, Y
		lda dog_xh, Y
		adc #>32
		sta dog_xh, Y
	@setup:
	jmp dog_init_lizard_empty_common

dog_tick_lizard_empty_right:
	lda lizard_dismount
	jne @lizard_dismount_on
		DOG_BOUND -10,-5,11,-1
		jsr lizard_touch_death
		beq @no_kill
			lda dog_param, Y
			cmp last_lizard
			bne :+
				lda #$FF
				sta last_lizard
			:
			lda dog_y, Y
			clc
			adc #5
			sta dog_y, Y
			lda #DATA_sprite0_lizard_skull_dismount
			ldx #0 ; flip
			jmp bones_convert
		@no_kill:
		lda lizard_face
		cmp #1
		bne @not_ready ; lizard_face == 1
		lda lizard_y+0
		cmp dog_y, Y
		bne @not_ready ; lizard_py == dgy
		lda dog_x, Y
		clc
		adc #<37
		sta i
		lda dog_xh, Y
		adc #>37
		sta ih
		lda i
		cmp lizard_x+0
		lda ih
		sbc lizard_xh
		bcc @not_ready ; lizard_px <= dgx + 37
		lda dog_x, Y
		clc
		adc #<25
		sta i
		lda dog_xh, Y
		adc #>25
		sta ih
		lda i
		cmp lizard_x+0
		lda ih
		sbc lizard_xh
		bcs @not_ready ; lizard_px > (dgx+25)
			lda dgd_EMPTY_ON, Y
			bne :+
				lda #1
				sta lizard_face
				jsr dog_dismounter_start
			:
			rts
		@not_ready:
			lda #0
			sta dgd_EMPTY_ON, Y
			rts
		;
	@lizard_dismount_on:
		; slide
		lda dgd_DISMOUNT_ANIM+DISMOUNT_SLOT
		cmp #4
		bcc :+
		lda dgd_DISMOUNT_SLID+DISMOUNT_SLOT
		cmp #42
		bcs :+
			jsr lizard_x_dec
			inc dgd_DISMOUNT_SLID+DISMOUNT_SLOT
		:
		; animate
		lda dgd_EMPTY_SPEED, Y
		clc
		adc #1
		sta dgd_EMPTY_SPEED, Y
		cmp #LIZARD_SWAP_SPEED
		bcs :+
			rts
		:
		lda #0
		sta dgd_EMPTY_SPEED, Y
		; align dismount
		lda dgd_DISMOUNT_ANIM+DISMOUNT_SLOT
		cmp #4
		bcs @aligned
			lda dog_x, Y
			clc
			adc #<37
			sta i
			lda dog_xh, Y
			adc #>37
			sta ih
			lda lizard_x+0
			cmp i
			lda lizard_xh
			sbc ih
			bcs :+
				jsr lizard_x_inc
				jsr dog_dismounter_locate
				inc dgd_DISMOUNT_ANIM+DISMOUNT_SLOT
				lda dgd_DISMOUNT_ANIM+DISMOUNT_SLOT
				and #3
				sta dgd_DISMOUNT_ANIM+DISMOUNT_SLOT
				rts
			:
				lda #4
				sta dgd_DISMOUNT_ANIM+DISMOUNT_SLOT
				rts
			;
		@aligned:
			inc dgd_DISMOUNT_ANIM+DISMOUNT_SLOT
			lda dgd_DISMOUNT_ANIM+DISMOUNT_SLOT
			cmp #13
			bne @end_dismount
				; 13 is middle frame of dismount
				jsr dog_dismounter_flip
				lda #DOG_LIZARD_EMPTY_LEFT
				sta dog_type, Y
				lda dog_x, Y
				clc
				adc #<32
				sta dog_x, Y
				lda dog_xh, Y
				adc #>32
				sta dog_xh, Y
				lda #0
				sta dgd_DISMOUNT_FLIP+DISMOUNT_SLOT
				rts
			@end_dismount:
			cmp #21
			bcs :+
				rts
			:
				; 21 is end frame of dismount
				lda #0
				sta lizard_dismount
				lda #1
				sta lizard_face
				sta dgd_EMPTY_ON, Y
				rts
			;
		;
	;
dog_draw_lizard_empty_right:
	DX_SCROLL_EDGE
	lda dog_y, Y
	tay
	lda #DATA_sprite0_lizard_empty_right
	jsr sprite0_add
	rts

; DOG_LIZARD_DISMOUNTER
stat_dog_lizard_dismounter:

.segment "DATA"

dismount_list:
.byte DATA_sprite0_lizard_walk0_dismount
.byte DATA_sprite0_lizard_walk1_dismount
.byte DATA_sprite0_lizard_walk2_dismount
.byte DATA_sprite0_lizard_walk3_dismount
.byte DATA_sprite0_lizard_dismount0
.byte DATA_sprite0_lizard_dismount1
.byte DATA_sprite0_lizard_dismount2
.byte DATA_sprite0_lizard_dismount3
.byte DATA_sprite0_lizard_dismount2
.byte DATA_sprite0_lizard_dismount4
.byte DATA_sprite0_lizard_dismount5
.byte DATA_sprite0_lizard_dismount6
.byte DATA_sprite0_lizard_dismount7
.byte DATA_sprite0_lizard_dismount7
.byte DATA_sprite0_lizard_dismount6
.byte DATA_sprite0_lizard_dismount5
.byte DATA_sprite0_lizard_dismount4
.byte DATA_sprite0_lizard_dismount3
.byte DATA_sprite0_lizard_dismount2
.byte DATA_sprite0_lizard_dismount1
.byte DATA_sprite0_lizard_dismount0

.export hair_color
hair_color:
; light skin hair
.byte $16,$16,$06,$17,$17,$07,$07,$28,$28,$18,$01,$01,$00,$29,$21,$25
.byte $16,$16,$06,$17,$17,$07,$07,$28,$28,$18,$01,$01,$00,$00,$18,$06
; dark skin hair
.byte $06,$06,$07,$07,$07,$27,$27,$28,$28,$05,$04,$00,$00,$29,$21,$25
.byte $06,$06,$07,$07,$07,$27,$27,$28,$28,$05,$04,$00,$00,$06,$06,$05

.segment "CODE"

get_hair_color: ; clobbers X
	lda human0_hair
	and #31
	sta i
	lda human0_pal
	asl
	asl
	asl
	asl
	asl
	and #%00100000
	ora i
	tax
	lda hair_color, X
	rts

dog_init_lizard_dismounter:
	ldx #0
	@loop_dogs:
		lda dog_type, X
		cmp #DOG_DIAGNOSTIC
		bne :+
			lda human0_pal
			clc
			adc #DATA_palette_human0
			tax
			lda #6
			jsr palette_load
			jsr get_hair_color
			sta palette+27
			rts
		:
		inx
		cpx #16
		bcc @loop_dogs
	rts

dog_tick_lizard_dismounter = dog_tick_none

dog_draw_lizard_dismounter:
	lda lizard_dismount
	bne :+
		rts
	:
	DX_SCROLL_EDGE
	stx u
	; dx in u
	lda #DATA_sprite0_lizard_stand_dismount
	ldx dgd_DISMOUNT_ANIM, Y
	cpx #21
	bcs :+
		lda dismount_list, X
	:
	pha
	cmp #DATA_sprite0_lizard_dismount6
	beq @hair_palette
	cmp #DATA_sprite0_lizard_dismount7
	bne @hair_palette_end
	@hair_palette:
		jsr get_hair_color
		sta palette+23
	@hair_palette_end:
	ldx dgd_DISMOUNT_FLIP, Y
	bne :+
		ldx u
		ldy dog_y+DISMOUNT_SLOT
		pla
		jsr sprite0_add
		rts
	:
		ldx u
		ldy dog_y+DISMOUNT_SLOT
		pla
		jsr sprite0_add_flipped
		rts
	;

; DOG_SPLASHER
stat_dog_splasher:

dgd_SPLASHER_ANIM     = dog_data0
dgd_SPLASHER_BUBBLE   = dog_data1
dgd_SPLASHER_BUBBLE_X = dog_data2
dgd_SPLASHER_BUBBLE_Y = dog_data3
dgd_SPLASHER_SCROLL   = dog_data4;
dog_init_splasher:
	lda #11
	sta dgd_SPLASHER_ANIM + SPLASHER_SLOT
	lda scroll_x+0
	sta dgd_SPLASHER_SCROLL + SPLASHER_SLOT
	rts
dog_tick_splasher:
	; splash
	lda dgd_SPLASHER_ANIM + SPLASHER_SLOT
	cmp #11
	bcs @splash_end
		inc dgd_SPLASHER_ANIM + SPLASHER_SLOT
	@splash_end:
	; bubble
	lda dgd_SPLASHER_BUBBLE + SPLASHER_SLOT
	bne @bubble_update
		lda lizard_wet
		beq @tick_splasher_end_short
		jsr prng
		cmp #2
		bcs @tick_splasher_end_short
		lda lizard_dead
		bne @tick_splasher_end_short
		lda #1
		sta dgd_SPLASHER_BUBBLE + SPLASHER_SLOT
		lda lizard_x+0
		sec
		sbc scroll_x+0 ; NOTE: high byte unnecessary
		sec
		sbc #4
		sta dgd_SPLASHER_BUBBLE_X + SPLASHER_SLOT
		lda lizard_y+0
		sec
		sbc #18
		sta dgd_SPLASHER_BUBBLE_Y + SPLASHER_SLOT
		lda scroll_x+0
		sta dgd_SPLASHER_SCROLL + SPLASHER_SLOT
		jmp @bubble_update
		@tick_splasher_end_short:
			rts ; make sure tick_splasher is also an rts
		;
	@bubble_update:
	lda dgd_SPLASHER_BUBBLE_X + SPLASHER_SLOT
	sta o ; o = ox
	jsr prng2
	and #7
	beq @bubble_right
	cmp #1
	beq @bubble_left
	jmp @bubble_x_end
	@bubble_right:
		lda dgd_SPLASHER_BUBBLE_X + SPLASHER_SLOT
		clc
		adc #1
		sta dgd_SPLASHER_BUBBLE_X + SPLASHER_SLOT
		jmp @bubble_x_end
	@bubble_left:
		lda dgd_SPLASHER_BUBBLE_X + SPLASHER_SLOT
		sec
		sbc #1
		sta dgd_SPLASHER_BUBBLE_X + SPLASHER_SLOT
	@bubble_x_end:
	; adjust for scroll
	lda dgd_SPLASHER_SCROLL + SPLASHER_SLOT
	sec
	sbc scroll_x+0
	beq @bubble_scroll_end
		clc
		adc dgd_SPLASHER_BUBBLE_X + SPLASHER_SLOT
		sta dgd_SPLASHER_BUBBLE_X + SPLASHER_SLOT
		lda scroll_x+0
		sta dgd_SPLASHER_SCROLL + SPLASHER_SLOT
	@bubble_scroll_end:
	lda dgd_SPLASHER_BUBBLE_X + SPLASHER_SLOT
	eor o
	bpl :+ ; did not wrap or cross middle
	lda o
	rol
	eor o
	bmi :+ ; not in quadrant 0 or 3
		; wrapped: kill
		lda #0
		sta dgd_SPLASHER_BUBBLE + SPLASHER_SLOT
	:
	; bubble y update
	lda dgd_SPLASHER_BUBBLE_Y + SPLASHER_SLOT
	sec
	sbc #1
	sta dgd_SPLASHER_BUBBLE_Y + SPLASHER_SLOT
	clc
	adc #6
	cmp water
	bcc @bubble_pop
	lda dgd_SPLASHER_BUBBLE_X + SPLASHER_SLOT
	clc
	adc scroll_x+0
	pha
	lda #0
	adc #0
	sta ih
	pla
	clc
	adc #4
	tax
	lda ih
	adc scroll_x+1
	sta ih
	lda dgd_SPLASHER_BUBBLE_Y + SPLASHER_SLOT
	clc
	adc #6
	tay
	jsr collide_tile
	beq @tick_splasher_end
	@bubble_pop:
		ldy dog_now
		lda #0
		sta dgd_SPLASHER_BUBBLE + SPLASHER_SLOT
	@tick_splasher_end: ; make sure tick_splasher_end_short is also an rts
	rts
dog_draw_splasher:
	; bubble
	lda dgd_SPLASHER_BUBBLE + SPLASHER_SLOT
	beq :+
		ldx dgd_SPLASHER_BUBBLE_X + SPLASHER_SLOT
		ldy dgd_SPLASHER_BUBBLE_Y + SPLASHER_SLOT
		lda #$01
		sta i ; attribute
		lda #$3E
		jsr sprite_tile_add
	:
	; splash
	ldy dog_now
	DX_SCROLL
	lda dgd_SPLASHER_ANIM + SPLASHER_SLOT
	cmp #11
	bcc :+
		rts
	:
	cmp #8
	bcs :+
		lda #1
		sta i
		ldy dog_y + SPLASHER_SLOT
		lda #$38
		jmp sprite_tile_add
		;rts
	:
		lda #1
		sta i
		ldy dog_y + SPLASHER_SLOT
		lda #$39
		jmp sprite_tile_add
		;rts
	;

; DOG_DISCO
stat_dog_disco:

dgd_DISCO_ANIM_BALL  = dog_data0
dgd_DISCO_ANIM_FLOOR = dog_data1
dgd_DISCO_BALL_PAL   = dog_data2
dgd_DISCO_MELT_TIMER = dog_data3
dgd_DISCO_MELT_INDEX = dog_data4

dog_init_disco:
dog_tick_disco:
dog_draw_disco:
	; NOT IN DEMO
	rts

; DOG_WATER_PALETTE
stat_dog_water_palette:

dgd_WATER_PALETTE_FRAME = dog_data0
dgd_WATER_PALETTE_CYCLE = dog_data1
dog_init_water_palette:
	; sound
	lda #$A
	sta player_bg_noise_freq
	lda #2
	sta player_bg_noise_vol
	rts
dog_tick_water_palette:
	; palette
	lda dgd_WATER_PALETTE_FRAME, Y
	clc
	adc #1
	cmp #3
	bcs :+
		sta dgd_WATER_PALETTE_FRAME, Y
		rts
	:
	lda #0
	sta dgd_WATER_PALETTE_FRAME, Y
	lda dgd_WATER_PALETTE_CYCLE, Y
	clc
	adc #1
	cmp #3
	bcc :+
		lda #0
	:
	sta dgd_WATER_PALETTE_CYCLE, Y
	clc
	adc #DATA_palette_waterfall0
	tax ; X = #DATA_palette_waterfall0 + dgd(WATER_PALETTE_CYCLE)
	lda #1 ; slot 1
	jmp palette_load
	;jsr palette_load
	;rts
dog_draw_water_palette:
	rts

; DOG_GRATE
stat_dog_grate:

dog_init_grate:
	DOG_BLOCKER 0,0,15,13
	rts
dog_tick_grate:
	rts
dog_draw_grate:
	DX_SCROLL_EDGE
	lda dog_y, Y
	tay
	lda #DATA_sprite0_grate
	jmp sprite0_add
	;rts

; DOG_GRATE90
stat_dog_grate90:

dog_init_grate90:
	DOG_BLOCKER 0,0,13,15
	rts
dog_tick_grate90:
	rts
dog_draw_grate90:
	DX_SCROLL_EDGE
	lda dog_y, Y
	tay
	lda #DATA_sprite0_grate90
	jmp sprite0_add
	;rts

; DOG_WATER_FLOW
stat_dog_water_flow:

dog_init_water_flow:
	rts
dog_tick_water_flow:
	DOG_BOUND 0,0,32,7
	jsr lizard_overlap
	bne :+
		rts
	:
	lda #1
	sta lizard_flow
	rts
dog_draw_water_flow:
	rts

; DOG_RAINBOW_PALETTE
stat_dog_rainbow_palette:

dgd_RAINBOW_PALETTE_TIME = dog_data0
dog_init_rainbow_palette:
	rts
dog_tick_rainbow_palette:
	ldx dgd_RAINBOW_PALETTE_TIME, Y
	inx
	txa
	sta dgd_RAINBOW_PALETTE_TIME, Y
	lda dog_param, Y
	cmp #4
	bcs @spr_pal
	@bg_pal:
		txa
		and #7
		beq @do_pal
		rts
	@spr_pal:
		txa
		and #3
		beq @do_pal
		rts
	@do_pal:
	lda dog_param, Y
	and #7
	asl
	asl
	tax
	ldy #4
	@rainbow_loop:
		lda palette, X
		sta i ; temporarily store original value
		and #$0F ; A = hue
		cmp #$1
		bcc @end_shift ; hue < 0x1, not a colour
		cmp #$D
		bcs @end_shift ; hue > 0xC, not a colour
			clc
			adc #1
			cmp #$D
			bcc :+
				; wrap $D back to $1
				lda #$1
			:
			sta j ; store new hue
			lda i ; recover original
			and #$F0 ; mask off hue
			ora j ; replace hue
			sta palette, X
		@end_shift:
		inx
		dey
		bne @rainbow_loop
	rts
dog_draw_rainbow_palette:
	rts

; DOG_PUMP
stat_dog_pump:

dog_init_pump:
	jsr prng8
	sta dog_param, Y
	rts
dog_tick_pump:
	lda dog_param, Y
	clc
	adc #2
	sta dog_param, Y
	rts
dog_draw_pump:
	; result of DX_SCROLL is wasted, end up recomputing scroll+x
	ldx dog_param, Y
	lda circle4, X
	DX_SCROLL_OFFSET
	lda dog_y, Y
	tay
	dey
	lda #$03
	sta i
	lda #$F9
	jmp sprite_tile_add

; DOG_SECRET_STEAM
stat_dog_secret_steam:

dog_init_secret_steam:
	; sound
	lda #$D
	sta player_bg_noise_freq
	lda #2
	sta player_bg_noise_vol
	rts
dog_tick_secret_steam:
	; smoke
	jsr prng
	sta dog_data0, Y
	jsr prng
	sta dog_data1, Y
	rts
dog_draw_secret_steam:
	DX_SCREEN
	stx ih ; store X temporarily
	tya
	pha ; store Y temporarily
	lda dog_data0, Y
	and #$40
	ora #$01
	sta i
	ldx ih ; dog_x, Y
	lda dog_y, Y
	tay
	dey
	lda #$23
	jsr sprite_tile_add
	pla ; recover Y
	tay
	lda dog_data1, Y
	and #$40
	ora #$01
	sta i
	lda ih ; dog_x, Y
	clc
	adc #8
	tax
	lda dog_y, Y
	tay
	dey
	lda #$23
	jsr sprite_tile_add
	rts

; DOG_CEILING_FREE
stat_dog_ceiling_free:

dog_init_ceiling_free:
	lda #0
	ldx #0
	:
		sta collision+240, X
		inx
		cpx #16
		bcc :-
	rts
dog_tick_ceiling_free = dog_tick_none
dog_draw_ceiling_free = dog_draw_none

; DOG_BLOCK_COLUMN
stat_dog_block_column:
dog_init_block_column:
	DOG_BLOCKER 0,0,3,255
	rts
dog_tick_block_column:
dog_draw_block_column:
	rts

; DOG_SAVE_STONE
stat_dog_save_stone:

;dgd_SAVE_STONE_ON   = dog_data0 ; enums.s
dgd_SAVE_STONE_ANIM = dog_data1
dog_init_save_stone:
	jsr password_read
	beq @not_on ; password is invalid
	lda current_room+0
	cmp i
	bne @not_on
	lda current_room+1
	cmp k
	bne @not_on
	lda current_lizard
	cmp j
	bne @not_on
	lda last_lizard
	cmp last_lizard_save
	bne @not_on
	lda continued
	beq @not_on
	lda coin_saved
	beq @not_on
	lda #1
	sta dgd_SAVE_STONE_ON, Y
@not_on:
	rts
dog_tick_save_stone:
	; flicker palette 1 in 4 frames
	jsr prng
	and #3
	bne :+
		lda dgd_SAVE_STONE_ANIM, Y
		eor #1
		sta dgd_SAVE_STONE_ANIM, Y
	:
	; skip tick if stone is already on
	lda dgd_SAVE_STONE_ON, Y
	beq :+
		rts
	:
	; check for overlap
	DOG_BOUND -4,4,3,15
	jsr lizard_overlap
	beq @touch_end
		jsr set_continued
		lda #1
		sta dgd_SAVE_STONE_ON, Y
		PLAY_SOUND SOUND_FIRE
		jsr password_build
	@touch_end:
	rts
dog_draw_save_stone:
	lda dgd_SAVE_STONE_ON, Y
	bne :+
		ldx #DATA_palette_save_stone0
		jmp @draw
	:
		lda dgd_SAVE_STONE_ANIM, Y
		bne :+
			ldx #DATA_palette_save_stone1
			jmp @draw
		:
			ldx #DATA_palette_save_stone2
		;
	;
@draw:
	tya
	and #1
	ora #6
	jsr palette_load
	ldy dog_now
	DX_SCROLL_EDGE
	lda dog_y, Y
	tay
	lda #DATA_sprite0_save_stone
	jsr sprite0_add
	rts

; DOG_COIN
stat_dog_coin:

; dogs_common.s
;.segment "DATA"
;coin_cycle_tile: .byte $E2, $E3, $E4, $E5, $E6, $E5, $E4, $E3
;coin_cycle_att:  .byte $03, $03, $03, $43, $03, $03, $43, $43

;dgd_COIN_ON   = dog_data0
;dgd_COIN_ANIM = dog_data1

.segment "CODE"

dog_init_coin:
	lda #1
	sta dgd_COIN_ON, Y
	tya
	pha
	lda dog_param, Y
	jsr coin_read
	beq :+
		pla
		tay
		lda #0
		sta dgd_COIN_ON, Y
		rts
	:
		pla
		tay
		jsr prng8
		sta dgd_COIN_ANIM, Y
		rts
	;
dog_tick_coin:
	lda dgd_COIN_ON, Y
	bne :+
		rts
	:
	DOG_BOUND 3,0,4,7
	jsr lizard_overlap
	beq :+
		lda #0
		sta dgd_COIN_ON, Y
		tya
		pha
		lda dog_param, Y
		jsr coin_take
		pla
		tay
		PLAY_SOUND SOUND_COIN
	:
	tya
	tax
	inc dgd_COIN_ANIM, X
	rts
;dog_draw_coin: ; dogs_common.s

; DOG_MONOLITH
stat_dog_monolith:

dog_init_monolith:
	rts
dog_tick_monolith:
	lda lizard_dead
	beq :+
		rts
	:
	lda lizard_power
	beq :+
	lda current_lizard
	cmp #LIZARD_OF_KNOWLEDGE
	beq :++
	:
		rts
	: ; if lizard_power > 0 && current_lizard == LIZARD_OF_KNOWLEDGE
	lda lizard_face
	bne :+ ; lizard_face == 0
		lda dog_x, Y
		clc
		adc #12
		sta i
		lda dog_xh, Y
		adc #0
		sta ih
		lda i
		cmp lizard_x
		lda ih
		sbc lizard_xh
		bcs :++ ; if (dgx+12) >= lizard_x
		rts
	: ; lizard_face != 0
		lda lizard_x
		clc
		adc #12
		sta i
		lda lizard_xh
		adc #0
		sta ih
		lda i
		cmp dog_x, Y
		lda ih
		sbc dog_xh, Y
		bcs :+ ; if lizard_x >= (dgx-12)
		rts
	:
	lda dog_x, Y
	sec
	sbc #<50
	sta i
	lda dog_xh, Y
	sbc #>50
	sta ih
	lda dog_x, Y
	clc
	adc #<49
	sta k
	lda dog_xh, Y
	adc #>49
	sta kh
	lda #0
	sta j
	lda #255
	sta l
	; dgx-50,0,dgx+49,255
	jsr lizard_overlap
	beq :+
		lda dog_param, Y
		clc
		adc #TEXT_MONOLITH0
		sta game_message
		lda #1
		sta text_select ; message
		sta game_pause
	:
	rts
dog_draw_monolith:
	rts

; DOG_ICEBLOCK
stat_dog_iceblock:

dgd_ICEBLOCK_ANIM = dog_data0

dog_melt_iceblock:
	; dog_now == Y (this condition must be true)
	lda dog_type, Y
	cmp #DOG_ICEBLOCK
	beq :+
		rts
	:
	lda dgd_ICEBLOCK_ANIM, Y
	beq :+
		rts
	:
	lda #1
	sta dgd_ICEBLOCK_ANIM, Y
	; clear collision tiles
	lda dog_xh, Y
	ror
	lda dog_x, Y
	ror
	lsr
	lsr
	sec
	sbc #1
	sta i ; tx = (x>>3) - 1
	lda dog_y, Y
	lsr
	lsr
	lsr
	sta j ; ty = y>>3
	ldx i
	ldy j
	jsr collide_clear_tile ; tx+0,ty+0
	ldx i
	inx
	ldy j
	jsr collide_clear_tile ; tx+1,ty+0
	ldx i
	inc j
	ldy j
	jsr collide_clear_tile ; tx+0,ty+1
	ldx i
	inx
	ldy j
	jsr collide_clear_tile ; tx+1,ty+1
	; begin animation
	ldy dog_now
	lda dog_param, Y
	jsr flag_set
	ldy dog_now
	rts

dog_init_iceblock:
	lda dog_param, Y
	jsr flag_read
	beq :+
		ldy dog_now
		jmp empty_dog
	:
	ldy dog_now
	; turn on collision tiles
	lda dog_xh, Y
	ror
	lda dog_x, Y
	ror
	lsr
	lsr
	sec
	sbc #1
	sta i ; tx = (x>>3) - 1
	lda dog_y, Y
	lsr
	lsr
	lsr
	sta j ; ty = y>>3
	ldx i
	ldy j
	jsr collide_set_tile ; tx+0,ty+0
	ldx i
	inx
	ldy j
	jsr collide_set_tile ; tx+1,ty+0
	ldx i
	inc j
	ldy j
	jsr collide_set_tile ; tx+0,ty+1
	ldx i
	inx
	ldy j
	jsr collide_set_tile ; tx+1,ty+1
	rts
dog_tick_iceblock:
	lda dgd_ICEBLOCK_ANIM, Y
	beq @solid
		clc
		adc #1
		sta dgd_ICEBLOCK_ANIM, Y
		cmp #(7*4)
		bcc :+
			jmp empty_dog
		:
		rts
	@solid:
		lda current_lizard
		cmp #LIZARD_OF_HEAT
		bne :+
		DOG_BOUND -8,0,7,15
		jsr lizard_touch
		beq :+
			jsr dog_melt_iceblock
		:
		rts
	;
dog_draw_iceblock:
	DX_SCROLL_EDGE
	lda dgd_ICEBLOCK_ANIM, Y
	lsr
	lsr
	clc
	adc #DATA_sprite0_iceblock
	pha
	lda dog_y, Y
	tay
	pla
	jmp sprite0_add

; DOG_VATOR
stat_dog_vator:

dgd_VATOR_ANGLE = dog_data0
dog_tick_vator_blocker:
	DOG_BLOCKER -12,0,11,7
	rts
dog_init_vator:
	lda dog_param, Y
	cmp #0
	bne @check_1
		jsr @rnd_angle
		jsr move_finish_x ; dog_x:xh += A
		jmp @done
		@rnd_angle:
			jsr prng8
			sta dgd_VATOR_ANGLE, Y
			tax
			lda circle32, X
			rts
		;
	@check_1:
	cmp #1
	bne @check_4
		jsr @rnd_angle
		jsr move_finish_y ; dog_y += A
		jmp @done
	@check_4:
	cmp #4
	bne @check_5
		lda dog_y, Y
		cmp #120
		bcc :+
			lda #16
			sta dgd_VATOR_ANGLE, Y
			jmp @done
		:
		lda #(128+16)
		sta dgd_VATOR_ANGLE, Y
		ldx #(128+16+64)
		lda circle32, X
		clc
		adc #<396
		sta dog_x, Y
		lda circle32, X
		jsr load_sign
		adc #>396
		sta dog_xh, Y
		ldx #(128+16+0)
		lda circle32, X
		clc
		adc #120
		sta dog_y, Y
		jmp @done
	@check_5:
	cmp #5
	bne @done
		lda dog_y, Y
		cmp #120
		bcs :+
			lda #32
			sta dgd_VATOR_ANGLE, Y
			jmp @done
		:
		lda #(128+32)
		sta dgd_VATOR_ANGLE, Y
		ldx #(128+32+192)
		lda circle32, X
		clc
		adc #<396
		sta dog_x, Y
		lda circle32, X
		jsr load_sign
		adc #>396
		sta dog_xh, Y
		ldx #(128+32+128)
		lda circle32, X
		clc
		adc #120
		sta dog_y, Y
		;jmp @done
	@done:
	jmp dog_tick_vator_blocker
dog_tick_vator:
	lda #0
	sta m ; m = dx
	sta n ; n = dy
	lda dog_param, Y
	cmp #0
	bne @cmp1
		; 0 > sine X
		ldx dgd_VATOR_ANGLE, Y
		inx
		lda circle32, X
		dex
		sec
		sbc circle32, X
		sta m
		jmp @end_move
	@cmp1:
	cmp #1
	bne @cmp2
		; 1 > sine Y
		ldx dgd_VATOR_ANGLE, Y
		inx
		lda circle32, X
		dex
		sec
		sbc circle32, X
		sta n
		jmp @end_move
	@cmp2:
	cmp #2
	bne @cmp3
		; 2 > up
		lda #-1
		sta n
		jmp @end_move
	@cmp3:
	cmp #3
	bne @cmp4
		; 3 > down
		lda #1
		sta n
		jmp @end_move
	@cmp4:
	cmp #4
	bne @cmp5
		; 4 > round top
		lda dgd_VATOR_ANGLE, Y
		cmp #128
		bcs @round_top
			lda dog_x, Y
			cmp #<396
			lda dog_xh, Y
			sbc #>396
			bcs :+
				lda #-1
				sta n
				jmp @end_move
			:
			lda #1
			sta n
			lda dgd_VATOR_ANGLE, Y
			cmp #127
			bcc :+
				lda #<364
				sec
				sbc dog_x, Y
				sta m ; high byte irrelevant to dx
				lda #248
				sec
				sbc dog_y, Y
				sta n
				lda #255
				sta dgd_VATOR_ANGLE, Y
			:
			jmp @end_move
		@round_top:
			clc
			adc #65
			tax
			lda circle32, X
			dex
			sec
			sbc circle32, X
			sta m
			ldx dgd_VATOR_ANGLE, Y
			inx
			lda circle32, X
			dex
			sec
			sbc circle32, X
			sta n
			jmp @end_move
	@cmp5:
	cmp #5
	bne @end_move
		; 5 > round bottom
		lda dgd_VATOR_ANGLE, Y
		cmp #128
		bcs @round_bottom
			lda dog_x, Y
			cmp #<396
			lda dog_xh, Y
			sbc #>396
			bcc :+
				lda #1
				sta n
				jmp @end_move
			:
			lda #-1
			sta n
			lda dgd_VATOR_ANGLE, Y
			cmp #127
			bcc :+
				lda #<428
				sec
				sbc dog_x, Y
				sta m ; high byte irrelevant to dx
				lda #-8
				sec
				sbc dog_y, Y
				sta n
				lda #255
				sta dgd_VATOR_ANGLE, Y
			:
			jmp @end_move
		@round_bottom:
			clc
			adc #193
			tax
			lda circle32, X
			dex
			sec
			sbc circle32, X
			sta m
			lda dgd_VATOR_ANGLE, Y
			clc
			adc #129
			tax
			lda circle32, X
			dex
			sec
			sbc circle32, X
			sta n
		;jmp @end_move
	@end_move:
	; is lizard riding?
	DOG_BOUND -12,-1,11,-1
	jsr lizard_overlap
	beq @not_riding
		lda lizard_x+0
		clc
		adc m
		sta lizard_x+0
		lda m
		jsr load_sign
		adc lizard_xh
		sta lizard_xh
		lda lizard_y+0
		clc
		adc n
		sta lizard_y+0
		jsr do_scroll
		lda m
		jsr move_finish_x
		lda n
		jsr move_finish_y
		lda dgd_VATOR_ANGLE, Y
		clc
		adc #1
		sta dgd_VATOR_ANGLE, Y
		jmp dog_tick_vator_blocker
	@not_riding:
	; i:ih = dgx + dx - 12
	lda dog_x, Y
	clc
	adc m
	sta i
	lda m
	jsr load_sign
	adc dog_xh, Y
	sta ih
	lda i
	sec
	sbc #<12
	sta i
	lda ih
	sbc #>12
	sta ih
	; k:kh = dgx + dx + 11
	lda i
	clc
	adc #<(11+12)
	sta k
	lda ih
	adc #>(11+12)
	sta kh
	; j = dgy + dy
	lda dog_y, Y
	clc
	adc n
	sta j
	; l = dgy + dy + 7
	clc
	adc #7
	sta l
	; i = dgx+dx-12
	; j = dgy+dy
	; k = dgx+dx+11
	; l = dgy+dy+7
	jsr lizard_overlap ; is the lizard blocking the way?
	beq :+
		rts
	:
	lda m
	jsr move_finish_x
	lda n
	jsr move_finish_y
	lda dgd_VATOR_ANGLE, Y
	clc
	adc #1
	sta dgd_VATOR_ANGLE, Y
	jmp dog_tick_vator_blocker
	;
dog_draw_vator:
	DX_SCROLL_EDGE
	lda dog_y, Y
	tay
	lda #DATA_sprite0_vator
	jsr sprite0_add
	rts

; DOG_NOISE
stat_dog_noise:

dog_init_noise:
	lda dog_x, Y
	and #15
	sta player_bg_noise_freq
	lda dog_y, Y
	and #15
	sta player_bg_noise_vol
	rts
dog_tick_noise:
dog_draw_noise:
	rts


; DOG_SNOW
stat_dog_snow:

dgd_DOG_SNOW_X = dog_data0
dgd_DOG_SNOW_Y = dog_data6
dog_init_snow:
	lda scroll_x+0
	sta dog_x, Y
	lda scroll_x+1
	sta dog_xh, Y
	ldx #6
	:
		jsr prng2
		sta dgd_DOG_SNOW_X, Y
		jsr prng2
		sta dgd_DOG_SNOW_Y, Y
		; increment Y+16 skips to next dog_data#
		tya
		clc
		adc #16
		tay
		dex
		bne :-
	;
	ldy dog_now
	lda #$01
	sta dog_param, Y
	lda dog_type + DISMOUNT_SLOT
	cmp #DOG_LIZARD_DISMOUNTER
	bne :+
		lda #$03
		sta dog_param, Y
	:
	;jmp dog_tick_snow
dog_tick_snow:
	; update scroll
	lda dog_x, Y
	sec
	sbc scroll_x+0
	beq @scroll_ready
		sta i ; i = dgx - zp.scroll_x
		; store new scroll
		lda scroll_x+0
		sta dog_x, Y
		lda scroll_x+1
		sta dog_xh, Y
		; move snow
		ldx #6
		:
			lda dgd_DOG_SNOW_X, Y
			clc
			adc i
			sta dgd_DOG_SNOW_X, Y
			tya
			clc
			adc #16
			tay
			dex
			bne :-
		ldy dog_now
	@scroll_ready:
	; update snow
	ldx #6
	@tick_snow_loop:
		sty j
		stx k
		; move
		jsr prng
		and #1
		beq :+
			lda dgd_DOG_SNOW_X, Y
			clc
			adc #2
			sta dgd_DOG_SNOW_X, Y
		:
		lda dgd_DOG_SNOW_Y, Y
		clc
		adc #2
		sta dgd_DOG_SNOW_Y, Y
		; respawn
		lda dgd_DOG_SNOW_X, Y
		clc
		adc scroll_x+0
		tax
		lda scroll_x+1
		adc #0
		sta ih
		lda dgd_DOG_SNOW_Y, Y
		tay
		jsr collide_tile
		beq @respawn_end
		@respawn:
			ldy j
			jsr prng
			and #1
			beq @spawn_top
			@spawn_side:
				lda #0
				sta dgd_DOG_SNOW_X, Y
				clc
				adc scroll_x+0
				tax
				lda scroll_x+1
				adc #0
				sta ih
				jsr prng
				sta dgd_DOG_SNOW_Y, Y
				tay
				jsr collide_tile
				beq @respawn_end
				ldy j
				; spawn on side failed, just spawn on top
			@spawn_top:
				jsr prng
				sta dgd_DOG_SNOW_X, Y
				lda #0
				sta dgd_DOG_SNOW_Y, Y
			;
		@respawn_end:
		; increment Y+16 skips to next dog_data#
		lda j
		clc
		adc #16
		tay
		ldx k
		dex
		bne @tick_snow_loop
	rts
dog_draw_snow:
	lda dog_param, Y
	sta i ; attribute
	lda #$C6
	sta l ; tile
dog_draw_snow_particles:
	ldx #6
	:
		sty j
		stx k
		lda dgd_DOG_SNOW_X, Y
		tax
		lda dgd_DOG_SNOW_Y, Y
		tay
		lda l
		jsr sprite_tile_add
		; increment Y+16 skips to next dog_data#
		lda j
		clc
		adc #16
		tay
		ldx k
		dex
		bne :-
	rts

; DOG_RAIN
stat_dog_rain:

; note: rain sound is duplicated in main.s for special case when rain applies during pause

.segment "CODE"
dog_init_rain:
	lda #$E
	sta player_bg_noise_freq
	lda #3
	sta player_bg_noise_vol
	jsr dog_init_snow
	ldy dog_now
	lda #$03
	sta dog_param, Y
	rts
dog_tick_rain:
	sty l
	jsr dog_tick_snow
	ldy l
	jmp dog_tick_snow
	;rts
dog_draw_rain:
	lda dog_param, Y
	sta i ; attribute
	lda #$FC
	sta l ; tile
	jmp dog_draw_snow_particles

; DOG_RAIN_BOSS
stat_dog_rain_boss:

dog_init_rain_boss:
	jsr dog_init_rain
	lda #$02
	sta dog_param, Y
	rts
dog_tick_rain_boss = dog_tick_rain
dog_draw_rain_boss = dog_draw_rain

; DOG_DRIP
stat_dog_drip:

dgd_DRIP_ANIM = dog_data0
dgd_DRIP_X0   = dog_data1
dgd_DRIP_X1   = dog_data2
dgd_DRIP_Y    = dog_data3

dog_init_drip:
	lda dog_xh, Y
	sta dgd_DRIP_X0, Y
	lda dog_x, Y
	sta dgd_DRIP_X1, Y
	lda dog_y, Y
	sta dgd_DRIP_Y, Y
	jsr prng2
	sta dgd_DRIP_ANIM, Y
	rts
dog_tick_drip:
	lda dgd_DRIP_ANIM, Y
	beq @fall
		sec
		sbc #1
		sta dgd_DRIP_ANIM, Y
		bne :+
			lda dgd_DRIP_Y, Y
			sta dog_y, Y
			jsr prng
			and #7
			clc
			adc dgd_DRIP_X1, Y
			sta dog_x, Y
			lda #0
			adc dgd_DRIP_X0, Y
			sta dog_xh, Y
		:
		rts
	@fall:
	lda dog_y, Y
	clc
	adc #4
	sta dog_y, Y
	cmp #240
	bcs @drip_reset
	clc
	adc #2
	sta j ; temp = dgy+2
	lda dog_xh, Y
	sta ih
	ldx dog_x, Y
	ldy j ; dgy+2
	jsr collide_tile
	beq @drip_reset_end
	@drip_reset:
		ldy dog_now
		jsr prng2
		and #63
		clc
		adc #192
		sta dgd_DRIP_ANIM, Y
		rts
	@drip_reset_end:
	cpy water
	bcc @splash_end
		ldy dog_now
		PLAY_SOUND_SCROLL SOUND_DRIP
		lda dog_type + SPLASHER_SLOT
		cmp #DOG_SPLASHER
		bne @splash_end
		lda dog_x, Y
		sec
		sbc #4
		sta dog_x + SPLASHER_SLOT
		lda dog_xh, Y
		sbc #0
		sta dog_xh + SPLASHER_SLOT
		lda water
		sec
		sbc #9
		sta dog_y + SPLASHER_SLOT
		lda #0
		sta dog_data0 + SPLASHER_SLOT
		jmp @drip_reset
	@splash_end:
	rts
dog_draw_drip:
	lda dgd_DRIP_ANIM, Y
	beq :+
		rts
	:
	DX_SCROLL
	lda dog_y, Y
	tay
	lda #$03
	sta i
	lda #$FC
	jmp sprite_tile_add

; DOG_HOLD_SCREEN
stat_dog_hold_screen:

dog_init_hold_screen:
dog_tick_hold_screen:
dog_draw_hold_screen:
	rts

; DOG_BOSS_DOOR
stat_dog_boss_door:

;dgd_RAINBOW_PALETTE_TIME = dog_data0
;dgd_BOSS_DOOR_ANIM   = dog_data1
;dgd_BOSS_DOOR_WOBBLE = dog_data2

dog_tick_boss_door_wobble:
	ldx dgd_BOSS_DOOR_WOBBLE, Y
	lda circle4, X
	sta i
	inx
	txa
	sta dgd_BOSS_DOOR_WOBBLE, Y
	lda circle4, X
	sec
	sbc i
	clc
	adc dog_y, Y
	sta dog_y, Y ; dog_y += (b-a)
	rts

; had to define these up here because they interfere with local labels
METRIC_BLOCK_START = metric_time_h
METRIC_BLOCK_END   = metric_cheater

;dog_init_boss_door ; dogs_common.s
dog_tick_boss_door:
	lda dgd_BOSS_DOOR_ANIM, Y
	cmp #22
	jcc @open_end
		lda #22
		sta dgd_BOSS_DOOR_ANIM, Y
		lda gamepad
		and #PAD_U
		beq @open_end
		DOG_BOUND -8,0,7,15
		jsr lizard_overlap
		beq @enter_end
			lda dog_type, Y
			cmp #DOG_BOSS_RUSH
			beq @enter_rush
				lda #1
				sta current_door
				jmp @enter_rush_end
			@enter_rush:
				sty current_door
				lda boss_rush
				bne @enter_rush_end
				lda #1
				sta boss_rush
				; reset metrics on first rush entry
				.assert (metric_time_h   >= METRIC_BLOCK_START && metric_time_h   < METRIC_BLOCK_END), error, "metric RAM block not contiguous"
				.assert (metric_time_m   >= METRIC_BLOCK_START && metric_time_m   < METRIC_BLOCK_END), error, "metric RAM block not contiguous"
				.assert (metric_time_s   >= METRIC_BLOCK_START && metric_time_s   < METRIC_BLOCK_END), error, "metric RAM block not contiguous"
				.assert (metric_time_f   >= METRIC_BLOCK_START && metric_time_f   < METRIC_BLOCK_END), error, "metric RAM block not contiguous"
				.assert (metric_bones    >= METRIC_BLOCK_START && metric_bones    < METRIC_BLOCK_END), error, "metric RAM block not contiguous"
				.assert (metric_jumps    >= METRIC_BLOCK_START && metric_jumps    < METRIC_BLOCK_END), error, "metric RAM block not contiguous"
				.assert (metric_continue >= METRIC_BLOCK_START && metric_continue < METRIC_BLOCK_END), error, "metric RAM block not contiguous"
				lda #0
				tax
				:
					sta METRIC_BLOCK_START, X
					inx
					cpx #(METRIC_BLOCK_END - METRIC_BLOCK_START)
					bcc :-
				; reset all the boss flags when the boss rush begins (just in case)
				tya
				pha
				ldx #FLAG_BOSS_0_MOUNTAIN
				@cheat_test:
					txa
					pha
					jsr flag_clear
					pla
					tax
					inx
					cpx #(FLAG_BOSS_5_VOID+1)
					bcc @cheat_test
				pla
				tay
			@enter_rush_end:
			ldx current_door
			lda door_link, X
			sta current_room+0
			lda door_linkh, X
			sta current_room+1
			lda #1
			sta room_change
		@enter_end:
	@open_end:
	jsr dog_tick_boss_door_wobble
	lda dog_type, Y
	cmp #DOG_BOSS_RUSH
	bne :+
		rts
	:
	lda dgd_RAINBOW_PALETTE_TIME, Y
	and #1
	bne :+
		tya
		tax
		inc dgd_BOSS_DOOR_ANIM, X
	:
	jsr dog_tick_rainbow_palette
	rts

dog_draw_boss_door:
	lda dog_type+0
	cmp #DOG_FROB
	bne @scroll_normal
	@scroll_frob:
		DX_SCREEN
		jmp @scroll_end
	@scroll_normal:
		DX_SCROLL_EDGE
	@scroll_end:
	; read these into ZP for quick use and to free up Y
	; j = boss_door_anim
	; k = dog_x
	; l = dog_y
	lda dgd_BOSS_DOOR_ANIM, Y
	sta j
	;lda dog_x, Y
	;sta k
	stx k
	lda dog_y, Y
	sta l
	;
	; 4 corners
	;
	lda #0
	sta m ; corner_offset
	sta n ; top_offset
	lda j
	cmp #5
	bcs :+
		lda #5
		sec
		sbc j
		sta m ; corner_offset = 5 - anim
		lda #16
		sta n ; top_offset = 16
		jmp :++
	:
	cmp #21
	bcs :+
		lda #21
		sec
		sbc j
		sta n ; top_offset = 21 - anim
	:
	; corner bottom left
	lda l
	clc
	adc #15
	tay
	sty o ; o = dgy+15
	lda k
	sec
	sbc #16
	clc
	adc m
	tax
	stx p ; p = (dgx-16)+corner_offset
	lda #$02
	sta i
	lda #$C8
	jsr sprite_tile_add_clip ; clobbers A,X
	; corner bottom right
	lda k
	clc
	adc #8
	sec
	sbc m
	tax
	stx q ; q = (dgx+8)-corner_offset
	lda #$42
	sta i
	lda #$C8
	jsr sprite_tile_add_clip
	; corner top left
	ldx p
	lda l
	sec
	sbc #9
	clc
	adc n
	tay
	sty r ; r = (dgy-9)+top_offset
	lda #$82
	sta i
	lda #$C8
	jsr sprite_tile_add_clip
	; corner top right
	ldx q
	lda #$C2
	sta i
	lda #$C8
	jsr sprite_tile_add_clip
	;
	; top/bottom
	;
	lda #0
	sta m ; bottom_offset = 0
	lda j
	cmp #3
	bcs :+
		lda #2
		sta m ; bottom_offset = 2
	:
	; bottom left
	lda k
	clc
	adc m
	sec
	sbc #8
	tax
	stx p ; p = (dgx-8)+bottom_offset
	ldy o ; dgy+15
	lda #$02
	sta i
	lda #$C9
	jsr sprite_tile_add_clip
	; bottom right
	lda k
	sec
	sbc m
	tax
	stx q ; q = (dgx+0)-bottom_offset
	lda #$C9
	jsr sprite_tile_add_clip
	; top left
	ldx p
	ldy r ; (dgy-9)+top_offset
	lda #$82
	sta i
	lda #$C9
	jsr sprite_tile_add_clip
	; top right
	ldx q
	lda #$C9
	jsr sprite_tile_add_clip
	;
	; lower-middle
	;
	ldy j
	cpy #6
	bcc @draw_end ; if anim<6 return
	lda l
	clc
	adc #7 ; dgy+7
	cpy #10 ; if anim<10 y+=4
	ldy #$D8
	bcs :+
		ldy #$F7 ; short tile if 
	:
	jsr @middle_row
	;
	; upper_middle
	;
	ldy j
	cpy #14
	bcc @draw_end ; if anim<14 return
	lda l
	sec
	sbc #1
	ldy j
	cpy #18 ; if anim<14 y+=r
	ldy #$D8
@middle_row:
	; A = anim
	; Y = tile
	; carry = anim test for y += 4
	sty s ; tile
	bcs :+
		;clc
		adc #4
	:
	tay ; y
	lda k
	sec
	sbc #8
	tax ; dgx-8
	lda #$02
	sta i
	lda #$D9
	jsr sprite_tile_add_clip
	ldx k ; dgx+8
	lda #$D9
	jsr sprite_tile_add_clip
	lda k
	sec
	sbc #16
	tax ; dgx-16
	lda s
	jsr sprite_tile_add_clip
	lda k
	clc
	adc #8
	tax ; dgx+8
	lda #$42
	sta i
	lda s
	jsr sprite_tile_add_clip
@draw_end:
	rts

; DOG_BOSS_DOOR_RAIN

stat_dog_boss_door_rain:

dog_init_boss_door_rain = dog_init_boss_door

dog_tick_boss_door_rain:
	jsr dog_tick_boss_door
	lda #$12
	sta palette + 27
	rts

dog_draw_boss_door_rain = dog_draw_boss_door

; DOG_BOSS_DOOR_EXIT
stat_dog_boss_door_exit:

dog_init_boss_door_exit:
	jsr dog_init_boss_door
dog_init_boss_door_exit_common:
	PLAY_SOUND SOUND_BOSS_DOOR_CLOSE
	lda #22
	sta dgd_BOSS_DOOR_ANIM, Y
	rts

dog_tick_boss_door_exit_common:
	lda dgd_BOSS_DOOR_ANIM, Y
	bne :+
		jmp empty_dog
	:
	jsr dog_tick_boss_door_wobble
	lda dgd_RAINBOW_PALETTE_TIME, Y
	and #1
	bne :+
		tya
		tax
		dec dgd_BOSS_DOOR_ANIM, X
	:
	rts

dog_tick_boss_door_exit:
	jsr dog_tick_boss_door_exit_common
	jmp dog_tick_rainbow_palette

dog_draw_boss_door_exit = dog_draw_boss_door

; DOG_BOSS_DOOR_EXEUNT
stat_dog_boss_door_exeunt:

dog_init_boss_door_exeunt:
	jsr dog_init_boss_door_common
	jmp dog_init_boss_door_exit_common

dog_tick_boss_door_exeunt:
	jsr dog_tick_boss_door_exit_common
	tya
	tax
	inc dgd_RAINBOW_PALETTE_TIME, X
	rts

dog_draw_boss_door_exeunt = dog_draw_boss_door

; DOG_BOSS_RUSH
stat_dog_boss_rush:

dog_init_boss_rush:
	tya
	pha
	lda dog_param, Y
	clc
	adc #FLAG_BOSS_0_MOUNTAIN
	jsr flag_read
	beq :+
		pla
		tay
		jmp empty_dog
	:
	pla
	tay
	lda #22
	sta dgd_BOSS_DOOR_ANIM, Y
	jsr prng8
	sta dgd_BOSS_DOOR_WOBBLE, Y
	tax
	lda dog_y, Y
	clc
	adc circle4, X
	sta dog_y, Y
	jmp dog_init_boss_door_palette

dog_tick_boss_rush = dog_tick_boss_door
dog_draw_boss_rush = dog_draw_boss_door

; DOG_OTHER
stat_dog_other:

dgd_OTHER_ANIM    = dog_data0
dgd_OTHER_FACE    = dog_data1
dgd_OTHER_SPRITE  = dog_data2
dgd_OTHER_BLINK   = dog_data3
dgd_OTHER_PALETTE = dog_data4
OTHER_HITBOX_L = -(5+4)
OTHER_HITBOX_R = (4+4)
OTHER_HITBOX_T = -14
OTHER_HITBOX_B = -1
OTHER_BLINK_POINT = 233

dog_init_other:
	; data
	lda #0
	sta dgd_OTHER_ANIM, Y
	sta dgd_OTHER_FACE, Y
	sta dgd_OTHER_PALETTE, Y
	lda #DATA_sprite0_other_stand
	sta dgd_OTHER_SPRITE, Y
	jsr prng
	sta dgd_OTHER_BLINK, Y
	; facing
	lda lizard_x
	cmp dog_x, Y
	tya
	tax
	rol dgd_OTHER_FACE, X 
	; blocker
	lda dog_x, Y
	clc
	adc #<OTHER_HITBOX_L
	sta blocker_x0+0
	lda dog_xh, Y
	adc #>OTHER_HITBOX_L
	sta blocker_xh0+0
	lda dog_x, Y
	clc
	adc #<OTHER_HITBOX_R
	sta blocker_x1+0
	lda dog_xh, Y
	adc #>OTHER_HITBOX_R
	sta blocker_xh1+0
	lda dog_y, Y
	clc
	adc #OTHER_HITBOX_T
	sta blocker_y0+0
	lda dog_y, Y
	adc #OTHER_HITBOX_B
	sta blocker_y1+0
	ldy dog_now
	; palettes
	lda human1_pal
	clc
	adc #DATA_palette_human0_black
	tax
	lda #6
	jsr palette_load
	ldx #DATA_palette_lizard10
	lda #7
	jsr palette_load
	lda dog_type + 3
	cmp #DOG_RAIN_BOSS
	bne :+
		lda #$12
		sta palette+(6*4)+3
	:
	rts
dog_tick_other_talk_in:
	.assert (TEXT_END_KNOWLEDGE + LIZARD_OF_KNOWLEDGE = TEXT_END_KNOWLEDGE),error,"Ending text out of order."
	.assert (TEXT_END_KNOWLEDGE + LIZARD_OF_BOUNCE    = TEXT_END_BOUNCE   ),error,"Ending text out of order."
	.assert (TEXT_END_KNOWLEDGE + LIZARD_OF_SWIM      = TEXT_END_SWIM     ),error,"Ending text out of order."
	.assert (TEXT_END_KNOWLEDGE + LIZARD_OF_HEAT      = TEXT_END_HEAT     ),error,"Ending text out of order."
	.assert (TEXT_END_KNOWLEDGE + LIZARD_OF_SURF      = TEXT_END_SURF     ),error,"Ending text out of order."
	.assert (TEXT_END_KNOWLEDGE + LIZARD_OF_PUSH      = TEXT_END_PUSH     ),error,"Ending text out of order."
	.assert (TEXT_END_KNOWLEDGE + LIZARD_OF_STONE     = TEXT_END_STONE    ),error,"Ending text out of order."
	.assert (TEXT_END_KNOWLEDGE + LIZARD_OF_COFFEE    = TEXT_END_COFFEE   ),error,"Ending text out of order."
	.assert (TEXT_END_KNOWLEDGE + LIZARD_OF_LOUNGE    = TEXT_END_LOUNGE   ),error,"Ending text out of order."
	.assert (TEXT_END_KNOWLEDGE + LIZARD_OF_DEATH     = TEXT_END_DEATH    ),error,"Ending text out of order."
	.assert (TEXT_END_KNOWLEDGE + LIZARD_OF_BEYOND    = TEXT_END_BEYOND   ),error,"Ending text out of order."
	lda current_lizard
	clc
	adc #TEXT_END_KNOWLEDGE
	sta game_message
	lda #1
	sta game_pause
	rts
dog_tick_other_kill_doors:
	ldx #0
	@loop:
		lda dog_type, X
		cmp #DOG_DOOR
		beq :+
		cmp #DOG_PASS
		beq :+
		cmp #DOG_PASS_X
		beq :+
		cmp #DOG_PASS_Y
		beq :+
		jmp :++
		:
			lda #DOG_NONE
			sta dog_type, X
		:
		inx
		cpx #16
		bcc @loop
	rts
dog_tick_other_ending:
	lda #0
	sta current_door
	lda current_lizard
	clc
	adc #<DATA_room_ending0
	sta current_room+0
	lda #>DATA_room_ending0
	adc #0
	sta current_room+1
	lda #1
	sta room_change
	rts
dog_tick_other_idle:
	; Y = dog
	; adjust facing
	lda lizard_y
	cmp #240
	bcs @face_end
		lda lizard_x
		sec
		sbc dog_x, Y
		bcc :+ ; lizard_x < dgx
		cmp #8
		bcc :+
			; if (lizard_x-dgx) >= 8
			lda #1
			sta dgd_OTHER_FACE, Y
			jmp :++
		:
		lda dog_x, Y
		sec
		sbc lizard_x
		bcc :+ ; dgx < dgx
		cmp #8
		bcc :+
			; if (dgx-lizard_x) >= 8
			lda #0
			sta dgd_OTHER_FACE, Y
		:
	@face_end:
	; blink
	ldx dgd_OTHER_BLINK, Y
	cpx #(OTHER_BLINK_POINT+8)
	bcc :+
		lda #0
		sta dgd_OTHER_BLINK, Y
		rts
	:
		inx
		txa
		sta dgd_OTHER_BLINK, Y
		rts
	;
dog_tick_other_simple:
	; Y = dog
	lda dgd_OTHER_ANIM, Y
	beq @idle
		cmp #64
		bcc :+
			jsr dog_tick_other_ending
		:
		clc
		adc #1
		sta dgd_OTHER_ANIM, Y
		rts
	@idle:
	jsr dog_tick_other_idle
	lda lizard_power
	beq :+
	DOG_BOUND -20,-5,20,-1
	jsr lizard_overlap
	beq :+
		lda #1
		sta dgd_OTHER_ANIM, Y
		jsr dog_tick_other_kill_doors
		lda #0
		sta lizard_vx+0
		sta lizard_vx+1
		jsr dog_tick_other_talk_in
	:
	rts
dog_tick_other_push:
	; Y = dog
	lda dgd_OTHER_ANIM, Y
	beq @idle
		cmp #64
		bcs @phase_2
			; dgd_OTHER_ANIM < 64
			lda #DATA_sprite0_other_skid
			sta dgd_OTHER_SPRITE, Y
			lda dgd_OTHER_ANIM, Y
			and #1
			beq @done
				lda dgd_OTHER_FACE, Y
				bne @move_left
				@move_right:
					lda dog_x, Y
					clc
					adc #1
					sta dog_x, Y
					inc blocker_x0+0
					bne :+
						inc blocker_xh0+0
					:
					inc blocker_x1+0
					bne :+
						inc blocker_xh1+0
					:
					jmp @done
				@move_left:
					lda dog_x, Y
					sec
					sbc #1
					sta dog_x, Y
					dec blocker_x0+0
					lda blocker_x0+0
					cmp #255
					bne :+
						dec blocker_xh0+0
					:
					dec blocker_x1+0
					lda blocker_x1+0
					cmp #255
					bne :+
						dec blocker_xh1+0
					:
				;
			jmp @done
		@phase_2:
		bne @phase_3
			; dgd_OTHER_ANIM == 64
			lda #DATA_sprite0_other_stand
			sta dgd_OTHER_SPRITE, Y
			jsr dog_tick_other_talk_in
			jmp @done
		@phase_3:
		cmp #128
		bcc @done
			; dgd_OTHER_ANIM >= 128
			jsr dog_tick_other_ending
			jmp @done
		@done:
		lda dgd_OTHER_ANIM, Y
		clc
		adc #1
		sta dgd_OTHER_ANIM, Y
		rts
	@idle:
	jsr dog_tick_other_idle
	lda lizard_power
	bne :+
		rts
	:
	lda gamepad
	and #PAD_R
	beq :+
	DOG_BOUND (OTHER_HITBOX_L-1),OTHER_HITBOX_T,(OTHER_HITBOX_L-1),OTHER_HITBOX_B
	jsr lizard_overlap
	beq :+
		PLAY_SOUND SOUND_PUSH
		lda #1
		sta dgd_OTHER_ANIM, Y
		jsr dog_tick_other_kill_doors
	:
	lda gamepad
	and #PAD_L
	beq :+
	DOG_BOUND (OTHER_HITBOX_R+1),OTHER_HITBOX_T,(OTHER_HITBOX_R+1),OTHER_HITBOX_B
	jsr lizard_overlap
	beq :+
		PLAY_SOUND SOUND_PUSH
		lda #1
		sta dgd_OTHER_ANIM, Y
		jsr dog_tick_other_kill_doors
	:
	rts
dog_tick_other_heat:
	lda dgd_OTHER_ANIM, Y
	bne @running
		jsr dog_tick_other_idle
		lda lizard_power
		beq :+
		DOG_BOUND OTHER_HITBOX_L,OTHER_HITBOX_T,OTHER_HITBOX_R,OTHER_HITBOX_B
		jsr lizard_touch
		beq :+
			lda #0
			sta dgd_OTHER_PALETTE, Y
			sta dgd_OTHER_BLINK, Y
			lda #1
			sta dgd_OTHER_ANIM, Y
			jsr dog_tick_other_kill_doors
		:
		rts
	@running:
	cmp #80
	bcs @phase_2
		; play sound
		PLAY_SOUND SOUND_FIRE_CONTINUE
		; cycle burning palette
		lda dgd_OTHER_BLINK, Y
		cmp #3
		bcc :+
			lda #0
			sta dgd_OTHER_BLINK, Y
			lda dgd_OTHER_PALETTE, Y
			clc
			adc #1
			sta dgd_OTHER_PALETTE, Y
			jmp :++
		:
			clc
			adc #1
			sta dgd_OTHER_BLINK, Y
		:
		lda dgd_OTHER_PALETTE, Y
		cmp #3
		bcc :+
			lda #0
			sta dgd_OTHER_PALETTE, Y
		:
		; set burning palette
		tya
		pha
		lda dgd_OTHER_PALETTE, Y
		clc
		adc #DATA_palette_lava0
		tax
		lda #7
		jsr palette_load ; clobbers Y
		pla
		tay
		; blink
		lda #DATA_sprite0_other_blink
		sta dgd_OTHER_SPRITE, Y
		jmp @done
	@phase_2:
	cmp #161
	bcs @phase_3
		; set charred palette
		tya
		pha
		lda #7
		ldx #DATA_palette_lizard9
		jsr palette_load ; clobbers Y
		pla
		tay
		; blink
		lda dgd_OTHER_ANIM, Y
		and #31
		cmp #23
		bcc :+
			lda #DATA_sprite0_other_stand
			sta dgd_OTHER_SPRITE, Y
			jmp :++
		:
			lda #DATA_sprite0_other_blink
			sta dgd_OTHER_SPRITE, Y
		:
		; end of phase
		lda dgd_OTHER_ANIM, Y
		cmp #160
		bne :+
			lda #DATA_sprite0_other_stand
			sta dgd_OTHER_SPRITE, Y
			jsr dog_tick_other_talk_in
		:
		jmp @done
	@phase_3:
	cmp #254
	bcs @phase_4
		lda #DATA_sprite0_other_stand
		sta dgd_OTHER_SPRITE, Y
		jmp @done
	@phase_4:
		jsr dog_tick_other_ending
	@done:
	lda dgd_OTHER_ANIM, Y
	clc
	adc #1
	sta dgd_OTHER_ANIM, Y
	rts
dog_tick_other_death:
	; Y = dog
	lda dgd_OTHER_ANIM, Y
	bne @timeout
		jsr dog_tick_other_idle
		lda lizard_power
		beq :+
		DOG_BOUND OTHER_HITBOX_L,OTHER_HITBOX_T,OTHER_HITBOX_R,OTHER_HITBOX_B
		jsr lizard_touch
		beq :+
			; initialize bones in slot OTHER_BONES_SLOT
			lda #DOG_BONES
			sta dog_type+OTHER_BONES_SLOT
			lda dog_x, Y
			sta dog_x+OTHER_BONES_SLOT
			lda dog_y, Y
			sta dog_y+OTHER_BONES_SLOT
			lda #DATA_sprite0_other_bones
			sta dog_param+OTHER_BONES_SLOT
			lda #0
			sta dgd_BONES_INIT+OTHER_BONES_SLOT
			lda dgd_OTHER_FACE, Y
			sta dgd_BONES_FLIP+OTHER_BONES_SLOT
			; remove blocker
			ldy #0
			jsr empty_blocker
			ldy dog_now
			; other misc stuff
			lda #1
			sta dgd_OTHER_ANIM, Y
			lda #DATA_sprite0_none0
			sta dgd_OTHER_SPRITE, Y
			jsr inc_bones
			PLAY_SOUND SOUND_BONES
			jsr dog_tick_other_kill_doors
		:
		rts
	@timeout:
	cmp #250
	bcc :+
		jsr dog_tick_other_ending
	:
	lda dgd_OTHER_ANIM, Y
	clc
	adc #1
	sta dgd_OTHER_ANIM, Y
	rts
dog_tick_other_substance:
	; Y = dog
	lda dgd_OTHER_ANIM, Y
	jne dog_tick_other_simple
	jsr dog_tick_other_idle
	lda #30
	cmp lizard_power
	bcs :+
	DOG_BOUND -20,-5,20,-1
	jsr lizard_overlap
	beq :+
		; if lizard_power > 30 &&
		; lizard_overlap(dgx-20,dgy-5,dgx+20,dgy-1)
		lda #1
		sta dgd_OTHER_ANIM, Y
		jsr dog_tick_other_kill_doors
		lda #0
		sta lizard_vx+0
		sta lizard_vx+1
		jsr dog_tick_other_talk_in
	:
	rts
dog_tick_other:
	lda current_lizard
	cmp #LIZARD_OF_HEAT
	jeq dog_tick_other_heat
	cmp #LIZARD_OF_PUSH
	jeq dog_tick_other_push
	cmp #LIZARD_OF_DEATH
	jeq dog_tick_other_death
	cmp #LIZARD_OF_COFFEE
	beq :+
	cmp #LIZARD_OF_LOUNGE
	beq :+
	jmp :++
	:
		jmp dog_tick_other_substance
	:
	jmp dog_tick_other_simple
dog_draw_other:
	
	; if in rest pose and it's blink time, choose the blink sprite
	lda dgd_OTHER_SPRITE, Y
	cmp #DATA_sprite0_none0
	bne :+
		rts
	:
	pha
	cmp #DATA_sprite0_other_stand
	bne :+
	lda dgd_OTHER_BLINK, Y
	cmp #OTHER_BLINK_POINT
	bcc :+
		pla
		lda #DATA_sprite0_other_blink
		pha
	:
	; draw with facing
	DX_SCREEN
	lda dgd_OTHER_FACE, Y
	bne :+
		lda dog_y, Y
		tay
		pla
		jsr sprite0_add
		rts
	:
		lda dog_y, Y
		tay
		pla
		jsr sprite0_add_flipped
		rts
	;

; DOG_ENDING
stat_dog_ending:

dog_init_ending:
	; store human1 palette
	ldx human1_pal
	lda current_lizard
	cmp #LIZARD_OF_DEATH
	bne :+
		ldx #3
	:
	txa
	ldx human1_next
	sta human1_set, X
	lda human1_hair
	sta human1_het, X
	; trigger ending
	lda #1
	sta ending
	rts
dog_tick_ending = dog_tick_none
dog_draw_ending = dog_draw_none

; DOG_RIVER_EXIT
stat_dog_river_exit:

dog_init_river_exit:
	lda current_lizard
	cmp #LIZARD_OF_SURF
	beq :+
		lda #LIZARD_OF_SURF
		sta current_lizard
		sta next_lizard
		lda #$FF
		sta last_lizard
	:
	lda #>999
	sta lizard_vx+0
	lda #<999
	sta lizard_vx+1
	lda #2
	sta lizard_dismount
	lda #0
	sta lizard_face
	sta lizard_wet
	rts
dog_tick_river_exit:
	lda lizard_dead
	beq :+
		rts
	:
	lda lizard_x+0
	cmp #92
	bcs :+
		lda #2
		sta lizard_dismount
	:
	lda lizard_y+0
	cmp water
	bcc :+
	beq :+
		jsr dog_init_river_exit
		lda water
		sta lizard_y+0
		dec lizard_y+0
		lda #0
		sta lizard_y+1
	:
	rts
dog_draw_river_exit:
	rts

; DOG_BONES
stat_dog_bones:

;dgd_BONES_INIT = dog_data0
;dgd_BONES_FLIP = dog_data1
;dgd_BONES_VX0 = dog_data2
;dgd_BONES_VX1 = dog_data3
;dgd_BONES_VY0 = dog_data4
;dgd_BONES_VY1 = dog_data5
;dgd_BONES_X1  = dog_data6
;dgd_BONES_Y1  = dog_data7

;dog_init_bones ; in dogs_common.s
;dog_tick_bones ; in dogs_common.s
dog_draw_bones:
	lda dgd_BONES_INIT, Y
	beq @skip
	cmp #255
	beq @skip
	DX_SCROLL_EDGE
	lda dgd_BONES_FLIP, Y
	beq :+ ; if dgd(BONES_FLIP)
		lda dog_param, Y
		pha
		;ldx dog_x, Y
		lda dog_y, Y
		tay
		pla
		jsr sprite0_add_flipped
		rts
	: ; else (if ! dgd(BONES_FLIP))
		lda dog_param, Y
		pha
		;ldx dog_x, Y
		lda dog_y, Y
		tay
		pla
		jsr sprite0_add
	;
@skip:
	rts

; DOG_EASY
stat_dog_easy:

dgd_EASY_Y     = dog_data0
dgd_EASY_ANGLE = dog_data1

EASY_SPEED = 5
EASY_WAVE  = 50

dog_init_easy:
	lda easy
	cmp #$FF
	bne :+
		lda #4
		sta dgd_EASY_Y, Y
		rts
	:
		lda dog_y, Y
		sta dgd_EASY_Y, Y
		rts
	;
dog_tick_easy:
	lda easy
	bne :+
		rts
	:
	cmp #$FF
	bne :+
		lda #1
		sta easy
	:
	lda dgd_EASY_Y, Y
	cmp dog_y, Y
	bcs :+
		clc
		adc #1
		sta dgd_EASY_Y, Y
	:
	lda dgd_EASY_ANGLE, Y
	clc
	adc #EASY_SPEED
	sta dgd_EASY_ANGLE, Y
	rts
dog_draw_easy:
	lda easy
	beq	:+
	cmp #$FF
	bne :++
	:
		rts
	:
	lda dgd_EASY_ANGLE, Y
	sta m ; m = angle
	lda dgd_EASY_Y, Y
	sta n ; n = dy
	lda dog_x, Y
	sec
	sbc #16
	sta o ; o = dx - 16
	lda #$80
	sta p ; p = tile
	lda #4
	sta q ; q = counter
	lda #$03
	sta i ; attrib
	; draw EASY
	:
		ldx m
		lda circle4, X
		clc
		adc n
		tay ; Y = dy + circle4[angle]
		ldx o ; X = dx offset by tile
		lda p ; A = tile
		; i = attrib
		jsr sprite_tile_add
		lda o
		clc
		adc #8
		sta o ; dx += 8
		lda m
		clc
		adc #EASY_WAVE
		sta m ; angle += EASY_WAVE
		inc p ; ++tile
		dec q
		bne :-
	rts

; DOG_SPRITE0
stat_dog_sprite0:

dog_init_sprite0:
dog_tick_sprite0:
	rts
dog_draw_sprite0:
	DX_SCROLL_EDGE
	lda dog_param, Y
	pha
	lda dog_y, Y
	tay
	pla
	jmp sprite0_add

stat_dog_sprite2:

dog_init_sprite2:
dog_tick_sprite2:
	rts
dog_draw_sprite2:
	DX_SCROLL_EDGE
	lda dog_param, Y
	pha
	lda dog_y, Y
	tay
	pla
	jmp sprite2_add_banked

; DOG_HINTD
stat_dog_hintd:

dgd_HINT_ON = dog_data0

dog_init_hintd:
	rts
dog_tick_hintd:
	lda #0
	sta dgd_HINT_ON, Y
	; visibility test
	lda lizard_dead
	beq :+
		rts
	:
	lda lizard_power
	bne :+
		rts
	:
	lda current_lizard
	cmp #LIZARD_OF_KNOWLEDGE
	beq :+
		rts
	:
	DOG_BOUND -8,-7,16,17
	jsr lizard_overlap
	beq :+
		rts
	:
	; visible
	lda #1
	sta dgd_HINT_ON, Y
	rts
dog_draw_hintd:
	lda #$FD
	sta t
	lda #$01
	sta i
	;jmp dog_draw_hint_common
dog_draw_hint_common:
	lda dgd_HINT_ON, Y
	bne :+
		rts
	:
	DX_SCROLL
	lda dog_y, Y
	tay
	lda t
	jmp sprite_tile_add

; DOG_HINTU
stat_dog_hintu:

dog_init_hintu = dog_init_hintd
dog_tick_hintu = dog_tick_hintd
dog_draw_hintu:
	lda #$FD
	sta t
	lda #$81
	sta i
	jmp dog_draw_hint_common
	rts

; DOG_HINTL
stat_dog_hintl:

dog_init_hintl = dog_init_hintd
dog_tick_hintl = dog_tick_hintd
dog_draw_hintl:
	lda #$FE
	sta t
	lda #$41
	sta i
	jmp dog_draw_hint_common
	rts

; DOG_HINTR
stat_dog_hintr:

dog_init_hintr = dog_init_hintd
dog_tick_hintr = dog_tick_hintd
dog_draw_hintr:
	lda #$FE
	sta t
	lda #$01
	sta i
	jmp dog_draw_hint_common

; DOG_HINT_PENGUIN
stat_dog_hint_penguin:

dog_hint_penguin_locate:
	tya
	tax
	dex
	lda dog_y, X
	sec
	sbc #32
	sta dog_y, Y
	lda dog_x, X
	sec
	sbc #<4
	sta dog_x, Y
	lda dog_xh, X
	sbc #>4
	sta dog_xh, Y
	rts
dog_init_hint_penguin = dog_hint_penguin_locate
dog_tick_hint_penguin:
	jsr dog_hint_penguin_locate
	jsr dog_tick_hintd
	rts
dog_draw_hint_penguin = dog_draw_hintd

; DOG_BIRD
stat_dog_bird:

dgd_BIRD_FACE  = dog_data0
dgd_BIRD_ANIM  = dog_data1
dgd_BIRD_FRAME = dog_data2

.segment "DATA"
bird_flap:
.byte $91, $92, $93, $92

.segment "CODE"

BIRD_RANGE = 16

dog_init_bird:
	jsr prng
	and #1
	sta dgd_BIRD_FACE, Y
	lda #$90
	sta dgd_BIRD_FRAME, Y
	rts
dog_tick_bird:
	; death/fire
	DOG_BOUND -2,-6,1,-2
	jsr lizard_touch
	beq :+
		lda #DATA_sprite0_bird_skull
		ldx dgd_BIRD_FACE, Y
		jmp bones_convert
	:
	; sitting
	lda dgd_BIRD_FRAME, Y
	cmp #$90
	bne @not_sitting
		DOG_BOUND -BIRD_RANGE,-(BIRD_RANGE+4),BIRD_RANGE-1,(BIRD_RANGE-4)
		jsr lizard_overlap
		bne :+
			rts
		:
		ldx #0
		lda bird_flap, X
		sta dgd_BIRD_FRAME, Y
		lda dog_x, Y
		cmp lizard_x+0
		lda dog_xh, Y
		sbc lizard_xh
		bcs :+
			lda #1
			jmp :++
		:
			lda #0
		:
		sta dgd_BIRD_FACE, Y
	@not_sitting:
	; move horizontal
	lda dgd_BIRD_FACE, Y
	bne @fly_left
	@fly_right:
		lda #2
		jsr move_finish_x
		lda room_scrolling
		beq :+
			DOG_CMP_X 510
			bcs @bird_done
			jmp :++
		:
			DOG_CMP_X 254
			bcs @bird_done
		:
		jmp @fly_end
	@fly_left:
		lda #-2
		jsr move_finish_x
		DOG_CMP_X 5
		bcc @bird_done
	@fly_end:
	; move vertical
	ldx dog_y, Y
	dex
	jsr prng
	and #1
	beq :+
		dex
	:
	txa
	sta dog_y, Y
	cmp #10
	bcc @bird_done
	; animation
	lda dgd_BIRD_ANIM, Y
	clc
	adc #1
	sta dgd_BIRD_ANIM, Y
	lsr
	lsr
	and #3
	tax
	lda bird_flap, X
	sta dgd_BIRD_FRAME, Y
	rts
@bird_done:
	jmp empty_dog
dog_draw_bird:
	lda #-4
	DX_SCROLL_OFFSET
	lda dgd_BIRD_FACE, Y
	lsr
	ror
	ror
	ora #1
	sta i
	; X = dx-4
	lda dgd_BIRD_FRAME, Y
	pha
	lda dog_y, Y
	sec
	sbc #9
	tay
	pla
	jmp sprite_tile_add

; DOG_FROG
stat_dog_frog:

dgd_FROG_FACE = dog_data0
dgd_FROG_VY0  = dog_data1
dgd_FROG_VY1  = dog_data2
dgd_FROG_Y1   = dog_data3
dgd_FROG_ANIM = dog_data4
dgd_FROG_FLY  = dog_data5

FROG_RANGE = 12
FROG_JUMP = -500
FROG_GRAVITY = 60

dog_bound_frog:
	DOG_BOUND -3,-4,2,-1
	rts
dog_range_frog:
	DOG_BOUND -FROG_RANGE,-(FROG_RANGE+1),FROG_RANGE-1,(FROG_RANGE-4)
	rts

dog_init_frog:
	jsr prng
	sta dgd_FROG_ANIM, Y
	jsr prng
	and #1
	sta dgd_FROG_FACE, Y
	rts
dog_tick_frog:
	; remove if offscreen
	DOG_CMP_X 509
	bcs @remove
	DOG_CMP_X 5
	bcc @remove
	lda dog_y, Y
	cmp #240
	bcc @remove_not
	@remove:
		jmp empty_dog
	@remove_not:
	; fraction
	lda current_lizard
	cmp #LIZARD_OF_KNOWLEDGE
	bne @fraction_end
	lda lizard_power
	beq @fraction_end
	lda frogs_fractioned
	cmp #FROG_FRACTION_COUNT
	bne @fraction_end
	jsr dog_range_frog
	jsr lizard_overlap
	beq @fraction_end
		lda #255
		sta frogs_fractioned
		lda #TEXT_FROG
		sta game_message
		lda #1
		sta game_pause
		rts
	@fraction_end:
	; touch of death/fire
	jsr dog_bound_frog
	jsr lizard_touch
	beq @alive
		lda frogs_fractioned
		cmp #FROG_FRACTION_COUNT
		bcs :+
			inc frogs_fractioned
		:
		lda #DATA_sprite0_frog_skull
		ldx dog_type, Y
		cpx #DOG_GROG
		bne :+
			lda #DATA_sprite0_grog_skull
		:
		ldx dgd_FROG_FACE, Y ; flip
		jmp bones_convert
	@alive:
	; not flying
	lda dgd_FROG_FLY, Y
	bne @fly
		; animate croak
		lda dgd_FROG_ANIM, Y
		bne :+
			; random croak time
			jsr prng2
			sta dgd_FROG_ANIM, Y
			; random chance to jump
			jsr prng3
			and #15
			beq @hop
			jmp :++
		:
			sec
			sbc #1
			sta dgd_FROG_ANIM, Y
		:
		; scared by lizard
		jsr dog_range_frog
		jsr lizard_overlap
		bne :+
			rts
		:
		; begin hop
		@hop:
		lda #1
		sta dgd_FROG_FLY, Y
		lda dog_x, Y
		cmp lizard_x+0
		lda dog_xh, Y
		sbc lizard_xh
		bcc :+
			lda #0
			jmp :++
		:
			lda #1
		:
		sta dgd_FROG_FACE, Y
		@frog_jump:
			jsr prng
			sta i
			lda #<FROG_JUMP
			sec
			sbc i
			sta dgd_FROG_VY1, Y
			lda #>FROG_JUMP
			sbc #0
			sta dgd_FROG_VY0, Y
			rts
		;
	; flying
	@fly:
	; move vertically
	lda dgd_FROG_Y1, Y
	clc
	adc dgd_FROG_VY1, Y
	sta dgd_FROG_Y1, Y
	lda dog_y, Y
	sta m ; m = old_dgy
	adc dgd_FROG_VY0, Y
	sec
	sbc dog_y, Y
	; note that dog_y is not updated here, this gets done by dy after move_dog
	sta v ; v = dy
	; gravity
	lda dgd_FROG_VY1, Y
	clc
	adc #<FROG_GRAVITY
	sta dgd_FROG_VY1, Y
	lda dgd_FROG_VY0, Y
	adc #>FROG_GRAVITY
	; cap to <6 pixels per frame
	bmi :+ ; is negative, no problem
	cmp #6
	bcc :+ ; is < 6, no problem
		lda #5 ; clamp to [-128,5]
	:
	sta dgd_FROG_VY0, Y
	; move horizontally
	lda dgd_FROG_FACE, Y
	bne :+
		lda #1
		jmp :++
	:
		lda #-1
	:
	sta u ; u = dx
	jsr dog_bound_frog
	jsr move_dog
	jsr move_finish
	; check for collision with ground
	lda w
	and #4
	beq @land_end
		lda #0
		sta dgd_FROG_Y1, Y
		jsr prng
		and #1
		sta dgd_FROG_FACE, Y
		jsr prng
		and #1
		bne :+
			lda #0
			sta dgd_FROG_FLY, Y
			jmp :++
		:
			jsr @frog_jump
		:
	@land_end:
	; splash
	lda m
	cmp water
	bcc @splash_old_above
	@splash_old_below:
		lda dog_y, Y
		cmp water
		bcs @splash_end
	@splash_do:
		PLAY_SOUND_SCROLL SOUND_SPLASH_SMALL
		lda dog_type + SPLASHER_SLOT
		cmp #DOG_SPLASHER
		bne @splash_end
		lda dog_x, Y
		sec
		sbc #4
		sta dog_x + SPLASHER_SLOT
		lda dog_xh, Y
		sbc #0
		sta dog_xh + SPLASHER_SLOT
		lda water
		sec
		sbc #9
		sta dog_y + SPLASHER_SLOT
		lda #0
		sta dog_data0 + SPLASHER_SLOT
		jmp @splash_end
	@splash_old_above:
		lda dog_y, Y
		cmp water
		bcs @splash_do
	@splash_end:
	rts
dog_draw_frog:
	lda #2
	sta i ; attribute or
	lda #$80
	sta l ; tile or
	jmp dog_draw_frog_common

dog_draw_frog_common:
	lda i
	sta s ; temp (clobbered by DX_SCROLL_OFFSET)
	lda #-4
	DX_SCROLL_OFFSET
	lda s
	sta i ; attrib
	lda dgd_FROG_FACE, Y
	lsr
	ror
	ror
	ora i
	sta i ; attribute
	lda dgd_FROG_FLY, Y
	pha
	; X = dx-4
	lda dog_y, Y
	sec
	sbc #9
	sty j ; temporarily store Y
	tay
	pla
	beq :+
		lda #$02
		ora l
		jmp sprite_tile_add
	:
	sty k ; temporarily store dgy-9
	ldy j
	lda dgd_FROG_ANIM, Y
	cmp #6
	bcc :+
		lda #$00
		jmp :++
	:
		lda #$01
	:
	ldy k
	ora l
	jmp sprite_tile_add

; DOG_GROG
stat_dog_grog:

dog_init_grog = dog_init_frog
dog_tick_grog:
	jsr dog_tick_frog
	ldy dog_now
	jsr dog_bound_frog
	jsr lizard_overlap_no_stone
	beq :+
		jsr lizard_die
	:
	rts
dog_draw_grog:
	tya
	and #1
	ora #2
	sta i ; attribute or
	lda #$70
	sta l ; tile or
	jmp dog_draw_frog_common

; DOG_PANDA
stat_dog_panda:

dgd_PANDA_MODE = dog_data0
dgd_PANDA_TIME = dog_data1
dgd_PANDA_TALK = dog_data2

dog_init_panda:
	DOG_BLOCKER -8,-26,7,-19
	rts
dog_tick_panda:
	; touch of fire/death
	DOG_BOUND -14,-20,13,-1
	jsr lizard_touch
	beq @no_touch
		lda current_lizard
		cmp #LIZARD_OF_DEATH
		bne @no_death
			jsr empty_blocker
			lda #DATA_sprite0_panda_skull
			ldx dgd_PANDA_MODE, Y
			cmp #2
			beq :+
				ldx #0
				jmp :++
			:
				ldx #1
			:
			jmp bones_convert
		@no_death:
		cmp #LIZARD_OF_HEAT
		bne @no_fire
			lda dog_type, Y
			cmp #DOG_PANDA_FIRE
			beq @no_fire
			lda #DOG_PANDA_FIRE
			sta dog_type, Y
			jmp dog_tick_panda_fire
		@no_fire:
	@no_touch:
	; sharp claws
	@claws:
		lda dgd_PANDA_MODE, Y
		cmp #1
		bne @not_left
			DOG_BOUND -15,-26,-10,-8
			jsr lizard_overlap_no_stone
			beq @no_claws
			jsr lizard_die
			jmp @no_claws
		@not_left:
		cmp #2
		bne @no_claws
			DOG_BOUND 9,-26,14,-8
			jsr lizard_overlap_no_stone
			beq @no_claws
			jsr lizard_die
			;jmp @no_claws
		;
	@no_claws:
	; talking
	lda dgd_PANDA_TALK, Y
	bne @no_talk
	lda lizard_dead
	bne @no_talk
	lda lizard_power
	beq @no_talk
	lda current_lizard
	cmp #LIZARD_OF_KNOWLEDGE
	bne @no_talk
	DOG_BOUND -64,-64,64,32
	jsr lizard_overlap
	beq @no_talk
		lda #TEXT_PANDA_TALK
		sta game_message
		lda dgd_PANDA_MODE, Y
		cmp #3
		bne :+
			lda #TEXT_PANDA_HEAD
			sta game_message
			jmp :++
		:
			lda #1
			sta dgd_PANDA_TALK, Y
		:
		lda #1
		sta game_pause
		rts
	@no_talk:
	; head step
	DOG_BOUND -8,-27,7,-27
	jsr lizard_overlap
	beq @no_step
		lda dgd_PANDA_MODE, Y
		cmp #3
		beq :+
			PLAY_SOUND SOUND_PANDA_SIGH
			lda #3
			sta dgd_PANDA_MODE, Y
			lda #0
			sta dgd_PANDA_TALK, Y
		:
		lda #15
		sta dgd_PANDA_TIME, Y
		rts
	@no_step:
	lda dgd_PANDA_TIME, Y
	cmp #0
	beq :+
		jmp @no_time
	:
		DOG_BOUND -30,-50,-15,-1
		jsr lizard_overlap
		beq @not_left_swipe
			lda dgd_PANDA_MODE, Y
			cmp #1
			beq :+
				lda #1
				sta dgd_PANDA_MODE, Y
				PLAY_SOUND SOUND_PANDA_SWIPE
			:
			lda #15
			sta dgd_PANDA_TIME, Y
			rts
		@not_left_swipe:
		DOG_BOUND 14,-50,29,-1
		jsr lizard_overlap
		beq @not_right_swipe
			lda dgd_PANDA_MODE, Y
			cmp #2
			beq :+
				lda #2
				sta dgd_PANDA_MODE, Y
				PLAY_SOUND SOUND_PANDA_SWIPE
			:
			lda #15
			sta dgd_PANDA_TIME, Y
			rts
		@not_right_swipe:
		lda #0
		sta dgd_PANDA_MODE, Y
		rts
	@no_time:
		sec
		sbc #1
		sta dgd_PANDA_TIME, Y
		rts
	;
dog_draw_panda:
	DX_SCROLL_EDGE
	lda dgd_PANDA_MODE, Y
	pha
	;ldx dog_x, Y
	lda dog_y, Y
	tay
	pla
	beq @default
	cmp #1
	bne :+
		lda #DATA_sprite0_panda_mad
		jmp sprite0_add
	:
	cmp #2
	bne :+
		lda #DATA_sprite0_panda_mad
		jmp sprite0_add_flipped
	:
	cmp #3
	bne :+
		lda #DATA_sprite0_panda_step
		jmp sprite0_add
	:
	@default:
		lda #DATA_sprite0_panda
		jmp sprite0_add
	;

; DOG_GOAT
stat_dog_goat:

dgd_GOAT_MODE = dog_data0
dgd_GOAT_FLIP = dog_data1
dgd_GOAT_GUAT = dog_data2
dgd_GOAT_ANIM = dog_data3
dgd_GOAT_TIME = dog_data4
dgd_GOAT_FACE = dog_data5
dgd_GOAT_LOOK = dog_data6
;dgd_FIRE_PAL  = dog_data10
;dgd_FIRE_ANIM = dog_data11

GUAT_SHIFT = DATA_sprite0_guat - DATA_sprite0_goat

dog_bound_goat:
	DOG_BOUND -8,-8,7,-1
	rts

dog_init_goat:
	jsr prng3
	sta dgd_GOAT_TIME, Y
	jsr prng
	and #1
	sta dgd_GOAT_FLIP, Y
	rts
dog_tick_goat:
	; touched by player
	jsr dog_bound_goat
	jsr lizard_touch
	beq @no_touch
		lda current_lizard
		cmp #LIZARD_OF_DEATH
		bne @no_death
			lda #DATA_sprite0_goat_skull
			ldx dgd_GOAT_GUAT, Y
			beq :+
				clc
				adc #GUAT_SHIFT
			:
			ldx dgd_GOAT_FLIP, Y
			jmp bones_convert
		@no_death:
		cmp #LIZARD_OF_HEAT
		bne @no_fire
			; if standing, force a jump immediately
			lda dgd_GOAT_MODE, Y
			bne :+
				lda dgd_GOAT_TIME, Y
				sta dgd_GOAT_ANIM, Y
			:
		@no_fire:
	@no_touch:
	; tick animate
	ldx dog_now
	inc dgd_GOAT_ANIM, X
	; mode
	lda dgd_GOAT_MODE, Y
	bne @mode_1
		; 0 = stand
		; animate face
		lda dgd_GOAT_LOOK, Y
		bne @no_look
			jsr prng
			and #3
			cmp #3
			bne :+
				jsr prng
				and #1
			:
			sta dgd_GOAT_FACE, Y
			tax
			jsr prng3
			and #127
			cpx #2
			bne :+
				and #15
			:
			clc
			adc #5
			sta dgd_GOAT_LOOK, Y
			jmp @try_jump
		@no_look:
			sec
			sbc #1
			sta dgd_GOAT_LOOK, Y
		;
		@try_jump:
		lda dgd_GOAT_ANIM, Y
		cmp dgd_GOAT_TIME, Y
		bcc @try_jump_end
			lda #0
			sta dgd_GOAT_ANIM, Y
			lda #1
			sta dgd_GOAT_MODE, Y
			PLAY_SOUND_SCROLL SOUND_GOAT_JUMP
		@try_jump_end:
		jmp @mode_end
	@mode_1:
	cmp #1
	bne @mode_2
		lda dgd_GOAT_ANIM, Y
		cmp #32
		jcc @mode_end
		ldx dog_now
		lda dgd_GOAT_GUAT, Y
		beq :+
			inc dog_y, X
			jmp :++
		:
			dec dog_y, X
		:
		lda #2
		sta dgd_GOAT_MODE, Y
		jmp @fly_fallthrough
	@mode_2:
	cmp #2
	bne @mode_3
	@fly_fallthrough:
		DOG_POINT_X 0
		lda dgd_GOAT_GUAT, Y
		bne @inverted
			lda dog_y, Y
			sec
			sbc #3
			sta dog_y, Y
			sec
			sbc #8
			tay
			jsr collide_tile_up ; shift result in A/j
			beq :+
				ldy dog_now
				clc
				adc dog_y, Y
				sta dog_y, Y
				jmp @do_splat
			:
			jmp @mode_end
		;
		@inverted:
			lda dog_y, Y
			clc
			adc #3
			sta dog_y, Y
			tay
			dey
			jsr collide_tile_down ; shift result in A/j
			beq :+
				ldy dog_now
				lda dog_y, Y
				sec
				sbc j
				sta dog_y, Y
				jmp @do_splat
			:
			jmp @mode_end
		;
		@do_splat:
			lda #0
			sta dgd_GOAT_ANIM, Y
			lda #3
			sta dgd_GOAT_MODE, Y
			PLAY_SOUND_SCROLL SOUND_GOAT_SPLAT
			jmp @mode_end
	@mode_3:
	cmp #3
	bne @mode_end
		lda dgd_GOAT_ANIM, Y
		cmp #4
		bne :+
			lda dgd_GOAT_GUAT, Y
			eor #1
			sta dgd_GOAT_GUAT, Y
			jmp @mode_end
		:
		cmp #12
		bcc @mode_end
			lda #0
			sta dgd_GOAT_ANIM, Y
			sta dgd_GOAT_MODE, Y
			jsr prng3
			and #127
			clc
			adc #8
			sta dgd_GOAT_TIME, Y
			;jmp @mode_end
		;
	@mode_end:
	ldy dog_now
	; kill lizard
	jsr dog_bound_goat
	jsr lizard_overlap_no_stone
	beq :+
		jsr lizard_die
	:
	rts
dog_draw_goat:
	DX_SCROLL_EDGE
	stx l ; temporarily store X
	lda #DATA_sprite0_goat
	sta i
	lda dgd_GOAT_MODE, Y
	bne :+
		lda dgd_GOAT_FACE, Y
		clc
		adc i
		sta i
		jmp @draw
	:
	cmp #1
	bne :+
		lda #DATA_sprite0_goat_crouch
		sta i
		jmp @draw
	:
	cmp #2
	bne :+
		jsr prng
		and #1
		clc
		adc #DATA_sprite0_goat_fly0
		sta i
		jmp @draw
	:
	cmp #3
	bne @draw
		lda #DATA_sprite0_goat_splat
		ldx dgd_GOAT_ANIM, Y
		cpx #8
		bcc :+
			lda #DATA_sprite0_goat_crouch
		:
		sta i
		;jmp @draw
	;
@draw:
	lda dgd_GOAT_GUAT, Y
	beq :+
		lda i
		clc
		adc #GUAT_SHIFT
		sta i
	:
	lda dgd_GOAT_FLIP, Y
	php
	;ldx dog_x, Y
	ldx l
	lda dog_y, Y
	tay
	lda i
	plp
	beq :+
		jmp sprite0_add_flipped
	:
		jmp sprite0_add
	;

; DOG_DOG
stat_dog_dog:

dgd_DDOG_ANIM = dog_data0
dgd_DDOG_TIME = dog_data1
dgd_DDOG_MODE = dog_data2
dgd_DDOG_FLIP = dog_data3
dgd_DDOG_Y1   = dog_data4
dgd_DDOG_VY0  = dog_data5
dgd_DDOG_VY1  = dog_data6
dgd_DDOG_LAND = dog_data7
;dgd_FIRE_PAL  = dog_data10
;dgd_FIRE_ANIM = dog_data11

DDOG_JUMP = -940
DDOG_GRAV = 60

dog_dog_mode:
	sta dgd_DDOG_MODE, Y
	lda #0
	sta dgd_DDOG_ANIM, Y
	jsr prng3
	and #127
	clc
	adc #64
	sta dgd_DDOG_TIME, Y
	rts
dog_dog_patrol:
	lda dog_y, Y
	sbc lizard_y+0
	cmp #10
	bcc :+
	cmp #(255-10)
	bcs :+
		rts
	:
	lda dgd_DDOG_FLIP, Y
	bne :+
		lda lizard_x+0
		cmp dog_x, Y
		lda lizard_xh
		sbc dog_xh, Y
		bcc @patrol_run
		rts
	:
		lda dog_x, Y
		cmp lizard_x+0
		lda dog_xh, Y
		sbc lizard_xh
		bcc @patrol_run
		rts
	@patrol_run:
	lda #4
	jmp dog_dog_mode
	;
dog_dog_leap:
	jsr prng3
	and #7
	beq :+
		rts
	:
	lda dgd_DDOG_FLIP, Y
	jne @leap_right
	@leap_left:
		lda lizard_x+0
		cmp dog_x, Y
		lda lizard_xh
		sbc dog_xh, Y
		jcs @leap_dont
		DOG_CMP_X 80
		jcc @leap_dont
		DOG_POINT -72, 0
		jsr collide_tile
		jeq @leap_dont
		ldy dog_now
		DOG_POINT -56, 0
		jsr collide_tile
		jeq @leap_dont
		ldy dog_now
		DOG_POINT -72, -1
		jsr collide_tile
		bne @leap_dont
		ldy dog_now
		DOG_BOUND -78,-8,-70,-1
		jsr lizard_overlap
		jne @leap_do
		DOG_BOUND -60,-40,-16,-16
		jsr lizard_overlap
		jne @leap_do
		rts
	@leap_dont:
		ldy dog_now
		rts
	@leap_right:
		lda dog_x, Y
		cmp lizard_x+0
		lda dog_xh, Y
		sbc lizard_xh
		bcs @leap_dont
		DOG_CMP_X (511-80)
		bcs @leap_dont
		DOG_POINT 56, 0
		jsr collide_tile
		beq @leap_dont
		ldy dog_now
		DOG_POINT 72, 0
		jsr collide_tile
		beq @leap_dont
		ldy dog_now
		DOG_POINT 72, -1
		jsr collide_tile
		bne @leap_dont
		ldy dog_now
		DOG_BOUND 69,-8,77,-1
		jsr lizard_overlap
		bne @leap_do
		DOG_BOUND 15,-40,59,-16
		jsr lizard_overlap
		bne @leap_do
		rts ;jmp @leap_dont
	@leap_do:
	lda dog_y, Y
	sta dgd_DDOG_LAND, Y
	lda #0
	sta dgd_DDOG_Y1, Y
	lda #<DDOG_JUMP
	sta dgd_DDOG_VY1, Y
	lda #>DDOG_JUMP
	sta dgd_DDOG_VY0, Y
	lda #5
	jsr dog_dog_mode
	PLAY_SOUND SOUND_BARK
	rts
dog_dog_collide_left: ; collide_tile(dgx-10,dgy-8)
	DOG_POINT_X -10
dog_dog_collide_common:
	DOG_POINT_Y -8
	jsr collide_tile
	php
	ldy dog_now
	plp
	rts
dog_dog_collide_right: ; collide_tile(dgx+9,dgy-8)
	DOG_POINT_X 9
	jmp dog_dog_collide_common
	;DOG_POINT_Y -8
	;jsr collide_tile
	;php
	;ldy dog_now
	;plp
	;rts
dog_dog_move:
	lda dgd_DDOG_MODE, Y
	cmp #2
	bne :+
		rts ; turning
	:
	lda dgd_DDOG_FLIP, Y
	bne @move_right
	@move_left:
		jsr dog_dog_collide_left
		bne @turn
		DOG_POINT -11,0
		jsr collide_tile
		beq @turn
		ldy dog_now
		DOG_CMP_X 18
		bcc @turn
		; not turning
		lda #-1
		jmp move_finish_x
		;rts
	@move_right:
		jsr dog_dog_collide_right
		bne @turn
		DOG_POINT 10,0
		jsr collide_tile
		beq @turn
		ldy dog_now
		DOG_CMP_X 495
		bcs @turn
		; not turning
		lda #1
		jmp move_finish_x
		;rts
	@turn:
		ldy dog_now
		lda #2
		jmp dog_dog_mode
	;
dog_dog_bound_body:
	lda dgd_DDOG_MODE, Y
	cmp #2
	bcs :+
		DOG_BOUND -6,-10,5,-1
		rts
	:
		DOG_BOUND -8,-11,7,-4
		rts
	;
dog_dog_bound_head:
	lda dgd_DDOG_MODE, Y
	beq @bound_sit
	cmp #3
	jcc @bound_default
	cmp #5
	beq @bound_leap
	cmp #6
	jcs @bound_default
@bound_34:
	DOG_BOUND -16,-12,-9,-11
	jmp @bound_finish
@bound_leap:
	DOG_BOUND -16,-10,-9,-9
	jmp @bound_finish
@bound_sit:
	DOG_BOUND -10,-12,-3,-11
	jmp @bound_finish
@bound_default:
	DOG_BOUND 0,-8,0,-8
	;jmp @bound_finish
@bound_finish:
	lda dgd_DDOG_FLIP, Y
	beq :+
		; flip X0 and X1 (ih:i,ik:k)
		; in: X0 = dog_x + bound_x0
		; in: X1 = dog_x + bound_x1
		;
		; out: X0 = dog_x - bound_x1 - 1
		;      X1 = dog_x - bound_x0 - 1
		;
		lda dog_x, Y
		sec
		sbc k
		sta jh ; temp result in jh
		lda dog_xh, Y
		sbc kh
		sta lh ; temp result in lh
		; lh:jh = - bound_x1
		lda dog_x, Y
		sec
		sbc i
		sta k
		lda dog_xh, Y
		sbc ih
		sta kh
		; kh:k = - bound_x0
		lda k
		sec
		sbc #<1
		sta k
		lda kh
		sbc #>1
		sta kh
		; kh:k = - bound_x0 - 1
		lda jh
		sec
		sbc #<1
		sta i
		lda lh
		sbc #>1
		sta ih
		; ih:i = - bound_x1 - 1
		lda k
		clc
		adc dog_x, Y
		sta k
		lda kh
		adc dog_xh, Y
		sta kh
		; kh:k = dog_x - bound_x0 - 1 (finished)
		lda i
		clc
		adc dog_x, Y
		sta i
		lda ih
		adc dog_xh, Y
		sta ih
		; ih:i = dog_x - bound_x1 - 1 (finished)
	:
	rts

dog_init_dog:
	jsr prng
	and #1
	sta dgd_DDOG_FLIP, Y
	lda #16
	sta dgd_DDOG_TIME, Y
	rts
dog_tick_dog:
	; touched by player
	jsr dog_dog_bound_body
	jsr lizard_touch
	bne @touched
	jsr dog_dog_bound_head
	jsr lizard_touch
	beq @touched_end
	@touched:
		lda current_lizard
		cmp #LIZARD_OF_DEATH
		bne :+
			ldx dgd_DDOG_FLIP, Y
			lda #DATA_sprite0_dog_skull
			jmp bones_convert
		:
		cmp #LIZARD_OF_HEAT
		bne @heat_end
			lda dgd_DDOG_MODE, Y
			cmp #5
			beq @heat_end
			cmp #1
			beq @lick_continue
			@lick_force:
				lda #1
				jsr dog_dog_mode
				jmp @lick_finish
			@lick_continue:
				lda dgd_DDOG_ANIM, Y
				and #31
				sta dgd_DDOG_ANIM, Y
			@lick_finish:
				; last at least 2 seconds
				lda dgd_DDOG_TIME, Y
				ora #128
				sta dgd_DDOG_TIME, Y
			;
		@heat_end:
	@touched_end:
	; increment animation
	ldx dgd_DDOG_ANIM, Y
	inx
	txa
	sta dgd_DDOG_ANIM, Y
	; note: X = ANIM, used in mode behaviour
	; various modes
	lda dgd_DDOG_MODE, Y
	bne @cmp_1
		; sit
		txa
		cmp dgd_DDOG_TIME, Y
		bcc @sit_continue
			jsr prng3
			and #7
			bne :+
				lda #1 ; lick
				jmp :++
			:
				lda #3 ; walk
			:
			jsr dog_dog_mode
		@sit_continue:
		jsr dog_dog_patrol
		jmp @mode_end
	@cmp_1:
	cmp #1
	bne @cmp_2
		; lick
		txa
		cmp dgd_DDOG_TIME, Y
		bcc :+
			lda #0 ; sit
			jsr dog_dog_mode
		:
		jmp @mode_end
	@cmp_2:
	cmp #2
	bne @cmp_3
		; turn
		cpx #8
		bne :+
			lda dgd_DDOG_FLIP, Y
			eor #1
			sta dgd_DDOG_FLIP, Y
		:
		cpx #16
		bcc :+
			lda #3 ; walk
			jsr dog_dog_mode
		:
		jmp @mode_end
	@cmp_3:
	cmp #3
	bne @cmp_4
		; walk
		txa
		pha ; store copy of ANIM
		and #1
		bne :+ ; only move every second frame
			jsr dog_dog_move
		:
		pla ; restore ANIM to A
		cmp dgd_DDOG_TIME, Y
		bcc @walk_continue
			;jsr prng2
			;and #3
			;bne :+
			;	lda #2 ; turn
			;	jmp :++
			;:
				lda #0 ; sit
			;:
			jsr dog_dog_mode
		@walk_continue:
		jsr dog_dog_patrol
		jmp @mode_end
	@cmp_4:
	cmp #4
	bne @cmp_5
		; run
		cpx #5
		bcc :+
			ldx dog_now
			inc dgd_DDOG_TIME, X
			lda #0
			sta dgd_DDOG_ANIM, Y
		:
		jsr dog_dog_move
		jsr dog_dog_move
		jsr dog_dog_leap
		jmp @mode_end
	@cmp_5:
	cmp #5
	bne @mode_end
		; leap
		; move horizontal
		lda dgd_DDOG_FLIP, Y
		bne @leap_right
		@leap_left:
			jsr dog_dog_collide_left
			bne @leap_horizontal_end
			lda #-2
			jsr move_finish_x
			jmp @leap_horizontal_end
		@leap_right:
			jsr dog_dog_collide_right
			bne @leap_horizontal_end
			lda #2
			jsr move_finish_x
			;jmp @leap_horizontal_end
		@leap_horizontal_end:
		; move vertical
		; y += vy
		lda dgd_DDOG_Y1, Y
		clc
		adc dgd_DDOG_VY1, Y
		sta dgd_DDOG_Y1, Y
		lda dog_y, Y
		adc dgd_DDOG_VY0, Y
		sta dog_y, Y
		; vy += gravity
		lda dgd_DDOG_VY1, Y
		clc
		adc #<DDOG_GRAV
		sta dgd_DDOG_VY1, Y
		lda dgd_DDOG_VY0, Y
		adc #>DDOG_GRAV
		sta dgd_DDOG_VY0, Y
		; landing
		lda dog_y, Y
		cmp dgd_DDOG_LAND, Y
		bcc :+
			lda dgd_DDOG_LAND, Y
			sta dog_y, Y
			lda #4 ; run
			jsr dog_dog_mode
		:
		;jmp @mode_end
	@mode_end:
	; hurt player
	jsr dog_dog_bound_body
	jsr lizard_overlap_no_stone
	bne @hurt
	jsr dog_dog_bound_head
	jsr lizard_overlap_no_stone
	beq @hurt_end
	@hurt:
		jsr lizard_die
	@hurt_end:
	rts
dog_draw_dog:
	DX_SCROLL_EDGE
	stx l ; temporary
	lda #DATA_sprite0_dog ; sit
	sta j ; temp variable "s" for sprite
	lda dgd_DDOG_MODE, Y
	beq @draw
	cmp #1
	bne :+
		; lick
		lda dgd_DDOG_ANIM, Y
		lsr
		lsr
		lsr
		lsr
		and #1
		clc
		adc #DATA_sprite0_dog_lick0
		sta j
		jmp @draw
	:
	cmp #2
	bne :++
		; turn
		lda dgd_DDOG_ANIM, Y
		cmp #8
		bcs :+
			lda #DATA_sprite0_dog_turn
			sta j
		:
		jmp @draw
	:
	cmp #3
	bne :+
		; walk
		lda dgd_DDOG_ANIM, Y
		lsr
		lsr
		lsr
		and #3
		clc
		adc #DATA_sprite0_dog_walk0
		sta j
		jmp @draw
	:
	cmp #4
	bne :+
		; run
		lda dgd_DDOG_TIME, Y
		and #3
		clc
		adc #DATA_sprite0_dog_run0
		sta j
		jmp @draw
	:
	cmp #5
	bne :+
		; leap
		lda #DATA_sprite0_dog_leap
		sta j
		;jmp @draw
	:
@draw:
	lda dgd_DDOG_FLIP, Y
	php
	;ldx dog_x, Y
	ldx l
	lda dog_y, Y
	tay
	lda j
	plp
	beq :+
		jmp sprite0_add_flipped
	:
		jmp sprite0_add
	;

; DOG_WOLF
stat_dog_wolf:
dog_init_wolf = dog_init_dog;
dog_tick_wolf = dog_tick_dog;
dog_draw_wolf = dog_draw_dog;

; DOG_OWL
stat_dog_owl:

dgd_OWL_ANIM = dog_data0
dgd_OWL_MODE = dog_data1
dgd_OWL_Y1   = dog_data2
dgd_OWL_VY0  = dog_data3
dgd_OWL_VY1  = dog_data4
dgd_OWL_DIR  = dog_data5
dgd_OWL_FLAP = dog_data6
;dgd_FIRE_PAL  = dog_data10
;dgd_FIRE_ANIM = dog_data11

OWL_ACCEL = 60
OWL_HANG_TIME = 20
OWL_DIVE_TIME = 68
OWL_FLAP_TIME = 8
OWL_PEAK_TIME = OWL_HANG_TIME / 4
OWL_DIVE_A = OWL_DIVE_TIME / 4
OWL_DIVE_B = 3 * OWL_DIVE_A

dog_blocker_owl:
	lda dog_type, Y
	cmp #DOG_OWL_FIRE
	bne :+
		jmp empty_blocker
		;rts
	:
	DOG_BLOCKER -5,-14,3,-6
	rts

dog_init_owl:
	lda dog_param, Y
	and #1
	sta dog_param, Y
	lda #OWL_PEAK_TIME
	sta dgd_OWL_ANIM, Y
	jmp dog_blocker_owl
	;rts
dog_tick_owl:
	DOG_BOUND -6,-12,4,-1
	jsr lizard_touch_death
	beq @no_touch
		jsr empty_blocker
		lda #DATA_sprite0_owl_skull
		ldx #0
		jmp bones_convert
	@no_touch:
	; temporary storage
	lda #0
	sta u ; u = dx
	sta v ; v = dy
	lda dgd_OWL_DIR, Y
	sta q ; q = old_dir
	lda dgd_OWL_ANIM, Y
	sta s ; s = old_anim
	lda dgd_OWL_MODE, Y
	sta t ; t = old_mode
	; behave
	; Z contains 0 test for MODE
	jne @mode_dive
	@mode_hang:
		ldx dgd_OWL_ANIM, Y
		cpx #(OWL_HANG_TIME/2)
		bcs :+
			lda #0
			jmp :++
		:
			lda #1
		:
		sta dgd_OWL_DIR, Y
		cpx #(OWL_HANG_TIME/2)
		bne @sound_end
			PLAY_SOUND_SCROLL SOUND_OWL_FLAP
			ldx dgd_OWL_ANIM, Y
		@sound_end:
		inx
		txa
		sta dgd_OWL_ANIM, Y
		cmp #OWL_PEAK_TIME
		jne @no_dive
			jsr prng2
			and #7
			sta r ; temp
			lda dog_param, Y
			bne @bait_right
			@bait_left:
				DOG_BOUND -64,0,8,72
				jsr lizard_overlap_no_stone
				beq @bait_end
			@baited:
				lda #6
				ora r
				sta r
				jmp @bait_end
			@bait_right:
				DOG_BOUND -8,0,64,72
				jsr lizard_overlap_no_stone
				bne @baited
			@bait_end:
			lda r
			cmp #7
			bne :+
				lda #1
				sta dgd_OWL_MODE, Y
				lda #0
				sta dgd_OWL_ANIM, Y
				sta dgd_OWL_FLAP, Y
			:
			jmp @mode_done
		@no_dive:
		cmp #OWL_HANG_TIME
		bcc @mode_done
			lda #0
			sta dgd_OWL_ANIM, Y
			jmp @mode_done
		;
		;jmp @mode_done
	@mode_dive:
		lda dgd_OWL_ANIM, Y
		cmp #OWL_DIVE_A
		bcc @dive_dir_0
		cmp #OWL_DIVE_B
		bcs @dive_dir_0
		jmp @dive_dir_1
		@dive_dir_0:
			lda #0
			sta dgd_OWL_DIR, Y
			jmp @dive_dir_end
		@dive_dir_1:
			lda dgd_OWL_FLAP, Y
			and #(OWL_FLAP_TIME-1)
			bne @flap_sound_end
				PLAY_SOUND_SCROLL SOUND_OWL_FLAP
			@flap_sound_end:
			lda #1
			sta dgd_OWL_DIR, Y
			;jmp @dive_dir_end
		@dive_dir_end:
		tya
		tax
		inc dgd_OWL_FLAP, X
		; movement
		lda dog_param, Y
		bne :+
			lda #-1
			sta u ; dx = -1
			jmp :++
		:
			lda #1
			sta u ; dx = 1
		:
		; tick animate
		ldx dgd_OWL_ANIM, Y
		inx
		txa
		sta dgd_OWL_ANIM, Y
		cmp #OWL_DIVE_TIME
		bcc :+
			lda #OWL_PEAK_TIME
			sta dgd_OWL_ANIM, Y
			lda #0
			sta dgd_OWL_MODE, Y
			lda dog_param, Y
			eor #1
			sta dog_param, Y
		:
		;jmp @mode_done
	@mode_done:
	; calculate vertical movement
	lda dgd_OWL_Y1, Y
	clc
	adc dgd_OWL_VY1, Y
	sta n ; n = new y1
	lda dog_y, Y
	adc dgd_OWL_VY0, Y
	sta m ; m = new y0
	sec
	sbc dog_y, Y
	sta v ; v = dy
	DOG_BOUND -5,-15,3,-15
	jsr lizard_overlap
	beq :+
		lda lizard_x+0
		clc
		adc u
		sta lizard_x+0
		lda lizard_y+0
		clc
		adc v
		sta lizard_y+0
		jsr do_scroll
		lda u
		jsr move_finish_x ; dog_x/xh += u
		lda m
		sta dog_y, Y
		jmp @allow_motion
	:
	lda dog_y, Y
	sta p ; p = old_dgy
	lda dog_x, Y
	sta o ; o = old_dgx
	lda dog_xh, Y
	sta jh ; jh = old_dgxh
	; test new bounds for lizard blockage
	lda u
	jsr move_finish_x ; dog_x/xh += u
	lda m
	sta dog_y, Y
	DOG_BOUND -5, -14, 3, -6
	jsr lizard_overlap
	beq @allow_motion
	@disallow_motion:
		lda o
		sta dog_x, Y
		lda jh
		sta dog_xh, Y
		lda p
		sta dog_y, Y
		lda q
		sta dgd_OWL_DIR, Y
		lda s
		sta dgd_OWL_ANIM, Y
		lda t
		sta dgd_OWL_MODE, Y
		jmp @motion_end
	@allow_motion:
		; dog_x/dog_y already updated
		lda n
		sta dgd_OWL_Y1, Y
		lda dgd_OWL_DIR, Y
		bne :+
			lda dgd_OWL_VY1, Y
			clc
			adc #<OWL_ACCEL
			sta dgd_OWL_VY1, Y
			lda dgd_OWL_VY0, Y
			adc #>OWL_ACCEL
			sta dgd_OWL_VY0, Y
			jmp :++
		:
			lda dgd_OWL_VY1, Y
			sec
			sbc #<OWL_ACCEL
			sta dgd_OWL_VY1, Y
			lda dgd_OWL_VY0, Y
			sbc #>OWL_ACCEL
			sta dgd_OWL_VY0, Y
		:
	@motion_end:
	jsr dog_blocker_owl
	; overlap
	DOG_BOUND -6,-10,4,-1
	jsr lizard_overlap_no_stone
	beq :+
		jsr lizard_die
	:
	rts
dog_draw_owl:
	DX_SCROLL_EDGE
	lda #DATA_sprite0_owl
	pha
	lda dgd_OWL_DIR, Y
	beq @draw
	lda dgd_OWL_MODE, Y
	beq :+
	lda dgd_OWL_FLAP, Y
	and #(OWL_FLAP_TIME-1)
	cmp #(OWL_FLAP_TIME/2)
	bcc :+
	jmp @draw
	:
		pla
		lda #DATA_sprite0_owl_flap
		pha
	;
@draw:
	; X = dx
	lda dog_y, Y
	tay
	pla
	jmp sprite0_add

; DOG_ARMADILLO
stat_dog_armadillo:

dgd_ARMADILLO_ANIM = dog_data0
dgd_ARMADILLO_MODE = dog_data1
dgd_ARMADILLO_FLIP = dog_data2
dgd_ARMADILLO_ROLL = dog_data3
dgd_ARMADILLO_SHOW = dog_data4

ARMADILLO_ROLL_TIME = 8

.segment "DATA"
armadillo_turn_sprite:
.byte DATA_sprite0_armadillo_roll
.byte DATA_sprite0_armadillo_ball0
.byte DATA_sprite0_armadillo_ball0
.byte DATA_sprite0_armadillo_roll

.segment "CODE"

dog_bound_armadillo:
	lda dgd_ARMADILLO_MODE, Y
	cmp #2
	beq :+
		DOG_BOUND -6,-8,5,-1
		rts
	:
		DOG_BOUND -5,-10,4,-2
		rts
	;
dog_move_armadillo:
	lda dgd_ARMADILLO_FLIP, Y
	bne @move_right
	@move_left:
		lda #-1
		jsr move_finish_x
		DOG_CMP_X 17
		bcc @stop
		DOG_POINT -6,-3
		jsr collide_all
		bne @stop
		ldy dog_now
		DOG_POINT -7,0
		jsr collide_all
		beq @stop
		jmp @continue
	;
	@move_right:
		lda #1
		jsr move_finish_x
		DOG_CMP_X 501
		bcs @stop
		DOG_POINT 5,-3
		jsr collide_all
		bne @stop
		ldy dog_now
		DOG_POINT 6,0
		jsr collide_all
		beq @stop
		;jmp @continue
	;
	@continue:
		ldy dog_now
		lda #0
		rts
	@stop:
		ldy dog_now
		lda #1
		rts
	;
dog_random_roll_armadillo:
	jsr prng4
	ora #64
	sta dgd_ARMADILLO_ROLL, Y
	rts

dog_init_armadillo:
	jsr dog_random_roll_armadillo
	jsr prng
	and #1
	sta dgd_ARMADILLO_FLIP, Y
	rts
dog_tick_armadillo:
	; touch
	jsr dog_bound_armadillo
	jsr lizard_touch
	beq @touch_end
		lda current_lizard
		cmp #LIZARD_OF_DEATH
		bne :+
			ldx dgd_ARMADILLO_FLIP, Y
			lda #DATA_sprite0_armadillo_skull
			jmp bones_convert
		:
		cmp #LIZARD_OF_HEAT
		bne :++
			lda dgd_ARMADILLO_ROLL, Y
			cmp #ARMADILLO_ROLL_TIME
			bcc :+
				lda #ARMADILLO_ROLL_TIME
				sta dgd_ARMADILLO_ROLL, Y
			:
		:
	@touch_end:
	; mode
	lda dgd_ARMADILLO_MODE, Y
	bne @mode_1
	@mode_0:
		lda dgd_ARMADILLO_ROLL, Y
		cmp #ARMADILLO_ROLL_TIME
		bcs @no_roll
			cmp #1
			bcs @roll_start_end
				PLAY_SOUND_SCROLL SOUND_ARMADILLO
				lda #2
				sta dgd_ARMADILLO_MODE, Y
				jmp @mode_end
			@roll_start_end:
			sec
			sbc #1
			sta dgd_ARMADILLO_ROLL, Y
			jmp @mode_end
		@no_roll:
		lda dgd_ARMADILLO_ANIM, Y
		and #1
		bne :+
			ldx dog_now
			inc dgd_ARMADILLO_ANIM, X
			jmp @mode_end
		:
			ldx dog_now
			dec dgd_ARMADILLO_ROLL, X
			lda dgd_ARMADILLO_ANIM, Y
			and #3
			cmp #3
			bne @show_end
				lda dgd_ARMADILLO_SHOW, Y
				clc
				adc #1
				cmp #3
				bcc :+
					lda #0
				:
				sta dgd_ARMADILLO_SHOW, Y
			@show_end:
			inc dgd_ARMADILLO_ANIM, X
			jsr dog_move_armadillo
			beq :+
				lda #1
				sta dgd_ARMADILLO_MODE, Y
				lda #0
				sta dgd_ARMADILLO_ANIM, Y
				sta dgd_ARMADILLO_SHOW, Y
			:
		;
		jmp @mode_end
	;
	@mode_1:
	cmp #1
	bne @mode_2
		lda dgd_ARMADILLO_ANIM, Y
		cmp #ARMADILLO_ROLL_TIME
		bcc @turn_end
			lda #0
			sta dgd_ARMADILLO_ANIM, Y
			lda dgd_ARMADILLO_SHOW, Y
			clc
			adc #1
			sta dgd_ARMADILLO_SHOW, Y
			cmp #4
			bcc :+
				; end of turn
				lda #0
				sta dgd_ARMADILLO_MODE, Y
				sta dgd_ARMADILLO_SHOW, Y
				jmp @mode_end
			:
			cmp #2
			bne @turn_end
				lda dgd_ARMADILLO_FLIP, Y
				eor #1
				sta dgd_ARMADILLO_FLIP, Y
				lda dgd_ARMADILLO_ROLL, Y
				cmp #ARMADILLO_ROLL_TIME
				bcs :+
					lda #0
					sta dgd_ARMADILLO_ROLL, Y
					lda #2
					sta dgd_ARMADILLO_MODE, Y
					jmp @mode_end
				:
			;
		@turn_end:
		ldx dog_now
		inc dgd_ARMADILLO_ANIM, X
		jmp @mode_end
	;
	@mode_2:
	cmp #2
	bne @mode_end
		jsr dog_move_armadillo
		beq :+
			lda #1
			sta dgd_ARMADILLO_MODE, Y
			sta dgd_ARMADILLO_SHOW, Y
			lda #ARMADILLO_ROLL_TIME
			sta dgd_ARMADILLO_ANIM, Y
			jsr dog_random_roll_armadillo
		:
		;jmp @mode_end
	;
	@mode_end:
	; overlap
	jsr dog_bound_armadillo
	jsr lizard_overlap_no_stone
	beq :+
		jsr lizard_die
	:
	rts
dog_draw_armadillo:
	DX_SCROLL_EDGE
	stx jh ; temporarily store dx
	lda #DATA_sprite0_armadillo
	sta s
	lda dgd_ARMADILLO_MODE, Y
	bne @mode_1
		lda dgd_ARMADILLO_ROLL, Y
		cmp #ARMADILLO_ROLL_TIME
		bcs :+
			lda #DATA_sprite0_armadillo_roll
			jmp :++
		:
			lda dgd_ARMADILLO_SHOW, Y
			clc
			adc #DATA_sprite0_armadillo
		:
		sta s
		jmp @draw
	@mode_1:
	cmp #1
	bne @mode_2
		ldx dgd_ARMADILLO_SHOW, Y
		lda armadillo_turn_sprite, X
		sta s
		jmp @draw
	@mode_2:
	cmp #2
	bne @draw
		lda dog_x, Y
		lsr
		lsr
		lsr
		and #1
		clc
		adc #DATA_sprite0_armadillo_ball0
		sta s
		;jmp @draw
	;
@draw:
	lda dgd_ARMADILLO_FLIP, Y
	sta t
	ldx jh ; dx
	lda dog_y, Y
	tay
	lda t
	bne :+
		lda s
		jmp sprite0_add
	:
		lda s
		jmp sprite0_add_flipped
	;

; DOG_BEETLE
stat_dog_beetle:

dgd_BEETLE_ANIM = dog_data0
dgd_BEETLE_MODE = dog_data1 ; 0 hide, 1 wake, 2 walk left, 3 walk right, 4 fall
dgd_BEETLE_PASS = dog_data2

BEETLE_RANGE = 16

dog_beetle_block:
	lda dog_type, Y
	cmp #DOG_SKEETLE
	bne :+
		rts
	:
	DOG_BLOCKER -7,-12,6,-1
	rts
dog_beetle_stop_left:
	DOG_CMP_X 9
	bcc dog_beetle_yes_stop
	DOG_POINT -8,0
	jsr collide_all
	beq dog_beetle_yes_stop ; no ground
	tya
	sec
	sbc #8
	tay; DOG_POINT -8,-8
	jsr collide_all
	bne dog_beetle_yes_stop ; wall
dog_beetle_no_stop:
	ldy dog_now
	lda #0
	rts
dog_beetle_yes_stop:
	ldy dog_now
	lda #1
	rts
dog_beetle_stop_right:
	lda room_scrolling
	bne :+
		DOG_CMP_X (255-9)
		bcs dog_beetle_yes_stop
		jmp :++
	:
		DOG_CMP_X (511-9)
		bcs dog_beetle_yes_stop
	:
	DOG_POINT 7,0
	jsr collide_all
	beq dog_beetle_yes_stop ; no ground
	tya
	sec
	sbc #8
	tay ; DOG_POINT 7,-8
	jsr collide_all
	bne dog_beetle_yes_stop ; wall
	jmp dog_beetle_no_stop
dog_beetle_bound:
	DOG_BOUND -7,-12,6,-1
	rts

dog_init_beetle:
	jsr dog_beetle_block
	lda #8
	sta dgd_BEETLE_ANIM, Y
	jsr prng
	and #127
	clc
	adc #10
	sta dgd_BEETLE_PASS, Y
	rts
dog_tick_beetle:
	jsr dog_beetle_bound
	jsr lizard_touch_death
	beq @no_death
		jsr empty_blocker
		lda dog_type, Y
		lda #DATA_sprite0_beetle_skull
		ldx #0
		jmp bones_convert
	@no_death:
	DOG_BOUND -BEETLE_RANGE,-(BEETLE_RANGE+8),BEETLE_RANGE,(BEETLE_RANGE-8)
	jsr lizard_overlap
	beq @end_near
		lda dgd_BEETLE_MODE, Y
		beq :+
			lda #0
			sta dgd_BEETLE_ANIM, Y
			sta dgd_BEETLE_MODE, Y
			jmp @end_hide
		:
			lda dgd_BEETLE_ANIM, Y
			cmp #7
			bcc :+
				lda #7
				sta dgd_BEETLE_ANIM, Y
			:
		;
		@end_hide:
		jsr dog_beetle_bound
		jsr do_push
		beq @end_near
		cmp #1
		bne :+
			lda #1
			jsr move_finish_x
			jsr dog_beetle_block
			jmp @end_near
		:
		cmp #2
		bne :+
			lda #-1
			jsr move_finish_x
			jsr dog_beetle_block
			;jmp @end_near
		:
	@end_near:
	lda dog_y, Y
	cmp #240
	bcs @fall
	jsr dog_beetle_bound
	jsr do_fall
	beq @fall_wake
	@fall:
		lda #4
		sta dgd_BEETLE_MODE, Y
		lda dog_y, Y
		clc
		adc #1
		sta dog_y, Y
		cmp #255
		bcc @on_screen
			lda dog_type, Y
			cmp #DOG_SKEETLE
			beq :+
				jsr empty_blocker
			:
			jmp empty_dog
		@on_screen:
		cmp water
		bne @splash_end
			PLAY_SOUND_SCROLL SOUND_SPLASH_SMALL
			lda dog_type + SPLASHER_SLOT
			cmp #DOG_SPLASHER
			bne @splash_end
			lda dog_x, Y
			sec
			sbc #4
			sta dog_x + SPLASHER_SLOT
			lda dog_xh, Y
			sbc #0
			sta dog_xh + SPLASHER_SLOT
			lda water
			sec
			sbc #9
			sta dog_y + SPLASHER_SLOT
			lda #0
			sta dog_data0 + SPLASHER_SLOT
		@splash_end:
		jsr dog_beetle_block
		jmp @fall_wake_end
	@fall_wake:
	lda dgd_BEETLE_MODE, Y
	cmp #4
	bne :+
		lda #1
		sta dgd_BEETLE_MODE, Y
		lda #0
		sta dgd_BEETLE_ANIM, Y
	:
	@fall_wake_end:
dog_tick_beetle_mid:
	; beetle behaviour
	lda dgd_BEETLE_MODE, Y
	bne @not_0
		; mode 0 = hide
		lda dgd_BEETLE_ANIM, Y
		bne :+
			; if dgd(BEETLE_ANIM) == 0
			lda #1
			sta dgd_BEETLE_ANIM, Y
			jsr prng
			and #127
			clc
			adc #10
			sta dgd_BEETLE_PASS, Y
			rts
		:
		cmp #8
		bcc :+
		cmp dgd_BEETLE_PASS, Y
		bcc :+
			; if dgd(BEETLE_ANIM) >= 8 and >= dgd(BEETLE_PASS)
			lda #0
			sta dgd_BEETLE_ANIM, Y
			lda #1
			sta dgd_BEETLE_MODE, Y
			rts
		:
		cmp #255
		beq :+
			clc
			adc #1
			sta dgd_BEETLE_ANIM, Y
		:
		rts
	@not_0:
	cmp #1
	bne @not_1
		; mode 1 = wake
		lda dgd_BEETLE_ANIM, Y
		cmp #8
		bcs :+
			clc
			adc #1
			sta dgd_BEETLE_ANIM, Y
			rts
		:
		jsr dog_beetle_stop_right
		beq :+
			lda #2
			jmp @wake_end
		:
		jsr dog_beetle_stop_left
		beq :+
			lda #3
			jmp @wake_end
		:
			; else random direction
			jsr prng
			and #1
			clc
			adc #2
		@wake_end:
		sta dgd_BEETLE_MODE, Y
		jsr prng
		cmp #192
		bcc :+
			and #191
		:
		clc
		adc #25
		sta dgd_BEETLE_ANIM, Y
		rts
	@not_1:
	cmp #2
	bne @not_2
		; 2 = walk left
		lda dgd_BEETLE_ANIM, Y
		beq @hide
		jsr dog_beetle_stop_left
		bne @hide
		lda dgd_BEETLE_ANIM, Y
		sec
		sbc #1
		sta dgd_BEETLE_ANIM, Y
		and #3
		bne :+
			lda #-1
			jsr move_finish_x
			jsr dog_beetle_block
		:
		rts
	@hide:
		lda #0
		sta dgd_BEETLE_ANIM, Y
		sta dgd_BEETLE_MODE, Y
		rts
	@not_2:
	cmp #3
	beq :+
		; 4+ = return
		rts
	:
		; 3 = walk right
		lda dgd_BEETLE_ANIM, Y
		beq @hide
		jsr dog_beetle_stop_right
		bne @hide
		lda dgd_BEETLE_ANIM, Y
		sec
		sbc #1
		sta dgd_BEETLE_ANIM, Y
		and #3
		bne :+
			lda #1
			jsr move_finish_x
			jsr dog_beetle_block
		:
		rts
	;
dog_draw_beetle:
	DX_SCROLL_EDGE
	lda #DATA_sprite0_beetle
	sta l
	lda dgd_BEETLE_MODE, Y
	cmp #2
	bcs :++
		lda dgd_BEETLE_ANIM, Y
		cmp #8
		bcs :+
			lda #DATA_sprite0_beetle_hide
			sta l
		:
		jmp @draw
	:
	cmp #4
	bcs @draw
		lda #DATA_sprite0_beetle_walk
		sta l
	;
	@draw:
	; X = dx
	lda dog_x, Y
	ror
	bcs :+
		lda dog_y, Y
		tay
		lda l
		jmp sprite0_add
	:
		lda dog_y, Y
		tay
		lda l
		jmp sprite0_add_flipped
	;rts

; DOG_SKEETLE
stat_dog_skeetle:

dog_init_skeetle = dog_init_beetle
dog_tick_skeetle:
	jsr dog_beetle_bound
	jsr lizard_touch_death
	beq @no_death
		lda dog_type, Y
		lda #DATA_sprite0_beetle_skull
		ldx #0
		jmp bones_convert
	@no_death:
	DOG_BOUND -BEETLE_RANGE,-(BEETLE_RANGE+8),BEETLE_RANGE,(BEETLE_RANGE-8)
	jsr lizard_overlap_no_stone
	beq @end_near
		lda dgd_BEETLE_MODE, Y
		beq :+
			lda #0
			sta dgd_BEETLE_ANIM, Y
			sta dgd_BEETLE_MODE, Y
			jmp @end_hide
		:
			lda dgd_BEETLE_ANIM, Y
			cmp #7
			bcc :+
				lda #7
				sta dgd_BEETLE_ANIM, Y
			:
		;
		@end_hide:
	@end_near:
	jsr dog_tick_beetle_mid
	ldy dog_now ; just in case dog_tick_beetle_mid clobbers Y
	DOG_BOUND -7,-11,6,-1
	jsr lizard_overlap_no_stone
	beq :+
		jsr lizard_die
	:
	rts
dog_draw_skeetle = dog_draw_beetle

; DOG_SEEKER_FISH
stat_dog_seeker_fish:

dgd_SEEKER_FISH_TURN = dog_data0
dgd_SEEKER_FISH_FLIP = dog_data1
dgd_SEEKER_FISH_ANIM = dog_data2
dgd_SEEKER_FISH_SWIM = dog_data3
dgd_SEEKER_FISH_PASS = dog_data4
dgd_SEEKER_FISH_DIR  = dog_data5
dgd_SEEKER_FISH_X1   = dog_data6
dgd_SEEKER_FISH_Y1   = dog_data7
dgd_SEEKER_FISH_VX0  = dog_data8
dgd_SEEKER_FISH_VX1  = dog_data9
dgd_SEEKER_FISH_VY0  = dog_data10
dgd_SEEKER_FISH_VY1  = dog_data11

SEEKER_FISH_TURN_TIME = 12
SEEKER_FISH_MOVE      = 20
SEEKER_FISH_MAX       = 250

.segment "DATA"
seeker_fish_st:
.byte DATA_sprite0_seeker_fish
.byte DATA_sprite0_seeker_fish_walk0
.byte DATA_sprite0_seeker_fish
.byte DATA_sprite0_seeker_fish_walk1

.segment "CODE"

dog_bound_seeker_fish:
	DOG_BOUND -6,-11,6,-3
	rts

dog_init_seeker_fish:
	jsr prng
	sta dgd_SEEKER_FISH_DIR, Y
	jsr prng
	and #1
	sta dgd_SEEKER_FISH_FLIP, Y
	rts
dog_tick_seeker_fish:
	; death
	jsr dog_bound_seeker_fish
	jsr lizard_touch_death
	beq :+
		lda #DATA_sprite0_seeker_fish_skull
		ldx dgd_SEEKER_FISH_FLIP, Y
		jmp bones_convert
	:
	; turning
	ldx dgd_SEEKER_FISH_TURN, Y
	beq :+
		dex
		txa
		sta dgd_SEEKER_FISH_TURN, Y
	:
	; behaviour
	lda dgd_SEEKER_FISH_ANIM, Y
	bne @start_end
		jsr prng
		and #31
		clc
		adc #SEEKER_FISH_TURN_TIME
		sta dgd_SEEKER_FISH_PASS, Y
		jsr prng
		sta m ; temp
		jsr prng
		and #$10
		ora m
		sta dgd_SEEKER_FISH_DIR, Y
		lda lizard_dead
		beq :+
			lda dgd_SEEKER_FISH_DIR, Y
			and #$0F
			sta dgd_SEEKER_FISH_DIR, Y
		:
		lda #0 ; ANIM
	@start_end:
	clc
	adc #1 ; ++ANIM
	cmp dgd_SEEKER_FISH_PASS, Y
	bcc :+
		lda #0 ; ANIM = 0
	:
	sta dgd_SEEKER_FISH_ANIM, Y
	; update direction if seeking
	lda dgd_SEEKER_FISH_DIR, Y
	and #$10
	tax ; store dir in X temporarily
	beq @seek_end
		; horizontal
		lda dog_xh, Y
		cmp lizard_xh
		bne :+
			lda dog_x, Y
			cmp lizard_x+0
			beq @h_end
			jmp :++
		:
			lda dog_x, Y
			cmp lizard_x+0
		:
		lda dog_xh, Y
		sbc lizard_xh
		bcs @h_gt
		@h_lt:
			txa
			ora #$01
			tax
			jmp @h_end
		@h_gt:
			txa
			ora #$02
			tax
			;jmp @h_end
		@h_end:
		; vertical
		txa ; A = dir
		ldx dog_y, Y
		cpx lizard_y+0
		beq @v_end
		bcs @v_gt
		@v_lt:
			ora #$04
			jmp @v_end
		@v_gt:
			ora #$08
			;jmp @v_end
		@v_end:
		sta dgd_SEEKER_FISH_DIR, Y
	@seek_end:
	; turn if direction changes
	lda dgd_SEEKER_FISH_VX0, Y
	bpl @not_left
		lda dgd_SEEKER_FISH_FLIP, Y
		beq @turn_end
		lda #0
		jmp @turn_common
	@not_left:
	;lda dgd_SEEKER_FISH_VX0, Y
	ora dgd_SEEKER_FISH_VX1, Y
	beq @not_right
		lda dgd_SEEKER_FISH_FLIP, Y
		bne @turn_end
		lda #1
		;jmp @turn_common
	@turn_common:
		sta dgd_SEEKER_FISH_FLIP, Y
		lda dgd_SEEKER_FISH_TURN, Y
		bne :+
			lda #SEEKER_FISH_TURN_TIME
			sta dgd_SEEKER_FISH_TURN, Y
		:
	@not_right:
	@turn_end:
	; swim animation
	ldx dgd_SEEKER_FISH_SWIM, Y
	inx
	txa
	sta dgd_SEEKER_FISH_SWIM, Y
	; adjust velocity
	lda dgd_SEEKER_FISH_DIR, Y
	and #$01
	beq @dxr_end
		lda dgd_SEEKER_FISH_VX1, Y
		cmp #<SEEKER_FISH_MAX
		lda dgd_SEEKER_FISH_VX0, Y
		sbc #>SEEKER_FISH_MAX
		bvc :+
		eor #$80
		:
		bpl @dxr_end
		lda dgd_SEEKER_FISH_VX1, Y
		clc
		adc #<SEEKER_FISH_MOVE
		sta dgd_SEEKER_FISH_VX1, Y
		lda dgd_SEEKER_FISH_VX0 ,Y
		adc #>SEEKER_FISH_MOVE
		sta dgd_SEEKER_FISH_VX0, Y
	@dxr_end:
	lda dgd_SEEKER_FISH_DIR, Y
	and #$02
	beq @dxl_end
		lda dgd_SEEKER_FISH_VX1, Y
		cmp #<(-SEEKER_FISH_MAX)
		lda dgd_SEEKER_FISH_VX0, Y
		sbc #>(-SEEKER_FISH_MAX)
		bvc :+
		eor #$80
		:
		bmi @dxl_end
		lda dgd_SEEKER_FISH_VX1, Y
		clc
		adc #<(-SEEKER_FISH_MOVE)
		sta dgd_SEEKER_FISH_VX1, Y
		lda dgd_SEEKER_FISH_VX0 ,Y
		adc #>(-SEEKER_FISH_MOVE)
		sta dgd_SEEKER_FISH_VX0, Y
	@dxl_end:
	lda dgd_SEEKER_FISH_DIR, Y
	and #$04
	beq @dxd_end
		lda dgd_SEEKER_FISH_VY1, Y
		cmp #<SEEKER_FISH_MAX
		lda dgd_SEEKER_FISH_VY0, Y
		sbc #>SEEKER_FISH_MAX
		bvc :+
		eor #$80
		:
		bpl @dxd_end
		lda dgd_SEEKER_FISH_VY1, Y
		clc
		adc #<SEEKER_FISH_MOVE
		sta dgd_SEEKER_FISH_VY1, Y
		lda dgd_SEEKER_FISH_VY0 ,Y
		adc #>SEEKER_FISH_MOVE
		sta dgd_SEEKER_FISH_VY0, Y
	@dxd_end:
	lda dgd_SEEKER_FISH_DIR, Y
	and #$08
	beq @dxu_end
		lda dgd_SEEKER_FISH_VY1, Y
		cmp #<(-SEEKER_FISH_MAX)
		lda dgd_SEEKER_FISH_VY0, Y
		sbc #>(-SEEKER_FISH_MAX)
		bvc :+
		eor #$80
		:
		bmi @dxu_end
		lda dgd_SEEKER_FISH_VY1, Y
		clc
		adc #<(-SEEKER_FISH_MOVE)
		sta dgd_SEEKER_FISH_VY1, Y
		lda dgd_SEEKER_FISH_VY0 ,Y
		adc #>(-SEEKER_FISH_MOVE)
		sta dgd_SEEKER_FISH_VY0, Y
	@dxu_end:
	; move the dog
	lda dgd_SEEKER_FISH_X1, Y
	clc
	adc dgd_SEEKER_FISH_VX1, Y
	sta dgd_SEEKER_FISH_X1, Y
	lda dog_x, Y
	adc dgd_SEEKER_FISH_VX0, Y
	; A = temp dog_x (dog_xh is irrelevant if dx is small enough)
	sec
	sbc dog_x, Y
	sta u ; u = dx
	lda dgd_SEEKER_FISH_Y1, Y
	clc
	adc dgd_SEEKER_FISH_VY1, Y
	sta dgd_SEEKER_FISH_Y1, Y
	lda dog_y, Y
	adc dgd_SEEKER_FISH_VY0, Y
	; A = new Y0
	sec
	sbc dog_y, Y
	sta v ; v = dy
	jsr dog_bound_seeker_fish
	jsr move_dog
	jsr move_finish
	DOG_CMP_X 9
	bcs :+
		lda #1
		jsr move_finish_x
	:
	DOG_CMP_X 504
	bcc :+
		lda #-1
		jsr move_finish_x
	:
	lda water
	clc
	adc #18
	cmp dog_y, Y
	bcc :+
		; if dgy < water + 17 (water + 18 >= dgy)
		sta dog_y, Y
	:
	; overlap
	jsr dog_bound_seeker_fish
	jsr lizard_overlap_no_stone
	beq :+
		jsr lizard_die
	:
	rts
dog_draw_seeker_fish:
	DX_SCROLL_EDGE
	stx jh ; temp
	; facing reversed if turning
	ldx dgd_SEEKER_FISH_FLIP, Y
	lda dgd_SEEKER_FISH_TURN, Y
	cmp #(SEEKER_FISH_TURN_TIME/2)
	bcc :+
		txa
		eor #1
		tax
	:
	stx i ; flip
	; sprite
	lda dgd_SEEKER_FISH_SWIM, Y
	lsr
	lsr
	lsr
	and #3
	tax
	lda seeker_fish_st, X
	ldx dgd_SEEKER_FISH_TURN, Y
	beq :+
		lda #DATA_sprite0_seeker_fish_turn
	:
	pha ; put sprite on stack
	; X/Y
	ldx jh ; dx
	lda dog_y, Y
	tay
	; draw
	lda i
	beq :+
		pla
		jmp sprite0_add_flipped
	:
		pla
		jmp sprite0_add

; DOG_MANOWAR
stat_dog_manowar:

dgd_MANOWAR_ANIM = dog_data0
dgd_MANOWAR_FLIP = dog_data1
dgd_MANOWAR_MODE = dog_data2 ; 0=float, 1=turn, 2=puff
dgd_MANOWAR_PASS = dog_data3
dgd_MANOWAR_X1   = dog_data4
dgd_MANOWAR_Y1   = dog_data5
dgd_MANOWAR_VX0  = dog_data6
dgd_MANOWAR_VX1  = dog_data7
dgd_MANOWAR_VY0  = dog_data8
dgd_MANOWAR_VY1  = dog_data9

MANOWAR_DRAG       = 8
MANOWAR_PUFF_RIGHT = 320
MANOWAR_PUFF_LEFT  = -MANOWAR_PUFF_RIGHT
MANOWAR_PUFF_UP    = -350
MANOWAR_GRAVITY    = 13
MANOWAR_MAX_DOWN   = 90

dog_bound_manowar:
	DOG_BOUND -6,-14,5,-8
	rts

dog_init_manowar:
	jsr prng
	and #1
	sta dgd_MANOWAR_FLIP, Y
	rts
dog_tick_manowar:
	; death
	jsr dog_bound_manowar
	jsr lizard_touch_death
	beq :+
		lda #DATA_sprite0_manowar_skull
		ldx dgd_MANOWAR_FLIP, Y
		jmp bones_convert
	:
	; updates by mode
	lda dgd_MANOWAR_MODE, Y
	bne @not_mode0
		; mode 0 float
		lda dgd_MANOWAR_ANIM, Y
		bne :+
			jsr prng
			and #63
			sta dgd_MANOWAR_PASS, Y
			lda #0 ; dgd_MANOWAR_ANIM = 0
		:
		clc
		adc #1
		sta dgd_MANOWAR_ANIM, Y
		cmp #9
		bcc @wake_end
		cmp dgd_MANOWAR_PASS, Y
		bcc @wake_end
			; if dgd_MANOWAR_ANIM > 8 && dgd_MANOWAR_ANIM >= dgd_MANOWAR_PASS
			lda #0
			sta dgd_MANOWAR_ANIM, Y
			jsr prng
			and #1
			bne :+
				lda dog_y, Y
				cmp lizard_y+0
				rol ; A&1 = 1 if dog_y >= lizard_y
				and #1
				clc
				adc #1
				sta dgd_MANOWAR_MODE, Y
				jmp @end_modes
			:
				lda lizard_x+0
				cmp dog_x, Y
				lda lizard_xh
				sbc dog_xh, Y
				rol ; A&1 = 1 if lizard_x >= dog_x
				eor dgd_MANOWAR_FLIP, Y
				and #1
				clc
				adc #1
				sta dgd_MANOWAR_MODE, Y
				; jmp @end_modes
			;
		@wake_end:
		jmp @end_modes
	@not_mode0:
	cmp #1
	bne @not_mode1
		; mode 1 turn
		lda dgd_MANOWAR_ANIM, Y
		clc
		adc #1
		sta dgd_MANOWAR_ANIM, Y
		cmp #11
		bcc :+
			jsr prng
			and #2
			sta dgd_MANOWAR_MODE, Y ; 50% puff chance
			lda dgd_MANOWAR_FLIP, Y
			eor #1
			sta dgd_MANOWAR_FLIP, Y
			lda #0
			sta dgd_MANOWAR_ANIM, Y
		:
		jmp @end_modes
	@not_mode1:
	cmp #2
	bne @end_modes
		; mode 2 puff
		lda dgd_MANOWAR_ANIM, Y
		bne @end_start_puff
			lda dgd_MANOWAR_FLIP, Y
			beq :+
				jsr prng
				and #31
				clc
				adc #<MANOWAR_PUFF_LEFT
				sta dgd_MANOWAR_VX1, Y
				lda #0
				adc #>MANOWAR_PUFF_LEFT
				sta dgd_MANOWAR_VX0, Y
				; VX = PUFF_LEFT + prng & 31
				jmp :++
			:
				jsr prng
				and #31
				sta m ; m = prng & 31
				lda #<MANOWAR_PUFF_RIGHT
				sec
				sbc m
				sta dgd_MANOWAR_VX1, Y
				lda #>MANOWAR_PUFF_RIGHT
				sbc #0
				sta dgd_MANOWAR_VX0, Y
				; VX = PUFF_RIGHT - prng & 31
			:
			lda #<MANOWAR_PUFF_UP
			sta dgd_MANOWAR_VY1, Y
			lda #>MANOWAR_PUFF_UP
			sta dgd_MANOWAR_VY0, Y
			lda #0 ; dgd_MANOWAR_ANIM = 0
		@end_start_puff:
		clc
		adc #1
		sta dgd_MANOWAR_ANIM, Y
		cmp #31
		bcc :+
			lda #0
			sta dgd_MANOWAR_MODE, Y
			sta dgd_MANOWAR_ANIM, Y
		:
		jmp @end_modes
	@end_modes:
	; proposed move
	lda dgd_MANOWAR_X1, Y
	clc
	adc dgd_MANOWAR_VX1, Y
	sta dgd_MANOWAR_X1, Y ; LSB always updated regardless of collision
	lda dog_x, Y
	adc dgd_MANOWAR_VX0, Y
	sta u ; u = tx (temporarily store MSB in case of collision)
	; note: dog_xh not needed, dx is always small enough
	lda dgd_MANOWAR_Y1, Y
	clc
	adc dgd_MANOWAR_VY1, Y
	sta dgd_MANOWAR_Y1, Y
	lda dog_y, Y
	adc dgd_MANOWAR_VY0, Y
	sta v ; v = ty
	; gravity and cap VY
	lda dgd_MANOWAR_VY1, Y
	clc
	adc #<MANOWAR_GRAVITY
	sta dgd_MANOWAR_VY1, Y
	lda dgd_MANOWAR_VY0, Y
	adc #>MANOWAR_GRAVITY
	sta dgd_MANOWAR_VY0, Y
	lda #<MANOWAR_MAX_DOWN
	cmp dgd_MANOWAR_VY1, Y
	lda #>MANOWAR_MAX_DOWN
	sbc dgd_MANOWAR_VY0, Y
	bvc :+
	eor #$80
	:
	bpl :+
		; MANOWAR_MAX_DOWN < dgd_MANOWAR_VY1
		lda #<MANOWAR_MAX_DOWN
		sta dgd_MANOWAR_VY1, Y
		lda #>MANOWAR_MAX_DOWN
		sta dgd_MANOWAR_VY0, Y
	:
	; horizontal drag
	lda dgd_MANOWAR_VX0, Y
	bmi @vx_negative
	ora dgd_MANOWAR_VX1, Y
	beq @end_vx
	@vx_positive:
		lda dgd_MANOWAR_VX1, Y
		sec
		sbc #<MANOWAR_DRAG
		sta dgd_MANOWAR_VX1, Y
		lda dgd_MANOWAR_VX0, Y
		sbc #>MANOWAR_DRAG
		sta dgd_MANOWAR_VX0, Y
		bmi @vx_zero
		jmp @end_vx
	@vx_zero:
		lda #0
		sta dgd_MANOWAR_VX0, Y
		sta dgd_MANOWAR_VX1, Y
		jmp @end_vx
	@vx_negative:
		lda dgd_MANOWAR_VX1, Y
		sec
		sbc #<(-MANOWAR_DRAG)
		sta dgd_MANOWAR_VX1, Y
		lda dgd_MANOWAR_VX0, Y
		sbc #>(-MANOWAR_DRAG)
		sta dgd_MANOWAR_VX0, Y
		bpl @vx_zero
	@end_vx:
	; do the move
	lda u
	sec
	sbc dog_x, Y ; dx = tx - dog_x
	sta u
	lda v
	sec
	sbc dog_y, Y ; dy = ty - dog_y
	sta v
	; u,v = dx,dy
	; i,j,k,l are already set up from the death test
	jsr dog_bound_manowar
	jsr move_dog
	jsr move_finish
	; bound X coordinate
	DOG_CMP_X 9
	bcs :+
		lda #<9
		sta dog_x, Y
		lda #>9
		sta dog_xh, Y
	:
	DOG_CMP_X (511-8)
	bcc :+
		lda #<(511-8)
		sta dog_x, Y
		lda #>(511-8)
		sta dog_xh, Y
	:
	; bound Y coordinate
	lda water
	clc
	adc #16
	sta m ; m = water+16
	lda dog_y, Y
	cmp m
	bcs :+
		lda m
		sta dog_y, Y
	:
	; lizard harm
	jsr dog_bound_manowar
	jsr lizard_overlap_no_stone
	beq :+
		jsr lizard_die
	:
	rts
dog_draw_manowar:
	DX_SCROLL_EDGE
	stx lh ; temporary
	lda #DATA_sprite0_manowar
	ldx dgd_MANOWAR_MODE, Y
	cpx #1
	bne :+
		lda #DATA_sprite0_manowar_turn
		jmp @mode_end
	:
	cpx #2
	bne @mode_end
		lda dgd_MANOWAR_ANIM, Y
		and #8
		beq :+
			lda #DATA_sprite0_manowar_puff1
			jmp @mode_end
		:
			lda #DATA_sprite0_manowar_puff0
		;
	@mode_end:
	sta m
	ldx lh ; dx
	lda dog_y, Y
	sta n
	lda dgd_MANOWAR_FLIP, Y
	beq :+
		lda m
		ldy n
		jmp sprite0_add_flipped
		;rts
	:
		lda m
		ldy n
		jmp sprite0_add
	;rts

; DOG_SNAIL
stat_dog_snail:

dgd_SNAIL_ANIM  = dog_data0
dgd_SNAIL_FLIP  = dog_data1
dgd_SNAIL_START = dog_data2

; snail motion tables
.segment "DATA"
snail_floor_x:   .byte  -3,  3, -3,  3
snail_floor_y:   .byte  -3, -3,  3,  3
snail_trail_x:   .byte  -2,  2, -2,  2,  3, -3,  3, -3
snail_trail_y:   .byte   3,  3, -3, -3,  2,  2, -2, -2
snail_rotate:    .byte   7,  6,  5,  4,  0,  1,  2,  3
snail_wall_x:    .byte   3, -3,  3, -3,  0,  0,  0,  0
snail_wall_y:    .byte   0,  0,  0,  0, -3, -3,  3,  3
snail_move_x:    .byte   1, -1,  1, -1,  0,  0,  0,  0
snail_move_y:    .byte   0,  0,  0,  0, -1, -1,  1,  1
snail_flip_wall: .byte   4,  5,  6,  7,  3,  2,  1,  0
snail_sprite:    .byte $A6,$A6,$A6,$A6,$B6,$B6,$B6,$B6
snail_dx:        .byte  -3, -4, -3, -4, -5, -2, -5, -2
snail_dy:        .byte  -6, -6, -3, -3, -5, -5, -4, -4


.segment "CODE"

dog_snail_blocker:
	lda dgd_SNAIL_FLIP, Y
	and #6
	cmp #2
	bne :+
		DOG_BLOCKER -2,-4,2,2
		rts
	:
		DOG_BLOCKER -2,-2,2,4
		rts
	;

dog_init_snail:
	jsr prng
	sta dgd_SNAIL_ANIM, Y
	jsr prng
	and #1
	sta dgd_SNAIL_FLIP, Y
	jmp dog_snail_blocker
	;rts
dog_tick_snail:
	DOG_BOUND -2,-2,2,2
	; death
	jsr lizard_touch_death
	beq :+
		jsr empty_blocker
		lda dgd_SNAIL_FLIP, Y
		and #1
		tax
		lda #DATA_sprite0_snail_skull
		jmp bones_convert
	:
	; push
	;DOG_BOUND -2,-2,2,2
	jsr do_push
	beq @no_push
	cmp #1
	bne @not_1
		lda #1
		jsr move_finish_x ; ++dgx
		lda dog_x, Y
		cmp #<509
		lda dog_xh, Y
		sbc #>509
		bcc :+
			jmp empty_dog
		:
		lda #0
		sta dgd_SNAIL_FLIP, Y
		lda #1
		sta dgd_SNAIL_START, Y
		jmp dog_snail_blocker
	@not_1:
	cmp #2
	bne @no_push
		lda #-1
		jsr move_finish_x ; --dgx
		lda dog_x, Y
		cmp #<5
		lda dog_xh, Y
		sbc #>5
		bcs :+
			jmp empty_dog
		:
		lda #1
		sta dgd_SNAIL_START, Y
		sta dgd_SNAIL_FLIP, Y
		jmp dog_snail_blocker
	@no_push:
	;
	jsr empty_blocker
	ldx #0
	@dir_test_loop:
		stx m ; temp store m
		; dgx + SNAIL_FLOOR_X[i]
		lda dog_x, Y
		clc
		adc snail_floor_x, X
		sta n
		lda snail_floor_x, X
		jsr load_sign
		adc dog_xh, Y
		sta ih
		; dgy + SNAIL_FLOOR_Y[i]
		lda dog_y, Y
		clc
		adc snail_floor_y, X
		tay
		ldx n
		jsr collide_tile ; only test with tiles, don't climb movable objects
		bne @test_true
		ldx m
		ldy dog_now
		inx
		cpx #4
		bcc @dir_test_loop
	; test failed, snail will fall
		DOG_BOUND -2, 3, 2, 3
		jsr lizard_overlap
		jne dog_snail_blocker ; don't fall on lizard
		lda dgd_SNAIL_FLIP, Y
		and #1 ; orient to floor
		sta dgd_SNAIL_FLIP, Y
		lda dog_y, Y
		clc
		adc #1
		sta dog_y, Y
		cmp #237
		bcc :+
			jmp empty_dog
		:
		jmp dog_snail_blocker
	@test_true:
	; snail is not falling
	ldy dog_now
	lda dgd_SNAIL_FLIP, Y
	and #7
	tax
	sta m ; temporarily store FLIP & 7
	; dgx + SNAIL_TRAIL_X[r]
	lda dog_x, Y
	clc
	adc snail_trail_x, X
	sta n
	lda snail_trail_x, X
	jsr load_sign
	adc dog_xh, Y
	sta ih
	; dgy + SNAIL_TRAIL_Y[r]
	lda dog_y, Y
	clc
	adc snail_trail_y, X
	tay
	ldx n
	jsr collide_all
	bne @trail_on
		ldy dog_now
		lda dgd_SNAIL_START, Y
		bne :+
			ldx m
			lda snail_rotate, X
			sta dgd_SNAIL_FLIP, Y
			and #7
			sta m
			tax
			lda #1
			sta dgd_SNAIL_START, Y
		:
		jmp @trail_end
	@trail_on:
		ldy dog_now
		lda #0
		sta dgd_SNAIL_START, Y
	@trail_end:
	; tick animation
	lda dgd_SNAIL_ANIM, Y
	clc
	adc #1
	sta dgd_SNAIL_ANIM, Y
	and #63
	jne dog_snail_blocker
	; stop moving near lizard
	DOG_BOUND -3, -5, 3, 5
	jsr lizard_overlap
	jne dog_snail_blocker
	; move
	ldx m
	; dgx + SNAIL_WALL_X[r]
	lda dog_x, Y
	clc
	adc snail_wall_x, X
	sta n
	lda snail_wall_x, X
	jsr load_sign
	adc dog_xh, Y
	sta ih
	; dgy + SNAIL_WALL_Y[r]
	lda dog_y, Y
	clc
	adc snail_wall_y, X
	tay
	ldx n
	jsr collide_all
	beq :+
		ldy dog_now
		ldx m
		lda snail_flip_wall, X
		sta dgd_SNAIL_FLIP, Y
		jmp dog_snail_blocker
	:
		ldy dog_now
		ldx m
		; dgx += SNAIL_MOVE_X[r]
		lda snail_move_x, X
		jsr move_finish_x
		; bounds check dgx
		lda dog_x, Y
		cmp #<509
		lda dog_xh, Y
		sbc #>509
		bcs @snail_off
		lda dog_x, Y
		cmp #<5
		lda dog_xh, Y
		sbc #>5
		bcc @snail_off
		; dgy += SNAIL_MOVE_Y[r]
		lda dog_y, Y
		clc
		adc snail_move_y, X
		sta dog_y, Y
		; boundx check dgy
		cmp #237
		bcs @snail_off
		cmp #5
		bcc @snail_off
		jmp dog_snail_blocker
	@snail_off:
		jmp empty_dog
	;
dog_draw_snail:
	lda dgd_SNAIL_FLIP, Y
	lsr
	ror
	ror
	and #%11000000
	ora #3
	sta s ; attrib = ((SNAIL_FLIP & 3) << 6) | 3
	lda dgd_SNAIL_FLIP, Y
	and #7
	tax
	stx r ; temp
	lda snail_dx, X
	DX_SCROLL_OFFSET ; clobbers i
	stx j ; j = dx
	lda s
	sta i ; attrib
	ldx r ; restore temp
	; y = dgy + SNAIL_DY[r]
	lda dog_y, Y
	clc
	adc snail_dy, X
	sta k ; k = y
	lda snail_sprite, X
	tax
	lda dgd_SNAIL_ANIM, Y
	and #32
	beq :+
		inx ; if (ANIM & 32) then ++sprite
	:
	txa
	ldx j
	ldy k
	jmp sprite_tile_add

; DOG_SNAPPER
stat_dog_snapper:

dgd_SNAPPER_ANIM  = dog_data0
dgd_SNAPPER_MODE  = dog_data1
dgd_SNAPPER_VY0   = dog_data2
dgd_SNAPPER_VY1   = dog_data3
dgd_SNAPPER_Y1    = dog_data4
dgd_SNAPPER_FLOOR = dog_data5
dgd_SNAPPER_WAIT  = dog_data6

SNAPPER_JUMP  = -860
SNAPPER_GRAV  = 30
SNAPPER_MAX   = -SNAPPER_JUMP
SNAPPER_PAUSE = 32

dog_bound_snapper:
	DOG_BOUND -7,-15,6,-4
	rts

dog_init_snapper:
	jsr prng
	and #1
	sta dgd_SNAPPER_MODE, Y
	lda dog_y, Y
	sta dgd_SNAPPER_FLOOR, Y
	rts
dog_tick_snapper:
	tya
	tax
	inc dgd_SNAPPER_ANIM, X
	; death
	jsr dog_bound_snapper
	jsr lizard_touch
	beq @no_touch
		lda current_lizard
		cmp #LIZARD_OF_DEATH
		bne :+
			lda #DATA_sprite0_snapper_skull
			ldx #0
			jmp bones_convert
		:
		cmp #LIZARD_OF_HEAT
		bne :+
			lda #0
			sta dgd_SNAPPER_WAIT, Y
		:
	@no_touch:
	;
	ldx dgd_SNAPPER_WAIT, Y
	beq :+
		dex
		txa
		sta dgd_SNAPPER_WAIT, Y
		jmp @jump_end
	:
		lda dgd_SNAPPER_MODE, Y
		cmp #2
		beq @jump_end
		DOG_BOUND -20,-72,19,0
		jsr lizard_overlap_no_stone
		beq @jump_end
		jsr prng
		and #3
		beq @jump_do
		DOG_BOUND -8,-72,7,0
		jsr lizard_overlap_no_stone
		beq @jump_end
		; begin jump
	@jump_do:
		PLAY_SOUND SOUND_SNAPPER_JUMP
		lda #2
		sta dgd_SNAPPER_MODE, Y
		lda #>SNAPPER_JUMP
		sta dgd_SNAPPER_VY0, Y
		lda #<SNAPPER_JUMP
		sta dgd_SNAPPER_VY1, Y
		lda #0
		sta dgd_SNAPPER_Y1, Y
	@jump_end:
	; move based on mode
	lda dgd_SNAPPER_MODE, Y
	bne @move_0_end
		DOG_POINT -8,-6
		jsr collide_tile
		bne @stop_0
		ldy dog_now
		DOG_POINT -7,0
		jsr collide_tile
		beq @stop_0
			ldy dog_now
			lda #-1
			jsr move_finish_x
			jmp @move_end
		@stop_0:
			ldy dog_now
			lda #1
			sta dgd_SNAPPER_MODE, Y
			jmp @move_end
		;
	@move_0_end:
	cmp #1
	bne @move_1_end
		DOG_POINT 7,-6
		jsr collide_tile
		bne @stop_1
		ldy dog_now
		DOG_POINT 6,0
		jsr collide_tile
		beq @stop_1
			ldy dog_now
			lda #1
			jsr move_finish_x
			jmp @move_end
		@stop_1:
			ldy dog_now
			lda #0
			sta dgd_SNAPPER_MODE, Y
			jmp @move_end
		;
	@move_1_end:
	cmp #2
	bne @move_end
		; y16 += vy16
		lda dgd_SNAPPER_Y1, Y
		clc
		adc dgd_SNAPPER_VY1, Y
		sta dgd_SNAPPER_Y1, Y
		lda dog_y, Y
		adc dgd_SNAPPER_VY0, Y
		sta dog_y, Y
		; vy16 += SNAPPER_GRAV
		lda dgd_SNAPPER_VY1, Y
		clc
		adc #<SNAPPER_GRAV
		sta dgd_SNAPPER_VY1, Y
		lda dgd_SNAPPER_VY0, Y
		adc #>SNAPPER_GRAV
		sta dgd_SNAPPER_VY0, Y
		; clamp
		lda dgd_SNAPPER_VY1, Y
		cmp #<SNAPPER_MAX
		lda dgd_SNAPPER_VY0, Y
		sbc #>SNAPPER_MAX
		bvc :+
		eor #$80
		:
		bmi :+
			lda #<SNAPPER_MAX
			sta dgd_SNAPPER_VY1, Y
			lda #>SNAPPER_MAX
			sta dgd_SNAPPER_VY0, Y
		:
		; landing
		lda dgd_SNAPPER_FLOOR, Y
		cmp dog_y, Y
		bcs :+
			sta dog_y, Y
			jsr dog_init_snapper
			jsr prng
			and #15
			clc
			adc #SNAPPER_PAUSE
			sta dgd_SNAPPER_WAIT, Y
		:
	@move_end:
	; overlap
	jsr dog_bound_snapper
	jsr lizard_overlap_no_stone
	beq :+
		jsr lizard_die
	:
	rts
dog_draw_snapper:
	DX_SCROLL_EDGE
	; select sprite
	lda #DATA_sprite0_snapper
	sta i
	lda dgd_SNAPPER_MODE, Y
	cmp #2
	beq :+
	lda dgd_SNAPPER_VY0, Y
	bpl :+
		lda #DATA_sprite0_snapper_jump
		sta i
	:
	; flip
	; x = dx
	lda dgd_SNAPPER_ANIM, Y
	pha
	lda dog_y, Y
	tay
	pla
	lsr
	lsr
	ror
	bcc :+
		lda i
		jmp sprite0_add_flipped
	:
		lda i
		jmp sprite0_add
	;

; DOG_VOIDBALL
stat_dog_voidball:

dgd_VOIDBALL_DIR = dog_data0

.segment "DATA"
voidball_dir_x: .byte  1, -1,  1, -1
voidball_dir_y: .byte  1,  1, -1, -1

.segment "CODE"

dog_init_voidball:
	jsr prng
	and #3
	sta dgd_VOIDBALL_DIR, Y
	rts
dog_tick_voidball:
	lda dgd_VOIDBALL_DIR, Y
	tax
	lda voidball_dir_x, X
	sta u
	lda voidball_dir_y, X
	sta v
	lda dog_y, Y
	cmp #243
	bcc :+ ; cycle breaker
		jsr prng2
		and #7
		sec
		sbc #3
		clc
		adc u
		sta u
	:
	DOG_BOUND -3,-3,2,2
	jsr move_dog
	; A/w = reflect
	and #3
	beq :+ ; horizontal bounce
		lda dog_y, Y
		cmp #3
		bcc :+
		cmp #254
		bcs :+ ; prevent wrap average error
		lda dgd_VOIDBALL_DIR, Y
		eor #1
		sta dgd_VOIDBALL_DIR, Y
	:
	lda w
	and #12
	beq :+ ; vertical bounce
		lda dog_x, Y
		cmp #3
		bcc :+
		cmp #254
		bcs :+ ; prevent wrap average error
		lda dgd_VOIDBALL_DIR, Y
		eor #2
		sta dgd_VOIDBALL_DIR, Y
	:
	lda dog_x, Y
	clc
	adc u
	sta dog_x, Y
	lda dog_y, Y
	clc
	adc v
	sta dog_y, Y
	; death
	DOG_BOUND -3,-3,2,2
	jsr lizard_overlap_no_stone
	beq :+
		jmp lizard_die
	:
	rts
dog_draw_voidball:
	lda dog_x, Y
	sec
	sbc #4
	tax
	lda dog_y, Y
	sec
	sbc #5
	tay
	lda #$03
	sta i
	lda #$60
	jmp sprite_tile_add

; DOG_BALLSNAKE
stat_dog_ballsnake:

dgd_BALLSNAKE_DIR0 = dog_data0 ; 0,1,2,3 = L U R D
dgd_BALLSNAKE_DIR1 = dog_data1
dgd_BALLSNAKE_DIR2 = dog_data2
dgd_BALLSNAKE_DIR3 = dog_data3
dgd_BALLSNAKE_DIR4 = dog_data4
dgd_BALLSNAKE_DIR5 = dog_data5
dgd_BALLSNAKE_DIR6 = dog_data6
dgd_BALLSNAKE_DIR7 = dog_data7
dgd_BALLSNAKE_ANIM = dog_data8

.segment "DATA"
ballsnake_dir_x:  .byte    -1,    0,    1,    0
ballsnake_dir_y:  .byte     0,   -1,    0,    1
ballsnake_attrib: .byte   $02,  $02,  $42,  $82
ballsnake_try:    .byte     0,   -1,    1,    2
ballsnake_try_x:  .byte    -1,    4,    8,    4
ballsnake_try_y:  .byte     4,   -1,    4,    8
ballsnake_draw:   .byte     1,    7,    3,    5

.segment "CODE"

; A = direction to try
; return: 1 on success, 0 on fail
; updates dgd_BALLSNAKE_DIR0 on succeed
; clobbers u,v,A,X
dog_ballsnake_try_dir: ; A = direction to try
	and #3
	sta v ; temporary storage of A
	tax
	lda #0
	sta ih ; clear high byte for collide_tile
	lda dog_x, Y
	clc
	adc ballsnake_try_x, X
	sta u
	lda dog_y, Y
	clc
	adc ballsnake_try_y, X
	tay
	ldx u
	jsr collide_tile
	beq :+
		ldy dog_now
		lda #0
		rts
	:
	ldy dog_now
	lda v
	sta dgd_BALLSNAKE_DIR0, Y
	lda #1
	rts

; clobbers AXY, rstuvw
dog_ballsnake_build:
	; set up variables in ZP temporaries
	lda dog_x, Y
	sta u
	lda dog_y, Y
	sta v
	lda dgd_BALLSNAKE_DIR0, Y
	sta w
	lda dgd_BALLSNAKE_ANIM, Y
	sta t
	lda #8
	sec
	sbc t
	sta s
	ldx #0
	; r = temp
	; s = move_a
	; t = move_b
	; u = tx
	; v = ty
	; w = dir
	; X = i
	; Y = index to dgd_BALLSNAKE_DIR# (increment by 16)
	@build_loop:
		lda u
		sta scratch+ 0, X ; ball X
		lda v
		sta scratch+ 8, X ; ball Y
		stx r ; temp store i
		ldx w ; dir
		lda ballsnake_attrib, X
		ldx r ; restore i
		sta scratch+16, X ; ball attribute
		lda w
		and #1
		ora #$40
		cpx #0
		beq :+
			ora #$10 ; body
		:
		sta scratch+24, X ; ball tile
		; move_a
		jsr @build_move
		; increment
		inx
		cpx #8
		bcc :+
			rts
		:
		tya
		clc
		adc #16
		tay
		lda dgd_BALLSNAKE_DIR0, Y
		sta w ; dir = dgd_BALLSNAKE_DIR0+i
		; move_b
		lda s
		pha
		lda t
		sta s ; s = move_b (temporary replacement)
		jsr @build_move
		pla
		sta s ; s = move_a again for next loop
		; loop
		jmp @build_loop
	;
	@build_move: ; w = dir, s = move_a/b
		lda w
		bne :+
			lda u
			clc
			adc s
			sta u ; tx += move_a/b
			rts
		:
		cmp #1
		bne :+
			lda v
			clc
			adc s
			sta v ; ty += move_a/b
			rts
		:
		cmp #2
		bne :+
			lda u
			sec
			sbc s
			sta u ; tx -= move_a/b
			rts
		:
		;cmp #3
		;bne :+
			lda v
			sec
			sbc s
			sta v ; ty -= move_a/b
			;rts
		;:
		rts
	;

dog_init_ballsnake:
	rts
dog_tick_ballsnake:
	lda dgd_BALLSNAKE_ANIM, Y
	bne @turn_end
		; propagate directions
		ldx #7
		tya
		clc
		adc #(16*6)
		; for i=6 to 0
		:
			tay
			lda dgd_BALLSNAKE_DIR0, Y
			sta dgd_BALLSNAKE_DIR1, Y
			tya
			sec
			sbc #16
			dex
			bne :-
		;
		lda dog_now ; restore Y
		; try random direction
		ldx dgd_BALLSNAKE_DIR0, Y
		jsr prng2
		and #15
		bne :+
			dex
			jmp :++
		:
		cmp #1
		bne :+
			inx
		:
		txa
		jsr dog_ballsnake_try_dir
		bne @good_dir
			; random turn was bad, get systematic
			ldx #0
			:
				lda dgd_BALLSNAKE_DIR0, Y
				clc
				adc ballsnake_try, X
				stx m ; temporary
				jsr dog_ballsnake_try_dir
				bne @good_dir
				ldx m
				inx
				cpx #4
				bcc :-
			;
		@good_dir:
		lda #8 ; dgd_BALLSNAKE_ANIM = 8
	@turn_end:
	sec
	sbc #1
	sta dgd_BALLSNAKE_ANIM, Y
	; move
	ldx dgd_BALLSNAKE_DIR0, Y
	lda dog_x, Y
	clc
	adc ballsnake_dir_x, X
	sta dog_x, Y
	lda dog_y, Y
	clc
	adc ballsnake_dir_y, X
	sta dog_y, Y
	; collisions
	jsr dog_ballsnake_build
	; Y is now clobbered, reload from dog_now if needed
	ldx #0
	stx ih ; clear high byte of tx
	@overlap_loop:
		lda scratch+0, X ; tx
		clc
		adc #1
		sta i
		clc
		adc #5
		sta k
		lda #0
		adc #0
		sta kh
		lda scratch+8, X ; ty
		clc
		adc #1
		sta j
		clc
		adc #5
		sta l
		jsr lizard_overlap_no_stone
		beq :+
			jmp lizard_die
		:
		inx
		cpx #8
		bcc @overlap_loop
	rts
dog_draw_ballsnake:
	jsr dog_ballsnake_build
	ldy dog_now ; recover after dog_ballsnake_build
	lda dgd_BALLSNAKE_ANIM, Y
	and #3
	tax
	lda ballsnake_draw, X
	sta n ; n = index increment
	ldx #0
	:
		stx m ; m = index
		lda scratch+16, X ; attrib
		sta i
		lda scratch+24, X ; tile
		pha
		ldy scratch+ 8, X ; Y
		dey
		lda scratch+ 0, X ; X
		tax
		pla
		jsr sprite_tile_add
		; increment
		lda m
		clc
		adc n
		and #7
		tax
		bne :-
	rts

; DOG_MEDUSA
stat_dog_medusa:

dgd_MEDUSA_DIVE  = dog_data0
dgd_MEDUSA_FLIP  = dog_data1
dgd_MEDUSA_SPAWN = dog_data2
dgd_MEDUSA_TIME  = dog_data3
dgd_MEDUSA_VY0   = dog_data4
dgd_MEDUSA_VY1   = dog_data5
dgd_MEDUSA_Y1    = dog_data6
;dgd_FIRE_PAL  = dog_data10
;dgd_FIRE_ANIM = dog_data11

MEDUSA_MIN   = -300
MEDUSA_MAX   = 400
MEDUSA_ACCEL = 10

.segment "DATA"
medusa_flap_cycle:
.byte DATA_sprite0_medusa0
.byte DATA_sprite0_medusa1
.byte DATA_sprite0_medusa2
.byte DATA_sprite0_medusa1

.segment "CODE"

dog_spawn_medusa:
	; always starts in dive
	lda #1
	sta dgd_MEDUSA_DIVE, Y
	; spawn at edge away from player (as if not scrolling)
	lda lizard_x+0
	bmi :++
		lda #0
		sta dog_xh, Y
		sta dgd_MEDUSA_FLIP, Y
		lda room_scrolling
		beq :+
			lda #1
			sta dog_xh, Y
		:
		lda #247
		jmp :++
	:
		lda #0
		sta dog_xh, Y
		lda #1
		sta dgd_MEDUSA_FLIP, Y
		lda #8
	:
	sta dog_x, Y
	; spawn above the param line
	jsr prng4
	:
		cmp dog_param, Y
		beq :+
		bcc :+
		lsr
		jmp :-
	:
	sta dog_y, Y
	; reset velocity
	lda #<MEDUSA_MAX
	sta dgd_MEDUSA_VY1, Y
	lda #>MEDUSA_MAX
	sta dgd_MEDUSA_VY0, Y
	lda #0
	sta dgd_MEDUSA_Y1, Y
	rts
dog_time_medusa:
	jsr prng2
	and #31
	ora #16
	sta dgd_MEDUSA_TIME, Y
	rts
dog_bound_medusa:
	DOG_BOUND -8,-4,7,-1
	rts

dog_init_medusa:
	jsr prng4
	ora #1
	and #127
	sta dgd_MEDUSA_SPAWN, Y
	jmp dog_time_medusa
	;rts
dog_tick_medusa:
	; spwan timer
	lda dgd_MEDUSA_SPAWN, Y
	beq @spawn_end
		sec
		sbc #1
		sta dgd_MEDUSA_SPAWN, Y
		beq :+
			rts
		:
		jsr dog_spawn_medusa
	@spawn_end:
	; touch
	jsr dog_bound_medusa
	jsr lizard_touch_death
	beq @touch_end
		ldx dgd_MEDUSA_FLIP, Y
		lda #DATA_sprite0_medusa_skull
		jmp bones_convert
	@touch_end:
	; horizontal motion
	lda dgd_MEDUSA_FLIP, Y
	bne @h_right
	@h_left:
		lda #-1
		jsr move_finish_x
		DOG_CMP_X 8
		bcc dog_init_medusa
		jmp @h_end
	@h_right:
		lda #1
		jsr move_finish_x
		lda room_scrolling
		bne :+
			DOG_CMP_X 248
			jmp :++
		:
			DOG_CMP_X 503
		:
		bcs dog_init_medusa
		;jmp @h_end
	@h_end:
	; flap sound
	lda dgd_MEDUSA_VY0, Y
	bpl @flap_end
	lda dog_x, Y
	and #15
	bne @flap_end
		PLAY_SOUND_SCROLL SOUND_OWL_FLAP
	@flap_end:
	; dive control
	lda dog_y, Y
	cmp dog_param, Y
	bcc :+
		lda #0
		sta dgd_MEDUSA_DIVE, Y
		jmp @dive_control_common
	:
	lda dog_y, Y
	cmp #32
	bcs :+
		lda #1
		sta dgd_MEDUSA_DIVE, Y
		jmp @dive_control_common
	:
	lda dgd_MEDUSA_TIME, Y
	bne :+
		jsr prng
		and #1
		sta dgd_MEDUSA_DIVE, Y
		@dive_control_common:
			jsr dog_time_medusa
			jmp @dive_control_end
		;
	:
	; else
		sec
		sbc #1
		sta dgd_MEDUSA_TIME, Y
	;
	@dive_control_end:
	; vertical motion
	lda dgd_MEDUSA_DIVE, Y
	bne @dive_down
	@dive_up:
		lda dgd_MEDUSA_VY1, Y
		sec
		sbc #<MEDUSA_ACCEL
		sta dgd_MEDUSA_VY1, Y
		lda dgd_MEDUSA_VY0, Y
		sbc #>MEDUSA_ACCEL
		sta dgd_MEDUSA_VY0, Y
		; clamp
		lda dgd_MEDUSA_VY1, Y
		cmp #<MEDUSA_MIN
		lda dgd_MEDUSA_VY0, Y
		sbc #>MEDUSA_MIN
		bvc :+
		eor #$80
		:
		bpl :+
			lda #<MEDUSA_MIN
			sta dgd_MEDUSA_VY1, Y
			lda #>MEDUSA_MIN
			sta dgd_MEDUSA_VY0, Y
		:
		jmp @dive_end
	@dive_down:
		lda dgd_MEDUSA_VY1, Y
		clc
		adc #<MEDUSA_ACCEL
		sta dgd_MEDUSA_VY1, Y
		lda dgd_MEDUSA_VY0, Y
		adc #>MEDUSA_ACCEL
		sta dgd_MEDUSA_VY0, Y
		; clamp
		lda dgd_MEDUSA_VY1, Y
		cmp #<MEDUSA_MAX
		lda dgd_MEDUSA_VY0, Y
		sbc #>MEDUSA_MAX
		bvc :+
		eor #$80
		:
		bmi :+
			lda #<MEDUSA_MAX
			sta dgd_MEDUSA_VY1, Y
			lda #>MEDUSA_MAX
			sta dgd_MEDUSA_VY0, Y
		:
		;jmp @dive_end
	@dive_end:
	lda dgd_MEDUSA_Y1, Y
	clc
	adc dgd_MEDUSA_VY1, Y
	sta dgd_MEDUSA_Y1, Y
	lda dog_y, Y
	adc dgd_MEDUSA_VY0, Y
	sta dog_y, Y
	; overlap
	jsr dog_bound_medusa
	jsr lizard_overlap_no_stone
	beq :+
		jsr lizard_die
	:
	rts
dog_draw_medusa:
	DX_SCROLL_EDGE
	stx lh ; temporary
	lda dgd_MEDUSA_SPAWN, Y
	beq :+
		rts
	:
	lda #DATA_sprite0_medusa
	pha
	lda dgd_MEDUSA_VY0, Y
	bpl :+
		lda dog_x, Y
		lsr
		lsr
		and #3
		tax
		pla
		lda medusa_flap_cycle, X
		pha
	:
	lda dgd_MEDUSA_FLIP, Y
	php
	ldx lh ; dx
	lda dog_y, Y
	tay
	plp
	bne :+
		pla
		jmp sprite0_add
	:
		pla
		jmp sprite0_add_flipped
	;

; DOG_PENGUIN
stat_dog_penguin:

dgd_PENGUIN_MODE = dog_data0
dgd_PENGUIN_ANIM = dog_data1
dgd_PENGUIN_FLIP = dog_data2

PENGUIN_MODE_TURN = 0
PENGUIN_MODE_WALK = 1
PENGUIN_MODE_FEAR = 2
PENGUIN_MODE_HATE = 3

.segment "DATA"
penguin_walk_sprite:
.byte DATA_sprite0_penguin_walk2
.byte DATA_sprite0_penguin_walk1
.byte DATA_sprite0_penguin_walk0
.byte DATA_sprite0_penguin_walk1

.segment "CODE"

dog_penguin_turn:
	jsr prng
	and #15
	clc
	adc #16
	sta dgd_PENGUIN_ANIM, Y
	lda #PENGUIN_MODE_TURN
	sta dgd_PENGUIN_MODE, Y
	rts

dog_penguin_fear:
	lda dgd_PENGUIN_MODE, Y
	cmp #PENGUIN_MODE_FEAR
	bne :+
		rts
	:
	cmp #PENGUIN_MODE_HATE
	bne :+
		lda dgd_PENGUIN_ANIM, Y
		and #15
		sta dgd_PENGUIN_ANIM, Y
		rts
	:
	lda #0
	sta dgd_PENGUIN_ANIM, Y
	lda #PENGUIN_MODE_FEAR
	sta dgd_PENGUIN_MODE, Y
	PLAY_SOUND_SCROLL SOUND_PENGUIN_FEAR
	rts

dog_penguin_blocker:
	DOG_BLOCKER -6, -14, 5, -1
	rts

dog_penguin_bound:
	DOG_BOUND -6, -14, 5, -1
	rts

dog_init_penguin:
	jsr prng
	and #1
	sta dgd_PENGUIN_FLIP, Y
	jsr dog_penguin_turn
	jmp dog_penguin_blocker
	;trs

dog_tick_penguin:
	; touch
	jsr dog_penguin_bound
	jsr lizard_touch
	beq @touch_end
		lda current_lizard
		cmp #LIZARD_OF_DEATH
		bne :+
			jsr empty_blocker
			ldx dgd_PENGUIN_FLIP, Y
			lda #DATA_sprite0_penguin_skull
			jmp bones_convert
			;rts
		:
		cmp #LIZARD_OF_HEAT
		bne :+
			jsr dog_penguin_fear
		:
	@touch_end:
	; push
	;jsr dog_penguin_bound
	jsr do_push
	beq @push_end
	cmp #1
	bne @push_right
	@push_left:
		ldx dog_now
		inc dog_x, X
		bne :+
			inc dog_xh, X
		:
		jmp @push_finish
	@push_right:
		ldx dog_now
		lda dog_x, Y
		bne :+
			dec dog_xh, X
		:
		dec dog_x, X
		;jmp @push_finish
	@push_finish:
		jsr dog_penguin_fear
		jsr dog_penguin_blocker
		jsr dog_penguin_bound
	@push_end:
	; fall
	;jsr dog_penguin_bound
	jsr do_fall
	beq @fall_end
		lda dog_y, Y
		cmp #255
		bne :+
			jsr empty_blocker
			jmp empty_dog
			;rts
		:
		ldx dog_now
		inc dog_y, X
		jsr dog_penguin_fear
		jsr dog_penguin_blocker
	@fall_end:
	; stand on penguin
	DOG_BOUND -6, -15, 5, -15
	jsr lizard_overlap
	beq :+
		jsr dog_penguin_fear
	:
	; modes
	lda dgd_PENGUIN_MODE, Y
	cmp #PENGUIN_MODE_TURN
	bne @mode_walk
		lda dgd_PENGUIN_ANIM, Y
		bne :+
			lda #PENGUIN_MODE_WALK
			sta dgd_PENGUIN_MODE, Y
			lda dgd_PENGUIN_FLIP, Y
			eor #1
			sta dgd_PENGUIN_FLIP, Y
			jmp @mode_walk_fallthrough
		:
		ldx dog_now
		dec dgd_PENGUIN_ANIM, X
		rts
	@mode_walk:
	cmp #PENGUIN_MODE_WALK
	jne @mode_fear
	@mode_walk_fallthrough:
		lda dgd_PENGUIN_ANIM, Y
		and #3
		jne  @walk_end
		lda dgd_PENGUIN_FLIP, Y
		bne @walk_right
		@walk_left:
			DOG_BOUND -7, -14, 5, -1
			jsr lizard_overlap
			bne @walk_turn
			lda dog_x, Y
			sec
			sbc #<7
			tax
			lda dog_xh, Y
			sbc #>7
			sta ih
			lda dog_y, Y
			tay
			jsr collide_all ; dgx-7, dgy
			beq @walk_turn
			dey
			jsr collide_all ; dgx-7, dgy-1
			bne @walk_turn
			tya
			sec
			sbc #(14-1)
			tay
			jsr collide_all ; dgx-7, dgy-14
			bne @walk_turn
			ldy dog_now
			lda dog_x, Y
			cmp #<9
			lda dog_xh, Y
			sbc #>9
			bcc @walk_turn
			; no turn, move
			ldx dog_now
			ldy dog_now
			lda dog_x, Y
			bne :+
				dec dog_xh, X
			:
			dec dog_x, X
			jmp @walk_finish
		@walk_turn:
			ldy dog_now
			jmp dog_penguin_turn
		@walk_right:
			DOG_BOUND -6, -14, 6, -1
			jsr lizard_overlap
			bne @walk_turn
			lda dog_x, Y
			clc
			adc #<6
			tax
			lda dog_xh, Y
			adc #>6
			sta ih
			lda dog_y, Y
			tay
			jsr collide_all ; dgx+6, dgy
			beq @walk_turn
			dey
			jsr collide_all ; dgx+6, dgy-1
			bne @walk_turn
			tya
			sec
			sbc #(14-1)
			tay
			jsr collide_all ; dgy+6, dgy-14
			bne @walk_turn
			ldy dog_now
			lda room_scrolling
			bne :+
				lda dog_x, Y
				cmp #<248
				lda dog_xh, Y
				sbc #>248
				bcs @walk_turn
				jmp :++
			:
				lda dog_x, Y
				cmp #<504
				lda dog_xh, Y
				sbc #>504
				bcs @walk_turn
			:
			; no turn, move
			ldx dog_now
			ldy dog_now
			inc dog_x, X
			bne :+
				inc dog_xh, X
			:
			;jmp @walk_finish
		@walk_finish:
			jsr dog_penguin_blocker
		@walk_end:
		ldx dog_now
		inc dgd_PENGUIN_ANIM, X
		rts
	@mode_fear:
	cmp #PENGUIN_MODE_FEAR
	bne @mode_hate
		ldx dog_now
		inc dgd_PENGUIN_ANIM, X
		lda dgd_PENGUIN_ANIM, Y
		cmp #60
		bcs :+
			rts
		:
		lda #0
		sta dgd_PENGUIN_ANIM, Y
		lda #PENGUIN_MODE_HATE
		sta dgd_PENGUIN_MODE, Y
		jmp @mode_hate_fallthrough
	@mode_hate:
	cmp #PENGUIN_MODE_HATE
	bne @mode_invalid
	@mode_hate_fallthrough:
		lda dgd_PENGUIN_ANIM, Y
		and #15
		bne @sound_end
			PLAY_SOUND_SCROLL SOUND_PENGUIN_HATE
		@sound_end:
		ldx dog_now
		inc dgd_PENGUIN_ANIM, X
		lda dgd_PENGUIN_ANIM, Y
		cmp #47
		bcc :+
			jmp dog_penguin_turn
			;rts
		:
		DOG_BOUND -3, -19, 2, -15
		jsr lizard_overlap_no_stone
		bne @kill
		DOG_BOUND -10, -15, 9, -11
		jsr lizard_overlap_no_stone
		bne @kill
		rts
		@kill:
			jmp lizard_die
			;rts
		;
	@mode_invalid:
		; invalid mode?
		jmp dog_penguin_turn
		;rts
	;

dog_draw_penguin:
	DX_SCROLL_EDGE
	stx m
	lda #DATA_sprite0_penguin
	sta s
	lda dgd_PENGUIN_FLIP, Y
	sta n
	; choose mode
	lda dgd_PENGUIN_MODE, Y
	cmp #PENGUIN_MODE_TURN
	bne @mode_walk
		lda dgd_PENGUIN_ANIM, Y
		cmp #4
		bcc @mode_end
		cmp #10
		bcs @mode_end
			lda #DATA_sprite0_penguin_blink
			sta s
		jmp @mode_end
	@mode_walk:
	cmp #PENGUIN_MODE_WALK
	bne @mode_fear
		lda dgd_PENGUIN_ANIM, Y
		lsr
		lsr
		lsr
		and #3
		tax
		lda penguin_walk_sprite, X
		sta s
		jmp @mode_end
	@mode_fear:
	cmp #PENGUIN_MODE_FEAR
	bne @mode_hate
		lda dgd_PENGUIN_ANIM, Y
		lsr
		lsr
		lsr
		and #1
		clc
		adc #DATA_sprite0_penguin_fear0
		sta s
		lda dgd_PENGUIN_ANIM, Y
		lsr
		lsr
		lsr
		lsr
		and #1
		eor n
		sta n ; flip ^= (dgd_PENGUIN_ANIM >> 4) & 1
		jmp @mode_end
	@mode_hate:
	cmp #PENGUIN_MODE_HATE
	bne @mode_end ; invalid mode?
		lda dgd_PENGUIN_ANIM, Y
		lsr
		lsr
		lsr
		and #1
		clc
		adc #DATA_sprite0_penguin_hate0
		sta s
		;jmp @mode_end
	@mode_end:
	lda dog_y, Y
	tay
	ldx m ; scroll_dx
	lda s ; sprite
	pha
	lda n ; flip
	bne :+
		pla
		jmp sprite0_add
	:
		pla
		jmp sprite0_add_flipped
	;rts

; DOG_MAGE
stat_dog_mage:

dgd_MAGE_ANIM  = dog_data0
dgd_MAGE_BLINK = dog_data1
dgd_MAGE_FLIP  = dog_data2

MAGE_WAKE_WIDTH = 100

dog_init_mage:
	jsr prng8
	sta dgd_MAGE_BLINK, Y
	jsr prng
	and #1
	sta dgd_MAGE_FLIP, Y
	rts
dog_tick_mage:
	; blink
	ldx dgd_MAGE_BLINK, Y
	bne :+
		jsr prng
		ora #$80
		jmp :++
	:
		dex
		txa
	:
	sta dgd_MAGE_BLINK, Y
	; waking
	ldx dgd_MAGE_ANIM, Y
	bne @awake
		lda current_lizard
		cmp #LIZARD_OF_STONE
		bne :+
			lda lizard_power
			bne @wake_false
		:
		lda lizard_x+0
		cmp dog_x, Y
		lda lizard_xh
		sbc dog_xh, Y
		bcs @wake_test_right
		@wake_test_left:
			lda dog_x, Y
			sec
			sbc #<MAGE_WAKE_WIDTH
			sta i
			lda dog_xh, Y
			sbc #>MAGE_WAKE_WIDTH
			sta ih
			bcc @wake_true
			lda lizard_x+0
			cmp i
			lda lizard_xh
			sbc ih
			bcs @wake_true
			jmp @wake_false
		@wake_test_right:
			lda lizard_x+0
			sec
			sbc dog_x, Y
			sta i
			lda lizard_xh
			sbc dog_xh, Y
			sta ih
			lda i
			cmp #<(MAGE_WAKE_WIDTH+1)
			lda ih
			sbc #>(MAGE_WAKE_WIDTH+1)
			bcs @wake_false
			;jmp @wake_true
		@wake_true:
			lda #1
			sta dgd_MAGE_ANIM, Y
		@wake_false:
			; do nothing
		rts
	;
	@awake:
		;ldx dgd_MAGE_ANIM, Y
		inx
		txa
		sta dgd_MAGE_ANIM, Y
		; facing
		cpx #32
		bcc :+
			ldx dog_now
			lda #0
			sta dgd_MAGE_FLIP, Y
			lda lizard_x+0
			cmp dog_x, Y
			lda lizard_xh
			sbc dog_xh, Y
			; carry set if lizard_px >= dgx
			rol dgd_MAGE_FLIP, X
		:
		; manage the ball
		ldx dgd_MAGE_ANIM, Y
		cpx #31
		bne @ball_create_end
			PLAY_SOUND SOUND_TWINKLE
			ldx dog_now
			inx
			inx
			lda dog_x, Y
			sta dog_x, X
			lda dog_xh, Y
			sta dog_xh, X
			lda dog_y, Y
			sec
			sbc #29
			sta dog_y, X
			lda #1
			sta dgd_MAGE_BALL_MODE, X
			jmp @ball_end
		@ball_create_end:
		cpx #64
		beq @ball_activate
		bcs @ball_end
		cpx #32
		bcc @ball_end
			; flicker fade in
			ldx dog_now
			inx
			inx
			lda #0
			sta dgd_MAGE_BALL_MODE, X
			jsr prng
			sta i
			lda dgd_MAGE_ANIM, Y
			asl
			asl
			asl
			asl
			asl
			ora #$1F
			cmp i
			; carry set if p >= prng
			rol dgd_MAGE_BALL_MODE, X
			jmp @ball_end
		@ball_activate:
			ldx dog_now
			inx
			inx
			lda #2
			sta dgd_MAGE_BALL_MODE, X
			;jmp @ball_end
		@ball_end:
		; loop in casting
		ldx dgd_MAGE_ANIM, Y
		cpx #80
		bcc :+
			ldx #64
			txa
			sta dgd_MAGE_ANIM, Y
		:
		; touch/death
		cpx #24
		bcs :+
			rts
		:
		jsr @bound
		jsr lizard_overlap_no_stone
		beq :+
			jsr lizard_die
		:
		jsr lizard_touch
		beq @touch_end
			ldx dog_now
			inx
			inx
			lda dgd_MAGE_BALL_MODE, X
			beq :+
				lda #5
				sta dgd_MAGE_BALL_MODE, X
			:
			lda current_lizard
			cmp #LIZARD_OF_DEATH
			bne :+
				ldx dgd_MAGE_FLIP, Y
				lda #DATA_sprite0_mage_skull
				jmp bones_convert
			:
			cmp #LIZARD_OF_HEAT
			bne :+
				lda #10
				sta dgd_MAGE_BLINK, Y
			:
		@touch_end:
		rts
	; auxiliary function
	@bound:
		DOG_BOUND -4,-15,3,-1
		rts
	;

.segment "DATA"
mage_frames:
.byte DATA_sprite0_mage_wake0
.byte DATA_sprite0_mage_wake1
.byte DATA_sprite0_mage_wake2
.byte DATA_sprite0_mage_cast0
.byte DATA_sprite0_mage_cast1
.byte DATA_sprite0_mage_cast0
.byte DATA_sprite0_mage_cast1
.byte DATA_sprite0_mage_cast0
.byte DATA_sprite0_mage_cast1
.byte DATA_sprite0_mage_cast0

.segment "CODE"

dog_draw_mage:
	DX_SCROLL_EDGE
	stx i ; dx
	lda dgd_MAGE_ANIM, Y
	lsr
	lsr
	lsr
	tax
	lda mage_frames, X
	sta j ; sprite
	lda dgd_MAGE_BLINK, Y
	cmp #11
	bcs @blink_end
		lda j
		cmp #DATA_sprite0_mage_cast0
		bne :+
			lda #DATA_sprite0_mage_blink0
			sta j
			jmp @blink_end
		:
		cmp #DATA_sprite0_mage_cast1
		bne :+
			lda #DATA_sprite0_mage_blink1
			sta j
			;jmp @blink_end
		:
	@blink_end:
	lda dgd_MAGE_FLIP, Y
	bne :+
		lda dog_y, Y
		tay
		ldx i
		lda j
		jmp sprite0_add
	:
		lda dog_y, Y
		tay
		ldx i
		lda j
		jmp sprite0_add_flipped
	;

; DOG_MAGE_BALL
stat_dog_mage_ball:

;dgd_MAGE_BALL_MODE  = dog_data0
dgd_MAGE_BALL_ANIM  = dog_data1
dgd_MAGE_BALL_FRAME = dog_data2
dgd_MAGE_BALL_X1    = dog_data3
dgd_MAGE_BALL_Y1    = dog_data4
dgd_MAGE_BALL_VX0   = dog_data5
dgd_MAGE_BALL_VX1   = dog_data6
dgd_MAGE_BALL_VY0   = dog_data7
dgd_MAGE_BALL_VY1   = dog_data8
dgd_MAGE_BALL_TY    = dog_data9

MAGE_BALL_ACCEL = 23
MAGE_BALL_CLAMP = 27 * MAGE_BALL_ACCEL

dog_init_mage_ball:
	rts
dog_tick_mage_ball:
	lda dgd_MAGE_BALL_MODE, Y
	bne :+
		rts
	:
	ldx dog_now
	inc dgd_MAGE_BALL_ANIM, X
	lda dgd_MAGE_BALL_ANIM, Y
	cmp #5
	bcc @spin_end
		lda #0
		sta dgd_MAGE_BALL_ANIM, Y
		inc dgd_MAGE_BALL_FRAME, X
		lda dgd_MAGE_BALL_FRAME, X
		cmp #3
		bcc :+
			lda #0
			sta dgd_MAGE_BALL_FRAME, Y
		:
	@spin_end:
	lda dgd_MAGE_BALL_MODE, Y
	cmp #2
	bcs :+
		rts
	:
	; moving
	; x += vx
	lda dgd_MAGE_BALL_VX1, Y
	clc
	adc dgd_MAGE_BALL_X1, Y
	sta dgd_MAGE_BALL_X1, Y
	lda dgd_MAGE_BALL_VX0, Y
	; VX is 16 bit sign-extended to 24 bits
	bmi :+
		adc dog_x, Y
		sta dog_x, Y
		lda #0
		adc dog_xh, Y
		jmp :++
	:
		adc dog_x, Y
		sta dog_x, Y
		lda #-1
		adc dog_xh, Y
	:
	sta ih ; ih = nx high
	; y += vy
	lda dgd_MAGE_BALL_VY1, Y
	clc
	adc dgd_MAGE_BALL_Y1, Y
	sta dgd_MAGE_BALL_Y1, Y
	lda dgd_MAGE_BALL_VY0, Y
	adc dog_y, Y
	sta jh ; jh = nx high
	; bounds
	lda #0
	sta k ; bound
	lda ih
	beq @bound_x_end
	bpl :+
		lda #0
		sta ih
		sta dog_x, Y
		sta dgd_MAGE_BALL_X1, Y
		lda #1
		sta k
		jmp @bound_x_end
	:
	cmp #$02
	bcc :+
		lda #$01
		sta ih
		lda #$FF
		sta dog_x, Y
		sta dgd_MAGE_BALL_X1, Y
		lda #1
		sta k
		jmp @bound_x_end
	:
	lda room_scrolling
	bne :+
		lda #$00
		sta ih
		lda #$FF
		sta dog_x, Y
		sta dgd_MAGE_BALL_X1, Y
		lda #1
		sta k
		;jmp @bound_x_end
	:
	@bound_x_end:
	lda jh
	cmp #252
	bcc :+
		lda #0
		sta jh
		sta dgd_MAGE_BALL_Y1, Y
		lda #1
		sta k
		jmp @bound_y_end
	:
	cmp #244
	bcc :+
		lda #243
		sta jh
		lda #$FF
		sta dgd_MAGE_BALL_Y1, Y
		lda #1
		sta k
	:
	@bound_y_end:
	; if bound && BALL_MODE >= 5 ball should die
	lda k
	beq :+
	lda dgd_MAGE_BALL_MODE, Y
	cmp #5
	bcc :+
		lda #0
		sta dgd_MAGE_BALL_MODE, Y
		rts
	:
	lda ih
	sta dog_xh, Y
	lda jh
	sta dog_y, Y
	; hurt player
	DOG_BOUND -3,-2,3,4
	jsr lizard_overlap_no_stone
	beq :+
		jsr lizard_die
	:
	; steering
	lda #0
	sta u ; dx
	sta v ; dy
	lda lizard_dead
	bne @steer_escape
	lda dgd_MAGE_BALL_MODE, Y
	cmp #5
	bcc @steer_seek
	@steer_escape:
		lda #5
		sta dgd_MAGE_BALL_MODE, Y
		lda dgd_MAGE_BALL_VX0, Y
		bmi :+
			lda #1
			sta u
		:
		lda dgd_MAGE_BALL_VY0, Y
		bmi :+
			lda #1
			sta v
		:
		jmp @steer_finish
	@steer_seek:
		lda lizard_x+0
		sta i
		lda lizard_xh
		sta ih
		lda lizard_y
		sec
		sbc #7
		sta j
		; ih:i = tx, j = ty
		ldx dgd_MAGE_BALL_MODE, Y
		cpx #3
		bcc :+
			lda dgd_MAGE_BALL_TY, Y
			sta j
			cpx #4
			bcc :++
				lda #128
				sta i
				lda #0
				sta ih
			:
		:
		; target
		lda dog_x, Y
		cmp i
		lda dog_xh, Y
		sbc ih
		bcs :+
			lda #1
			sta u
		:
		lda dog_y, Y
		cmp j
		bcs :+
			lda #1
			sta v
		:
	@steer_finish:
	; dx
	@steer_horizontal:
	lda u
	beq @steer_left
	@steer_right:
		lda dgd_MAGE_BALL_VX1, Y
		clc
		adc #<MAGE_BALL_ACCEL
		sta dgd_MAGE_BALL_VX1, Y
		lda dgd_MAGE_BALL_VX0, Y
		adc #>MAGE_BALL_ACCEL
		sta dgd_MAGE_BALL_VX0, Y
		; clamp
		lda dgd_MAGE_BALL_VX1, Y
		cmp #<MAGE_BALL_CLAMP
		lda dgd_MAGE_BALL_VX0, Y
		sbc #>MAGE_BALL_CLAMP
		bvc :+
		eor #$80
		:
		bmi :+
			lda #<MAGE_BALL_CLAMP
			sta dgd_MAGE_BALL_VX1, Y
			lda #>MAGE_BALL_CLAMP
			sta dgd_MAGE_BALL_VX0, Y
		:
		jmp @steer_vertical
	@steer_left:
		lda dgd_MAGE_BALL_VX1, Y
		sec
		sbc #<MAGE_BALL_ACCEL
		sta dgd_MAGE_BALL_VX1, Y
		lda dgd_MAGE_BALL_VX0, Y
		sbc #>MAGE_BALL_ACCEL
		sta dgd_MAGE_BALL_VX0, Y
		; clamp
		lda dgd_MAGE_BALL_VX1, Y
		cmp #<-MAGE_BALL_CLAMP
		lda dgd_MAGE_BALL_VX0, Y
		sbc #>-MAGE_BALL_CLAMP
		bvc :+
		eor #$80
		:
		bpl :+
			lda #<-MAGE_BALL_CLAMP
			sta dgd_MAGE_BALL_VX1, Y
			lda #>-MAGE_BALL_CLAMP
			sta dgd_MAGE_BALL_VX0, Y
		:
		;jmp @steer_vertical
	@steer_vertical:
	lda v
	beq @steer_up
	@steer_down:
		lda dgd_MAGE_BALL_VY1, Y
		clc
		adc #<MAGE_BALL_ACCEL
		sta dgd_MAGE_BALL_VY1, Y
		lda dgd_MAGE_BALL_VY0, Y
		adc #>MAGE_BALL_ACCEL
		sta dgd_MAGE_BALL_VY0, Y
		; clamp
		lda dgd_MAGE_BALL_VY1, Y
		cmp #<MAGE_BALL_CLAMP
		lda dgd_MAGE_BALL_VY0, Y
		sbc #>MAGE_BALL_CLAMP
		bvc :+
		eor #$80
		:
		bmi :+
			lda #<MAGE_BALL_CLAMP
			sta dgd_MAGE_BALL_VY1, Y
			lda #>MAGE_BALL_CLAMP
			sta dgd_MAGE_BALL_VY0, Y
		:
		jmp @steer_end
	@steer_up:
		lda dgd_MAGE_BALL_VY1, Y
		sec
		sbc #<MAGE_BALL_ACCEL
		sta dgd_MAGE_BALL_VY1, Y
		lda dgd_MAGE_BALL_VY0, Y
		sbc #>MAGE_BALL_ACCEL
		sta dgd_MAGE_BALL_VY0, Y
		; clamp
		lda dgd_MAGE_BALL_VY1, Y
		cmp #<-MAGE_BALL_CLAMP
		lda dgd_MAGE_BALL_VY0, Y
		sbc #>-MAGE_BALL_CLAMP
		bvc :+
		eor #$80
		:
		bpl :+
			lda #<-MAGE_BALL_CLAMP
			sta dgd_MAGE_BALL_VY1, Y
			lda #>-MAGE_BALL_CLAMP
			sta dgd_MAGE_BALL_VY0, Y
		:
		;jmp @steer_end
	@steer_end:
	rts
dog_draw_mage_ball:
	lda dgd_MAGE_BALL_MODE, Y
	bne :+
		rts
	:
	lda #-4
	DX_SCROLL_OFFSET
	tya
	and #1
	ora #$2
	sta i ; attribute = 0x2 | (d & 1)
	lda dgd_MAGE_BALL_FRAME, Y
	clc
	adc #$8B
	pha
	lda dog_y, Y
	sec
	sbc #4
	tay ; Y = dgy-4
	pla ; A = 0x8B + MAGE_BALL_FRAME
	; X = dx
	jmp sprite_tile_add

; DOG_GHOST
stat_dog_ghost:

dgd_GHOST_ANIM  = dog_data0
dgd_GHOST_MODE  = dog_data1
dgd_GHOST_VX0   = dog_data2
dgd_GHOST_VX1   = dog_data3
dgd_GHOST_VY0   = dog_data4
dgd_GHOST_VY1   = dog_data5
dgd_GHOST_X1    = dog_data6
dgd_GHOST_Y1    = dog_data7
dgd_GHOST_FRAME = dog_data8
dgd_GHOST_TICK  = dog_data9
dgd_GHOST_FLOAT = dog_data10
dgd_GHOST_FLIP  = dog_data11
dgd_GHOST_BOSS  = dog_data12

dog_init_ghost:
	jsr prng8
	sta dgd_GHOST_ANIM, Y
	rts
dog_tick_ghost:
	lda dgd_GHOST_MODE, Y
	jne @wait_end
		lda dgd_GHOST_ANIM, Y
		beq :++
			lda dgd_GHOST_BOSS, Y
			bne :+
				ldx dog_now
				dec dgd_GHOST_ANIM, X
			:
			rts
		:
		lda #1
		sta dgd_GHOST_MODE, Y
		PLAY_SOUND SOUND_TWINKLE
		lda dgd_GHOST_BOSS, Y
		bne @boss_dir
		; random vertical position
		jsr prng3
		and #127
		clc
		adc #56
		sta dog_y, Y
		; random direction
		jsr prng
		and #1
		sta u
		lda lizard_x+0
		cmp #<128
		lda lizard_xh
		sbc #>128
		bcs :+
			lda #0
			sta u
			jmp @apply_dir
		:
		lda room_scrolling
		beq :+
		lda lizard_x+0
		cmp #<(256+128)
		lda lizard_xh
		sbc #>(256+128)
		bcc :++
		:
			lda #1
			sta u
			;jmp @apply_dir
		:
		@apply_dir:
		lda u
		sta dgd_GHOST_FLIP, Y
		bne :+
			lda lizard_x+0
			clc
			adc #<116
			sta dog_x, Y
			lda lizard_xh
			adc #>116
			sta dog_xh, Y
			jmp :++
		:
			lda lizard_x+0
			sec
			sbc #<116
			sta dog_x, Y
			lda lizard_xh
			sbc #>116
			sta dog_xh, Y
		:
		jmp @wait_end
		@boss_dir:
			lda dog_x, Y
			cmp lizard_x+0
			lda dog_xh, Y
			sbc lizard_xh
			lda #0
			rol
			sta dgd_GHOST_FLIP, Y
			;jmp @wait_end
		;
	@wait_end:
	; animate / float
	lda dgd_GHOST_FLOAT, Y
	clc
	adc #4
	sta dgd_GHOST_FLOAT, Y
	lda dgd_GHOST_TICK, Y
	clc
	adc #1
	cmp #6
	bcc :+
		lda dgd_GHOST_FRAME, Y
		clc
		adc #1
		and #3
		sta dgd_GHOST_FRAME, Y
		lda #0
	:
	sta dgd_GHOST_TICK, Y
	; fade in
	lda dgd_GHOST_MODE, Y
	cmp #1
	bne @fade_in_end
		lda dgd_GHOST_ANIM, Y
		clc
		adc #1
		sta dgd_GHOST_ANIM, Y
		cmp #64
		bcs :+
			rts
		:
		lda #3
		sta dgd_GHOST_MODE, Y
		lda #0
		sta dgd_GHOST_ANIM, Y
		PLAY_SOUND SOUND_BOO
		lda dog_xh, Y
		sta ih
		lda dog_x, Y
		asl
		rol ih
		asl
		rol ih
		sta i ; ih:i = dgx << 2
		lda #0
		sta jh
		sta lh ; convenience for below
		lda dog_y, Y
		asl
		rol jh
		asl
		rol jh
		sta j ; jh:j = dgy << 2
		; dy16 = ty16 - sy16
		;lda #0
		;sta lh ; done above already
		lda lizard_y+0
		sec
		sbc #8
		asl
		rol lh
		asl
		rol lh
		sec
		sbc j
		sta dgd_GHOST_VY1, Y
		lda lh
		sbc jh
		sta dgd_GHOST_VY0, Y
		; dx16 = tx16 - sx16
		lda lizard_xh
		sta kh
		lda lizard_x+0
		asl
		rol kh
		asl
		rol kh ; kh:A = lizard_px << 2
		sec
		sbc i
		sta dgd_GHOST_VX1, Y
		lda kh
		sbc ih
		sta dgd_GHOST_VX0, Y
		; set flip
		rol
		rol
		and #1
		eor #1
		sta dgd_GHOST_FLIP, Y
	@fade_in_end:
	; move
	lda dgd_GHOST_X1, Y
	clc
	adc dgd_GHOST_VX1, Y
	sta dgd_GHOST_X1, Y
	lda dog_x, Y
	adc dgd_GHOST_VX0, Y
	sta dog_x, Y
	lda dgd_GHOST_VX0, Y
	bmi :+
		lda #0
		jmp :++
	:
		lda #-1
	:
	adc dog_xh, Y
	sta dog_xh, Y
	lda dgd_GHOST_Y1, Y
	clc
	adc dgd_GHOST_VY1, Y
	sta dgd_GHOST_Y1, Y
	lda dog_y, Y
	adc dgd_GHOST_VY0, Y
	sta dog_y, Y
	; stop at edges
	cmp #248
	jcs @wait
	lda room_scrolling
	bne :+
		lda dog_xh, Y
		bne @wait
		jmp :++
	:
		lda dog_xh, Y
		cmp #2
		bcs @wait
	:
	; fade out
	lda dgd_GHOST_MODE, Y
	cmp #2
	bne @fade_out_end
		lda dgd_GHOST_ANIM, Y
		beq @wait
		sec
		sbc #1
		sta dgd_GHOST_ANIM, Y
		rts
	@fade_out_end:
	; hit
	cmp #3
	beq :+
		rts ; shouldn't reach this
	:
	lda dgd_GHOST_ANIM, Y
	clc
	adc #1
	sta dgd_GHOST_ANIM, Y
	cmp #96
	bcc :+
		lda #64
		sta dgd_GHOST_ANIM, Y
		lda #2
		sta dgd_GHOST_MODE, Y
		rts
	:
	DOG_BOUND -6,-6,5,5
	ldx dgd_GHOST_FLOAT, Y
	lda circle4, X
	pha
	clc
	adc j
	sta j
	pla
	clc
	adc l
	sta l
	jsr lizard_overlap_no_stone
	beq :+
		jsr lizard_die
	:
	rts
@wait:
	jsr prng3
	and #127
	eor #128
	sta dgd_GHOST_ANIM, Y
	lda #0
	sta dgd_GHOST_MODE, Y
	rts
dog_draw_ghost:
	lda dgd_GHOST_MODE, Y
	bne :+
		rts
	:
	cmp #3
	bcs :+
		; fading
		jsr prng
		and #63
		cmp dgd_GHOST_ANIM, Y
		bcc :+
		rts
	:
	DX_SCROLL_EDGE
	stx u ; u = dx
	ldx dgd_GHOST_FLOAT, Y
	lda circle4, X
	clc
	adc dog_y, Y
	sta v ; v = dgy + circle4[GHOST_FLOAT]
	lda #DATA_sprite0_ghost0
	clc
	adc dgd_GHOST_FRAME, Y
	sta s
	lda dgd_GHOST_FLIP, Y
	php
	lda s
	ldx u
	ldy v
	plp
	bne :+
		jmp sprite0_add
	:
		jmp sprite0_add_flipped
	;
	rts

; DOG_PIGGY
stat_dog_piggy:

dgd_PIGGY_MODE      = dog_data0
dgd_PIGGY_COIN_Y    = dog_data1
dgd_PIGGY_BLINK     = dog_data2
dgd_PIGGY_NUM_TIME  = dog_data3
dgd_PIGGY_FADE      = dog_data4
dgd_PIGGY_COUNT     = dog_data5
dgd_PIGGY_TEXT      = dog_data6

;.assert (PIGGY_BURST = DATA_COIN_COUNT - 3), error, "PIGGY_BURST is supposed to be 3 less than all coins."


.segment "DATA"
piggy_text_list:
.byte TEXT_PIGGY0, TEXT_PIGGY1, TEXT_PIGGY2, TEXT_PIGGY3, TEXT_PIGGY4
piggy_text_range:
.byte 0, 25, 50, 75, 100

.segment "CODE"

; clobbers Y
dog_piggy_spawn_beyond_star:
	lda current_lizard
	cmp #LIZARD_OF_BEYOND
	beq @spawn_end
		ldy #0
		@drip_loop:
			lda dog_type, Y
			cmp #DOG_DRIP
			bne :+
				jsr empty_dog
			:
			iny
			cpy #16
			bcc @drip_loop
		lda #7
		ldx #DATA_palette_lizard10
		jsr palette_load
		lda #DOG_BEYOND_STAR
		sta dog_type + BEYOND_STAR_SLOT
		ldy #BEYOND_STAR_SLOT
		jsr dog_init_banked_D
	@spawn_end:
	rts

dog_piggy_range:
	ldx dgd_PIGGY_TEXT, Y
	cpx #4
	bcc :+
		rts
	:
	@loop:
		inx
		lda piggy_bank
		cmp piggy_text_range, X
		bcc @done
		txa
		sta dgd_PIGGY_TEXT, Y
		cmp #4
		bcs @done
		jmp @loop
	@done:
	rts

dog_init_piggy:
	lda piggy_bank
	cmp #PIGGY_BURST
	bcc :+
		jsr empty_dog
		jmp dog_piggy_spawn_beyond_star
		;rts
	:
	jsr coin_count
	ldy dog_now
	sta dgd_PIGGY_COUNT, Y
	DOG_BLOCKER -8,-13,10,-1
	jsr prng
	and #127
	sta dgd_PIGGY_BLINK, Y
	lda #127
	sta dgd_PIGGY_FADE, Y
	jsr dog_piggy_range
	; setup counter
	jsr decimal_clear
	lda piggy_bank
	jsr decimal_add
	rts
dog_tick_piggy:
	; blink, counter timer
	ldx dog_now
	inc dgd_PIGGY_BLINK, X
	lda dgd_PIGGY_BLINK, X
	cmp #159
	bcc :+
		jsr prng
		and #31
		sta dgd_PIGGY_BLINK, X
	:
	lda dgd_PIGGY_NUM_TIME, X
	beq :+
		inc dgd_PIGGY_NUM_TIME, X
	:
	; fade mode
	lda dgd_PIGGY_MODE, Y
	cmp #2
	bne @no_fade
		lda dgd_PIGGY_FADE, Y
		bne @fade_started
		@fade_start:
			PLAY_SOUND SOUND_TWINKLE
			jsr dog_piggy_spawn_beyond_star
			lda #128
			sta dgd_BEYOND_STAR_FADE + BEYOND_STAR_SLOT
			; kill particles
			lda #DOG_NONE
			sta dog_type+6
			sta dog_type+7
			sta dog_type+8
			; done
			ldy dog_now
			jmp empty_dog
			;rts
		@fade_started:
			ldx dog_now
			dec dgd_PIGGY_FADE, X
			rts
		;
	@no_fade:
	; death / heat
	DOG_BOUND -8,-13,10,-1
	jsr lizard_touch
	beq @touch_end
		lda current_lizard
		cmp #LIZARD_OF_DEATH
		bne :+
			jsr empty_blocker
			lda #DATA_sprite0_piggy_skull
			ldx #0
			jmp bones_convert
			;rts
		:
		cmp #LIZARD_OF_HEAT
		bne :+
			lda #153
			sta dgd_PIGGY_BLINK, Y
		:
	@touch_end:
	; talk
	lda lizard_dead
	bne :+
	lda lizard_power
	beq :+
	lda current_lizard
	cmp #LIZARD_OF_KNOWLEDGE
	bne :+
	DOG_BOUND -40,-10,32,-1
	jsr lizard_overlap
	beq :+
	ldx dgd_PIGGY_TEXT, Y
	cpx #5
	bcs :+
	lda piggy_bank
	cmp piggy_text_range, X
	bcc :+
		jsr dog_piggy_range
		ldx dgd_PIGGY_TEXT, Y
		lda piggy_text_list, X
		sta game_message
		inx
		txa
		sta dgd_PIGGY_TEXT, Y
		lda #1
		sta game_pause
		rts
	:
	; coin mode
	lda dgd_PIGGY_MODE, Y
	cmp #1
	bne @mode_0
	@mode_1:
		lda dgd_PIGGY_COIN_Y, Y
		clc
		adc #7
		sta dgd_PIGGY_COIN_Y, Y
		lda dog_y, Y
		sec
		sbc #12
		cmp dgd_PIGGY_COIN_Y, Y
		beq :+
		bcc :+
			rts
		:
		inc piggy_bank
		lda #0
		sta dgd_PIGGY_MODE, Y
		sta coin_saved
		lda #1
		jsr decimal_add
		PLAY_SOUND SOUND_COIN
		rts
	@mode_0:
		lda piggy_bank
		cmp #PIGGY_BURST
		bcc @no_burst
			; spawn particles
			ldy #6
			:
				lda #DOG_PARTICLE
				sta dog_type, Y
				jsr dog_init_banked_D
				iny
				cpy #9
				bcc :-
			; fade
			ldy dog_now
			lda #2
			sta dgd_PIGGY_MODE, Y
			jsr empty_blocker
			lda #128
			sta dgd_PIGGY_FADE, Y
			rts
		@no_burst:
		DOG_BOUND -34,-29,36,-1
		jsr lizard_overlap
		beq @overlap_end
			lda piggy_bank
			cmp dgd_PIGGY_COUNT, Y
			bcs :+
				lda #1
				sta dgd_PIGGY_MODE, Y
				sta dgd_PIGGY_COIN_Y, Y
			:
			lda #128
			sta dgd_PIGGY_NUM_TIME, Y
		@overlap_end:
		rts
	;

dog_draw_piggy_body:
	; randomized fadeout
	lda dgd_PIGGY_MODE, Y
	cmp #2
	bne @fade_end
		jsr prng4
		and #127
		cmp dgd_PIGGY_FADE, Y
		bcc :+
			rts
		:
	@fade_end:
	;
	lda dgd_PIGGY_BLINK, Y
	cmp #153
	bcc :+
		lda #DATA_sprite0_piggy_blink
		jmp :++
	:
		lda #DATA_sprite0_piggy_stand
	:
	pha
	lda dog_y, Y
	tay
	pla
	jsr sprite0_add
	ldy dog_now
	rts
dog_draw_piggy_coin:
	lda dog_y, Y
	pha
	lda dgd_PIGGY_COIN_Y, Y
	sta dog_y, Y
	jsr dog_draw_coin
	pla
	ldy dog_now
	sta dog_y, Y
	rts
dog_draw_piggy_number:
	; Y = dog_now
	; m = dx
	; n = offset
	; o = number
	lda n ; n = offset
	clc
	adc m ; m = dx
	tax
	lda dog_y, Y
	sec
	sbc #31
	tay
	lda #$01
	sta i ; attribute
	lda o ; o = num
	clc
	adc #$A0
	jsr sprite_tile_add_clip
	ldy dog_now
	rts
dog_draw_piggy:
	DX_SCROLL_EDGE
	stx m ; m = dx
	; draw body
	jsr dog_draw_piggy_body
	; draw coin
	lda dgd_PIGGY_MODE, Y
	cmp #1
	bne :+
		ldx m
		jsr dog_draw_piggy_coin
	:
	; draw number
	lda dgd_PIGGY_NUM_TIME, Y
	bne :+
		rts
	:
	lda decimal+2
	beq @digits_two
@digits_three:
	sta o ; num
	lda #-11
	sta n ; offset
	jsr dog_draw_piggy_number
	lda decimal+1
	sta o
	lda #-3
	sta n
	jsr dog_draw_piggy_number
	lda decimal+0
	sta o
	lda #5
	sta n
	jmp dog_draw_piggy_number
	;rts
@digits_two:
	lda decimal+1
	beq @digits_one
	sta o
	lda #-7
	sta n
	jsr dog_draw_piggy_number
	lda decimal+0
	sta o
	lda #1
	sta n
	jmp dog_draw_piggy_number
	;rts
@digits_one:
	lda decimal+0
	sta o
	lda #-3
	sta n
	jmp dog_draw_piggy_number
	;rts

; fire common
stat_dog_fire_common:

dgd_FIRE_PAL  = dog_data10
dgd_FIRE_ANIM = dog_data11

dog_tick_fire_common:
	lda dgd_FIRE_ANIM, Y
	beq :+
		sec
		sbc #1
		sta dgd_FIRE_ANIM, Y
		rts
	:
	lda #5
	sta dgd_FIRE_ANIM, Y
	lda dgd_FIRE_PAL, Y
	clc
	adc #1
	cmp #3
	bcc :+
		lda #0
	:
	sta dgd_FIRE_PAL, Y
	clc
	adc #DATA_palette_lava0
	tax
	tya
	and #1
	ora #6 ; palette index = 6 | (dog & 1)
	jsr palette_load
	ldy dog_now ; restore y index
	rts

; DOG_PANDA_FIRE
stat_dog_panda_fire:
dog_init_panda_fire = dog_init_panda
dog_tick_panda_fire:
	jsr dog_tick_fire_common
	DOG_BOUND -8,-28,7,-16
	jsr lizard_overlap_no_stone
	beq :+
		jsr lizard_die
	:
	jmp dog_tick_panda
dog_draw_panda_fire = dog_draw_panda

; DOG_GOAT_FIRE
stat_dog_goat_fire:

dog_init_goat_fire = dog_init_goat
dog_tick_goat_fire:
	jsr dog_tick_fire_common
	jmp dog_tick_goat
dog_draw_goat_fire = dog_draw_goat

; DOG_DOG_FIRE
stat_dog_dog_fire:

dog_init_dog_fire = dog_init_dog
dog_tick_dog_fire:
	jsr dog_tick_fire_common
	jmp dog_tick_dog
dog_draw_dog_fire = dog_draw_dog

; DOG_OWL_FIRE
stat_dog_owl_fire:

dog_init_owl_fire = dog_init_owl
dog_tick_owl_fire:
	jsr dog_tick_fire_common
	jmp dog_tick_owl
dog_draw_owl_fire = dog_draw_owl

; DOG_MEDUSA_FIRE
stat_dog_medusa_fire:

dog_init_medusa_fire = dog_init_medusa
dog_tick_medusa_fire:
	jsr dog_tick_fire_common
	jmp dog_tick_medusa
dog_draw_medusa_fire = dog_draw_medusa

; DOG_ARROW_LEFT
stat_dog_arrow_left:

dgd_ARROW_ANIM  = dog_data0
dgd_ARROW_FLASH = dog_data1

dog_tick_arrow_common_wait:
	ldx dog_now
	dec dgd_ARROW_ANIM, X
	bne :+
		lda dgd_ARROW_FLASH, Y
		sta dgd_ARROW_ANIM, Y
	:
	lda dog_y, Y
	clc
	adc #(4+14)
	cmp lizard_y+0 ; if dgy+4 >= lizard_y-14
	bcc @inactive
	clc
	adc #(((3+1)-(4+14))-1)
	cmp lizard_y+0 ; if dgy+3 <= lizard_y-1
	bcs @inactive
	lda dog_type, Y
	cmp #DOG_ARROW_LEFT
	bne @test_right
	@test_left:
		lda dog_x, Y
		clc
		adc #>8
		sta i
		lda dog_xh, Y
		adc #<8
		sta ih
		lda i
		cmp lizard_x+0
		lda ih
		sbc lizard_xh
		bcs @active
		jmp @inactive
	@test_right:
		lda dog_x, Y
		cmp lizard_x+0
		lda dog_xh, Y
		sbc lizard_xh
		bcs @inactive
		;jmp @active
	@active:
		lda #1
		sta dog_param, Y
		PLAY_SOUND SOUND_ARROW_FIRE
		sec
		rts
	;
	@inactive:
		clc
		rts
	;
dog_tick_arrow_common_collide:
	DOG_POINT_Y 3
	jsr collide_tile
	bne :+
		ldy dog_now
		clc
		rts
	:
	ldy dog_now
	lda dog_type, Y
	cmp #DOG_ARROW_LEFT
	bne :+
		lda #8
		jsr move_finish_x
	:
	lda #DATA_sprite0_arrow_bone
	ldx #0 ; flip
	jsr bones_convert_silent
	PLAY_SOUND SOUND_ARROW_HIT
	sec
	rts
dog_tick_arrow_common_lizard:
	DOG_BOUND 0,3,7,4
	jsr lizard_overlap_no_stone
	beq :+
		jmp lizard_die
	:
	rts
	;
dog_draw_arrow_common:
	DX_SCROLL
	lda dog_param, Y
	bne @flying
		lda dgd_ARROW_ANIM, Y
		cmp #20
		bcs @normal
		cmp #10
		bcs :+
			lda i
			ora #$80 ; animate flash
			sta i
		:
		lda #$EE ; flash
		jmp @draw
	@normal:
		lda #$E8
		jmp @draw
	@flying:
		lda #$E9
		;jmp @draw
	@draw:
	pha ; temp (sprite)
	; X = dx
	lda dog_y, Y
	tay
	dey
	pla
	jmp sprite_tile_add

dog_init_arrow_left:
	lda #0
	sta dog_param, Y
	jsr prng8
	and #63
	clc
	adc #1
	sta dgd_ARROW_ANIM, Y
	jsr prng4
	and #15
	clc
	adc #170
	sta dgd_ARROW_FLASH, Y
	rts
dog_tick_arrow_left:
	lda dog_param, Y
	bne :+
		jsr dog_tick_arrow_common_wait
		bcs :++ ; activated this frame, skip move
		rts
	:
		; move
		lda #-8
		jsr move_finish_x
	:
	DOG_POINT_X -1
	jsr dog_tick_arrow_common_collide
	bcc :+
		lda #1
		sta dgd_BONES_FLIP, Y
		rts
	:
	jmp dog_tick_arrow_common_lizard
dog_draw_arrow_left:
	lda #$43
	sta i ; attrib
	jmp dog_draw_arrow_common

; DOG_ARROW_RIGHT
stat_dog_arrow_right:

dog_init_arrow_right = dog_init_arrow_left
dog_tick_arrow_right:
	lda dog_param, Y
	bne :+
		jsr dog_tick_arrow_common_wait
		bcs :++ ; activated this frame, skip move
		rts
	:
		; move
		lda #8
		jsr move_finish_x
	:
	DOG_POINT_X 8
	jsr dog_tick_arrow_common_collide
	bcc :+
		rts
	:
	jmp dog_tick_arrow_common_lizard
dog_draw_arrow_right:
	lda #$03
	sta i ; attrib
	jmp dog_draw_arrow_common

; DOG_SAW
stat_dog_saw:

dgd_SAW_ANIM  = dog_data0
dgd_SAW_FRAME = dog_data1

dog_init_saw:
	jsr prng3
	and #7
	sta dgd_SAW_ANIM, Y
	jsr prng2
	sta dgd_SAW_FRAME, Y
	rts
dog_tick_saw:
	DOG_BOUND -8,-3,7,-1
	jsr lizard_overlap
	beq @overlap_end
		lda lizard_power
		beq :+
		lda current_lizard
		cmp #LIZARD_OF_STONE
		bne :+
			rts ; clogged the gears
		:
		jsr lizard_die
	@overlap_end:
	ldx dog_now
	inc dgd_SAW_ANIM, X
	lda dgd_SAW_ANIM, X
	cmp #6
	bcc :+
		lda #0
		sta dgd_SAW_ANIM, X
		inc dgd_SAW_FRAME, X
	:
	rts
dog_draw_saw:
	DX_SCROLL_EDGE
	lda dgd_SAW_FRAME, Y
	and #3
	clc
	adc #DATA_sprite0_saw0
	sta s
	tya
	and #1
	php
	lda dog_y, Y
	tay
	lda s
	plp
	beq :+
		jmp sprite0_add
	:
		jmp sprite0_add_flipped
	;rts

; DOG_STEAM
stat_dog_steam:

dgd_STEAM_ANIM  = dog_data0
dgd_STEAM_FRAME = dog_data1
dgd_STEAM_FADE  = dog_data2
dgd_STEAM_MODE  = dog_data3
dgd_STEAM_TICK  = dog_data4

.segment "DATA"

steam_sprite:
.byte 0,                          0,                          0,                          0
.byte DATA_sprite0_steam_pre0,     DATA_sprite0_steam_pre1,     DATA_sprite0_steam_pre2,     DATA_sprite0_steam_pre3
.byte DATA_sprite0_steam_low0,     DATA_sprite0_steam_low1,     DATA_sprite0_steam_low2,     DATA_sprite0_steam_low3
.byte DATA_sprite0_steam_mid0,     DATA_sprite0_steam_mid1,     DATA_sprite0_steam_mid2,     DATA_sprite0_steam_mid3
.byte DATA_sprite0_steam_high0,    DATA_sprite0_steam_high1,    DATA_sprite0_steam_high2,    DATA_sprite0_steam_high3
.byte DATA_sprite0_steam_release0, DATA_sprite0_steam_release1, DATA_sprite0_steam_release2, DATA_sprite0_steam_release3
.byte DATA_sprite0_steam_out0,     DATA_sprite0_steam_out1,     DATA_sprite0_steam_out2,     DATA_sprite0_steam_out3
.byte DATA_sprite0_steam_off0,     DATA_sprite0_steam_off1,     DATA_sprite0_steam_off2,     DATA_sprite0_steam_off3

steam_off0: .byte   0,   0, -15, -23, -31, -31, -31,   0
steam_off1: .byte   0,   0,  -1,  -1,  -1,  -9, -17,   0
steam_time: .byte   0,  60,   6,   6,  60,   6,   6,   6

.segment "CODE"

dog_init_steam:
	:
		jsr prng3
		and #7
		cmp #6
		bcs :-
	sta dgd_STEAM_ANIM, Y
	jsr prng3
	sta dgd_STEAM_FRAME, Y
	:
		jsr prng2
		and #3
		cmp #3
		bcs :-
	sta dgd_STEAM_FADE, Y
	:
		jsr prng8
		cmp dog_param, Y
		bcc :+
		jmp :-
	:
	sta dgd_STEAM_TICK, Y
	rts

dog_tick_steam:
	ldx dog_now ; for inc
	; constant animation
	inc dgd_STEAM_ANIM, X
	lda dgd_STEAM_ANIM, X
	cmp #6
	bcc :+
		lda #0
		sta dgd_STEAM_ANIM, X
		inc dgd_STEAM_FRAME, X
	:
	; tick
	inc dgd_STEAM_TICK, X
	lda dgd_STEAM_MODE, X
	bne :+
		lda dgd_STEAM_TICK, X
		cmp dog_param, X
		bcc @tick_done
		jmp @tick_mode
	:
	lda dgd_STEAM_TICK, X
	ldx dgd_STEAM_MODE, Y
	cmp steam_time, X
	bcc @tick_done
	@tick_mode:
		lda #0
		sta dgd_STEAM_TICK, Y
		lda dgd_STEAM_MODE, Y
		clc
		adc #1
		and #7
		sta dgd_STEAM_MODE, Y
		cmp #2
		bne @tick_done
			PLAY_SOUND_SCROLL SOUND_STEAM
		;
	@tick_done:
	; hitboxes
	ldx dgd_STEAM_MODE, Y
	lda steam_off0, X
	bne :+
		rts
	:
	pha
	lda steam_off1, X
	pha
	DOG_BOUND -5,0,4,0
	pla
	clc
	adc l
	sta l ; l += steam_off1, X
	pla
	clc
	adc j
	sta j ; j += steam_off0, X
	jsr lizard_overlap_no_stone
	beq :+
		jsr lizard_die
	:
	rts

dog_draw_steam:
	ldx dog_now
	inc dgd_STEAM_FADE, X
	lda dgd_STEAM_FADE, X
	cmp #3
	bcc :+
		lda #0
		sta dgd_STEAM_FADE, X
		rts
	:
	lda dgd_STEAM_MODE, Y
	bne :+
		rts
	:
	DX_SCROLL_EDGE
	txa
	pha
	lda dgd_STEAM_MODE, Y
	asl
	asl
	sta s
	lda dgd_STEAM_FRAME, Y
	and #3
	ora s
	tax
	lda steam_sprite, X
	sta s
	pla
	tax
	lda dog_y, Y
	tay
	lda s
	jmp sprite0_add
	;rts

; DOG_SPARKD
stat_dog_sparkd:

dgd_SPARK_LEN = dog_data0

dog_init_spark:
	lda dog_param, Y
	and #7
	clc
	adc #1
	asl
	asl
	asl
	sta dgd_SPARK_LEN, Y
	lda dog_param, Y
	and #$38
	sta dog_param, Y
	rts

dog_hit_spark:
	; m:n = x-offset
	; o = y-offset
	lda dog_param, Y
	cmp dgd_SPARK_LEN, Y
	bcc :+
		rts
	:
	DOG_BOUND 2,2,5,5
	lda i
	clc
	adc n
	sta i
	lda ih
	adc m
	sta ih
	lda j
	clc
	adc o
	sta j
	lda k
	clc
	adc n
	sta k
	lda kh
	adc m
	sta kh
	lda l
	clc
	adc o
	sta l
	; bound now adjusted by offsets m:n, o
	jsr lizard_overlap_no_stone
	beq :+
		jsr lizard_die
	:
	rts

dog_draw_spark:
	; m:n = x-offset
	; o = y-offset
	lda dog_param, Y
	cmp dgd_SPARK_LEN, Y
	bcc :+
		rts
	:
	lda dog_x, Y
	clc
	adc n
	sta i
	lda dog_xh, Y
	adc m
	sta ih ; ih:i = dog_x + m:n offset
	lda i
	sec
	sbc scroll_x+0
	sta i
	lda ih
	sbc scroll_x+1
	bcs :+
		rts ; sdx < scroll
	:
	beq :+
		rts ; (sdx - scroll) >= 256
	:
	lda dog_y, Y
	clc
	adc o
	tay
	dey ; y = dy-1
	ldx i ; x = sdx - scroll
	lda #$23
	sta i ; attribute (background)
	lda nmi_count
	asl
	asl
	and #$30
	ora #$46 ; tile
	jmp sprite_tile_add

dog_tick_spark:
	lda dog_param, Y
	clc
	adc #1
	and #63
	sta dog_param, Y
	rts

dog_offset_sparkd:
	sta o ; o = dog_param
	lda #0
	sta m
	sta n ; m:n = 0
	rts

dog_init_sparkd = dog_init_spark
dog_tick_sparkd:
	jsr dog_tick_spark
	; A = dog_param, Y
	jsr dog_offset_sparkd
	jmp dog_hit_spark
dog_draw_sparkd:
	lda dog_param, Y
	jsr dog_offset_sparkd
	jmp dog_draw_spark

; DOG_SPARKU
stat_dog_sparku:

dog_offset_sparku:
	lda #0
	sta m
	sta n
	sec
	sbc dog_param, Y
	sta o
	rts

dog_init_sparku = dog_init_spark
dog_tick_sparku:
	jsr dog_tick_spark
	jsr dog_offset_sparku
	jmp dog_hit_spark
dog_draw_sparku:
	jsr dog_offset_sparku
	jmp dog_draw_spark

; DOG_SPARKL
stat_dog_sparkl:

dog_offset_sparkl:
	lda #0
	sta o
	sec
	sbc dog_param, Y
	sta n
	beq :+
		lda #-1
		sta m
		rts
	:
		sta m
		rts
	;

dog_init_sparkl = dog_init_spark
dog_tick_sparkl:
	jsr dog_tick_spark
	jsr dog_offset_sparkl
	jmp dog_hit_spark
dog_draw_sparkl:
	jsr dog_offset_sparkl
	jmp dog_draw_spark

; DOG_SPARKR
stat_dog_sparkr:

dog_offset_sparkr:
	sta n
	lda #0
	sta m
	sta o
	rts

dog_init_sparkr = dog_init_spark
dog_tick_sparkr:
	jsr dog_tick_spark
	jsr dog_offset_sparkr
	jmp dog_hit_spark
dog_draw_sparkr:
	lda dog_param, Y
	jsr dog_offset_sparkr
	jmp dog_draw_spark

; DOG_FROB_FLY
stat_dog_frob_fly:

dog_init_frob_fly:
dog_tick_frob_fly:
dog_draw_frob_fly:
	; NOT IN DEMO
	rts

; ===========
; jump tables
; ===========
.segment "DATA"
DOG_TABLE0 = 1
DOG_TABLE1 = 0
DOG_TABLE2 = 0
DOG_TABLE_LEN = DOG_BANK1_START - DOG_BANK0_START
.include "dogs_tables.s"

.segment "CODE"

; Y = dog, assume all regs clobbered
dog_init_jump:
	ldx dog_type, Y
	cpx #DOG_BANK1_START
	bcs :+
	lda dog_init_table_high, X
	pha
	lda dog_init_table_low, X
	pha
	rts
:
	cpx #DOG_BANK2_START
	bcs :+
	jmp dog_init_D
:
	jmp dog_init_F

; Y = dog, assume all regs clobbered
dog_tick_jump:
	ldx dog_type, Y
	cpx #DOG_BANK1_START
	bcs :+
	lda dog_tick_table_high, X
	pha
	lda dog_tick_table_low, X
	pha
	rts
:
	cpx #DOG_BANK2_START
	bcs :+
	jmp dog_tick_D
:
	jmp dog_tick_F

; Y = dog, assume all regs clobbered
dog_draw_jump:
	ldx dog_type, Y
	cpx #DOG_BANK1_START
	bcs :+
	lda dog_draw_table_high, X
	pha
	lda dog_draw_table_low, X
	pha
	rts
:
	cpx #DOG_BANK2_START
	bcs :+
	jmp dog_draw_D
:
	jmp dog_draw_F

;
; some dog code is moved to bank D for space,
; see dogs1.s for the implementations
;

dog_init_D:
	ldx #253
	lda #$D
	jmp bank_call

dog_tick_D:
	ldx #254
	lda #$D
	jmp bank_call

dog_draw_D:
	ldx #255
	lda #$D
	jmp bank_call

; for when one dog needs to initialize another dog in bank D, dog_now also has to be replaced:
; preserved: Y, dog_now
; clobbered: A, X, whatever the init does
dog_init_banked_D:
	tya
	pha
	lda dog_now
	pha
	sty dog_now
	ldx #253
	lda #$D
	jsr bank_call
	pla
	sta dog_now
	pla
	tay
	rts

;
; other dog code is moved to bank F for space,
; see dogs2.s for the implementations
;

dog_init_F:
	ldx #248
	lda #$F
	jmp bank_call

dog_tick_F:
	ldx #249
	lda #$F
	jmp bank_call

dog_draw_F:
	ldx #250
	lda #$F
	jmp bank_call

; ============
; public stuff
; ============
stat_dog_public:

dogs_init:
	; clear dogs data
	lda #0
	ldx #0
	:
		sta dog_data0, X
		sta dog_data1, X
		sta dog_data2, X
		sta dog_data3, X
		sta dog_data4, X
		sta dog_data5, X
		sta dog_data6, X
		sta dog_data7, X
		sta dog_data8, X
		sta dog_data9, X
		sta dog_data10, X
		sta dog_data11, X
		sta dog_data12, X
		sta dog_data13, X
		inx
		cpx #16
		bcc :-
	; call init functions
	ldy #0
	:
		sty dog_now
		tya
		pha
		jsr dog_init_jump
		pla
		tay
		iny
		cpy #16
		bcc :-
	rts

dogs_tick:
	; set up cycling order
	jsr dogs_cycle
	; tick the dogs
	lda #0
	:
		sta dog_now
		pha
		tay
		jsr dog_tick_jump
		pla
		; prevent multi-door conflicts by this early-out if a door has been activated
		ldx room_change
		bne :+
		; advance to next dog
		clc
		adc dog_add
		and #15
		bne :-
	:
	; wrap to room size
	lda room_scrolling
	bne @wrap_2s
	@wrap_1s:
		lda #0
		ldy #0
		:
			sta dog_xh, Y
			iny
			cpy #16
			bcc :-
		rts
	@wrap_2s:
		ldy #0
		:
			lda dog_xh, Y
			and #1
			sta dog_xh, Y
			iny
			cpy #16
			bcc :-
		rts
	;

dogs_draw:
	; draw the dogs
	lda #0
	:
		sta dog_now
		pha
		tay
		jsr dog_draw_jump
		pla
		clc
		adc dog_add
		and #15
		bne :-
	rts

stat_dog_end_of_file:
; end of file
