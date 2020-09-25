@echo off

mkdir ..\build
pushd ..\build
cl -std:c++17 -FC -Zi ..\source\mariposa.cpp user32.lib Gdi32.lib
popd