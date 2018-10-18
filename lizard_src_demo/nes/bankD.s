; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; bank D
;
; auxilliary code for dogs (individual updates)
;
; dogs
;   X = 255 dog_draw
;   X = 254 dog_tick
;   X = 253 dog_init
;
; banked code for extra_a/b use:
;   X = 252 lizard_overlap_no_stone (out: tb)
;   X = 251 prng (out: tb)
;   X = 250 lizard_overlap (out: tb)
;   X = 249 palette_load (in: tb = load index, t = slot)
;   X = 248 lizard_die
;   X = 247 dog_init_boss_door (in: tb = Y)
;   X = 246 flag_read (in/out: tb)

.import bank_call
.import bank_return
.import bank_nmi

.import dog_draw
.import dog_tick
.import dog_init

.import lizard_overlap_no_stone
.import prng
.import lizard_overlap
.import palette_load
.import lizard_die
.import dog_init_boss_door
.import flag_read
.importzp t
.importzp tb

.exportzp BANK
BANK = $D

.segment "SPRITE"

stat_sprite1_data:
.include "../assets/export/sprite1.s"
stat_sprite1_data_end:

data_sprite_tiles      = data_sprite1_tiles
data_sprite_low        = data_sprite1_low
data_sprite_high       = data_sprite1_high
data_sprite_tile_count = data_sprite1_tile_count
data_sprite_vpal       = data_sprite1_vpal
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
		jsr dog_draw
		jmp bank_return
	:
	cpx #254
	bne :+
		jsr dog_tick
		jmp bank_return
	:
	cpx #253
	bne :+
		jsr dog_init
		jmp bank_return
	:
	cpx #252
	bne :+
		jsr lizard_overlap_no_stone
		sta tb
		jmp bank_return
	:
	cpx #251
	bne :+
		jsr prng
		sta tb
		jmp bank_return
	:
	cpx #250
	bne :+
		jsr lizard_overlap
		sta tb
		jmp bank_return
	:
	cpx #249
	bne :+
		lda t
		ldx tb
		jsr palette_load
		jmp bank_return
	:
	cpx #248
	bne :+
		jsr lizard_die
		jmp bank_return
	:
	cpx #247
	bne :+
		ldy tb
		jsr dog_init_boss_door
		jmp bank_return
	:
	cpx #246
	bne :+
		lda tb
		jsr flag_read
		sta tb
		jmp bank_return
	:
	brk ; shouldn't happen

; end of file
