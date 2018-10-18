; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; bank F
;
; main game bank
;
; for bank calls:
;   X = 255 -> build_frob_row_banked (s = frob state, r = row 0-7)
;   X = 254 -> sprite2_add_banked
;   X = 253 -> sprite2_add_flipped_banked
;   X = 252 -> text_load (t = text)
;   X = 251 -> circle108 (t = input, X = return)
;   X = 250 -> dog_draw
;   X = 249 -> dog_tick
;   X = 248 -> dog_init
;   X = 247 -> hair_6e (in/out: tb)
;

.include "ram.s"
.import bank_call
.import bank_return
.import getTVSystem
.import main
.import build_frob_row_banked
.import sprite2_add_banked_
.import sprite2_add_flipped_banked_
.import text_load
.import circle108
.import dog_draw
.import dog_tick
.import dog_init
.import hair_6e

.exportzp BANK
BANK = $F

.segment "SPRITE"

stat_sprite2_data:
.include "../assets/export/sprite2.s"
stat_sprite2_data_end:

data_sprite_tiles      = data_sprite2_tiles
data_sprite_low        = data_sprite2_low
data_sprite_high       = data_sprite2_high
data_sprite_tile_count = data_sprite2_tile_count
data_sprite_vpal       = data_sprite2_vpal
.export data_sprite_tiles, data_sprite_low, data_sprite_high, data_sprite_tile_count, data_sprite_vpal

.segment "CODE"

; nmi_handler is in main.s
; irq_handler is in main.s

.export bank_reset
bank_reset:
	; reset
	sei       ; mask interrupts
	lda #0
	sta $2000 ; disable NMI
	sta $2001 ; disable rendering
	sta $4010 ; disable DMC IRQ
	sta $4015 ; disable APU sound
	lda #$40
	sta $4017 ; disable APU IRQ
	cld       ; disable decimal mode
	ldx #$FF
	txs       ; initialize stack
	; save off password and random seed before wiping RAM
	lda seed+1
	pha
	lda seed+0
	pha
	lda password+0
	pha
	lda password+1
	pha
	lda password+2
	pha
	lda password+3
	pha
	lda password+4
	pha
	; wait for first vblank
	bit $2002
	:
		bit $2002
		bpl :-
	; clear RAM
	lda #0
	ldx #0
	:
		sta $0000,X
		sta $0200,X
		sta $0300,X
		sta $0400,X
		sta $0500,X
		sta $0600,X
		sta $0700,X
		inx
		bne :-
	; wait for second vblank
	:
		bit $2002
		bpl :-
	; restore password and random seed
	pla
	sta password+4
	pla
	sta password+3
	pla
	sta password+2
	pla
	sta password+1
	pla
	sta password+0
	pla
	sta seed+0
	pla
	sta seed+1
	ora seed+0
	bne :+
		lda #1
		sta seed+0
	:
	; now clear the stack area (password and seed have been preserved)
	lda #0
	tax
	:
		sta $100, X
		inx
		bne :-
	; put OAM offscreen, just in case
	;ldx #0
	lda #$FF
	:
		sta oam, X
		inx
		bne :-
	; music_init
	lda #$C ; MUSIC
	ldx #1
	jsr bank_call
	; detect for PAL system
	lda #1
	sta player_pause ; pause music (keeps music in NMI from interfering with test)
	lda #0
	sta nmi_ready    ; NMI will not upload data
	lda $2002
	lda #%10000000
	sta $2000        ; enable NMI for test
	jsr getTVSystem
	sta player_pal   ; save result of test
	lda #0
	sta $2000        ; disable NMI
	sta player_pause ; unpause music
	; begin game
	jmp main

.export bank_entry
bank_entry:
	cpx #255
	bne :+
		jsr build_frob_row_banked
		jmp bank_return
	:
	cpx #254
	bne :+
		jsr sprite2_add_banked_
		jmp bank_return
	:
	cpx #253
	bne :+
		jsr sprite2_add_flipped_banked_
		jmp bank_return
	:
	cpx #252
	bne :+
		lda t
		jsr text_load
		jmp bank_return
	:
	cpx #251
	bne :+
		ldx t
		lda circle108, X
		tax
		jmp bank_return
	:
	cpx #250
	bne :+
		jsr dog_draw
		jmp bank_return
	:
	cpx #249
	bne :+
		jsr dog_tick
		jmp bank_return
	:
	cpx #248
	bne :+
		jsr dog_init
		jmp bank_return
	:
	cpx #247
	bne :+
		ldx tb
		lda hair_6e + 0, X
		and hair_6e + 8, X
		sta tb
		jmp bank_return
	:
	; should not happen
	brk

; end of file
