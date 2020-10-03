@echo off

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build
cl -MT -nologo -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -FC -Zi -Fmwin32_mariposa.map ..\source\win32_mariposa.cpp /link -opt:ref -subsystem:windows,5.1 user32.lib Gdi32.lib
popd