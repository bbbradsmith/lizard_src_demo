; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; extra_b (bank B)
;
; wiquence tick
;

.macpack longbranch

.include "ram.s"
.include "enums.s"
.include "../assets/export/names.s"
.include "../assets/export/text_set.s"
.include "dogs_inc.s"

.export tick_wiquence

.import bank_call

.segment "CODE"

; DOG_WIQUENCE

tick_wiquence:
	; NOT IN DEMO
	rts

; end of file
