@echo off

if not exist bin\ (mkdir bin\)

::release
if "%1" == "rls" (
   if not exist bin\rls\ (mkdir bin\rls\)
   set CLANGCLFlags= -Ofast -D XE_PLAT_WIN=true -D CLANG_COMPILER=true -D XE_DBG=false -D XE_TEST=false
   clang++ %CLANGCLFlags% src\main.cc -o bin\rls\xenon.exe
)

::debug
if not exist bin\dbg\ (mkdir bin\dbg\)
set MSVCCLFlags= /nologo /std:c++14 /Zi /arch:AVX /EHsc /D XE_PLAT_WIN=true /D MSVC_COMPILER=true /D XE_DBG=true /D XE_TEST=true
cl %MSVCCLFlags% src/main.cc /Fo"bin\dbg\xenon.obj" /Fd"bin\dbg\xenon.pdb" /Fe"bin\dbg\xenon.exe"
