#if(WIN)
#define MICROSOFT_CRAZINESS_IMPLEMENTATION
#include "microsoft_craziness.h"
#endif

/*
  (dd): (bytecode file id, specific id)
  register:  %_dd
  struct:    %_struct.dd
  global:    @_Gd
  procedure: @__dd
 */
void writeReg(Bytecode bc, s16 id){
    Reg reg = (Reg)bc;
    if(reg < 0){
	write("@_G%d", reg*-1);
	return;
    };
    write("%%_%d%d", id, reg);
};
void writeType(Bytecode bc, s16 id){
    Type type = (Type)bc;
    switch(type){
    case Type::XE_VOID: write("void");break;
    case Type::COMP_DECIMAL:
    case Type::F_64:    write("double");break;
    case Type::COMP_INTEGER:
    case Type::U_64:
    case Type::S_64:    write("i64");break;
    case Type::U_32:
    case Type::S_32:    write("i32");break;
    case Type::U_16:
    case Type::S_16:    write("i16");break;
    case Type::U_8:
    case Type::S_8:     write("i8");break;
    case Type::PTR:     write("ptr");break;
    default: write("%%_struct.%d%d", id, bc);break;
    };
};

void translate(BytecodeBucket *buc, u32 &off, s16 id, Config *config){
    off += 1;
    Bytecode *bytecodes = buc->bytecodes;
    char *binOpName;
    switch(bytecodes[off-1]){
    case Bytecode::STRUCT:{
	Bytecode structID = getBytecode(bytecodes, off);
	Bytecode count    = getBytecode(bytecodes, off);
	write("%%_struct.%d%d = type{", id, structID);
	for(u32 x=0; x<(u32)count - 1; x+=1){
	    writeType(getBytecode(bytecodes, off), id);
	    write(", ");
	};
	writeType(getBytecode(bytecodes, off), id);
	write("}");
    }break;
    case Bytecode::CAST:{
	Bytecode targetType = getBytecode(bytecodes, off);
	Bytecode targetReg  = getBytecode(bytecodes, off);
	Bytecode srcType    = getBytecode(bytecodes, off);
	Bytecode srcReg     = getBytecode(bytecodes, off);
	if(targetType < srcType){
	    writeReg(targetReg, id);
	    write(" = trunc ");
	    writeType(srcType, id);
	    write(" ");
	    writeReg(srcReg, id);
	    write(" to ");
	    writeType(targetType, id);
	};
    }break;
    case Bytecode::CALL:{
	Bytecode fileID = getBytecode(bytecodes, off);
	Bytecode procID = getBytecode(bytecodes, off);
	write("call ");
	u32 retCount = (u32)getBytecode(bytecodes, off);
	if(retCount == 0){
	    write("%s ", "void");
	};
	//FIXME: doesnt work in llvm api. work around?
	while(retCount != 0){
	    writeType(getBytecode(bytecodes, off), id);
	    write(" ");
	    retCount -= 1;
	};
	write("@__%d%d(", fileID, procID);
	u32 inCount = (u32)getBytecode(bytecodes, off);
	while(inCount > 1){
	    writeType(getBytecode(bytecodes, off), id);
	    write(" ");
	    writeReg(getBytecode(bytecodes, off), id);
	    write(", ");
	    inCount -= 1;
	};
	if(inCount == 1){
	    writeType(getBytecode(bytecodes, off), id);
	    write(" ");
	    writeReg(getBytecode(bytecodes, off), id);
	};
	write(")");
    }break;
    case Bytecode::JMP:{
	Bytecode label = getBytecode(bytecodes, off);
	write("br label %%__%d%d", id, label);
    }break;
    case Bytecode::ADD: binOpName = "add"; goto COMPILE_BINOP;
    case Bytecode::SHR: binOpName = "shr"; goto COMPILE_BINOP;
    case Bytecode::SHL:{
	binOpName = "shl";
	COMPILE_BINOP:
	Bytecode type = getBytecode(bytecodes, off);
	writeReg(getBytecode(bytecodes, off), id);
	write(" = %s ", binOpName);
	writeType(type, id);
	write(" ");
	writeReg(getBytecode(bytecodes, off), id);
	write(", ");
	writeReg(getBytecode(bytecodes, off), id);
    }break;
    case Bytecode::STORE:{
	Bytecode type = getBytecode(bytecodes, off);
	Bytecode dst = getBytecode(bytecodes, off);
	Bytecode src = getBytecode(bytecodes, off);
	write("store ");
	writeType(type, id);
	write(" ");
	writeReg(src, id);
	write(", ptr ");
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
	write(" = load ");
	writeType(type, id);
	write(", ");
	writeType(type, id);
	if((Type)type == Type::PTR){write(" ");}
	else{write("* ");};
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
	Bytecode type = getBytecode(bytecodes, off);
	Bytecode bc = getBytecode(bytecodes, off);
	writeReg(bc, id);
	write(" = add ");
	writeType(type, id);
	write(" 0, ");
	if(isInt((Type)type)){
	    write("%lld", getConstIntAndUpdate(bytecodes, off));
	}else{
	    write("%f", getConstDecAndUpdate(bytecodes, off));
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
	write("define ");
	writeType(outputType, id);
	write(" @__");
	if((s16)bc == config->entryPointProcID && id == config->entryPointFileID){
	    write("main(");
	}else{
	    write("%d%d(", id, bc);
	};
	s64 inCount = getConstIntAndUpdate(bytecodes, off);
	if(inCount != 0){
	    while(inCount != 1){
		Bytecode inputType = getBytecode(bytecodes, off);
		bc = getBytecode(bytecodes, off);
		writeType(inputType, id);
		write(" ");
		writeReg(bc, id);
		write(", ");
		inCount -= 1;
	    };
	    Bytecode inputType = getBytecode(bytecodes, off);
	    bc = getBytecode(bytecodes, off);
	    writeType(inputType, id);
	    write(" ");
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
	writeReg(reg, id);
	write(" = global ");
	writeType(type, id);
	write(" ");
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
	write(" = %s %c%s ", cmp, type, cmpMethod);
	writeType(bc, id);
	write(" ");
	writeReg(lhs, id);
	write(", ");
	writeReg(rhs, id);
    }break;
    case Bytecode::ALLOC:{
	Bytecode type = getBytecode(bytecodes, off);
	Bytecode reg = getBytecode(bytecodes, off);
	writeReg(reg, id);
	write(" = alloca ");
	writeType(type, id);
    }break;
    case Bytecode::GET_ELEMENT:{
	Bytecode type = getBytecode(bytecodes, off);
	Bytecode dest = getBytecode(bytecodes, off);
	Bytecode src  = getBytecode(bytecodes, off);
	Bytecode offType = getBytecode(bytecodes, off);
	Bytecode offset = getBytecode(bytecodes, off);
	writeReg(dest, id);
	write(" = getelementptr inbounds ");
	writeType(type, id);
	write(", ptr ");
	writeReg(src, id);
	write(", ");
	writeType(offType, id);
	write(" ");
	writeReg(offset, id);
    }break;
    case Bytecode::GET_MEMBER:{
	Bytecode structID = getBytecode(bytecodes, off);
	Bytecode dest = getBytecode(bytecodes, off);
	Bytecode src  = getBytecode(bytecodes, off);
	Bytecode offset  = getBytecode(bytecodes, off);
	writeReg(dest, id);
	write(" = getelementptr inbounds ");
	writeType(structID, id);
	write(", ptr ");
	writeReg(src, id);
	write(", i32 0, i32 %d", offset);
    }break;
    case Bytecode::_TEXT_STARTUP_START:{
	write("define internal void @_GLOBAL__sub_I_%d() section \".text.startup\" {", id);
    }break;
    case Bytecode::_TEXT_STARTUP_END:{
	write("ret void\n}");
    }break;
    default:
	UNREACHABLE;
    };
    write("\n");
};

void backendCompileStage1(BytecodeBucket *buc, s16 id, Config *config){
    BytecodeBucket *curBucket = buc;
    u32 off = 0;
    while(curBucket){
	switch(curBucket->bytecodes[off]){
	case Bytecode::NONE: return;
	case Bytecode::NEXT_BUCKET:
	    curBucket = curBucket->next;
	    off = 0;
	    continue;
	};
	translate(curBucket, off, id, config);
    };
    return;
};
void initLLVMBackend(){
    initBackend();
};
void uninitLLVMBackend(){
    uninitBackend();
};
bool backendCompileStage2(Config *config){
    char buff[100];
    char targetBuff[100];

    u32 off = 0;
    switch(config->arch){
    case Arch::x64:{
	off = sprintf(targetBuff, "x86_64");
    }break;
    case Arch::x86:{
	off = sprintf(targetBuff, "x86");
    }break;
    };
    switch(config->target){
    case Target::WINDOWS:{
#if(WIN)
	Find_Result res = find_visual_studio_and_windows_sdk();
	if(res.windows_sdk_version == 0){
	    printf("\n[ERROR] cannot find windows sdk\n");
	    return false;
	};
	sprintf(targetBuff+off, "-pc-windows-msvc");
	free_resources(&res);
#endif
    }break;
    case Target::LINUX:{
	sprintf(targetBuff+off, "-unknown-linux-gnu");
    }break;
    case Target::RISCV:{
	u32 arch;
	if(config->arch == Arch::x64){arch = 64;}
	else if(config->arch == Arch::x86){arch = 32;};
	sprintf(targetBuff, "riscv%d -march=rv%di -mabi=lp64d", arch, arch);
    }break;
    };
    
    
    sprintf(buff, "%s.ll", config->out);
    FILE *f = fopen(buff, "w");
    for(u32 x=0; x<pages.count; x+=1){
	Page &pg = pages[x];
	fwrite(pg.mem, 1, pg.watermark, f);
    };
    fclose(f);
    sprintf(buff, "clang -Wno-override-module -target %s %s.ll -c -o %s.obj", targetBuff, config->out, config->out);

    printf("\n[LLVM] %s\n", buff);
    if(system(buff) != 0){return false;};
    if(config->end == EndType::STATIC){return true;};
#if(WIN)
    if(config->target != Target::WINDOWS){return true;};
    sprintf(buff, "link /NOLOGO /SUBSYSTEM:WINDOWS /ENTRY:__main %s.obj /OUT:%s.%s", config->out, config->out, (config->end==EndType::EXECUTABLE)?"exe":"dll");
    printf("[LINKER] %s\n", buff);
    system(buff);
#endif
    return true;
};
