; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

; include file for dog implementations

; from dogs_common.s

.global load_sign
.global dx_screen_
.global dx_scroll_
.global dx_scroll_offset_
.global dx_scroll_edge_
.global dx_scroll_edge_common_
.global dx_scroll_river_
.global dog_draw_coin
.global bones_convert ; A=sprite, X=flip, Y=dog index (clobbers A/X)
.global bones_convert_silent ; does not play sound, does not increment BONES counter
.global do_fall ; ih:i,j,kh:k,l=bounding box, result in A/z, restores ih:i,j,kh:k,l (clobbers jh,n,o,p)
.global do_push ; ih:i,j,kh:k,l=bounding box, result in A/z (0=no,1=right,2=left), restores ih:i,j,kh:k,l (clobbers jh,lh,n,o,p)
.global move_dog
.global move_finish
.global move_finish_x
.global move_finish_y
.global lizard_overlap ; ih:i,j,kh:k,l = x0,y0,x1,y1, clobbers A, flags
.global lizard_touch ; ih:i,j,kh:k,l = x0,y0,x1,y1, clobbers A, flags
.global lizard_overlap_no_stone
.global lizard_touch_death
.global empty_dog ; clobbers A
.global dog_init_bones
.global dog_tick_bones
.global dog_init_boss_door_palette ; clobbers r,s,t
.global dog_init_boss_door ; clobbers r,s,t
.global dog_init_boss_door_common
.global dog_tick_fire_common
.global sprite2_add_banked
.global sprite2_add_flipped_banked
.global decimal_clear
.global decimal_add
.global decimal_maxed_out_

; from common.s

.import prng ; result in A
.import prng2
.import prng3
.import prng4
.import prng8
.import lizard_die ; clobbers A
.import inc_bones ; clobbers z flag
.import dogs_cycle
.import empty_blocker ; clobbers A,X
.import coin_read ; clobbers A,X,Y
.import coin_take
.import coin_return
.import coin_count
.import flag_read
.import flag_set
.import flag_clear

; common sound stuff

.include "sound_common.s"

; from collide.s

.import collide_set_tile
.import collide_clear_tile
.import collide_tile
.import collide_tile_left
.import collide_tile_right
.import collide_tile_up
.import collide_tile_down
.import collide_all
.import collide_all_left
.import collide_all_right
.import collide_all_up
.import collide_all_down

; ==================
; common definitions
; ==================

dgd_BONES_INIT = dog_data0
dgd_BONES_FLIP = dog_data1

dgd_COIN_ON   = dog_data0
dgd_COIN_ANIM = dog_data1

dgd_BOSS_DOOR_ANIM   = dog_data1
dgd_BOSS_DOOR_ROOM   = dog_data2
dgd_BOSS_DOOR_WOBBLE = dog_data3


; =================
; macro definitions
; =================

; DOG_BOUND sets up i,j,k,l as bounding box
; Y = dog index
; clobbers A
.macro DOG_BOUND px0, py0, px1, py1
	lda dog_x, Y
	.if px0 = 0
		sta i
		lda dog_xh, Y
		sta ih
	.else
		clc
		adc #<px0
		sta i
		lda dog_xh, Y
		adc #>px0
		sta ih
	.endif
	.if px1 = px0
		;lda ih
		sta kh
		lda i
		sta k
	.else
		lda i
		clc
		adc #<(px1-px0)
		sta k
		lda ih
		adc #>(px1-px0)
		sta kh
	.endif
	lda dog_y, Y
	.if py0 <> 0
		clc
		adc #py0
	.endif
	sta j
	.if py1 <> py0
		clc
		adc #(py1-py0)
	.endif
	sta l
.endmacro

; DOG_BLOCKER updates blocker bounding box
; Y = dog index
; clobbers A,X
.macro DOG_BLOCKER px0, py0, px1, py1
	tya
	and #3
	tax
	lda dog_x, Y
	.if px0 = 0
		sta blocker_x0, X
		lda dog_xh, Y
		sta blocker_xh0, X
	.else
		clc
		adc #<px0
		sta blocker_x0, X
		lda dog_xh, Y
		adc #>px0
		sta blocker_xh0, X
	.endif
	.if px1 = px0
		sta blocker_xh1, X
		lda blocker_x0, X
		sta blocker_x1, X
	.else
		lda dog_x, Y
		clc
		adc #<px1
		sta blocker_x1, X
		lda dog_xh, Y
		adc #>px1
		sta blocker_xh1, X
	.endif
	lda dog_y, Y
	.if py0 <> 0
		clc
		adc #py0
	.endif
	sta blocker_y0, X
	.if py1 <> py0
		clc
		adc #(py1-py0)
	.endif
	sta blocker_y1, X
.endmacro

; see DOG_POINT below (this only sets up ih:X and clobbers A,Y)
.macro DOG_POINT_X px
	.if px = 0
		ldx dog_x, Y
		lda dog_xh, Y
		sta ih
	.else
		lda dog_x, Y
		clc
		adc #<px
		tax
		lda dog_xh, Y
		adc #>px
		sta ih
	.endif
.endmacro

; see DOG_POINT below (this only sets up Y and clobbers A,Y)
.macro DOG_POINT_Y py
	lda dog_y, Y
	.if py = 0
		tay
	.else
		clc
		adc #py
		tay
	.endif
.endmacro

; loads point relative to dog into ih:X, Y
; suitable for collision tests
;
; input: Y = dog index
; clobbers: A,X,Y
; output: ih:X, Y
; note: Y is clobbered, probably need to ldy dog_now after collision
.macro DOG_POINT px, py
	DOG_POINT_X px
	DOG_POINT_Y py
.endmacro

; bounding box without reference to dog
.macro FIXED_BOUND px0, py0, px1, py1
	lda #<px0
	sta i
	lda #>px0
	sta ih
	lda #py0
	sta j
	lda #<px1
	sta k
	lda #>px1
	sta kh
	lda #py1
	sta l
.endmacro

; 16-bit dog_x/xh - px, result in carry flag (NOT EQ), clobbers A
.macro DOG_CMP_X px
	lda dog_x, Y
	cmp #<px
	lda dog_xh, Y
	sbc #>px
.endmacro

; loads dog_x into X, sprite_center
; Y is dog index
; clobbers A
.macro DX_SCREEN
	jsr dx_screen_
.endmacro

; loads dog_x adjusted for scroll, rts from enclosing function if offscreen
; Y is dog index
; clobbers A
; result in X, sprite_center
.macro DX_SCROLL
	jsr dx_scroll_
	beq :+
		rts
	:
.endmacro

; like DX_SCROLL but adds A to dog_x
; clobbers A/i/ih
; result in X
.macro DX_SCROLL_OFFSET
	jsr dx_scroll_offset_
	beq :+
		rts
	:
.endmacro

.macro DX_SCROLL_EDGE
	jsr dx_scroll_edge_
	beq :+
		rts
	:
.endmacro

.macro DX_SCROLL_RIVER
	jsr dx_scroll_river_
	beq :+
		rts
	:
.endmacro

.macro PPU_LATCH addr
	lda $2002
	lda #>addr
	sta $2006
	lda #<addr
	sta $2006
.endmacro

.macro PPU_LATCH_AT ax,ay
	PPU_LATCH ($2000+(ay*32)+ax)
.endmacro

.macro NMI_UPDATE_AT ax,ay
	lda #<($2000+(ay*32)+ax)
	sta nmi_load+0
	lda #>($2000+(ay*32)+ax)
	sta nmi_load+1
.endmacro

; end of file
