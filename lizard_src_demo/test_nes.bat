cd nes
call compile_nes.bat SKIP_PAUSE
@if ERRORLEVEL 1 goto failed
start temp\lizard_demo.nes
@goto end
:failed
pause
:end