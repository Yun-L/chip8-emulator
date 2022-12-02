@echo off
pushd "%~dp0"
mkdir .\build
pushd .\build
cl /Zi ..\src\main.cpp
popd
popd
