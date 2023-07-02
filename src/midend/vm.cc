#define BYTECODE_INPUT Bytecode *page, VM &vm

struct Register{
    union{
	s64 sint;
	u64 uint;
	f64 dec;
    };
};
struct VM{
    Register *registers;
    DynamicArray<Bytecode*> procs;
    Bytecode *page;
    u32 off;

    void init(BytecodeFile &bf, u32 offset){
	page = bf.bcs.mem;
	off = offset;
	registers = (Register*)mem::alloc(sizeof(Register) * REGISTER_COUNT);
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
s8 none(BYTECODE_INPUT){return 0;};
s8 reg(BYTECODE_INPUT){return 0;};
s8 global(BYTECODE_INPUT){return 0;};
s8 type(BYTECODE_INPUT){return 0;};
s8 const_int(BYTECODE_INPUT){return 0;};
s8 const_dec(BYTECODE_INPUT){return 0;};
s8 proc_gives(BYTECODE_INPUT){return 0;};
s8 proc_start(BYTECODE_INPUT){return 0;};
s8 proc_end(BYTECODE_INPUT){return 0;};
s8 label(BYTECODE_INPUT){return 0;};
//TODO: 
s8 ret(BYTECODE_INPUT){return 0;};

#define BIN_OP_TEMPLATE(SIGN, TYPE)						\
    u16 dest = (u16)page[2];						\
    u16 lhs = (u16)page[4];						\
    u16 rhs = (u16)page[6];						\
    vm.registers[dest].TYPE = vm.registers[lhs].TYPE SIGN vm.registers[rhs].TYPE; \
    return 6;								\

#define BIN_DIV_TEMPLATE(TYPE)						\
    u16 dest = (u16)page[2];						\
    u16 lhs = (u16)page[4];						\
    u16 rhs = (u16)page[6];						\
    if(vm.registers[rhs].TYPE == 0){					\
	printf("TODO: div by 0 VM");					\
	return 0;							\
    }									\
    vm.registers[dest].TYPE = vm.registers[lhs].TYPE / vm.registers[rhs].TYPE; \
    return 6;								\

#define BIN_CMP_TEMPLATE(TYPE)						\
    u16 outputReg = (u16)page[2];					\
    u16 lhsReg = (u16)page[4];						\
    u16 rhsReg = (u16)page[6];						\
    vm.registers[outputReg].sint = vm.registers[lhsReg].TYPE - vm.registers[rhsReg].TYPE; \
    return 6;								\

#define SET_TEMPLATE(OP)						\
    u16 outputReg = (u16)page[2];					\
    u16 inputReg  = (u16)page[4];					\
    vm.registers[outputReg].uint = (vm.registers[inputReg].sint OP 0)?1:0; \
    return 4;								\

s8 cast(BYTECODE_INPUT){
    /*
       ot       nt
      float -> sint
      uint  -> sint
      sint  -> float
      uint  -> float
     */
    u16 newReg = (u16)page[4];
    u16 oldReg = (u16)page[8];
    BytecodeType newType = typeToBytecodeType((Type)page[2]);
    BytecodeType oldType = typeToBytecodeType((Type)page[6]);
    if(newType == oldType){
	vm.registers[newReg] = vm.registers[oldReg];
	return 8;
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
    return 8;
};
s8 movi(BYTECODE_INPUT){
    u16 dest = (u16)page[2];
    s8 x = reg_in_stream;
    switch(page[3]){
    case Bytecode::REG:{
	u16 src = (u16)page[4];
	vm.registers[dest].sint = vm.registers[src].sint;
	x += reg_in_stream;
    }break;
    case Bytecode::CONST_INT:{
	vm.registers[dest].sint = getConstInt(page + 4);
	x += const_in_stream + 1;
    }break;
    };
    return x;
};
s8 movu(BYTECODE_INPUT){
    u16 dest = (u16)page[2];
    s8 x = reg_in_stream;
    switch(page[3]){
    case Bytecode::REG:{
	u16 src = (u16)page[4];
	vm.registers[dest].sint = vm.registers[src].sint;
	x += reg_in_stream;
    }break;
    case Bytecode::CONST_INT:{
	vm.registers[dest].uint = getConstInt(page + 4);
	x += const_in_stream + 1;
    }break;
    };
    return x;
};
s8 movf(BYTECODE_INPUT){
    u16 dest = (u16)page[2];
    s8 x = reg_in_stream;
    switch(page[3]){
    case Bytecode::REG:{
	u32 src = (u16)page[4];
	vm.registers[dest].dec = vm.registers[src].dec;
	x += reg_in_stream;
    }break;
    case Bytecode::CONST_DEC:{
	vm.registers[dest].dec = getConstDec(page + 4);
	x += const_in_stream+1;
    }break;
    };
    return x;
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
    u16 reg = (u16)page[2];
    if(vm.registers[reg].uint == 0){
	vm.off += (u16)page[3];
    };
    return 3;
};
s8 jmp(BYTECODE_INPUT){
    return 0;
};
s8 def(BYTECODE_INPUT){
    u32 x = 2;
    u32 count = 0;
    while(page[x] != Bytecode::PROC_END){x += 1;};
    x -= 2;
    count = x;
    Bytecode *proc = (Bytecode*)mem::alloc(sizeof(Bytecode) * count);  //TODO: change allocator?
    memcpy(proc, page+2, sizeof(Bytecode)*count);
    proc[x] = Bytecode::NONE;
    vm.procs.push(proc);
    return count+2;
};
s8 neg(BYTECODE_INPUT){
    BytecodeType bt = typeToBytecodeType((Type)page[2]);
    u16 reg = (u32)page[4];
    switch(bt){
    case BytecodeType::INTEGER_S: vm.registers[reg].sint *= -1; break;
    case BytecodeType::INTEGER_U:
	vm.registers[reg].sint = vm.registers[reg].uint * -1;
	break;
    case BytecodeType::DECIMAL_S: vm.registers[reg].dec  *= -1; break;
    };
    return 4;
};

/*
  NEG TYPE 1 REG 1
   0   1   2  3  4     (offsets which the byteProc will use)

   NEG will return 4
*/
s8 (*byteProc[])(BYTECODE_INPUT) = {
    none, reg, global,
    cast,
    type, const_int, const_dec,
    movi,
    movu,
    movf,
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
	if(x == 0){return false;};
	vm.off += x;
    };
    return true;
};

#if(XE_DBG)
static_assert((u16)Bytecode::BYTECODE_COUNT == ARRAY_LENGTH(byteProc), "Bytecode and byteProc not one-to-one");
#endif
