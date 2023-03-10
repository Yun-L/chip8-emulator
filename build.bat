@echo off
pushd "%~dp0"
mkdir .\build
pushd .\build
cl /I ..\extern\SDL2-2.24.1\include\ ^
   /Zi ..\src\main.cpp ^
/link ^
   /LIBPATH:..\extern\SDL2-2.24.1\lib\x64\ ^
   /SUBSYSTEM:WINDOWS ^
   shell32.lib SDL2.lib SDL2main.lib
xcopy /Y ..\extern\SDL2-2.24.1\lib\x64\SDL2.dll .
popd
popd
