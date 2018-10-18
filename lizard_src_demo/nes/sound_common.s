; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; sound_common.s
;
; common definitions for playing sound

.include "../assets/export/data_music_enums.s"

; xxx must be a constant, clobbers A
.macro PLAY_SOUND xxx
	lda #xxx
	sta player_next_sound + .ident(.sprintf("SOUND_MODE__%d",xxx))
.endmacro

; xxx must be a constant, Y=dog, clobbers A, X
.macro PLAY_SOUND_SCROLL xxx
	jsr dx_scroll_
	bne :+
		PLAY_SOUND xxx
	:
.endmacro

; end of file
