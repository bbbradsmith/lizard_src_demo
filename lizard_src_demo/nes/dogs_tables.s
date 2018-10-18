; Lizard
; Copyright Brad Smith 2018
; http://lizardnes.com

;
; dogs_tables
; uses macro redefinition to build jump tables for dog subroutines
;
; define DOG_TABLE_LEN with the expected table size

stat_dog_jump_tables:

; init table

dog_init_table_low:
.define TABLE_ENTRY(xxx) .byte <(.ident(.concat ("dog_init_",.string(xxx)))-1)
.include "dogs_table.s"
.assert (*-dog_init_table_low)=DOG_TABLE_LEN, error, "dog_init_table_low incomplete"
.undef TABLE_ENTRY

dog_init_table_high:
.define TABLE_ENTRY(xxx) .byte >(.ident(.concat ("dog_init_",.string(xxx)))-1)
.include "dogs_table.s"
.assert (*-dog_init_table_high)=DOG_TABLE_LEN, error, "dog_init_table_high incomplete"
.undef TABLE_ENTRY

; tick table

dog_tick_table_low:
.define TABLE_ENTRY(xxx) .byte <(.ident(.concat ("dog_tick_",.string(xxx)))-1)
.include "dogs_table.s"
.assert (*-dog_tick_table_low)=DOG_TABLE_LEN, error, "dog_tick_table_low incomplete"
.undef TABLE_ENTRY

dog_tick_table_high:
.define TABLE_ENTRY(xxx) .byte >(.ident(.concat ("dog_tick_",.string(xxx)))-1)
.include "dogs_table.s"
.assert (*-dog_tick_table_high)=DOG_TABLE_LEN, error, "dog_tick_table_high incomplete"
.undef TABLE_ENTRY

; draw table

dog_draw_table_low:
.define TABLE_ENTRY(xxx) .byte <(.ident(.concat ("dog_draw_",.string(xxx)))-1)
.include "dogs_table.s"
.assert (*-dog_draw_table_low)=DOG_TABLE_LEN, error, "dog_draw_table_low incomplete"
.undef TABLE_ENTRY

dog_draw_table_high:
.define TABLE_ENTRY(xxx) .byte >(.ident(.concat ("dog_draw_",.string(xxx)))-1)
.include "dogs_table.s"
.assert (*-dog_draw_table_high)=DOG_TABLE_LEN, error, "dog_draw_table_high incomplete"
.undef TABLE_ENTRY

stat_dog_jump_tables_end:

; end of file
