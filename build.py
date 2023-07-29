from sys import argv
from subprocess import run
import os

def runCmd(cmd, lin = False):
    print("[CMD]", cmd)
    if(run(cmd, shell=(lin==True)).returncode != 0): quit()

defines = {}
defines["DBG"] = not("rls" in argv)
defines["SIMD"] = "simd" in argv

plat = False
if("lin" in argv):
    plat = "lin"
    defines["PLAT_LIN"] = True
if("win" in argv) or (plat == False):
    plat = "win"
    defines["PLAT_WIN"] = True

folder = "bin/"+plat+"/"
if(defines["DBG"]): folder += "dbg/"
else: folder += "rls/"

if not os.path.isdir(folder): os.makedirs(folder)

def genDefinesString(define):
    d = ""
    for i in defines:
        d += define + " " + i + "=" + str(defines[i]).lower() + " "
    return d

loki = folder + "loki"
if plat == "win":
    if defines["DBG"]:
        defines["MSVC_COMPILER"] = True
        defineStr = genDefinesString("/D")
        runCmd("cl /nologo /std:c++14 /Zi /arch:AVX /EHsc " + defineStr +  " src/main.cc /Fo:"+loki+".obj /Fd:"+loki+".pdb /Fe:"+loki+".exe")
    else:
        defines["CLANG_COMPILER"] = True
        defineStr = genDefinesString("-D")
        runCmd("clang++ -Ofast " + defineStr + " src\main.cc -o "+folder+"loki.exe")
elif plat == "lin":
    defines["CLANG_COMPILER"] = True
    defineStr = genDefinesString("-D")
    runCmd("clang++ -Ofast " + defineStr + " src/main.cc -o "+folder+"loki", True)
