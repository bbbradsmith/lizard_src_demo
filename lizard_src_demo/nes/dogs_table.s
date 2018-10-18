; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; dogs_table
;
; included multiple times in dogs_tables.s
; with different definitions of TABLE_ENTRY
; to build jump tables for dog subroutines
;
; define DOG_TABLE0/1/2 to include each bank's table entries

.if DOG_TABLE0 <> 0
TABLE_ENTRY none
TABLE_ENTRY door
TABLE_ENTRY pass
TABLE_ENTRY pass_x
TABLE_ENTRY pass_y
TABLE_ENTRY password_door
TABLE_ENTRY lizard_empty_left
TABLE_ENTRY lizard_empty_right
TABLE_ENTRY lizard_dismounter
TABLE_ENTRY splasher
TABLE_ENTRY disco
TABLE_ENTRY water_palette
TABLE_ENTRY grate
TABLE_ENTRY grate90
TABLE_ENTRY water_flow
TABLE_ENTRY rainbow_palette
TABLE_ENTRY pump
TABLE_ENTRY secret_steam
TABLE_ENTRY ceiling_free
TABLE_ENTRY block_column
TABLE_ENTRY save_stone
TABLE_ENTRY coin
TABLE_ENTRY monolith
TABLE_ENTRY iceblock
TABLE_ENTRY vator
TABLE_ENTRY noise
TABLE_ENTRY snow
TABLE_ENTRY rain
TABLE_ENTRY rain_boss
TABLE_ENTRY drip
TABLE_ENTRY hold_screen
TABLE_ENTRY boss_door
TABLE_ENTRY boss_door_rain
TABLE_ENTRY boss_door_exit
TABLE_ENTRY boss_door_exeunt
TABLE_ENTRY boss_rush
TABLE_ENTRY other
TABLE_ENTRY ending
TABLE_ENTRY river_exit
TABLE_ENTRY bones
TABLE_ENTRY easy
TABLE_ENTRY sprite0
TABLE_ENTRY sprite2
TABLE_ENTRY hintd
TABLE_ENTRY hintu
TABLE_ENTRY hintl
TABLE_ENTRY hintr
TABLE_ENTRY hint_penguin
TABLE_ENTRY bird
TABLE_ENTRY frog
TABLE_ENTRY grog
TABLE_ENTRY panda
TABLE_ENTRY goat
TABLE_ENTRY dog
TABLE_ENTRY wolf
TABLE_ENTRY owl
TABLE_ENTRY armadillo
TABLE_ENTRY beetle
TABLE_ENTRY skeetle
TABLE_ENTRY seeker_fish
TABLE_ENTRY manowar
TABLE_ENTRY snail
TABLE_ENTRY snapper
TABLE_ENTRY voidball
TABLE_ENTRY ballsnake
TABLE_ENTRY medusa
TABLE_ENTRY penguin
TABLE_ENTRY mage
TABLE_ENTRY mage_ball
TABLE_ENTRY ghost
TABLE_ENTRY piggy
TABLE_ENTRY panda_fire
TABLE_ENTRY goat_fire
TABLE_ENTRY dog_fire
TABLE_ENTRY owl_fire
TABLE_ENTRY medusa_fire
TABLE_ENTRY arrow_left
TABLE_ENTRY arrow_right
TABLE_ENTRY saw
TABLE_ENTRY steam
TABLE_ENTRY sparkd
TABLE_ENTRY sparku
TABLE_ENTRY sparkl
TABLE_ENTRY sparkr
TABLE_ENTRY frob_fly
.endif

.if DOG_TABLE1 <> 0
TABLE_ENTRY password
TABLE_ENTRY lava_palette
TABLE_ENTRY water_split
TABLE_ENTRY block
TABLE_ENTRY block_on
TABLE_ENTRY block_off
TABLE_ENTRY drawbridge
TABLE_ENTRY rope
TABLE_ENTRY boss_flame
TABLE_ENTRY river
TABLE_ENTRY river_enter
TABLE_ENTRY sprite1
TABLE_ENTRY beyond_star
TABLE_ENTRY beyond_end
TABLE_ENTRY other_end_left
TABLE_ENTRY other_end_right
TABLE_ENTRY particle
TABLE_ENTRY info
TABLE_ENTRY diagnostic
TABLE_ENTRY metrics
TABLE_ENTRY super_moose
TABLE_ENTRY brad_dungeon
TABLE_ENTRY brad
TABLE_ENTRY heep_head
TABLE_ENTRY heep
TABLE_ENTRY heep_tail
TABLE_ENTRY lava_left
TABLE_ENTRY lava_right
TABLE_ENTRY lava_left_wide
TABLE_ENTRY lava_right_wide
TABLE_ENTRY lava_left_wider
TABLE_ENTRY lava_right_wider
TABLE_ENTRY lava_down
TABLE_ENTRY lava_poop
TABLE_ENTRY lava_mouth
TABLE_ENTRY bosstopus
TABLE_ENTRY bosstopus_egg
TABLE_ENTRY cat
TABLE_ENTRY cat_smile
TABLE_ENTRY cat_sparkle
TABLE_ENTRY cat_star
TABLE_ENTRY raccoon
TABLE_ENTRY raccoon_launcher
TABLE_ENTRY raccoon_lavaball
TABLE_ENTRY raccoon_valve
TABLE_ENTRY frob
TABLE_ENTRY frob_hand_left
TABLE_ENTRY frob_hand_right
TABLE_ENTRY frob_zap
TABLE_ENTRY frob_tongue
TABLE_ENTRY frob_block
TABLE_ENTRY frob_platform
TABLE_ENTRY queen
TABLE_ENTRY hare
TABLE_ENTRY harecicle
TABLE_ENTRY hareburn
TABLE_ENTRY rock
TABLE_ENTRY log
TABLE_ENTRY duck
TABLE_ENTRY ramp
TABLE_ENTRY river_seeker
TABLE_ENTRY barrel
TABLE_ENTRY wave
TABLE_ENTRY snek_loop
TABLE_ENTRY snek_head
TABLE_ENTRY snek_tail
TABLE_ENTRY river_loop
TABLE_ENTRY watt
TABLE_ENTRY waterfall
.endif

.if DOG_TABLE2 <> 0
TABLE_ENTRY tip
TABLE_ENTRY wiquence
TABLE_ENTRY witch
TABLE_ENTRY book
.endif

; end of file

