#define BYTECODE_INPUT Bytecode *page, VM &vm

struct Register{
    union{
	s64 sint;
	u64 uint;
	f64 dec;
    };
};
struct VM{
    DynamicArray<Bytecode*> procs;
    DynamicArray<u32>      *labels;
    Register *registers;
    Bytecode *page;
    u32 off;

    void init(BytecodeFile &bf, u32 offset){
	page = bf.bcs.mem;
	off = offset;
	registers = (Register*)mem::alloc(sizeof(Register) * REGISTER_COUNT);
	labels = &bf.labels;
	procs.init(3);
    };
    void uninit(){
	mem::free(registers);
	for(u32 x=0; x<procs.count; x+=1){
	    mem::free(procs[x]);
	};
	procs.uninit();
    };
};

//NOTE: these functions are for continuity
s8 none(BYTECODE_INPUT){return -1;};
s8 const_int(BYTECODE_INPUT){return 0;};
s8 const_dec(BYTECODE_INPUT){return 0;};
s8 proc_gives(BYTECODE_INPUT){return 0;};
s8 proc_start(BYTECODE_INPUT){return 0;};
s8 proc_end(BYTECODE_INPUT){return 0;};
s8 label(BYTECODE_INPUT){return 1;};
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
	u16 off = (u16)page[2];
	vm.off = vm.labels->getElement(off) - 1;
	return 0;
    };
    return 2;
};
s8 jmp(BYTECODE_INPUT){
    u16 off = (u16)page[1];
    vm.off = vm.labels->getElement(off) - 1;
    return 0;
};
s8 def(BYTECODE_INPUT){
    u32 x = 2;
    while(page[x] != Bytecode::PROC_END){x += 1;};
    u32 count = x - 2;
    Bytecode *proc = (Bytecode*)mem::alloc(sizeof(Bytecode) * count + 1);  //TODO: change allocator?
    memcpy(proc, page+2, sizeof(Bytecode)*count);
    proc[count] = Bytecode::NONE;
    vm.procs.push(proc);
    return x;
};
s8 neg(BYTECODE_INPUT){
    BytecodeType bt = typeToBytecodeType((Type)page[1]);
    u16 reg = (u32)page[2];
    switch(bt){
    case BytecodeType::INTEGER_S: vm.registers[reg].sint *= -1; break;
    case BytecodeType::INTEGER_U:
	vm.registers[reg].sint = vm.registers[reg].uint * -1;
	break;
    case BytecodeType::DECIMAL_S: vm.registers[reg].dec  *= -1; break;
    };
    return 2;
};

/*
  NEG TYPE 1 REG 1
   0   1   2  3  4     (offsets which the byteProc will use)

   NEG will return 4
*/
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
    jmp,
    def,
    proc_gives, proc_start, proc_end,
    ret,
    neg,
    label,
};

bool execBytecode(u32 endOff, VM &vm){
    while(vm.off != endOff){
	Bytecode bc = vm.page[vm.off];
	if(bc == Bytecode::NONE){return true;};
	s8 x = byteProc[(u16)bc](vm.page+vm.off, vm) + 1;
	if(x == -1){return false;};
	vm.off += x;
    };
    return true;
};

#if(XE_DBG)
static_assert((u16)Bytecode::BYTECODE_COUNT == ARRAY_LENGTH(byteProc), "Bytecode and byteProc not one-to-one");
#endif
