@echo off

mkdir ..\build
pushd ..\build
cl -FC -Zi ..\source\mariposa.cpp user32.lib Gdi32.lib
popd