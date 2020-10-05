@echo off

set CommonCompilerFlags=-MT -nologo -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -FC -Zi
set CommonLinkerFlags=-opt:ref user32.lib Gdi32.lib winmm.lib

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

REM x86 build
REM cl %CommonCompilerFlags%  ..\source\win32_mariposa.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM x64 build
cl %CommonCompilerFlags% -Fmmariposa.map ..\source\mariposa.cpp /link /DLL /EXPORT:GameUpdateAndRender /EXPORT:GetSoundSamples
cl %CommonCompilerFlags% -Fmwin32_mariposa.map ..\source\win32_mariposa.cpp /link %CommonLinkerFlags%
popd