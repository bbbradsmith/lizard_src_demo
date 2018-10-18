; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; bank B
;
; CHR loader
;   X = 1k CHR block number to write to $2007
; 255 = tick_wiquence
;

.include "ram.s"
.import bank_call
.import bank_return
.import bank_nmi
.import chr_load ; chr.s
.import tick_wiquence ; extra_b.s

.exportzp BANK
BANK = $B

.include "../assets/export/chr0.s"
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
	cpx #255
	beq :+
		jsr chr_load
		jmp bank_return
	:
		jsr tick_wiquence
		jmp bank_return
	;

; end of file
