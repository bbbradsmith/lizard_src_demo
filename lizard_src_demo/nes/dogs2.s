; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; dogs3 (bank F)
;
; dog update and drawing
; a few leftover dogs that didn't fit into banks D or E
;

.feature force_range
.macpack longbranch

.export dog_init
.export dog_tick
.export dog_draw

.include "ram.s"
.include "enums.s"
.include "dogs_inc.s"
.import bank_call

;.import sprite_prepare
.import sprite_add
.import sprite_add_flipped
.import sprite_tile_add
.import sprite_tile_add_clip
.import palette_load
.import text_load
.import text_start
.import text_continue

.import ppu_nmi_write_row ; from common_df.s

sprite2_add         = sprite_add
sprite2_add_flipped = sprite_add_flipped

.include "../assets/export/names.s"
.include "../assets/export/text_set.s"

; ====
; Dogs
; ====

.segment "CODE"

; duplicated from dogs_common.s (this bank doesn't need all of dogs_common)
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

; DOG_TIP

stat_dog_tip:

dog_tip_count_lines:
	pha
	:
		lda text_more
		beq :+
		pla
		clc
		adc #1
		pha
		jsr text_continue
		jmp :-
	:
	pla
	rts

dog_tip_write_lines:
	jsr text_start
	:
		lda text_more
		beq :+
		jsr text_continue
		jsr ppu_nmi_write_row
		jmp :-
	:
	rts

dog_init_tip:
	lda dog_param, Y
	beq :+
		; forced ending text
		lda #TIP_MAX
		sta tip_index
		lda #1
		sta tip_return_door
		lda door_linkh + 1
		sta tip_return_room + 1
		lda door_link + 1
		sta tip_return_room + 0
		lda #0
		jmp :++
	:
	; centre the text
	lda #TEXT_TIP_HEADER
	jsr text_start
	lda #0
	jsr dog_tip_count_lines
	: ; ending text skips header
	pha
	lda #TEXT_TIP0
	clc
	adc tip_index
	jsr text_start
	pla
	jsr dog_tip_count_lines
	; A = line count
	lsr
	sta i
	lda #15
	sec
	sbc i
	; A = 15 - line count
	ldx #0
	stx ih
	bit $2002
	.repeat 5
		asl
		rol ih
	.endrepeat
	and #$FE
	clc
	adc #<$2000
	pha
	lda ih
	adc #>$2000
	sta $2006
	pla
	sta $2006
	; draw text
	ldy dog_now
	lda dog_param, Y
	bne :+
	lda #TEXT_TIP_HEADER
	jsr dog_tip_write_lines
	: ; ending text skips header
	lda #TEXT_TIP0
	clc
	adc tip_index
	jsr dog_tip_write_lines
	; push start
	lda #TEXT_BLANK
	jsr text_load
	jsr ppu_nmi_write_row
	lda #TEXT_UNPAUSE
	jsr text_load
	jsr ppu_nmi_write_row
	; increment index
	inc tip_index
	rts
dog_tick_tip:
dog_draw_tip:
	rts

; DOG_WIQUENCE
dot_stat_wiquence:

dog_init_wiquence:
dog_tick_wiquence:
dog_draw_wiquence:
	; NOT IN DEMO
	rts

; DOG_WITCH
dog_stat_witch:

dog_init_witch:
dog_tick_witch:
dog_draw_witch:
	; NOT IN DEMO
	rts

; DOG_BOOK
dog_stat_book:

dog_init_book:
dog_tick_book:
dog_draw_book:
	; NOT IN DEMO
	rts

; ===========
; jump tables
; ===========
.segment "DATA"
DOG_TABLE0 = 0
DOG_TABLE1 = 0
DOG_TABLE2 = 1
DOG_TABLE_LEN = DOG_COUNT - DOG_BANK2_START
.include "dogs_tables.s"

; ============
; public stuff
; ============
.segment "CODE"
stat_dog_public:

dog_init:
	ldy dog_now
	ldx dog_type, Y
	cmp #DOG_COUNT
	bcc :+
		brk ; crash on bad dog!
	:
	lda dog_init_table_high-DOG_BANK2_START, X
	pha
	lda dog_init_table_low-DOG_BANK2_START, X
	pha
	rts

dog_tick:
	ldy dog_now
	ldx dog_type, Y
	lda dog_tick_table_high-DOG_BANK2_START, X
	pha
	lda dog_tick_table_low-DOG_BANK2_START, X
	pha
	rts

dog_draw:
	ldy dog_now
	ldx dog_type, Y
	lda dog_draw_table_high-DOG_BANK2_START, X
	pha
	lda dog_draw_table_low-DOG_BANK2_START, X
	pha
	rts

stat_dog_end_of_file:
; end of file
