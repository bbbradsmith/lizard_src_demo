set BASE_SDL=SDL2-2.0.7
del /s /q include
del /s /q lib
md include
md lib
md lib\debug
md lib\release
copy %BASE_SDL%\include\*.* include\
copy %BASE_SDL%\VisualC\Win32\Debug\SDL2.lib lib\debug\
copy %BASE_SDL%\VisualC\Win32\Release\SDL2.lib lib\release\
copy %BASE_SDL%\VisualC\Win32\Debug\SDL2main.lib lib\debug\
copy %BASE_SDL%\VisualC\Win32\Release\SDL2main.lib lib\release\
REM copy %BASE_SDL%\VisualC\SDL\Win32\Debug\vc120.pdb lib\debug\
copy %BASE_SDL%\VisualC\SDL\Win32\Release\vc120.pdb lib\release\
pause