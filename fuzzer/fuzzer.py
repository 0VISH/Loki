from termcolor import colored        #pip install termcolor
from random import randint, sample, uniform
import string
import subprocess
import os

GARBAGE_COUNT = 100
fuzzFile = "bin\\fuzz.xe"
command = "bin\\win\\dbg\\loki.exe " + fuzzFile
outputFileName = "bin\\fuzzOutput.txt"
tab = "    "

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

def genDecimal(type):
    rawDecimal = uniform(-10, 10)
    return str(round(rawDecimal, 5))
def genOperator():
    operators = ['+', '-', '*', '/']
    return operators[randint(0, len(operators)-1)]
def genOperandStrict(type):
    if isFloat(type): return genDecimal(type)
    if isUnsigned(type): return str(randint(0, 10))
    return str(randint(-10, 10))
def genOperandLoose(type):
    if isFloat(type):
        generateNewType = randint(0, 1)
        if generateNewType:
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

def genVarDef(varToType, tabs):
    type = genType()
    name = genIdentifier()
    expr = genExpression(type)

    varToType[name] = type

    loCode = tabs*tab + name + " :" + getTypeName(type) + " = " + expr
    pyCode = name + " = " + expr

    exec(pyCode, pyScope)

    loCode += '\n' + tabs*tab + "//" + name + " should be " + str(round(pyScope[name], 5))
    return loCode
def genVarDecl(varToType, tabs):
    type = genType()
    name = genIdentifier()

    varToType[name] = type

    loCode = tabs*tab + name + " :" + getTypeName(type)
    pyCode = name + " = 0"

    exec(pyCode, pyScope)

    loCode += '\n' + tabs*tab + "//" + name + " should be 0"
    return loCode
