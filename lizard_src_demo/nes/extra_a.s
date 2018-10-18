; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; extra_a (bank A)
;
; tick_witch
; init_bok
; tick_book

.feature force_range
.macpack longbranch

.include "ram.s"
.include "enums.s"
.include "../assets/export/names.s"
.include "../assets/export/text_set.s"
.include "dogs_inc.s"

.export tick_book
.export tick_witch
.export init_book

.import bank_call

.segment "CODE"

; DOG_WITCH

tick_witch:
	; NOT IN DEMO
	rts

; DOG_BOOK

init_book:
tick_book:
	; NOT IN DEMO
	rts

; end of file
