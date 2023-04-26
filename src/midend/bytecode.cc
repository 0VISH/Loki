#define BYTECODE_PAGE_COUNT 100
#define REGISTER_COUNT      100

enum class Bytecode : u16{
    NONE = 0,
    NEXT_PAGE,
    REG,
    CAST,
    TYPE,
    CONST_INTS,
    CONST_INTU,
    CONST_DEC,
    MOVI,
    MOVU,
    MOVF,
    ADDI,
    ADDU,
    ADDF,
    DEF,
    RET,
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
    DynamicArray<Bytecode*> bytecodePages;
    u16 pageBrim;

    void init(){
	pageBrim = 0;
	bytecodePages.init(2);
	newBytecodePage();
    };
    void uninit(){
	bytecodePages.uninit();
    };
    void emit(Bytecode bc){
#if(XE_DBG)
	if(pageBrim + 1 >= BYTECODE_PAGE_COUNT){
	    printf("\n[ERROR]: forgot to call emitNextPageIfReq?");
	};
#endif
	Bytecode *page = bytecodePages[bytecodePages.count-1];
	page[pageBrim] = bc;
	pageBrim += 1;
    };
    void emitReg(u32 regID){
	emit(Bytecode::REG);
	emit((Bytecode)regID);
    };
    void emitType(Type type){
	emit(Bytecode::TYPE);
	emit((Bytecode)type);
    };
    //encoding constant into bytecode page cause why not?
    void emitConstInt(s64 num){
	Bytecode *page = bytecodePages[bytecodePages.count-1];
	s64 *mem = (s64*)(page + pageBrim);
	*mem = num;
	pageBrim += const_in_stream;
    };
    void emitConstDec(f64 num){
	Bytecode *page = bytecodePages[bytecodePages.count-1];
	f64 *mem = (f64*)(page + pageBrim);
	*mem = num;
	pageBrim += const_in_stream;
    };
    void emitNextPageIfReq(u32 count){
	if(pageBrim+count >= BYTECODE_PAGE_COUNT){
	    emit(Bytecode::NEXT_PAGE);
	    newBytecodePage();
	};
    };
private:
    void newBytecodePage(){
	Bytecode *page = (Bytecode*)mem::alloc(sizeof(Bytecode) * BYTECODE_PAGE_COUNT + 1); //NOTE: +1 for NEXT_PAGE if there is no more space left
	bytecodePages.push(page);
	pageBrim = 0;
    };
};
struct BytecodeContext{
    Map varToReg;
    Map procToID;
    Type *types;
    u32 registerID;
    u32 procID;
    u8 contextID;

    void init(u32 varCount, u32 procCount){
	registerID = 0;
	if(varCount != 0){
	    varToReg.init(varCount);
	    types = (Type*)mem::alloc(sizeof(Type)*REGISTER_COUNT);
	};
	if(procCount != 0){
	    procToID.init(procCount);
	};
	procID = 0;
	contextID = 0;  //TODO: change while multithreading
    };
    void uninit(){
	varToReg.uninit();
	mem::free(types);
    };
    u32 newReg(Type type){
	u32 reg = registerID;
	registerID += 1;
	types[reg] = type;
	return reg;
    };
};

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
void compileExprToBytecode(u32 outputRegister, ASTBase *node, Lexer &lexer, ScopeEntities *se, BytecodeContext &bc, BytecodeFile &bf, bool isExprU=false){
    ASTType type = node->type;
    switch(type){
    case ASTType::NUM_INTEGER:{
	ASTNumInt *numInt = (ASTNumInt*)node;
	String str = makeStringFromTokOff(numInt->tokenOff, lexer);
	s64 num = string2int(str);    //TODO: maybe have a sep func which returns u64
	bf.emitNextPageIfReq(1 + reg_in_stream + 1 + const_in_stream);
	if(isExprU){bf.emit(Bytecode::MOVU);}
	else{bf.emit(Bytecode::MOVI);};
	bf.emitReg(outputRegister);
	if(isExprU){bf.emit(Bytecode::CONST_INTU);}
	else{bf.emit(Bytecode::CONST_INTS);};
	bf.emitConstInt(num);
    }break;
    case ASTType::NUM_DECIMAL:{
	ASTNumDec *numDec = (ASTNumDec*)node;
	String str = makeStringFromTokOff(numDec->tokenOff, lexer);
	f64 num = string2float(str);
	bf.emitNextPageIfReq(1 + reg_in_stream + 1 + const_in_stream);
	bf.emit(Bytecode::MOVF);
	bf.emitReg(outputRegister);
	bf.emit(Bytecode::CONST_DEC);
	bf.emitConstDec(num);
    }break;
    case ASTType::BIN_ADD:{
	ASTBinOp *op = (ASTBinOp*)node;
	Type lhsType = op->lhsType;
	Type rhsType = op->rhsType;
	Type ansType = greaterType(lhsType, rhsType);
	BytecodeType lbt = typeToBytecodeType(lhsType);
	BytecodeType rbt = typeToBytecodeType(rhsType);
	BytecodeType abt = typeToBytecodeType(ansType);
	u32 lhsReg = bc.newReg(lhsType);
	u32 rhsReg = bc.newReg(rhsType);
	compileExprToBytecode(lhsReg, op->lhs, lexer, se, bc, bf, isExprU);
	compileExprToBytecode(rhsReg, op->rhs, lexer, se, bc, bf, isExprU);
	if(lbt != rbt){
	    u32 newReg = bc.newReg(ansType);
	    bf.emitNextPageIfReq(1 + reg_in_stream *2 + type_in_stream*2);
	    bf.emit(Bytecode::CAST);
	    bf.emitType(ansType);
	    bf.emitReg(newReg);
	    if(lbt != abt){
		bf.emitType(lhsType);
		bf.emitReg(lhsReg);
		lhsReg = newReg;
	    }else{
		bf.emitType(rhsType);
		bf.emitReg(rhsReg);
		rhsReg = newReg;
	    };
	};
	bf.emitNextPageIfReq(1 + reg_in_stream*3);
	BytecodeType type = typeToBytecodeType(ansType);
	if(isExprU){bf.emit(Bytecode::ADDU);}
	else if(type == BytecodeType::INTEGER_S){bf.emit(Bytecode::ADDI);}
	else{bf.emit(Bytecode::ADDF);};
	bf.emitReg(outputRegister);
	bf.emitReg(lhsReg);
	bf.emitReg(rhsReg);
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
	compileExprToBytecode(ansReg, var->rhs, lexer, se, bc, bf, typeToBytecodeType(entity.type)==BytecodeType::INTEGER_U);
	bc.varToReg.insertValue(entity.name, regID);
	bf.emitNextPageIfReq(1 + reg_in_stream*2);
	BytecodeType type = typeToBytecodeType(entity.type);
	if(type == BytecodeType::DECIMAL){bf.emit(Bytecode::MOVF);}
	else if(type == BytecodeType::INTEGER_S){bf.emit(Bytecode::MOVI);}
	else{bf.emit(Bytecode::MOVU);};
	bf.emitReg(regID);
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
	compileExprToBytecode(ansReg, var->rhs, lexer, se, bc, bf, typeToBytecodeType(firstEntityType)==BytecodeType::INTEGER_U);
	Bytecode byte;
	BytecodeType type = typeToBytecodeType(firstEntityType);
	if(type == BytecodeType::DECIMAL){byte = Bytecode::MOVF;}
	else if(type == BytecodeType::INTEGER_S){byte = Bytecode::MOVI;}
	else{byte = Bytecode::MOVU;};
	for(u32 x=0; x<names.count; x+=1){
	    u32 id = se->varMap.getValue(names[x]);
	    const VariableEntity &entity = se->varEntities[id];
	    u32 regID = bc.newReg(firstEntityType);
	    bc.varToReg.insertValue(entity.name, regID);
	    bf.emitNextPageIfReq(1 + reg_in_stream*2);
	    bf.emit(byte);
	    bf.emitReg(regID);
	    bf.emitReg(ansReg);
	};
    }break;
    case ASTType::PROC_DEFENITION:{
	ASTProcDef *proc = (ASTProcDef*)node;
	u32 procBytecodeID = bc.procID;
	bc.procToID.insertValue(proc->name, procBytecodeID);
	bc.procID += 1;
	u32 procID = se->procMap.getValue(proc->name);
	ScopeEntities *procSE = se->procEntities[procID].se;
	BytecodeContext &procBC = bca.newElem();
        procBC.init(procSE->varMap.count, procSE->procMap.count);
	// DEF + bc_context_id + proc_id + IN_START  + in_count + OUT_START  + out_count + BODY_START
	u32 reserveCount = 1 + 1 + 1 + 1 + (proc->inCommaCount * (2 + 2)) + 1 + (proc->out.count * (2 + 2)) + 1;
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
	case Bytecode::MOVI: printf("movi");flag = false;
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
	case Bytecode::ADDI: printf("addi");flag = false;
	case Bytecode::ADDU: if(flag){printf("addu");flag = false;};
	case Bytecode::ADDF:{
	    if(flag){printf("addf");};
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
	default:
	    printf("%d", page[x]);
	    DEBUG_UNREACHABLE;
	    return false;
	};
	return true;
    };
    void dumpBytecodePages(DynamicArray<Bytecode*> &pages){
	printf("\n\n[STARTING DUMPING BYTECODE PAGES]\n");
	for(u32 x=0; x<pages.count; x+=1){
	    printf(" -------------------PAGE %d-------------------\n", x);
	    Bytecode *page = pages[x];
	    for(u32 y=0; y<BYTECODE_PAGE_COUNT; y+=1){
		if(page[y] == Bytecode::NEXT_PAGE){
		    printf(" next_page\n");
		    break;
		};
		if(dumpBytecode(page, y) == false){goto DUMP_BYTECODE_PAGES_OVER;};
		printf("\n");
	    };
	};
    DUMP_BYTECODE_PAGES_OVER:
	printf("\n[FINISHED DUMPING BYTECODE PAGES]\n\n");
    };
};
#endif
