; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; dogs2 (bank D)
;
; dog update and drawing
;

.feature force_range
.macpack longbranch

.export dog_init
.export dog_tick
.export dog_draw

.include "ram.s"
.include "enums.s"
.include "version.s"
.include "dogs_inc.s"
.import bank_call

.import sprite_prepare
.import sprite_add
.import sprite_add_flipped
.import sprite_tile_add
.import sprite_tile_add_clip
.import palette_load
.import hex_to_ascii

.import ppu_nmi_write_row ; from common.dfs

sprite1_add         = sprite_add
sprite1_add_flipped = sprite_add_flipped

.include "../assets/export/names.s"
.include "../assets/export/text_set.s"

; ====
; Misc
; ====

.segment "CODE"

; used by DX_SCROLL_RIVER
dx_scroll_river_:
	ldx dog_x, Y
	lda dog_xh, Y
	jmp dx_scroll_edge_common_

sprite2_add_flipped_banked:
	sta i
	stx j
	sty k
	lda #$F
	ldx #253
	jmp bank_call

; input X = index
; output A = result, X = index
; clobbers: tb
circle4_banked:
	tya
	pha
	stx tb
	ldx #252
	lda #$E
	jsr bank_call
	; m = result
	; X = index
	pla
	tay
	lda tb
	rts

; input X = index
; output A/X = result
; clobbers A,X,Y
circle108_banked:
	stx t
	ldx #251
	lda #$F
	jsr bank_call
	txa
	rts

text_load_banked:
	sta t
	ldx #252
	lda #$F
	jmp bank_call

text_row_write:
	jsr text_load_banked
	jmp ppu_nmi_write_row

; set dog_data0...13 to 0
; clobbers: A
zero_dog_data:
	tya
	pha
	:
		lda #0
		sta dog_data0, Y
		tya
		clc
		adc #16
		tay
		cpy #(14*16)
		bcc :-
	pla
	tay
	rts

; carry = verso
dog_init_boss_door_out_partial:
	lda boss_rush ; does not affect carry
	beq @end
	; NOT IN DEMO
@end:
	rts

; carry = verso
dog_init_boss_door_out:
	jsr dog_init_boss_door_out_partial
	lda #6
	sta dog_param, Y
	jmp dog_init_boss_door

; clobbers A/X
ppu_nmi_update_row:
	lda $2002
	lda nmi_load+1
	sta $2006
	lda nmi_load+0
	sta $2006
	jmp ppu_nmi_write_row

; clobbers A/X
ppu_nmi_update_double:
	lda $2002
	lda nmi_load+1
	sta $2006
	lda nmi_load+0
	sta $2006
	ldx #0
	:
		lda nmi_update, X
		sta $2007
		inx
		cpx #64
		bcc :-
	rts

; ====
; Dogs
; ====

.segment "CODE"

; ==================
; Decimal conversion
; ==================

; moved to dogs_common.s:
;
; decimal_clear
; decimal_add
; decimal_maxed_out_

; l:k:j:i = 32 bit number to add
; clobbers A,X,Y,i,j,k,l
decimal_add32:
	lda i
	cmp #<(99999)
	lda j
	sbc #<(99999>>8)
	lda k
	sbc #<(99999>>16)
	lda l
	sbc #<(99999>>24)
	jcs decimal_maxed_out_
	@loop_256:
		; if l:k:j:i < 256 finish off
		lda j
		ora k
		ora l
		beq @finish
		; decimal_add(256)
		lda #255
		jsr decimal_add
		lda #1
		jsr decimal_add
		; l:k:j:i -= 256
		lda j
		bne :++
			lda k
			bne :+
				dec l
			:
			dec k
		:
		dec j
		; loop
		jmp @loop_256
	@finish:
	; l:k:j == 0, just finish off by adding i directly
	lda i
	jmp decimal_add

decimal_print:
	ldx #4
	:
		lda decimal, X
		bne :+
		lda #$70
		sta $2007
		dex
		bne :-
	:
		lda decimal, X
		ora #$60
		sta $2007
		dex
		cpx #-1
		bne :-
	;
	rts

; ====
; Dogs
; ====

; DOG_PASSWORD
stat_dog_password:

dgd_PASSWORD_ON    = dog_data0
;dgd_PASSWORD_VALUE = dog_data1

dog_init_password:
	ldx dog_param, Y
	cpx #255
	beq :+
	cpx #5
	bcs :+
		lda password, X
		sta dgd_PASSWORD_VALUE, Y
	:
	rts
dog_tick_password:
	DOG_BOUND 0,0,7,3
	jsr lizard_overlap
	beq @button_up
	lda dgd_PASSWORD_ON, Y
	bne @clicked_end
		lda #1
		sta dgd_PASSWORD_ON, Y
		lda dog_param, Y
		cmp #255
		beq @click_lizard
		@click_password:
			ldx dgd_PASSWORD_VALUE, Y
			inx
			txa
			and #7
			sta dgd_PASSWORD_VALUE, Y
			jmp @click_sound
		@click_lizard:
			inc current_lizard
			lda current_lizard
			.if DEBUG
				cmp #LIZARD_OF_COUNT
			.else
				cmp #LIZARD_OF_COFFEE
			.endif
			bcc :+
				lda #0
			:
			sta current_lizard
			sta next_lizard
			lda #0
			sta lizard_power
			ldx #0
			:
				lda dog_type, X
				cmp #DOG_SAVE_STONE
				bne :+
					lda #0
					sta dgd_SAVE_STONE_ON, X
				:
				inx
				cpx #16
				bcc :--
			;
		@click_sound:
		PLAY_SOUND SOUND_SWITCH
	@clicked_end:
	rts
@button_up:
	lda #0
	sta dgd_PASSWORD_ON, Y
	rts
dog_draw_password:
	DX_SCREEN
	tya
	pha
	lda dgd_PASSWORD_ON, Y
	bne :+
		lda dog_x, Y
		tax
		lda dog_y, Y
		tay
		lda #DATA_sprite1_password
		jsr sprite1_add
		jmp :++
	:
		ldx dog_x, Y
		lda dog_y, Y
		tay
		lda #DATA_sprite1_password_down
		jsr sprite1_add
	:
	pla
	tay
	lda dog_param, Y
	cmp #255
	bne :+
		rts
	:
	lda #%00000001
	sta i ; attribute
	lda dgd_PASSWORD_VALUE, Y
	clc
	adc #$61
	pha
	ldx dog_x, Y
	lda dog_y, Y
	sec
	sbc #48
	tay
	pla
	jsr sprite_tile_add
	rts

; DOG_LAVA_PALETTE
stat_dog_lava_palette:

dgd_LAVA_PALETTE_FRAME = dog_data0
dgd_LAVA_PALETTE_CYCLE = dog_data1

dog_init_lava_palette:
	rts
dog_tick_lava_palette:
	tya
	tax
	inc dgd_LAVA_PALETTE_FRAME, X
	lda dgd_LAVA_PALETTE_FRAME, Y
	cmp #9
	bcs :+
		rts
	:
	lda #0
	sta dgd_LAVA_PALETTE_FRAME, Y
	inc dgd_LAVA_PALETTE_CYCLE, X
	lda dgd_LAVA_PALETTE_CYCLE, Y
	cmp #3
	bcc :+
		lda #0
		sta dgd_LAVA_PALETTE_CYCLE, Y
	:
	clc
	adc #DATA_palette_lava0
	tax
	lda #3
	jsr palette_load
	ldy dog_now ; restore Y index
	rts
dog_draw_lava_palette:
	rts

; DOG_WATER_SPLIT
stat_water_split:

dog_init_water_split:
	lda water
	sta dog_param, Y
	;jmp dog_tick_water_split
dog_tick_water_split:
	lda lizard_xh
	bne :+
		lda #255
		sta water
		lda #0
		sta lizard_wet
		rts
	:
		; lizard_px >= 256
		lda dog_param, Y
		sta water
		rts
	;
dog_draw_water_split:
	rts

; DOG_BLOCK
stat_dog_block:

dog_init_block: ; only updates the blocker
	; Y = dog, dog_now untouched/unread
	DOG_BLOCKER -8,0,7,15
	rts

dog_block_shift:
	; Y = dog
	; u = amount to shift
	jsr empty_blocker
	lda #0
	sta v
	DOG_BOUND -8,0,7,15
	sty dog_now
	jsr move_dog
	bne @shift_no
	lda u
	jsr move_finish_x
	; check bounds
	lda dog_x, Y
	cmp #<9
	lda dog_xh, Y
	sbc #>9
	bcs :+
		; if dgx < 9
		lda #<9
		sta dog_x, Y
		lda #>9
		sta dog_xh, Y
		jmp @shift_no
	:
	lda room_scrolling
	bne :+
		lda dog_x, Y
		cmp #<248
		lda dog_xh, Y
		sbc #>248
		bcc @shift_yes
		; if dgx >= 248
		lda #<248
		sta dog_x, Y
		lda #>248
		sta dog_xh, Y
		jmp @shift_no
	:
		lda dog_x, Y
		cmp #<504
		lda dog_xh, Y
		sbc #>504
		bcc @shift_yes
		; if dgx >= 504
		lda #<504
		sta dog_x, Y
		lda #>504
		sta dog_xh, Y
		jmp @shift_no
	;
	@shift_yes:
		jsr dog_init_block
		jmp dog_block_stack
		;rts
	;
	@shift_no:
		jmp dog_init_block
		;rts
	;

dog_block_stack:
	lda u
	bne :+
		rts
	:
	; Y = dog_now = index of current dog
	; u = amount to shift
	lda #0
	sec
	sbc u ; invert u
	sta v ; v = -u
	clc
	adc dog_x, Y
	sta i
	lda v
	jsr load_sign
	adc dog_xh, Y
	sta ih ; ih:i = bx
	lda dog_y, Y
	sec
	sbc #16
	sta v ; v = by
	; u = u
	; ih:i = bx
	; v = by
	ldy #0
	@stack_loop:
		cpy dog_now
		beq @stack_loop_end
		lda dog_type, Y
		cmp #DOG_BLOCK
		beq @stack_loop_block
		cmp #DOG_BLOCK_ON
		bne @stack_loop_end
	@stack_loop_block:
		lda v
		cmp dog_y, Y
		bne @stack_loop_end ; if dog_y != by
		lda dog_x, Y
		clc
		adc #<11
		sta j
		lda dog_xh, Y
		adc #>11
		sta jh
		; jh:j = dog_x + 11
		lda j
		cmp i
		lda jh
		sbc ih
		bcc @stack_loop_end ; if dog_x + 11 < bx
		lda i
		clc
		adc #<11
		sta j
		lda ih
		adc #>11
		sta jh
		; jh:j = bx + 11
		lda j
		cmp dog_x, Y
		lda jh
		sbc dog_xh, Y
		bcc @stack_loop_end ; if bx + 11 < dog_x
	@stack_loop_shift:
		; preserve loop variables
		lda dog_now
		pha
		lda u
		pha
		lda v
		pha
		lda ih
		pha
		lda i
		pha
		jsr dog_block_shift ; shift the block
		pla
		sta i
		pla
		sta ih
		pla
		sta v
		pla
		sta u
		pla
		sta dog_now
	@stack_loop_end:
		iny
		cpy #16
		bcc @stack_loop
	ldy dog_now
	rts

dog_tick_block:
	DOG_BOUND -8,0,7,15
	jsr do_fall
	beq @fall_end
		inc j ; j = dog_y+0
		inc l
		lda j
		sta dog_y, Y
		cmp #240
		bcc :+
			jsr empty_blocker
			jmp empty_dog
		:
		jsr dog_init_block
	@fall_end:
	; DOG_BOUND is still valid
	jsr do_push
	bne :+
		rts
	:
	cmp #1
	bne :+
		lda #1
		sta u
		jsr move_finish_x
		jsr dog_init_block
		jsr dog_block_stack
	:
	cmp #2
	bne :+
		lda #-1
		sta u
		jsr move_finish_x
		jsr dog_init_block
		jsr dog_block_stack
	:
	jmp dog_init_block
	;rts

dog_draw_block:
	DX_SCROLL_EDGE
	lda dog_y, Y
	tay
	; override for block in dismount room
	lda dog_type + DISMOUNT_SLOT
	cmp #DOG_LIZARD_DISMOUNTER
	bne @draw_pal3
	lda current_lizard
	cmp #LIZARD_OF_PUSH
	bne @draw_pal3
@draw_pal0:
	; use lizard palette for block
	lda #DATA_sprite1_block_alt
	jmp sprite1_add
@draw_pal3:
	; block is normally palette 3
	lda #DATA_sprite1_block
	jmp sprite1_add

; DOG_BLOCK_ON
stat_dog_block_on:

dog_init_block_on:
	lda dog_param, Y
	jsr flag_read
	beq :+
		ldy dog_now
		jmp empty_dog
	:
	ldy dog_now
	jmp dog_init_block
dog_tick_block_on:
	DOG_BOUND -8,0,7,15
	jsr do_fall
	beq :+
		lda dog_param, Y
		jsr flag_set
		ldy dog_now
		lda #DOG_BLOCK
		sta dog_type, Y
	:
	jmp dog_tick_block
dog_draw_block_on = dog_draw_block

; DOG_BLOCK_OFF
stat_dog_block_off:

dog_init_block_off:
	lda dog_param, Y
	jsr flag_read
	bne :+
		ldy dog_now
		jmp empty_dog
	:
	ldy dog_now
	lda #DOG_BLOCK
	sta dog_type, Y
	jmp dog_init_block
dog_tick_block_off = dog_tick_block
dog_draw_block_off = dog_draw_block

; DOG_DRAWBRIDGE
stat_dog_drawbridge:

dgd_DRAWBRIDGE_BUTTON      = dog_data0
dgd_DRAWBRIDGE_ANIM        = dog_data1
dgd_DRAWBRIDGE_TILE_OFFSET = dog_data2

.segment "CHR"
drawbridge_chain:
.byte $02,$02,$02,$03,$01,$00,$02,$02,$04,$04,$04,$00,$00,$02,$04,$04 ; regular
.byte $40,$40,$40,$00,$80,$00,$40,$40,$00,$00,$00,$00,$00,$00,$00,$00
.byte $0F,$0F,$0F,$0F,$00,$00,$00,$00,$00,$00,$00,$00,$0F,$0F,$0F,$0F ; 2-bit version
.byte $F0,$F0,$F0,$F0,$F0,$F0,$F0,$F0,$00,$00,$00,$00,$00,$00,$00,$00

.segment "CODE"
dog_drawbridge_test:
	lda #0
	sta dgd_DRAWBRIDGE_BUTTON, Y
	lda #>184
	sta ih
	ldx #<184
	ldy #119
	jsr collide_all
	beq :+
		lda #2
		ldy dog_now
		sta dgd_DRAWBRIDGE_BUTTON, Y
		rts
	:
	ldy dog_now
	FIXED_BOUND 180,117,187,119
	jsr lizard_overlap
	beq :+
		lda #1
		sta dgd_DRAWBRIDGE_BUTTON, Y
	:
	rts

dog_drawbridge_block:
	DOG_BLOCKER -8,0,7,63
	rts

dog_drawbridge_move:
	jsr dog_drawbridge_block
	lda dog_y, Y
	cmp #144
	bcc :+
		PLAY_SOUND SOUND_STONE
		jmp :++
	:
		PLAY_SOUND SOUND_DRAWBRIDGE
	:
	; CHR tiles CA/CB, 32 bytes
	lda #$0C
	sta nmi_load+1
	lda #$A0
	sta nmi_load+0
	lda #NMI_ROW
	sta nmi_next
	lda #0
	sec
	sbc dog_y, Y
	and #7
	clc
	adc dgd_DRAWBRIDGE_TILE_OFFSET, Y
	sta s
	tay
	ldx #0
	:
		lda drawbridge_chain+ 0, Y
		sta nmi_update      + 0, X
		lda drawbridge_chain+ 8, Y
		sta nmi_update      + 8, X
		lda drawbridge_chain+16, Y
		sta nmi_update      +16, X
		lda drawbridge_chain+24, Y
		sta nmi_update      +24, X
		inx
		iny
		tya
		and #31
		cmp #8
		bcc :-
	tya
	sec
	sbc #8
	tay
	cpy s
	bcs :++
	:
		lda drawbridge_chain+ 0, Y
		sta nmi_update      + 0, X
		lda drawbridge_chain+ 8, Y
		sta nmi_update      + 8, X
		lda drawbridge_chain+16, Y
		sta nmi_update      +16, X
		lda drawbridge_chain+24, Y
		sta nmi_update      +24, X
		inx
		iny
		cpy s
		bcc :-
	:
	rts

dog_init_drawbridge:
	lda #FLAG_EYESIGHT
	jsr flag_read
	beq :+
		ldy dog_now
		lda #32
		sta dgd_DRAWBRIDGE_TILE_OFFSET, Y
	:
	ldy dog_now
	jsr dog_drawbridge_test
	jmp dog_drawbridge_block

dog_tick_drawbridge:
	jsr dog_drawbridge_test
	ldx dog_now
	inc dgd_DRAWBRIDGE_ANIM, X
	lda dgd_DRAWBRIDGE_ANIM, X
	and #3
	beq :+
		rts
	:
	lda dgd_DRAWBRIDGE_BUTTON, Y
	bne @button_down
	@button_up:
		DOG_BOUND -8,0,7,64
		jsr lizard_overlap
		beq :+
			rts
		:
		lda dog_y, Y
		cmp #144
		bcc :+
			rts
		:
		ldx dog_now
		inc dog_y, X
		jmp dog_drawbridge_move
	@button_down:
		lda dog_y, Y
		cmp #94
		bcs :+
			rts
		:
		ldx dog_now
		dec dog_y, X
		jmp dog_drawbridge_move
	;

dog_draw_drawbridge_door:
	DX_SCROLL_EDGE
	lda dog_y, Y
	tay
	lda #DATA_sprite1_drawbridge_door
	jmp sprite1_add
	;rts

dog_draw_drawbridge_button:
	lda dgd_DRAWBRIDGE_BUTTON, Y
	cmp #2
	bcc :+
		rts
	:
	lda #92
	DX_SCROLL_OFFSET
	lda #$83
	sta i
	lda dgd_DRAWBRIDGE_BUTTON, Y
	eor #$E0
	ldy #111
	jmp sprite_tile_add
	;rts

dog_draw_drawbridge:
	jsr dog_draw_drawbridge_door
	ldy dog_now
	jmp dog_draw_drawbridge_button
	;rts

; DOG_ROPE
stat_dog_rope:

dgd_ROPE_TILT = dog_data0
dgd_ROPE_BURN = dog_data1

dog_init_rope:
	lda dog_param, Y
	jsr flag_read
	beq :+
		ldy dog_now
		jmp empty_dog
	:
	ldy dog_now
	jmp dog_init_block ; blocker
dog_tick_rope:
	DOG_BOUND -8,-1,-8,-1
	jsr lizard_overlap
	bne @tilt_left
	DOG_BOUND  7,16, 7,16
	jsr lizard_overlap
	beq @tilt_next
	@tilt_left:
		lda #1
		jmp @tilt_end
	@tilt_next:
	DOG_BOUND  7,-1, 7,-1
	jsr lizard_overlap
	bne @tilt_right
	DOG_BOUND -8,16,-8,16
	jsr lizard_overlap
	beq @tilt_none
	@tilt_right:
		lda #2
		jmp @tilt_end
	@tilt_none:
	; else
		lda #0
	@tilt_end:
	sta dgd_ROPE_TILT, Y
	; burn
	ldx dgd_ROPE_BURN, Y
	beq @burn_touch
	@burn_tick:
		inx
		txa
		sta dgd_ROPE_BURN, Y
		cpx #128
		bcs :+
			rts
		:
		lda dog_param, Y
		jsr flag_set
		ldy dog_now ; Y clobbered by flag_set
		lda #DOG_BLOCK
		sta dog_type, Y
		rts
	@burn_touch:
		lda current_lizard
		cmp #LIZARD_OF_HEAT
		bne @burn_touch_end
		DOG_BOUND -1,-64,0,-1
		jsr lizard_touch
		beq @burn_touch_end
			lda #1
			sta dgd_ROPE_BURN, Y
			rts
		@burn_touch_end:
		rts
dog_draw_rope_block:
	DX_SCROLL_EDGE
	lda dgd_ROPE_TILT,Y
	cmp #1
	bne :+
		lda #DATA_sprite1_block_tilt_left
		jmp @sprite_selected
	:
	cmp #2
	bne :+
		lda #DATA_sprite1_block_tilt_right
		jmp @sprite_selected
	:
		lda #DATA_sprite1_block
	@sprite_selected:
	pha
	lda dog_y, Y
	tay
	pla
	jmp sprite1_add
dog_draw_rope_cord:
	lda #-4
	DX_SCROLL_OFFSET
	stx u ; u = dx
	lda dog_y, Y
	sec
	sbc #65
	sta v ; v = dy
	lda #$01
	sta i ; i = attrib
	lda #$DF
	sta t ; t = tile
	lda dgd_ROPE_BURN, Y
	beq :+
		lsr
		lsr
		and #1
		ora #$CE
		sta t
		lda #$00
		sta i
	:
	lda #8
	:
		pha
		ldx u
		ldy v
		lda t
		jsr sprite_tile_add
		lda v
		clc
		adc #8
		sta v
		pla
		tax
		dex
		beq :+
		txa
		jmp :-
	:
	rts
dog_draw_rope:
	jsr dog_draw_rope_block
	ldy dog_now
	jmp dog_draw_rope_cord

; DOG_BOSS_FLAME
stat_dog_boss_flame:

dgd_BOSS_FLAME_ANIM   = dog_data0
dgd_BOSS_FLAME_ATTRIB = dog_data1

dog_init_boss_flame:
	jsr prng
	and #$40
	ora #$03
	sta dgd_BOSS_FLAME_ATTRIB, Y
	jsr prng8
	sta dgd_BOSS_FLAME_ANIM, Y
	; test flag
	lda dog_param, Y
	clc
	adc #FLAG_BOSS_0_MOUNTAIN
	jsr flag_read
	bne :+
		ldy dog_now
		jmp empty_dog
	:
	; test all flags
	lda dog_type+1
	cmp #DOG_NONE
	beq :+
		rts ; already on
	:
	lda #FLAG_BOSS_0_MOUNTAIN
	@loop:
		pha
		jsr flag_read
		bne :+
			pla
			rts
		:
		pla
		clc
		adc #1
		cmp #(FLAG_BOSS_5_VOID+1)
		bcc @loop
	; disabled for demo
	;lda #DOG_BOSS_DOOR
	;sta dog_type+1
	;lda #6
	;sta dog_param+1
	;ldy #1
	;jmp dog_init_boss_door
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	rts

dog_tick_boss_flame:
	ldx dog_now
	inc dgd_BOSS_FLAME_ANIM, X
	DOG_BOUND -2,-5,1,-1
	jsr lizard_overlap_no_stone
	beq :+
		jsr lizard_die
	:
	rts

dog_draw_boss_flame:
	lda #-4
	DX_SCROLL_OFFSET
	lda dgd_BOSS_FLAME_ATTRIB, Y
	sta i
	lda dgd_BOSS_FLAME_ANIM, Y
	lsr
	lsr
	and #$7
	clc
	adc #$84
	sta t
	lda dog_y, Y
	sec
	sbc #9
	tay
	lda t
	jmp sprite_tile_add

; DOG_RIVER
stat_dog_river:

.export DOG_RIVER ; for lua hitbox script

;dgd_RIVER_SCROLL_A0   = dog_data0
;dgd_RIVER_SCROLL_A1   = dog_data1
;dgd_RIVER_SCROLL_B0   = dog_data2
;dgd_RIVER_SCROLL_B1   = dog_data3
;dgd_RIVER_SPLASH_TIME = dog_data4
;dgd_RIVER_SPLASH_FLIP = dog_data5
;dgd_RIVER_OVERLAP     = dog_data6
dgd_RIVER_HP          = dog_data7
dgd_RIVER_SEQ0        = dog_data8
dgd_RIVER_SEQ1        = dog_data9
dgd_RIVER_LOOP0       = dog_data10
dgd_RIVER_LOOP1       = dog_data11
dgd_RIVER_SEQ_TIME    = dog_data12
dgd_RIVER_SNEK_Y      = dog_data13
;dog_param is RIVER_SNEK_SEQ

RIVER_HP_TOTAL = 5

.enum
RE_LOOP = 0
RE_ROCK0
RE_ROCK1
RE_ROCK2
RE_ROCK3
RE_ROCK4
RE_ROCK5
RE_ROCK6
RE_ROCK7
RE_LOG
RE_DUCK
RE_RAMP
RE_RIVER_SEEKER
RE_BARREL
RE_WAVE
RE_SNEK_LOOP
RE_SNEK_HEAD
RE_SNEK_TAIL
RE_COUNT
.endenum

river_sequence:
;.if !FASTBOSS
.byte 240, RE_ROCK0,        165
.if 0 ; removed for demo
.byte 100, RE_ROCK1,        115
.byte  45, RE_ROCK2,        180
.byte   2, RE_ROCK7,        186
.byte  40, RE_ROCK7,        108
.byte   2, RE_ROCK4,        104
.byte   4, RE_ROCK1,        119
; first log
.byte  15, RE_LOG,          143
.byte  97, RE_ROCK0,        173
.byte  26, RE_ROCK1,        108
.byte  71, RE_ROCK0,        130
.byte  25, RE_ROCK1,        171
.byte   3, RE_ROCK7,        165
; log trio
.byte   7, RE_LOG,          163
.byte  53, RE_LOG,          123
.byte  78, RE_LOG,          143
; first seeker
.byte 140, RE_RIVER_SEEKER, 160
.byte 210, RE_ROCK1,        130
.byte   4, RE_ROCK7,        123
; log + rocks
.byte  68, RE_LOG,          164
.byte  30, RE_ROCK0,        121
.byte  60, RE_LOG,          128
.byte  66, RE_ROCK4,        186
.byte   4, RE_ROCK3,        188
.byte   2, RE_ROCK7,        175
.byte  59, RE_ROCK6,        109
.byte   3, RE_ROCK7,        114
.byte   1, RE_ROCK5,        106
; first ramp
.byte   4, RE_RAMP,         152
.byte  53, RE_ROCK5,        160
.byte   2, RE_ROCK3,        152
.byte   0, RE_ROCK7,        126
.byte  74, RE_LOG,          164
.byte  40, RE_ROCK1,        106
.byte   5, RE_ROCK2,        109
; seeker gauntlet
.byte  95, RE_RIVER_SEEKER, 170
.byte  79, RE_RIVER_SEEKER, 115
.byte  87, RE_RIVER_SEEKER, 160
.byte  74, RE_RIVER_SEEKER, 140
.byte  92, RE_RIVER_SEEKER, 122
.byte 220, RE_ROCK0,        165
.byte  60, RE_LOG,          122
.byte  50, RE_ROCK1,        188
.byte   4, RE_ROCK5,        182
.byte   3, RE_ROCK7,        178
; ramp gauntlet
.byte  83, RE_RAMP,         155
.byte  90, RE_ROCK2,        107
.byte   3, RE_ROCK7,        113
.byte  17, RE_ROCK0,        180
.byte  70, RE_RAMP,         140
.byte  54, RE_ROCK7,        110
.byte   2, RE_ROCK3,        136
.byte   1, RE_ROCK2,        145
.byte   3, RE_ROCK5,        151
.byte   8, RE_ROCK0,        186
.byte   4, RE_ROCK1,        168
.byte 130, RE_RAMP,         170
.byte  53, RE_ROCK5,        187
.byte   1, RE_ROCK7,        179
.byte   2, RE_ROCK3,        170
.byte   0, RE_ROCK4,        145
.byte  10, RE_ROCK0,        113
.byte   4, RE_ROCK2,        121
.byte  30, RE_RAMP,         135
.byte  56, RE_ROCK3,        122
.byte   0, RE_ROCK3,        146
.byte   0, RE_ROCK7,        155
.byte   4, RE_ROCK2,        186
.byte   5, RE_ROCK1,        180
.byte   2, RE_ROCK7,        171
.byte  28, RE_RAMP,         175
.byte  57, RE_ROCK3,        186
.byte   0, RE_ROCK7,        161
.byte   1, RE_ROCK0,        148
.byte   4, RE_ROCK2,        114
.byte   3, RE_ROCK1,        106
.byte  35, RE_RAMP,         130
.byte  56, RE_ROCK7,        106
.byte   2, RE_ROCK3,        130
.byte   1, RE_ROCK2,        141
.byte   4, RE_ROCK0,        160
.byte   3, RE_ROCK4,        180
.byte   2, RE_ROCK7,        186
.byte  32, RE_RAMP,         155
.byte  57, RE_ROCK7,        175
.byte   0, RE_ROCK3,        165
.byte   0, RE_ROCK3,        141
.byte   1, RE_ROCK7,        115
.byte   3, RE_ROCK0,        187
.byte   4, RE_ROCK1,        106
; duck and log gauntlet
.byte 210, RE_DUCK,         152
.byte 180, RE_LOG,          123
.byte  66, RE_LOG,          163
.byte  64, RE_LOG,          122
.byte  62, RE_LOG,          165
.byte  60, RE_LOG,          123
.byte  58, RE_LOG,          164
.byte  56, RE_LOG,          123
.byte  54, RE_LOG,          163
.byte  52, RE_LOG,          122
.byte  50, RE_LOG,          164
.byte  48, RE_LOG,          123
.byte  46, RE_LOG,          165
.byte  44, RE_LOG,          122
.byte  42, RE_LOG,          163
.byte  40, RE_LOG,          122
.byte  38, RE_LOG,          164
.byte  36, RE_LOG,          123
.byte  34, RE_LOG,          163
.byte  32, RE_LOG,          122
.byte  30, RE_LOG,          165
.byte  28, RE_LOG,          123
.byte  26, RE_LOG,          164
.byte  24, RE_LOG,          122
.byte  22, RE_LOG,          165
.endif
;.byte 130, RE_ROCK0,        125
; boss
.byte 190, RE_LOOP,           1
.if 0 ; removed boss for demo
.byte 210, RE_SNEK_LOOP,    130
.byte  60, RE_SNEK_LOOP,    150
.byte  60, RE_SNEK_LOOP,    170
.byte 150, RE_SNEK_HEAD,    146
.byte  10, RE_BARREL,       146
.byte 190, RE_SNEK_LOOP,    108
.byte  60, RE_SNEK_LOOP,    145
.byte  60, RE_SNEK_LOOP,    162
.byte  60, RE_SNEK_LOOP,    145
.byte  60, RE_SNEK_LOOP,    162
.byte  60, RE_SNEK_LOOP,    184
.byte 192, RE_SNEK_TAIL,    146
.byte  70, RE_SNEK_TAIL,    146
.endif
.byte  70, RE_LOOP,           0
.byte  60

river_event_type:
.byte DOG_RIVER_LOOP
.byte DOG_ROCK, DOG_ROCK, DOG_ROCK, DOG_ROCK, DOG_ROCK, DOG_ROCK, DOG_ROCK, DOG_ROCK
.byte DOG_LOG
.byte DOG_DUCK
.byte DOG_RAMP
.byte DOG_RIVER_SEEKER
.byte DOG_BARREL
.byte DOG_WAVE
.byte DOG_SNEK_LOOP
.byte DOG_SNEK_HEAD
.byte DOG_SNEK_TAIL
.assert (*-river_event_type)=RE_COUNT,error,"river_event_type entry mismatch"

; type in X
; y-value in A
; returns dog in dog_now, or Y if not overridden by its dog_init
dog_river_spawn:
	pha ; stack = y
	; find an empty slot (override 15 if none found-- shouldn't happen)
	ldy #1
	:
		lda dog_type, Y
		cmp #DOG_NONE
		beq :+
		iny
		cpy #15
		bcc :-
	:
	; setup new dog
	; X = type
	; stack = y
	lda river_event_type, X
	sta dog_type, Y
	txa
	sta dog_param, Y
	pla
	sta dog_y, Y
	lda #0
	sta dog_x, Y
	sta dog_xh, Y
	sty dog_now
	jmp dog_init

dog_river_event:
	; temp_ptr0 = river_sequence + seq_pos
	lda #<river_sequence
	clc
	adc dgd_RIVER_SEQ0 + RIVER_SLOT
	sta temp_ptr0+0
	lda #>river_sequence
	adc dgd_RIVER_SEQ1 + RIVER_SLOT
	sta temp_ptr0+1
	; seq_pos += 3
	lda dgd_RIVER_SEQ0 + RIVER_SLOT
	clc
	adc #<3
	sta dgd_RIVER_SEQ0 + RIVER_SLOT
	lda dgd_RIVER_SEQ1 + RIVER_SLOT
	adc #>3
	sta dgd_RIVER_SEQ1 + RIVER_SLOT
	; seq_type
	ldy #0
	lda (temp_ptr0), Y
	tax
	; seq_y
	iny
	lda (temp_ptr0), Y
	pha
	; seq_time
	iny
	lda (temp_ptr0), Y
	sta dgd_RIVER_SEQ_TIME + RIVER_SLOT
	pla
	; X = seq_type
	; A = seq_y
	jmp dog_river_spawn

dog_init_river:
	lda #0
	sta dog_param, Y
	sta dgd_RIVER_SEQ1 + RIVER_SLOT
	lda #1
	sta dgd_RIVER_SEQ0 + RIVER_SLOT
	lda river_sequence + 0
	sta dgd_RIVER_SEQ_TIME + RIVER_SLOT
	rts
dog_tick_river:
	; scroll top
	lda dgd_RIVER_SCROLL_A0 + RIVER_SLOT
	clc
	adc #<2
	sta dgd_RIVER_SCROLL_A0 + RIVER_SLOT
	lda dgd_RIVER_SCROLL_A1 + RIVER_SLOT
	adc #>2
	sta dgd_RIVER_SCROLL_A1 + RIVER_SLOT
	; scroll split
	lda dgd_RIVER_SCROLL_B0 + RIVER_SLOT
	clc
	adc #<4
	sta dgd_RIVER_SCROLL_B0 + RIVER_SLOT
	lda dgd_RIVER_SCROLL_B1 + RIVER_SLOT
	adc #>4
	sta dgd_RIVER_SCROLL_B1 + RIVER_SLOT
	; splash move
	lda dgd_RIVER_SPLASH_TIME + RIVER_SLOT
	cmp #16
	bcs @splash_on_end
		inc dgd_RIVER_SPLASH_TIME + RIVER_SLOT
		lda smoke_x
		sec
		sbc #4
		bcs :+
			lda #255
			sta smoke_y
			jmp @splash_on_end
		:
		sta smoke_x
		; random flip
		jsr prng
		and #$40
		sta dgd_RIVER_SPLASH_FLIP + RIVER_SLOT
	@splash_on_end:
	; sequence
	lda dgd_RIVER_SEQ_TIME + RIVER_SLOT
	beq :+
		dec dgd_RIVER_SEQ_TIME + RIVER_SLOT
		jmp :++
	:
		jsr dog_river_event
	:
	; check if dead, skip rest otherwise
	lda lizard_dead
	beq :+
		rts
	:
	; splash new
	lda dgd_RIVER_SPLASH_TIME + RIVER_SLOT
	cmp #16
	bcc @splash_off_end
		lda lizard_wet
		beq :+
			lda #17
			sta dgd_RIVER_SPLASH_TIME + RIVER_SLOT
			jmp @splash_off_end
		:
		jsr prng
		cmp #196
		bcs :+
		lda dgd_RIVER_SPLASH_TIME + RIVER_SLOT
		cmp #17
		beq :++
		:
			lda #0
			sta smoke_xh
			lda lizard_y+0
			sec
			sbc #8
			sta smoke_y
			lda lizard_x+0
			sec
			sbc #11
			sta smoke_x
			cmp lizard_x+0
			bcs :+
			lda smoke_y
			cmp lizard_y+0
			bcs :+
			lda #0
			sta dgd_RIVER_SPLASH_TIME + RIVER_SLOT
		:
		;
	@splash_off_end:
	;rts
dog_draw_river:
	rts

; DOG_RIVER_ENTER
stat_dog_river_enter:

dog_init_river_enter:
	lda #FLAG_BOSS_1_RIVER
	jsr flag_read
	bne :+
		rts
	:
	ldy dog_now
	lda #5
	sta dog_param, Y
	lda #<DATA_room_end_river_again
	sta door_link+5
	lda #>DATA_room_end_river_again
	sta door_linkh+5
	rts
dog_tick_river_enter:
	lda lizard_y+0
	sec
	sbc water
	bcc :+ ; lizard_y < water
	cmp #9
	bcc :+ ; lizard_y - water >= 8
		jmp lizard_die
	:
	lda lizard_dead
	bne :+
	lda current_lizard
	cmp #LIZARD_OF_SURF
	bne :+
	lda lizard_x+0
	cmp #<488
	lda lizard_xh
	sbc #>488
	bcc :+
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
dog_draw_river_enter:
	rts

; DOG_SPRITE1
stat_dog_sprite1:

dog_init_sprite1:
dog_tick_sprite1:
	rts
dog_draw_sprite1:
	DX_SCROLL_EDGE
	lda dog_param, Y
	pha
	lda dog_y, Y
	tay
	pla
	jmp sprite1_add

; DOG_BEYOND_STAR
stat_dog_beyond_star:

dgd_BEYOND_STAR_ANIM   = dog_data0
dgd_BEYOND_STAR_FRAME  = dog_data1
;dgd_BEYOND_STAR_FADE   = dog_data2
;dgd_BEYOND_STAR_DIE    = dog_data3

dog_init_beyond_star:
	; NOTE may not rely on dog_now
	rts
dog_tick_beyond_star:
	ldx dog_now ; for easy increment instructions
	; rotate
	inc dgd_BEYOND_STAR_ANIM, X
	lda dgd_BEYOND_STAR_ANIM, X
	cmp #5
	bcc @rot_end
		lda #0
		sta dgd_BEYOND_STAR_ANIM, X
		inc dgd_BEYOND_STAR_FRAME, X
		lda dgd_BEYOND_STAR_FRAME, X
		cmp #3
		bcc :+
			lda #0
			sta dgd_BEYOND_STAR_FRAME, X
		:
	@rot_end:
	; fade
	lda dgd_BEYOND_STAR_FADE, X
	beq @no_fade
		lda dgd_BEYOND_STAR_DIE, X
		beq @dec_fade
		@inc_fade:
			inc dgd_BEYOND_STAR_FADE, X
			lda dgd_BEYOND_STAR_FADE, X
			cmp #128
			bcc :+
				jmp empty_dog
			:
			rts
		;
		@dec_fade:
			dec dgd_BEYOND_STAR_FADE, X
			rts
		;
	@no_fade:
	; touch
	DOG_BOUND -7,-13,6,-4
	jsr lizard_overlap
	beq @touch_end
		PLAY_SOUND SOUND_SECRET
		ldx #DATA_palette_lizard10
		lda #4
		jsr palette_load
		lda #LIZARD_OF_BEYOND
		sta current_lizard
		sta next_lizard
		lda #$FF
		sta last_lizard
		lda #1
		ldy dog_now
		sta dgd_BEYOND_STAR_FADE, Y
		sta dgd_BEYOND_STAR_DIE, Y
	@touch_end:
	rts
dog_draw_beyond_star:
	lda dgd_BEYOND_STAR_FADE, Y
	beq :+
		jsr prng
		and #127
		cmp dgd_BEYOND_STAR_FADE, Y
		bcs :+
			rts
		;
	:
	DX_SCROLL_EDGE
	lda dgd_BEYOND_STAR_FRAME, Y
	clc
	adc #DATA_sprite1_beyond_star
	pha
	lda dog_y, Y
	tay
	pla
	jmp sprite1_add
	;rts

; DOG_PARTICLE
stat_dog_particle:

dgd_PARTICLE_XH   = dog_data0
dgd_PARTICLE_X    = dog_data1
dgd_PARTICLE_Y    = dog_data2
dgd_PARTICLE_WAIT = dog_data3
dgd_PARTICLE_TIME = dog_data4

dog_init_particle:
	; NOTE may not rely on dog_now or change Y
	lda dog_xh, Y
	sta dgd_PARTICLE_XH, Y
	lda dog_x, Y
	sta dgd_PARTICLE_X, Y
	lda dog_y, Y
	sta dgd_PARTICLE_Y, Y
	jsr prng3
	and #7
	sta dgd_PARTICLE_WAIT, Y
	rts
dog_tick_particle:
	lda dgd_PARTICLE_TIME, Y
	beq @not_rising
		ldx dog_now
		dec dog_y, X
		;lda dgd_PARTICLE_TIME, Y
		sec
		sbc #1
		sta dgd_PARTICLE_TIME, Y
		bne :+
			; wait for new particle
			jsr prng2
			and #7
			sta dgd_PARTICLE_WAIT, Y
		:
		rts
	@not_rising:
	; wait
	lda dgd_PARTICLE_WAIT, Y
	beq :+
		ldx dog_now
		dec dgd_PARTICLE_WAIT, X
		rts
	:
	; spawn
		jsr prng2
		and #7
		clc
		adc dgd_PARTICLE_X, Y
		sta dog_x, Y
		lda dgd_PARTICLE_XH, Y
		adc #0
		sta dog_xh, Y
		lda dgd_PARTICLE_Y, Y
		sta dog_y, Y
		jsr prng2
		and #7
		clc
		adc dog_param, Y
		sta dgd_PARTICLE_TIME, Y
		rts
	;
dog_draw_particle:
	lda dgd_PARTICLE_TIME, Y
	bne :+
		rts
	:
	DX_SCROLL
	lda dog_y, Y
	tay
	lda #$01
	sta i ; attribute
	lda #$C6 ; tile
	jmp sprite_tile_add
	;rts

; DOG_BEYOND_END
stat_dog_beyond_end:

dgd_BEYOND_END_TIMEOUT = dog_data0
dgd_BEYOND_END_DONE    = dog_data1

dog_init_beyond_end:
	lda human0_pal
	clc
	adc #1
	cmp #3
	bcc :+
		sec
		sbc #3
	:
	clc
	adc #DATA_palette_human0_black
	tax
	lda #6
	jsr palette_load
	lda human0_pal
	clc
	adc #2
	cmp #3
	bcc :+
		sec
		sbc #3
	:
	clc
	adc #DATA_palette_human0_sky
	tax
	lda #3
	jsr palette_load
	lda palette + $07
	sta palette + $0F
	rts
dog_tick_beyond_end:
	lda dgd_BEYOND_END_DONE, Y
	beq :+
		lda #1
		sta room_change
		sta current_door
		lda door_link+1
		sta current_room+0
		lda door_linkh+1
		sta current_room+1
		rts
	:
	lda dog_type + BEYOND_STAR_SLOT
	cmp #DOG_NONE
	beq :+
		rts
	:
	lda #3
	sta lizard_dismount ; forced fly
	ldx dog_now
	inc dgd_BEYOND_END_TIMEOUT, X
	lda dgd_BEYOND_END_TIMEOUT, Y
	cmp #250
	bcs :+
	lda lizard_y+0
	cmp #40
	bcc :+
		rts
	:
	lda #0
	sta lizard_vy+0
	sta lizard_vy+1
	lda #1
	sta dgd_BEYOND_END_DONE, Y
	lda #TEXT_BEYOND
	sta game_message
	lda #1
	sta game_pause
	rts
dog_draw_beyond_end:
	rts

; DOG_OTHER_END_LEFT
stat_dog_other_end_left:

dgd_OTHER_END_ANIM = dog_data0

dog_blocker_other_end:
	lda lizard_xh
	beq :+
		lda dog_xh, Y
		beq @skip
		jmp @block
	:
		lda dog_xh, Y
		bne @skip
		;jmp @block
	;
@block:
	DOG_BLOCKER -8,-14,7,-1
@skip:
	rts

; nametable address returned in ih:i (A = ih)
dog_nmt_other_end:
	lda dog_x, Y
	lsr
	lsr
	lsr
	sec
	sbc #$01
	and #$1F
	sta i
	lda dog_y, Y
	sec
	sbc #$10
	pha
	asl
	asl
	and #$E0
	ora i
	sta i
	pla ; dog_y - $10
	rol
	rol
	rol
	and #$03
	sta ih
	lda dog_xh, Y
	asl
	asl
	and #$04
	ora ih
	ora #$20
	sta ih
	rts

dog_nmt_clear_other_end:
	jsr dog_nmt_other_end
	; A = ih
	bit $2002
	sta $2006
	lda i
	sta $2006
	lda #$91
	sta $2007
	sta $2007
	rts

dog_init_other_end_left:
	jsr prng8
	sta dgd_OTHER_END_ANIM, Y
	ldx dog_param, Y
	lda human1_set, X
	cmp #3
	bcc :+
		jsr dog_nmt_clear_other_end
		jmp empty_dog
	:
	sec
	sbc human0_pal
	bpl :+
		clc
		adc #3
	:
	sta dog_param, Y
	cmp #2
	bcs :+
		jsr dog_nmt_clear_other_end
	:
	jmp dog_blocker_other_end
dog_tick_other_end_left:
	DOG_BOUND -8,-14,7,-1
	jsr lizard_touch_death
	beq @death_end
		; nametable update
		lda #2
		sta nmi_update + 0
		jsr dog_nmt_other_end
		sta nmi_update + 1
		lda i
		sta nmi_update + 2
		inc i
		bne :+
			inc ih
		:
		lda i
		sta nmi_update + 5
		lda ih
		sta nmi_update + 4
		lda #$91
		sta nmi_update + 3
		sta nmi_update + 6
		lda #NMI_STREAM
		sta nmi_next
		; death
		jsr empty_blocker
		ldx #0
		lda dog_type, Y
		cmp #DOG_OTHER_END_LEFT
		beq :+
			ldx #1
		:
		lda #DATA_sprite0_lizard_skull_dismount
		jmp bones_convert
		;rts
	@death_end:
	; blocker
	jsr dog_blocker_other_end
	; blink
	ldx dgd_OTHER_END_ANIM, Y
	beq :+
		dex
		txa
		sta dgd_OTHER_END_ANIM, Y
		jmp :++
	:
		jsr prng4
		ora #$80
		sta dgd_OTHER_END_ANIM, Y
	:
	; talk
	lda lizard_power
	beq :+
	lda current_lizard
	cmp #LIZARD_OF_KNOWLEDGE
	bne :+
	lda lizard_dead
	bne :+
	lda boss_talk
	bne :+
	DOG_BOUND -20,-31,19,32
	jsr lizard_overlap
	beq :+
		lda #1
		sta boss_talk
		lda #TEXT_OTHER_END
		sta game_message
		lda #1
		sta game_pause
	:
	rts

dog_draw_other_end_left:
	DX_SCROLL_EDGE
	lda #DATA_sprite1_other_end0
	sta s
	lda dgd_OTHER_END_ANIM, Y
	cmp #8
	bcs :+
		lda #DATA_sprite1_other_end_blink0
		sta s
	:
	lda dog_param, Y
	clc
	adc s
	sta s
	lda dog_type, Y
	cmp #DOG_OTHER_END_LEFT
	bne :+
		lda dog_y, Y
		tay
		lda s
		jmp sprite1_add
	:
		lda dog_y, Y
		tay
		lda s
		jmp sprite1_add_flipped
	;

; DOG_OTHER_END_RIGHT
stat_dog_other_end_right:

dog_init_other_end_right = dog_init_other_end_left
dog_tick_other_end_right = dog_tick_other_end_left
dog_draw_other_end_right = dog_draw_other_end_left

; DOG_INFO
stat_dog_info:

dog_init_info:
	; invalidate dog_b2 in case of eyesight
	lda #$FF
	sta chr_cache+3
	; Lizard
	PPU_LATCH_AT 0,4
	lda #TEXT_INFO0
	jsr text_row_write
	lda #TEXT_INFO1
	jsr text_row_write
	; Controls
	PPU_LATCH_AT 0,(4+32)
	lda #TEXT_INFO5
	jsr text_row_write
	PPU_LATCH_AT 0,(6+32)
	ldx #6
	:
		txa
		pha
		clc
		adc #TEXT_INFO0
		jsr text_row_write
		pla
		tax
		inx
		cpx #13
		bcc :-
	rts
dog_tick_info:
	rts
dog_draw_info:
	DX_SCROLL
	lda #$01
	sta i ; attribute
	lda #($60 + GAME_VERSION)
	ldy #207
	jmp sprite_tile_add

; DOG_DIAGNOSTIC
stat_dog_diagnostic:

dgd_DIAGNOSTIC_TOGGLE_ON   = dog_data0
; 1 removed
dgd_DIAGNOSTIC_SELECT_COIN = dog_data2
dgd_DIAGNOSTIC_SELECT_FLAG = dog_data3
dgd_DIAGNOSTIC_SELECT      = dog_data4
dgd_DIAGNOSTIC_HOLD        = dog_data5
dgd_DIAGNOSTIC_COIN_0      = dog_data6
dgd_DIAGNOSTIC_COIN_1      = dog_data7
dgd_DIAGNOSTIC_COIN_2      = dog_data8
dgd_DIAGNOSTIC_JUMP_0      = dog_data9
dgd_DIAGNOSTIC_JUMP_1      = dog_data10
dgd_DIAGNOSTIC_JUMP_2      = dog_data11
dgd_DIAGNOSTIC_JUMP_3      = dog_data12
dgd_DIAGNOSTIC_JUMP_4      = dog_data13

.segment "CODE"

dog_nmt_diagnostic_common:
	; blank second row
	lda #$70
	ldx #32
	:
		sta nmi_update, X
		inx
		cpx #64
		bcc :-
	; columns
	lda #$B8
	sta nmi_update+ 0
	sta nmi_update+30
	sta nmi_update+32
	sta nmi_update+62
	lda #$B9
	sta nmi_update+ 1
	sta nmi_update+31
	sta nmi_update+33
	sta nmi_update+63
	rts

dog_nmt_diagnostic_time:
	lda #TEXT_DIAGNOSTIC_PLAYTIME
	jsr text_load_banked
	jsr dog_nmt_diagnostic_common
	jsr decimal_clear
	lda metric_time_h
	jsr decimal_add
	ldx #15
	jsr dog_nmt_diagnostic_time_digits
	jsr decimal_clear
	lda metric_time_m
	jsr decimal_add
	ldx #18
	jsr dog_nmt_diagnostic_time_digits
	jsr decimal_clear
	lda metric_time_s
	jsr decimal_add
	ldx #21
	jsr dog_nmt_diagnostic_time_digits
	jsr decimal_clear
	lda metric_time_f
	jsr decimal_add
	ldx #24
	jsr dog_nmt_diagnostic_time_digits
	lda #$5E
	sta nmi_update+17
	sta nmi_update+20
	lda #$5C
	sta nmi_update+23
	NMI_UPDATE_AT 0,6
	rts
dog_nmt_diagnostic_time_digits:
	lda decimal+1
	ora #$60
	sta nmi_update,X
	lda decimal+0
	ora #$60
	sta nmi_update+1,X
	rts

; decimal = jump count
dog_nmt_diagnostic_jumps:
	lda #TEXT_DIAGNOSTIC_JUMPS
	jsr text_load_banked
	jsr dog_nmt_diagnostic_common
	.repeat 5,I
		lda decimal+(4-I)
		ora #$60
		sta nmi_update+11+I
	.endrepeat
	ldx #0
	:
		lda nmi_update+11, X
		cmp #$60
		bne :+
		lda #$70
		sta nmi_update+11, X
		inx
		cpx #4
		bcc :-
	:
	NMI_UPDATE_AT 0,8
	rts

dog_nmt_diagnostic_coins:
	jsr decimal_clear
	jsr coin_count
	jsr decimal_add
	ldy dog_now
	lda decimal+0
	sta dgd_DIAGNOSTIC_COIN_0, Y
	lda decimal+1
	sta dgd_DIAGNOSTIC_COIN_1, Y
	lda decimal+2
	sta dgd_DIAGNOSTIC_COIN_2, Y
	lda #TEXT_DIAGNOSTIC_COINS
	jsr text_load_banked
	jsr dog_nmt_diagnostic_common
	ldx #0
	ldy #0
	:
		tya
		pha
		lda coin, Y
		pha
		lsr
		lsr
		lsr
		lsr
		tay
		lda hex_to_ascii, Y
		sta nmi_update+12, X
		pla
		and #$0F
		tay
		lda hex_to_ascii, Y
		sta nmi_update+13, X
		pla
		tay
		pha
		lda coin+8, Y
		pha
		lsr
		lsr
		lsr
		lsr
		tay
		lda hex_to_ascii, Y
		sta nmi_update+32+12, X
		pla
		and #$0F
		tay
		lda hex_to_ascii, Y
		sta nmi_update+32+13, X
		pla
		tay
		inx
		inx
		iny
		cpy #8
		bcc :-
	NMI_UPDATE_AT 0,14
	rts

dog_nmt_diagnostic_flags:
	lda #TEXT_DIAGNOSTIC_FLAGS
	jsr text_load_banked
	jsr dog_nmt_diagnostic_common
	ldx #0
	ldy #0
	:
		tya
		pha
		lda flag, Y
		pha
		lsr
		lsr
		lsr
		lsr
		tay
		lda hex_to_ascii, Y
		sta nmi_update+12, X
		pla
		and #$0F
		tay
		lda hex_to_ascii, Y
		sta nmi_update+13, X
		pla
		tay
		pha
		lda flag+8, Y
		pha
		lsr
		lsr
		lsr
		lsr
		tay
		lda hex_to_ascii, Y
		sta nmi_update+32+12, X
		pla
		and #$0F
		tay
		lda hex_to_ascii, Y
		sta nmi_update+32+13, X
		pla
		tay
		inx
		inx
		iny
		cpy #8
		bcc :-
	NMI_UPDATE_AT 0,16
	rts

dog_init_diagnostic:
	; setup hold
	lda #1
	sta dgd_DIAGNOSTIC_HOLD, Y
	; return to room left
	lda diagnostic_room+0
	sta door_link+0
	lda diagnostic_room+1
	sta door_linkh+0
	; pre-draw time
	jsr dog_nmt_diagnostic_time
	jsr ppu_nmi_update_row
	; pre-draw bones
	PPU_LATCH_AT 11,7
	jsr decimal_clear
	lda metric_bones+0
	sta i
	lda metric_bones+1
	sta j
	lda metric_bones+2
	sta k
	lda metric_bones+3
	sta l
	jsr decimal_add32
	jsr decimal_print
	; pre-draw jump and setup
	PPU_LATCH_AT 11,8
	jsr decimal_clear
	lda metric_jumps+0
	sta i
	lda metric_jumps+1
	sta j
	lda metric_jumps+2
	sta k
	lda metric_jumps+3
	sta l
	jsr decimal_add32
	ldy dog_now
	lda decimal+0
	sta dgd_DIAGNOSTIC_JUMP_0, Y
	lda decimal+1
	sta dgd_DIAGNOSTIC_JUMP_1, Y
	lda decimal+2
	sta dgd_DIAGNOSTIC_JUMP_2, Y
	lda decimal+3
	sta dgd_DIAGNOSTIC_JUMP_3, Y
	lda decimal+4
	sta dgd_DIAGNOSTIC_JUMP_4, Y
	lda metric_jumps+0
	sta dog_param, Y
	jsr decimal_print
	; continues
	PPU_LATCH_AT 11,10
	jsr decimal_clear
	lda metric_continue+0
	sta i
	lda metric_continue+1
	sta j
	lda metric_continue+2
	sta k
	lda metric_continue+3
	sta l
	jsr decimal_add32
	jsr decimal_print
	; pre-draw NTSC/PAL
	PPU_LATCH_AT 10,12
	lda player_pal
	bne :+
		lda #TEXT_DIAGNOSTIC_NTSC
		jmp :++
	:
		lda #TEXT_DIAGNOSTIC_PAL
	:
	jsr text_load_banked
	ldx #0
	:
		lda nmi_update, X
		sta $2007
		inx
		cpx #4
		bcc :-
	; coins
	jsr dog_nmt_diagnostic_coins
	jsr ppu_nmi_update_double
	; flags
	jsr dog_nmt_diagnostic_flags
	jsr ppu_nmi_update_double
	rts

dog_tick_diagnostic:
	; invalidate CHR cache in case needed
	ldx #0
	lda #$FF
	:
		sta chr_cache, X
		inx
		cpx #8
		bcc :-
	; selector
	lda gamepad
	and #(PAD_U | PAD_D | PAD_SELECT)
	bne :+
		lda #0
		sta dgd_DIAGNOSTIC_HOLD, Y
		jmp @selector_end
	:
	ldx dgd_DIAGNOSTIC_HOLD, Y
	bne @selector_end
		ldx dog_now
		cmp #PAD_D
		beq @selector_d
		cmp #PAD_U
		beq @selector_u
		cmp #PAD_SELECT
		beq @selector_select
		jmp @selector_none
		@selector_d:
			lda dgd_DIAGNOSTIC_SELECT, Y
			bne :+
				dec dgd_DIAGNOSTIC_SELECT_COIN, X
				jmp :++
			:
				dec dgd_DIAGNOSTIC_SELECT_FLAG, X
			:
			jmp @selector_ud
		@selector_u:
			lda dgd_DIAGNOSTIC_SELECT, Y
			bne :+
				inc dgd_DIAGNOSTIC_SELECT_COIN, X
				jmp :++
			:
				inc dgd_DIAGNOSTIC_SELECT_FLAG, X
			:	
			;jmp @selector_ud
		@selector_ud:
			; clamp 0-127
			lda dgd_DIAGNOSTIC_SELECT_COIN, Y
			and #127
			sta dgd_DIAGNOSTIC_SELECT_COIN, Y
			lda dgd_DIAGNOSTIC_SELECT_FLAG, Y
			and #127
			sta dgd_DIAGNOSTIC_SELECT_FLAG, Y
			jmp @selector_none
		@selector_select:
			lda dgd_DIAGNOSTIC_SELECT, Y
			eor #1
			sta dgd_DIAGNOSTIC_SELECT, Y
			;jmp @selector_none
		@selector_none:
		lda #1
		sta dgd_DIAGNOSTIC_HOLD, Y
	@selector_end:
	; time
	jsr dog_nmt_diagnostic_time
	lda #NMI_ROW
	sta nmi_next
	; jumps
	ldy dog_now
	lda dog_param, Y
	cmp metric_jumps+0
	beq @jump_end
		lda dgd_DIAGNOSTIC_JUMP_0, Y
		sta decimal+0
		lda dgd_DIAGNOSTIC_JUMP_1, Y
		sta decimal+1
		lda dgd_DIAGNOSTIC_JUMP_2, Y
		sta decimal+2
		lda dgd_DIAGNOSTIC_JUMP_3, Y
		sta decimal+3
		lda dgd_DIAGNOSTIC_JUMP_4, Y
		sta decimal+4
		lda #1
		jsr decimal_add
		ldy dog_now
		lda decimal+0
		sta dgd_DIAGNOSTIC_JUMP_0, Y
		lda decimal+1
		sta dgd_DIAGNOSTIC_JUMP_1, Y
		lda decimal+2
		sta dgd_DIAGNOSTIC_JUMP_2, Y
		lda decimal+3
		sta dgd_DIAGNOSTIC_JUMP_3, Y
		lda decimal+4
		sta dgd_DIAGNOSTIC_JUMP_4, Y
		lda metric_jumps+0
		sta dog_param, Y
		jsr dog_nmt_diagnostic_jumps
		lda #NMI_ROW
		sta nmi_next
		jmp @toggle_end ; jump this frame prevents toggle
	@jump_end:
	; toggle
	FIXED_BOUND 24,184,(24+7),(184+3)
	jsr lizard_overlap
	beq @toggle_off
		ldy dog_now
		lda dgd_DIAGNOSTIC_TOGGLE_ON, Y
		bne @toggle_end
		lda #1
		sta dgd_DIAGNOSTIC_TOGGLE_ON, Y
		PLAY_SOUND SOUND_SWITCH
		lda dgd_DIAGNOSTIC_SELECT, Y
		bne @toggle_flag
		@toggle_coin:
			lda dgd_DIAGNOSTIC_SELECT_COIN, Y
			jsr coin_read
			php
			ldy dog_now
			lda dgd_DIAGNOSTIC_SELECT_COIN, Y
			plp
			beq :+
				jsr coin_return
				jmp :++
			:
				jsr coin_take
			:
			jsr dog_nmt_diagnostic_coins
			jmp @toggle_finish
		@toggle_flag:
			lda dgd_DIAGNOSTIC_SELECT_FLAG, Y
			jsr flag_read
			php
			ldy dog_now
			lda dgd_DIAGNOSTIC_SELECT_FLAG, Y
			plp
			beq :+
				jsr flag_clear
				jmp :++
			:
				jsr flag_set
			:
			jsr dog_nmt_diagnostic_flags
		@toggle_finish:
		lda #NMI_DOUBLE
		sta nmi_next
		jmp @toggle_end	
	@toggle_off:
		ldy dog_now
		lda #0
		sta dgd_DIAGNOSTIC_TOGGLE_ON, Y
	@toggle_end:
	; scuttle crash
	lda lizard_scuttle
	cmp #254
	bcc :+
		brk ; test of crash screen
	:
	; human switch
	lda gamepad
	cmp #(PAD_SELECT | PAD_D)
	bne @human_switch_end
		; reroll human
		jsr prng8
		sta human0_hair
		:
			jsr prng2
			and #3
			beq :-
		sta human0_pal
		dec human0_pal
		; new room
		lda #1
		sta room_change
		sta current_door
		lda #0
		sta coin_saved
	@human_switch_end:
	rts

dog_draw_diagnostic:
	lda #128
	jsr sprite_prepare
	; toggle button
	ldy dog_now
	lda dgd_DIAGNOSTIC_TOGGLE_ON, Y
	bne :+
		lda #DATA_sprite1_password
		jmp :++
	:
		lda #DATA_sprite1_password_down
	:
	ldx #24
	ldy #184
	jsr sprite1_add
	; controller display
	lda #$00
	sta i ; common attribute
	ldy #87 ; common Y value
	lda gamepad
	and #PAD_U
	beq :+
		ldx #128
		lda #$E7
		jsr sprite_tile_add
	:
	lda gamepad
	and #PAD_D
	beq :+
		ldx #136
		lda #$E7
		jsr sprite_tile_add
	:
	lda gamepad
	and #PAD_L
	beq :+
		ldx #144
		lda #$E7
		jsr sprite_tile_add
	:
	lda gamepad
	and #PAD_R
	beq :+
		ldx #152
		lda #$E7
		jsr sprite_tile_add
	:
	lda gamepad
	and #PAD_B
	beq :+
		ldx #160
		lda #$E7
		jsr sprite_tile_add
	:
	lda gamepad
	and #PAD_A
	beq :+
		ldx #168
		lda #$E7
		jsr sprite_tile_add
	:
	lda gamepad
	and #PAD_SELECT
	beq :+
		ldx #184
		lda #$E7
		jsr sprite_tile_add
	:
	lda gamepad
	and #PAD_START
	beq :+
		ldx #208
		lda #$E7
		jsr sprite_tile_add
	:
	; toggle select
	ldy dog_now
	lda dgd_DIAGNOSTIC_SELECT, Y
	asl
	asl
	asl
	clc
	adc #151
	tay
	ldx #152
	lda #$43
	sta i
	lda #$FE
	jsr sprite_tile_add
	; coin select
	lda #$00
	sta i ; common attribute
	ldy dog_now
	lda dgd_DIAGNOSTIC_SELECT_COIN, Y
	pha
	pha
	and #$F
	tax
	lda hex_to_ascii, X
	eor #$C0
	pha
	ldy #151
	ldx #104
	pla
	jsr sprite_tile_add
	pla
	lsr
	lsr
	lsr
	lsr
	tax
	lda hex_to_ascii, X
	eor #$C0
	pha
	ldy #151
	ldx #96
	pla
	jsr sprite_tile_add
	pla
	jsr coin_read
	and #1
	ora #$A0
	ldy #151
	ldx #136
	jsr sprite_tile_add
	; flag select
	lda #$00
	sta i ; common attribute
	ldy dog_now
	lda dgd_DIAGNOSTIC_SELECT_FLAG, Y
	pha
	pha
	and #$F
	tax
	lda hex_to_ascii, X
	eor #$C0
	pha
	ldy #159
	ldx #104
	pla
	jsr sprite_tile_add
	pla
	lsr
	lsr
	lsr
	lsr
	tax
	lda hex_to_ascii, X
	eor #$C0
	pha
	ldy #159
	ldx #96
	pla
	jsr sprite_tile_add
	pla
	jsr flag_read
	and #1
	ora #$A0
	ldy #159
	ldx #136
	jsr sprite_tile_add
	; coin
	lda #$01
	sta i ; common attribute
	ldy dog_now
	lda dgd_DIAGNOSTIC_COIN_2, Y
	bne @draw_coin_2
	lda dgd_DIAGNOSTIC_COIN_1, Y
	bne @draw_coin_1
	lda dgd_DIAGNOSTIC_COIN_0, Y
	jmp @draw_coin_0
@draw_coin_2:
	ora #$A0
	pha
	ldx #104
	ldy #71
	pla
	jsr sprite_tile_add
	ldy dog_now
	lda dgd_DIAGNOSTIC_COIN_1, Y
@draw_coin_1:
	ora #$A0
	pha
	ldx #112
	ldy #71
	pla
	jsr sprite_tile_add
	ldy dog_now
	lda dgd_DIAGNOSTIC_COIN_0, Y
@draw_coin_0:
	ora #$A0
	pha
	ldx #120
	ldy #71
	pla
	jsr sprite_tile_add
	; version
	lda #$01
	sta i ; common attribute
	ldx #(88+ 0)
	ldy #223
	lda #($A0 | VERSION_MAJOR)
	jsr sprite_tile_add
	ldx #(88+16)
	ldy #223
	lda #($A0 | VERSION_MINOR)
	jsr sprite_tile_add
	ldx #(88+32)
	ldy #223
	lda #($A0 | VERSION_BETA)
	jsr sprite_tile_add
	ldx #(88+48)
	ldy #223
	lda #($A0 | VERSION_REVISED)
	jsr sprite_tile_add
	; human
	lda #$02
	sta i ; comon attribute
	ldx #216
	ldy #63
	lda #$6E
	jsr sprite_tile_add
	ldx #215
	ldy #71
	lda #$6F
	jmp sprite_tile_add
	;rts

; DOG_METRICS
stat_dog_metrics:

; decimal_print modified for metrics tileset
metric_print:
	ldx #4
	:
		lda decimal, X
		bne :+
		lda #$AB
		sta $2007
		dex
		bne :-
	:
		lda decimal, X
		ora #$A0
		sta $2007
		dex
		cpx #-1
		bne :-
	;
	rts

dog_metrics_time:
	jsr decimal_clear
	lda metric_time_h
	jsr decimal_add
	ldy dog_now
	lda decimal+1
	ora #$60
	sta dog_data0, Y
	lda decimal+0
	ora #$60
	sta dog_data1, Y
	jsr decimal_clear
	lda metric_time_m
	jsr decimal_add
	ldy dog_now
	lda decimal+1
	ora #$60
	sta dog_data2, Y
	lda decimal+0
	ora #$60
	sta dog_data3, Y
	jsr decimal_clear
	lda metric_time_s
	jsr decimal_add
	ldy dog_now
	lda decimal+1
	ora #$60
	sta dog_data4, Y
	lda decimal+0
	ora #$60
	sta dog_data5, Y
	jsr decimal_clear
	lda metric_time_f
	jsr decimal_add
	ldy dog_now
	lda decimal+1
	ora #$60
	sta dog_data6, Y
	lda decimal+0
	ora #$60
	sta dog_data7, Y
	rts

.segment "DATA"
rush_time: .byte 40, 48, 60

.segment "CODE"

dog_init_metrics:
	jsr dog_metrics_time
	; bones
	PPU_LATCH_AT 17,9
	jsr decimal_clear
	lda metric_bones+0
	sta i
	lda metric_bones+1
	sta j
	lda metric_bones+2
	sta k
	lda metric_bones+3
	sta l
	jsr decimal_add32
	jsr metric_print
	; jumps
	PPU_LATCH_AT 17,11
	jsr decimal_clear
	lda metric_jumps+0
	sta i
	lda metric_jumps+1
	sta j
	lda metric_jumps+2
	sta k
	lda metric_jumps+3
	sta l
	jsr decimal_add32
	jsr metric_print
	; coins
	PPU_LATCH_AT 17,13
	jsr decimal_clear
	jsr coin_count
	jsr decimal_add
	jsr metric_print
	; ending text
	lda dog_param + HOLD_SLOT
	cmp #255
	bcs :+
		rts
	:
	PPU_LATCH_AT 8,21
	lda boss_rush
	beq :+
		lda #TEXT_END_BOSS_RUSH
		jsr text_row_write
	:
	lda end_book
	beq :+
		lda #TEXT_END_BOOK
		jmp :++
	:
		lda #TEXT_END_LOOP
	:
	jsr text_row_write
	lda metric_continue + 0
	ora metric_continue + 1
	ora metric_continue + 2
	ora metric_continue + 3
	beq :+
		lda #TEXT_END_CONTINUED
		jsr text_row_write
	:
	lda metric_cheater
	beq :+
		lda #TEXT_END_CHEATED
		jsr text_row_write
	:
	lda boss_rush
	bne @speedy_text_end
	lda metric_time_h
	bne @speedy_text_end
	ldx #0
	lda player_pal
	beq :+
		ldx #1
	:
	lda easy
	beq :+
		ldx #2
	:
	lda rush_time, X
	sta i
	lda metric_time_m
	cmp i
	bcs @speedy_text_end
		lda end_verso
		bne :+
			lda #TEXT_END_SPEEDY_RECTO
			jmp :++
		:
			lda #TEXT_END_SPEEDY_VERSO
		:
		jsr text_row_write
	@speedy_text_end:
	lda lizard_big_head_mode
	bpl :+
		lda #TEXT_END_BIG_HEAD
		jsr text_row_write
	:
	lda player_pal
	beq :+
		lda #TEXT_END_PAL
		jmp :++
	:
		lda #TEXT_END_NTSC
	:
	jsr text_row_write
	rts
dog_tick_metrics:
	jmp dog_metrics_time
dog_draw_metrics:
	lda #84
	sta u
	lda #135
	sta v
	lda #$02
	sta i
	; draw tiles
	lda dog_data0, Y
	ldx u
	ldy v
	jsr sprite_tile_add
	lda u
	clc
	adc #8
	tax
	stx u
	ldy dog_now
	lda dog_data1, Y
	ldy v
	jsr sprite_tile_add
	lda u
	clc
	adc #16
	tax
	stx u
	ldy dog_now
	lda dog_data2, Y
	ldy v
	jsr sprite_tile_add
	lda u
	clc
	adc #8
	tax
	stx u
	ldy dog_now
	lda dog_data3, Y
	ldy v
	jsr sprite_tile_add
	lda u
	clc
	adc #16
	tax
	stx u
	ldy dog_now
	lda dog_data4, Y
	ldy v
	jsr sprite_tile_add
	lda u
	clc
	adc #8
	tax
	stx u
	ldy dog_now
	lda dog_data5, Y
	ldy v
	jsr sprite_tile_add
	lda u
	clc
	adc #16
	tax
	stx u
	ldy dog_now
	lda dog_data6, Y
	ldy v
	jsr sprite_tile_add
	lda u
	clc
	adc #8
	tax
	stx u
	ldy dog_now
	lda dog_data7, Y
	ldy v
	jsr sprite_tile_add
	rts

; DOG_SUPER_MOOSE
stat_dog_super_moose:

dgd_SUPER_MOOSE_TICK  = dog_data0
dgd_SUPER_MOOSE_FRAME = dog_data1
dgd_SUPER_MOOSE_TALK  = dog_data2

.segment "DATA"
super_moose_anims:
.byte DATA_sprite1_super_moose
.byte 0
.byte DATA_sprite1_super_moose_blink
.byte DATA_sprite1_super_moose
.byte DATA_sprite1_super_moose_blink
.byte 0
.byte DATA_sprite1_super_moose_step
.byte DATA_sprite1_super_moose_step
.byte DATA_sprite1_super_moose
.byte DATA_sprite1_super_moose_low
.byte 0

super_moose_tasks:
.byte $04, $04, $04, $04, $04 ; blink
.byte $02 ; blink 2
.byte $06, $06 ; step

super_moose_text_list:
.byte TEXT_MOOSE0,  TEXT_MOOSE1,  TEXT_MOOSE2,  TEXT_MOOSE3,  TEXT_MOOSE4
.byte TEXT_MOOSE5,  TEXT_MOOSE6,  TEXT_MOOSE7,  TEXT_MOOSE8,  TEXT_MOOSE9
.byte TEXT_MOOSE10, TEXT_MOOSE11, TEXT_MOOSE12, TEXT_MOOSE13, TEXT_MOOSE14
.byte TEXT_MOOSE15, TEXT_MOOSE16, TEXT_MOOSE17, TEXT_MOOSE18, TEXT_MOOSE19
.byte TEXT_MOOSE20, TEXT_MOOSE21, TEXT_MOOSE22, TEXT_MOOSE23, TEXT_MOOSE24
.byte TEXT_MOOSE25, TEXT_MOOSE26, TEXT_MOOSE27, TEXT_MOOSE28
SUPER_MOOSE_TEXT_COUNT = 29
.assert (*-super_moose_text_list)=SUPER_MOOSE_TEXT_COUNT,error,"super_moose_text_list length mismatch!"

.segment "CODE"

dog_init_super_moose:
	; invalidate dog_b2 in case of eyesight
	lda #$FF
	sta chr_cache+3
	jsr prng
	sta dgd_SUPER_MOOSE_TICK, Y
	DOG_BLOCKER -9, -16, 6, -1
	rts
dog_tick_super_moose:
	; death
	DOG_BOUND -7, -16, 4, -1
	jsr lizard_touch
	bne @touch
	DOG_BOUND 4, -23, 13, -17
	jsr lizard_touch
	bne @touch
	jmp @touch_end
	@touch:
		lda current_lizard
		cmp #LIZARD_OF_DEATH
		bne :+
			jsr empty_blocker
			ldx #0
			lda #DATA_sprite0_super_moose_skull
			jmp bones_convert
			;rts
		:
		cmp #LIZARD_OF_HEAT
		bne :+
			; hold on blink 2
			lda #$02
			sta dgd_SUPER_MOOSE_FRAME, Y
			lda #5
			sta dgd_SUPER_MOOSE_TICK, Y
		:
	@touch_end:
	; talk
	lda lizard_dead
	bne @talk_end
	lda lizard_power
	beq @talk_end
	lda current_lizard
	cmp #LIZARD_OF_KNOWLEDGE
	bne @talk_end
	DOG_BOUND 7, -24, 40, -1
	jsr lizard_overlap
	beq @talk_end
		ldx moose_text
		lda super_moose_text_list, X
		sta game_message
		lda moose_text_inc
		bne :+
			jsr prng4
			and #15
			clc
			adc #5
			;lda #1 ; for testing
			sta moose_text_inc
		:
		clc
		adc moose_text
		cmp #SUPER_MOOSE_TEXT_COUNT
		bcc :+
			sec
			sbc #SUPER_MOOSE_TEXT_COUNT
		:
		sta moose_text
		lda #1
		sta game_pause
		rts
	@talk_end:
	; tick countdown
	ldx dgd_SUPER_MOOSE_TICK, Y
	beq :+
		dex
		txa
		sta dgd_SUPER_MOOSE_TICK, Y
		rts
	:
	; advance one frame
	ldx dgd_SUPER_MOOSE_FRAME, Y
	inx
	txa
	sta dgd_SUPER_MOOSE_FRAME, Y
	lda #5
	sta dgd_SUPER_MOOSE_TICK, Y
	lda super_moose_anims, X
	beq :+
		rts
	:
	; end of task reached
	cpx #1
	bne :+
		jsr prng3
		and #7
		tax
		lda super_moose_tasks, X
		sta dgd_SUPER_MOOSE_FRAME, Y
		rts
	:
		lda #0
		sta dgd_SUPER_MOOSE_FRAME, Y
		jsr prng4
		and #127
		clc
		adc #90
		sta dgd_SUPER_MOOSE_TICK, Y
		rts
	;
dog_draw_super_moose:
	DX_SCROLL_EDGE
	lda dgd_SUPER_MOOSE_FRAME, Y
	tay
	lda super_moose_anims, Y
	pha
	ldy dog_now
	lda dog_y, Y
	tay
	pla
	jmp sprite1_add
	;rts

; DOG_BRAD_DUNGEON
stat_dog_brad_dungeon:

dog_init_brad_dungeon:
dog_tick_brad_dungeon:
dog_draw_brad_dungeon:
	; NOT IN DEMO
	rts

; DOG_BRAD
stat_dog_brad:

dog_init_brad:
dog_tick_brad:
dog_draw_brad:
	; NOT IN DEMO
	rts

; DOG_HEEP_HEAD
stat_dog_heep_head:

dog_init_heep_head:
dog_tick_heep_head:
dog_draw_heep_head:
	; NOT IN DEMO
	rts

; DOG_HEEP
stat_dog_heep:

dog_init_heep:
dog_tick_heep:
dog_draw_heep:
	; NOT IN DEMO
	rts

; DOG_HEEP_TAIL
stat_dog_heep_tail:

dog_init_heep_tail:
dog_tick_heep_tail:
dog_draw_heep_tail:
	; NOT IN DEMO
	rts

; DOG_LAVA_LEFT
stat_dog_lava_left:

dog_bound_lava:
	DOG_BOUND 0,0,31,7
	rts
dog_init_lava_left:
	rts
dog_tick_lava_left:
	jsr dog_bound_lava
dog_tick_lava_left_common:
	jsr lizard_overlap
	bne :+
		rts
	:
	; flow left
	lda #1
	sta lizard_flow
lava_flow_common_kill:
	; stone is not killed by lava
	lda current_lizard
	cmp #LIZARD_OF_STONE
	bne :+
	lda lizard_power
	beq :+
		rts
	:
	jsr lizard_die
	rts
dog_draw_lava_left:
	rts

; DOG_LAVA_RIGHT
stat_dog_lava_right:

dog_init_lava_right:
	rts
dog_tick_lava_right:
	jsr dog_bound_lava
dog_tick_lava_right_common:
	jsr lizard_overlap
	bne :+
		rts
	:
	; flow right
	lda #2
	sta lizard_flow
	jmp lava_flow_common_kill
dog_draw_lava_right:
	rts

; DOG_LAVA_LEFT_WIDE
stat_dog_lava_left_wide:

dog_bound_lava_wide:
	DOG_BOUND 0,0,71,7
	rts
dog_init_lava_left_wide:
	rts
dog_tick_lava_left_wide:
	jsr dog_bound_lava_wide
	jmp dog_tick_lava_left_common
dog_draw_lava_left_wide:
	rts

; DOG_LAVA_RIGHT_WIDE
stat_dog_lava_right_wide:

dog_init_lava_right_wide:
	rts
dog_tick_lava_right_wide:
	jsr dog_bound_lava_wide
	jmp dog_tick_lava_right_common
dog_draw_lava_right_wide:
	rts

; DOG_LAVA_LEFT_WIDER
stat_dog_lava_left_wider:

dog_bound_lava_wider:
	DOG_BOUND 0,0,119,7
	rts
dog_init_lava_left_wider:
	rts
dog_tick_lava_left_wider:
	jsr dog_bound_lava_wider
	jmp dog_tick_lava_left_common
dog_draw_lava_left_wider:
	rts

; DOG_LAVA_RIGHT_WIDER
stat_dog_lava_right_wider:

dog_init_lava_right_wider:
	rts
dog_tick_lava_right_wider:
	jsr dog_bound_lava_wider
	jmp dog_tick_lava_right_common
dog_draw_lava_right_wider:
	rts

; DOG_LAVA_DOWN
stat_dog_lava_down:

dog_init_lava_down:
	rts
dog_tick_lava_down:
	DOG_BOUND 0,0,7,23
	jsr lizard_overlap
	beq @overlap_end
		lda current_lizard
		cmp #LIZARD_OF_STONE
		bne @die
		lda lizard_power
		bne @not_die
		@die:
			jsr lizard_die
			rts
		;
		@not_die:
			DOG_BOUND 3,0,4,23
			jsr lizard_overlap
			beq :+
				lda #0
				sta lizard_vx+0
				sta lizard_vx+1
			:
			rts
		;
	@overlap_end:
	rts
dog_draw_lava_down:
	rts

; DOG_LAVA_POOP
stat_dog_lava_poop:

dgd_LAVA_POOP_FRAME = dog_data0
dgd_LAVA_POOP_CYCLE = dog_data1
dgd_LAVA_POOP_WAIT  = dog_data2
dog_init_lava_poop:
	rts
dog_tick_lava_poop:
	; palette
	tya
	pha ; temporarily store Y
	tax
	inc dgd_LAVA_POOP_FRAME, X
	lda dgd_LAVA_POOP_FRAME, Y
	cmp #5
	bcc @palette_end
		lda #0
		sta dgd_LAVA_POOP_FRAME, Y
		inc dgd_LAVA_POOP_CYCLE, X
		lda dgd_LAVA_POOP_CYCLE, Y
		cmp #3
		bcc :+
			lda #0
			sta dgd_LAVA_POOP_CYCLE, Y
		:
		clc
		adc #DATA_palette_lava0
		tax
		lda #7
		jsr palette_load
	@palette_end:
	pla
	tay ; restore Y
	; wait
	lda dgd_LAVA_POOP_WAIT, Y
	beq :+
		sec
		sbc #1
		sta dgd_LAVA_POOP_WAIT, Y
		rts
	:
	; move
	lda dog_y, Y
	clc
	adc #4
	cmp #144
	bcc :+
	cmp #240
	bcs :+
		lda #240
		sta dog_y, Y
		jsr prng
		and #31
		sta dgd_LAVA_POOP_WAIT, Y
		rts
	:
	sta dog_y, Y
	; not fully onscreen, don't collide yet
	;cmp #240
	bcc :+
		rts
	:
	; collide
	DOG_BOUND -3,3,2,23
	jsr lizard_overlap_no_stone
	beq :+
		jsr lizard_die
	:
	rts
dog_draw_lava_poop:
	lda dgd_LAVA_POOP_WAIT, Y
	beq :+
		rts
	:
	; scroll
	DX_SCROLL
	stx j ; temporarily store dx
	; splash
	lda dog_y, Y
	cmp #121
	bcc :+
	cmp #240
	bcs :+
		; if dgy >= 121 && dgy < 240
		lda j
		pha ; temporarily store j
		tax
		ldy #0
		lda #DATA_sprite1_lava_poop_splash
		jsr sprite1_add
		pla
		sta j
	:
	; dx -= 4
	lda j
	sec
	sbc #4
	bcs :+
		rts
	:
	sta j
	; tile 1
	ldy dog_now
	lda #$23
	sta i
	ldx j
	lda dog_y, Y
	tay
	lda #$D5
	jsr sprite_tile_add
	; tile 2
	ldy dog_now
	lda dog_y, Y
	cmp #136
	bcc :+
	cmp #240
	bcs :+
	jmp @tile2_end
	:
		; if dgy < 136 || dgy >= 240
		ldx j
		clc
		adc #8
		tay
		lda #$D6
		; i = $23
		jsr sprite_tile_add
		ldy dog_now
		lda dog_y, Y
	@tile2_end:
	;ldy dog_now
	;lda dog_y, Y
	cmp #128
	bcc :+
	cmp #240
	bcs :+
	jmp @tile3_end
	:
		; if dgy < 128 || dgy >= 240
		ldx j
		clc
		adc #16
		tay
		lda #$D7
		; i = $23
		jsr sprite_tile_add
		ldy dog_now
		lda dog_y, Y
	@tile3_end:
	rts

; DOG_LAVA_MOUTH

MAX_DOWN_LAVA_MOUTH = 255
MIN_DOWN_LAVA_MOUTH = -60
ACCEL_LAVA_MOUTH = 65

dog_init_lava_mouth:
	rts
dog_tick_lava_mouth:
	DOG_BOUND 0,0,31,143
	jsr lizard_overlap
	bne :+
		rts
	:
	lda lizard_power
	beq @die
	lda current_lizard
	cmp #LIZARD_OF_STONE
	bne @die
	@not_die:
		; if lizard.vy >= MIN_DOWN_LAVA_MOUTH
		lda lizard_vy+1
		cmp #<MIN_DOWN_LAVA_MOUTH
		lda lizard_vy+0
		sbc #>MIN_DOWN_LAVA_MOUTH
		bvc :+
		eor #$80
		:
		bmi @end_not_die
		; lizard.vy -= ACCEL_LAVA_MOUTH
		lda lizard_vy+1
		sec
		sbc #<ACCEL_LAVA_MOUTH
		sta lizard_vy+1
		lda lizard_vy+0
		sbc #>ACCEL_LAVA_MOUTH
		sta lizard_vy+0
		; if lizard.vy >= MAX_DOWN_LAVA_MOUTH
		lda lizard_vy+1
		cmp #<MAX_DOWN_LAVA_MOUTH
		lda lizard_vy+0
		sbc #>MAX_DOWN_LAVA_MOUTH
		bvc :+
		eor #$80
		:
		bmi :+
			lda #<MAX_DOWN_LAVA_MOUTH
			sta lizard_vy+1
			lda #>MAX_DOWN_LAVA_MOUTH
			sta lizard_vy+0
		:
		@end_not_die:
		rts
	@die:
		jmp lizard_die
	;
dog_draw_lava_mouth:
	rts

; ======
; BOSSES
; ======

.segment "DATA"
boss_talk_text:
.byte TEXT_BUNNY0     ;  0
.byte TEXT_BUNNY1     ;  1
.byte TEXT_RACCOON0   ;  2
.byte TEXT_RACCOON1   ;  3
.byte TEXT_RACCOON2   ;  4
.byte TEXT_RACCOON3   ;  5
.byte TEXT_FROB0      ;  6
.byte TEXT_BOSSTOPUS0 ;  7
.byte TEXT_BOSSTOPUS1 ;  8
.byte TEXT_CAT0       ;  9
.byte TEXT_SNEK0      ; 10
.byte TEXT_SNEK1      ; 11

boss_talk_offset:
.byte  0 ; BUNNY
.byte 10 ; SNEK
.byte  7 ; BOSSTOPUS
.byte  2 ; RACCOON
.byte  6 ; FROB
.byte  9 ; CAT

boss_talk_count:
.byte 2 ; BUNNY
.byte 2 ; SNEK
.byte 2 ; BOSSTOPUS
.byte 4 ; RACCOON
.byte 1 ; FROB
.byte 1 ; CAT

.segment "CODE"

; carry set if true
dog_talk_test:
	lda lizard_power
	beq @talk_no
	lda current_lizard
	cmp #LIZARD_OF_KNOWLEDGE
	bne @talk_no
	lda lizard_dead
	bne @talk_no
	sec
	rts
@talk_no:
	clc
	rts


; X = boss index (clobbered)
dog_boss_talk_common:
	jsr dog_talk_test
	bcs :+
		rts
	:
dog_boss_talk_execute:
	; text for boss talking
	lda boss_talk
	cmp boss_talk_count, X
	bcs @talk_end
	clc
	adc boss_talk_offset, X
	tax
	lda boss_talk_text, X
	inc boss_talk
	; do message
	sta game_message
	lda #1
	sta game_pause
@talk_end:
	rts

; DOG_BOSSTOPUS
stat_dog_bosstopus:

dog_init_bosstopus:
dog_tick_bosstopus:
dog_draw_bosstopus:
	; NOT IN DEMO
	rts

; DOG_BOSSTOPUS_EGG
stat_dog_bosstopus_egg:

dog_init_bosstopus_egg:
dog_tick_bosstopus_egg:
dog_draw_bosstopus_egg:
	; NOT IN DEMO
	rts

; DOG_CAT
stat_dog_cat:

dog_init_cat:
dog_tick_cat:
dog_draw_cat:
	; NOT IN DEMO
	rts

; DOG_CAT_SMILE
stat_dog_cat_smile:

dog_init_cat_smile:
dog_tick_cat_smile:
dog_draw_cat_smile:
	; NOT IN DEMO
	rts

; DOG_CAT_SPARKLE
stat_dog_cat_sparkle:

dog_init_cat_sparkle:
dog_tick_cat_sparkle:
dog_draw_cat_sparkle:
	; NOT IN DEMO
	rts

; DOG_CAT_STAR
stat_dog_cat_star:

dog_init_cat_star:
dog_tick_cat_star:
dog_draw_cat_star:
	; NOT IN DEMO
	rts

; DOG_RACCOON
stat_dog_raccoon:

dog_init_raccoon:
dog_tick_raccoon:
dog_draw_raccoon:
	; NOT IN DEMO
	rts

; DOG_RACCOON_LAUNCHER
stat_dog_raccoon_launcher:

dog_init_raccoon_launcher:
dog_tick_raccoon_launcher:
dog_draw_raccoon_launcher:
	; NOT IN DEMO
	rts

; DOG_RACCOON_LAVABALL
stat_dog_raccoon_lavaball:

dog_init_raccoon_lavaball:
dog_tick_raccoon_lavaball:
dog_draw_raccoon_lavaball:
	; NOT IN DEMO
	rts

; DOG_RACCOON_VALVE
stat_dog_raccoon_valve:

dog_init_raccoon_valve:
dog_tick_raccoon_valve:
dog_draw_raccoon_valve:
	; NOT IN DEMO
	rts

; DOG_FROB
stat_dog_frob:

dog_init_frob:
dog_tick_frob:
dog_draw_frob:
	; NOT IN DEMO
	rts

; DOG_FROB_HAND_LEFT
stat_dog_frob_hand_left:

dog_init_frob_hand_left:
dog_tick_frob_hand_left:
dog_draw_frob_hand_left:
	; NOT IN DEMO
	rts

; DOG_FROB_HAND_RIGHT
stat_dog_frob_hand_right:

dog_init_frob_hand_right:
dog_tick_frob_hand_right:
dog_draw_frob_hand_right:
	; NOT IN DEMO
	rts

; DOG_FROB_ZAP
stat_dog_frob_zap:

dog_init_frob_zap:
dog_tick_frob_zap:
dog_draw_frob_zap:
	; NOT IN DEMO
	rts

; DOG_FROB_TONGUE
stat_dog_frob_tongue:

dog_init_frob_tongue:
dog_tick_frob_tongue:
dog_draw_frob_tongue:
	; NOT IN DEMO
	rts

; DOG_FROB_BLOCK
stat_dog_frob_block:

dog_init_frob_block:
dog_tick_frob_block:
dog_draw_frob_block:
	; NOT IN DEMO
	rts

; DOG_FROB_PLATFORM
stat_dog_frob_platform:

dog_init_frob_platform:
dog_tick_frob_platform:
dog_draw_frob_platform:
	; NOT IN DEMO
	rts

; DOG_QUEEN
stat_dog_queen:

dog_init_queen:
dog_tick_queen:
dog_draw_queen:
	; NOT IN DEMO
	rts

; DOG_HARE
stat_dog_hare:

dgd_HARE_ANIM   = dog_data0
dgd_HARE_MODE   = dog_data1
dgd_HARE_MOOD   = dog_data2
dgd_HARE_Y1     = dog_data3
dgd_HARE_VY0    = dog_data4
dgd_HARE_VY1    = dog_data5
dgd_HARE_VX     = dog_data6
dgd_HARE_HEAT   = dog_data7
dgd_HARE_SPRITE = dog_data8
dgd_HARE_FLIP   = dog_data9
dgd_HARE_HP     = dog_data10
dgd_HARE_SIT    = dog_data11
dgd_HARE_HOPS   = dog_data12

.enum
	HARE_MODE_IDLE      = 0
	HARE_MODE_HOP       = 1
	HARE_MODE_FLY       = 2
	HARE_MODE_SKID      = 3
	HARE_MODE_TURN      = 4
	HARE_MODE_STUN      = 5
	HARE_MODE_SIT       = 6
	HARE_MODE_GET_UP    = 7
	HARE_MODE_HIDE      = 8
.endenum

.enum
	HARE_MOOD_ENTER  = 0
	HARE_MOOD_CHASE  = 1
	HARE_MOOD_ESCAPE = 2
	HARE_MOOD_ASCEND = 3
	HARE_MOOD_STOMPL = 4
	HARE_MOOD_STOMPR = 5
.endenum

HARE_HP_MAX        = 4
HARE_FIRE_STRENGTH = 25
HARE_HEAT_STRENGTH = 8

HARE_HOP_COUNT = 6
HARE_GRAVITY = 50

.segment "DATA"
hare_hop_vy: .word  -345,  -900, -1290,  -575,     0,  -345 ; initial Y velocity
hare_hop_vx: .byte     2,     2,     2,     2,     2,     2 ; X speed (pixels per second)
hare_hop_sk: .byte     0,    10,    16,     0,    10,     0 ; frames of post-jump skid
hare_hop_s0: .byte   255,     5,     8,   255,   255,   255 ; frames of slight squat
hare_hop_s1: .byte   255,    37,    40,   255,   255,   255 ; frames of deep squat
hare_hop_jp: .byte    28,    42,    48,     8,     8,     4 ; frame of jump
.segment "CODE"

HARE_IDLE = 6
HARE_TURN = 8
HARE_HIDE = 60
HARE_CHASE_TIMEOUT = 25

HARE_ESCAPE_L = 385
HARE_ESCAPE_R = 421
HARE_ESCAPE_M = (HARE_ESCAPE_L + HARE_ESCAPE_R) / 2

HARE_SPLIT_L = 57+2
HARE_SPLIT_H = 95-2
HARE_SPLUT_L = ($FFFF - HARE_SPLIT_L) + 1
HARE_SPLUT_H = ($FFFF - HARE_SPLIT_H) + 1

HAREBURN_X = 68
HAREBURN_Y = 144
HAREBURN_W = 39
HAREBURN_H = 23

dog_init_hare:
	lda #HARE_MODE_HIDE
	sta dgd_HARE_MODE + HARE_SLOT
	lda #HARE_MOOD_ENTER
	sta dgd_HARE_MOOD + HARE_SLOT
	rts

dog_bound_hare:
	ldy #HARE_SLOT
	DOG_BOUND -14, -29, 13, -1
	rts

; return A!=0 (!Z) if collided with ground
dog_fly_hare:
	; calculate new Y position
	lda dgd_HARE_Y1 + HARE_SLOT
	clc
	adc dgd_HARE_VY1 + HARE_SLOT
	; low bits of Y always pass through
	sta dgd_HARE_Y1 + HARE_SLOT
	; high bits become a temporary target (m)
	lda dog_y + HARE_SLOT
	adc dgd_HARE_VY0 + HARE_SLOT
	sta m ; m = yt8
	sec
	sbc dog_y + HARE_SLOT
	sta v
	; horizontal offset is fixed, not accelerating
	lda dgd_HARE_VX + HARE_SLOT
	sta u
	; update velocity for gravity
	lda dgd_HARE_VY1 + HARE_SLOT
	clc
	adc #<HARE_GRAVITY
	sta dgd_HARE_VY1 + HARE_SLOT
	lda dgd_HARE_VY0 + HARE_SLOT
	adc #>HARE_GRAVITY
	sta dgd_HARE_VY0 + HARE_SLOT
	; move normally
	lda dog_x + HARE_SLOT
	cmp #<32
	lda dog_xh + HARE_SLOT
	sbc #>32
	bcc @move_outer
	lda dog_x + HARE_SLOT
	cmp #<488
	lda dog_xh + HARE_SLOT
	sbc #>488
	bcs @move_outer
@move_inner:
	jsr dog_bound_hare
	jsr move_dog
	jmp @move_end
@move_outer:
	lda v
	beq @move_end
	bmi @move_end
	lda #>511
	sta ih
	ldx #<511
	ldy m
	jsr collide_all_down
	; v -= collide result
	sta w
	lda v
	sec
	sbc w
	sta v
@move_end:
	; apply the collided move
	ldy #HARE_SLOT
	jsr move_finish ; dgx + u, dgy + v
	; HARE_Y1 is set earlier
	lda dog_y + HARE_SLOT
	cmp m
	bcs :+
		; landed
		lda #DATA_sprite2_hare
		sta dgd_HARE_SPRITE + HARE_SLOT
		lda #1
		rts ; return true
	:
	; select sprite
	lda dgd_HARE_ANIM + HARE_SLOT
	lsr
	lsr
	and #1
	sta s
	lda dgd_HARE_MODE + HARE_SLOT
	cmp #HARE_MODE_STUN
	bne :+
		.assert DATA_sprite2_hare_back1 = DATA_sprite2_hare_back0 + 1, error, "hare_back sprites out of order!"
		lda #DATA_sprite2_hare_back0
		jmp @finish
	:
	lda dgd_HARE_VY0 + HARE_SLOT
	bpl :+
		.assert DATA_sprite2_hare_leap1 = DATA_sprite2_hare_leap0 + 1, error, "hare_leap sprites out of order!"
		lda #DATA_sprite2_hare_leap0
		jmp @finish
	:
		.assert DATA_sprite2_hare_fall1 = DATA_sprite2_hare_fall0 + 1, error, "hare_fall sprites out of order!"
		lda #DATA_sprite2_hare_fall0
		;jmp @finish
	;
@finish:
	clc
	adc s
	sta dgd_HARE_SPRITE + HARE_SLOT
	lda #0
	rts ; return false

; A = warm amount
; clobbers w
dog_warm_hare:
	clc
	adc dgd_HARE_HEAT + HARE_SLOT
	sta w
	lda dgd_HARE_HEAT + HARE_SLOT
	cmp w
	php
	lda w
	plp
	bcc :+
		lda #255
	:
	sta dgd_HARE_HEAT + HARE_SLOT
	rts

dog_hit_hare:
	lda dgd_HARE_HP + HARE_SLOT
	cmp #HARE_HP_MAX
	bcs :+
		PLAY_SOUND SOUND_SAD
		inc dgd_HARE_HP + HARE_SLOT
	:
	lda dgd_HARE_HP + HARE_SLOT
	cmp #HARE_HP_MAX
	bcc :+
		lda #MUSIC_SILENT
		sta player_next_music
		lda #HARE_HP_MAX
		sta dgd_HARE_HP + HARE_SLOT
	:
	rts

; A = throw type
dog_throw_hare:
	sta dog_param + HARE_SLOT
	lda #0
	sta dgd_HARE_ANIM + HARE_SLOT
	sta dgd_HARE_Y1 + HARE_SLOT
	lda #HARE_MODE_FLY
	sta dgd_HARE_MODE + HARE_SLOT
	lda dgd_HARE_FLIP + HARE_SLOT
	asl
	asl
	sec
	sbc #2
	sta dgd_HARE_VX + HARE_SLOT
	; set velocity
	lda dog_param + HARE_SLOT
	asl
	tax
	lda hare_hop_vy+0, X
	sta i
	lda hare_hop_vy+1, X
	sta ih
	lda dgd_HARE_MOOD + HARE_SLOT
	cmp #HARE_MOOD_STOMPL
	beq @randomize
	cmp #HARE_MOOD_STOMPR
	bne @randomize_end
	@randomize:
		jsr prng
		and #127
		sta r
		lda i
		sec
		sbc r
		sta i
		lda ih
		sbc #0
		sta ih
	@randomize_end:
	lda i
	sta dgd_HARE_VY1 + HARE_SLOT
	lda ih
	sta dgd_HARE_VY0 + HARE_SLOT
	rts

dog_tick_hare:
	; dissipate heat
	lda dgd_HARE_HEAT + HARE_SLOT
	beq :+
		dec dgd_HARE_HEAT + HARE_SLOT
	:
	; touch fire?
	HAREBURN_BX = HAREBURN_X + HAREBURN_W + 14 + 1
	lda dog_x + HARE_SLOT
	cmp #<HAREBURN_BX
	lda dog_xh + HARE_SLOT
	sbc #>HAREBURN_BX
	bcs :+
	lda dog_y + HARE_SLOT
	cmp #144
	bcc :+
		lda #HARE_FIRE_STRENGTH
		jsr dog_warm_hare
	:
	jsr dog_bound_hare
	jsr lizard_touch
	beq @touch_end
		lda current_lizard
		cmp #LIZARD_OF_HEAT
		bne :+
			lda #HARE_HEAT_STRENGTH
			jsr dog_warm_hare
			jmp @touch_end
		:
		cmp #LIZARD_OF_DEATH
		bne :+
			lda #MUSIC_SILENT
			sta player_next_music
			lda #DATA_sprite0_hare_skull
			ldx dgd_HARE_FLIP + HARE_SLOT
			ldy #HARE_SLOT
			jmp bones_convert
			;rts
		;
	@touch_end:
	jsr dog_bound_hare
	jsr lizard_overlap_no_stone
	beq :+
		jsr lizard_die
	:
	; talk
	lda dog_y + HARE_SLOT
	bpl :+
	lda dgd_HARE_MODE + HARE_SLOT
	cmp #HARE_MODE_HIDE
	beq :+
		ldx #0
		jsr dog_boss_talk_common
	:
	; animate
	inc dgd_HARE_ANIM + HARE_SLOT
;
; hare modes
;
	lda dgd_HARE_MODE + HARE_SLOT
;
; HARE_MODE_IDLE
;
	cmp #HARE_MODE_IDLE
	jne hare_mode_idle_end
hare_mode_idle:
	jsr hare_sit_test
	;
	lda dgd_HARE_ANIM + HARE_SLOT
	cmp #HARE_IDLE
	bcc :+
		; IDLE finished, proceed to HOP unless already interrupted by MOOD
		lda #0
		sta dgd_HARE_ANIM + HARE_SLOT
		lda #HARE_MODE_HOP
		sta dgd_HARE_MODE + HARE_SLOT
		rts
	:
	cmp #2
	bcc :+
		; idle for a moment before doing anything
		rts
	:
	; first frame of IDLE, realize the current MOOD
	lda #1
	sta dgd_HARE_ANIM + HARE_SLOT
	; randomly select blink or stand sprite
	jsr prng
	and #3
	bne :+
		lda #DATA_sprite2_hare_blink
		jmp :++
	:
		lda #DATA_sprite2_hare
	:
	sta dgd_HARE_SPRITE + HARE_SLOT
	; mood will set hop target i:ih = tx
	lda #0
	sta i
	sta ih
	sta dog_param + HARE_SLOT ; hop parameter
	lda dgd_HARE_MOOD + HARE_SLOT
	cmp #HARE_MOOD_ENTER
	bne @mood_enter_end
	@mood_enter:
		lda #<120
		sta i
		;lda #>120
		;sta ih
		lda #1
		sta dog_param + HARE_SLOT
		lda dog_y + HARE_SLOT
		cmp #176
		bcs :+
			jmp @mood_finish ; run on top area
		:
			lda #HARE_MOOD_CHASE
			sta dgd_HARE_MOOD + HARE_SLOT
			lda #0
			sta dgd_HARE_HOPS + HARE_SLOT
			jmp @mood_chase
		;
	@mood_enter_end:
	cmp #HARE_MOOD_CHASE
	jne @mood_chase_end
	@mood_chase:
		lda dgd_HARE_HOPS + HARE_SLOT
		cmp #HARE_CHASE_TIMEOUT
		bcc :+
			; too many hops, proceed to ESCAPE
			lda #HARE_MOOD_ESCAPE
			sta dgd_HARE_MOOD + HARE_SLOT
			jmp @mood_escape
		:
			inc dgd_HARE_HOPS + HARE_SLOT
			; target the lizard
			lda lizard_x+0
			sta i
			lda lizard_xh
			sta ih
			; if dead, don't jump into the fire (as much)
			lda lizard_dead
			beq :+
			lda i
			cmp #<103
			lda ih
			sbc #>103
			bcs :+
				lda #<103
				sta i
				lda #>103
				sta ih
			:
			lda #0
			sta dog_param + HARE_SLOT
			; long jump in specific range:
			lda i
			sec
			sbc dog_x + HARE_SLOT
			sta j
			lda ih
			sbc dog_xh + HARE_SLOT
			sta jh
			; j:jh = dtx = tx - dgx
			bcs @chase_target_right
			@chase_target_left:
				lda j
				cmp #<HARE_SPLUT_L
				lda jh
				sbc #>HARE_SPLUT_L
				jcs @mood_finish
				lda j
				cmp #<HARE_SPLUT_H
				lda jh
				sbc #>HARE_SPLUT_H
				jcc @mood_finish
					jmp @chase_target_long
				;
			@chase_target_right:
				lda j
				cmp #<HARE_SPLIT_L
				lda jh
				sbc #>HARE_SPLIT_L
				jcc @mood_finish
				lda j
				cmp #<HARE_SPLIT_H
				lda jh
				sbc #>HARE_SPLIT_H
				jcs @mood_finish
					;jmp @chase_target_long
				;
			@chase_target_long:
				; do long hop
				lda #1
				sta dog_param + HARE_SLOT
			;
			jmp @mood_finish
		;
	@mood_chase_end:
	cmp #HARE_MOOD_ESCAPE
	bne @mood_escape_end
	@mood_escape:
		lda #<HARE_ESCAPE_M
		sta i
		lda #>HARE_ESCAPE_M
		sta ih
		lda #5
		sta dog_param + HARE_SLOT
		; if in target range, progress to ASCEND
		lda dog_x + HARE_SLOT
		cmp #<HARE_ESCAPE_L
		lda dog_xh + HARE_SLOT
		sbc #>HARE_ESCAPE_L
		bcc :+
		lda dog_x + HARE_SLOT
		cmp #<HARE_ESCAPE_R
		lda dog_xh + HARE_SLOT
		sbc #>HARE_ESCAPE_R
		bcs :+
			lda #HARE_MOOD_ASCEND
			sta dgd_HARE_MOOD + HARE_SLOT
			jmp @mood_ascend
		:
		jmp @mood_finish
	@mood_escape_end:
	cmp #HARE_MOOD_ASCEND
	bne @mood_ascend_end
	@mood_ascend:
		lda #<(512+100)
		sta i
		lda #>(512+100)
		sta ih
		lda #2
		sta dog_param + HARE_SLOT
		; if on high ground, do a shorter jump
		lda #120
		cmp dog_y + HARE_SLOT
		bcc :+
			lda #3
			sta dog_param + HARE_SLOT
		:
		jmp @mood_finish
	@mood_ascend_end:
	cmp #HARE_MOOD_STOMPL
	bne @mood_stompl_end
	@mood_stompl:
		lda #<120
		sta i
		;lda #>120
		;sta ih
		lda #5
		sta dog_param + HARE_SLOT
		; turn around if moved far enough left
		lda dog_x + HARE_SLOT
		cmp #<120
		lda dog_xh + HARE_SLOT
		sbc #>120
		bcc :+
			jmp @mood_finish
		:
			lda #HARE_MOOD_STOMPR
			sta dgd_HARE_MOOD + HARE_SLOT
			jmp @mood_stompr
		;
	@mood_stompl_end:
	cmp #HARE_MOOD_STOMPR
	bne @mood_stompr_end
	@mood_stompr:
		lda #<(512+100)
		sta i
		lda #>(512+100)
		sta ih
		lda #5
		sta dog_param + HARE_SLOT
		jmp @mood_finish
	@mood_stompr_end:
	jmp @mood_enter ; default
@mood_finish:
	; turn to face target
	lda dog_x + HARE_SLOT
	cmp i
	lda dog_xh + HARE_SLOT
	sbc ih
	bcs @turn_right
	@turn_left:
		lda dgd_HARE_FLIP + HARE_SLOT
		beq @turn_do
		jmp @turn_end
	@turn_right:
		lda dgd_HARE_FLIP + HARE_SLOT
		beq @turn_end
		;jmp @turn_do
	@turn_do:
		lda #0
		sta dgd_HARE_ANIM + HARE_SLOT
		lda #HARE_MODE_TURN
		sta dgd_HARE_MODE + HARE_SLOT
		jmp hare_mode_turn
	@turn_end:
	rts
hare_mode_idle_end:
;
; HARE_MODE_HOP
;
	cmp #HARE_MODE_HOP
	bne hare_mode_hop_end
hare_mode_hop:
	jsr hare_sit_test
	ldx dog_param + HARE_SLOT
	lda dgd_HARE_ANIM + HARE_SLOT
	cmp hare_hop_jp, X
	bcc :+
		txa
		jsr dog_throw_hare
		PLAY_SOUND SOUND_HARE
		rts
	:
	cmp hare_hop_s0, X
	bcc @squat0
	cmp hare_hop_s1, X
	bcc :+
	@squat0:
		lda #DATA_sprite2_hare_squat0
		sta dgd_HARE_SPRITE + HARE_SLOT
		rts
	:
		cmp hare_hop_s0, X
		bne :+
			PLAY_SOUND SOUND_GOAT_JUMP
		:
		lda #DATA_sprite2_hare_squat1
		sta dgd_HARE_SPRITE + HARE_SLOT
		rts
	;
hare_mode_hop_end:
;
; HARE_MODE_FLY
;
	cmp #HARE_MODE_FLY
	bne hare_mode_fly_end
hare_mode_fly:
	jsr dog_fly_hare
	beq hare_mode_evaluate_hide
		lda #HARE_MODE_SKID
		sta dgd_HARE_MODE + HARE_SLOT
		lda #0
		sta dgd_HARE_ANIM + HARE_SLOT
		lda dog_y + HARE_SLOT
		cmp #72
		bcs :+
			ldy #HARE_SLOT
			jsr dog_harecicle_spawn
		:
	;
hare_mode_evaluate_hide:
	lda dog_x + HARE_SLOT
	cmp #<16
	lda dog_xh + HARE_SLOT
	sbc #>16
	bcc @hide_end
	lda dog_x + HARE_SLOT
	cmp #<40
	lda dog_xh + HARE_SLOT
	sbc #>40
	bcs @hide_end
		lda #HARE_MODE_HIDE
		sta dgd_HARE_MODE + HARE_SLOT
		lda #0
		sta dgd_HARE_ANIM + HARE_SLOT
		lda dgd_HARE_MOOD + HARE_SLOT
		cmp #HARE_MOOD_ASCEND
		bne :+
			lda #HARE_MOOD_STOMPL
			jmp :++
		:
			lda #HARE_MOOD_ENTER
		:
		sta dgd_HARE_MOOD + HARE_SLOT
	@hide_end:
	rts
hare_mode_fly_end:
;
; HARE_MODE_SKID
;
	cmp #HARE_MODE_SKID
	bne hare_mode_skid_end
hare_mode_skid:
	ldx dog_param + HARE_SLOT
	lda dgd_HARE_ANIM + HARE_SLOT
	cmp hare_hop_sk, X
	bcc :+
		; finished skid
		lda #0
		sta dgd_HARE_ANIM + HARE_SLOT
		lda #HARE_MODE_IDLE
		sta dgd_HARE_MODE + HARE_SLOT
		rts
	:
		; skid move
		lda #0
		sta v
		lda dgd_HARE_FLIP + HARE_SLOT
		beq :+
			lda #1
			jmp :++
		:
			lda #-1
		:
		sta u
		jsr dog_bound_hare
		jsr move_dog
		jsr move_finish
		; sprite
		.assert DATA_sprite2_hare_skid1 = DATA_sprite2_hare_skid0 + 1, error, "hare_skid sprites out of order!"
		lda dgd_HARE_ANIM + HARE_SLOT
		lsr
		lsr
		and #1
		clc
		adc #DATA_sprite2_hare_skid0
		sta dgd_HARE_SPRITE + HARE_SLOT
		jmp hare_mode_evaluate_hide
	;rts
hare_mode_skid_end:
;
; HARE_MODE_TURN
;
	cmp #HARE_MODE_TURN
	bne hare_mode_turn_end
hare_mode_turn:
	jsr hare_sit_test
	lda #DATA_sprite2_hare_turn
	sta dgd_HARE_SPRITE + HARE_SLOT
	lda dgd_HARE_ANIM + HARE_SLOT
	cmp #HARE_TURN
	bne :+
		lda #1
		eor dgd_HARE_FLIP + HARE_SLOT
		sta dgd_HARE_FLIP + HARE_SLOT
		rts
	:
	cmp #HARE_TURN*2
	bcc :+
		lda #0
		sta dgd_HARE_ANIM + HARE_SLOT
		lda #HARE_MODE_IDLE
		sta dgd_HARE_MODE + HARE_SLOT
	:
	rts
hare_mode_turn_end:
;
; HARE_MODE_STUN
;
	cmp #HARE_MODE_STUN
	bne hare_mode_stun_end
hare_mode_stun:
	jsr dog_fly_hare
	beq :+
		lda #HARE_MODE_SIT
		sta dgd_HARE_MODE + HARE_SLOT
		lda #0
		sta dgd_HARE_ANIM + HARE_SLOT
		jmp hare_mode_sit
	:
	rts
;
; a little convenient function to move to mode_sit if overheated
;
hare_sit_test:
	lda dgd_HARE_HEAT + HARE_SLOT
	cmp #255
	bcc :+
		lda #0
		sta dgd_HARE_ANIM + HARE_SLOT
		sta dgd_HARE_SIT + HARE_SLOT
		lda #HARE_MODE_SIT
		sta dgd_HARE_MODE + HARE_SLOT
		jsr dog_hit_hare
		; remove callee from the stack and go directly to mode_sit
		pla
		pla
		jmp hare_mode_sit
	:
	rts
hare_mode_stun_end:
;
; HARE_MODE_SIT
;
	cmp #HARE_MODE_SIT
	jne hare_mode_sit_end
hare_mode_sit:
	; stay out of fire
	.assert HAREBURN_BX = HAREBURN_X + HAREBURN_W + 14 + 1, error, "check definition of HAREBURN_BX"
	lda dog_x + HARE_SLOT
	cmp #<HAREBURN_BX
	lda dog_xh + HARE_SLOT
	sbc #>HAREBURN_BX
	bcs :+
		lda #0
		jsr dog_throw_hare
		lda #0
		sta dgd_HARE_FLIP + HARE_SLOT
		lda #2
		sta dgd_HARE_VX + HARE_SLOT
		lda #HARE_MODE_STUN
		sta dgd_HARE_MODE + HARE_SLOT
	:
	; sprite/finish
	lda dgd_HARE_HP + HARE_SLOT
	cmp #HARE_HP_MAX
	bcc :+
		lda #255
		sta dgd_HARE_HEAT + HARE_SLOT
		lda #DATA_sprite2_hare_sit_final
		jmp :++
	:
		lda #DATA_sprite2_hare_sit
	:
	sta dgd_HARE_SPRITE + HARE_SLOT
	; get up
	lda dgd_HARE_HEAT + HARE_SLOT
	bne :+
		lda #0
		sta dgd_HARE_ANIM + HARE_SLOT
		lda #HARE_MODE_GET_UP
		sta dgd_HARE_MODE + HARE_SLOT
		lda #HARE_MOOD_ESCAPE
		sta dgd_HARE_MOOD + HARE_SLOT
		rts
	:
	; repeated hits if under constant heat
	lda dgd_HARE_HEAT + HARE_SLOT
	cmp #196
	bcc @cooling
	@still_hot:
		inc dgd_HARE_SIT + HARE_SLOT
		bne :+
			jsr dog_hit_hare
		:
		jmp @cooling_end
	@cooling:
		lda #0
		sta dgd_HARE_SIT + HARE_SLOT
	@cooling_end:
	; select sprite
	lda dgd_HARE_ANIM + HARE_SLOT
	cmp #2
	bcs :+
		lda #DATA_sprite2_hare_back0
		jmp @sprite
	:
	cmp #4
	bcs :+
		lda #DATA_sprite2_hare_back1
		jmp @sprite
	:
	bne :+
		jsr prng
		and #31
		clc
		adc #4
		sta dgd_HARE_ANIM + HARE_SLOT
		rts
	:
	cmp #167
	bcc :+
		lda #3
		sta dgd_HARE_ANIM + HARE_SLOT
		rts
	:
	cmp #160
	bcs :+
		rts
	:
		lda #DATA_sprite2_hare_sit_blink
	;
@sprite:
	sta dgd_HARE_SPRITE + HARE_SLOT
	rts
hare_mode_sit_end:
;
; HARE_MODE_GET_UP
;
	cmp #HARE_MODE_GET_UP
	bne hare_mode_get_up_end
hare_mode_get_up:
	lda dgd_HARE_ANIM + HARE_SLOT
	cmp #5
	bcs :+
		lda #DATA_sprite2_hare_squat1
		sta dgd_HARE_SPRITE + HARE_SLOT
		rts
	:
	cmp #10
	bcs :+
		lda #DATA_sprite2_hare_squat0
		sta dgd_HARE_SPRITE + HARE_SLOT
		rts
	:
		lda #0
		sta dgd_HARE_ANIM + HARE_SLOT
		lda #HARE_MODE_IDLE
		sta dgd_HARE_MODE + HARE_SLOT
		rts
	;
hare_mode_get_up_end:
	cmp #HARE_MODE_HIDE
	bne hare_mode_hide_end
;
; HARE_MODE_HIDE
;
hare_mode_hide:
	lda dgd_HARE_ANIM + HARE_SLOT
	cmp #HARE_HIDE
	bcs :+
		rts
	:
	lda dgd_HARE_MOOD + HARE_SLOT
	cmp #HARE_MOOD_STOMPL
	bne :+
		lda #(56-18)
		jmp :++
	:
		lda #(120-18)
	:
	sta dog_y + HARE_SLOT
	lda #<(512+15)
	sta dog_x + HARE_SLOT
	lda #>(512+15)
	sta dog_xh + HARE_SLOT
	lda #0
	sta dgd_HARE_FLIP + HARE_SLOT
	lda #4
	jsr dog_throw_hare
	jmp hare_mode_fly
	;rts
hare_mode_hide_end:
	; should not happen
	lda #HARE_MODE_HIDE
	sta dgd_HARE_MODE + HARE_SLOT
	rts

dog_draw_hare:
	; dx_scroll_edge -16
	lda dog_x + HARE_SLOT
	sec
	sbc #<16
	sta i
	lda dog_xh + HARE_SLOT
	sbc #>16
	and #$01
	sta ih
	; dx_scroll_edge_ preamble
	lda i
	sec
	sbc scroll_x+0
	tax
	lda ih
	sbc scroll_x+1
	jsr dx_scroll_edge_common_
	beq :+
		rts
	:
	; post-adjustment of dx
	txa
	clc
	adc #16
	sta u ; u = dx
	;
	lda dgd_HARE_MODE + HARE_SLOT
	cmp #HARE_MODE_HIDE
	bne :+
		rts
	:
	; set palette
	.assert DATA_palette_hare0 + 1 = DATA_palette_hare1, error, "hare palettes out of order!"
	.assert DATA_palette_hare0 + 2 = DATA_palette_hare2, error, "hare palettes out of order!"
	.assert DATA_palette_hare0 + 3 = DATA_palette_hare3, error, "hare palettes out of order!"
	.assert DATA_palette_hare0_eye + 1 = DATA_palette_hare1_eye, error, "hare palettes out of order!"
	.assert DATA_palette_hare0_eye + 2 = DATA_palette_hare2_eye, error, "hare palettes out of order!"
	.assert DATA_palette_hare0_eye + 3 = DATA_palette_hare3_eye, error, "hare palettes out of order!"
	lda dgd_HARE_HEAT + HARE_SLOT
	lsr
	lsr
	lsr
	lsr
	lsr
	lsr
	sta p
	lda dgd_HARE_HP + HARE_SLOT
	cmp #HARE_HP_MAX
	bcs :+
		lda #DATA_palette_hare0_eye
		clc
		adc p
		tax
		lda #6
		jsr palette_load
	:
	lda #DATA_palette_hare0
	clc
	adc p
	tax
	lda #7
	jsr palette_load
	; draw sprite
	ldx u
	ldy dog_y + HARE_SLOT
	lda dgd_HARE_FLIP + HARE_SLOT
	php
	lda dgd_HARE_SPRITE + HARE_SLOT
	plp
	bne :+
		jmp sprite2_add_banked
	:
		jmp sprite2_add_flipped_banked
	;rts

; DOG_HARECICLE
stat_dog_harecicle:

dgd_HARECICLE_ANIM = dog_data0
dgd_HARECICLE_FLIP = dog_data1 ; random flip + palette attribute for sprite tile
; dgp = mode 0 = falling, 1 = collided

dog_harecicle_spawn:
	; Y = calling object (its dgx will be copied to the new harecicle)
	; clobbers Y
	MIN_HARECICLE_SLOT = 2
	MAX_HARECICLE_SLOT = 10
	;
	ldx #MIN_HARECICLE_SLOT
	@find_loop:
		lda dog_type, X
		cmp #DOG_NONE
		bne :+
			jmp @found
		:
		inx
		cpx #MAX_HARECICLE_SLOT
		bcc @find_loop
			rts
		;
	;
@found:
	; X = index, Y = caller index
	lda dog_x, Y
	sta dog_x, X
	cmp #<40
	lda dog_xh, Y
	sta dog_xh, X
	sbc #>40
	bcs :+
		rts ; dgx < 40
	:
	lda dog_xh, Y
	cmp #>512
	bcc :+
		rts ; dgx >= 512
	:
	txa
	tay
	;jmp dog_init_harecicle
dog_init_harecicle:
	; can't rely on dog_now, must use Y
	lda #DOG_HARECICLE
	sta dog_type, Y
	lda #66
	sta dog_y, Y
	lda #0
	sta dgd_HARECICLE_ANIM, Y
	jsr prng
	and #$40
	ora #$01
	sta dgd_HARECICLE_FLIP, Y
	lda #0
	sta dog_param, Y
	PLAY_SOUND SOUND_ICEFALL
	rts

dog_tick_harecicle:
	tya
	tax
	lda dog_param, Y
	bne @fallen
@falling:
	lda dog_y, Y
	clc
	adc #3
	sta dog_y, Y
	inc dgd_HARECICLE_ANIM, X
	DOG_BOUND -3,0,2,4
	jsr lizard_overlap_no_stone
	beq :+
		jsr lizard_die
	:
	lda dog_xh, Y
	sta ih
	lda dog_x, Y
	tax
	lda dog_y, Y
	clc
	adc #8
	tay
	jsr collide_tile
	beq :+
		ldy dog_now
		lda #1
		sta dog_param, Y
		lda #10
		sta dgd_HARECICLE_ANIM, Y
	:
	rts
@fallen:
	dec dgd_HARECICLE_ANIM, X
	bne :+
		jmp empty_dog
	
	:
	rts

dog_draw_harecicle:
	DX_SCROLL_EDGE
	lda dgd_HARECICLE_FLIP, Y
	sta i
	lda dgd_HARECICLE_ANIM, Y
	cmp #10
	bcs :+
		lda #$38
		pha
		jmp @draw
	:
	cmp #5
	bcs :+
		lda #$39
		pha
		jmp @draw
	:
		lda #$7B
		pha
		; jmp @draw
@draw:
	lda dog_y, Y
	tay
	pla ; sprite
	jmp sprite_tile_add_clip
	;rts

; DOG_HAREBURN
stat_dog_hareburn:

dgd_HAREBURN_ANIM  = dog_data0
dgd_HAREBURN_FRAME = dog_data1
dgd_HAREBURN_WAIT  = dog_data2

.segment "DATA"

hareburn_palette:
.byte DATA_palette_hare_fire0, DATA_palette_hare_fire1, DATA_palette_hare_fire2
.byte DATA_palette_hare_fire0, DATA_palette_hare_fire1, DATA_palette_hare_fire2
.byte DATA_palette_hare_fire_low0, DATA_palette_hare_fire_low1
.byte DATA_palette_hare_fire_low0, DATA_palette_hare_fire_low1
.byte DATA_palette_hare_fire_low0, DATA_palette_hare_fire_low1
.assert (*-hareburn_palette)=(6*2),error,"hareburn_palette array length problem"


.segment "CODE"

dog_init_hareburn:
	jsr prng
	and #7
	sta dgd_HAREBURN_ANIM, Y
	rts

dog_tick_hareburn:
	tya
	tax
	inc dgd_HAREBURN_ANIM, X
	lda dgd_HAREBURN_ANIM, Y
	cmp #8
	bcc @anim_end
		lda #0
		sta dgd_HAREBURN_ANIM, Y
		inc dgd_HAREBURN_FRAME, X
		lda dgd_HAREBURN_FRAME, Y
		cmp #6
		bcc :+
			lda #0
			sta dgd_HAREBURN_FRAME, Y
		:
	@anim_end:
	ldx dgd_HAREBURN_FRAME, Y
	lda hareburn_palette+6, X
	pha
	lda hareburn_palette+0, X
	tax
	lda #3
	jsr palette_load
	pla
	tax
	lda #2
	jsr palette_load
	ldy dog_now
	;
	DOG_BOUND 0,0,HAREBURN_W,HAREBURN_H
	jsr lizard_overlap_no_stone
	beq :+
		jsr lizard_die
	:
	; hareburn responsible for ending the boss
	.if FASTBOSS
		jmp :+
	.endif
	lda dog_type + HARE_SLOT
	cmp #DOG_HARE
	bne :+
	lda dgd_HARE_HP + HARE_SLOT
	cmp #HARE_HP_MAX
	bcs :+
		rts
	:
	tya
	tax
	inc dgd_HAREBURN_WAIT, X
	lda dgd_HAREBURN_WAIT, Y
	cmp #100
	bcc :+
		lda #FLAG_BOSS_0_MOUNTAIN
		jsr flag_set
		lda #0
		sta human1_next
		; Y clobbered
		lda dog_type + 14
		cmp #DOG_BOSS_DOOR
		beq :+
			lda #DOG_BOSS_DOOR
			sta dog_type + 14
			ldy #14
			clc ; recto
			jmp dog_init_boss_door_out
		;
	:
	rts

dog_draw_hareburn:
	rts

; ==========
; river dogs
; ==========
stat_dog_river_common:

.segment "DATA"
RDC = 18
;                    LP,R0,R1,R2,R3,R4,R5,R6,R7,LG,DK,RP,SK,BR,WV,SL,SH,ST
river_event_sp: .byte 0, 4, 4, 4, 4, 4, 4, 4, 4, 2, 1, 4, 1, 4, 4, 0, 0, 0
river_bound_x0: .byte 0,16,16,15,16, 8, 8, 8, 6, 8,23,16,16,16,11,18,18, 8
river_bound_y0: .byte 0, 7, 4, 3,22, 3, 3, 4, 6,19,10,16, 4, 6, 4, 4, 4, 4
river_bound_x1: .byte 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,10,10, 1
river_bound_y1: .byte 0, 1, 1, 1, 1, 1, 1, 1, 1, 9, 3, 1, 1, 1, 1, 1, 1, 1
river_cover_x0: .byte 0,16,16,15,16, 8, 8, 8, 6, 8,23,16,16,16,11,18,18, 8
river_cover_y0: .byte 0,17, 9, 7,25, 8, 7, 8, 9,26,25,25,10,15,17,25,25,14
river_cover_x1: .byte 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 0, 3
river_cover_y1: .byte 0, 8, 5, 4,23, 4, 4, 5, 7,20,11,17, 5, 7, 5, 5, 5, 5
river_bound_zh: .byte 0, 8, 3, 2, 1, 3, 2, 2, 1, 4,13, 7, 4, 9, 6, 8, 8,10
river_pal_slot: .byte 0, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 6, 7, 7, 7, 6
river_pal_used:
.byte 0
.byte DATA_palette_rock
.byte DATA_palette_rock
.byte DATA_palette_rock
.byte DATA_palette_rock
.byte DATA_palette_rock
.byte DATA_palette_rock
.byte DATA_palette_rock
.byte DATA_palette_rock
.byte DATA_palette_river_wood
.byte DATA_palette_duck
.byte DATA_palette_duck
.byte DATA_palette_coin
.byte DATA_palette_river_wood
.byte DATA_palette_wave
.byte DATA_palette_snek
.byte DATA_palette_snek
.byte DATA_palette_snek_tail

.assert RDC=(river_bound_x0 - river_event_sp),error,"river table size mismatch"
.assert RDC=(river_bound_y0 - river_bound_x0),error,"river table size mismatch"
.assert RDC=(river_bound_x1 - river_bound_y0),error,"river table size mismatch"
.assert RDC=(river_bound_y1 - river_bound_x1),error,"river table size mismatch"
.assert RDC=(river_cover_x0 - river_bound_y1),error,"river table size mismatch"
.assert RDC=(river_cover_y0 - river_cover_x0),error,"river table size mismatch"
.assert RDC=(river_cover_x1 - river_cover_y0),error,"river table size mismatch"
.assert RDC=(river_cover_y1 - river_cover_x1),error,"river table size mismatch"
.assert RDC=(river_bound_zh - river_cover_y1),error,"river table size mismatch"
.assert RDC=(river_pal_slot - river_bound_zh),error,"river table size mismatch"
.assert RDC=(river_pal_used - river_pal_slot),error,"river table size mismatch"
.assert RDC=(*              - river_pal_used),error,"river table size mismatch"

.segment "CODE"
dog_init_river_common:
	jsr zero_dog_data
	lda #<(256+24)
	sta dog_x, Y
	lda #>(256+24)
	sta dog_xh, Y
	ldx dog_param, Y
	lda river_pal_slot, X
	cmp #0
	bne :+
		rts
	:
	sta i ; temporarily store slot
	tya
	pha ; temporarily store Y on stack
	lda river_pal_used, X
	tax
	lda i
	jsr palette_load
	pla
	tay
	rts

dog_overlap_river_common:
	ldx dog_param, Y
	; ih:i = x0
	lda dog_x, Y
	sec
	sbc river_bound_x0, X
	sta i
	lda dog_xh, Y
	sbc #0
	sta ih
	bpl :+
		lda #0
		sta i
		sta ih
	:
	; kh:k = x1
	lda dog_x, Y
	sec
	sbc river_bound_x1, X
	sta k
	lda dog_xh, Y
	sbc #0
	sta kh
	; j = y0
	lda dog_y, Y
	sec
	sbc river_bound_y0, X
	sta j
	; l = y1
	lda dog_y, Y
	sec
	sbc river_bound_y1, X
	sta l
	; finish
	jmp lizard_overlap

dog_cover_river_common:
	ldx dog_param, Y
	;
	lda river_cover_y0, X
	sec
	sbc river_cover_y1, X
	cmp lizard_z+0
	bcs :+
		lda #0
		rts
	:
	; ih:i = x0
	lda dog_x, Y
	sec
	sbc river_cover_x0, X
	sta i
	lda dog_xh, Y
	sbc #0
	sta ih
	bpl :+
		lda #0
		sta i
		sta ih
	:
	; kh:k = x1
	lda dog_x, Y
	sec
	sbc river_cover_x1, X
	sta k
	lda dog_xh, Y
	sbc #0
	sta kh
	; j = y0
	lda dog_y, Y
	sec
	sbc river_cover_y0, X
	sta j
	; l = y1
	lda dog_y, Y
	sec
	sbc river_cover_y1, X
	sta l
	; finish
	jmp lizard_overlap

dog_tick_river_common:
	ldx dog_param, Y
	; move
	lda dog_x, Y
	sec
	sbc river_event_sp, X
	sta dog_x, Y
	lda dog_xh, Y
	sbc #0
	sta dog_xh, Y
	bpl :+
		jmp empty_dog
	:
	; if hit
	lda lizard_z+0
	cmp river_bound_zh, X
	bcs :+
	jsr dog_overlap_river_common
	beq :+
		jmp lizard_die
	:
	; if cover
	lda lizard_y+0
	cmp dog_y, Y
	bcs :+
	jsr dog_cover_river_common
	beq :+
		lda #1
		sta dgd_RIVER_OVERLAP + RIVER_SLOT
	:
	rts

dog_draw_river_common:
	sta i
	DX_SCROLL_RIVER
	lda dog_y, Y
	tay
	lda i
	jmp sprite1_add

; DOG_ROCK
stat_dog_rock:
dog_init_rock = dog_init_river_common
dog_tick_rock = dog_tick_river_common
dog_draw_rock:
	.assert DATA_sprite1_rock1 = (DATA_sprite1_rock0 + 1), error, "Rock sprites out of order."
	.assert DATA_sprite1_rock2 = (DATA_sprite1_rock0 + 2), error, "Rock sprites out of order."
	.assert DATA_sprite1_rock3 = (DATA_sprite1_rock0 + 3), error, "Rock sprites out of order."
	.assert DATA_sprite1_rock4 = (DATA_sprite1_rock0 + 4), error, "Rock sprites out of order."
	.assert DATA_sprite1_rock5 = (DATA_sprite1_rock0 + 5), error, "Rock sprites out of order."
	.assert DATA_sprite1_rock6 = (DATA_sprite1_rock0 + 6), error, "Rock sprites out of order."
	.assert DATA_sprite1_rock7 = (DATA_sprite1_rock0 + 7), error, "Rock sprites out of order."
	lda dog_param, Y
	clc
	adc #(DATA_sprite1_rock0 - RE_ROCK0)
	jmp dog_draw_river_common

; DOG_LOG
dog_stat_log:

dgd_LOG_ANIM  = dog_data0
dgd_LOG_FRAME = dog_data1

dog_init_log:
dog_tick_log:
dog_draw_log:
	; NOT IN DEMO
	rts

; DOG_DUCK
dog_stat_duck:

dog_init_duck:
dog_tick_duck:
dog_draw_duck:
	; NOT IN DEMO
	rts

; DOG_RAMP
stat_dog_ramp:

dog_init_ramp:
dog_tick_ramp:
dog_draw_ramp:
	; NOT IN DEMO
	rts

; DOG_RIVER_SEEKER
stat_dog_river_seeker:

dog_init_river_seeker:
dog_tick_river_seeker:
dog_draw_river_seeker:
	; NOT IN DEMO
	rts

; DOG_BARREL
dog_stat_barrel:

dog_init_barrel:
dog_tick_barrel:
dog_draw_barrel:
	; NOT IN DEMO
	rts

; DOG_WAVE
dog_stat_wave:

dog_init_wave:
dog_tick_wave:
dog_draw_wave:
	; NOT IN DEMO
	rts

; DOG_SNEK_LOOP
dog_stat_snek_loop:

dog_init_snek_loop:
dog_tick_snek_loop:
dog_draw_snek_loop:
	; NOT IN DEMO
	rts

; DOG_SNEK_HEAD
dog_stat_snek_head:

dog_init_snek_head:
dog_tick_snek_head:
dog_draw_snek_head:
	; NOT IN DEMO
	rts

; DOG_SNEK_TAIL
dog_stat_snek_tail:

dog_init_snek_tail:
dog_tick_snek_tail:
dog_draw_snek_tail:
	; NOT IN DEMO
	rts

; DOG_RIVER_LOOP
stat_dog_river_loop:

dog_init_river_loop:
	; exit on first loop for demo
	;.if FASTBOSS
		lda #2
		sta dog_y, Y
	;.endif
	; reset snek sequence
	lda #0
	sta dog_param + RIVER_SLOT
	; setup loop
	lda dog_y, Y
	bne @loop_not
		; execute loop point
		lda dgd_RIVER_LOOP0 + RIVER_SLOT
		sta dgd_RIVER_SEQ0  + RIVER_SLOT
		lda dgd_RIVER_LOOP1 + RIVER_SLOT
		sta dgd_RIVER_SEQ1  + RIVER_SLOT
		rts
	@loop_not:
	cmp #2
	bne @loop_set
		; boss has been defeated
		lda #MUSIC_SILENT
		sta player_next_music
		lda #$A
		sta player_bg_noise_freq
		lda #2
		sta player_bg_noise_vol
		lda #255
		sta dgd_RIVER_SEQ_TIME + RIVER_SLOT
		lda #0
		sta dog_param, Y
		rts
	@loop_set:
		; set loop point to start boss round
		lda #MUSIC_BOSS
		sta player_next_music
		lda dgd_RIVER_SEQ0  + RIVER_SLOT
		sta dgd_RIVER_LOOP0 + RIVER_SLOT
		lda dgd_RIVER_SEQ1  + RIVER_SLOT
		sta dgd_RIVER_LOOP1 + RIVER_SLOT
		rts
	;

dog_tick_river_loop:
	lda dog_y, Y
	cmp #2
	beq :+
		jmp empty_dog
	:
	lda #255
	sta dgd_RIVER_SEQ_TIME + RIVER_SLOT
	tya
	tax
	inc dog_param, X
	lda dog_param, Y
	cmp #200
	bcs :+
		rts
	:
	lda #200
	sta dog_param, Y
	lda lizard_dead
	beq :+
		rts
	:
	;lda #FLAG_BOSS_1_RIVER
	;jsr flag_set ; disabled for demo
	ldx #1
	stx human1_next
	stx room_change
	stx current_door
	clc ; recto
	jsr dog_init_boss_door_out_partial
	lda door_link+1
	sta current_room+0
	lda door_linkh+1
	sta current_room+1
	;rts
dog_draw_river_loop:
	rts

; DOG_WATT
dog_stat_watt:

dog_init_watt:
dog_tick_watt:
dog_draw_watt:
	; NOT IN DEMO
	rts

; DOG_WATERFALL
stat_dog_waterfall:

dog_tick_waterfall:
	lda current_lizard
	cmp #LIZARD_OF_SWIM
	bne @end
	lda lizard_power
	cmp #1
	bne @end
	lda lizard_x+0
	cmp dog_x, Y
	lda lizard_xh
	sbc dog_xh, Y
	bcc @end
	lda lizard_y+0
	cmp dog_y, Y
	bcc @end
	; if param > 0 it has a right and bottom edge
	lda dog_param, Y
	beq @climb
	lda lizard_y+0
	cmp water
	bcs @end
	lda lizard_x+0
	sec
	sbc dog_x, Y
	sta i
	lda lizard_xh
	sbc dog_xh, Y
	bne @end ; >= 256
	lda i
	cmp #<25 ; >= 25
	bcs @end
	@climb:
		lda #<-550
		sta lizard_vy+1
		lda #>-550
		sta lizard_vy+0
	@end:
dog_init_waterfall:
dog_draw_waterfall:
	rts

; ===========
; jump tables
; ===========
.segment "DATA"
DOG_TABLE0 = 0
DOG_TABLE1 = 1
DOG_TABLE2 = 0
DOG_TABLE_LEN = DOG_BANK2_START - DOG_BANK1_START
.include "dogs_tables.s"

; ============
; public stuff
; ============
.segment "CODE"
stat_dog_public:

dog_init:
	ldy dog_now
	ldx dog_type, Y
	cmp #DOG_BANK2_START
	bcc :+
		brk ; crash on bad dog!
	:
	lda dog_init_table_high-DOG_BANK1_START, X
	pha
	lda dog_init_table_low-DOG_BANK1_START, X
	pha
	rts

dog_tick:
	ldy dog_now
	ldx dog_type, Y
	lda dog_tick_table_high-DOG_BANK1_START, X
	pha
	lda dog_tick_table_low-DOG_BANK1_START, X
	pha
	rts

dog_draw:
	ldy dog_now
	ldx dog_type, Y
	lda dog_draw_table_high-DOG_BANK1_START, X
	pha
	lda dog_draw_table_low-DOG_BANK1_START, X
	pha
	rts

stat_dog_end_of_file:
; end of file
