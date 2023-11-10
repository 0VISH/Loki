#include "backend.cc"
//#include "../frontend/typeBasic.cc"

char *typeIDToName[] = {
    "unkown",
    "void",
    "struct",
    "string",
    "double",
    "i64",
    "i64",
    "float",
    "i32",
    "i32",
    "i16",
    "i16",
    "i8",
    "i8",
    "double",
    "i64",
};

void writeReg(Bytecode bc){
    Reg reg = (Reg)bc;
    if(reg < 0){
	write("@_%d", reg*-1);
	return;
    };
    write("%%_%d", reg);
};
void translate(BytecodeBucket *buc, u32 &off){
    off += 1;
    Bytecode *bytecodes = buc->bytecodes;
    switch(bytecodes[off-1]){
    case Bytecode::JMP:{
	Bytecode label = getBytecode(bytecodes, off);
	write("br label %%__%d", label);
    }break;
    case Bytecode::ADD:{
	Bytecode type = getBytecode(bytecodes, off);
	writeReg(getBytecode(bytecodes, off));
	write(" = add %s ", typeIDToName[(u16)type]);
	writeReg(getBytecode(bytecodes, off));
	write(", ");
	writeReg(getBytecode(bytecodes, off));
    }break;
    case Bytecode::STORE:{
	Bytecode type = getBytecode(bytecodes, off);
	Bytecode dst = getBytecode(bytecodes, off);
	Bytecode src = getBytecode(bytecodes, off);
	char *typeName = typeIDToName[(u16)type];
	write("store %s ", typeName);
	writeReg(src);
	write(", %s* ", typeName);
	writeReg(dst);
    }break;
    case Bytecode::JMPS:{
	Bytecode cmpReg = getBytecode(bytecodes, off);
	Bytecode labelT = getBytecode(bytecodes, off);
	Bytecode labelF = getBytecode(bytecodes, off);
	write("br i1 ");
	writeReg(cmpReg);
	write(", label %%__%d, label %%__%d", labelT, labelF);
    }break;
    case Bytecode::LOAD:{
	Bytecode type = getBytecode(bytecodes, off);
	writeReg(getBytecode(bytecodes, off));
	char *typeName = typeIDToName[(u16)type];
	write(" = load %s, %s* ", typeName, typeName);
	writeReg(getBytecode(bytecodes, off));
    }break;
    case Bytecode::RET:{
	Bytecode bc = getBytecode(bytecodes, off);
	write("ret ");
	if((bool)bc == true){
	    write("void");
	}else{
	    bc = getBytecode(bytecodes, off);
	    writeReg(bc);
	};
	off += 1;
    }break;
    case Bytecode::MOV_CONST:{
	Bytecode bc = getBytecode(bytecodes, off);
	if(bc == (Bytecode)Type::COMP_INTEGER){
	    bc = getBytecode(bytecodes, off);
	    writeReg(bc);
	    write(" = add i64 0, %lld", getConstIntAndUpdate(bytecodes, off));
	}else{
	    bc = getBytecode(bytecodes, off);
	    writeReg(bc);
	    write(" = add double 0, %f", getConstDecAndUpdate(bytecodes, off));
	};
    }break;
    case Bytecode::DEF:{
	s64 outCount = getConstIntAndUpdate(bytecodes, off);
	Bytecode outputType;
	if(outCount == 0){outputType = (Bytecode)Type::XE_VOID;}
	else{
	    //TODO: outCount > 1???????
	    outputType = getBytecode(bytecodes, off);
	};
	Bytecode bc = getBytecode(bytecodes, off);
	write("define %s @__%d(", typeIDToName[(u16)outputType], bc);
	s64 inCount = getConstIntAndUpdate(bytecodes, off);
	if(inCount != 0){
	    while(inCount != 1){
		Bytecode inputType = getBytecode(bytecodes, off);
		bc = getBytecode(bytecodes, off);
		write("%s ,", typeIDToName[(u16)inputType]);
		writeReg(bc);
		inCount -= 1;
	    };
	    Bytecode inputType = getBytecode(bytecodes, off);
	    bc = getBytecode(bytecodes, off);
	    write("%s ", typeIDToName[(u16)inputType]);
	    writeReg(bc);
	};
    }break;
    case Bytecode::BLOCK_START:{
	write("{");
    }break;
    case Bytecode::BLOCK_END:{
	write("}");
    }break;
    case Bytecode::LABEL:{
	Bytecode bc = getBytecode(bytecodes, off);
	write("br label %%__%d\n__%d:", bc, bc);
    }break;
    case Bytecode::CMP:{
	char *cmp = "icmp";
	char  type = NULL;
	char *cmpMethod;
	switch(getBytecode(bytecodes, off)){
	case Bytecode::SETG:  cmpMethod="gt";break;
	case Bytecode::SETL:  cmpMethod="lt";break;
	case Bytecode::SETE:
	    cmpMethod="eq";
	    type = ' ';
	    break;
	case Bytecode::SETGE: cmpMethod="ge";break;
	case Bytecode::SETLE: cmpMethod="le";break;
	};
       
	Bytecode bc = getBytecode(bytecodes, off);
	if(isFloat((Type)bc)){
	    type = 'o';
	    cmp  = "fcmp";
	}else if(type == NULL){
	    if(isIntU((Type)bc)){type = 's';}
	    else{type = 'u';};
	};

	Bytecode dest = getBytecode(bytecodes, off);
	Bytecode lhs  = getBytecode(bytecodes, off);
	Bytecode rhs  = getBytecode(bytecodes, off);

	writeReg(dest);
	write(" = %s %c%s %s ", cmp, type, cmpMethod, typeIDToName[(u16)bc]);
	writeReg(lhs);
	write(", ");
	writeReg(rhs);
    }break;
    case Bytecode::ALLOC:{
	Bytecode type = getBytecode(bytecodes, off);
	Bytecode reg = getBytecode(bytecodes, off);
	writeReg(reg);
	write(" = alloca %s", typeIDToName[(u16)type]);
    }break;
    default:
	UNREACHABLE;
    };
    write("\n");
};

EXPORT void BackendCompile(BytecodeFile *bf, Config *config){
    BytecodeBucket *curBucket = bf->firstBucket;
    u32 off = 0;
    while(curBucket){
	switch(curBucket->bytecodes[off]){
	case Bytecode::NONE: return;
	case Bytecode::NEXT_BUCKET:
	    curBucket = curBucket->next;
	    off = 0;
	    continue;
	};
	translate(curBucket, off);
    };
    return;
};
EXPORT void initLLVMBackend(){
    initBackend();
};
EXPORT void uninitLLVMBackend(){
    uninitBackend();
};
EXPORT void callExternalDeps(){
    char buff[1024];
    sprintf(buff, "clang out.ll -c");
    printf("\n[LLVM] %s\n", buff);
    if(system(buff) != 0){return;};
    sprintf(buff, "link /NOLOGO /SUBSYSTEM:WINDOWS /ENTRY:__%d out.o", config.entryPointID);
    printf("[LINKER] %s\n", buff);
    system(buff);
};
