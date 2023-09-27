import Omen
from sys import argv

dbg = not("rls" in argv)
simd = "simd" in argv
plat = "win"
if "lin" in argv: plat = "lin"

Omen.setBuildDir(plat, dbg)

if dbg and plat=="win":
    extra = "/std:c++14 /arch:AVX /EHsc"
    Omen.build("src/main.cc", "Loki", "cl", extraSwitches=extra)
else:
    Omen.build("src/main.cc", "Loki", "clang++")
