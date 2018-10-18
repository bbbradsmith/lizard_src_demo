REM remove temporary stuff
md temp
del temp\*.o
del temp\*.bin
del temp\*.txt
del temp\*.nl
del temp\lizard_demo.nes

REM change version
REM @cd ..
REM @python version.py
REM @cd nes

REM build code
cc65\bin\ca65 ram.s         -g -D RAM_EXPORT -o temp\ram.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 header.s      -g -o temp\header.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 fixed.s       -g -o temp\fixed.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 room.s        -g -o temp\room.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 chr.s         -g -o temp\chr.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 music.s       -g -o temp\music.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 draw.s        -g -o temp\draw.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 collide.s     -g -o temp\collide.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 lizard.s      -g -o temp\lizard.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 common.s      -g -o temp\common.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 dogs_common.s -g -o temp\dogs_common.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 common_df.s   -g -o temp\common_df.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 common_ef.s   -g -o temp\common_ef.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 dogs0.s       -g -o temp\dogs0.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 dogs1.s       -g -o temp\dogs1.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 dogs2.s       -g -o temp\dogs2.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 extra_a.s     -g -o temp\extra_a.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 extra_b.s     -g -o temp\extra_b.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 text.s        -g -o temp\text.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 detect_pal.s  -g -o temp\detect_pal.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 main.s        -g -o temp\main.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 bank0.s       -g -o temp\bank0.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 bank1.s       -g -o temp\bank1.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 bank2.s       -g -o temp\bank2.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 bank3.s       -g -o temp\bank3.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 bank4.s        -g -o temp\bank4.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 bank5.s       -g -o temp\bank5.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 bank6.s       -g -o temp\bank6.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 bank7.s       -g -o temp\bank7.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 bank8.s       -g -o temp\bank8.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 bank9.s       -g -o temp\bank9.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 bankA.s       -g -o temp\bankA.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 bankB.s       -g -o temp\bankB.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 bankC.s       -g -o temp\bankC.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 bankD.s       -g -o temp\bankD.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 bankE.s       -g -o temp\bankE.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ca65 bankF.s       -g -o temp\bankF.o
@IF ERRORLEVEL 1 GOTO badbuild

REM link header and all banks
cc65\bin\ld65 -o temp\header.bin -C header.cfg -m temp\header.map.txt temp\header.o
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ld65 -o temp\0.bin -C bank32k.cfg -m temp\0.map.txt temp\ram.o temp\bank0.o temp\fixed.o temp\room.o -Ln temp\nes.0.labels.txt
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ld65 -o temp\1.bin -C bank32k.cfg -m temp\1.map.txt temp\ram.o temp\bank1.o temp\fixed.o temp\room.o -Ln temp\nes.1.labels.txt
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ld65 -o temp\2.bin -C bank32k.cfg -m temp\2.map.txt temp\ram.o temp\bank2.o temp\fixed.o temp\room.o -Ln temp\nes.2.labels.txt
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ld65 -o temp\3.bin -C bank32k.cfg -m temp\3.map.txt temp\ram.o temp\bank3.o temp\fixed.o temp\room.o -Ln temp\nes.3.labels.txt
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ld65 -o temp\4.bin -C bank32k.cfg -m temp\4.map.txt temp\ram.o temp\bank4.o temp\fixed.o temp\room.o -Ln temp\nes.4.labels.txt
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ld65 -o temp\5.bin -C bank32k.cfg -m temp\5.map.txt temp\ram.o temp\bank5.o temp\fixed.o temp\room.o -Ln temp\nes.5.labels.txt
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ld65 -o temp\6.bin -C bank32k.cfg -m temp\6.map.txt temp\ram.o temp\bank6.o temp\fixed.o temp\room.o -Ln temp\nes.6.labels.txt
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ld65 -o temp\7.bin -C bank32k.cfg -m temp\7.map.txt temp\ram.o temp\bank7.o temp\fixed.o temp\room.o -Ln temp\nes.7.labels.txt
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ld65 -o temp\8.bin -C bank32k.cfg -m temp\8.map.txt temp\ram.o temp\bank8.o temp\fixed.o temp\room.o -Ln temp\nes.8.labels.txt
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ld65 -o temp\9.bin -C bank32k.cfg -m temp\9.map.txt temp\ram.o temp\bank9.o temp\fixed.o temp\room.o -Ln temp\nes.9.labels.txt
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ld65 -o temp\A.bin -C bank32k.cfg -m temp\A.map.txt temp\ram.o temp\bankA.o temp\fixed.o temp\chr.o temp\extra_a.o -Ln temp\nes.A.labels.txt
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ld65 -o temp\B.bin -C bank32k.cfg -m temp\B.map.txt temp\ram.o temp\bankB.o temp\fixed.o temp\chr.o temp\extra_b.o -Ln temp\nes.B.labels.txt
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ld65 -o temp\C.bin -C bank32k.cfg -m temp\C.map.txt temp\ram.o temp\bankC.o temp\fixed.o temp\music.o -Ln temp\nes.C.labels.txt
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ld65 -o temp\D.bin -C bank32k.cfg -m temp\D.map.txt temp\ram.o temp\bankD.o temp\fixed.o temp\common.o temp\dogs_common.o temp\common_df.o temp\collide.o temp\draw.o temp\dogs1.o -Ln temp\nes.D.labels.txt
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ld65 -o temp\E.bin -C bank32k.cfg -m temp\E.map.txt temp\ram.o temp\bankE.o temp\fixed.o temp\common.o temp\dogs_common.o temp\common_ef.o temp\collide.o temp\draw.o temp\dogs0.o -Ln temp\nes.E.labels.txt
@IF ERRORLEVEL 1 GOTO badbuild
cc65\bin\ld65 -o temp\F.bin -C bank32k.cfg -m temp\F.map.txt temp\ram.o temp\bankF.o temp\fixed.o temp\common.o temp\common_df.o temp\common_ef.o temp\collide.o temp\detect_pal.o temp\draw.o temp\text.o temp\main.o temp\lizard.o temp\dogs2.o -Ln temp\nes.F.labels.txt
@IF ERRORLEVEL 1 GOTO badbuild

REM composite NES from banks
copy /b temp\0.bin+temp\1.bin+temp\2.bin+temp\3.bin temp\z03.bin
copy /b temp\4.bin+temp\5.bin+temp\6.bin+temp\7.bin temp\z47.bin
copy /b temp\8.bin+temp\9.bin+temp\A.bin+temp\B.bin temp\z8B.bin
copy /b temp\C.bin+temp\D.bin+temp\E.bin+temp\F.bin temp\zCF.bin
copy /b temp\z03.bin+temp\z47.bin+temp\z8B.bin+temp\zCF.bin temp\lizard.prg
copy /b temp\header.bin+temp\z03.bin+temp\z47.bin+temp\z8B.bin+temp\zCF.bin temp\lizard_demo.nes

REM generate debug symbols
python symbols.py

@echo.
@echo.
@echo Build complete and successful!
@IF NOT "%1"=="" GOTO endbuild
@pause
@GOTO endbuild

:badbuild
@echo.
@echo.
@echo Build error!
@IF NOT "%1"=="" GOTO endbuild
@pause
:endbuild