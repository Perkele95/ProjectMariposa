@echo off

set CommonCompilerFlags=-MTd -nologo -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -FC -Z7 -I ..\Vulkan\Include -I ..\stb_image
set CommonLinkerFlags= -incremental:no -opt:ref user32.lib Gdi32.lib winmm.lib /LIBPATH:..\Vulkan\Lib vulkan-1.lib

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

REM x64 build
cl %CommonCompilerFlags% -Fmwin32_mariposa.map ..\source\project_mariposa.c /link /LIBPATH:..\Vulkan\Lib\vulkan-1.lib %CommonLinkerFlags%
popd