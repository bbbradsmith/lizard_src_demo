; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; common
;
; some common routines that need to be available in more than one bank
;

.feature force_range
.macpack longbranch

.export prng
.export prng2
.export prng3
.export prng4
.export prng8
.export lizard_die
.export inc_bones
.export dogs_cycle
.export empty_blocker
.export coin_read
.export coin_take
.export coin_return
.export coin_count
.export flag_read
.export flag_set
.export flag_clear

; common definitions

.include "ram.s"
.include "enums.s"
.include "sound_common.s"
.include "../assets/export/names.s"

dgd_BONES_INIT = dog_data0
dgd_BONES_FLIP = dog_data1

; from collide.s

.import collide_tile
.import collide_tile_left
.import collide_tile_right
.import collide_tile_up
.import collide_tile_down
.import collide_all
.import collide_all_left
.import collide_all_right
.import collide_all_up
.import collide_all_down

.import palette_load ; from draw.s

.segment "CODE"

stat_common:

; ====
; coin
; ====

coin_pos:
	pha
	and #7
	tax
	pla
	lsr
	lsr
	lsr
	tay
	rts

coin_bit:
	lda #1
	cpx #0
	beq :++
	:
		asl
		dex
		bne :-
	:
	rts

 ; clobbers A,X,Y returns coin in A
coin_read:
	jsr coin_pos
	lda coin, Y
coin_read_finish:
	cpx #0
	beq :++
	:
		ror
		dex
		bne :-
	:
	ror
	bcs :+
		lda #0
		rts
	:
		lda #1
		rts
	;

; clobbers A,X,Y
coin_take:
	jsr coin_pos
	jsr coin_bit
	ora coin, Y
	sta coin, Y
	lda #0
	sta coin_saved
	rts

; clobbers A,X,Y
coin_return:
	jsr coin_pos
	jsr coin_bit
	eor #%11111111
	and coin, Y
	sta coin, Y
	lda #0
	sta coin_saved
	rts

; clobbers A,X,Y,i,j returns count in A
coin_count:
	lda #0
	sta i ; i = index
	sta j ; j = count
	:
		; A = i
		jsr coin_read
		clc
		adc j
		sta j
		inc i
		lda i
		cmp #128
		bcc :-
	lda j
	rts

; clobbers A,X,Y returns flag in A
flag_read:
	jsr coin_pos
	lda flag, Y
	jmp coin_read_finish

; clobbers A,X,Y
flag_set:
	jsr coin_pos
	jsr coin_bit
	ora flag, Y
	sta flag, Y
	lda #0
	sta coin_saved
	rts

; clobbers A,X,Y
flag_clear:
	jsr coin_pos
	jsr coin_bit
	eor #%11111111
	and flag, Y
	sta flag, Y
	lda #0
	sta coin_saved
	rts

; ======
; random
; ======

prng:
	lda seed+0
	asl
	rol seed+1
	bcc :+
		eor #$D7
	:
	sta seed+0
	rts

prng8:
	jsr prng4
prng4:
	jsr prng
prng3:
	jsr prng
prng2:
	jsr prng
	jmp prng

; ======
; lizard
; ======

lizard_die:
	lda lizard_dead
	beq :+
		rts ; already dead
	:
	jsr inc_bones
	; kill lizard
	PLAY_SOUND SOUND_BONES
	lda #1
	sta lizard_dead
	lda #0
	sta lizard_power
	sta lizard_scuttle
	;sta lizard_big_head_mode
	lda #>(-800)
	sta lizard_vy+0
	lda #<(-800)
	sta lizard_vy+1
	lda #0
	sta lizard_vx+0
	jsr prng ; lizard.vx = prng() << 1
	asl
	sta lizard_vx+1
	rol lizard_vx+0
	jsr prng
	and #1
	beq :+
		; negate lizard.vx
		lda lizard_vx+0
		eor #$FF
		sta lizard_vx+0
		lda lizard_vx+1
		eor #$FF
		clc
		adc #1
		sta lizard_vx+1
		lda lizard_vx+0
		adc #0
		sta lizard_vx+0
	:
	rts

; ==========
; dogs stuff
; ==========

.segment "CODE"

inc_bones:
	; increment 32-bit bones counter
	inc metric_bones+0
	bne :+
	inc metric_bones+1
	bne :+
	inc metric_bones+2
	bne :+
	inc metric_bones+3
	:
	rts

; ===========
; common dogs
; ===========

.segment "DATA"
dog_add_prime: .byte 1, 5, 11, 15, 7, 3, 13, 9 ; relatively prime to 16 to cycle all dogs

.segment "CODE"
dogs_cycle:
	inc dog_add_select
	lda dog_add_select
	and #7
	sta dog_add_select
	tax
	lda dog_add_prime, X
	sta dog_add
	rts

empty_blocker:
	tya
	and #3
	tax
	lda #127
	sta blocker_xh0, X
	sta blocker_xh1, X
	lda #255
	sta blocker_x0, X
	sta blocker_x1, X
	sta blocker_y0, X
	sta blocker_y1, X
	rts

stat_common_end_of_file:
; end of file
