@echo off

pushd ..\source\Shaders
set compilerpath=..\..\Vulkan\Bin32

%compilerpath%\glslc.exe shader.vert -o vert.spv
%compilerpath%\glslc.exe shader.frag -o frag.spv

popd
pause