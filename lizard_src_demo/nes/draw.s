; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; draw
;
; sprite drawing and other render code
;

.include "ram.s"

.segment "DATA"
.include "../assets/export/palette.s"
.import data_sprite_tiles, data_sprite_low, data_sprite_high, data_sprite_tile_count, data_sprite_vpal

; sprite center: parameter set by DX_SCREEN, DX_SCROLL, DX_SCROLL_OFFSET, DX_SCROLL_EDGE, or sprite_prepare
;   bit 6 set means the sprite is in the centre 50% of screen (no edge testing)
;   bit 7 is high bit of X, used for edge testing (flipped if on an offscreen edge)

.segment "CODE"
.export sprite_prepare       ; A=x (clobbers A) sets sprite_center for fixed pos in A
.export sprite_add           ; sprite A, X=x, Y=y, sprite_center (clobbers A,X,Y,i,j,k)
.export sprite_add_flipped   ; sprite A, X=x, Y=y, sprite_center (clobbers A,X,Y,i,j,k)
.export sprite_tile_add      ; A=tile, X=x, Y=y, i=attribute (clobbers A,X)
.export sprite_tile_add_clip ; A=tile, X=x, Y=y, i=attribute, sprite_center (clobbers A,X)
.export palette_load         ; A=slot, X=palette (clobbers Y)

;
; sprite_prapare
;

; A = X position
sprite_prepare:
	sta sprite_center
	lsr
	eor sprite_center
	and #$C0
	sta sprite_center
	rts

;
; sprite_add_edge (internal)
;

sprite_add_edge:
	; i,j = x,y
	; k = tile count
	; temp_ptr0 = tiles
	; oam_pos = oam position to write
	ldx oam_pos
	bne :+
		rts ; OAM is full
	:
	ldy #0
	@loop:
		lda (temp_ptr0), Y
		clc
		adc j
		cmp #239
		bcs @skip_bottom_tile
		sta oam, X
		inx
		iny
		lda (temp_ptr0), Y
		sta oam, X
		inx
		iny
		lda (temp_ptr0), Y
		ora att_or
		sta oam, X
		inx
		iny
		lda (temp_ptr0), Y
		clc
		adc i
		sta oam, X
		; edge test
		eor sprite_center
		bpl @not_wrap
		@wrap:
			dex
			dex
			dex
			lda #$FF
			sta oam, X
			jmp @continue
		@not_wrap:
			inx
			bne @continue
				stx oam_pos
				rts ; OAM is full
			@skip_bottom_tile:
				iny
				iny
				iny
			;
		@continue:
		iny
		dec k
		bne @loop
	stx oam_pos
	rts

;
; sprite_add
; sprite_add_flipped
;

; A = sprite
; X = X position
; Y = Y position
sprite_add:
	; setup
	stx i
	sty j
	tax
	lda data_sprite_low, X
	sta temp_ptr0+0
	lda data_sprite_high, X
	sta temp_ptr0+1
	lda data_sprite_vpal, X
	beq :+
		lda dog_now
		and #1
		sta att_or
		jmp :++
	:
		lda #0
		sta att_or
	:
	lda data_sprite_tile_count, X
	sta k
	; add sprite
	bit sprite_center
	bvc sprite_add_edge
	; i,j = x,y
	; k = tile count
	; temp_ptr0 = tiles
	; oam_pos = oam position to write
	ldx oam_pos
	bne :+
		rts ; OAM is full
	:
	ldy #0
	@loop:
		lda (temp_ptr0), Y
		clc
		adc j
		cmp #239
		bcs @skip_bottom_tile
		sta oam, X
		inx
		iny
		lda (temp_ptr0), Y
		sta oam, X
		inx
		iny
		lda (temp_ptr0), Y
		ora att_or
		sta oam, X
		inx
		iny
		lda (temp_ptr0), Y
		clc
		adc i
		sta oam, X
		inx
		bne @continue
			stx oam_pos
			rts ; OAM is full
		@skip_bottom_tile:
			iny
			iny
			iny
		@continue:
		iny
		dec k
		bne @loop
	stx oam_pos
	rts

sprite_add_flipped:
	; setup
	stx i
	sty j
	tax
	lda data_sprite_low, X
	sta temp_ptr0+0
	lda data_sprite_high, X
	sta temp_ptr0+1
	lda data_sprite_vpal, X
	beq :+
		lda dog_now
		and #1
		sta att_or
		jmp :++
	:
		lda #0
		sta att_or
	:
	lda data_sprite_tile_count, X
	sta k
	; add sprite
	bit sprite_center
	bvc sprite_add_flipped_edge
	; i,j = x,y
	; k = tile count
	; temp_ptr0 = tiles
	; oam_pos = oam position to write
	ldx oam_pos
	bne :+
		rts ; OAM is full
	:
	ldy #0
	@loop:
		lda (temp_ptr0), Y
		clc
		adc j
		cmp #239
		bcs @skip_bottom_tile
		sta oam, X
		inx
		iny
		lda (temp_ptr0), Y
		sta oam, X
		inx
		iny
		lda (temp_ptr0), Y
		eor #$40 ; flip horizontal
		ora att_or
		sta oam, X
		inx
		iny
		; subtract X position
		lda i
		sec
		sbc #8
		sec
		sbc (temp_ptr0), Y
		sta oam, X
		inx
		bne @continue
			stx oam_pos
			rts ; OAM is full
		@skip_bottom_tile:
			iny
			iny
			iny
		@continue:
		iny
		dec k
		bne @loop
	stx oam_pos
	rts
; optimization ideas:
; - could pre-subtract 8 from X
; - the att_or could probably have been an XOR instead of OR,
;   which would let me roll the #$40 into it, in hindsight.
; (won't try these this late in production, maybe in my next game)

sprite_add_flipped_edge:
	; i,j = x,y
	; k = tile count
	; temp_ptr0 = tiles
	; oam_pos = oam position to write
	ldx oam_pos
	bne :+
		rts ; OAM is full
	:
	ldy #0
	@loop:
		lda (temp_ptr0), Y
		clc
		adc j
		cmp #239
		bcs @skip_bottom_tile
		sta oam, X
		inx
		iny
		lda (temp_ptr0), Y
		sta oam, X
		inx
		iny
		lda (temp_ptr0), Y
		eor #$40 ; flip horizontal
		ora att_or
		sta oam, X
		inx
		iny
		; subtract X position
		lda i
		sec
		sbc #8
		sec
		sbc (temp_ptr0), Y
		sta oam, X
		; edge test
		eor sprite_center
		bpl @not_wrap
		@wrap:
			dex
			dex
			dex
			lda #$FF
			sta oam, X
			jmp @continue
		@not_wrap:
			inx
			bne @continue
				stx oam_pos
				rts ; OAM is full
			@skip_bottom_tile:
				iny
				iny
				iny
			;
		@continue:
		iny
		dec k
		bne @loop
	stx oam_pos
	rts

;
; public single tile sprite functions
;

; A = tile
; X = x
; Y = y
; i = attribute
sprite_tile_add:
	pha
	txa
	ldx oam_pos
	beq @oam_full
	sta oam+3, X ; x
	pla
	sta oam+1, X ; tile
	tya
	sta oam+0, X ; y
	lda i
	sta oam+2, X ; attribute
	inx
	inx
	inx
	inx
	stx oam_pos
	rts
@oam_full:
	pla
	rts

; sprite_tile_add but clips tiles near the edge based on reference in sprite_center
sprite_tile_add_clip:
	pha
	txa
	cmp #64
	bcc @edge
	cmp #192
	bcs @edge
	bit sprite_center
	bvc @edge
	; middle of screen, just do it
@do_it:
	pla
	jmp sprite_tile_add
@edge:
	eor sprite_center
	; sign matches reference, do it
	bpl @do_it
	; sign mismatch, clip it
	pla
	rts

;
; palette
;

; A = slot
; X = palette
palette_load:
	asl
	asl
	tay
	lda data_palette_0, X
	sta palette+0, Y
	lda data_palette_1, X
	sta palette+1, Y
	lda data_palette_2, X
	sta palette+2, Y
	lda data_palette_3, X
	sta palette+3, Y
	rts

; end of file
