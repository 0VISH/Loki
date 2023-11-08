#include "backend.cc"

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

void translateBlock(BytecodeBucket *buc, u32 &x);
void translate(BytecodeBucket *buc, u32 &off){
    off += 1;
    Bytecode *bytecodes = buc->bytecodes;
    switch(bytecodes[off-1]){
    case Bytecode::MOV_CONST:{
	Bytecode bc = getBytecode(bytecodes, off);
	if(bc == (Bytecode)Type::COMP_INTEGER){
	    bc = getBytecode(bytecodes, off);
	    write("%%%d = add i64 0, %lld", bc, getConstIntAndUpdate(bytecodes, off));
	}else{
	    bc = getBytecode(bytecodes, off);
	    write("%%%d = add double 0, %lld", bc, getConstDecAndUpdate(bytecodes, off));
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
	write("define %s @%d(", typeIDToName[(u16)outputType], bc);
	s64 inCount = getConstIntAndUpdate(bytecodes, off);
	while(inCount != 1){
	    Bytecode inputType = getBytecode(bytecodes, off);
	    bc = getBytecode(bytecodes, off);
	    write("%s %%%d,", typeIDToName[(u16)inputType], bc);
	    inCount -= 1;
	};
	Bytecode inputType = getBytecode(bytecodes, off);
	bc = getBytecode(bytecodes, off);
	write("%s %%%d){\n", typeIDToName[(u16)inputType], bc);
	off += 1;
	translateBlock(buc, off);
	write("}");
    }break;
    };
    write("\n");
};
void translateBlock(BytecodeBucket *buc, u32 &x){
    u32 off = x;
    while(buc){
	while(buc->bytecodes[off] != Bytecode::NEXT_BUCKET && off != bytecodes_in_bucket && buc->bytecodes[off] != Bytecode::NONE && buc->bytecodes[off] != Bytecode::BLOCK_END){
	    translate(buc, off);
	    off += 1;
	};
	buc = buc->next;
	off = 0;
    };
    x = off;
};

EXPORT bool BackendCompile(BytecodeFile *bf, Config *config){
    BytecodeBucket *curBucket = bf->firstBucket;
    u32 off = 0;
    translateBlock(curBucket, off);
    return true;
};
EXPORT void initLLVMBackend(){
    initBackend();
};
EXPORT void uninitLLVMBackend(){
    uninitBackend();
};
