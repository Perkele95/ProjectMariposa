@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
set PATH=j:\source;%PATH%

pushd ..\build
devenv project_mariposa.exe
popd