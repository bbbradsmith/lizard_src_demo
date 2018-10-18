; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; dogs_common
;

.feature force_range
.macpack longbranch

.include "ram.s"
.include "enums.s"
.include "dogs_inc.s"

.include "../assets/export/names.s"

.import bank_call

; from draw.s
.import sprite_tile_add
.import palette_load

.segment "CODE"

; ======
; macros
; ======

; DOG_BOUND x0,y0,x1,y1 -> sets up ih:i,j,kh:k,l as bounding box (Y = dog index, clobbers A)
; DOG_BLOCKER x0,y0,x1,y1 -> updates blocker bounding box (Y = dog index, clobbers A,X)

; DOG_POINT_X offset -> sets up ih:X + offset (clobbers A,X)
; DOG_POINT_Y offset -> sets up Y + offset (clobbers A,Y
; DOG_POINT x,y -> sets up ih:X and Y (clobbers A,X,Y)

; FIXED_BOUND sets up bounding box without reference to dog (clobbers A)

; DOG_CMP_X px -> 16-bit dog_x/xh - px, result in carry flag (NOT EQ) (Y = dog index, clobbers A)

; DX_SCROLL -> for drawing with X position X = dog_x - scroll for scroll, rts from enclosing function if offscreen (Y = dog index, clobbers A, result in X)
; DX_SCROLL_OFFSET -> like DX_SCROLL but adds A to dog_x (clobbers A,i,ih)


; PPU_LATCH addr -> directly latches the PPU (clobbers A)
; PPU_LATCH_AT ax,ay -> directly latches PPU by nametable tile coordinates (ax = 0-31, ay = 0-29 or add 32 for second nmt) (clobbers A)
; NMI_UPDATE_AT ax,ay -> same calculation as PPU_LATCH_AT but result is stored in nmi_load instead (clobbers A)

.segment "CODE"

; parameter: sign flag
; result in A
load_sign:
	bmi :+
		lda #0
		rts
	:
		lda #-1
		rts
	;

dx_screen_:
	lda dog_x, Y
	tax
	sta sprite_center
	lsr
	eor sprite_center
	and #$C0
	sta sprite_center
	rts

dx_scroll_:
	lda dog_x, Y
	sec
	sbc scroll_x+0
	tax
	lda dog_xh, Y
	sbc scroll_x+1
	bne :+
		txa
		sta sprite_center
		lsr
		eor sprite_center
		and #$C0
		sta sprite_center
		lda #0
	:
	rts

dx_scroll_offset_:
	sta ih ; temporary
	clc
	adc dog_x, Y
	sta i
	bit ih
	jsr load_sign
	adc dog_xh, Y
	sta ih
	lda i
	sec
	sbc scroll_x+0
	tax
	lda ih
	sbc scroll_x+1
	bne :+
		txa
		sta sprite_center
		lsr
		eor sprite_center
		and #$C0
		sta sprite_center
		lda #0
	:
	rts

; duplicated in dogs2.s
dx_scroll_edge_:
	lda dog_x, Y
	sec
	sbc scroll_x+0
	tax
	lda dog_xh, Y
	sbc scroll_x+1
dx_scroll_edge_common_:
	bne :+
		txa
		sta sprite_center
		lsr
		eor sprite_center
		and #$C0
		sta sprite_center
		lda #0
		rts
	:
	bpl @scrolled_off_right
	@scrolled_off_left:
		txa
		and #$C0
		cmp #$C0
		beq @scrolled_off_near_edge
		;lda #1
		rts
	@scrolled_off_right:
		txa
		and #$C0
		;cmp #$00
		beq @scrolled_off_near_edge
		lda #1
		rts
	@scrolled_off_near_edge:
		eor #$80
		and #$80
		sta sprite_center
		lda #0
		rts
	;

; =============
; dog_draw_coin
; =============

.segment "DATA"
coin_cycle_tile: .byte $E2, $E3, $E4, $E5, $E6, $E5, $E4, $E3
coin_cycle_att:  .byte $03, $03, $03, $43, $03, $03, $43, $43

.segment "CODE"

;dgd_COIN_ON   = dog_data0
;dgd_COIN_ANIM = dog_data1

dog_draw_coin:
	DX_SCROLL
	stx j ; temporarily store X
	lda dgd_COIN_ON, Y
	bne :+
		rts
	:
	lda dgd_COIN_ANIM, Y
	lsr
	lsr
	lsr
	and #7
	tax
	lda coin_cycle_att, X
	sta i
	lda coin_cycle_tile, X
	pha
	ldx j
	lda dog_y, Y
	tay
	dey
	pla
	jsr sprite_tile_add
	rts

; =====
; bones
; =====

.segment "CODE"

; changes to bones, plays sound, increments bones metric
; A = sprite, X = flip, Y = dog index
bones_convert:
	jsr bones_convert_silent
	PLAY_SOUND SOUND_BONES
	jmp inc_bones

; changes to bones, no sound, no metric increment
bones_convert_silent:
	sta dog_param, Y
	txa
	sta dgd_BONES_FLIP, Y
	lda #0
	sta dgd_BONES_INIT, Y
	lda #DOG_BONES
	sta dog_type, Y
	jmp dog_tick_bones

; ========
; movement
; ========

.segment "CODE"

; fall helper
;
; ih:i,j,kh:k,l = bounding box
; result in A/z
; restores ih:i,j,kh:k,l
; clobbers n,o,p (collide_all)
do_fall:
	; store bounding box, Y
	tya
	pha
	lda l
	pha
	lda k
	pha
	lda kh
	pha
	lda j
	pha
	lda i
	pha
	lda ih
	pha
	; shift bounding box down by 1
	inc j
	inc l
	; test for bottom of screen
	lda l
	cmp #240
	bcs @yes_fall
	; test for lizard overlap
	jsr lizard_overlap
	bne @no_fall
	; test corners and midpoint for collision below
	ldx i
	ldy l
	jsr collide_all
	bne @no_fall
	lda ih
	sta jh ; temporary
	lda kh
	sta ih
	ldx k
	jsr collide_all
	bne @no_fall
	lda i
	clc
	adc k
	sta i
	lda jh ; ih argument
	adc kh
	lsr
	sta ih
	ror i ; ih:i = mix_x = (x0+x1 >> 1)
	ldx i
	jsr collide_all
	bne @no_fall
@yes_fall:
	pla
	sta ih
	pla
	sta i
	pla
	sta j
	pla
	sta kh
	pla
	sta k
	pla
	sta l
	pla
	tay
	lda #1
	rts
@no_fall:
	pla
	sta ih
	pla
	sta i
	pla
	sta j
	pla
	sta kh
	pla
	sta k
	pla
	sta l
	pla
	tay
	lda #0
	rts

; push helper
;
; ih:i,j,kh:k,l = bounding box
; result in A/z -> 0=no,1=right,2=left
; restores ih:i,j,kh:k,l
; clobbers n,o,p (collide_all)
do_push:
	; first the prerequisites
	lda current_lizard
	cmp #LIZARD_OF_PUSH
	bne :+
	lda lizard_power
	beq :+
	jmp :++
	:
		lda #0
		rts
	:
	; store bounding box, Y
	tya
	pha
	lda l
	pha
	lda k
	pha
	lda kh
	pha
	lda j
	pha
	lda i
	pha
	lda ih
	pha
	lda gamepad
	and #PAD_R
	jeq @no_r
		lda room_scrolling
		bne :+
			lda k
			cmp #<255
			lda kh
			sbc #>255
			bcs @no_push
			jmp :++
		:
			lda k
			cmp #<511
			lda kh
			sbc #>511
			bcs @no_push
		:
		lda k
		sta jh
		lda kh
		sta lh ; lh:jh = x1
		lda i
		sec
		sbc #1
		sta i
		sta k
		lda ih
		sbc #0
		sta ih
		sta kh
		; ih:i = x0-1
		;    j = y0
		; kh:k = x0-1
		;    l = y1
		jsr lizard_overlap
		beq @no_push
		lda jh
		clc
		adc #1
		tax
		lda lh
		adc #0
		sta ih ; ih:X = dgx+x1+1
		ldy j
		jsr collide_all
		bne @no_push
		ldy l
		jsr collide_all
		bne @no_push
		lda j
		lsr
		sta j
		lda l
		lsr
		clc
		adc j
		tay ; mid_y = y0>>1 + y1>>1
		jsr collide_all
		bne @no_push
			PLAY_SOUND SOUND_PUSH
			pla
			sta ih
			pla
			sta i
			pla
			sta j
			pla
			sta kh
			pla
			sta k
			pla
			sta l
			pla
			tay
			lda #1 ; 1 = push right
			rts
		;
	;
	@no_push: ; return state, in middle for closer branches
		pla
		sta ih
		pla
		sta i
		pla
		sta j
		pla
		sta kh
		pla
		sta k
		pla
		sta l
		pla
		tay
		lda #0
		rts
	@no_r:
	lda gamepad
	and #PAD_L
	beq @no_push
		lda i
		cmp #<2
		lda ih
		sbc #>2
		bcc @no_push
		lda i
		sta jh
		lda ih
		sta lh ; lh:jh = x0
		lda k
		clc
		adc #1
		sta i
		sta k
		lda kh
		adc #0
		sta ih
		sta kh
		; i = x1+1
		; j = y0
		; k = x1+1
		; l = y1
		jsr lizard_overlap
		beq @no_push
		lda jh
		sec
		sbc #1
		tax
		lda lh
		sbc #0
		sta ih ; ih:X = dgx+x0-1
		ldy j
		jsr collide_all
		bne @no_push
		ldy l
		jsr collide_all
		bne @no_push
		lda j
		lsr
		sta j
		lda l
		lsr
		clc
		adc j
		tay ; mid_y = y0>>1 + y1>>1
		jsr collide_all
		bne @no_push
			PLAY_SOUND SOUND_PUSH
			pla
			sta ih
			pla
			sta i
			pla
			sta j
			pla
			sta kh
			pla
			sta k
			pla
			sta l
			pla
			tay
			lda #2 ; 2 = push left
			rts
		;
	;

; movement helper
; ih:i,j,kh:k,l = bounding box
; u,v = dx/dy
;
; clobbers: A,X,Y, i,ih,jh,j n,o,p, q,r,s,t, u,v,w
; returns: u,v new dx/dy
;          A = %0000udlr collision happened
;          A also returned in w, and z flags
;          Y = dog_now
move_dog:
	; w is collision flag
	lda #0
	sta w
	; copy i,j bounds into q,r (collide_all will clobber i,j)
	lda i
	sta q ; q = x0
	lda j
	sta r ; r = y0
	; copy ih into jh (temporary)
	lda ih
	sta jh
	;    kh:k = x1
	;       l = y1
	;       u = dx
	;       v = dy
	lda u
	jeq @h_end
	bmi @h_negative
@h_positive:
	lda k
	clc
	adc u
	tax
	lda kh
	adc #0
	sta ih ; ih:X = x1+dx
	ldy r ;     Y = y0
	jsr @move_dog_right_handler
	;        ih:X = x1+dx
	ldy l ;     Y = y1
	jsr @move_dog_right_handler
	;           Y = y1
	tya
	lsr
	sta i
	lda r
	lsr
	clc
	adc i
	tay ; Y = (y1>>0)+(y0>>0)
	jsr @move_dog_right_handler
	jmp @h_end
	;
	@move_dog_right_handler:
		jsr collide_all_right
		beq @move_dog_right_handler_end
			lda u
			sec
			sbc j ; dx -= shift
			sta u ; u = dx (new)
			bmi :+
				clc
				adc k
				tax
				lda kh
				adc #0
				jmp :++
			:
				clc
				adc k
				tax
				lda kh
				adc #-1
			:
			sta ih ; ih:X = x1+dx (new)
			lda #1
			ora w
			sta w ; w |= 1 (collide right)
		@move_dog_right_handler_end:
		rts
	;
@h_negative:
	;    jh:q = x0
	;       r = y0
	;    kh:k = x1
	;       l = y1
	;       u = dx
	;       v = dy
	lda q
	clc
	adc u
	tax
	lda jh
	adc #-1
	sta ih ; ih:X = x0+dx
	ldy r ;     Y = y0
	jsr @move_dog_left_handler
	;        ih:X = x0+dx
	ldy l ;     Y = y1
	jsr @move_dog_left_handler
	;           Y = y1
	tya
	lsr
	sta i
	lda r
	lsr
	clc
	adc i
	tay ;   Y = (y1>>0)+(y0>>0)
	jsr @move_dog_left_handler
	jmp @h_end
	;
	@move_dog_left_handler:
		jsr collide_all_left
		beq @move_dog_left_handler_end
			clc
			adc u ; dx += shift
			sta u ; u = dx (new)
			bmi :+
				clc
				adc q
				tax
				lda jh
				adc #0
				jmp :++
			:
				clc
				adc q
				tax
				lda jh
				adc #-1
			:
			sta ih ; ih:X = x0+dx (new)
			lda #2
			ora w
			sta w ; w |= 2 (collide left)
		@move_dog_left_handler_end:
		rts
	;
@h_end:
	; x0 += dx, x1 += dx
	lda u
	bmi :+
		; u >= 0
		clc
		adc q
		sta q
		lda jh
		adc #0
		sta jh
		lda u
		clc
		adc k
		sta k
		lda kh
		adc #0
		sta kh
		jmp :++
	:
		; u < 0
		clc
		adc q
		sta q
		lda jh
		adc #-1
		sta jh
		lda u
		clc
		adc k
		sta k
		lda kh
		adc #-1
		sta kh
	:
	;    jh:q = x0 (new)
	;       r = y0
	;    kh:k = x1 (new)
	;       l = y1
	;       u = dx (new)
	;       v = dy
	lda v
	jeq @v_end
	bmi @v_negative
@v_positive:
	lda l
	clc
	adc v
	tay ;       Y = y1+dy
	ldx q
	lda jh
	sta ih ; ih:X = x0
	jsr @move_dog_down_handler
	;           Y = y1+dy
	ldx k
	lda kh
	sta ih ; ih:X = x1
	jsr @move_dog_down_handler
	;           Y = y1+dy
	lda k
	clc
	adc q
	sta i ; temp
	lda kh
	adc jh
	lsr
	sta ih
	ror i
	ldx i ;  ix:X = (x0+x1)>>1
	jsr @move_dog_down_handler
	jmp @v_end
	;
	@move_dog_down_handler:
		jsr collide_all_down
		beq :+
			lda v
			sec
			sbc j ; dy -= shift
			sta v ; v = dy (new)
			clc
			adc l
			tay ;   Y = y1+dy (new)
			lda #4
			ora w
			sta w ; w |= 4 (collide down)
		:
		rts
	;
@v_negative:
	lda r
	clc
	adc v
	tay ;       Y = y0+dy
	ldx q
	lda jh
	sta ih ; ih:X = x0
	jsr @move_dog_up_handler
	;           Y = y0+dy
	ldx k
	lda kh
	sta ih ; ih:X = x1
	jsr @move_dog_up_handler
	;           Y = y0+dy
	lda k
	clc
	adc q
	sta i ; temp
	lda kh
	adc jh
	lsr
	sta ih
	ror i
	ldx i ;  ix:X = (x0+x1)>>1
	jsr @move_dog_up_handler
	jmp @v_end
	;
	@move_dog_up_handler:
		jsr collide_all_up
		beq :+
			clc
			adc v ; dy += shift
			sta v ; v = dy (new)
			clc
			adc r
			tay ;   Y = y0+dy (new)
			lda #8
			ora w
			sta w ; w |= 8 (collide up)
		:
		rts
	;
@v_end:
	; restore Y, load result and return
	ldy dog_now
	lda w
	rts

; u,v = move dx/dy (signed)
; Y = dog
; clobbers: A
move_finish:
	lda u
	jsr move_finish_x
	lda v
	clc
	adc dog_y, Y
	sta dog_y, Y
	rts

; A = move dx (signed)
; Y = dog
; clobbers A
move_finish_x:
	cmp #0
move_finish_x_:
	bmi :+
		clc
		adc dog_x, Y
		sta dog_x, Y
		lda dog_xh, Y
		adc #0
		sta dog_xh, Y
		rts
	:
		clc
		adc dog_x, Y
		sta dog_x, Y
		lda dog_xh, Y
		adc #-1
		sta dog_xh, Y
		rts

; A = move dy (signed)
; Y = dog
; clobbers A
move_finish_y:
	clc
	adc dog_y, Y
	sta dog_y, Y
	rts

; =======
; Utility
; =======

; ih:i,j,kh:k,l = x0,y0,x1,y1
; clobbers A, flags
lizard_overlap:
	lda ih
	bmi @skip_x0
	lda lizard_hitbox_x1
	cmp i
	lda lizard_hitbox_xh1
	sbc ih
	bcc @no_overlap
@skip_x0:
	lda lizard_hitbox_y1
	cmp j
	bcc @no_overlap
	lda kh
	bmi @skip_x1
	lda k
	cmp lizard_hitbox_x0
	lda kh
	sbc lizard_hitbox_xh0
	bcc @no_overlap
@return_x1:
	lda l
	cmp lizard_hitbox_y0
	bcc @no_overlap
	lda lizard_dead
	bne @no_overlap
@overlap:
	lda #1
	rts
@no_overlap:
	lda #0
	rts
@skip_x1:
	lda #0
	cmp lizard_hitbox_x0
	lda #0
	sbc lizard_hitbox_xh0
	bcc @no_overlap
	jmp @return_x1

; ih:i,j,kh:k,l = x0,y0,x1,y1
; clobbers A, flags
lizard_touch:
	lda lizard_power
	beq @no_touch
	lda ih
	bmi @skip_x0
	lda lizard_touch_x1
	cmp i
	lda lizard_touch_xh1
	sbc ih
	bcc @no_touch
@skip_x0:
	lda lizard_touch_y1
	cmp j
	bcc @no_touch
	lda kh
	bmi @skip_x1
	lda k
	cmp lizard_touch_x0
	lda kh
	sbc lizard_touch_xh0
	bcc @no_touch
@return_x1:
	lda l
	cmp lizard_touch_y0
	bcc @no_touch
@touch:
	lda #1
	rts
@no_touch:
	lda #0
	rts
@skip_x1:
	lda #0
	cmp lizard_touch_x0
	lda #0
	sbc lizard_touch_xh0
	bcc @no_touch
	jmp @return_x1

lizard_overlap_no_stone:
	lda current_lizard
	cmp #LIZARD_OF_STONE
	bne @no_stone
	lda lizard_power
	bne @stone
@no_stone:
	jmp lizard_overlap
@stone:
	lda #0
	rts

lizard_touch_death:
	lda current_lizard
	cmp #LIZARD_OF_DEATH
	bne @no_death
	jmp lizard_touch
@no_death:
	lda #0
	rts

empty_dog:
	; duplicated in extra_a.s
	lda #DOG_NONE
	sta dog_type, Y
	rts

; DOG_BONES
stat_dog_bones_common:

;dgd_BONES_INIT = dog_data0
;dgd_BONES_FLIP = dog_data1
dgd_BONES_VX0 = dog_data2
dgd_BONES_VX1 = dog_data3
dgd_BONES_VY0 = dog_data4
dgd_BONES_VY1 = dog_data5
dgd_BONES_X1  = dog_data6
dgd_BONES_Y1  = dog_data7

dog_init_bones:
	lda #255
	sta dgd_BONES_INIT, Y
	rts
dog_tick_bones:
	lda dgd_BONES_INIT, Y
	bne @init_nonzero
		; set upward velocity
		lda #>(-800)
		sta dgd_BONES_VY0, Y
		lda #<(-800)
		sta dgd_BONES_VY1, Y
		; set X velocity
		jsr prng
		asl
		sta dgd_BONES_VX1, Y
		lda #0
		rol
		sta dgd_BONES_VX0, Y ; BONES_VX = prng() << 1
		jsr prng
		and #1
		beq :+
			; negate X velocity
			lda dgd_BONES_VX0, Y
			eor #$FF
			sta dgd_BONES_VX0, Y
			lda dgd_BONES_VX1, Y
			eor #$FF
			clc
			adc #1
			sta dgd_BONES_VX1, Y
			lda dgd_BONES_VX0, Y
			adc #0
			sta dgd_BONES_VX0, Y
		:
		lda #1
		sta dgd_BONES_INIT, Y
		rts
	@init_nonzero:
	cmp #255
	bne :+ ; if init == 255, do nothing
		rts
	:
	; main update
	; BONES_X += BONES_VX
	lda dgd_BONES_X1, Y
	clc
	adc dgd_BONES_VX1, Y
	sta dgd_BONES_X1, Y
	lda dog_x, Y
	adc dgd_BONES_VX0, Y
	sta dog_x, Y
	php
	lda dgd_BONES_VX0, Y
	bmi :+
		lda dog_xh, Y
		plp
		adc #0
		jmp :++
	:
		lda dog_xh, Y
		plp
		adc #-1
	:
	sta dog_xh, Y
	; kill if wrapped
	lda room_scrolling
	bne :+
		lda dog_xh, Y
		cmp #1
		bcs @kill_bones
		jmp :++
	:
		lda dog_xh, Y
		cmp #2
		bcs @kill_bones
	:
	; calculate projected Y
	lda dgd_BONES_Y1, Y
	clc
	adc dgd_BONES_VY1, Y
	sta j
	lda dog_y, Y
	adc dgd_BONES_VY0, Y
	sta i
	; if VY < 0 || ij >= Y
	lda dgd_BONES_VY0, Y
	bmi :+
	lda i
	cmp dog_y, Y
	bcs :+
	@kill_bones:
		; VY > 0 && ij < Y, time to kill bones
		lda  #255
		sta dgd_BONES_INIT, Y
		rts
	:
	lda i
	sta dog_y, Y
	lda j
	sta dgd_BONES_Y1, Y
	; add gravity
	lda dgd_BONES_VY1, Y
	clc
	adc #<(100)
	sta dgd_BONES_VY1, Y
	lda dgd_BONES_VY0, Y
	adc #>(100)
	sta dgd_BONES_VY0, Y
	rts
;dog_draw_bones is in dogs0.s

; DOG_BOSS_DOOR
stat_dog_boss_door_common:

;dgd_BOSS_DOOR_ANIM   = dog_data1
;dgd_BOSS_DOOR_ROOM   = dog_data2
;dgd_BOSS_DOOR_WOBBLE = dog_data3

; preserves Y but does not rely on it
dog_init_boss_door_palette:
	tya
	pha
	lda #6
	ldx #DATA_palette_boss_door
	jsr palette_load
	pla
	tay
	jsr prng
	and #$10
	sta s
	jsr prng4
	and #31
	sta r
	ldx #25
	@palette_loop:
		lda palette, X
		and #$0F
		clc
		adc r
		:
			cmp #$D
			bcc :+
			sec
			sbc #$C
			jmp :-
		:
		sta t
		lda palette, X
		and #$F0
		ora t
		clc
		adc s
		sta palette, X
		inx
		cpx #27
		bcc @palette_loop
	rts

dog_init_boss_door:
	jsr dog_init_boss_door_palette
dog_init_boss_door_common:
	; NOTE cannot rely on dog_now
	lda dog_param, Y
	cmp #6
	bcs @boss_end
		tya
		pha
		lda dog_param, Y
		clc
		adc #FLAG_BOSS_0_MOUNTAIN
		jsr flag_read
		beq :+
			lda #<DATA_room_start_again
			sta door_link+1
			lda #>DATA_room_start_again
			sta door_linkh+1
		:
		pla
		tay
	@boss_end:
	lda #6
	sta dog_param, Y
	lda #128
	sta dgd_BOSS_DOOR_WOBBLE, Y
	lda #0
	sta dgd_BOSS_DOOR_ANIM, Y
	PLAY_SOUND SOUND_BOSS_DOOR_OPEN
	rts

sprite2_add_banked:
	sta i
	stx j
	sty k
	lda #$F
	ldx #254
	jmp bank_call

; ==================
; Decimal conversion
; ==================

; clobbers A
decimal_clear:
	lda #0
decimal_set_all_:
	sta decimal+0
	sta decimal+1
	sta decimal+2
	sta decimal+3
	sta decimal+4
	rts

; clobbers A,X,Y
decimal_add:
	cmp #0
	beq @end
	tay
@increment:
	ldx #0
	cpy #100
	bcs @add_100
	cpy #10
	bcc @inc_loop
	@add_10:
		ldx #1
		tya
		sec
		sbc #9
		tay
		jmp @inc_loop
	@add_100:
		ldx #2
		tya
		sec
		sbc #99
		tay
		;jmp @inc_loop
	@inc_loop:
		inc decimal, X
		lda decimal, X
		cmp #10
		bcc :+ ; finished
		lda #0
		sta decimal, X
		inx
		cpx #5
		bcs decimal_maxed_out_
		jmp @inc_loop ; next digit
	:
	dey
	bne @increment
@end:
	rts
decimal_maxed_out_:
	lda #9
	jmp decimal_set_all_

; in dogs1.s:
;
; decimal_add32
; decimal_print

; end of file
