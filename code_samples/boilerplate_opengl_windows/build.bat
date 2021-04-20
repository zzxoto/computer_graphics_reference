@echo off

IF "%VCINSTALLDIR%"=="" (
  pushd "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC"
  call vcvarsall.bat x64
  popd
) ELSE (
  REM skip me
)

if not exist build mkdir build

set commonCompilerOpts=-Zi -nologo -FC -EHa

set includeRoot=%cd%\..\shared\include
set includes=-I%includeRoot%\freeglut\ -I%includeRoot%\glew\ -I%includeRoot%\glm\ -I%includeRoot%\zzxoto\

set libRoot=%cd%\..\shared\lib
set linkerOpts=-LIBPATH:%libRoot%\freeglut\x64 -LIBPATH:%libRoot%\glew\x64 OpenGL32.lib freeglut.lib glew32.lib 
set code=%cd%

pushd build
cl %code%/*.cpp %commonCompilerOpts% %includes% -Femain -link %linkerOpts%
popd