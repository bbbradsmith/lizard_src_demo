; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; global RAM reservations
;

; define RAM_EXPORT to compile this file as an object reserving RAM
; otherwise include this file to import all RAM labels

.macro RESZP label, size
	.ifdef RAM_EXPORT
		label: .res size
		.exportzp label
	.else
		.importzp label
	.endif
.endmacro

.macro RES label, size
	.ifdef RAM_EXPORT
		label: .res size
		.export label
	.else
		.import label
	.endif
.endmacro

;
; reservations
;

.segment "ZEROPAGE"

RESZP  i,                     1 ; temporary variables, do not use during NMI
RESZP  j,                     1
RESZP  k,                     1
RESZP  l,                     1
RESZP  m,                     1
RESZP  n,                     1
RESZP  o,                     1
RESZP  p,                     1
RESZP  q,                     1
RESZP  r,                     1
RESZP  s,                     1
RESZP  t,                     1
RESZP  u,                     1
RESZP  v,                     1
RESZP  w,                     1
RESZP  tb,                    1

RESZP  ih,                    1 ; high byte for i/j/k/l for 16 bit temporaries
RESZP  jh,                    1
RESZP  kh,                    1
RESZP  lh,                    1

RESZP  temp_ptr0,             2 ; temporary pointers, do not use during NMI
RESZP  temp_ptr1,             2
RESZP  temp_ptrn,             2 ; temporary pointer, only use during NMI

RESZP  nmi_count,             1
RESZP  nmi_lock,              1
RESZP  nmi_ready,             1
RESZP  nmi_next,              1 ; only use during dog tick (not draw) to select an NMI upload
RESZP  nmi_load,              2 ; little endian, unsigned
RESZP  scroll_x,              2 ; little endian, unsigned
RESZP  overlay_scroll_y,      1
RESZP  mode_temp,             1
RESZP  oam_pos,               1
RESZP  chr_cache,             8
RESZP  ending,                1
RESZP  continued,             1
RESZP  coin_saved,            1
RESZP  room_scrolling,        1
RESZP  sprite_center,         1

RESZP  easy,                  1
RESZP  game_pause,            1
RESZP  game_message,          1
RESZP  room_change,           1
RESZP  current_room,          2 ; little endian
RESZP  current_door,          1
RESZP  current_lizard,        1
RESZP  next_lizard,           1
RESZP  hold_x,                1
RESZP  hold_y,                1
RESZP  human0_pal,            1

RESZP  dog_add_select,        1
RESZP  dog_add,               1
RESZP  dog_now,               1
RESZP  att_or,                1

RESZP  water,                 1

RESZP  gamepad,               1

RESZP  seed,                  2 ; little endian
RESZP  password,              5

RESZP  smoke_x,               1
RESZP  smoke_xh,              1
RESZP  smoke_y,               1

RESZP  climb_assist_time,     1
RESZP  climb_assist,          1

; lizard

RESZP  lizard_xpad,           1 ; unused to mirror 32-bit PC implementation
RESZP  lizard_xh,             1 ; big endian high byte of 24-bit lizard_x
RESZP  lizard_x,              2 ; big endian, unsigned
RESZP  lizard_y,              2 ; big endian, unsigned
RESZP  lizard_z,              2 ; big endian, unsigned
RESZP  lizard_vx,             2 ; big endian, signed
RESZP  lizard_vy,             2 ; big endian, signed
RESZP  lizard_vz,             2 ; big endian, signed
RESZP  lizard_flow,           1
RESZP  lizard_face,           1
RESZP  lizard_fall,           1
RESZP  lizard_jump,           1
RESZP  lizard_skid,           1
RESZP  lizard_dismount,       1
RESZP  lizard_anim,           2 ; big endian, unsigned
RESZP  lizard_scuttle,        1
RESZP  lizard_dead,           1
RESZP  lizard_power,          1
RESZP  lizard_wet,            1
RESZP  lizard_big_head_mode,  1
RESZP  lizard_moving,         1

RESZP  lizard_hitbox_x0,      1
RESZP  lizard_hitbox_xh0,     1
RESZP  lizard_hitbox_y0,      1
RESZP  lizard_hitbox_x1,      1
RESZP  lizard_hitbox_xh1,     1
RESZP  lizard_hitbox_y1,      1

RESZP  lizard_touch_x0,       1
RESZP  lizard_touch_xh0,      1
RESZP  lizard_touch_y0,       1
RESZP  lizard_touch_x1,       1
RESZP  lizard_touch_xh1,      1
RESZP  lizard_touch_y1,       1

; audio

RESZP  player_pal,            1 ; 0 = NTSC, 1-5 = PAL (counts down for a doubling at 1)
RESZP  player_next_sound,     2
RESZP  player_next_music,     1
RESZP  player_pause,          1
RESZP  player_current_music,  1
RESZP  player_bg_noise_freq,  1
RESZP  player_bg_noise_vol,   1
RESZP  player_music_mask,     1

RESZP  player_speed,          1
RESZP  player_pattern_length, 1
RESZP  player_row,            1
RESZP  player_row_sub,        1
RESZP  player_order_frame,    1

RESZP  player_row_skip,       4
RESZP  player_macro_pos,      16
RESZP  player_note,           4
RESZP  player_vol,            4
RESZP  player_halt,           4
RESZP  player_pitch_low,      4
RESZP  player_pitch_high,     4

RESZP  player_sfx_on,         2
RESZP  player_sfx_pos,        2
RESZP  player_sfx_skip,       2

RESZP  player_vol_out,        4
RESZP  player_freq_out_low,   4
RESZP  player_freq_out_high,  4
RESZP  player_duty_out,       4
RESZP  player_apu_high,       4

RESZP  pointer_pattern_table, 2
RESZP  pointer_order,         2
RESZP  pointer_pattern,       8
RESZP  pointer_macro,         32
RESZP  pointer_sfx_pattern,   4

RESZP  player_rain_seed,      2

; reserve last two bytes for crash diagnostic pointer

RESZP  zp_padding,            2

.segment "STACK"
RES    nmi_update,            64
RES    scratch,               64

.segment "OAM"
RES    oam,                   256

.segment "RAM"
RES    collision,             256

RES    dog_data0,             16 ; 256 byte page aligned on purpose (for faster indexing)
RES    dog_data1,             16
RES    dog_data2,             16
RES    dog_data3,             16
RES    dog_data4,             16
RES    dog_data5,             16
RES    dog_data6,             16
RES    dog_data7,             16
RES    dog_data8,             16
RES    dog_data9,             16
RES    dog_data10,            16
RES    dog_data11,            16
RES    dog_data12,            16
RES    dog_data13,            16

RES    dog_param,             16
RES    dog_type,              16
RES    dog_y,                 16

RES    dog_xh,                16 ; high byte of dog_x
RES    dog_x,                 16
RES    blocker_xh0,           4 ; high byte of blocker_x0
RES    blocker_x0,            4
RES    blocker_xh1,           4 ; high byte of blocker_x1
RES    blocker_x1,            4
RES    door_xh,               8 ; high byte of door_x
RES    door_x,                8
RES    door_linkh,            8 ; high byte of door_link
RES    door_link,             8

RES    door_y,                8

RES    att_mirror,            128
RES    palette,               32
RES    shadow_palette,        32
RES    blocker_y0,            4
RES    blocker_y1,            4

RES    coin,                  16
RES    coin_save,             16
RES    flag,                  16
RES    flag_save,             16
RES    piggy_bank,            1
RES    piggy_bank_save,       1
RES    last_lizard,           1
RES    last_lizard_save,      1

RES    human1_pal,            1
RES    human0_hair,           1
RES    human1_hair,           1
RES    moose_text,            1
RES    moose_text_inc,        1
RES    heep_text,             1
RES    human1_set,            6
RES    human1_het,            6
RES    human1_next,           1

RES    decimal,               5

RES    metric_time_h,         1
RES    metric_time_m,         1
RES    metric_time_s,         1
RES    metric_time_f,         1
RES    metric_bones,          4 ; little endian
RES    metric_jumps,          4 ; little endian
RES    metric_continue,       4 ; little endian
RES    metric_cheater,        1

RES    diagnostic_room,       2
RES    bounced,               1
RES    frogs_fractioned,      1
RES    wave_draw_control,     1
RES    text_select,           1
RES    text_more,             1
RES    text_ptr,              2 ; little endian
RES    boss_talk,             1
RES    tip_index,             1
RES    tip_counter,           1
RES    tip_return_door,       1
RES    tip_return_room,       2 ; little endian
RES    boss_rush,             1
RES    end_book,              1
RES    end_verso,             1

; end of file
