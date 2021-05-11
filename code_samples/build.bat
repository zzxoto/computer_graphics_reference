@echo off
REM Usage: build.bat <project directory> e.g. build.bat camera_control

IF "%VCINSTALLDIR%"=="" (
  pushd "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC"
  call vcvarsall.bat x64
  popd
) ELSE (
  REM skip me
)

if not exist build mkdir build

set commonCompilerOpts=-Zi -nologo -FC -EHa

set includeRoot=%cd%\shared\include
set includes=-I%includeRoot%\

set libRoot=%cd%\shared\lib
set linkerOpts=-LIBPATH:%libRoot%\freeglut\x64 -LIBPATH:%libRoot%\glew\x64 OpenGL32.lib freeglut.lib glew32.lib 
set code=%cd%\%1

pushd build
cl %code%/*.cpp %commonCompilerOpts% %includes% -Femain -link %linkerOpts%
popd