@echo off
pushd "%~dp0"
mkdir .\build
pushd .\build
cl /I ..\extern\SDL2-2.24.1\include\ ^
   /I ..\extern\SDL2_mixer-2.6.3\include\ ^
   /I ..\extern\imgui\ ^
   /Zi ..\src\main.cpp ^
   ..\extern\imgui\imgui.cpp ^
   ..\extern\imgui\imgui_impl_sdlrenderer.cpp ^
   ..\extern\imgui\imgui_impl_sdl2.cpp ^
   ..\extern\imgui\imgui_tables.cpp ^
   ..\extern\imgui\imgui_widgets.cpp ^
   ..\extern\imgui\imgui_draw.cpp ^
/link ^
   /DEBUG:FULL ^
   /LIBPATH:..\extern\SDL2-2.24.1\lib\x64\ ^
   /LIBPATH:..\extern\SDL2_mixer-2.6.3\lib\x64\ ^
   /SUBSYSTEM:WINDOWS ^
   shell32.lib SDL2.lib SDL2main.lib SDL2_mixer.lib
xcopy /Y ..\extern\SDL2-2.24.1\lib\x64\SDL2.dll .
xcopy /Y ..\extern\SDL2_mixer-2.6.3\lib\x64\SDL2_mixer.dll .
rem build tools
rem cl /Zi ..\tools\hexToText.cpp
popd
popd
