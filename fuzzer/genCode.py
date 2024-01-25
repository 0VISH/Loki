import colorama
from random import randint, sample, uniform
from sys import argv
import string
import subprocess
import os

GARBAGE_COUNT = 100
ENTITY_COUNT  = 7
fuzzFileName = "bin/fuzz/fuzz.xe"
outputFileName = "bin/fuzz/fuzzOutput.txt"
tab = "    "

compilerPath = None
for i in argv:
    if i.startswith("exe:"):
        compilerPath = i[len("exe:"):]
if compilerPath == None:
    print("which compiler? exe:path_to_loki")
    quit()

if os.path.isfile(compilerPath) == False:
    print("path to compiler is incorrect:", compilerPath)
    quit()

if not os.path.isdir("bin/fuzz"): os.makedirs("bin/fuzz")

colorama.init(autoreset=True)

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

def isFloat(type):    return type == Type.f16 or type == Type.f32
def isUnsigned(type): return type == Type.u8  or type == Type.u16 or type == Type.u32
def getTypeName(type):
    typeNames = ["u8", "s8", "u16", "s16", "f16", "u32", "s32", "f32"]
    return typeNames[type]
def genType(): return randint(0, Type.TYPE_LEN-1)
def randBool(): return randint(0, 1)

def genDecimal(type):
    rawDecimal = uniform(-10, 10)
    return str(round(rawDecimal, 5))
def genOperator():
    operators = ['+', '-', '*', '/']
    return operators[randint(0, len(operators)-1)]
def genBoolOperator():
    operators = ['>', '>=', '==', '<=', '<']
    return operators[randint(0, len(operators)-1)]
def genOperandStrict(type):
    if isFloat(type): return genDecimal(type)
    if isUnsigned(type): return str(randint(0, 10))
    return str(randint(-10, 10))
def genOperandLoose(type):
    if isFloat(type):
        if randBool():
            newType = randint(0, type)
            return genOperandStrict(newType)
    return genOperandStrict(type)

def genExpression(type):
    len = randint(1,5)
    if len%2 == 0: len -= 1

    expr = ""
    isDiv = False
    for i in range(0, len):
        if i%2 == 0:
            operand = genOperandLoose(type)
            if isDiv:
                operandPy = int(float(operand))
                while operandPy == 0:
                    operand = genOperandLoose(type)
                    operandPy = int(float(operand))
                isDiv = False
            expr += operand + " "
        else:
            operator = genOperator()
            if isFloat(type) == False:
                while operator == '/': operator = genOperator()
            if operator == '/': isDiv = True
            expr += operator + " "
            
    exec("__value="+expr, globals())
    
    if isUnsigned(type):
        if __value < 0: return "(" + expr + ") * -1"
    return expr
def genIdentifier():
    #all identifiers start with '_'
    #we dont bother checking if the generated identifier is already generated cause the chances are too low
    s = string.ascii_lowercase+string.ascii_uppercase+string.digits
    len = randint(1,20)
    return "_" + ''.join(sample(s, len))

pyScope = {}

def genVarDef(varToType, tabs, depth):
    type = genType()
    name = genIdentifier()
    expr = genExpression(type)

    varToType[name] = type

    loCode = tabs*tab + name + " :" + getTypeName(type) + " = " + expr + ";"
    pyCode = name + " = " + expr

    exec(pyCode, pyScope)

    loCode += '\n' + tabs*tab + "//" + name + " should be " + str(round(pyScope[name], 5))
    return loCode
def genVarDecl(varToType, tabs, depth):
    type = genType()
    name = genIdentifier()

    varToType[name] = type

    loCode = tabs*tab + name + " :" + getTypeName(type) + ";"
    pyCode = name + " = 0"

    exec(pyCode, pyScope)

    loCode += '\n' + tabs*tab + "//" + name + " should be 0"
    return loCode
def genBody(varToType, tabs, depth, shouldBeExecuted):
    body = None
    if randBool(): body = "\n" + tabs*tab + "{\n"
    else: body = "{\n"

    if shouldBeExecuted: body += (tabs+1)*tab + "//should be executed\n"
    else: body += (tabs+1)*tab + "//should NOT be executed\n"
    
    len = randint(0, 5)
    varToTypeBody = {}
    for i in range(0, len):
        body += genEntity(varToTypeBody, tabs+1, depth+1) + "\n"
        
    body += tabs*tab + "}"
    return body
def genProcDef(varToType, tabs, depth):
    name = genIdentifier()
    input = randint(0, 5)
    output = randint(0, 5)

    inputTypes  = []
    outputTypes = []

    for i in range(0, input):
        inputTypes.append(genType())
    for i in range(0, output):
        outputTypes.append(genType())

    loCode = tabs*tab + name + " :: proc("

    if len(inputTypes) != 0:
        for i in inputTypes:
            inputName = genIdentifier()
            loCode += inputName + ": " + getTypeName(i) + ", "
        loCode = loCode[:-2] #removing ', '

    loCode += ')'
    if len(outputTypes) != 0:
        loCode += " -> ("
        for i in outputTypes:
            loCode += getTypeName(i) + ", "
        loCode = loCode[:-2]
        loCode += ")"

    varToTypeBody = {}
    loCode += genBody(varToTypeBody, tabs, depth, True) + ";"
    return loCode
def genIf(varToType, tabs, depth):
    loCode = tabs*tab + "if "
    type = genType()
    brackets = False
    if randBool():
        brackets = True
        loCode += "("
        
    lhsExpr = genExpression(type)
    rhsExpr = genExpression(type)
    op      = genBoolOperator()

    boolExpr = lhsExpr + " " + op + " " + rhsExpr
    loCode += boolExpr
    tempScope = {}
    exec("__value="+boolExpr, tempScope)
    shouldIfBodyBeExecuted = tempScope["__value"]
    
    if brackets: loCode += ")"

    varToTypeIfBody = {}
    loCode += genBody(varToTypeIfBody, tabs, depth, shouldIfBodyBeExecuted)
    if randBool(): return loCode + ";"

    #else if
    lhsExpr = genExpression(type)
    rhsExpr = genExpression(type)
    op      = genBoolOperator()
    
    loCode += "else if "
    if brackets: loCode += "("
    boolExpr = lhsExpr + " " + op + " " + rhsExpr
    loCode += boolExpr
    shouldElseIfBodyBeExecuted = False
    if shouldIfBodyBeExecuted == False:
        tempScope = {}
        exec("__value="+boolExpr, tempScope)
        shouldElseIfBodyBeExecuted = tempScope["__value"]
    if brackets: loCode += ")"

    varToTypElseIfBody = {}
    loCode += genBody(varToTypeIfBody, tabs, depth, shouldElseIfBodyBeExecuted)

    #else
    shouldElseBodyBeExecuted = (shouldIfBodyBeExecuted == False) and (shouldElseIfBodyBeExecuted == False)
    varToTypeElseBody = {}
    loCode += "else"
    loCode += genBody(varToTypeElseBody, tabs, depth, shouldElseBodyBeExecuted) + ";"
    return loCode
    
entities = [genVarDef, genVarDecl, genProcDef, genIf]

def genEntity(varToType, tabs, depth):
    if depth >= 4: return tabs*tab + "//depth limit exceeded, hence not generating code"
    rand = randint(0, len(entities)-1)
    return entities[rand](varToType, tabs, depth)



outputFile = open(outputFileName, "w")
fail = 0
failed = []
for j in range(0, GARBAGE_COUNT):
    loCode = ""
    print("["+str(j)+"] Generating garbage", end="."*35)
    x = {}
    fuzzFile = open(fuzzFileName, "w")
    loCode += "main :: proc(){\n"
    for i in range(0, ENTITY_COUNT):
        loCode += genEntity(x, 1, 0) + "\n"
    loCode += "};"
    fuzzFile.write(loCode)
    fuzzFile.close()
    
    process = subprocess.Popen("call \""+compilerPath+"\" " + fuzzFileName , shell=True, stdout=outputFile)
    process.wait()
    if process.returncode != 0:
        print(colorama.Fore.RED  + "FAIL")
        print(tab + "return code:", process.returncode)
        print(loCode)
        fName = "bin/fuzz/fuzz"+str(j)+".loki"
        f = open(fName, "w")
        f.write(loCode)
        f.close()
        print(tab + "wrote to", fName)
        fail += 1
        failed.append(fName)
    else:
        print(colorama.Fore.GREEN + "OK")
outputFile.close()

print("\n[STATUS]")
print(tab + "total:", GARBAGE_COUNT)
print(tab + "ok:", GARBAGE_COUNT - fail)
print(tab + "fail:", fail)
for i in failed:
    print(2*tab + i)
