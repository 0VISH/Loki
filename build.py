from sys import argv
from subprocess import run
import os

def runCmd(cmd, lin = False):
    print("[CMD]", cmd)
    if(run(cmd, shell=(lin==True)).returncode != 0): quit()

isDbg = not("rls" in argv)
plat = "win"
if("lin" in argv): plat = "lin"

folder = "bin/"+plat+"/"
if(isDbg): folder += "dbg/"
else: folder += "rls/"

if not os.path.isdir(folder): os.makedirs(folder)

if plat == "win":
    if isDbg:
        runCmd("cl /nologo /std:c++14 /Zi /arch:AVX /EHsc /D XE_PLAT_WIN=true /D MSVC_COMPILER=true /D XE_DBG=true /D XE_TEST=true src/main.cc /Fo:"+folder+"xenon.obj /Fd:"+folder+"xenon.pdb /Fe:"+folder+"xenon.exe")
    else:
        runCmd("clang++ -Ofast -D XE_PLAT_WIN=true -D CLANG_COMPILER=true -D XE_DBG=false -D XE_TEST=false src\main.cc -o "+folder+"xenon.exe")
elif plat == "lin":
    print(folder)
    runCmd("clang++ -Ofast -D XE_PLAT_LIN=true -D CLANG_COMPILER=true -D XE_DBG=false -D XE_TEST=false src/main.cc -o "+folder+"xenon", True)
