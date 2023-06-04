#define BYTECODE_PAGE_COUNT 100
#define REGISTER_COUNT      100

/*
  %d  register
  @d  global
  _d  proc
*/
enum class Bytecode : u16{
    NONE = 0,
    REG,
    GLOBAL,
    CAST,
    TYPE,
    CONST_INTS,
    CONST_INTU,
    CONST_DEC,
    MOVS,
    MOVU,
    MOVF,
    ADDS,
    ADDU,
    ADDF,
    SUBS,
    SUBU,
    SUBF,
    MULS,
    MULU,
    MULF,
    DIVS,
    DIVU,
    DIVF,
    DEF,
    PROC_GIVES,
    PROC_START,
    PROC_END,
    RET,
    NEG_S,
    NEG_U,
    NEG_D,
    BYTECODE_COUNT,
};
enum class BytecodeType : u16{
    INTEGER_S,
    INTEGER_U,
    DECIMAL,
};

const u16 const_in_stream = (sizeof(s64) / sizeof(Bytecode));
const u16 reg_in_stream = 2;
const u16 type_in_stream = 2;

struct BytecodeFile{
    DynamicArray<Bytecode> bcs;

    void init(){
	bcs.init(50);
    };
    void uninit(){
	bcs.uninit();
    };
    void emit(Bytecode bc){
	bcs.push(bc);
    };
    void emitReg(u32 regID){
	emit(Bytecode::REG);
	emit((Bytecode)regID);
    };
    void emitGlobal(u32 globID){
	emit(Bytecode::GLOBAL);
	emit((Bytecode)globID);
    };
    void emitType(Type type){
	emit(Bytecode::TYPE);
	emit((Bytecode)type);
    };
    //encoding constant into bytecode page for cache
    void emitConstInt(s64 num){
	bcs.reserve(const_in_stream);
	Bytecode *loc = bcs.mem + bcs.count;
	s64 *mem = (s64*)(loc);
	*mem = num;
        bcs.count += const_in_stream;
    };
    void emitConstDec(f64 num){
        bcs.reserve(const_in_stream);
	Bytecode *loc = bcs.mem + bcs.count;
	f64 *mem = (f64*)(loc);
	*mem = num;
	bcs.count += const_in_stream;
    };
};

static u32 procID = 0;

struct BytecodeContext{
    Map procToID;
    u32 *varToReg;
    Type *types;
    u32 registerID;

    void init(u32 varCount, u32 procCount){
	registerID = 0;
	if(varCount != 0){
	    varToReg = (u32*)mem::alloc(sizeof(u32)*varCount);
	    types = (Type*)mem::alloc(sizeof(Type)*REGISTER_COUNT);
	};
	procToID.len = 0;
	if(procCount != 0){
	    procToID.init(procCount);
	};
    };
    void uninit(){
	mem::free(varToReg);
	if(procToID.len != 0){procToID.uninit();};
	mem::free(types);
    };
    u32 newReg(Type type){
	u32 reg = registerID;
	registerID += 1;
	types[reg] = type;
	return reg;
    };
};

#define EMIT_BIN_OP_BC_TEMPLATE(SIGNED, UNSIGNED, DECIMAL)		\
    ASTBinOp *op = (ASTBinOp*)node;					\
    Type lhsType = op->lhsType;						\
    Type rhsType = op->rhsType;						\
    Type ansType = greaterType(lhsType, rhsType);			\
    BytecodeType lbt = typeToBytecodeType(lhsType);			\
    BytecodeType rbt = typeToBytecodeType(rhsType);			\
    BytecodeType abt = typeToBytecodeType(ansType);			\
    u32 lhsReg = bc.newReg(lhsType);					\
    u32 rhsReg = bc.newReg(rhsType);					\
    compileExprToBytecode(lhsReg, op->lhs, lexer, see, bca, bf, isExprU); \
    compileExprToBytecode(rhsReg, op->rhs, lexer, see, bca, bf, isExprU); \
    if(lbt != rbt){							\
	u32 newReg = bc.newReg(ansType);				\
	bf.emit(Bytecode::CAST);					\
	bf.emitType(ansType);						\
	bf.emitReg(newReg);						\
	if(lbt != abt){							\
	    bf.emitType(lhsType);					\
	    bf.emitReg(lhsReg);						\
	    lhsReg = newReg;						\
	}else{								\
	    bf.emitType(rhsType);					\
	    bf.emitReg(rhsReg);						\
	    rhsReg = newReg;						\
	};								\
    };									\
    BytecodeType type = typeToBytecodeType(ansType);			\
    if(isExprU){bf.emit(UNSIGNED);}					\
    else if(type == BytecodeType::INTEGER_S){bf.emit(SIGNED);}		\
    else{bf.emit(DECIMAL);};						\
    bf.emitReg(outputRegister);						\
    bf.emitReg(lhsReg);							\
    bf.emitReg(rhsReg);							\

s64 getConstInt(Bytecode *bytes){
    s64 *mem = (s64*)bytes;
    return *mem;
};
f64 getConstDec(Bytecode *bytes){
    f64 *mem = (f64*)bytes;
    return *mem;
};
BytecodeType typeToBytecodeType(Type type){
    switch(type){
    case Type::F_64:
    case Type::F_32:
    case Type::F_16:
    case Type::COMP_DECIMAL: return BytecodeType::DECIMAL;
    case Type::S_64:
    case Type::S_32:
    case Type::S_16:
    case Type::COMP_INTEGER: return BytecodeType::INTEGER_S;
    default: return BytecodeType::INTEGER_U;
    }
};
void compileExprToBytecode(u32 outputRegister, ASTBase *node, Lexer &lexer, DynamicArray<ScopeEntities*> &see, DynamicArray<BytecodeContext> &bca, BytecodeFile &bf, bool isExprU=false){
    ASTType type = node->type;
    BytecodeContext &bc = bca[bca.count - 1];
    switch(type){
    case ASTType::NUM_INTEGER:{
	ASTNumInt *numInt = (ASTNumInt*)node;
	String str = makeStringFromTokOff(numInt->tokenOff, lexer);
	s64 num = string2int(str);    //TODO: maybe have a sep func which returns u64
	if(isExprU){bf.emit(Bytecode::MOVU);}
	else{bf.emit(Bytecode::MOVS);};
	bf.emitReg(outputRegister);
	if(isExprU){bf.emit(Bytecode::CONST_INTU);}
	else{bf.emit(Bytecode::CONST_INTS);};
	bf.emitConstInt(num);
    }break;
    case ASTType::NUM_DECIMAL:{
	ASTNumDec *numDec = (ASTNumDec*)node;
	String str = makeStringFromTokOff(numDec->tokenOff, lexer);
	f64 num = string2float(str);
	bf.emit(Bytecode::MOVF);
	bf.emitReg(outputRegister);
	bf.emit(Bytecode::CONST_DEC);
	bf.emitConstDec(num);
    }break;
    case ASTType::VARIABLE:{
	ASTVariable *var = (ASTVariable*)node;
	Type type;
	bool isGlobal = false;
	u32 off = see.count-1;
	ScopeEntities *se = see[off];
	s32 id = se->varMap.getValue(var->name);
	if(id == -1){
	    off = 0;
	    se = see[0];
	    id = se->varMap.getValue(var->name);
	    isGlobal = true;
	};
	const VariableEntity &entity = se->varEntities[id];
	type = entity.type;
	//isGlobal = IS_BIT(entity.flag, Flags::GLOBAL) != 0;
	BytecodeType bytecodeType = typeToBytecodeType(type);
	if(isExprU){bf.emit(Bytecode::MOVU);}
	else if(bytecodeType == BytecodeType::INTEGER_S){bf.emit(Bytecode::MOVS);}
	else{bf.emit(Bytecode::MOVU);};
	BytecodeContext &correctBC = bca[off];
	bf.emitReg(outputRegister);
	if(isGlobal){bf.emitGlobal(correctBC.varToReg[id]);}
	else{bf.emitReg(correctBC.varToReg[id]);};
    }break;
    case ASTType::BIN_ADD:{
	EMIT_BIN_OP_BC_TEMPLATE(Bytecode::ADDS, Bytecode::ADDU, Bytecode::ADDF);
    }break;
    case ASTType::BIN_MUL:{
	EMIT_BIN_OP_BC_TEMPLATE(Bytecode::MULS, Bytecode::MULU, Bytecode::MULF);
    }break;
    case ASTType::BIN_DIV:{
	EMIT_BIN_OP_BC_TEMPLATE(Bytecode::DIVS, Bytecode::DIVU, Bytecode::DIVF);
    }break;
    case ASTType::UNI_NEG:{
	ASTUniOp *uniOp = (ASTUniOp*)node;
	compileExprToBytecode(outputRegister, uniOp->node, lexer, see, bca, bf, isExprU);
	BytecodeType type = typeToBytecodeType(bc.types[outputRegister]);
	switch(type){
	case BytecodeType::INTEGER_S: bf.emit(Bytecode::NEG_S); break;
	case BytecodeType::INTEGER_U:
	    bf.emit(Bytecode::NEG_U);
	    bc.types[outputRegister] = Type::S_64;
	    break;
	case BytecodeType::DECIMAL:   bf.emit(Bytecode::NEG_D); break;
	};
	bf.emitReg(outputRegister);
    }break;
    default:
	DEBUG_UNREACHABLE;
	break;
    };
};
void compileToBytecode(ASTBase *node, Lexer &lexer, DynamicArray<ScopeEntities*> &see, DynamicArray<BytecodeContext> &bca, BytecodeFile &bf){
    BytecodeContext &bc = bca[bca.count-1];
    ScopeEntities *se = see[see.count-1];
    ASTType type = node->type;
    switch(type){
    case ASTType::UNI_ASSIGNMENT_T_KNOWN:
    case ASTType::UNI_ASSIGNMENT_T_UNKNOWN:{
	ASTUniVar *var = (ASTUniVar*)node;
	u32 id = se->varMap.getValue(var->name);
	const VariableEntity &entity = se->varEntities[id];
	u32 regID = bc.newReg(entity.type);
	u32 ansReg = bc.newReg(entity.type);
	compileExprToBytecode(ansReg, var->rhs, lexer, see, bca, bf, typeToBytecodeType(entity.type)==BytecodeType::INTEGER_U);
	bc.varToReg[id] = regID;
	BytecodeType type = typeToBytecodeType(entity.type);
	if(type == BytecodeType::DECIMAL){bf.emit(Bytecode::MOVF);}
	else if(type == BytecodeType::INTEGER_S){bf.emit(Bytecode::MOVS);}
	else{bf.emit(Bytecode::MOVU);};
	if(IS_BIT(entity.flag, Flags::GLOBAL)){bf.emitGlobal(regID);}
	else{bf.emitReg(regID);};
	bf.emitReg(ansReg);
    }break;
    case ASTType::MULTI_ASSIGNMENT_T_KNOWN:
    case ASTType::MULTI_ASSIGNMENT_T_UNKNOWN:{
	ASTMultiVar *var = (ASTMultiVar*)node;
	DynamicArray<String> &names = var->names;
	u32 firstID = se->varMap.getValue(names[0]);
	const VariableEntity &firstEntityID = se->varEntities[firstID];
	Type firstEntityType = firstEntityID.type;
	u32 ansReg = bc.newReg(firstEntityType);
	compileExprToBytecode(ansReg, var->rhs, lexer, see, bca, bf, typeToBytecodeType(firstEntityType)==BytecodeType::INTEGER_U);
	Bytecode byte;
	BytecodeType type = typeToBytecodeType(firstEntityType);
	if(type == BytecodeType::DECIMAL){byte = Bytecode::MOVF;}
	else if(type == BytecodeType::INTEGER_S){byte = Bytecode::MOVS;}
	else{byte = Bytecode::MOVU;};
	Bytecode globalOrReg;
	if(IS_BIT(firstEntityID.flag, Flags::GLOBAL)){globalOrReg = Bytecode::GLOBAL;}
	else{globalOrReg=Bytecode::REG;};
	for(u32 x=0; x<names.count; x+=1){
	    u32 id = se->varMap.getValue(names[x]);
	    const VariableEntity &entity = se->varEntities[id];
	    u32 regID = bc.newReg(firstEntityType);
	    bc.varToReg[id] = regID;
	    bf.emit(byte);
	    bf.emit(globalOrReg);
	    bf.emit((Bytecode)regID);
	    bf.emitReg(ansReg);
	};
    }break;
    case ASTType::PROC_DEFENITION:{
	ASTProcDef *proc = (ASTProcDef*)node;
	u32 procBytecodeID = procID;
	bc.procToID.insertValue(proc->name, procBytecodeID);
	procID += 1;
	u32 procID = se->procMap.getValue(proc->name);
	ScopeEntities *procSE = se->procEntities[procID].se;
	BytecodeContext &procBC = bca.newElem();
        procBC.init(procSE->varMap.count, procSE->procMap.count);
	// DEF + proc_id + in + PROC_GIVES + out + PROC_START
	u32 reserveCount = 1 + 1 + (proc->inCommaCount * (2 + 2)) + 1 + (proc->out.count * (2 + 2)) + 1;
	bf.emit(Bytecode::DEF);
	bf.emit((Bytecode)procBytecodeID);
	for(u32 x=0; x<proc->inCommaCount; x+=1){
	    ASTBase *node = proc->body[x];
	    switch(node->type){
	    case ASTType::UNI_DECLERATION:{
		ASTUniVar *var = (ASTUniVar*)node;
		u32 id = procSE->varMap.getValue(var->name);
		const VariableEntity &entity = procSE->varEntities[id];
		u32 regID = procBC.newReg(entity.type);
		procBC.varToReg[id] = regID;
		bf.emitType(entity.type);
		bf.emitReg(regID);
	    }break;
	    case ASTType::MULTI_DECLERATION:{
		ASTMultiVar *var = (ASTMultiVar*)node;
		DynamicArray<String> &names = var->names;
		for(u32 y=0; y<names.count; y+=1){
		    u32 id = procSE->varMap.getValue(names[y]);
		    const VariableEntity &entity = procSE->varEntities[id];
		    u32 regID = procBC.newReg(entity.type);
		    procBC.varToReg[id] = regID;
		    bf.emitType(entity.type);
		    bf.emitReg(regID);
		};
	    }break;
	    };
	};
	bf.emit(Bytecode::PROC_GIVES);
	for(u32 x=0; x<proc->out.count; x+=1){
	    AST_Type *type = (AST_Type*)proc->out[x];
	    bf.emit(Bytecode::TYPE);
	    bf.emit((Bytecode)type->type);
	};
	bf.emit(Bytecode::PROC_START);
	see.push(procSE);
	for(u32 x=proc->inCount; x<proc->body.count; x+=1){
	    compileToBytecode(proc->body[x], lexer, see, bca, bf);
	};
	see.pop();
	bca.pop();
	bf.emit(Bytecode::PROC_END);
    }break;
    default:
	DEBUG_UNREACHABLE;
	break;
    };
};
void compileASTNodesToBytecode(DynamicArray<ASTBase*> &nodes, Lexer &lexer, DynamicArray<ScopeEntities*> &see, DynamicArray<BytecodeContext> &bca, BytecodeFile &bf){
    for(u32 x=0; x<nodes.count; x+=1){
	ASTBase *node = nodes[x];
	compileToBytecode(node, lexer, see, bca, bf);
    };
    bf.emit(Bytecode::NONE);
};

#if(XE_DBG)

#define DUMP_NEXT_BYTECODE			\
    x += 1;					\
    dumpBytecode(page, x);			\

namespace dbg{
    bool dumpBytecode(Bytecode *page, u32 &x){
	printf(" ");
	bool flag = true;
	switch(page[x]){
	case Bytecode::NONE: printf("NONE");return false;
	case Bytecode::REG:{
	    x += 1;
	    printf("%%%d", page[x]);
	}break;
	case Bytecode::GLOBAL:{
	    x += 1;
	    printf("@%d", page[x]);
	}break;
	case Bytecode::MOVS: printf("movs");flag = false;
	case Bytecode::MOVU: if(flag){printf("movu");flag = false;};
	case Bytecode::MOVF:{
	    if(flag){printf("movf");};
	    DUMP_NEXT_BYTECODE;
	    DUMP_NEXT_BYTECODE;
	}break;
	case Bytecode::TYPE:{
	    x += 1;
	    switch(page[x]){
	    case (Bytecode)Type::COMP_VOID: printf("void");break;
	    case (Bytecode)Type::S_64: printf("s64");break;
	    case (Bytecode)Type::U_64: printf("u64");break;
	    case (Bytecode)Type::S_32: printf("s32");break;
	    case (Bytecode)Type::U_32: printf("u32");break;
	    case (Bytecode)Type::S_16: printf("s16");break;
	    case (Bytecode)Type::U_16: printf("u16");break;
	    case (Bytecode)Type::S_8: printf("s8");break;
	    case (Bytecode)Type::U_8: printf("u8");break;
	    case (Bytecode)Type::COMP_INTEGER: printf("comp_int");break;
	    case (Bytecode)Type::COMP_DECIMAL: printf("comp_dec");break;
	    };
	}break;
	case Bytecode::CONST_INTS:{
	    s64 num = getConstInt(page+x+1);
	    x += const_in_stream;
	    printf("%lld", num);
	}break;
	case Bytecode::CONST_INTU:{
	    s64 num = getConstInt(page+x+1); //TODO: maybe sep func?
	    x += const_in_stream;
	    printf("%llu", num);
	}break;
	case Bytecode::CONST_DEC:{
	    f64 num = getConstDec(page+x+1);
	    x += const_in_stream;
	    printf("%f", num);
	}break;
	case Bytecode::ADDS: printf("adds");flag = false;
	case Bytecode::ADDU: if(flag){printf("addu");flag = false;};
	case Bytecode::ADDF:{
	    if(flag){printf("addf");};
	    DUMP_NEXT_BYTECODE;
	    DUMP_NEXT_BYTECODE;
	    DUMP_NEXT_BYTECODE;
	}break;
	case Bytecode::SUBS: printf("subs");flag = false;
	case Bytecode::SUBU: if(flag){printf("subu");flag = false;};
	case Bytecode::SUBF:{
	    if(flag){printf("subf");};
	    DUMP_NEXT_BYTECODE;
	    DUMP_NEXT_BYTECODE;
	    DUMP_NEXT_BYTECODE;
	}break;
	case Bytecode::MULS: printf("muls");flag = false;
	case Bytecode::MULU: if(flag){printf("mulu");flag = false;};
	case Bytecode::MULF:{
	    if(flag){printf("mulf");};
	    DUMP_NEXT_BYTECODE;
	    DUMP_NEXT_BYTECODE;
	    DUMP_NEXT_BYTECODE;
	}break;
	case Bytecode::DIVS: printf("divs");flag = false;
	case Bytecode::DIVU: if(flag){printf("divu");flag = false;};
	case Bytecode::DIVF:{
	    if(flag){printf("divf");};
	    DUMP_NEXT_BYTECODE;
	    DUMP_NEXT_BYTECODE;
	    DUMP_NEXT_BYTECODE;
	}break;
	case Bytecode::CAST:{
	    printf("cast");
	    DUMP_NEXT_BYTECODE;
	    DUMP_NEXT_BYTECODE;
	    DUMP_NEXT_BYTECODE;
	    DUMP_NEXT_BYTECODE;
	}break;
	case Bytecode::PROC_GIVES:
	case Bytecode::PROC_START:
	case Bytecode::PROC_END:break;
	case Bytecode::DEF:{
	    printf("def _");
	    x += 1;
	    printf("%d(", (u32)page[x]);
	    x += 1;
	    u8 comma = 0;
	    while(page[x] != Bytecode::PROC_GIVES){
		dumpBytecode(page, x);
		x += 1;
	        comma += 1;
		if(comma == 2){
		    comma = 0;
		    printf(",");
		};
	    };
	    printf(")");
	    x += 1;
	    if(page[x] != Bytecode::PROC_START){
		printf(" -> (");
		while(page[x] != Bytecode::PROC_START){
		    dumpBytecode(page, x);
		    x += 1;
		};
		printf(")");
	    };
	    printf("\n{\n");
	    x += 1;
	    while(page[x] != Bytecode::PROC_END){
		dumpBytecode(page, x);
		x += 1;
		printf("\n");
	    };
	    printf("}");
	}break;
        case Bytecode::NEG_S: printf("neg_s");flag = false;
	case Bytecode::NEG_U: if(flag){printf("neg_u");flag = false;};
	case Bytecode::NEG_D:{
	    if(flag){printf("neg_d");};
	    DUMP_NEXT_BYTECODE;
	}break;
	default:
	    DEBUG_UNREACHABLE;
	    return false;
	};
	return true;
    };
    void dumpBytecodeFile(BytecodeFile &bf){
	printf("\n\n[DUMPING BYTECODE FILE]\n");
	Bytecode *mem = bf.bcs.mem;
	for(u32 x=0; x<bf.bcs.count; x+=1){
	    if(mem[x] == Bytecode::NONE){break;};
	    if(dumpBytecode(mem, x) == false){break;};
	    printf("\n");
	};
	printf("\n[FINISHED DUMPING BYTECODE FILE]\n\n");
    };
};
#endif
