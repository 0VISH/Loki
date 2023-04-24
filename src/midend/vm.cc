struct Register{
    union{
	s64 sint;
	u64 uint;
	f64 dec;
    };
};
struct VM{
    Register *registers;
};

VM createVM(){
    VM vm;
    vm.registers = (Register*)mem::alloc(sizeof(Register) * REGISTER_COUNT);
    return vm;
};
void destroyVM(VM &vm){
    mem::free(vm.registers);
};

//NOTE: these functions are for continuity
s8 none(Bytecode *page, Type *regTypes, VM &vm){return 0;};
s8 next_page(Bytecode *page, Type *regTypes, VM &vm){return 0;};
s8 reg(Bytecode *page, Type *regTypes, VM &vm){return 0;};
s8 const_ints(Bytecode *page, Type *regTypes, VM &vm){return 0;};
s8 const_intu(Bytecode *page, Type *regTypes, VM &vm){return 0;};
s8 const_dec(Bytecode *page, Type *regTypes, VM &vm){return 0;};

s8 cast(Bytecode *page, Type *regTypes, VM &vm){
    /*
       ot       nt
      float -> sint
      uint  -> sint
      sint  -> float
      uint  -> float
     */
    u32 newReg = (u16)page[2];
    u32 oldReg = (u16)page[4];
    BytecodeType newType = typeToBytecodeType(regTypes[newReg]);
    BytecodeType oldType = typeToBytecodeType(regTypes[oldReg]);
    switch(newType){
    case BytecodeType::INTEGER_S:{
	switch(oldType){
	case BytecodeType::DECIMAL:
	    vm.registers[newReg].sint = (s64)vm.registers[oldReg].dec;
	    break;
	case BytecodeType::INTEGER_U:
	    vm.registers[newReg].sint = (s64)vm.registers[oldReg].uint;
	    break;
	};
    }break;
    case BytecodeType::DECIMAL:{
	switch(oldType){
	case BytecodeType::INTEGER_S:
	    vm.registers[newReg].dec = (f64)vm.registers[oldReg].sint;
	    break;
	case BytecodeType::INTEGER_U:
	    vm.registers[newReg].dec = (f64)vm.registers[oldReg].uint;
	    break;
	};
    }break;
    };
    return 4;
};
s8 movi(Bytecode *page, Type *regTypes, VM &vm){
    u32 dest = (u16)page[2];
    s8 x = 2;
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
s8 movu(Bytecode *page, Type *regTypes, VM &vm){
    u32 dest = (u16)page[2];
    s8 x = 2;
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
s8 movf(Bytecode *page, Type *regTypes, VM &vm){
    u32 dest = (u16)page[2];
    s8 x = 2;
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
s8 addi(Bytecode *page, Type *regTypes, VM &vm){
    u32 dest = (u16)page[2];
    u32 lhs = (u16)page[4];
    u32 rhs = (u16)page[6];
    vm.registers[dest].sint = vm.registers[lhs].sint + vm.registers[rhs].sint;
    return 6;
};
s8 addu(Bytecode *page, Type *regTypes, VM &vm){
    u32 dest = (u16)page[2];
    u32 lhs = (u16)page[4];
    u32 rhs = (u16)page[6];
    vm.registers[dest].uint = vm.registers[lhs].uint + vm.registers[rhs].uint;
    return 6;
};
s8 addf(Bytecode *page, Type *regTypes, VM &vm){
    u32 dest = (u16)page[2];
    u32 lhs = (u16)page[4];
    u32 rhs = (u16)page[6];
    vm.registers[dest].dec = vm.registers[lhs].dec + vm.registers[rhs].dec;
    return 6;
};

s8 (*byteProc[])(Bytecode *page, Type *regTypes, VM &vm) = {
    none, next_page, reg,
    cast,
    const_ints, const_intu, const_dec,
    movi,
    movu,
    movf,
    addi,
    addu,
    addf,
};

bool execBytecode(BytecodeFile &bf, u32 pageOff, u32 curOff, u32 endOff, Type *regTypes, VM &vm){
    Bytecode *page = bf.bytecodePages[pageOff];
    u32 off = curOff;
    while(off != endOff){
	switch(page[curOff]){
	case Bytecode::NONE: return true;
	case Bytecode::NEXT_PAGE:{
	    off += 1;
	    pageOff += 1;
	    page = bf.bytecodePages[pageOff];
	    curOff = 0;
	    continue;
	}break;
	};
	u32 x = byteProc[(u16)page[curOff]](page+curOff, regTypes, vm) + 1;
	curOff += x;
	off += x;
    };
    return true;
};
