call "%VS120COMNTOOLS%VsDevCmd.bat"
msbuild lizard.vcxproj /property:Configuration=Debug
@if ERRORLEVEL 1 goto failed
start Debug\lizard_demo.exe
@goto end
:failed
pause
:end