@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
set path=j:\source;%path%

pushd ..\build
devenv win32_mariposa.exe
popd