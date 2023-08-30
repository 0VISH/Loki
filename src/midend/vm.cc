#define BYTECODE_INPUT Bytecode *page, VM &vm
#define STACK_SIZE 10000

struct Register{
    union{
	s64   sint;
	u64   uint;
	f64   dec;
	char *ptr;
    };
};
struct VM{
    DynamicArray<Bytecode*>  procs;
    DynamicArray<Bytecode*> *labels;
    Register *registers;
    BytecodeBucket *buc;
    Bytecode *cursor;
    char stack[STACK_SIZE];
    u32 stackOff;
    u32 procBlock;

    void init(BytecodeBucket *bucket, DynamicArray<Bytecode*> *lbls, u32 procBlockID, u32 offset = 0){
	stackOff = 0;
	labels = lbls;
	procBlock = procBlockID;
	buc = bucket;
	cursor = buc->bytecodes + offset;
	registers = (Register*)mem::alloc(sizeof(Register) * REGISTER_COUNT);
	procs.init(3);
    };
    void uninit(){
	mem::free(registers);
	procs.uninit();
    };
};

BytecodeBucket *getBucketWithInsideAdd(BytecodeBucket *cur, Bytecode *add){
    if(add >= cur->bytecodes){
	while(cur){
	    BytecodeBucket *nextBuc = cur->next;
	    if((add >= cur->bytecodes) && (add <= nextBuc->bytecodes)){
		return cur;
	    };
	    cur = nextBuc;
	};
    }else{
	while(cur){
	    BytecodeBucket *prevBuc = cur->prev;
	    if((add >= prevBuc->bytecodes) && (add <= cur->bytecodes)){
		return prevBuc;
	    };
	    cur = prevBuc;
	};
    }
    return nullptr;
};

//NOTE: these functions are for continuity
s8 none(BYTECODE_INPUT){return -1;};
s8 proc_gives(BYTECODE_INPUT){return -1;};
s8 proc_start(BYTECODE_INPUT){return -1;};
s8 proc_end(BYTECODE_INPUT){return -1;};
s8 block_start(BYTECODE_INPUT){return -1;};
s8 block_end(BYTECODE_INPUT){return -1;};
//TODO: 
s8 ret(BYTECODE_INPUT){return 0;};

#define BIN_OP_TEMPLATE(SIGN, TYPE)					\
    u16 dest = (u16)page[1];						\
    u16 lhs = (u16)page[2];						\
    u16 rhs = (u16)page[3];						\
    vm.registers[dest].TYPE = vm.registers[lhs].TYPE SIGN vm.registers[rhs].TYPE; \
    return 3;								\

#define BIN_DIV_TEMPLATE(TYPE)						\
    u16 dest = (u16)page[1];						\
    u16 lhs = (u16)page[2];						\
    u16 rhs = (u16)page[3];						\
    if(vm.registers[rhs].TYPE == 0){					\
	printf("TODO: div by 0 VM");					\
	return 0;							\
    }									\
    vm.registers[dest].TYPE = vm.registers[lhs].TYPE / vm.registers[rhs].TYPE; \
    return 3;								\

#define BIN_CMP_TEMPLATE(TYPE)						\
    u16 outputReg = (u16)page[1];					\
    u16 lhsReg = (u16)page[2];						\
    u16 rhsReg = (u16)page[3];						\
    vm.registers[outputReg].sint = vm.registers[lhsReg].TYPE - vm.registers[rhsReg].TYPE; \
    return 3;								\

#define SET_TEMPLATE(OP)						\
    u16 outputReg = (u16)page[1];					\
    u16 inputReg  = (u16)page[2];					\
    vm.registers[outputReg].uint = (vm.registers[inputReg].sint OP 0)?1:0; \
    return 2;								\

s8 label(BYTECODE_INPUT){
    return 1;
};
s8 cast(BYTECODE_INPUT){
    /*
       ot       nt
      float -> sint
      uint  -> sint
      sint  -> float
      uint  -> float
     */
    BytecodeType newType = typeToBytecodeType((Type)page[1]);
    u16 newReg = (u16)page[2];
    BytecodeType oldType = typeToBytecodeType((Type)page[3]);
    u16 oldReg = (u16)page[4];
    if(newType == oldType){
	vm.registers[newReg] = vm.registers[oldReg];
	return 6;
    };
    switch(newType){
    case BytecodeType::INTEGER_S:{
	switch(oldType){
	case BytecodeType::DECIMAL_S:
	    vm.registers[newReg].sint = (s64)vm.registers[oldReg].dec;
	    break;
	case BytecodeType::INTEGER_U:
	    vm.registers[newReg].sint = (s64)vm.registers[oldReg].uint;
	    break;
	};
    }break;
    case BytecodeType::DECIMAL_S:{
	switch(oldType){
	case BytecodeType::INTEGER_S:
	    vm.registers[newReg].dec = (f64)vm.registers[oldReg].sint;
	    break;
	case BytecodeType::INTEGER_U:
	    vm.registers[newReg].dec = (f64)vm.registers[oldReg].uint;
	    break;
	};
    }break;
    case BytecodeType::INTEGER_U:{
	switch(oldType){
	case BytecodeType::INTEGER_S:
	    vm.registers[newReg].uint = (f64)vm.registers[oldReg].sint;
	    break;
	case BytecodeType::DECIMAL_S:
	    vm.registers[newReg].uint = (f64)vm.registers[oldReg].dec;
	    break;
	};
    };
    };
    return 4;
};
s8 mov_consts(BYTECODE_INPUT){
    u16 dest = (u16)page[1];
    vm.registers[dest].sint = getConstInt(page + 2);
    return 1 + const_in_stream;
};
s8 mov_constf(BYTECODE_INPUT){
    u16 dest = (u16)page[1];
    vm.registers[dest].sint = getConstDec(page + 2);
    return 1 + const_in_stream;
};
s8 movs(BYTECODE_INPUT){
    u16 dest = (u16)page[1];
    u16 src = (u16)page[2];
    vm.registers[dest].sint = vm.registers[src].sint;
    return 2;
};
s8 movu(BYTECODE_INPUT){
    u16 dest = (u16)page[1];
    u16 src = (u16)page[2];
    vm.registers[dest].uint = vm.registers[src].uint;
    return 2;
};
s8 movf(BYTECODE_INPUT){
    u16 dest = (u16)page[1];
    u16 src = (u16)page[2];
    vm.registers[dest].dec = vm.registers[src].dec;
    return 2;
};
s8 addi(BYTECODE_INPUT){
    BIN_OP_TEMPLATE(+, sint);
};
s8 addu(BYTECODE_INPUT){
    BIN_OP_TEMPLATE(+, uint);
};
s8 addf(BYTECODE_INPUT){
    BIN_OP_TEMPLATE(+, dec);
};
s8 subi(BYTECODE_INPUT){
    BIN_OP_TEMPLATE(-, sint);
};
s8 subu(BYTECODE_INPUT){
    BIN_OP_TEMPLATE(-, uint);
};
s8 subf(BYTECODE_INPUT){
    BIN_OP_TEMPLATE(-, dec);
};
s8 muli(BYTECODE_INPUT){
    BIN_OP_TEMPLATE(*, sint);
};
s8 mulu(BYTECODE_INPUT){
    BIN_OP_TEMPLATE(*, uint);
};
s8 mulf(BYTECODE_INPUT){
    BIN_OP_TEMPLATE(*, dec);
};
s8 divi(BYTECODE_INPUT){
    BIN_DIV_TEMPLATE(sint);
};
s8 divu(BYTECODE_INPUT){
    BIN_DIV_TEMPLATE(uint);
};
s8 divf(BYTECODE_INPUT){
    BIN_DIV_TEMPLATE(dec);
};
s8 cmps(BYTECODE_INPUT){
    BIN_CMP_TEMPLATE(sint);
};
s8 cmpu(BYTECODE_INPUT){
    BIN_CMP_TEMPLATE(uint);
};
s8 cmpf(BYTECODE_INPUT){
    BIN_CMP_TEMPLATE(dec);
};
s8 setg(BYTECODE_INPUT){
    SET_TEMPLATE(>);
};
s8 setl(BYTECODE_INPUT){
    SET_TEMPLATE(<);
};
s8 sete(BYTECODE_INPUT){
    SET_TEMPLATE(==);
};
s8 setge(BYTECODE_INPUT){
    SET_TEMPLATE(>=);
};
s8 setle(BYTECODE_INPUT){
    SET_TEMPLATE(<=);
};
s8 jmpns(BYTECODE_INPUT){
    u16 reg = (u16)page[1];
    if(vm.registers[reg].uint == 0){
	u16 label = (u16)page[2];
	Bytecode *add = vm.labels->getElement(label);
	BytecodeBucket *buc = getBucketWithInsideAdd(vm.buc, add);
	vm.buc = buc;
	vm.cursor = add - 1; //execBytecodes increments it by 1
	return 0;
    };
    return 2;
};
s8 jmps(BYTECODE_INPUT){
    u16 reg = (u16)page[1];
    if(vm.registers[reg].uint != 0){
        u16 label = (u16)page[2];
	Bytecode *add = vm.labels->getElement(label);
	BytecodeBucket *buc = getBucketWithInsideAdd(vm.buc, add);
	vm.buc = buc;
	vm.cursor = add - 1; //execBytecodes increments it by 1
	return 0;
    };
    return 2;
};
s8 jmp(BYTECODE_INPUT){
    u16 label = (u16)page[1];
    Bytecode *add = vm.labels->getElement(label);
    BytecodeBucket *buc = getBucketWithInsideAdd(vm.buc, add);
    vm.buc = buc;
    vm.cursor = add - 1; //execBytecodes increments it by 1
    return 0;
};
s8 def(BYTECODE_INPUT){
    u32 x = 2;
    //@incomplete: SIMD?
    while(page[x] != Bytecode::PROC_END){
	x += 1;
	if(page[x] == Bytecode::NEXT_BUCKET){
	    vm.buc = vm.buc->next;
	    vm.cursor = vm.buc->bytecodes;
	    
	    page = vm.cursor;
	    x = 0;
	};
    };
    vm.procs.push(page+1);
    return x;
};
s8 neg(BYTECODE_INPUT){
    u16 destReg = (u16)page[1];
    BytecodeType bt = typeToBytecodeType((Type)page[2]);
    u16 srcReg = (u16)page[3];
    switch(bt){
    case BytecodeType::INTEGER_S:
	vm.registers[destReg].sint = vm.registers[srcReg].sint * -1;
	break;
    case BytecodeType::INTEGER_U:
	vm.registers[destReg].sint = vm.registers[srcReg].uint * -1;
	break;
    case BytecodeType::DECIMAL_S:
	vm.registers[destReg].dec  = vm.registers[srcReg].dec * -1;
	break;
    };
    return 3;
};
s8 next_bucket(BYTECODE_INPUT){
    vm.buc = vm.buc->next;
    if(vm.buc == nullptr){return 0;};
    vm.cursor = vm.buc->bytecodes - 1; //execBytecodes increments it by 1
    return 0;
};
s8 decl_reg(BYTECODE_INPUT){
    return 2;
};
s8 alloc(BYTECODE_INPUT){
    u16 reg = (u16)page[1];
    s64 size = getConstInt(page + 2);
    vm.registers[reg].ptr = &vm.stack[vm.stackOff];
    vm.stackOff += size;
    return 1+const_in_stream;
};

/*
  NEG TYPE 1 REG 1
   0   1   2  3  4     (offsets which the byteProc will use)

   NEG will return 4
*/
//TODO: change i -> s
s8 (*byteProc[])(BYTECODE_INPUT) = {
    none,
    cast,
    movs,
    movu,
    movf,
    mov_consts,
    mov_constf,
    addi,
    addu,
    addf,
    subi,
    subu,
    subf,
    muli,
    mulu,
    mulf,
    divi,
    divu,
    divf,
    cmps,
    cmpu,
    cmpf,
    setg,
    setl,
    sete,
    setge,
    setle,
    jmpns,
    jmps,
    jmp,
    def,
    proc_gives, proc_start, proc_end,
    ret,
    neg,
    next_bucket,
    label,
    decl_reg,
    block_start,
    block_end,
    alloc,
};

bool execBytecode(VM &vm){
    BytecodeBucket *buc = vm.buc;
    while(true){
	Bytecode bc = *vm.cursor;
	ASSERT(bc < Bytecode::COUNT);
	if(bc == Bytecode::NONE){return true;};
	s8 x = byteProc[(u16)bc](vm.cursor, vm);
	if(x == -1){return false;};
	vm.cursor += x + 1;
    };
    return true;
};

#if(DBG)
static_assert((u16)Bytecode::COUNT == ARRAY_LENGTH(byteProc), "Bytecode and byteProc not one-to-one");
#endif
