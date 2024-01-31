import Omen
from sys import argv

dbg = not("rls" in argv)
simd = "simd" in argv
plat = "win"
backend = None
if "lin" in argv: plat = "lin"
if "llvm" in argv: backend = "llvm"
import platform
if platform.system() == "Linux": plat = "lin"

Omen.setBuildDir(plat, dbg)

if dbg and plat=="win":
    extra = "/std:c++14 /arch:AVX /EHsc"
    Omen.build("src/main.cc", "loki", "cl", extraSwitches=extra)
    if backend == "llvm":
        dir = Omen.getBuildDir()
        Omen.runCmd("cl /nologo /Z7 /std:c++14 -c /D WIN=true src/backend/llvmBackend.cc /Fo:"+dir+"llvmBackend.obj")
        Omen.runCmd("link /NOLOGO /DLL /DEBUG /PDB:"+dir+"llvmBackend.pdb "+dir+"llvmBackend.obj /OUT:"+dir+"llvmBackend.dll")
else:
    Omen.build("src/main.cc", "loki", "clang++", extraSwitches="-fuse-ld=mold")
    if backend == "llvm":
        Omen.build("src/backend/llvmBackend.cc", "llvmBackend", "clang++", intermediateOnly=True, extraSwitches="-fPIC -shared -fuse-ld=mold")
