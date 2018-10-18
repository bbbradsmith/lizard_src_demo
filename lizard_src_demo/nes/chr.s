; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; room
;
; common chr loader, used in all chr data banks
;

.include "ram.s"
.import bank_call
.import bank_return
.importzp BANK

.import data_chr_low
.import data_chr_high

.include "../assets/export/names.s"

CHR_PER_BANK = 31
LAST_BANK = $A

.segment "CODE"

; X = CHR to load

.export chr_load
chr_load:
	; if not in this bank, go to next
	cpx #CHR_PER_BANK
	bcc @this_bank
		txa
		sec
		sbc #CHR_PER_BANK
		tax
		lda #(BANK-1)
		cmp #LAST_BANK
		bcc :+
			jsr bank_call
			rts
		:
		brk ; out of banks!
	@this_bank:
	; just an uncompressed data copy
	lda data_chr_low, X
	sta temp_ptr1+0
	lda data_chr_high, X
	sta temp_ptr1+1
	ldx #0
	ldy #0
	:
		:
			lda (temp_ptr1), Y
			sta $2007
			iny
			bne :-
		inc temp_ptr1+1
		inx
		cpx #4
		bcc :--
	rts
