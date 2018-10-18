; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; bank C
;
; music
;   X=0 music_tick
;   X=1 music_init

.include "ram.s"
.import bank_call
.import bank_return
.import bank_nmi

.exportzp BANK
BANK = $C

.segment "CODE"

.import music_tick
.import music_init

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
	txa
	bne :+
		jsr music_tick
		jmp bank_return
	:
	jsr music_init
	jmp bank_return

; end of file

