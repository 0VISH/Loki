from termcolor import colored
from sys import argv
import subprocess
import os

outputFileName = "bin\\fuzz\\fuzzOutput.txt"
dotsCount = 40

exe = None
for i in argv:
    if i.startswith("exe:"):
        exe = i[len("exe:"):]
if exe == None:
    print("which compiler?")
    quit()

if not os.path.isdir("bin\\fuzz"): os.makedirs("bin\\fuzz")

def getFilesInFolder(folderPath):
    files = []
    entries = os.listdir(folderPath)

    for entry in entries:
        entryPath = os.path.join(folderPath, entry)
        if os.path.isfile(entryPath):
            files.append(folderPath + entry)

    return files

outputFile = open(outputFileName, "w")

for i in getFilesInFolder("test\\"):
    print(i, end="")
    remainingDots = dotsCount - len(i)
    print("." * remainingDots, end="")
    process = subprocess.Popen(exe + " " + i, shell=True, stdout=outputFile)
    process.wait()
    if process.returncode != 0:
        print(colored("FAIL", "red"))
    else:
        print(colored("OK", "green"))

outputFile.close()
