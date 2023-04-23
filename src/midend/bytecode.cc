#define BYTECODE_PAGE_COUNT 100

enum class Bytecode : u16{
    NONE = 0,                             //internal
    //Types from enum class Type
    NEXT_PAGE = (u16)Type::TYPE_COUNT,    //internal
    VAR,
    REG,
    CONST_INT,
    MOVI,
    BYTECODE_COUNT,                       //internal
};

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
	if(pageBrim >= BYTECODE_PAGE_COUNT){
	    newBytecodePage();
	};
	Bytecode *page = bytecodePages[bytecodePages.count-1];
	page[pageBrim] = bc;
	pageBrim += 1;
    };
    //encoding constant into bytecode page cause why not?
    void emitConstInt(s64 num){
	const u16 sizeReq = sizeof(s64) / sizeof(Bytecode);
	if(pageBrim+sizeReq >= BYTECODE_PAGE_COUNT){
	    if(pageBrim+1 < BYTECODE_PAGE_COUNT){
		Bytecode *page = bytecodePages[bytecodePages.count-1];
		page[pageBrim] = Bytecode::NEXT_PAGE;
	    };
	    newBytecodePage();
	};
	Bytecode *page = bytecodePages[bytecodePages.count-1];
	s64 *mem = (s64*)(page + pageBrim);
	*mem = num;
	pageBrim += sizeReq;
    };
private:
    void newBytecodePage(){
	Bytecode *page = (Bytecode*)mem::alloc(sizeof(Bytecode) * BYTECODE_PAGE_COUNT);
	bytecodePages.push(page);
	pageBrim = 0;
    };
};
struct BytecodeContext{
    u32 registerID;
    Map varToReg;

    void init(u32 varCount){
	registerID = 0;
	varToReg.init(varCount);
    };
    void uninit(){
	varToReg.uninit();
    };
    u32 emitReg(){
	u32 reg = registerID;
	registerID += 1;
	return reg;
    };
};

s64 getConstInt(Bytecode *bytes, u32 &x){
    x += sizeof(s64) / sizeof(Bytecode);
    s64 *mem = (s64*)bytes;
    return *mem;
};

//bytecode starts with a const or a register
void compileExprToBytecode(ASTBase *node, Lexer &lexer, ScopeEntities &se, BytecodeContext &bc, BytecodeFile &bf){
    ASTType type = node->type;
    switch(type){
    case ASTType::NUM_INTEGER:{
	ASTNumInt *numInt = (ASTNumInt*)node;
	String str = makeStringFromTokOff(numInt->tokenOff, lexer);
	s64 num = string2int(str);
	bf.emit(Bytecode::CONST_INT);
	bf.emitConstInt(num);
    }break;
    default:
	DEBUG_UNREACHABLE;
	break;
    };
};
void compileToBytecode(ASTBase *node, Lexer &lexer, ScopeEntities &se, BytecodeContext &bc, BytecodeFile &bf){
    ASTType type = node->type;
    switch(type){
    case ASTType::UNI_ASSIGNMENT_T_KNOWN:
    case ASTType::UNI_ASSIGNMENT_T_UNKNOWN:{
	ASTUniVar *var = (ASTUniVar*)node;
	u32 id = se.varMap.getValue(var->name);
	const VariableEntity &entity = se.varEntities[id];
	u32 regID = bc.emitReg();
	bc.varToReg.insertValue(entity.name, regID);
	bf.emit(Bytecode::VAR);
	bf.emit((Bytecode)entity.type);
	bf.emit(Bytecode::REG);
	bf.emit((Bytecode)regID);
	bf.emit(Bytecode::MOVI);
	bf.emit(Bytecode::REG);
	bf.emit((Bytecode)regID);
	compileExprToBytecode(var->rhs, lexer, se, bc, bf);
    }break;
    case ASTType::MULTI_ASSIGNMENT_T_KNOWN:
    case ASTType::MULTI_ASSIGNMENT_T_UNKNOWN:{
	ASTMultiVar *var = (ASTMultiVar*)node;
	DynamicArray<String> &names = var->names;
	u32 ansReg = bc.emitReg();
	bf.emit(Bytecode::MOVI);
	bf.emit(Bytecode::REG);
	bf.emit((Bytecode)ansReg);
	compileExprToBytecode(var->rhs, lexer, se, bc, bf);
	for(u32 x=0; x<names.count; x+=1){
	    u32 id = se.varMap.getValue(names[x]);
	    const VariableEntity &entity = se.varEntities[id];
	    u32 regID = bc.emitReg();
	    bc.varToReg.insertValue(entity.name, regID);
	    bf.emit(Bytecode::VAR);
	    bf.emit((Bytecode)entity.type);
	    bf.emit(Bytecode::REG);
	    bf.emit((Bytecode)regID);
	    bf.emit(Bytecode::MOVI);
	    bf.emit(Bytecode::REG);
	    bf.emit((Bytecode)regID);
	    bf.emit(Bytecode::REG);
	    bf.emit((Bytecode)ansReg);
	};
    }break;
    default:
	DEBUG_UNREACHABLE;
	break;
    };
};
void compileASTNodesToBytecode(DynamicArray<ASTBase*> &nodes, Lexer &lexer, ScopeEntities &se, BytecodeContext &bc, BytecodeFile &bf){
    for(u32 x=0; x<nodes.count; x+=1){
	ASTBase *node = nodes[x];
	compileToBytecode(node, lexer, se, bc, bf);
    };
    bf.emit(Bytecode::NONE);
};

#if(XE_DBG)
namespace dbg{
    bool dumpBytecode(Bytecode *page, u32 &x){
	printf(" ");
	switch(page[x]){
	case Bytecode::NONE: printf("NONE");return true;
	case Bytecode::NEXT_PAGE: printf("NEXT_PAGE");break;
	case Bytecode::VAR:{
	    printf("var");
	    x += 1;
	    dumpBytecode(page, x);
	    x += 1;
	    dumpBytecode(page, x);
	}break;
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
	case Bytecode::REG:{
	    x += 1;
	    printf("%%%d", page[x]);
	}break;
	case Bytecode::MOVI:{
	    printf("movi");
	    x += 1;
	    dumpBytecode(page, x);
	    x += 1;
	    printf(" ");
	    dumpBytecode(page, x);
	}break;
	case Bytecode::CONST_INT:{
	    s64 num = getConstInt(page+x+1, x);
	    printf("%lld", num);
	}break;
	default:
	    DEBUG_UNREACHABLE;
	    return false;
	};
	return false;
    };
    void dumpBytecodePages(DynamicArray<Bytecode*> &pages){
	printf("\n\n[STARTING DUMPING BYTECODE PAGES]\n");
	for(u32 x=0; x<pages.count; x+=1){
	    printf(" -------------------PAGE %d-------------------\n", x);
	    Bytecode *page = pages[x];
	    for(u32 y=0; y<BYTECODE_PAGE_COUNT; y+=1){
		if(dumpBytecode(page, y)){goto DUMP_BYTECODE_PAGES_OVER;};
		printf("\n");
	    };
	};
    DUMP_BYTECODE_PAGES_OVER:
	printf("\n[FINISHED DUMPING BYTECODE PAGES]\n\n");
    };
};
#endif
