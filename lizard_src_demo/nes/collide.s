; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; collide
;
; collision tests
;

.include "ram.s"

.segment "CODE"

.export collide_set_tile   ; X/Y = tile location, clobbers A,X,Y,n
.export collide_clear_tile ; X/Y = tile location, clobbers A,X,Y,n

.export collide_tile       ; ih:X/Y = pixel location, result in A and Z flag, clobbers n,o,p
.export collide_all        ; ih:X/Y = pixel location, result in A and Z flag, clobbers n,o,p

.export collide_tile_left  ; ih:X/Y = pixel location, shift result in A,j w/Z flag, clobbers i,j,n,o,p
.export collide_tile_right ; ih:X/Y = pixel location, shift result in A,j w/Z flag, clobbers i,j,n,o,p
.export collide_tile_up    ; ih:X/Y = pixel location, shift result in A,j w/Z flag, clobbers i,j,n,o,p
.export collide_tile_down  ; ih:X/Y = pixel location, shift result in A,j w/Z flag, clobbers i,j,n,o,p

.export collide_all_left   ; ih:X/Y = pixel location, shift result in A,j w/Z flag, clobbers i,j,n,o,p
.export collide_all_right  ; ih:X/Y = pixel location, shift result in A,j w/Z flag, clobbers i,j,n,o,p
.export collide_all_up     ; ih:X/Y = pixel location, shift result in A,j w/Z flag, clobbers i,j,n,o,p
.export collide_all_down   ; ih:X/Y = pixel location, shift result in A,j w/Z flag, clobbers i,j,n,o,p

; NOTE all collide functions leave X/Y intact, or in the shifting functions, X/Y will be adjusted for collision

;
; modify collision
;

; internal function to generate collision tile index
; input: X/Y tile index
; result:
; A = bitmask
; Y = index
; clobbers A,X,Y,n
collide_tile_index:
	txa
	and #31
	lsr
	lsr
	sta n
	tya
	asl
	asl
	asl
	ora n
	tay ; Y = cidx
	cpx #32
	bcs @right
@left:
	txa
	and #3
	tax
	lda #1
	cpx #0
	:
		beq :+
		asl
		dex
		jmp :-
	:
	rts
@right:
	txa
	and #3
	tax
	lda #$80
	cpx #0
	:
		beq :+
		lsr
		dex
		jmp :-
	:
	rts

collide_set_tile:
	cpx #64
	bcs :+
		jsr collide_tile_index
		sta n
		lda collision, Y
		ora n
		sta collision, Y
	:
	rts

collide_clear_tile:
	cpx #64
	bcs :+
		jsr collide_tile_index
		eor #%11111111
		sta n
		lda collision, Y
		and n
		sta collision, Y
	:
	rts

;
; blocker collisions, unrolled
;

.repeat 4, I
.ident(.sprintf("collide_blocker%d",I)):
	; test x overlap
	cpx blocker_x0+I ; test X >= x0
	lda ih
	sbc blocker_xh0+I
	bcc @no_collide
	cpx blocker_x1+I ; test X <= x1
	lda ih
	sbc blocker_xh1+I
	bcc @x_overlap
		bne @no_collide
		cpx blocker_x1+I ; re-compare low byte for equality test
		bne @no_collide
	@x_overlap:
	; x overlap succeeded
	; text y overlap
	cpy blocker_y0+I ; test Y >= y0
	bcc @no_collide
	cpy blocker_y1+I ; test Y <= y1
	bcc @collide
		bne @no_collide
	@collide:
	lda #1
	rts
@no_collide:
	lda #0
	rts
.endrepeat

;
; pixel vs world collisions
;

; ih:X/Y pixel location to test
; result in A and Z flag
collide_tile:
	stx o
	sty p
	txa
	lsr
	lsr
	lsr
	pha
	and #3
	tax ; X = tile x & 3
	pla
	lsr
	lsr
	sta n
	tya
	and #%11111000
	ora n
	tay ; Y = (tile y << 3) | (tile x >> 2)
	; branch on bit 0 of ih
	lda ih
	ror
	bcc @low
@high:
	lda collision, Y
	cpx #0
	bne :+
		rol
		rol
		jmp @finish
	:
	rol
	:
		rol
		dex
		bne :-
	rol ; desired bit now in bit 0
	jmp @finish
@low:
	lda collision, Y
	cpx #0
	beq :++
	:
		ror
		dex
		bne :-
	:
	;
@finish:
	ldy p
	ldx o
	and #1
	rts

; ih:X/Y pixel location to test
; result in A and Z flag
collide_all:
	jsr collide_tile
	bne @collide_all_end
	.repeat 4,I
		jsr .ident(.sprintf("collide_blocker%d",I))
		bne @collide_all_end
	.endrepeat
@collide_all_end:
	rts

;
; directional collision resolution
;

collide_tile_left:
	lda #0
	sta j
	jmp collide_tile_left_internal

collide_tile_right:
	lda #0
	sta j
	jmp collide_tile_right_internal

collide_tile_up:
	lda #0
	sta j
	jmp collide_tile_up_internal

collide_tile_down:
	lda #0
	sta j
	jmp collide_tile_down_internal

; note: because blocker/tile sizes are small (as long as it's <128, should be fine),
;       and collision test is always done before calculating shift,
;       the high bit of new_x - x is irrelevant, shift would be the same either way

collide_all_left:
	lda #0
	sta j ; j is shift
	.repeat 4,I
		jsr .ident(.sprintf("collide_blocker%d",I))
		beq :+
			stx i ; i is old x
			lda blocker_x1+I
			clc
			adc #1
			tax ; x = new_x = blocker_x1+1
			lda blocker_xh1+I
			adc #0
			sta ih ; ih = new_x (high)
			txa
			sec
			sbc i ; new_x - x (high bit irrelevant)
			clc
			adc j ; shift += (new_x - x)
			sta j
		:
	.endrepeat
collide_tile_left_internal:
	jsr collide_tile
	beq :+
		stx i ; i is old x
		txa
		and #(7^$FF)
		clc
		adc #8 ; new_x = x&~7 + 8
		tax ; x = new_x
		lda ih
		adc #0 ; ih = new_x (high)
		sta ih
		txa
		sec
		sbc i ; new_x - x (high bit irrelevant)
		clc
		adc j ; shift += (new_x - x)
		sta j
	:
	lda j ; A = shift
	rts

collide_all_right:
	lda #0
	sta j ; j is shift
	.repeat 4,I
		jsr .ident(.sprintf("collide_blocker%d",I))
		beq :+
			lda blocker_x0+I
			sec
			sbc #1 ; new_x = blocker_x0-1
			sta i ; i = new_x
			lda blocker_xh0+I
			sbc #0
			sta ih ; ih = new_x (high)
			txa
			sec
			sbc i ; x - new_x (high bit irrelevant)
			clc
			adc j ; shift += (x - new_x)
			sta j
			ldx i ; x = new_x
		:
	.endrepeat
collide_tile_right_internal:
	jsr collide_tile
	beq :+
		txa
		and #(7^$FF)
		sec
		sbc #1 ; new_x = x&~7 - 1
		sta i ; i = new_x
		lda ih
		sbc #0
		sta ih ; ih = new_x (high)
		txa
		sec
		sbc i ; x - new_x (high bit irrelevant)
		clc
		adc j ; shift += (x - new_x)
		sta j
		ldx i ; x = new_x
	:
	lda j ; A = shift
	rts

collide_all_up:
	lda #0
	sta j ; j is shift
	.repeat 4,I
		jsr .ident(.sprintf("collide_blocker%d",I))
		beq :+
			sty i ; i is old y
			lda blocker_y1+I
			clc
			adc #1
			tay ; y = new_y = blocker_y1+1
			sec
			sbc i ; new_y - y
			clc
			adc j ; shift += (new_y - y)
			sta j
		:
	.endrepeat
collide_tile_up_internal:
	jsr collide_tile
	beq :+
		sty i ; i is old y
		tya
		and #(7^$FF)
		clc
		adc #8 ; new_y = y&~7 + 8
		tay ; y = new_y
		sec
		sbc i ; new_y - y
		clc
		adc j ; shift += (new_y - y)
		sta j
	:
	lda j ; A = shift
	rts

collide_all_down:
	lda #0
	sta j ; j is shift
	.repeat 4,I
		jsr .ident(.sprintf("collide_blocker%d",I))
		beq :+
			lda blocker_y0+I
			sec
			sbc #1 ; new_y = blocker_y0-1
			sta i ; i = new_y
			tya
			sec
			sbc i ; y - new_y
			clc
			adc j ; shift += (y - new_y)
			sta j
			ldy i ; y = new_y
		:
	.endrepeat
collide_tile_down_internal:
	jsr collide_tile
	beq :+
		tya
		and #(7^$FF)
		sec
		sbc #1 ; new_y = y&~7 - 1
		sta i ; i = new_y
		tya
		sec
		sbc i ; y - new_y
		clc
		adc j ; shift += (y - new_y)
		sta j
		ldy i ; y = new_y
	:
	lda j ; A = shift
	rts

; end of file
