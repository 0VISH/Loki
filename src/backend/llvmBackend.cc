#include "backend.cc"
#include "../frontend/typeBasic.cc"

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

void writeReg(Bytecode bc, s16 id){
    Reg reg = (Reg)bc;
    if(reg < 0){
	write("@_%d", reg*-1);
	return;
    };
    write("%%_%d%d", id, reg);
};
void translate(BytecodeBucket *buc, u32 &off, s16 id){
    off += 1;
    Bytecode *bytecodes = buc->bytecodes;
    switch(bytecodes[off-1]){
    case Bytecode::JMP:{
	Bytecode label = getBytecode(bytecodes, off);
	write("br label %%__%d%d", id, label);
    }break;
    case Bytecode::ADD:{
	Bytecode type = getBytecode(bytecodes, off);
	writeReg(getBytecode(bytecodes, off), id);
	write(" = add %s ", typeIDToName[(u16)type]);
	writeReg(getBytecode(bytecodes, off), id);
	write(", ");
	writeReg(getBytecode(bytecodes, off), id);
    }break;
    case Bytecode::STORE:{
	Bytecode type = getBytecode(bytecodes, off);
	Bytecode dst = getBytecode(bytecodes, off);
	Bytecode src = getBytecode(bytecodes, off);
	char *typeName = typeIDToName[(u16)type];
	write("store %s ", typeName);
	writeReg(src, id);
	write(", %s* ", typeName);
	writeReg(dst, id);
    }break;
    case Bytecode::JMPS:{
	Bytecode cmpReg = getBytecode(bytecodes, off);
	Bytecode labelT = getBytecode(bytecodes, off);
	Bytecode labelF = getBytecode(bytecodes, off);
	write("br i1 ");
	writeReg(cmpReg, id);
	write(", label %%__%d%d, label %%__%d%d", id, labelT, id, labelF);
    }break;
    case Bytecode::LOAD:{
	Bytecode type = getBytecode(bytecodes, off);
	writeReg(getBytecode(bytecodes, off), id);
	char *typeName = typeIDToName[(u16)type];
	write(" = load %s, %s* ", typeName, typeName);
	writeReg(getBytecode(bytecodes, off), id);
    }break;
    case Bytecode::RET:{
	Bytecode bc = getBytecode(bytecodes, off);
	write("ret ");
	if((bool)bc == true){
	    write("void");
	}else{
	    bc = getBytecode(bytecodes, off);
	    writeReg(bc, id);
	};
	off += 1;
    }break;
    case Bytecode::MOV_CONST:{
	Bytecode bc = getBytecode(bytecodes, off);
	if(bc == (Bytecode)Type::COMP_INTEGER){
	    bc = getBytecode(bytecodes, off);
	    writeReg(bc, id);
	    write(" = add i64 0, %lld", getConstIntAndUpdate(bytecodes, off));
	}else{
	    bc = getBytecode(bytecodes, off);
	    writeReg(bc, id);
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
	write("define %s @__", typeIDToName[(u16)outputType]);
	if((u16)bc == 0){
	    write("main(");
	}else{
	    write("%d%d(", id, bc);
	};
	s64 inCount = getConstIntAndUpdate(bytecodes, off);
	if(inCount != 0){
	    while(inCount != 1){
		Bytecode inputType = getBytecode(bytecodes, off);
		bc = getBytecode(bytecodes, off);
		write("%s ,", typeIDToName[(u16)inputType]);
		writeReg(bc, id);
		inCount -= 1;
	    };
	    Bytecode inputType = getBytecode(bytecodes, off);
	    bc = getBytecode(bytecodes, off);
	    write("%s ", typeIDToName[(u16)inputType]);
	    writeReg(bc, id);
	};
	write("){");
    }break;
    case Bytecode::PROC_END:{
	write("}\n");
    }break;
    case Bytecode::LABEL:{
	Bytecode bc = getBytecode(bytecodes, off);
	write("br label %%__%d%d\n__%d%d:", id, bc, id, bc);
    }break;
    case Bytecode::GLOBAL:{
	Bytecode reg = getBytecode(bytecodes, off);
	Bytecode type = getBytecode(bytecodes, off);
	write("@_%d = global %s ", reg, typeIDToName[(u16)type]);
	if(isFloat((Type)type)){
	    f64 num = getConstDecAndUpdate(bytecodes, off);
	    write("%f", num);
	}else{
	    s64 num = getConstIntAndUpdate(bytecodes, off);
	    write("%lld", num);
	};
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

	writeReg(dest, id);
	write(" = %s %c%s %s ", cmp, type, cmpMethod, typeIDToName[(u16)bc]);
	writeReg(lhs, id);
	write(", ");
	writeReg(rhs, id);
    }break;
    case Bytecode::ALLOC:{
	Bytecode type = getBytecode(bytecodes, off);
	Bytecode reg = getBytecode(bytecodes, off);
	writeReg(reg, id);
	write(" = alloca %s", typeIDToName[(u16)type]);
    }break;
    default:
	UNREACHABLE;
    };
    write("\n");
};

EXPORT void backendCompileStage1(BytecodeFile *bf, Config *config){
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
	translate(curBucket, off, bf->id);
    };
    return;
};
EXPORT void initLLVMBackend(){
    initBackend();
};
EXPORT void uninitLLVMBackend(){
    uninitBackend();
};
EXPORT void backendCompileStage2(Config *config){
    char buff[1024];

    sprintf(buff, "%s.ll", config->out);
    FILE *f = fopen(buff, "w");
    for(u32 x=0; x<pages.count; x+=1){
	Page &pg = pages[x];
	fwrite(pg.mem, 1, pg.watermark, f);
    };
    fclose(f);
    
    sprintf(buff, "clang %s.ll -c -o %s.obj", config->out, config->out);
    printf("\n[LLVM] %s\n", buff);
    if(system(buff) != 0){return;};
    if(config->end == EndType::STATIC){return;};
    sprintf(buff, "link /NOLOGO /SUBSYSTEM:WINDOWS /ENTRY:__main %s.obj /OUT:%s.%s", config->out, config->out, (config->end==EndType::EXECUTABLE)?"exe":"dll");
    printf("[LINKER] %s\n", buff);
    system(buff);
};
