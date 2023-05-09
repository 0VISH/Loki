import random
import string

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
    
def genIdentifier():
    #all identifiers start with '_'
    #we dont bother checking if the generated identifier is already generated cause the chances are too low
    s = string.ascii_lowercase+string.ascii_uppercase+string.digits
    len = random.randint(1,20)
    return "_" + ''.join(random.sample(s, len))
def isFloat(type):    return type == Type.f16 or type == Type.f32
def isUnsigned(type): return type == Type.u8  or type == Type.u16 or type == Type.u32
def genOperand(type):
    #8
    if type < Type.u16: return str(random.randint(0, 200))
    #16
    if type < Type.u32:
        if isFloat(type): return str(random.uniform(0.0, 10000.0))
        return str(random.randint(0, 10000))
    #32
    if isFloat(type): return str(random.uniform(0.0, 10000000.0))
    return str(random.randint(0, 10000000))
def genOperator():
    operators = [' + ', ' - ', ' * ', ' / ']
    return operators[random.randint(0, len(operators)-1)]
def genType(): return random.randint(0, Type.TYPE_LEN-1)
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
    len = random.randint(1,20)
    if len%2 == 0: len -= 1
    expr = ""
    for i in range(0, len):
        if i%2 == 0:
            expr += genOperand(type)
            continue
        expr += genOperator()
    if isUnsigned(type):
        exec("value="+expr, globals())
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
    inputCount = random.randint(0, 30)
    inputVarMapType = {}
    xeCode = name + " :: proc("
    pyCode = "def " + name + "("
    for i in range(0, inputCount):
        type = genType()
        identifier = genIdentifier()
        inputVarMapType[identifier] = type
        xeCode += identifier + ":" + type2String(type) + ", "
        pyCode += identifier + ", "
    xeCode = xeCode.rstrip(", ")
    pyCode = pyCode.rstrip(", ")
    xeCode += ") "
    pyCode += ")"
    io.append(inputVarMapType)
    outputCount = random.randint(0, 30)
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
    xeCode += "{}"
    pyCode += ": pass\n"
    procMapIO[name] = tuple(io)
    exec(pyCode, pyScope)
    return xeCode, None, pyCode
    

genEntities = [genVarDefEntity, genVarDeclEntity, genProcDefEntity]

def genRandEntities(tabs = 0):
    varMapType = {}
    procMapIO = {}
    entityCount = random.randint(15, 30)
    xeCode = ""
    pyCode = ""
    for i in range(0,entityCount):
        entityID = random.randint(0, len(genEntities)-1)
        xe, xeCheck, py = genEntities[entityID](varMapType, procMapIO)
        xeCode += ("\t"*tabs) + xe + "\n"
        if xeCheck != None: xeCode += ("\t"*tabs) + xeCheck
        xeCode += "\n\n"
        pyCode += "\t" * tabs
        pyCode += py + "\n"
    return xeCode, pyCode

print("Generating garbage...")
xeCode, _ = genRandEntities()
file = open("fuzz.xe", "w")
file.write(xeCode)
file.close()
print("Done :)")
