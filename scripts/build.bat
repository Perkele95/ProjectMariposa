@echo off

set CommonCompilerFlags=-MT -nologo -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -FC -Z7
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib Gdi32.lib winmm.lib

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

REM x86 build
REM cl %CommonCompilerFlags%  ..\source\win32_mariposa.cpp /link -subsystem:windows,5.1 %CommonLinkerFlags%

REM x64 build
del *pdb > NUL 2> NUL
echo WAITING FOR PDB > lock.tmp
set DATENAME=%date:~-4,4%%date:~-10,2%%date:~-7,2%_%time:~0,2%%time:~3,2%%time:~6,2%
cl %CommonCompilerFlags% -Fmmariposa.map ..\source\mariposa.cpp -LD /link -incremental:no -opt:ref -PDB:%DATENAME%.pdb -EXPORT:GameUpdateAndRender -EXPORT:GetSoundSamples
del lock.tmp
cl %CommonCompilerFlags% -Fmwin32_mariposa.map ..\source\win32_mariposa.cpp /link %CommonLinkerFlags%
popd