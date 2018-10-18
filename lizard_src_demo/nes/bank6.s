; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; bank 6
;
; room loader
;   current_room = room to load
;   X = 0
;     load room
;     will automatically switch banks if room is not in this bank
;   X = 255
;     setup partial room load (bottom 8 rows, interleaved)
;     caller must preserve k, l, and temp_ptr0 while executing partial load
;   X = 254
;     load 32 bytes from partial room load
;

.import bank_call
.import bank_return
.import bank_nmi
.import room_load

.exportzp BANK
BANK = $6

.include "../assets/export/room6.s"
.export DATA_ROOM_BANK_START: absolute
.export DATA_ROOM_BANK_END: absolute
.export data_room_low
.export data_room_high

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
	jsr room_load
	jmp bank_return

; end of file
