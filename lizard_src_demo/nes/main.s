; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; main
;
; main game code
;

; PROFILE = 0 release
; PROFILE = 1 greyscale after update finished
; PROFILE = 2 lizard_tick green, dogs_tick red, draw_sprites blue

.ifdef BUILD_PROFILE
PROFILE = 2
.else
PROFILE = 0
.endif

.feature force_range
.macpack longbranch

.include "ram.s"
.import bank_call
.import bank_return

.include "enums.s"
.include "sound_common.s"
.import sprite_prepare
.import sprite_add
.import sprite_add_flipped
.import sprite_tile_add

sprite2_add         = sprite_add
sprite2_add_flipped = sprite_add_flipped

.include "version.s"
.include "../assets/export/names.s"
.include "../assets/export/text_set.s"

.include "../assets/export/mini.s"

.import text_load
.import text_start
.import text_continue
.import text_convert
.import text_confound
.import text_data_location

.import palette_load

; from common.s
.import prng
.import prng2
.import prng4
.import prng8
.import empty_blocker
.import flag_read

; from common_df.s
.import hex_to_ascii
.import ppu_nmi_write_row

; from common_ef.s
.import password_read
.import checkpoint
.import password_build

.segment "CODE"

; ==============
; helpful macros
; ==============

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

.macro NMI_UPDATE_ADDR addr
	lda #<addr
	sta nmi_load+0
	lda #>addr
	sta nmi_load+1
.endmacro

.macro TEMP_PTR0_LOAD addr
	lda #<addr
	sta temp_ptr0+0
	lda #>addr
	sta temp_ptr0+1
.endmacro

.macro NMI_UPDATE_TEXT ax,ay,enum
	NMI_UPDATE_AT ax,ay
	lda #enum
	jsr text_load
.endmacro

; =======
; gamepad
; =======

gamepad_poll:
	lda #1
	sta $4016
	lda #0
	sta $4016
	ldx #8
	:
		pha
		lda $4016
		and #%00000011
		cmp #%00000001
		pla
		ror
		dex
		bne :-
	sta gamepad
	lda gamepad
	rts

; ==========
; data setup
; ==========

.segment "DATA"
tip_point:
.byte 13,17,19,23,29,31

; hair tiles
.segment "CHR"

.export hair_6e
hair_6e:
.byte $00,$00,$00,$00,$00,$24,$18,$20,$00,$00,$18,$3C,$3C,$18,$00,$1C
.byte $3A,$7F,$6F,$43,$43,$25,$1A,$20,$3A,$7F,$7F,$7F,$7F,$19,$02,$1C
.byte $3C,$7E,$7E,$42,$42,$24,$18,$20,$3C,$7E,$7E,$7E,$7E,$18,$00,$1C
.byte $18,$3C,$66,$62,$42,$66,$7E,$62,$18,$3C,$7E,$7E,$7E,$5A,$66,$5E
.byte $1E,$3C,$24,$00,$00,$24,$18,$20,$1E,$3C,$3C,$3C,$3C,$18,$00,$1C
.byte $18,$3C,$76,$62,$42,$24,$18,$20,$18,$3C,$7E,$7E,$7E,$18,$00,$1C
.byte $00,$18,$24,$00,$00,$24,$18,$20,$00,$18,$3C,$3C,$3C,$18,$00,$1C
.byte $14,$7A,$24,$42,$00,$24,$18,$20,$14,$7A,$3C,$7E,$3C,$18,$00,$1C

hair_69:
.byte $00,$00,$00,$00,$00,$24,$1C,$30,$00,$00,$18,$3C,$3C,$18,$02,$0F
.byte $3A,$7F,$5F,$47,$43,$25,$1C,$30,$3A,$7F,$7F,$7F,$7F,$19,$02,$0F
.byte $1C,$3E,$7F,$43,$02,$24,$1C,$30,$1C,$3E,$7F,$7F,$3E,$18,$02,$0F
.byte $18,$3C,$2E,$02,$02,$27,$3D,$30,$18,$3C,$3E,$3E,$3E,$1B,$23,$0F
.byte $1C,$38,$2C,$04,$00,$24,$1C,$30,$1C,$38,$3C,$3C,$3C,$18,$02,$0F
.byte $18,$3C,$76,$46,$02,$24,$1C,$30,$18,$3C,$7E,$7E,$3E,$18,$02,$0F
.byte $00,$18,$2C,$04,$00,$24,$1C,$30,$00,$18,$3C,$3C,$3C,$18,$02,$0F
.byte $14,$7A,$2C,$46,$00,$24,$1C,$30,$14,$7A,$3C,$7E,$3C,$18,$02,$0F

.segment "CODE"
hair_override:
	lda chr_cache+5
	cmp #DATA_chr_lizard_dismount
	beq :+
		rts
	:
	lda #FLAG_EYESIGHT
	jsr flag_read
	beq :+
		rts
	:
	; apply hairstyle tiles
	lda human0_hair
	lsr
	and #%01110000
	pha
	tay
	PPU_LATCH $16E0
	ldx #16
	:
		lda hair_6e, Y
		sta $2007
		iny
		dex
		bne :-
	PPU_LATCH $1690
	pla
	tay
	ldx #16
	:
		lda hair_69, Y
		sta $2007
		iny
		dex
		bne :-
	rts

; A = CHR enum
; m = FLAG_EYESIGHT
chr_load:
	tax
	cpx #DATA_chr_font
	beq @font_load
	cpx #DATA_chr_metrics
	beq @regular_load
	lda m
	beq @regular_load
	cpx #DATA_chr_dog_b2
	jne @mini_load
	lda current_room+0
	cmp #<DATA_room_info
	bne :+
	lda current_room+1
	cmp #>DATA_room_info
	beq @regular_load
	lda current_room+0
	:
	cmp #<DATA_room_ice_F_single_moose
	bne :+
	lda current_room+1
	cmp #>DATA_room_ice_F_single_moose
	beq @regular_load
	lda current_room+0
	:
	jmp @mini_load
@regular_load:
	; regular CHR load (banked call)
	lda #$B ; CHR bank, X = CHR to load
	jsr bank_call
	rts
@font_load:
	jsr @regular_load
	; apply translation glyphs
	lda text_data_location+4
	sta temp_ptr1+0
	lda text_data_location+5
	sta temp_ptr1+1
	ldx #0
	@glyph_loop:
		cpx #45
		bcs @glyph_end ; failsafe if data is corrupted?
		ldy #0
		lda (temp_ptr1), Y ; tile selection
		beq @glyph_end ; tile 0 means end of list
		and #$3F
		ora #$40
		lsr
		lsr
		lsr
		lsr
		sta $2006
		lda (temp_ptr1), Y
		and #$3F
		;ora #$40 ; this bit is shifted out anyway
		asl
		asl
		asl
		asl
		sta $2006
		iny ; Y = 1
		: ; load 8 bytes to plane 0
			lda (temp_ptr1), Y
			iny
			sta $2007
			cpy #9
			bcc :-
		ldy #1
		: ; same 8 bytes to plane 1
			lda (temp_ptr1), Y
			iny
			sta $2007
			cpy #9
			bcc :-
		; move to next glyph
		lda temp_ptr1+0
		clc
		adc #<9
		sta temp_ptr1+0
		lda temp_ptr1+1
		adc #>9
		sta temp_ptr1+1
		inx
		jmp @glyph_loop
	@glyph_end:
	rts
@mini_load:
	; temp_ptr1 = (A * 64) + data_mini
	lda #0
	sta temp_ptr1+1
	txa
	pha
	.repeat 6
		asl
		rol temp_ptr1+1
	.endrepeat
	clc
	adc #<data_mini
	sta temp_ptr1+0
	lda #>data_mini
	adc temp_ptr1+1
	sta temp_ptr1+1
	; unpack
	ldy #0
	@mini_byte:
		lda (temp_ptr1), Y
		ldx #4
		:
			pha
			and #$88
			sta n
			lsr
			ora n
			lsr
			ora n
			lsr
			ora n
			sta $2007
			sta $2007
			sta $2007
			sta $2007
			pla
			asl
			dex
			bne :-
		iny
		cpy #64
		bcc @mini_byte
	pla
	; workaround: don't let eyesight corrupt sprite 0 hit tile
	cmp #DATA_chr_lizard
	bne :++
		PPU_LATCH $12F0
		lda #$03
		ldx #0
		:
			sta $2007
			inx
			cpx #8
			bcc :-
		;
	:
	rts

room_load:
	ldx #0 ; load room
	lda #$0 ; ROOM
	jsr bank_call
	; reset scroll
	lda #0
	sta scroll_x+0
	sta scroll_x+1
	jsr sprite_0_init
	; dog_data0 is storing palettes
	lda #0
	:
		pha
		tay
		ldx dog_data0, Y
		jsr palette_load
		pla
		clc
		adc #1
		cmp #8
		bcc :-
	; dog_data1 is storing CHR
	lda #FLAG_EYESIGHT
	jsr flag_read
	sta m ; m = eyesight flag for chr_load call
	lda #0
	@chr_loop:
		pha
		tax
		lda dog_data1, X
		cmp #254
		bcs @skip
		cmp chr_cache, X
		beq @skip
			sta chr_cache, X
			txa
			asl
			asl
			sta $2006 ; CHR address high = X << 10 (1k chunks)
			lda #$00
			sta $2006
			lda chr_cache, X
			jsr chr_load
		@skip:
		pla
		clc
		adc #1
		cmp #8
		bcc @chr_loop
	jsr hair_override
	; doors
	ldx current_door
	lda door_x, X
	sta i
	lda door_xh, X
	sta k
	lda door_y, X
	sta j
	; i,j,k are x,y params to lizard_init
	jsr lizard_init
	; wipe the blockers before dogs_init
	ldy #0
	:
		jsr empty_blocker
		iny
		cpy #4
	bcc :-
	lda #NMI_READY
	sta nmi_next ; cancel any pending nmi updates
	lda #0
	sta boss_talk
	sta text_select
	sta player_bg_noise_vol
	sta player_bg_noise_freq
	jsr dogs_init_banked
	jsr lizard_fall_test
	; PRNG failsafe
	lda seed+0
	ora seed+1
	bne :+
		lda nmi_count
		sta seed+0
		lda #$01
		sta seed+1
	:
	; setup_river sprite 0
	lda dog_type + RIVER_SLOT
	cmp #DOG_RIVER
	beq @setup_river
	rts
@setup_river:
	; position sprite 0
	lda #(95-6)
	sta oam+0
	; remove top 6 pixels of CHR
	PPU_LATCH $12F0
	lda #0
	tax
	:
		sta $2007
		inx
		cpx #6
		bcc :-
	; invalidate cache so CHR tile gets rebuilt on next screen
	lda #$FF
	sta chr_cache + 4
	rts

; ==========
; bank calls
; ==========

.export build_frob_row_banked
build_frob_row_banked:
	; NOT IN DEMO
	rts

.segment "DATA"

; used by DOG_CAT but didn't have room in its bank
.export circle108
circle108:
.byte    0,   3,   5,   8,  11,  13,  16,  18,  21,  24,  26,  29,  31,  34,  36,  39
.byte   41,  44,  46,  48,  51,  53,  55,  58,  60,  62,  64,  66,  68,  70,  72,  74
.byte   76,  78,  80,  81,  83,  85,  86,  88,  89,  91,  92,  94,  95,  96,  97,  98
.byte   99, 100, 101, 102, 103, 104, 104, 105, 105, 106, 106, 107, 107, 107, 107, 108
.byte  108, 108, 107, 107, 107, 107, 106, 106, 105, 105, 104, 104, 103, 102, 101, 100
.byte   99,  98,  97,  96,  95,  94,  92,  91,  89,  88,  86,  85,  83,  81,  80,  78
.byte   76,  74,  72,  70,  68,  66,  64,  62,  60,  58,  55,  53,  51,  48,  46,  44
.byte   41,  39,  36,  34,  31,  29,  26,  24,  21,  18,  16,  13,  11,   8,   5,   3
.byte    0,  -3,  -5,  -8, -11, -13, -16, -18, -21, -24, -26, -29, -31, -34, -36, -39
.byte  -41, -44, -46, -48, -51, -53, -55, -58, -60, -62, -64, -66, -68, -70, -72, -74
.byte  -76, -78, -80, -81, -83, -85, -86, -88, -89, -91, -92, -94, -95, -96, -97, -98
.byte  -99,-100,-101,-102,-103,-104,-104,-105,-105,-106,-106,-107,-107,-107,-107,-108
.byte -108,-108,-107,-107,-107,-107,-106,-106,-105,-105,-104,-104,-103,-102,-101,-100
.byte  -99, -98, -97, -96, -95, -94, -92, -91, -89, -88, -86, -85, -83, -81, -80, -78
.byte  -76, -74, -72, -70, -68, -66, -64, -62, -60, -58, -55, -53, -51, -48, -46, -44
.byte  -41, -39, -36, -34, -31, -29, -26, -24, -21, -18, -16, -13, -11,  -8,  -5,  -3

.segment "CODE"

.export sprite2_add_banked_
sprite2_add_banked_:
	lda i
	ldx j
	ldy k
	jmp sprite2_add

.export sprite2_add_flipped_banked_
sprite2_add_flipped_banked_:
	lda i
	ldx j
	ldy k
	jmp sprite2_add_flipped

; ====
; misc
; ====

; copies 32 bytes from nmi_update to nmi_update+32
nmi_update_shift:
	ldx #0
	:
		lda nmi_update+0, X
		sta nmi_update+32, X
		inx
		cpx #32
		bcc :-
	rts

; =======
; overlay
; =======

; setup sprite 0
sprite_0_init:
	lda #$FF ; Y position (overlay starts at Y+3)
	sta oam+0
	lda #$2F ; tile
	sta oam+1
	lda #%00100000 ; palette 0, beneath background
	;lda #0 ; for testing: make it visible
	sta oam+2
	lda #247 ; X position (permanent)
	sta oam+3
	rts

; clears the overlay nametable
overlay_cls:
	PPU_LATCH $2400
	lda #$70 ; fill first 960 bytes with spaces
	ldy #0
	ldx #0
	:
		sta $2007
		inx
		bne :-
		iny
		cpy #3
		bcc :-
	:
		sta $2007
		inx
		cpx #192
		bcc :-
	lda #0 ; fill final 64 bytes (attribute) with 0
	:
		sta $2007
		inx
		bne :-
	rts

; writes a null terminated string from temp_ptr0
overlay_string:
	ldy #0
	:
		lda (temp_ptr0), Y
		beq :+
		sta $2007
		iny
		jmp :-
	:
	rts

; ===========================
; rendering control and vsync
; ===========================

; waits until next NMI, turns rendering on if not already, uploads OAM and palette
render_frame:
	lda #NMI_READY
	sta nmi_ready
render_wait:
	.if PROFILE
		lda #%00011110
		sta $2001
	.endif
	:
		lda nmi_ready
		bne :-
	rts

; like render_frame but also uploads 32 bytes of nametable
render_row:
	lda #NMI_ROW
	sta nmi_ready
	jmp render_wait

; like render_row but uploads 64 bytes
render_double:
	lda #NMI_DOUBLE
	sta nmi_ready
	jmp render_wait

; like render_row but uploads 64 bytes across 2 nametables
render_wide:
	lda #NMI_WIDE
	sta nmi_ready
	jmp render_wait

; like render_frame but uses the variable nmi_next to control what it sent to NMI_READY
render_next:
	lda nmi_next
	sta nmi_ready
	lda #NMI_READY
	sta nmi_next
	jmp render_wait

; waits until next NMI, turns rendering off (now safe to write PPU memory)
render_off:
	lda #NMI_OFF
	sta nmi_ready
	jmp render_wait

.segment "ALIGNED" ; keep this code from crossing a page boundary
aligned_start:

; waits for sprite 0 hit and sets scrolling for the overlay, disables sprites
; call render_frame/line to finish the frame after render_overlay finishes
render_overlay:
	.if PROFILE
		lda #%00011110
		sta $2001
	.endif
	lda #1
	sta nmi_lock ; disables music temporarily in case sprite 0 hit fails
	:
		; sprite 0 hit doesn't clear until the end of vblank, make sure that's happened
		bit $2002
		bvs :-
	:
		bit $2002         ; will hit after dot 253
		bmi @overlay_fail ; if vblank is detected, we completely missed the sprite 0 hit
		bvc :-
	; [ 4 cycles elapsed since sprite 0 hit]
	; wait for hblank alignment
	; add 3 dots to get from 254 sprite hit to 257 hblank (1 cycle)
	; a line is 341 dots long (113.6 cycles NTSC, 106.6 cycles PAL)
	; thus, this wait needs: NTSC 84 cycles, PAL 77 cycles (+4 cycles above, +27 cycles below)
	lda player_pal          ; [ 3 ]
	bne :+                  ; [ +2 if NTSC, +3 if PAL ]
		nop
		nop
		nop
		nop
	:                       ; [ 13 NTSC, 6 PAL ]
	; 71 cycles to go
	lda player_pal          ; [ +3 ]
	.repeat 34
		nop
	.endrepeat              ; [ +68 ]
	; [ NTSC 84, PAL 77 cycles waited ]
	; wait finished
	lda #%00001010
	sta $2001            ; disable sprite rendering
	lda #%00000100
	sta $2006            ; 2006.1 > nametable %01
	lda overlay_scroll_y
	sta $2005            ; 2005.2 > Y scroll
	and #%00111000
	asl
	asl
	ldx #0
	; [ 27 cycles elapsed since wait finished ]
	; the last two write should fall in hblank (after dot 256, before dot 320)
	stx $2005            ; 2005.1 > X scroll = 0
	sta $2006            ; 2006.2 > Y scroll bits 3-5, X scroll low bits = 0
	lda #0
	sta nmi_lock
	rts
@overlay_fail: ; duplicate code so I can breakpoint the failure
	lda #0
	sta nmi_lock
	rts

render_river:
	.if PROFILE
		lda #%11111110
		sta $2001
	.endif
	lda #1
	sta nmi_lock ; disables music temporarily in case sprite 0 hit fails
	:
		; sprite 0 hit doesn't clear until the end of vblank, make sure that's happened
		bit $2002
		bvs :-
	:
		bit $2002         ; will hit after dot 253
		bmi @river_fail ; if vblank is detected, we completely missed the sprite 0 hit
		bvc :-
	; waste ~40 cycles just to ensure the writes happen safely in the middle of a line and not at the end
	ldx #8
	:
		dex
		bne :-
	; horizontal scroll split
	lda dgd_RIVER_SCROLL_B1 + RIVER_SLOT ; high bit of scroll (nametable select)
	and #$00000001
	ora #%10001000
	sta $2000
	lda dgd_RIVER_SCROLL_B0 + RIVER_SLOT ; low bits of scroll
	sta $2005
	lda #0
	sta $2005
	; return
	lda #0
	sta nmi_lock
	rts
@river_fail: ; duplicate return code so I can breakpoint the failure
	lda #0
	sta nmi_lock
	rts

aligned_end:
.assert ((aligned_end & $FF00)=(aligned_start & $FF00)),error,"aligned code overflows page"

.segment "CODE"

; ============
; update tasks
; ============

.import lizard_init
.import lizard_tick
.import lizard_draw

dogs_init_banked:
	ldx #253 ; dogs_init
	lda #$E  ; dogs bank
	jsr bank_call
	rts

; NOTE every call to dogs_tick should be followed by a render_next (they can set nmi_next)
dogs_tick_banked:
	.if PROFILE>1
		lda #%00111111 ; dogs_tick in red
		sta $2001
	.endif
	ldx #254 ; dogs_tick
	lda #$E  ; dogs bank
	jsr bank_call
	rts

dogs_draw_banked:
	ldx #255 ; dogs_draw
	lda #$E  ; dogs bank
	jsr bank_call
	rts

.import lizard_fall_test ; to be called after dogs_init
.import dogs_cycle

; =============
; palette fades
; =============

; copies palette to shadow_palette so it can be used for fading
fade_prepare:
	ldx #0
	:
		lda palette, X
		sta shadow_palette, X
		inx
		cpx #32
		bne :-
	rts

; copies shadow_palette to palette to restore it after fading in
fade_finish:
	ldx #0
	:
		lda shadow_palette, X
		sta palette, X
		inx
		cpx #32
		bne :-
	rts

; fade out 25%
fade_1:
	lda #$10
	sta i
	jmp fade_inner_all
; fade out 50%
fade_2:
	lda #$20
	sta i
	jmp fade_inner_all
; fade out 75%
fade_3:
	lda #$30
	sta i
	jmp fade_inner_all
; fade out 100%
fade_4:
	lda #$40
	sta i
	jmp fade_inner_all

; fade out 25%
fade_lizard_1:
	lda #$10
	sta i
	jmp fade_inner_lizard
; fade out 50%
fade_lizard_2:
	lda #$20
	sta i
	jmp fade_inner_lizard
; fade out 75%
fade_lizard_3:
	lda #$30
	sta i
	jmp fade_inner_lizard
; fade out 100%
fade_lizard_4:
	lda #$40
	sta i
	jmp fade_inner_lizard

fade_inner_all:
	lda #0
	sta j
	lda #32
	sta k
	jmp fade_inner_perform

fade_inner_lizard:
	lda #16
	sta j
	lda #24
	sta k
	jmp fade_inner_perform

; i=subtract, j=start,k=end 
; palette outside of the j,k range will just be copied
fade_inner_perform:
	ldx #0
	@fade_start:
		cpx j
		beq @fade_middle
		lda shadow_palette, X
		sta palette, X
		inx
		jmp @fade_start
	@fade_middle:
		lda shadow_palette, X
		sec
		sbc i
		bpl :+
			lda #$0F
		:
		sta palette, X
		inx
		cpx k
		bne @fade_middle
	@fade_end:
		cpx #32
		beq :+
		lda shadow_palette, X
		sta palette, X
		inx
		jmp @fade_end
	:
	rts

; ==========
; game modes
; ==========

.segment "DATA"
fps: .byte 60, 50, 50, 50, 50, 50

.segment "CODE"
; clobbers A,X,Y
tick_time:
	inc metric_time_f
	lda metric_time_f
	ldx player_pal
	cmp fps, X
	bcs :+
		rts
	:
	ldy #0
	sty metric_time_f
	inc metric_time_s
	lda metric_time_s
	cmp #60
	bcs :+
		rts
	:
	;ldy #0
	sty metric_time_s
	inc metric_time_m
	lda metric_time_m
	cmp #60
	bcs :+
		rts
	:
	;ldy #0
	sty metric_time_m
	inc metric_time_h
	lda metric_time_h
	cmp #100
	bcs :+
		rts
	:
	; max out at 99:59:59/59 or 49
	lda #99
	sta metric_time_h
	lda #59
	sta metric_time_m
	sta metric_time_s
	;ldx player_pal
	ldy fps, X
	dey
	sty metric_time_f
	rts

draw_sprites:
	; skip sprite 0
	lda #4
	sta oam_pos
draw_sprites_append:
	.if PROFILE>1
		lda #%10011111 ; draw_sprites in blue
		sta $2001
	.endif
	jsr lizard_draw
	jsr dogs_draw_banked
finish_sprites:
	; fill unused OAM with blanks (Y=$FF)
	ldx oam_pos
	bne :+
		rts ; OAM full, no need to fill
	:
	lda #$FF
	:
		sta oam, X
		inx
		inx
		inx
		inx
		bne :-
	rts

draw_sprites_river:
	; skip sprite 0
	lda #4
	sta oam_pos
	.if PROFILE>1
		lda #%10011111 ; draw_sprites in blue
		sta $2001
	.endif
draw_sprites_river_append:
	lda #$7F
	and wave_draw_control
	sta wave_draw_control
	; draw sprites
	lda dgd_RIVER_OVERLAP + RIVER_SLOT
	beq :+
		jsr dogs_draw_banked
		jsr lizard_draw
		jmp :++
	:
		jsr lizard_draw
		jsr dogs_draw_banked
	:
	; splash for river
	lda dgd_RIVER_SPLASH_TIME + RIVER_SLOT
	cmp #16
	bcs :+
		lsr
		lsr
		lsr
		ora #$38
		pha ; tile = (splash >> 3) | $38
		lda dgd_RIVER_SPLASH_FLIP + RIVER_SLOT
		ora #$05
		sta i ; attribute = (prng & $40) |$05
		ldx smoke_x
		ldy smoke_y
		pla
		jsr sprite_tile_add
	:
	; river shadow
	lda lizard_dead
	bne :+
	lda lizard_z
	beq :+
		lda #$02
		sta i
		lda lizard_x
		sec
		sbc #7
		tax
		lda lizard_y
		sec
		sbc #3
		tay
		lda #$C6
		jsr sprite_tile_add
		lda lizard_x
		sec
		sbc #3
		tax
		lda lizard_y
		sec
		sbc #3
		tay
		lda #$C6
		jsr sprite_tile_add
	:
	jmp finish_sprites

TOILET_CODE = PAD_SELECT | PAD_B

mode_title_gamepad_last = m
mode_title_gamepad_new = n
mode_title_select = o

mode_title_poll:
	jsr prng ; initialize random seed
	jsr gamepad_poll
	eor mode_title_gamepad_last
	and gamepad
	sta mode_title_gamepad_new
	lda gamepad
	sta mode_title_gamepad_last
	lda mode_title_gamepad_new
	rts

mode_title:
	lda #0
	sta easy
	jsr draw_sprites
	jsr fade_prepare
	jsr fade_lizard_4 ; fade lizard to black
	NMI_UPDATE_TEXT 0,13,TEXT_PUSH_START
	NMI_UPDATE_AT 0,13
	; toilet humour
	jsr gamepad_poll
	and #TOILET_CODE
	cmp #TOILET_CODE
	bne :+
		lda #TEXT_PUSH_SHART
		jsr text_load
	:
	jsr render_row ; render the first frame
mode_title_loop:
	; polling gamepad filters just new button presses
	; also ticks random number generator each time
	; (note: this is happening many times per frame)
	jsr mode_title_poll
	and #PAD_SELECT
	jne mode_title_settings
	lda mode_title_gamepad_new
	and #PAD_START
	beq mode_title_loop
mode_title_start:
	; convert "player_music_mask" setting to actual mask value
	lda player_music_mask
	bne :+
		; 0 = music on
		lda #$FF
		sta player_music_mask
		jmp :++
	:
		; else music off
		lda #0
		sta player_music_mask
	:
	lda #0
	sta mode_title_gamepad_last
	sta mode_title_gamepad_new
	sta mode_title_select
	; remove PUSH START text
	NMI_UPDATE_TEXT 0,13,TEXT_BLANK
	jsr render_row
	; randomize human hairstyle
	jsr prng8
	sta human0_hair
	jsr prng8
	sta human1_hair
	; randomize human palettes
	:
		jsr prng2
		and #3
		beq :- ; reroll a 0
	sta human0_pal
	dec human0_pal ; = one of 0,1,2
	:
		jsr prng2
		and #3
		beq :-
	sta human1_pal
	dec human1_pal
	; dead human1 set
	lda #3
	sta human1_set+0
	sta human1_set+1
	sta human1_set+2
	sta human1_set+3
	sta human1_set+4
	sta human1_set+5
	; for TAS, left+right resets PRNG
	lda gamepad
	and #(PAD_L | PAD_R)
	cmp #(PAD_L | PAD_R)
	bne :+
		lda #$01
		sta seed+0
		sta seed+1
		lda #0
		sta password+0
		sta password+1
		sta password+2
		sta password+3
		sta password+4
		sta nmi_count
	:
	; drawing the lizard applies the palette
	jsr fade_finish
	jsr draw_sprites
	jsr fade_prepare
	; fade in the lizard
	jsr fade_lizard_3
	jsr render_frame
	jsr render_frame
	jsr fade_lizard_2
	jsr render_frame
	jsr render_frame
	jsr fade_lizard_1
	jsr render_frame
	jsr render_frame
	jsr fade_finish
	jsr render_frame
	jmp mode_play

.segment "DATA"
text_music_line_table:
.byte TEXT_MUSIC_ON
.byte TEXT_MUSIC_SOUNDTRACK
.byte TEXT_MUSIC_OFF

.segment "CODE"
mode_title_settings:
	; SETTINGS heading replaces PUSH START
	lda #TEXT_BLANK
	jsr text_load
	jsr nmi_update_shift
	NMI_UPDATE_TEXT 0,11,TEXT_SETTINGS
	jsr render_double
	jsr mode_title_poll
@redraw:
	; music setting 0 = on, 1 = soundtrack, 2 = off
	lda player_music_mask
	tax
	lda text_music_line_table, X
	jsr text_load
	jsr nmi_update_shift
	; difficulty setting 0 = normal, FF = easy
	.assert (TEXT_DIFFICULTY_EASY = TEXT_DIFFICULTY_NORMAL + 1),error,"TEXT_DIFFICULTY_EASY out of order."
	lda easy
	and #1
	clc
	adc #TEXT_DIFFICULTY_NORMAL
	jsr text_load
	; arrows
	lda mode_title_select
	asl
	asl
	asl
	asl
	asl ; * 32
	tay
	lda #$AB
	jsr text_convert
	sta nmi_update + 9, Y
	lda #$AC
	jsr text_convert
	sta nmi_update + 22, Y
	; finish
	NMI_UPDATE_AT 0,13
	jsr render_double
@loop:
	jsr mode_title_poll
	; SELECT/START/B exits settings
	and #(PAD_SELECT | PAD_START | PAD_B)
	bne @settings_exit
	; UP/DOWN changes selection
	lda mode_title_gamepad_new
	and #(PAD_U | PAD_D)
	beq :+
		lda mode_title_select
		eor #1
		sta mode_title_select
		jmp @redraw
	:
	lda mode_title_select
	and #1
	bne @select_music
	@select_difficulty:
		lda mode_title_gamepad_new
		and #(PAD_L|PAD_R|PAD_A)
		beq :+
			lda easy
			eor #$FF
			sta easy
			jmp @redraw
		:
		jmp @loop
	@select_music:
		ldx player_music_mask
		; RIGHT/A advances setting
		lda mode_title_gamepad_new
		and #(PAD_R|PAD_A)
		beq :++
			inx
			cpx #3
			bcc :+
				ldx #0
			:
			stx player_music_mask
			jmp @redraw
		:
		; LEFT recedes setting
		lda mode_title_gamepad_new
		and #(PAD_L)
		beq :++
			ldx player_music_mask
			bne :+
				ldx #3
			:
			dex
			stx player_music_mask
			jmp @redraw
		:
		jmp @loop
	@select_end:
	jmp @loop
@settings_exit:
	lda player_music_mask
	cmp #1
	bne :+
		jmp soundtrack
	:
	; restore rows 11-14
	lda #TEXT_BLANK
	jsr text_load
	jsr nmi_update_shift ; copies blank to rows 11/12
	NMI_UPDATE_AT 0,11
	jsr render_double
	jsr mode_title_poll
	lda #TEXT_BLANK
	jsr text_load
	jsr nmi_update_shift
	NMI_UPDATE_TEXT 0,13,TEXT_PUSH_START
	lda gamepad
	and #TOILET_CODE
	cmp #TOILET_CODE
	bne :+
		lda #TEXT_PUSH_SHART
		jsr text_load
	:
	jsr render_double
	jmp mode_title_loop

.segment "DATA"
easy_frames:
.byte 4         ; NTSC skips 1/3 frames
.byte 6,6,6,6,6 ; PAL skips 1/5 frames (cycles through 1-5 to count)

.segment "CODE"

tick_easy:
	lda easy
	bne :+
		rts
	:
	cmp #$FF
	bne :+
		rts
	:
	inc easy
	lda easy
	ldx player_pal
	cmp easy_frames, X
	bcc :+
		lda #1
		sta easy
	:
	rts

mode_play:
	lda room_change
	beq :+
		jmp mode_room_change
	:
	; skip frame if easy = 1
	lda easy
	beq :+
	cmp #1
	bne :+
		jmp @skip_frame
	:
	; do frame
	lda game_pause
	beq :+
		jmp mode_pause
	:
	jsr gamepad_poll
	.if PROFILE>1
		lda #%01011111 ; lizard_tick in green
		sta $2001
	.endif
	ldx climb_assist_time
	beq :+
		lda gamepad
		ora climb_assist
		sta gamepad
		dex
		stx climb_assist_time
		jmp :++
	:
		; X = 0
		stx climb_assist
	:
	jsr lizard_tick
	lda game_pause
	beq :+
		; game is paused by lizard, lizard tick has been prevented, don't tick anything else before resolving
		jmp mode_pause
	:
	jsr dogs_tick_banked
@skip_frame:
	jsr draw_sprites
	lda dog_type + 0
	cmp #DOG_FROB
	bne :+
		lda dgd_FROB_SCREEN + 0
		sta scroll_x+1
		lda #0
		sta scroll_x+0
	:
	jsr tick_time
	jsr tick_easy
	jsr render_next
	jmp mode_play

mode_river:
	; if ready to leave mode
	lda room_change
	beq :+
		jmp mode_room_change
	:
	; skip frame if easy = 1
	lda easy
	beq :+
	cmp #1
	bne :+
		jmp @skip_river_frame
	:
	; top of screen is used by lizard_tick only, followed by a sprite-0 split at line 96
	lda game_pause
	beq :+
		jmp mode_pause
	:
	jsr gamepad_poll
	.if PROFILE>1
		lda #%01011111 ; lizard_tick in green
		sta $2001
	.endif
	jsr lizard_tick
	jsr render_river ; scroll split for river
	; handle pause
	lda game_pause
	beq :+
		jmp mode_pause
	:
	; tick dogs
	lda #0
	sta dgd_RIVER_OVERLAP + RIVER_SLOT
	jsr dogs_tick_banked
	; update scroll for top of screen
	lda dgd_RIVER_SCROLL_A0 + RIVER_SLOT
	sta scroll_x+0
	lda dgd_RIVER_SCROLL_A1 + RIVER_SLOT
	sta scroll_x+1
	; draw sprites
	jsr draw_sprites_river
@mode_river_end:
	jsr tick_time
	jsr tick_easy
	jsr render_next
	jmp mode_river
	;
@skip_river_frame:
	jsr render_river
	lda dgd_RIVER_SCROLL_A0 + RIVER_SLOT
	sta scroll_x+0
	lda dgd_RIVER_SCROLL_A1 + +RIVER_SLOT
	sta scroll_x+1
	jsr draw_sprites_river
	jmp @mode_river_end

; ==================
; pause and messages
; ==================

.segment "DATA"
lizard_cheat_code:
.byte 0, PAD_R, 0, PAD_U, 0, PAD_D, 0, PAD_D, PAD_D|PAD_SELECT

lizard_diagnostic_code:
.byte 0, PAD_D, 0, PAD_U, 0, PAD_L, 0, PAD_L, PAD_L|PAD_SELECT

.segment "CODE"

; fill 0-63 in nmi_update with A
nmi_double_fill:
	ldx #0
	:
		sta nmi_update, X
		inx
		cpx #64
		bcc :-
	rts

; shifts 0-31 in nmi_update over to match scroll position
nmi_double_scroll:
	lda dog_type + RIVER_SLOT
	cmp #DOG_RIVER
	bne :+
		lda dgd_RIVER_SCROLL_B1 + RIVER_SLOT
		ror
		lda dgd_RIVER_SCROLL_B0 + RIVER_SLOT
		jmp :++
	:
		lda scroll_x+1
		ror
		lda scroll_x+0
	:
	ror
	lsr
	lsr ; scroll_x / 8
	tax
	cmp #32
	bcs @over_31
@under_32:
	; <32 shift has to be done right to left
	pha
	clc
	adc #31
	tax
	ldy #31
	:
		lda nmi_update, Y
		sta nmi_update, X
		dex
		txa
		and #63
		tax
		dey
		cpy #32
		bcc :-
	;
	pla
	clc
	adc #32
	and #63
	tax
	jmp @finish
@over_31:
	; >=32 shift has to be done left to right
	ldy #0
	:
		lda nmi_update, Y
		sta nmi_update, X
		inx
		txa
		and #63
		tax
		iny
		cpy #32
		bcc :-
	;
@finish:
	; fill rest with blank
	ldy #0
	:
		lda #$70
		sta nmi_update, X
		inx
		txa
		and #63
		tax
		iny
		cpy #32
		bcc :-
	;
	rts

; A = kill line
pause_draw:
	pha
	; manually cycle draw order (usually done in dogs_tick, but it's skipped)
	jsr dogs_cycle
	; draw sprites
	; manually initialize skipping sprite 0
	lda #4
	sta oam_pos
	; place 8 hidden "blocking" sprites
	pla
	tay
	pha
	dey ; Y = kill line - 1
	ldx #4
	:
		tya ; kill line - 1
		sta oam+0, X
		lda #$2F ; tile
		sta oam+1, X
		lda #%00100001 ; attribute
		sta oam+2, X
		lda #252 ; X (half offscreen to hide opaque part of tile)
		sta oam+3, X
		txa
		clc
		adc #4
		tax
		cpx #(9*4)
		bcc :-
	stx oam_pos
	; sprites
	lda dog_type + RIVER_SLOT
	cmp #DOG_RIVER
	beq :+
		jsr draw_sprites_append
		jmp :++
	:
		jsr draw_sprites_river_append
	:
	; wipe out sprites at or below the kill line
	pla
	sta k
	ldx #4
	@kill_loop:
		lda oam+0, X
		cmp k
		bcc :+
			lda #255
			sta oam+0, X
		:
		txa
		clc
		adc #4
		tax
		bne @kill_loop
	;
	rts

pause_draw_holding:
	lda #(22*8)
	jmp pause_draw

pause_copy_att:
	ldx #0
	:
		lda att_mirror+32, X
		sta nmi_update+0, X
		lda att_mirror+96, X
		sta nmi_update+32, X
		inx
		cpx #32
		bcc :-
	;
	rts

pause_render_att:
	lda #(22*8) ; kill ine
	jsr pause_draw
	NMI_UPDATE_ADDR $23E0
	jmp pause_render_wide
	;rts

; A = row to black (0-7)
pause_black_row:
	clc
	adc #22
	pha
	asl
	asl
	asl ; kill ine = (row * 8)
pause_black_row_:
	jsr pause_draw
	lda #$70
	jsr nmi_double_fill
	lda #0
	sta nmi_load+1
	pla ; A = row to black + 22
	asl
	rol nmi_load+1
	asl
	rol nmi_load+1
	asl
	rol nmi_load+1
	asl
	rol nmi_load+1
	asl
	rol nmi_load+1
	sta nmi_load+0
	lda nmi_load+1
	clc
	adc #$20
	sta nmi_load+1
	jmp pause_render_wide
	;rts
; pause_black_row but retains kill line at top
pause_black_row_mid:
	clc
	adc #22
	pha
	lda #(22*8)
	ldx #1 ; priority override line
	jmp pause_black_row_

; A = row to restore (0-7)
pause_restore_row:
	sta j ; store row temporarily
	lda dog_type + 0
	cmp #DOG_FROB
	;beq @restore_row_frob
	@restore_row:
		ldx #254 ; setup partial room load
		lda i ; ROOM bank
		jsr bank_call
		jsr nmi_update_shift
		ldx #254
		lda i
		jsr bank_call
		; volcano valves
		lda dog_type + 3
		cmp #DOG_RACCOON_LAUNCHER
		bne @restore_row_end
		lda j
		cmp #6
		bcs @restore_row_end
			lsr
			sta t
			lda #15
			sec
			sbc t
			tax ; x = 15 - (row/2)
			lda dgd_RACCOON_VALVE_LOCK, X
			beq :+
				lda dog_x, X
				sta t
				lda dog_xh, X
				lsr
				ror t
				lsr
				ror t
				lsr
				ror t
				lda t
				clc
				adc #31
				and #63
				tax ; x = ((dog_x >> 3) + 31) & 63
				lda #$80
				sta nmi_update+0, X
				sta nmi_update+1, X
			:
		jmp @restore_row_end
	@restore_row_frob:
		; NOT IN DEMO
	@restore_row_end:
	; store temporaries needed for room loading
	lda i
	pha
	lda k
	pha
	lda l
	pha
	lda temp_ptr0+0
	pha
	lda temp_ptr0+1
	pha
	; load row
	lda j
	pha
	clc
	adc #23
	asl
	asl
	asl ; kill line = (row+1) * 8
	jsr pause_draw
	lda #0
	sta nmi_load+1
	pla
	clc
	adc #22
	asl
	rol nmi_load+1
	asl
	rol nmi_load+1
	asl
	rol nmi_load+1
	asl
	rol nmi_load+1
	asl
	rol nmi_load+1
	sta nmi_load+0
	lda nmi_load+1
	clc
	adc #$24
	sta nmi_load+1
	jsr pause_render_wide
	; restore temporaries
	pla
	sta temp_ptr0+1
	pla
	sta temp_ptr0+0
	pla
	sta l
	pla
	sta k
	pla
	sta i
	rts

pause_render_wide:
	jsr render_wide
pause_river_split:
	; if river, set up the split for the next frame before continuing
	lda dog_type + RIVER_SLOT
	cmp #DOG_RIVER
	bne :+
		jsr render_river
	:
	rts

pause_render_frame:
	jsr render_frame
	jmp pause_river_split

mode_pause:
	; text_select = 0 talk, text characters display gradually with a sound
	; text_select = 1 message, text is displayed immediately without sound
	; text_select = 2 just pause (music paused, show password)
	; game_message = text or message enum to use
	lda text_select
	cmp #2
	bne :+
		; pause music if text_select is 2 (pause)
		lda #1
		sta player_pause
	:
	; frame 0-7: black out 8 bottom rows (bottom to top)
	lda #7
	jsr pause_black_row
	lda #6
	jsr pause_black_row
	lda #5
	jsr pause_black_row
	lda #4
	jsr pause_black_row
	lda #3
	jsr pause_black_row
	lda #2
	jsr pause_black_row
	lda #1
	jsr pause_black_row
	lda #0
	jsr pause_black_row
	; frame 8: black out attributes
	; copy att_mirror bottom half to nmi_update
	jsr pause_copy_att
	; black out appropriate attributes
	ldx #0
	:
		; 23E0/27E0 - no change
		; 23E8/27E8 - black bottom half
		lda nmi_update+ 0+ 8, X
		and #%00001111
		sta nmi_update+ 0+ 8, X
		lda nmi_update+32+ 8, X
		and #%00001111
		sta nmi_update+32+ 8, X
		; 23F0/27F0 - black all
		lda #0
		sta nmi_update+ 0+16, X
		sta nmi_update+32+16, X
		; 23F8/27F8 - black all
		sta nmi_update+ 0+24, X
		sta nmi_update+32+24, X
		; next
		inx
		cpx #8
		bcc :-
	;
	jsr pause_render_att
	; frame 9: solid line at top
	lda #$6F
	jsr nmi_double_fill
	NMI_UPDATE_AT 0,22
	jsr pause_draw_holding
	jsr pause_render_wide
	; frame 10: text row 1
	NMI_UPDATE_AT 0,(22+1)
	lda text_select
	cmp #2
	bne :+
		lda #TEXT_MY_LIZARD ; pause
		jmp :+++
	:
	cmp #1
	bne :+
		lda game_message ; message
		jmp :++
	:
		lda #TEXT_BLANK ; talk
	:
	jsr text_load
	jsr nmi_double_scroll
	jsr pause_draw_holding
	jsr pause_render_wide
	; frame 11: text row 2
	NMI_UPDATE_AT 0,(22+2)
	lda text_select
	cmp #2
	bne :+
		.assert (TEXT_LIZARD_KNOWLEDGE = TEXT_LIZARD_KNOWLEDGE + LIZARD_OF_KNOWLEDGE), error, "Lizard name text out of order."
		.assert (TEXT_LIZARD_BOUNCE    = TEXT_LIZARD_KNOWLEDGE + LIZARD_OF_BOUNCE   ), error, "Lizard name text out of order."
		.assert (TEXT_LIZARD_SWIM      = TEXT_LIZARD_KNOWLEDGE + LIZARD_OF_SWIM     ), error, "Lizard name text out of order."
		.assert (TEXT_LIZARD_HEAT      = TEXT_LIZARD_KNOWLEDGE + LIZARD_OF_HEAT     ), error, "Lizard name text out of order."
		.assert (TEXT_LIZARD_SURF      = TEXT_LIZARD_KNOWLEDGE + LIZARD_OF_SURF     ), error, "Lizard name text out of order."
		.assert (TEXT_LIZARD_PUSH      = TEXT_LIZARD_KNOWLEDGE + LIZARD_OF_PUSH     ), error, "Lizard name text out of order."
		.assert (TEXT_LIZARD_STONE     = TEXT_LIZARD_KNOWLEDGE + LIZARD_OF_STONE    ), error, "Lizard name text out of order."
		.assert (TEXT_LIZARD_COFFEE    = TEXT_LIZARD_KNOWLEDGE + LIZARD_OF_COFFEE   ), error, "Lizard name text out of order."
		.assert (TEXT_LIZARD_LOUNGE    = TEXT_LIZARD_KNOWLEDGE + LIZARD_OF_LOUNGE   ), error, "Lizard name text out of order."
		.assert (TEXT_LIZARD_DEATH     = TEXT_LIZARD_KNOWLEDGE + LIZARD_OF_DEATH    ), error, "Lizard name text out of order."
		.assert (TEXT_LIZARD_BEYOND    = TEXT_LIZARD_KNOWLEDGE + LIZARD_OF_BEYOND   ), error, "Lizard name text out of order."
		lda current_lizard
		clc
		adc #TEXT_LIZARD_KNOWLEDGE ; pause
		jsr text_load
		jmp :+++
	:
	cmp #1
	bne :+
		jsr text_continue ; message
		jmp :++
	:
		lda #TEXT_BLANK ; talk
		jsr text_load
	:
	jsr nmi_double_scroll
	jsr pause_draw_holding
	jsr pause_render_wide
	; frame 12: text row 3
	NMI_UPDATE_AT 0,(22+3)
	lda text_select
	cmp #1
	bne :+
		jsr text_continue ; message
		jmp :++
	:
		lda #TEXT_BLANK ; pause/talk
		jsr text_load
	:
	jsr nmi_double_scroll
	jsr pause_draw_holding
	jsr pause_render_wide
	; frame 13: text row 4
	NMI_UPDATE_AT 0,(22+4)
	lda text_select
	cmp #2
	bne @password_end
		; pause
		lda #TEXT_PASSWORD
		jsr text_load
		ldx #0
		:
			lda password, X
			clc
			adc nmi_update+13, X
			sta nmi_update+13, X
			inx
			cpx #5
			bcc :-
		jmp :++
	@password_end:
	cmp #1
	bne :+
		jsr text_continue ; message
		jmp :++
	:
		lda #TEXT_BLANK ; talk
		jsr text_load
	:
	jsr nmi_double_scroll
	jsr pause_draw_holding
	jsr pause_render_wide
	; frame 14: text row 5
	NMI_UPDATE_AT 0,(22+5)
	lda text_select
	bne :+
		lda #TEXT_BLANK ; talk
		jmp :++
	:
		lda #TEXT_UNPAUSE
	:
	jsr text_load
	jsr nmi_double_scroll
	jsr pause_draw_holding
	jsr pause_render_wide
	; fully paused
mode_paused:
	lda text_select
	bne :+
		jsr mode_talk
	: ; wait for START to be released
		jsr pause_draw_holding
		jsr pause_render_frame
		jsr gamepad_poll
		and #PAD_START
		bne :-
	lda #0
	sta game_pause
	sta mode_temp
	pha ; index to lizard_cheat_code (m)
	pha ; index to lizard_diagnostic_code (n)
@pause_loop:
	jsr pause_draw_holding
	jsr pause_render_frame
	jsr gamepad_poll
	lda mode_temp
	bne :+
		; simple loop until START is released
		lda gamepad
		and #PAD_START
		bne @pause_loop
		lda #1
		sta mode_temp
	:
	; cheat code / diagnostic code
	pla
	sta n ; diagnostic index
	pla
	sta m ; cheat index
	lda gamepad
	ldx m
	cmp lizard_cheat_code, X
	beq @cheat_end
		; gamepad doesn't match current cheat index, advance it
		inx
		cmp lizard_cheat_code, X
		beq :+
			; cheat not matched, reset
			ldx #0
		:
		stx m
		cpx #8
		bcc @cheat_end
		; cheat string finished
		lda #1
		sta metric_cheater
		inc current_lizard
		lda current_lizard
		.if DEBUG
			cmp #LIZARD_OF_COUNT
		.else
			cmp #LIZARD_OF_COFFEE
		.endif
		bcc :+
			lda #0
			sta current_lizard
		:
		sta next_lizard
		lda #0
		sta lizard_power
		PLAY_SOUND SOUND_SWITCH
		ldx #0
		@stone_loop:
			lda dog_type, X
			cmp #DOG_SAVE_STONE
			bne :+
				lda #0
				sta dgd_SAVE_STONE_ON, X
			:
			inx
			cpx #16
			bcc @stone_loop
		lda #0
		pha
		pha
		jmp mode_unpause
	@cheat_end:
	ldx n
	lda gamepad
	cmp lizard_diagnostic_code, X
	beq @diagnostic_end
		inx
		cmp lizard_diagnostic_code, X
		beq :+
			ldx #0
		:
		stx n
		cpx #8
		; don't allow re-entry of diagnostic room
		bcc @diagnostic_end
		lda current_room+0
		cmp #<DATA_room_diagnostic
		bne :+
		lda current_room+1
		cmp #>DATA_room_diagnostic
		beq @diagnostic_end
		:
		; go to diagnostic room
		lda #1
		sta metric_cheater
		lda #2
		sta room_change
		lda #0
		sta current_door
		lda current_room+0
		sta diagnostic_room+0
		lda current_room+1
		sta diagnostic_room+1
		lda #<DATA_room_diagnostic
		sta current_room+0
		lda #>DATA_room_diagnostic
		sta current_room+1
		lda #0
		sta player_pause
		lda #0
		pha
		pha
		jmp mode_room_change
	@diagnostic_end:
	; loop
	lda m
	pha ; temporarily store cheat index
	lda n
	pha ; temporarily store diagnostic index
	; finish if START is pressed
	lda gamepad
	and #PAD_START
	bne mode_unpause
	jmp @pause_loop
mode_unpause:
	pla ; done with diagnostic code
	pla ; done with cheat code
	; black rows with text
	lda #5
	jsr pause_black_row_mid
	lda #4
	jsr pause_black_row_mid
	lda #3
	jsr pause_black_row_mid
	lda #2
	jsr pause_black_row_mid
	lda #1
	jsr pause_black_row_mid
	lda #0
	jsr pause_black_row
	; restore attributes
	jsr pause_copy_att
	jsr pause_render_att
	; restore tiles
	ldx #255 ; setup partial room load
	lda #$0 ; ROOM
	jsr bank_call
	lda #0
	jsr pause_restore_row
	lda #1
	jsr pause_restore_row
	lda #2
	jsr pause_restore_row
	lda #3
	jsr pause_restore_row
	lda #4
	jsr pause_restore_row
	lda #5
	jsr pause_restore_row
	lda #6
	jsr pause_restore_row
	lda #7
	jsr pause_restore_row
mode_pause_end:
	lda #0
	sta player_pause
	sta game_message
	sta text_select
	sta mode_temp ; player must release START to pause again
	lda end_book
	beq :+
		jmp mode_book
	:
	lda dog_type + RIVER_SLOT
	cmp #DOG_RIVER
	bne :+
		jsr render_frame
		jmp mode_river
	:
	jmp mode_play

mode_talk:
	lda game_message
	jsr text_start
	lda #0
	sta j ; frame counter
	@talk_loop:
		lda j
		and #63
		bne @load_line_end
			; every 64th frame load a new line
			lda text_more
			jeq @talk_loop_end
			jsr text_continue
			ldx #0
			:
				lda nmi_update, X
				sta scratch, X
				inx
				cpx #32
				bcc :-
		@load_line_end:
		; set line to draw
		NMI_UPDATE_AT 0,(22+2)
		lda j
		lsr
		and #(32 | 64)
		clc
		adc nmi_load+0
		sta nmi_load+0
		lda #0
		adc nmi_load+1
		sta nmi_load+1
		; copy text
		ldx #0
		:
			lda scratch, X
			sta nmi_update, X
			inx
			cpx #32
			bcc :-
		; confound
		lda current_lizard
		cmp #LIZARD_OF_KNOWLEDGE
		beq :+
		lda lizard_dismount
		cmp #3
		beq :+
			lda j
			jsr text_confound
		:
		; play sound
		lda j
		and #1
		beq :+
			lda j
			lsr
			and #31
			tax
			lda nmi_update, X
			cmp #$70 ; space
			beq :+
			PLAY_SOUND SOUND_TALK
		:
		; hide text gradually
		lda j
		lsr
		and #31
		tax
		inx
		cpx #32
		bcs :++
		:
			lda #$70
			sta nmi_update, X
			inx
			cpx #32
			bcc :-
		:
		; finish loop
		; store temporaries
		lda i
		pha
		lda j
		pha
		; stuff that might destroy temporaries
		jsr nmi_double_scroll
		jsr pause_draw_holding
		; restore temporaries
		pla
		sta j
		pla
		sta i
		; finish frame
		jsr pause_render_wide
		inc j
		lda j
		cmp #(64*3)
		bcs @talk_loop_end
		jmp @talk_loop
	@talk_loop_end:
	; finally display start indication before moving on
	NMI_UPDATE_TEXT 0,(22+5),TEXT_UNPAUSE
	jsr nmi_double_scroll
	jsr pause_draw_holding
	jsr pause_render_wide
	rts

mode_room_change:
	lda ending
	bne @fade_out_ending
	lda dog_type + RIVER_SLOT
	cmp #DOG_RIVER
	beq @fade_out_river
	jmp @fade_out
@fade_out_ending:
	jsr fade_prepare
	jsr fade_1
	jsr render_overlay
	jsr render_frame
	jsr render_overlay
	jsr render_frame
	jsr fade_2
	jsr render_overlay
	jsr render_frame
	jsr render_overlay
	jsr render_frame
	jsr fade_3
	jsr render_overlay
	jsr render_frame
	jsr render_overlay
	jsr render_frame
	jsr render_overlay
	jsr render_off
	lda #0
	sta ending
	jmp @change_room
@fade_out_river:
	jsr render_river
	jsr fade_prepare
	jsr fade_1
	jsr render_frame
	jsr render_river
	jsr render_frame
	jsr render_river
	jsr fade_2
	jsr render_frame
	jsr render_river
	jsr render_frame
	jsr render_river
	jsr fade_3
	jsr render_frame
	jsr render_river
	jsr render_frame
	jsr render_river
	jsr render_off
	jmp @change_room
@fade_out:
	jsr fade_prepare
	jsr fade_1
	jsr render_frame
	jsr render_frame
	jsr fade_2
	jsr render_frame
	jsr render_frame
	jsr fade_3
	jsr render_frame
	jsr render_frame
	jsr render_off
@change_room:
	; TIP NOT IN DMO
	lda next_lizard
	sta current_lizard
	jsr room_load
	; flip lizard if through door
	lda room_change
	cmp #2
	bne :+
		lda lizard_face
		eor #1
		sta lizard_face
	:
	lda #0
	sta room_change
	lda ending
	beq :+
		jmp mode_ending
	:
	lda dog_type + HOLD_SLOT
	cmp #DOG_HOLD_SCREEN
	bne :+
		; skip lizard draw
		lda #4
		sta oam_pos
		jsr dogs_draw_banked
		jsr finish_sprites
		jmp :++
	:
		jsr draw_sprites
	:
	; fade in
	jsr fade_prepare
	jsr fade_3
	jsr render_frame
	jsr render_frame
	jsr fade_2
	jsr render_frame
	jsr render_frame
	jsr fade_1
	jsr render_frame
	jsr render_frame
	jsr fade_finish
	; river needs to set up sprite 0 hit and go to mode_river
	lda dog_type + RIVER_SLOT
	cmp #DOG_RIVER
	beq @setup_river
	; not river, just finish fade and go to mode_play
@setup_play:
	jsr render_frame
	lda #0
	sta mode_temp ; player must release START before pause is allowed
	lda dog_type + HOLD_SLOT
	cmp #DOG_HOLD_SCREEN
	bne :+
		jmp mode_hold
	:
	jmp mode_play
@setup_river:
	jsr render_frame
	lda #0
	sta mode_temp
	jmp mode_river

; mode_hold
;
; holds until button press, or desired number of seconds (ticks dogs, not lizard, draws dogs)

mode_hold:
	lda #0
	sta mode_temp ; mode_hold_ready
	sta i ; mode_hold_frame
	sta j ; mode_hold_seconds
@loop:
	jsr gamepad_poll
	bne @press
		lda #1
		sta mode_temp
		jmp @press_end
	@press:
		and #(PAD_A | PAD_B | PAD_SELECT | PAD_START)
		beq @press_end
		lda mode_temp
		beq @press_end
		lda dog_param+HOLD_SLOT
		cmp #255
		bcs @press_end
		lda dog_type + TIP_SLOT
		cmp #DOG_TIP
		bne @press_do
		lda gamepad
		and #(PAD_START)
		beq @press_end
	@press_do:
		jmp mode_hold_finish
	@press_end:
	lda dog_param+HOLD_SLOT
	beq @wait_end
	@wait:
		inc i
		lda i
		cmp #60
		bcs @tick_second
		lda player_pal
		beq @tick_second_end
		lda i
		cmp #50
		bcc @tick_second_end
		@tick_second:
			lda #0
			sta i
			inc j
			lda j
			cmp dog_param+HOLD_SLOT
			bcc @tick_second_end
			lda dog_param+HOLD_SLOT
			cmp #255
			bcs @tick_second_end
			jmp mode_hold_finish
		@tick_second_end:
	@wait_end:
	jsr mode_hold_frame
	jmp @loop
mode_hold_frame:
	; temporarily store j/i
	lda j
	pha
	lda i
	pha
	; tick dogs
	lda easy
	cmp #1
	beq :+
		jsr dogs_tick_banked
	:
	; draw sprites without dogs
	lda #4
	sta oam_pos
	jsr dogs_draw_banked
	jsr finish_sprites
	; finish frame
	lda dog_param+HOLD_SLOT
	bne :+
		jsr tick_time
	:
	jsr tick_easy
	jsr render_next
	; restore j/i
	pla
	sta i
	pla
	sta j
	rts
mode_hold_finish:
	jsr mode_hold_frame
	lda #2
	sta room_change
	lda #1
	sta current_door
	tax
	lda door_link, X
	sta current_room+0
	lda door_linkh, X
	sta current_room+1
	lda dog_y + HOLD_SLOT
	cmp #255
	bne :+
		; tip return
		lda #1
		sta room_change
		lda tip_return_door
		sta current_door
		lda tip_return_room+0
		sta current_room+0
		lda tip_return_room+1
		sta current_room+1
	:
	jmp mode_room_change

; mode_ending
;
; scrolls ending text until START pressed

mode_ending:
	lda #0
	sta mode_temp
	; draw first page of 15 lines (should be blank lines so they don't pop in after fade in)
	jsr overlay_cls
	PPU_LATCH $2400
	lda #TEXT_CREDITS
	jsr text_start
	lda #0
	sta i
	:
		jsr text_continue
		jsr ppu_nmi_write_row
		inc i
		lda i
		cmp #16
		bcc :-
	; draw ending card
	lda #4
	sta oam_pos
	jsr dogs_draw_banked
	jsr finish_sprites
	; load palettes
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
	; palette 6 set by data
	lda human1_pal
	clc
	adc #DATA_palette_human0
	tax
	lda #7
	jsr palette_load
	; set sprite for split point
	jsr sprite_0_init
	lda #(120-4)
	sta oam+0
	lda #0
	sta scroll_x+0
	sta scroll_x+1
	sta overlay_scroll_y
	; fade in
	jsr fade_prepare
	jsr fade_3
	jsr render_frame
	jsr render_frame
	jsr fade_2
	jsr render_frame
	jsr render_frame
	jsr fade_1
	jsr render_frame
	jsr render_frame
	jsr fade_finish
	jsr render_frame
	; infinite loop on credits
	lda #16
	sta l ; l stores current write line
	lda #0
	sta k ; k stores current sub-scroll
@credits_loop:
	; check for end of text and halt scroll
	lda text_more
	beq @scroll_end
	; increment scroll every 4 frames (tracked by k)
	lda k
	bne @scroll_end
		lda overlay_scroll_y
		and #7
		bne @line_end ; every 8th frame we need a new row
			; load row of text
			jsr text_continue
			; set PPU address of row
			lda l
			asl
			asl
			asl
			asl
			asl
			sta nmi_load+0
			lda l
			lsr
			lsr
			lsr
			clc
			adc #$24
			sta nmi_load+1
			; increment line
			inc l
			lda l
			cmp #30
			bcc :+
				lda #0
				sta l
			:
		@line_end:
		inc overlay_scroll_y
		lda overlay_scroll_y
		cmp #240
		bcc :+
			lda #0
			sta overlay_scroll_y
		:
	@scroll_end:
	inc k
	lda k
	cmp #4
	bcc :+
		lda #0
		sta k
	:
	jsr render_overlay
	jsr render_row
	; exit on START after all input released
	jsr gamepad_poll
	bne :+
		lda #1
		sta mode_temp
		jmp @credits_loop
	:
	cmp #PAD_START
	bne :+
	lda mode_temp
	beq :+
		jmp mode_ending_fade_out
	:
	jmp @credits_loop
mode_ending_fade_out:
	; randomize human1 hair/palette
	jsr prng8
	sta human1_hair
	:
		jsr prng
		jsr prng
		and #3
		beq :- ; reroll a 0
	sta human1_pal
	dec human1_pal ; = one of 0,1,2
	; return to "starting" room
	lda #LIZARD_OF_KNOWLEDGE
	sta current_lizard
	sta next_lizard
	lda #$FF
	sta last_lizard
	lda #<DATA_room_start_again
	sta current_room+0
	lda #>DATA_room_start_again
	sta current_room+1
	lda #0
	sta current_door
	sta lizard_face
	sta lizard_power
	lda #1
	sta room_change
	jmp mode_room_change

; mode book
; 
; facilitates the automated book reading ending sequence without ticking game timer

mode_book:
	; skip frame if easy = 1
	lda easy
	beq :+
	cmp #1
	bne :+
		jmp @skip_frame
	:
	; do book
	lda #0
	sta lizard_power
	lda dog_param + BOOK_SLOT
	bne :+
		; 0 -> 2 (recto -> turn to verso)
		lda #2
		sta dog_param + BOOK_SLOT
		jmp @book_end
	:
	cmp #1
	bne :+
		; 1 -> 4 (verso -> turn to blank)
		lda #4
		sta dog_param + BOOK_SLOT
		PLAY_SOUND SOUND_TWINKLE
		jmp @book_end
	:
	cmp #5
	bne @book_end
		inc mode_temp
		bne @book_end
		inc end_book
		lda end_book
		cmp #5
		bcc @book_end
			lda #1
			sta room_change
			sta current_door
			; NOT IN DEMO
			;lda #<DATA_room_end_chain0
			;sta current_room + 0
			;lda #>DATA_room_end_chain0
			;sta current_room + 1
			jmp mode_room_change
		;
	@book_end:
	; tick dogs
	jsr dogs_tick_banked
@skip_frame:
	; draw sprites
	lda #4
	sta oam_pos
	.if PROFILE>1
		lda #%10011111 ; draw_sprites in blue
		sta $2001
	.endif
	lda dgd_BOOK_HUMAN + BOOK_SLOT
	cmp #64
	bcs :+
	jsr prng
	and #63
	cmp dgd_BOOK_HUMAN + BOOK_SLOT
	bcc :+
		jsr lizard_draw
	:
	jsr dogs_draw_banked
	jsr finish_sprites
	; do not tick_time
	jsr tick_easy
	jsr render_next
	jmp mode_book

; ==========
; soundtrack
; ==========

soundtrack:
	lda #$FF
	sta player_music_mask ; music on
	jsr render_off
	lda #<DATA_room_soundtrack
	sta current_room+0
	lda #>DATA_room_soundtrack
	sta current_room+1
	jsr room_load
	; wipe OAM
	lda #4
	sta oam_pos
	jsr finish_sprites
	;
	@loop:
		; draw the lizard indicator
		lda #4
		sta oam_pos
		lda player_next_music
		asl
		asl
		asl
		pha ; store track * 8
		clc
		adc #64
		tay ; y = 64 + (8 * player_next_music)
		lda #((88 & $80) | $40)
		sta sprite_center
		ldx #88
		lda #DATA_sprite2_lizard_stand
		jsr sprite2_add
		lda #2
		sta i ; attribute
		pla
		clc
		adc #55
		tay ; y = 55 + (8 * player_next_music)
		ldx #95
		lda #$67
		jsr sprite_tile_add
		jsr finish_sprites
		jsr render_frame
		: ; wait for release
			jsr gamepad_poll
			bne :-
		: ; wait for press
			jsr gamepad_poll
			beq :-
		; something has been pressed
		lda gamepad
		and #PAD_SELECT
		beq :+
			jmp ($FFFC) ; reset
		:
		lda gamepad
		and #PAD_U
		beq :++
			lda player_next_music
			cmp #1
			bcc :+
				dec player_next_music
			:
		:
		lda gamepad
		and #PAD_D
		beq :++
			lda player_next_music
			cmp #18
			bcs :+
				inc player_next_music
			:
		:
	jmp @loop

; ===========
; NMI handler
; ===========

.export nmi_handler
nmi_handler:
	; prevent NMI re-entry
	lda nmi_lock
	beq :+
		rts
	:
	lda #1
	sta nmi_lock
	; increment frame counter
	inc nmi_count
	;
	lda nmi_ready
	.assert NMI_NONE = 0, error, "NMI_NONE != 0"
	bne :+
		jmp @ppu_update_done
	:
	; sprite OAM DMA
	ldx #0
	stx $2003
	.assert <oam = 0, error, "oam allocation misaligned"
	lda #>oam
	sta $4014
	; palettes
	lda #%10001000
	sta $2000 ; set horizontal nametable increment
	lda $2002 ; reset latch
	lda #$3F  ; latch $3F00
	sta $2006
	stx $2006
	.repeat 32, I
		lda palette+I
		sta $2007
	.endrepeat
	; latch for nametable update
	lda nmi_load+1
	sta $2006
	lda nmi_load+0
	sta $2006
	;
	lda nmi_ready
	cmp #NMI_ROW ; horizontal 32 bytes
	bne @nmi_row_end
		tsx ; store stack pointer in Y
		txa
		tay
		ldx #$FF ; point stack at 0 (nmi_update)
		txs
		ldx #4 ; 4 segment unroll
	@ppu_write_8:
		:
			.repeat 8
				pla
				sta $2007
			.endrepeat
			dex
			bne :-
		tya ; restore stack pointer
		tax
		txs
		jmp @ppu_update_scroll
	@nmi_row_end:
	cmp #NMI_DOUBLE ; horizontal 64 bytes
	bne @nmi_double_end
		tsx ; store stack pointer in Y
		txa
		tay
		ldx #$FF ; point stack at 0 (nmi_update)
		txs
		ldx #8 ; 4 segment unroll
		jmp @ppu_write_8
	@nmi_double_end:
	cmp #NMI_WIDE ; horizontal split 64 bytes
	bne @nmi_wide_end
		tsx
		txa
		tay
		ldx #$FF ; point stack at 0 (nmi_update)
		txs
		ldx #4
		:
			.repeat 8
				pla
				sta $2007
			.endrepeat
			dex
			bne :-
		lda nmi_load+1
		eor #$04
		sta $2006
		lda nmi_load+0
		sta $2006
		ldx #4
		jmp @ppu_write_8
	@nmi_wide_end:
	cmp #NMI_STREAM ; triplet stream
	bne @nmi_stream_end
		tsx
		txa
		tay
		ldx #$FF
		txs
		pla
		beq @stream_end
		cmp #22
		bcc :+
			lda #21
		:
		tax
		@stream_triplet:
			pla
			sta $2006
			pla
			sta $2006
			pla
			sta $2007
			dex
			bne @stream_triplet
		@stream_end:
		tya
		tax
		txs
		jmp @ppu_update_scroll
	@nmi_stream_end:
	cmp #NMI_OFF ; render off
	bne @nmi_off_end
		lda #%00000000
		sta $2001
		ldx #NMI_NONE
		stx nmi_ready
		jmp @ppu_update_done
	@nmi_off_end:
	; should be NMI_READY if we get here
@ppu_update_scroll:
	; scroll
	lda scroll_x+1
	and #%00000001
	ora #%10001000
	sta $2000
	lda scroll_x+0
	sta $2005
	lda #0
	sta $2005
	; enable rendering
	lda #%00011110
	sta $2001
	; flag ppu update complete
	ldx #NMI_NONE
	stx nmi_ready
@ppu_update_done:
	; music_play
	lda #$C ; MUSIC
	ldx #0
	jsr bank_call
	; unlock
	lda #0
	sta nmi_lock
	rts

; ===========
; IRQ handler
; ===========

; crash diagnostic screen

.segment "CODE"

crash_text:
	jsr text_load
	jmp ppu_nmi_write_row

.macro CRASH_TEXT tx, ty, text_
	PPU_LATCH_AT tx, ty
	lda #text_
	jsr crash_text
.endmacro

; A = value to write
crash_hex:
	pha
	lsr
	lsr
	lsr
	lsr
	jsr :+
	pla
	and #$F
:
	tay
	lda hex_to_ascii, Y
	sta $2007
	rts

.macro CRASH_HEX tx, ty
	pha
	PPU_LATCH_AT tx, ty
	pla
	jsr crash_hex
.endmacro

.export irq_handler
irq_handler:
	; stop rendering and sound
	lda #0
	sta $2001
	sta $4015
	; load ASCII CHR
	PPU_LATCH $0400
	lda #0
	sta m ; FLAG_EYESIGHT for chr_load
	lda #DATA_chr_font
	jsr chr_load
	; palette
	PPU_LATCH $3F00
	ldx #8
	:
		lda #$01 ; blue screen of death
		sta $2007
		lda #$21 ; light blue
		sta $2007
		lda #$31 ; lightest blue
		sta $2007
		lda #$30 ; white
		sta $2007
		dex
		bne :-
	; wipe middle of screen
	PPU_LATCH_AT 0,6
	lda #$70 ; blank
	ldy #18 ; lines to blank
	:
		ldx #32
		:
			sta $2007
			dex
			bne :-
		dey
		bne :--
	; display crash info
	;
	; STACK:
	;  1 bank
	;  2 y
	;  3 x
	;  4 a
	;  5 p
	;  6 pc lsb
	;  7 pc msb
	;  8 stack...
	;
	CRASH_TEXT 11, 7,TEXT_CRASH_CRASH
	CRASH_TEXT  8, 9,TEXT_CRASH_A
	tsx
	lda $100+4,X
	CRASH_HEX  11, 9
	CRASH_TEXT  8,10,TEXT_CRASH_X
	tsx
	lda $100+3,X
	CRASH_HEX  11,10
	CRASH_TEXT  8,11,TEXT_CRASH_Y
	tsx
	lda $100+2,X
	CRASH_HEX  11,11
	CRASH_TEXT  8,12,TEXT_CRASH_S
	tsx
	txa
	clc
	adc #7
	CRASH_HEX  11,12
	CRASH_TEXT  8,13,TEXT_CRASH_P
	tsx
	lda $100+5,X
	CRASH_HEX  11,13
	CRASH_TEXT  8,14,TEXT_CRASH_PC
	tsx
	lda $100+6,X
	sec
	sbc #2
	sta $FF
	lda $100+7,X
	sbc #0 ; PC-2 MSB
	CRASH_HEX  12,14
	lda $FF ; PC-2 LSB
	jsr crash_hex 
	CRASH_TEXT  8,15,TEXT_CRASH_BANK
	tsx
	lda $100+1,X
	CRASH_HEX  14,15
	PPU_LATCH_AT 8,17
	tsx
	txa
	clc
	adc #8
	beq @stack_end
	tax
	ldy #4
	sty $FE
	@stack_line:
		ldy #8
		:
			lda $100,X
			sty $FF
			jsr crash_hex
			inx
			beq @stack_end
			ldy $FF
			dey
			bne :-
		ldy #16
		lda #$70 ; blank
		:
			sta $2007
			dey
			bne :-
		dec $FE
		bne @stack_line
	@stack_end:
	CRASH_TEXT  6,22,TEXT_CRASH_STOP
	; start rendering (no sprites)
	lda #0
	sta $2005
	sta $2005
	lda #%00001110
	sta $2001
	; wait for control release
	:
		jsr gamepad_poll
		bne :-
	; wait for START
	:
		jsr gamepad_poll
		cmp #PAD_START
		bne :-
	; reset
	jmp ($FFFC)

; ====
; main
; ====

.export main
main:
	; all RAM is set to 0 before main is entered
	; except for the password and PRNG seed
	;
	; set the starting lizard
	lda #LIZARD_OF_START
	sta current_lizard
	sta next_lizard
	; clear the CHR cache
	lda #$FF
	.repeat 8,I
		sta chr_cache+I
	.endrepeat
	; reset last lizard
	sta last_lizard
	sta last_lizard_save
	sta easy ; set back to 0 after dogs init
	; silence
	lda #MUSIC_SILENT
	sta player_next_music
	; load the starting room
	lda #<DATA_room_start
	sta current_room+0
	sta tip_return_room+0
	lda #>DATA_room_start
	sta current_room+1
	sta tip_return_room+1
	jsr room_load
	; display start text
	PPU_LATCH_AT 0,13
	lda #TEXT_PUSH_START
	jsr text_load
	; display version text
	PPU_LATCH_AT 0,11
	lda #TEXT_VERSION
	jsr text_load
	.if BETA
		lda hex_to_ascii + VERSION_MAJOR
		sta nmi_update+18
		lda hex_to_ascii + VERSION_MINOR
		sta nmi_update+20
		lda hex_to_ascii + VERSION_BETA
		sta nmi_update+22
		lda hex_to_ascii + VERSION_REVISED
		sta nmi_update+24
	.else
		; to keep binary footprint the same when BETA ends
		.repeat 3*8
			nop
		.endrepeat
	.endif
	jsr ppu_nmi_write_row
	; fix the password if invalid (happens at power on)
	jsr password_read
	beq @bad_password
	jsr checkpoint
	bne @good_password
	@bad_password:
		jsr password_build
	@good_password:
	jsr sprite_0_init
	lda #NMI_READY
	sta nmi_next
	; enable NMI to kick things off (NMI will never be disabled)
	lda $2002
	lda #%10001000
	sta $2000
	jmp mode_title ; begin the title screen

; end of file
