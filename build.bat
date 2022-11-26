@echo off
pushd "%~dp0"
mkdir .\build
pushd .\build
cl ..\src\main.cpp
rem cl /I ..\..\SDL2-2.24.1\include\ ^
rem    /I ..\..\SDL2_image-2.6.2\include\ ^
rem /link ^
rem    /LIBPATH:..\..\SDL2-2.24.1\lib\x64\ ^
rem    /LIBPATH:..\..\SDL2_image-2.6.2\lib\x64\ ^
rem    /SUBSYSTEM:WINDOWS ^
rem    shell32.lib SDL2.lib SDL2main.lib SDL2_image.lib
rem copy DLL's to executable directory
rem xcopy /Y ..\..\SDL2-2.24.1\lib\x64\SDL2.dll .
rem xcopy /Y ..\..\SDL2_image-2.6.2\lib\x64\SDL2_image.dll .
popd
popd
