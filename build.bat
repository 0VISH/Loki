@echo off

set MSVCCLFlags= /nologo /std:c++14 /Zi /arch:AVX

if not exist bin\ (mkdir bin\)
cl %MSVCCLFlags% src/main.cc /Fo"bin\xenon.obj" /Fd"bin\xenon.pdb" /Fe"bin\xenon.exe"
