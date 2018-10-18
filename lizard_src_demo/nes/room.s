; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; room
;
; common room loader, used in all room data banks
;

.macpack longbranch

.include "ram.s"

.import bank_call
.import bank_return
.importzp BANK

.import DATA_ROOM_BANK_START
.import DATA_ROOM_BANK_END
.import data_room_low
.import data_room_high

.include "../assets/export/names.s"

LAST_BANK = $A

.segment "CODE"

; X = 0 load whole room
; X = 255 set up for partial load and load first row
; X = 254 continue partial load

.export room_load
room_load:
	; partial load in progress
	cpx #254
	jeq room_partial
	; resolve bank
	lda current_room+0
	cmp #<DATA_ROOM_BANK_END
	lda current_room+1
	sbc #>DATA_ROOM_BANK_END
	bcc @this_bank
		; go to next bank
		lda #(BANK+1)
		; check if we've gone too far
		cmp #LAST_BANK
		bcc :+
			brk ; out of banks!
		:
		jmp bank_call
	@this_bank:
	; calculate in-bank index
	lda current_room+0
	sec
	sbc #<DATA_ROOM_BANK_START
	tay ; note: high bit doesn't matter (pointer table is always <= 256 bytes)
	lda data_room_low, Y
	sta temp_ptr0+0
	lda data_room_high, Y
	sta temp_ptr0+1
	; partial load begin?
	cpx #255
	jeq room_partial_begin
	; reset PPU latch
	lda $2002
	; setup temporary variables for unpacking
	lda #0
	stx m ; m is room
	sta j ; j is collision table pointer
	sta k ; k = RLE run
	sta l ; l = RLE value
	; wipe collision table, solid row 30/31
	ldx #0
	lda #0
	:
		sta collision, X
		inx
		cpx #$F0
		bcc :-
	lda #$FF
	:
		sta collision, X
		inx
		bne :-
	;
	; block 0 (bottom 8 rows of nametable interleaved)
	;
	lda #(22*8)
	sta j ; move collision write pointer 22 rows down
	ldx #0
	; uv = PPU write address for row
	lda #$22
	sta u
	lda #$C0
	sta v
	@bottom_loop:
		txa
		pha
		; load nmt0 row
		jsr room_unpack_32
		lda u
		sta $2006
		lda v
		sta $2006
		jsr room_load_nmt0
		; load nmt1 row
		jsr room_unpack_32
		lda j
		sec
		sbc #8
		sta j ; rewind collision write pointer by 8 bytes
		lda u
		eor #$04 ; switch nametable
		sta $2006
		lda v
		sta $2006
		jsr room_load_nmt1
		; advance to next row
		lda v
		clc
		adc #32
		sta v
		lda u
		adc #0
		sta u
		; count rows
		pla
		tax
		inx
		cpx #8
		bcc @bottom_loop
	;
	; block 1 (everything else)
	;
	; row S0
	; 8 palettes, 8 CHR, water, music, scrolling
	jsr room_unpack_32
	ldx #0
	@s0_loop:
		; palettes are not in this bank,
		; temporarily store palette index in dog_data0
		lda nmi_update+0, X
		sta dog_data0, X
		; temporarily store CHR index in dog_data1
		lda nmi_update+8, X
		sta dog_data1, X
		inx
		cpx #8
		bcc @s0_loop
	lda nmi_update+16
	sta water
	lda nmi_update+17
	sta player_next_music
	lda nmi_update+18
	sta room_scrolling
	; row S1
	; 8 x door x0, door x1, link0, link1
	jsr room_unpack_32
	ldx #0
	@s1_loop:
		lda nmi_update+ 0, X
		sta door_xh, X
		lda nmi_update+ 8, X
		sta door_x, X
		lda nmi_update+16, X
		sta door_linkh, X
		lda nmi_update+24, X
		sta door_link, X
		inx
		cpx #8
		bcc @s1_loop
	; row S2
	; 8 x door y, padding, 16 x dog type
	jsr room_unpack_32
	ldx #0
	@s2_loop0:
		lda nmi_update+ 0, X
		sta door_y, X
		inx
		cpx #8
		bcc @s2_loop0
	ldx #0
	@s2_loop1:
		lda nmi_update+16, X
		sta dog_type, X
		inx
		cpx #16
		bcc @s2_loop1
	; row S3
	; 16 x dog x0, dog x1
	jsr room_unpack_32
	ldx #0
	@s3_loop:
		lda nmi_update+ 0, X
		sta dog_xh, X
		lda nmi_update+16, X
		sta dog_x, X
		inx
		cpx #16
		bcc @s3_loop
	; row S4
	; 16 x dog y, dog p
	jsr room_unpack_32
	ldx #0
	@s4_loop:
		lda nmi_update+ 0, X
		sta dog_y, X
		lda nmi_update+16, X
		sta dog_param, X
		inx
		cpx #16
		bcc @s4_loop
	; 22 rows nametable 0
	lda #$20
	sta $2006
	lda #0
	sta $2006
	sta j ; reset collision write pointer
	ldx #0
	:
		txa
		pha
		jsr room_unpack_32
		jsr room_load_nmt0
		pla
		tax
		inx
		cpx #22
		bcc :-
	; 22 rows nametable 1
	lda #$24
	sta $2006
	lda #0
	sta $2006
	sta j ; collision write pointer
	ldx #0
	:
		txa
		pha
		jsr room_unpack_32
		jsr room_load_nmt1
		pla
		tax
		inx
		cpx #22
		bcc :-
	; attributes
	jsr room_unpack_32
	lda #$23
	sta $2006
	lda #$C0
	sta $2006
	ldx #0 ; att_mirror position
	jsr room_load_att
	jsr room_unpack_32
	ldx #32
	jsr room_load_att
	jsr room_unpack_32
	lda #$27
	sta $2006
	lda #$C0
	sta $2006
	ldx #64
	jsr room_load_att
	jsr room_unpack_32
	ldx #96
	jsr room_load_att
	; room data is now unpacked and loaded
	rts

; loads a 32 byte line into the nametable 0 and collision table
;   reads j as index to collision table, clobbers i
room_load_nmt0:
	ldx j ; X is index to collision table
	ldy #0
	:
		lda #0
		sta i ; i will store collision
		.repeat 4
			lda nmi_update, Y
			sta $2007
			rol ; high bit of nametable is collision bit
			ror i
			iny
		.endrepeat
		lda i
		lsr
		lsr
		lsr
		lsr
		ora collision, X
		sta collision, X
		inx
		cpy #32
		bne :-
	stx j ; collision index
	rts

; complement of room_load_nmt1 but for right-side collision table
; nmt0 collision uses bits 0-3 for collision
; nmt1 will use bits 7-4 (backwards)
; (must load nmt0 first, nmt1 will OR with table, not remove it)
room_load_nmt1:
	ldx j ; X is index to collision table
	ldy #0
	:
		lda #0
		sta i ; i will store collision
		.repeat 4
			lda nmi_update, Y
			sta $2007
			rol ; high bit of nametable is collision bit
			rol i
			iny
		.endrepeat
		asl i
		asl i
		asl i
		asl i
		lda collision, X
		ora i
		sta collision, X
		inx
		cpy #32
		bne :-
	stx j ; collision index
	rts

; loads a 32 byte line into the attribute table
; X is index to att_mirror
room_load_att:
	ldy #0
	:
		lda nmi_update, Y
		sta $2007
		sta att_mirror, X
		inx
		iny
		cpy #32
		bne :-
	rts

; unpacks 32 bytes from ROOM data into nmi_update for temporary storage
;   data is RLE compressed, a value of 255 means RLE followed by run length, then value to use
;   stores run in k
;   stores value in l
room_unpack_32:
	ldx #0 ; storage index for nmi_update
@rle_loop:
	ldy k
	beq @no_run
		; currently unpacking run
		lda l
	@run_loop:
		sta nmi_update, X
		inx
		cpx #32
		bcc :+ ; stop run for now, resume next unpack
			sty k
			dec k
			rts
		:
		dey
		bne @run_loop
		sty k
	@no_run:
		; Y = 0
		lda (temp_ptr0), Y
		cmp #255
		bne @single_byte
			iny
			lda (temp_ptr0), Y
			sta k
			iny
			lda (temp_ptr0), Y
			sta l
			; temp_ptr0 += 3
			clc
			lda temp_ptr0+0
			adc #3
			sta temp_ptr0+0
			lda temp_ptr0+1
			adc #0
			sta temp_ptr0+1
			; A = value, Y = run length
			lda l
			ldy k
			jmp @run_loop
		@single_byte:
		; just an uncompressed single byte
		inc temp_ptr0+0 ; increment data pointer, byte has been read
		bne :+
			inc temp_ptr0+1
		:
		sta nmi_update, X
		inx
		cpx #32
		bcs :+
			jmp @no_run
		:
	rts

room_partial:
	jmp room_unpack_32

room_partial_begin:
	lda #BANK
	sta i ; return bank in i
	lda #0
	sta k ; RLE run
	sta l ; RLE value
	rts

; end of file
