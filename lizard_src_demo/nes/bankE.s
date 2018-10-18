; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; bank E
;
; dog updates (all at once)
;
; dogs
;   X = 255 dogs_draw
;   X = 254 dogs_tick
;   X = 253 dogs_init
;   X = 252 circle4 lookup (in: tb = index, out; tb = value, X = index)
;   X = 251 hair_color lookup (in/out: tb)

.import bank_call
.import bank_return
.import bank_nmi

.import dogs_draw
.import dogs_tick
.import dogs_init
.import circle4
.import hair_color
.importzp tb

.exportzp BANK
BANK = $E

.segment "SPRITE"

stat_sprite0_data:
.include "../assets/export/sprite0.s"
stat_sprite0_data_end:

data_sprite_tiles      = data_sprite0_tiles
data_sprite_low        = data_sprite0_low
data_sprite_high       = data_sprite0_high
data_sprite_tile_count = data_sprite0_tile_count
data_sprite_vpal       = data_sprite0_vpal
.export data_sprite_tiles, data_sprite_low, data_sprite_high, data_sprite_tile_count, data_sprite_vpal

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
	bne :+
		jsr dogs_draw
		jmp bank_return
	:
	cpx #254
	bne :+
		jsr dogs_tick
		jmp bank_return
	:
	cpx #253
	bne :+
		jsr dogs_init
		jmp bank_return
	:
	cpx #252
	bne :+
		ldx tb
		lda circle4, X
		sta tb
		jmp bank_return
	:
	cpx #251
	bne :+
		ldx tb
		lda hair_color, X
		sta tb
		jmp bank_return
	:
	brk ; shouldn't happen

; end of file
