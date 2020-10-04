@echo off

set CommonCompilerFlags=-MT -nologo -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -FC -Zi -Fmwin32_mariposa.map
set CommonLinkerFlags=-opt:ref user32.lib Gdi32.lib

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

REM x86 build
REM cl %CommonCompilerFlags%  ..\source\win32_mariposa.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM x64 build
cl %CommonCompilerFlags% ..\source\win32_mariposa.cpp /link %CommonLinkerFlags%
popd