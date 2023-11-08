#include "backend.cc"

void translate(Bytecode *bytecodes, u32 &off, char *typeIDToName[]){
    switch(bytecodes[off]){
    case Bytecode::DEF:{
	off += 1;
	s64 outCount = getConstIntAndUpdate(bytecodes, off);
	//TODO: outCount > 1???????
	Bytecode outputType = getBytecode(bytecodes, off);
	Bytecode bc = getBytecode(bytecodes, off);
	write("define %s @%d(", typeIDToName[(u16)outputType], bc);
	s64 inCount = getConstIntAndUpdate(bytecodes, off);
	while(inCount != 0){
	    Bytecode inputType = getBytecode(bytecodes, off);
	    bc = getBytecode(bytecodes, off);
	    write("%s %%%d,", typeIDToName[(u16)inputType], bc);
	    inCount -= 1;
	};
	write("){}");
    }break;
    };
};

EXPORT bool BackendCompile(BytecodeFile *bf, Config *config, char *typeIDToName[]){
    initBackend();
    BytecodeBucket *curBucket = bf->firstBucket;
    while(curBucket){
	u32 off = 0;
	while(curBucket->bytecodes[off] != Bytecode::NEXT_BUCKET || off != bytecodes_in_bucket){
	    translate(curBucket->bytecodes, off, typeIDToName);
	    off += 1;
	};
	curBucket = curBucket->next;
    };
    printf("%s", pages[pages.count-1]);
    uninitBackend();
    return true;
};
