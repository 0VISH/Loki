from termcolor import colored        #pip install termcolor
from random import randint, sample
import string
import subprocess
import os

GARBAGE_COUNT = 100
fuzzFile = "bin\\fuzz.xe"
command = "bin\\win\\dbg\\xenon.exe " + fuzzFile
outputFileName = "bin\\fuzzOutput.txt"

class Type():
    u8  = 0
    s8  = 1
    u16 = 2
    s16 = 3
    f16 = 4
    u32 = 5
    s32 = 6
    f32 = 7
    TYPE_LEN = 8

#             u8   s8    u16    s16    f16      u32          s32       f32
typeMaxVal = (255, 127, 65535, 32767, 1000.0, 4294967295, 2147483647, 1000.0)

def genDecimal():
    int = str(randint(0, 200))
    dec = str(randint(0, 200))
    return int + "." + dec

def genIdentifier():
    #all identifiers start with '_'
    #we dont bother checking if the generated identifier is already generated cause the chances are too low
    s = string.ascii_lowercase+string.ascii_uppercase+string.digits
    len = randint(1,20)
    return "_" + ''.join(sample(s, len))
def isFloat(type):    return type == Type.f16 or type == Type.f32
def isUnsigned(type): return type == Type.u8  or type == Type.u16 or type == Type.u32
def genOperand(type):
    #8
    if type < Type.u16: return str(randint(0, 200))
    #16
    if type < Type.u32:
        if isFloat(type): return genDecimal()
        return str(randint(0, 10000))
    #32
    if isFloat(type): return genDecimal()
    return str(randint(0, 10000000))
def genOperator():
    operators = ['+', '-', '* ', '/ ']
    return operators[randint(0, len(operators)-1)]
def genType(): return randint(0, Type.TYPE_LEN-1)
def type2String(type):
    if type == Type.u8:  return "u8"
    if type == Type.u16: return "u16"
    if type == Type.u32: return "u32"
    if type == Type.s8:  return "s8"
    if type == Type.s16: return "s16"
    if type == Type.s32: return "s32"
    if type == Type.f16: return "f16"
    if type == Type.f32: return "f32"
def genExpression(type):
    #TODO: avoid gen div by 0
    len = randint(1,20)
    if len%2 == 0: len -= 1
    expr = ""
    div = False
    for i in range(0, len):
        if i%2 == 0:
            operand = genOperand(type)
            if div:
                #avoid div by 0
                div = False
                while int(operand) == 0:
                    operand = genOperand(type)
            expr += operand
            continue
        op = genOperator()
        if op == '/': div = True
        expr += ' ' + op + ' '
    #checkin if value of expr is greater than var size
    exec("value="+expr, globals())
    max = typeMaxVal[type]
    if value > max:
        expr += "- " + str(value-max + 10)
    elif value < -max:
        expr += "+ " + str(max-value - 10)
    if isUnsigned(type):
        if value < 0: return "(" + expr + ") * -1"
    return expr

pyScope = {}

def genVarDefEntity(varMapType, procMapIO):
    type = genType()
    identifier = genIdentifier()
    varMapType[identifier] = type
    expr = genExpression(type)
    xeCode = identifier + ":" + type2String(type) + " = " + expr
    pyCode = identifier + " = " + expr
    exec(pyCode, pyScope)
    xeCheck = "//" + identifier + " should be " + str(pyScope[identifier])
    return xeCode, xeCheck, pyCode
def genVarDeclEntity(varMapType, procMapIO):
    type = genType()
    identifier = genIdentifier()
    varMapType[identifier] = type
    xeCode = identifier + ":" + type2String(type)
    pyCode = identifier + " = 0"
    exec(pyCode, pyScope)
    xeCheck = "//" + identifier + " should be " + str(pyScope[identifier])
    return xeCode, xeCheck, pyCode
def genProcDefEntity(varMapType, procMapIO):
    name = genIdentifier()
    io = []
    inputCount = randint(0, 30)
    inputVarMapType = {}
    xeCode = name + " :: proc("
    pyCode = "def " + name + "("
    for i in range(0, inputCount):
        type = genType()
        identifier = genIdentifier()
        #generate unique identifiers
        while identifier in inputVarMapType: identifier = genIdentifier()
        inputVarMapType[identifier] = type
        xeCode += identifier + ":" + type2String(type) + ", "
        pyCode += identifier + ", "
    xeCode = xeCode.rstrip(", ")
    pyCode = pyCode.rstrip(", ")
    xeCode += ")"
    pyCode += ") "
    io.append(inputVarMapType)
    outputCount = randint(0, 30)
    if outputCount != 0:
        xeCode += "-> ("
        types = []
        for i in range(0, outputCount):
            type = genType()
            xeCode += type2String(type) + ", "
            types.append(type)
        xeCode = xeCode.rstrip(", ")
        xeCode += ")"
        io.append(types)
    xeCode += "\n{\n"
    pyCode += ":\n"
    xe, py = genRandEntities(1, True)
    xeCode += xe + "}"
    pyCode += py +"\tpass"
    procMapIO[name] = tuple(io)
    exec(pyCode, pyScope)
    return xeCode, None, pyCode
    

genEntities = [genVarDefEntity, genVarDeclEntity, genProcDefEntity]

def genRandEntities(tabs = 0, inProc = False):
    varMapType = {}
    procMapIO = {}
    entityCount = randint(2, 5)
    xeCode = ""
    pyCode = ""
    tabs = "\t" * tabs
    for i in range(0,entityCount):
        entityID = randint(0, len(genEntities)-1)
        if entityID == 2 and inProc == True: continue
        xe, xeCheck, py = genEntities[entityID](varMapType, procMapIO)
        xeCode += tabs + xe + "\n"
        if xeCheck != None: xeCode += tabs + xeCheck
        xeCode += "\n\n"
        pyCode += tabs
        pyCode += py + "\n"
    return xeCode, pyCode

outputFile = open(outputFileName, "w")
file = open(fuzzFile, "w")

fail = 0
ok = 0
for i in range(1, GARBAGE_COUNT+1):
    print("["+str(i)+"] Generating garbage...")
    xeCode, _ = genRandEntities()
    file.write(xeCode)
    file.flush()
    print("wrote garbage to", fuzzFile)
    print("calling:", command)
    process = subprocess.Popen(command, shell=True, stdout=outputFile)
    process.wait()
    if process.returncode != 0:
        fName = "bin\\fuzz"+str(i)+".xe"
        f = open(fName, "w")
        f.write(xeCode)
        f.close()
        print("wrote to", fName)
        print(colored("FAIL", "red"))
        fail += 1
    else:
        print(colored("OK", "green"))
        ok += 1
    print()
    file.seek(0)

file.close()
outputFile.close()
os.remove(fuzzFile)
print("OK:", ok)
print("FAIL:", fail)
print("total:", ok+fail, "\n")
print("Done :)")
