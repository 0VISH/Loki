#define BYTECODE_INPUT Bytecode *page, VM &vm, ExecContext &execContext

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
};
struct ExecContext{
    Bytecode *page;
    u32 off;

    void init(BytecodeFile &bf, u32 o = 0){
	page = bf.bcs.mem;
	off = o;
    }
};

VM createVM(){
    VM vm;
    vm.registers = (Register*)mem::alloc(sizeof(Register) * REGISTER_COUNT);
    vm.procs.init(3);
    return vm;
};
void destroyVM(VM &vm){
    mem::free(vm.registers);
    vm.procs.uninit();
};

//NOTE: these functions are for continuity
s8 none(BYTECODE_INPUT){return 0;};
s8 reg(BYTECODE_INPUT){return 0;};
s8 global(BYTECODE_INPUT){return 0;};
s8 type(BYTECODE_INPUT){return 0;};
s8 const_ints(BYTECODE_INPUT){return 0;};
s8 const_intu(BYTECODE_INPUT){return 0;};
s8 const_dec(BYTECODE_INPUT){return 0;};
s8 proc_gives(BYTECODE_INPUT){return 0;};
s8 proc_start(BYTECODE_INPUT){return 0;};
s8 proc_end(BYTECODE_INPUT){return 0;};

#define BIN_OP_TEMPLATE(SIGN, TYPE)						\
    u32 dest = (u16)page[2];						\
    u32 lhs = (u16)page[4];						\
    u32 rhs = (u16)page[6];						\
    vm.registers[dest].TYPE = vm.registers[lhs].TYPE SIGN vm.registers[rhs].TYPE; \
    return 6;								\

#define BIN_DIV_TEMPLATE(TYPE)						\
    u32 dest = (u16)page[2];						\
    u32 lhs = (u16)page[4];						\
    u32 rhs = (u16)page[6];						\
    if(vm.registers[rhs].TYPE == 0){					\
	printf("TODO: div by 0 VM");					\
	return 0;							\
    }									\
    vm.registers[dest].TYPE = vm.registers[lhs].TYPE / vm.registers[rhs].TYPE; \
    return 6;								\

//TODO: 
s8 ret(BYTECODE_INPUT){return 0;};

s8 cast(BYTECODE_INPUT){
    /*
       ot       nt
      float -> sint
      uint  -> sint
      sint  -> float
      uint  -> float
     */
    u32 newReg = (u16)page[4];
    u32 oldReg = (u16)page[8];
    BytecodeType newType = typeToBytecodeType((Type)page[2]);
    BytecodeType oldType = typeToBytecodeType((Type)page[6]);
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
    u32 dest = (u16)page[2];
    s8 x = reg_in_stream;
    switch(page[3]){
    case Bytecode::REG:{
	u32 src = (u16)page[4];
	vm.registers[dest].sint = vm.registers[src].sint;
	x += reg_in_stream;
    }break;
    case Bytecode::CONST_INTS:{
	vm.registers[dest].sint = getConstInt(page + 4);
	x += const_in_stream + 1;
    }break;
    };
    return x;
};
s8 movu(BYTECODE_INPUT){
    u32 dest = (u16)page[2];
    s8 x = reg_in_stream;
    switch(page[3]){
    case Bytecode::REG:{
	u32 src = (u16)page[4];
	vm.registers[dest].sint = vm.registers[src].sint;
	x += reg_in_stream;
    }break;
    case Bytecode::CONST_INTU:{
	vm.registers[dest].uint = getConstInt(page + 4);
	x += const_in_stream + 1;
    }break;
    };
    return x;
};
s8 movf(BYTECODE_INPUT){
    u32 dest = (u16)page[2];
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
s8 def(BYTECODE_INPUT){
    u32 x = 2;
    u32 count = 0;
    while(page[x] != Bytecode::PROC_END){x += 1;};
    x -= 2;
    count = x;
    //TODO: handle next page
    Bytecode *proc = (Bytecode*)mem::alloc(sizeof(Bytecode) * count);  //TODO: change allocator?
    memcpy(proc, page+2, sizeof(Bytecode)*count);
    proc[x] = Bytecode::NONE;
    return count+2;
};
s8 neg(BYTECODE_INPUT){
    BytecodeType bt = typeToBytecodeType((Type)page[1]);
    u32 reg = (u32)page[3];
    switch(bt){
    case BytecodeType::INTEGER_S: vm.registers[reg].sint *= -1; break;
    case BytecodeType::INTEGER_U: vm.registers[reg].uint *= -1; break;
    case BytecodeType::DECIMAL_S: vm.registers[reg].dec  *= -1; break;
    };
    return 4;
};

s8 (*byteProc[])(BYTECODE_INPUT) = {
    none, reg, global,
    cast,
    type, const_ints, const_intu, const_dec,
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
    def,
    proc_gives, proc_start, proc_end,
    ret,
    neg,
};

bool execBytecode(ExecContext &execContext, u32 endOff, VM &vm){
    while(execContext.off != endOff){
	Bytecode bc = execContext.page[execContext.off];
	if(bc == Bytecode::NONE){return true;};
	u32 x = byteProc[(u16)bc](execContext.page+execContext.off, vm, execContext) + 1;
	if(x == 0){return false;};
	execContext.off += x;
    };
    return true;
};

#if(XE_DBG)
static_assert((u16)Bytecode::BYTECODE_COUNT == ARRAY_LENGTH(byteProc), "Bytecode and byteProc not one-to-one");
#endif
