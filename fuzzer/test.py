import colorama
from sys import argv
import subprocess
import os

outputFileName = "bin/fuzz/fuzzOutput.txt"
dotsCount = 40

compilerPath = None
for i in argv:
    if i.startswith("exe:"):
        compilerPath = i[len("exe:"):]
if compilerPath == None:
    print("which compiler? exe:path_to_exe")
    quit()

if os.path.isfile(compilerPath) == False:
    print("path to compiler is incorrect")
    quit()

colorama.init(autoreset = True)
    
if not os.path.isdir("bin/fuzz"): os.makedirs("bin/fuzz")

def getFilesInFolder(folderPath):
    files = []
    entries = os.listdir(folderPath)

    for entry in entries:
        entryPath = os.path.join(folderPath, entry)
        if os.path.isfile(entryPath):
            files.append(folderPath + entry)

    return files

outputFile = open(outputFileName, "w")

for i in getFilesInFolder("test/"):
    print(i, end="")
    remainingDots = dotsCount - len(i)
    print("." * remainingDots, end="")
    process = subprocess.Popen("call \""+compilerPath+"\" " + i, shell=True, stdout=outputFile)
    process.wait()
    if process.returncode != 0:
        print(colorama.Fore.RED + "FAIL")
    else:
        print(colorama.Fore.GREEN + "OK")

outputFile.close()
