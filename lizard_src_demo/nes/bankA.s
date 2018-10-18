; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; bank A
;
; CHR loader
;   X = 1k CHR block number to write to $2007
; 253 = tick_book
; 254 = tick_witch
; 255 = init_book
;

.include "ram.s"
.import bank_call
.import bank_return
.import bank_nmi
.import chr_load ; chr.s
.import tick_book ; extra_a.s
.import tick_witch ; extra_a.s
.import init_book ; extra_a.s

.exportzp BANK
BANK = $A

.include "../assets/export/chr1.s"
.export data_chr_low
.export data_chr_high

.segment "CODE"

.export nmi_handler
nmi_handler:
	jmp bank_nmi

.export irq_handler
.export bank_reset
irq_handler:
bank_reset:
	; this shouldn't happen
	jmp ($FFFC)

.export bank_entry
bank_entry:
	cpx #253
	bcs :+
		jsr chr_load
		jmp bank_return
	:
	bne :+
		jsr tick_book
		jmp bank_return
	:
	cpx #254
	bne :+
		jsr tick_witch
		jmp bank_return
	:
	; X = 255
		jsr init_book
		jmp bank_return
	;

; end of file
