@echo off

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build
cl -DMP_INTERNAL=1 -DMP_PERFORMANCE=0 -FC -Zi ..\source\win32_mariposa.cpp user32.lib Gdi32.lib
popd