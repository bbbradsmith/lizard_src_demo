; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; common_ef
;
; code/data common to banks E and F (dogs1, dogs2, lizard, main)
;

.feature force_range
.macpack longbranch

.include "ram.s"
.include "enums.s"
.include "../assets/export/names.s"

.export hex_to_ascii
.export ppu_nmi_write_row

.segment "DATA"

hex_to_ascii:
.byte $60, $61, $62, $63, $64, $65, $66, $67, $68, $69, $41, $42, $43, $44, $45, $46

.segment "CODE"

ppu_nmi_write_row:
	ldx #0
	:
		lda nmi_update, X
		sta $2007
		inx
		cpx #32
		bcc :-
	rts

; end of file
