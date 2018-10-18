; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; common_ef
;
; code/data common to banks E and F (dogs0, dogs2, lizard, main)
;

.feature force_range
.macpack longbranch

.include "ram.s"
.include "enums.s"
.include "../assets/export/names.s"

.export password_read
.export password_build
.export checkpoint
.export do_scroll ; clobbers A

.include "../assets/export/checkpoints.s"

.segment "CODE"

password_scramble:
	lda password+0
	eor #$2
	sta password+0
	lda password+1
	eor #$1
	sta password+1
	lda password+3
	eor #$6
	sta password+3
	lda password+4
	eor #$5
	sta password+4
	rts

; return: i = room, k = room high
;         j = lizard
;         A/Z: 0 = failed, 1 = good
password_read:
	; check for out of range values
	ldx #0
	@range_loop:
		lda password, X
		cmp #8
		bcc :+
			jmp @bad_password
		:
		inx
		cpx #5
		bcc @range_loop
	; unscramble
	jsr password_scramble
	; check parity
	lda password+4
	eor password+3
	eor password+1
	eor password+0
	eor password+2
	and #%11111110
	bne @bad_password
	; unpack room
	lda #0
	sta i
	lda password+4
	ror
	ror i
	lda password+3
	ror
	ror i
	lda password+1
	ror
	ror i
	lda password+0
	ror
	ror i
	lda password+4
	ror
	ror
	ror i
	lda password+3
	ror
	ror
	ror i
	lda password+1
	ror
	ror
	ror i
	lda password+0
	ror
	ror
	ror i
	lda password+2
	and #%00000001
	sta k
	; unpack lizard
	lda #0
	sta j
	lda password+4
	ror
	ror
	ror
	ror j
	lda password+3
	ror
	ror
	ror
	ror j
	lda password+1
	ror
	ror
	ror
	ror j
	lda password+0
	ror
	ror
	ror
	ror j
	clc
	ror j
	ror j
	ror j
	ror j
	; rescramble
	jsr password_scramble
	; check ranges
	lda i
	cmp #<DATA_room_COUNT
	lda k
	sbc #>DATA_room_COUNT
	bcs @bad_password_scrambled
	lda j
	cmp #LIZARD_OF_COUNT
	bcs @bad_password_scrambled
	; success
	lda #1
	rts
@bad_password:
	; rescramble
	jsr password_scramble
@bad_password_scrambled:
	lda #0
	rts

password_build:
	; wipe the password
	lda #0
	sta password+0
	sta password+1
	sta password+2
	sta password+3
	sta password+4
	; password[4] = lizard 0, room 4, room 0
	lda current_lizard
	ror
	rol password+4
	lda current_room+0
	ror
	ror
	ror
	ror
	ror
	rol password+4
	lda current_room+0
	ror
	rol password+4
	; password[3] = lizard 1, room 5, room 1
	lda current_lizard
	ror
	ror
	rol password+3
	lda current_room+0
	ror
	ror
	ror
	ror
	ror
	ror
	rol password+3
	lda current_room+0
	ror
	ror
	rol password+3
	; password[1] = lizard 2, room 6, room 2
	lda current_lizard
	ror
	ror
	ror
	rol password+1
	lda current_room+0
	ror
	ror
	ror
	ror
	ror
	ror
	ror
	rol password+1
	lda current_room+0
	ror
	ror
	ror
	rol password+1
	; password[0] = lizard 3, room 7, room 3
	lda current_lizard
	ror
	ror
	ror
	ror
	rol password+0
	lda current_room+0
	ror
	ror
	ror
	ror
	ror
	ror
	ror
	ror
	rol password+0
	lda current_room+0
	ror
	ror
	ror
	ror
	rol password+0
	; password[2] = parity, parity, current_room high
	lda password+4
	eor password+3
	eor password+1
	eor password+0
	lsr
	sta password+2
	lda current_room+1
	ror
	rol password+2
	; scramble
	jsr password_scramble
	; done
	rts

checkpoint:
	; k:i = room
	ldx #0
	:
		lda data_checkpoints_low, X
		cmp i
		bne @not_good
		lda data_checkpoints_high, X
		cmp k
		beq @good
	@not_good:
		inx
		cpx #DATA_checkpoints_COUNT
		bcc :-
	@bad:
		lda #0
		rts
	@good:
		lda #1
		rts
	;

do_scroll:
	lda room_scrolling
	bne @scrolling
@fixed:
	lda #0
	sta lizard_xh ; wrap
	sta scroll_x+0
	sta scroll_x+1
	rts
@scrolling:
	; wrap to 1 bit of high position
	lda #1
	and lizard_xh
	sta lizard_xh
	; if lizard_px <= 128
	lda lizard_x+0
	sec
	sbc #128
	sta scroll_x+0
	lda lizard_xh
	sbc #0
	sta scroll_x+1
	bcs :+
		; lizard_px <= 128
		lda #0
		sta scroll_x+0
		sta scroll_x+1
		rts
	:
	lda lizard_x+0
	sec
	sbc #<384
	lda lizard_xh
	sbc #>384
	bcc :+
		; lizard_px >= 384
		lda #<256
		sta scroll_x+0
		lda #>256
		sta scroll_x+1
		;rts
	:
	; scroll_x = lizard_px - 128
	rts

