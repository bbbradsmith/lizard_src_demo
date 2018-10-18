; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; text
;
; text data and retrieval routines
;

.include "ram.s"
.include "enums.s"

.export text_convert     ; converts a single character in A, clobbers X
.export text_confound    ; converts nmi_update, clobbers A, Y
.export text_load        ; loads first line of text into nmi_update (start + continue), clobbers A,X,Y,text_ptr,temp_ptr0
.export text_start       ; sets up text, but does not load, clobbers A,Y,text_ptr,temp_ptr0
.export text_continue    ; loads subsequent line of text into nmi_update, clobbers A,X,Y,text_ptr,temp_ptr0

.export text_data_location ; 6 byte table at $8000:
;.addr text_table_low
;.addr text_table_high
;.addr text_glyphs
;
; this is intentionally a redirection table so that translation patches can be a single isolated block

; for custom cartridge labels
.export text_INFO2
.export text_INFO3
.export text_INFO4

.segment "TEXT"
stat_text_data:
.include "../assets/export/text_default.s"
stat_text_data_end:

.segment "DATA"

ascii_table:
.byte "llllllllllllllll"
.byte "llllllllllllllll"
.byte "p[_llll_llllkn]\"
.byte "`abcdefghi^^lnlj"
.byte "@ABCDEFGHIJKLMNO"
.byte "PQRSTUVWXYZl\llp"
.byte "_ABCDEFGHIJKLMNO"
.byte "PQRSTUVWXYZlllnl"

special_table:
.byte $0D ; $80 シ
.byte $0E ; $81 ル
.byte $0F ; $82 エ
.byte $1D ; $83 ッ
.byte $1E ; $84 ト
.byte $1F ; $85 や
.byte $2A ; $86 か
.byte $2B ; $87 け
.byte $2C ; $88 ゛
.byte $2D ; $89 く
.byte $2E ; $8A め
.byte $2F ; $8B い
.byte $3B ; $8C を
.byte $3C ; $8D み
.byte $3D ; $8E て
.byte $3E ; $8F る
.byte $89 ; $90 。
.byte $8A ; $91 も
.byte $96 ; $92 う
.byte $9A ; $93 ん
.byte $A9 ; $94 ごく (1 of 2)
.byte $AA ; $95 ごく (2 of 2)
.byte $B9 ; $96 の
.byte $BA ; $97 じ
.byte $BB ; $98 ゆ
.byte $BC ; $99 だん (1 of 2)
.byte $BD ; $9A だん (2 of 2)
.byte $BE ; $9B は
.byte $BF ; $9C な
.byte $6C ; $9D
.byte $6C ; $9E
.byte $6C ; $9F
.byte $DD ; $A0 up
.byte $ED ; $A1 down
.byte $EE ; $A2 left
.byte $EF ; $A3 right
.byte $DE ; $A4 B
.byte $DF ; $A5 A
.byte $FD ; $A6 select (1 of 3)
.byte $FE ; $A7 select (2 of 3)
.byte $FF ; $A8 select (3 of 3)
.byte $CF ; $A9 option selector
.byte $27 ; $AA bullet point (ending)
.byte $0E ; $AB left (start)
.byte $0F ; $AC right (start)
.byte $6C ; $AD
.byte $6C ; $AE
.byte $6C ; $AF
.assert (*-special_table)=48,error,"special_table should have 48 entries"

.segment "CODE"

; converts a single character in A, clobbers X
text_convert:
	cmp #$80
	bcs :+
		tax
		lda ascii_table, X
		rts
	:
	cmp #$B0
	and #$7F ; carry not effected
	bcs :+
		tax
		lda special_table, X
	:
	rts

text_confound:
	; convert A parameter into alphabetic rotation (15 = rot13)
	clc
	adc #13
	and #15
	clc
	adc #1
	tay
	; preserve temporary s,u,v
	lda s
	pha
	lda u
	pha
	lda v
	pha
	sty s ; s = rotation seed
	; A-Z text bounds
	lda #'A'
	jsr text_convert
	sta u
	lda #'Z'
	jsr text_convert
	clc
	adc #1
	sta v
	; loop
	ldy #0
	@loop:
		lda nmi_update, Y
		cmp u ; A
		bcc @not_alpha
			cmp v ; Z+1
			bcc @alpha
		@not_alpha:
			; special glyphs
			cmp #$6D
			bne :+
				lda v
				sec
				sbc #2
				jmp @alpha
			:
			cmp #$40
			bne :+
				lda v
				sec
				sbc #1
				jmp @alpha
			:
			cmp #$74
			bcc @next
			cmp #$7D
			bcs @next
				sec
				sbc #$74
				clc
				adc u
				;jmp @alpha
			;
		@alpha:
			clc
			adc s
			:
				cmp v
				bcc :+
					sec
					sbc v
					clc
					adc u ; -(Z+1-A)
				jmp :-
			:
			sta nmi_update, Y
		@next:
		iny
		cpy #32
		bcc @loop
	; restore temporary s,u,v
	pla
	sta v
	pla
	sta u
	pla
	sta s
	rts

text_load:
	jsr text_start
	jmp text_continue

text_start:
	tay
	; text_table_low
	lda text_data_location+0
	sta temp_ptr0+0
	lda text_data_location+1 
	sta temp_ptr0+1
	lda (temp_ptr0), Y
	sta text_ptr+0
	; text_table_high
	lda text_data_location+2
	sta temp_ptr0+0
	lda text_data_location+3 
	sta temp_ptr0+1
	lda (temp_ptr0), Y
	sta text_ptr+1
	; finish
	lda #1
	sta text_more
	rts

text_continue:
	lda #0
	tax
	tay
	sta text_more
	lda text_ptr +0
	sta temp_ptr0+0
	lda text_ptr +1
	sta temp_ptr0+1
	@read_loop:
		lda (temp_ptr0), Y
		beq @read_end
		iny
		cmp #'$' ; newline
		bne :+
			lda #1
			sta text_more
			jmp @read_end
		:
		sta nmi_update, X
		inx
		cpx #32
		bcc @read_loop
	@read_end:
	; update text pointer
	tya
	clc
	adc temp_ptr0+0
	sta text_ptr +0
	lda #0
	adc temp_ptr0+1
	sta text_ptr +1
	; fill with spaces
	cpx #32
	bcs @space_end
	lda #' '
	@space_loop:
		sta nmi_update, X
		inx
		cpx #32
		bcc @space_loop
	@space_end:
	; convert ASCII to font set
	ldy #0
	:
		lda nmi_update, Y
		jsr text_convert
		sta nmi_update, Y
		iny
		cpy #32
		bcc :-
	rts

; end of file
