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
set includes=-I%cd%\include\freeglut\ -I%cd%\include\glew\
set linkerOpts=-LIBPATH:%cd%\lib\freeglut\x64 -LIBPATH:%cd%\lib\glew\x64 OpenGL32.lib freeglut.lib glew32.lib 
set code=%cd%

pushd build
cl %code%/*.c %commonCompilerOpts% %includes% -Femain -link %linkerOpts%
popd
