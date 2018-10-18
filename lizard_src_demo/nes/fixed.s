; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; fixed
;
; vector table and bank entry trampoline
;

;
; Banking:
;   jsr bank_call to switch to bank A and jump to bank_entry with X parameter (Y clobbered)
;   jmp bank_return to return from a bank_call
;

;
; NMI:
;   controlled by nmi_ready, always ticks music playback, sets nmi_ready to NMI_NONE when finished
;
;   nmi_ready = NMI_NONE   tick music only, no PPU update
;   nmi_ready = NMI_READY  render on, upload OAM, palette
;   nmi_ready = NMI_ROW    render on, upload OAM, palette, 32 horizontal bytes
;   nmi_ready = NMI_DOUBLE render on, upload OAM, palette, 64 horizontal bytes
;   nmi_ready = NMI_WIDE   render on, upload OAM, palette, 64 horizontal bytes split across 2 nametables
;   nmi_ready = NMI_OFF    render off
;

.include "ram.s"

.segment "FIXED"

.import irq_handler
.import nmi_handler
.import bank_entry
.import bank_reset
.export bank_call
.export bank_return
.export bank_nmi
.importzp BANK

stat_fixed:

bus_conflict:
.repeat 16, I
	.byte I
.endrepeat
 
bank_call:
	tay
	lda #BANK
	pha
	tya
	sta bus_conflict, Y
	jmp bank_entry

bank_return:
	pla
	tay
	sta bus_conflict, Y
	rts

bank_nmi:
	lda #BANK
	pha
	lda #15
	tay
	sta bus_conflict, Y
	jsr nmi_handler
	pla
	tay
	sta bus_conflict, Y
	rts

vector_reset:
	; mask IRQs
	sei
	; disable NMI
	lda #0
	sta $2000
	; reset begins code at bank 7 bank_entry
	lda #15
	tay
	sta bus_conflict, Y
	jmp bank_reset

vector_irq:
	; IRQ is a crash handler, not intended to return
	; save A
	pha
	; disable NMI
	lda #%00001000
	sta $2000
	; save X, Y
	txa
	pha
	tya
	pha
	; save bank
	lda #BANK
	pha
	; go to IRQ handler in bank F
	lda #$F
	tay
	sta bus_conflict, Y
	jmp irq_handler

vector_nmi:
	pha
	txa
	pha
	tya
	pha
	jsr nmi_handler
	pla
	tay
	pla
	tax
	pla
	rti

.segment "VECTORS"
.word vector_nmi
.word vector_reset
.word vector_irq

; end of file
