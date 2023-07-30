#define BYTECODE_PAGE_COUNT 100
#define REGISTER_COUNT      100

/*
  %d  register
  @d  proc
*/
enum class Bytecode : u16{
    NONE = 0,
    CAST,
    MOVS,
    MOVU,
    MOVF,
    MOV_CONSTS,
    MOV_CONSTF,
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
    CMPS,
    CMPU,
    CMPF,
    SETG,
    SETL,
    SETE,
    SETGE,
    SETLE,
    JMPNS,        //jumps if given register is 0
    JMPS,         //jumps if given register is not 0
    JMP,
    DEF,
    PROC_GIVES,
    PROC_START,
    PROC_END,
    RET,
    NEG,
    NEXT_BUCKET,
    BYTECODE_COUNT,
};
enum class BytecodeType : u16{
    INTEGER_S,
    INTEGER_U,
    DECIMAL_S,
};

const u16 const_in_stream = sizeof(s64) / sizeof(Bytecode);
const u16 pointer_in_stream = sizeof(Bytecode*) / sizeof(Bytecode);
const u16 bytecodes_in_bucket = 30;

struct BytecodeBucket{
    BytecodeBucket *next;
    Bytecode bytecodes[bytecodes_in_bucket];
    u16 cursor;
};

struct BytecodeFile{
    BytecodeBucket          *firstBucket;
    BytecodeBucket          *curBucket;

    void init(){
	firstBucket = (BytecodeBucket*)mem::alloc(sizeof(BytecodeBucket));
	firstBucket->next = nullptr;
	firstBucket->cursor = 0;
	curBucket = firstBucket;
    };
    void uninit(){
	BytecodeBucket *buc = firstBucket;
	while(buc){
	    mem::free(buc);
	    buc = buc->next;
	};
    };
    void newBucketAndUpdateCurBucket(){
	ASSERT(curBucket);
    
	BytecodeBucket *nb = (BytecodeBucket*)mem::alloc(sizeof(BytecodeBucket));
	nb->next = nullptr;
	nb->cursor = 0;
	curBucket->next = nb;
	curBucket = nb;
    };
    void emit(Bytecode bc){
        if(curBucket->cursor == bytecodes_in_bucket){
	    newBucketAndUpdateCurBucket();
	};
	curBucket->bytecodes[curBucket->cursor] = bc;
	curBucket->cursor += 1;
    };
    void emit(u16 bc){
        if(curBucket->cursor == bytecodes_in_bucket){
	    newBucketAndUpdateCurBucket();
	};
	curBucket->bytecodes[curBucket->cursor] = (Bytecode)bc;
	curBucket->cursor += 1;
    };
    void emit(Type type){
	if(curBucket->cursor == bytecodes_in_bucket){
	    newBucketAndUpdateCurBucket();
	};
	curBucket->bytecodes[curBucket->cursor] = (Bytecode)type;
	curBucket->cursor += 1;
    };
    Bytecode *getCurBytecodeAdd(){
	return curBucket->bytecodes + curBucket->cursor;
    };
    void reserve(u16 reserve){
	if(curBucket->cursor + reserve >= bytecodes_in_bucket){
	    emit(Bytecode::NEXT_BUCKET);
	    newBucketAndUpdateCurBucket();
	};
    };
    void emitPointer(Bytecode* pointer){
        reserve(pointer_in_stream);
	emit(Bytecode::NEXT_BUCKET);
	Bytecode *mem = getCurBytecodeAdd();
	u64 *memTemp = (u64*)mem;
	*memTemp = (u64)pointer;
	curBucket->cursor += pointer_in_stream;
    };
    //encoding constant into bytecode page for cache
    void emitConstInt(s64 num){
	reserve(const_in_stream);
	Bytecode *mem = getCurBytecodeAdd();
	u64 *memNum = (u64*)(mem);
	*memNum = num;
	curBucket->cursor += const_in_stream;
    };
    void emitConstDec(f64 num){
	reserve(const_in_stream);
	Bytecode *mem = getCurBytecodeAdd();
	f64 *memNum = (f64*)(mem);
	*memNum = num;
	curBucket->cursor += const_in_stream;
    };
};

static u16 procID  = 0;

struct BytecodeContext{
    Map procToID;
    u32 *varToReg;
    Type *types;
    u16 registerID;

    void init(u32 varCount, u32 procCount, u16 rgsID = 0){
	varToReg = nullptr;
	registerID = rgsID;
	types = (Type*)mem::alloc(sizeof(Type)*REGISTER_COUNT);
	if(varCount != 0){
	    varToReg = (u32*)mem::alloc(sizeof(u32)*varCount);
	};
	procToID.len = 0;
	if(procCount != 0){
	    procToID.init(procCount);
	};
    };
    void uninit(){
	if(varToReg != nullptr){mem::free(varToReg);};
	if(procToID.len != 0){procToID.uninit();};
	mem::free(types);
    };
    u16 newReg(Type type){
	u16 reg = registerID;
	registerID += 1;
	types[reg] = type;
	return reg;
    };
};
BytecodeType typeToBytecodeType(Type type){
    switch(type){
    case Type::F_64:
    case Type::F_32:
    case Type::F_16:
    case Type::COMP_DECIMAL: return BytecodeType::DECIMAL_S;
    case Type::S_64:
    case Type::S_32:
    case Type::S_16:
    case Type::COMP_INTEGER: return BytecodeType::INTEGER_S;
    default: return BytecodeType::INTEGER_U;
    }
};
u16 compileExprToBytecode(ASTBase *node, Lexer &lexer, DynamicArray<ScopeEntities*> &see, DynamicArray<BytecodeContext> &bca, BytecodeFile &bf);
u16 emitBinOpBc(Bytecode s, Bytecode u, Bytecode d, ASTBase *node, Lexer &lexer, DynamicArray<BytecodeContext> &bca, BytecodeFile &bf, DynamicArray<ScopeEntities*> &see){
    ASTBinOp *op = (ASTBinOp*)node;
    BytecodeContext &bc = bca[bca.count-1];
    u16 lhsReg = compileExprToBytecode(op->lhs, lexer, see, bca, bf);
    u16 rhsReg = compileExprToBytecode(op->rhs, lexer, see, bca, bf);
    Type lhsType = bc.types[lhsReg];
    Type rhsType = bc.types[rhsReg];
    Type ansType = greaterType(lhsType, rhsType);
    u16 outputReg = bc.newReg(ansType);
    BytecodeType lbt = typeToBytecodeType(lhsType);
    BytecodeType rbt = typeToBytecodeType(rhsType);
    BytecodeType abt = typeToBytecodeType(ansType);
    if(lbt != rbt){
	u32 newReg = bc.newReg(ansType);
	bf.emit(Bytecode::CAST);
	bf.emit(ansType);
	bf.emit(newReg);
	if(lbt != abt){
	    bf.emit(lhsType);
	    bf.emit(lhsReg);	
	    lhsReg = newReg;
	}else{
	    bf.emit(rhsType);
	    bf.emit(rhsReg);
	    rhsReg = newReg;
	};
    };
    switch(abt){
    case BytecodeType::INTEGER_S:   bf.emit(s);  break;
    case BytecodeType::INTEGER_U:   bf.emit(u);  break;
    case BytecodeType::DECIMAL_S:   bf.emit(d);  break;
    };
    bf.emit(outputReg);
    bf.emit(lhsReg);
    bf.emit(rhsReg);
    return outputReg;
};

void nextBucket(BytecodeBucket **pbuc){
    BytecodeBucket *buc = *pbuc;
    buc = buc->next;
};
s64 getConstInt(BytecodeBucket **pbuc, u32 &x){
    BytecodeBucket *buc = *pbuc;
    if(x + const_in_stream >= bytecodes_in_bucket){
	nextBucket(pbuc);
	x = 0;
    };
    s64 *mem = (s64*)(buc->bytecodes + x);
    x += const_in_stream;
    return *mem;
};
f64 getConstDec(BytecodeBucket **pbuc, u32 &x){
    BytecodeBucket *buc = *pbuc;
    if(x + const_in_stream >= bytecodes_in_bucket){
	nextBucket(pbuc);
	x = 0;
    };
    f64 *mem = (f64*)(buc->bytecodes + x);
    x += const_in_stream;
    return *mem;
};
Bytecode *getPointer(BytecodeBucket **pbuc, u32 &x){
    BytecodeBucket *buc = *pbuc;
    if(x + pointer_in_stream >= bytecodes_in_bucket){
	nextBucket(pbuc);
	x = 0;
    };
    Bytecode *mem = (Bytecode*)(buc->bytecodes + x);
    x += pointer_in_stream;
    return mem;
};
u16 compileExprToBytecode(ASTBase *node, Lexer &lexer, DynamicArray<ScopeEntities*> &see, DynamicArray<BytecodeContext> &bca, BytecodeFile &bf){
    ASTType type = node->type;
    BytecodeContext &bc = bca[bca.count - 1];
    u16 outputReg;
    switch(type){
    case ASTType::NUM_INTEGER:{
	outputReg = bc.newReg(Type::COMP_INTEGER);
	ASTNumInt *numInt = (ASTNumInt*)node;
	String str = makeStringFromTokOff(numInt->tokenOff, lexer);
	s64 num = string2int(str);
	bf.emit(Bytecode::MOV_CONSTS);
	bf.emit(outputReg);
	bf.emitConstInt(num);
	return outputReg;
    }break;
    case ASTType::NUM_DECIMAL:{
	outputReg = bc.newReg(Type::COMP_DECIMAL);
	ASTNumDec *numDec = (ASTNumDec*)node;
	String str = makeStringFromTokOff(numDec->tokenOff, lexer);
	f64 num = string2float(str);
	bf.emit(Bytecode::MOV_CONSTS);
	bf.emit(outputReg);
	bf.emitConstDec(num);
	return outputReg;
    }break;
    case ASTType::VARIABLE:{
	ASTVariable *var = (ASTVariable*)node;
	u32 off = see.count-1;
	ScopeEntities *se = see[off];
	s32 id = se->varMap.getValue(var->name);
	if(id == -1){
	    off = 0;
	    se = see[0];
	    id = se->varMap.getValue(var->name);
	};
	BytecodeContext &correctBC = bca[off];
	return correctBC.varToReg[id];	
    }break;
    case ASTType::BIN_ADD:{									
	return emitBinOpBc(Bytecode::ADDS, Bytecode::ADDU, Bytecode::ADDF, node, lexer, bca, bf, see);
    }break;
    case ASTType::BIN_MUL:{
	return emitBinOpBc(Bytecode::MULS, Bytecode::MULU, Bytecode::MULF, node, lexer, bca, bf, see);
    }break;
    case ASTType::BIN_DIV:{
	return emitBinOpBc(Bytecode::DIVS, Bytecode::DIVU, Bytecode::DIVF, node, lexer, bca, bf, see);
    }break;
    case ASTType::BIN_GRT:
    case ASTType::BIN_GRTE:
    case ASTType::BIN_LSR:
    case ASTType::BIN_LSRE:
    case ASTType::BIN_EQU:{
        ASTBinOp *op = (ASTBinOp*)node;					
	u32 lhsReg = compileExprToBytecode(op->lhs, lexer, see, bca, bf);	
	u32 rhsReg = compileExprToBytecode(op->rhs, lexer, see, bca, bf);	
	Type lhsType = bc.types[lhsReg];					
	Type rhsType = bc.types[rhsReg];					
	Type ansType = greaterType(lhsType, rhsType);			
	u16 cmpOutReg = bc.newReg(ansType);					
	BytecodeType lbt = typeToBytecodeType(lhsType);			
	BytecodeType rbt = typeToBytecodeType(rhsType);			
	BytecodeType abt = typeToBytecodeType(ansType);			
	if(lbt != rbt){							
	    u16 newReg = bc.newReg(ansType);				
	    bf.emit(Bytecode::CAST);					
	    bf.emit(ansType);						
	    bf.emit(newReg);						
	    if(lbt != abt){							
		bf.emit(lhsType);					
		bf.emit(lhsReg);						
		lhsReg = newReg;						
	    }else{								
		bf.emit(rhsType);					
		bf.emit(rhsReg);						
		rhsReg = newReg;						
	    };								
	};
	Bytecode set;
	Bytecode cmp;
	switch(op->type){
	case ASTType::BIN_GRTE: set = Bytecode::SETGE; break;
	case ASTType::BIN_GRT:  set = Bytecode::SETG;  break;
	case ASTType::BIN_LSR:  set = Bytecode::SETL;  break;
	case ASTType::BIN_LSRE: set = Bytecode::SETLE; break;
	case ASTType::BIN_EQU:  set = Bytecode::SETE;  break;
	};
	switch(abt){							
	case BytecodeType::INTEGER_S: cmp = Bytecode::CMPS; break;	
	case BytecodeType::INTEGER_U: cmp = Bytecode::CMPU; break;
	case BytecodeType::DECIMAL_S: cmp = Bytecode::CMPF; break;
	};
	
	outputReg = bc.newReg(ansType);
	
	bf.emit(cmp);
	bf.emit(cmpOutReg);						
	bf.emit(lhsReg);							
	bf.emit(rhsReg);

	bf.emit(set);
	bf.emit(outputReg);
	bf.emit(cmpOutReg);
	
	return outputReg;
    }break;
    case ASTType::UNI_NEG:{
	ASTUniOp *uniOp = (ASTUniOp*)node;
	u32 outputReg = compileExprToBytecode(uniOp->node, lexer, see, bca, bf);
	Type type = bc.types[outputReg];
	if(typeToBytecodeType(type) == BytecodeType::INTEGER_U){
	    bc.types[outputReg] = Type::S_64;
	};
	bf.emit(Bytecode::NEG);
	bf.emit(type);
	bf.emit(outputReg);
	return outputReg;
    }break;
    default:
	UNREACHABLE;
	break;
    };
    return 0;
};
void compileToBytecode(ASTBase *node, Lexer &lexer, DynamicArray<ScopeEntities*> &see, DynamicArray<BytecodeContext> &bca, BytecodeFile &bf){
    BytecodeContext &bc = bca[bca.count-1];
    ScopeEntities *se = see[see.count-1];
    ASTType type = node->type;
    switch(type){
	//TODO: compile decleration
    case ASTType::UNI_ASSIGNMENT_T_KNOWN:
    case ASTType::UNI_ASSIGNMENT_T_UNKNOWN:{
	ASTUniVar *var = (ASTUniVar*)node;
	u32 id = se->varMap.getValue(var->name);
	const VariableEntity &entity = se->varEntities[id];
	u32 regID = compileExprToBytecode(var->rhs, lexer, see, bca, bf);
	Type regType = bc.types[regID];
	if(regType != Type::COMP_DECIMAL && regType != Type::COMP_INTEGER){
	    if(regType != entity.type){
		u32 newReg = bc.newReg(entity.type);
		bf.emit(Bytecode::CAST);
		bf.emit(entity.type);
		bf.emit(newReg);
		bf.emit(regType);
		bf.emit(regID);
		bc.varToReg[id] = newReg;
	    }else{
		bc.varToReg[id] = regID;
	    };
        };
    }break;
    case ASTType::MULTI_ASSIGNMENT_T_KNOWN:
    case ASTType::MULTI_ASSIGNMENT_T_UNKNOWN:{
	ASTMultiVar *var = (ASTMultiVar*)node;
	DynamicArray<String> &names = var->names;
	u32 firstID = se->varMap.getValue(names[0]);
	const VariableEntity &firstEntityID = se->varEntities[firstID];
	Type firstEntityType = firstEntityID.type;
	u32 ansReg = compileExprToBytecode(var->rhs, lexer, see, bca, bf);
	Bytecode byte;
	BytecodeType type = typeToBytecodeType(firstEntityType);
	if(type == BytecodeType::DECIMAL_S){byte = Bytecode::MOVF;}
	else if(type == BytecodeType::INTEGER_S){byte = Bytecode::MOVS;}
	else{byte = Bytecode::MOVU;};
	for(u32 x=0; x<names.count; x+=1){
	    u32 id = se->varMap.getValue(names[x]);
	    const VariableEntity &entity = se->varEntities[id];
	    u16 regID = bc.newReg(firstEntityType);
	    bc.varToReg[id] = regID;
	    bc.types[regID] = bc.types[ansReg];
	    bf.emit(byte);
	    bf.emit(regID);
	    bf.emit(ansReg);
	};
    }break;
#if 0 
    case ASTType::FOR:{
	ASTFor *For = (ASTFor*)node;
	ScopeEntities *ForSe = (ScopeEntities*)For->ForSe;
	BytecodeContext &blockBC = bca.newElem();
	blockBC.init(ForSe->varMap.count, ForSe->procMap.count, bc.registerID);
	see.push(ForSe);
	switch(For->loopType){
	case ForType::FOR_EVER:{
	    u16 loopStartLbl = newLabel();
	    bf.emitLabel(loopStartLbl);
	    for(u32 x=0; x<For->body.count; x+=1){
		compileToBytecode(For->body[x], lexer, see, bca, bf);
	    };
	    bf.emit(Bytecode::JMP);
	    bf.emit(loopStartLbl);
	}break;
	case ForType::C_LES:
	case ForType::C_EQU:{
	    ASTUniVar *var = (ASTUniVar*)For->body[0];
	    u32 id = ForSe->varMap.getValue(var->name);
	    VariableEntity &ent = ForSe->varEntities[id];
	    BytecodeType varBcType = typeToBytecodeType(ent.type);
	    Bytecode cmp;
	    Bytecode add;
	    switch(varBcType){
	    case BytecodeType::INTEGER_S:
		cmp = Bytecode::CMPS;
		add = Bytecode::ADDS;
		break;
	    case BytecodeType::INTEGER_U:
		cmp = Bytecode::CMPU;
		add = Bytecode::ADDU;
		break;
	    case BytecodeType::DECIMAL_S:
		cmp = Bytecode::CMPF;
		add = Bytecode::ADDF;
		break;
	    };

	    u16 loopStartLbl = newLabel();
	    u16 loopEndLbl = newLabel();

	    u16 incrementReg;
	    if(For->increment == nullptr){
		incrementReg = blockBC.newReg(ent.type);
		bf.emit(Bytecode::MOV_CONSTS);
		bf.emit(incrementReg);
		bf.emitConstInt(1);
	    };
	    compileToBytecode(For->body[0], lexer, see, bca, bf);
	    u16 varReg = blockBC.registerID-1;
	    bf.emitLabel(loopStartLbl);
	    
	    u16 cond = compileExprToBytecode(For->end, lexer, see, bca, bf);
	    u16 cmpOutReg = blockBC.newReg(ent.type);
	    bf.emit(cmp);
	    bf.emit(cmpOutReg);
	    bf.emit(varReg);
	    bf.emit(cond);
	    
	    if(For->loopType == ForType::C_EQU){
		bf.emit(Bytecode::SETE);
	    }else{
		bf.emit(Bytecode::SETL);
	    };
	    u16 outputReg = blockBC.newReg(ent.type);
	    bf.emit(outputReg);
	    bf.emit(cmpOutReg);

	    bf.emit(Bytecode::JMPS);
	    bf.emit(outputReg);
	    bf.emit(loopEndLbl);

	    for(u32 x=1; x<For->body.count; x+=1){
		compileToBytecode(For->body[x], lexer, see, bca, bf);
	    };

	    if(For->increment != nullptr){
		incrementReg = compileExprToBytecode(For->increment, lexer, see, bca, bf);
	    };
	    bf.emit(add);
	    bf.emit(varReg);
	    bf.emit(varReg);
	    bf.emit(incrementReg);

	    bf.emit(Bytecode::JMP);
	    bf.emit(loopStartLbl);

	    bf.emitLabel(loopEndLbl);
	}break;
	};
	see.pop();
	bca.pop().uninit();
	ForSe->uninit();
	mem::free(ForSe);
    }break;
    case ASTType::IF:{
	ASTIf *If = (ASTIf*)node;
	u32 exprID = compileExprToBytecode(If->expr, lexer, see, bca, bf);
	BytecodeContext &blockBC = bca.newElem();
	ScopeEntities *IfSe = (ScopeEntities*)If->IfSe;
	blockBC.init(IfSe->varMap.count, IfSe->procMap.count, bc.registerID);
	see.push(IfSe);
	u16 outIfLbl = newLabel();
	u16 inElseLbl = outIfLbl;
	if(If->elseBody.count != 0){
	    inElseLbl = newLabel();
	};
	bf.emit(Bytecode::JMPNS);
	bf.emit(exprID);
	bf.emit(inElseLbl);
	for(u32 x=0; x<If->body.count; x+=1){
	    compileToBytecode(If->body[x], lexer, see, bca, bf);
	};
	see.pop();
	IfSe->uninit();
	mem::free(IfSe);
	bca.pop().uninit();
	if(If->elseBody.count != 0){
	    bf.emit(Bytecode::JMP);
	    bf.emit(outIfLbl);

	    bf.emitLabel(inElseLbl);
	    ScopeEntities *ElseSe = (ScopeEntities*)If->ElseSe;
	    see.push(ElseSe);
	    BytecodeContext &elseBC = bca.newElem();
	    elseBC.init(ElseSe->varMap.count, ElseSe->procMap.count, bc.registerID);
	    for(u32 x=0; x<If->elseBody.count; x+=1){
		compileToBytecode(If->elseBody[x], lexer, see, bca, bf);
	    };
	    see.pop();
	    ElseSe->uninit();
	    mem::free(ElseSe);
	    bca.pop().uninit();
	};
	bf.emitLabel(outIfLbl);
    }break;
#endif
    case ASTType::PROC_DEFENITION:{
	ASTProcDef *proc = (ASTProcDef*)node;
	u32 procBytecodeID = procID;
	bc.procToID.insertValue(proc->name, procBytecodeID);
	procID += 1;
	u32 procID = se->procMap.getValue(proc->name);
	ScopeEntities *procSE = se->procEntities[procID].se;
	BytecodeContext &procBC = bca.newElem();
        procBC.init(procSE->varMap.count, procSE->procMap.count, bc.registerID);
	bf.emit(Bytecode::DEF);
	bf.emit(procBytecodeID);
	for(u32 x=0; x<proc->inCount;){
	    ASTBase *node = proc->body[x];
	    switch(node->type){
		//TODO: compile input body?
	    case ASTType::UNI_DECLERATION:{
		ASTUniVar *var = (ASTUniVar*)node;
		u32 id = procSE->varMap.getValue(var->name);
		const VariableEntity &entity = procSE->varEntities[id];
		u16 regID = procBC.newReg(entity.type);
		procBC.varToReg[id] = regID;
		bf.emit(entity.type);
		bf.emit(regID);
		x += 1;
	    }break;
	    case ASTType::MULTI_DECLERATION:{
		ASTMultiVar *var = (ASTMultiVar*)node;
		DynamicArray<String> &names = var->names;
		for(u32 y=0; y<names.count; y+=1){
		    u32 id = procSE->varMap.getValue(names[y]);
		    const VariableEntity &entity = procSE->varEntities[id];
		    u16 regID = procBC.newReg(entity.type);
		    procBC.varToReg[id] = regID;
		    bf.emit(entity.type);
		    bf.emit(regID);
		};
		x += var->names.count;
	    }break;
	    };
	};
	bf.emit(Bytecode::PROC_GIVES);
	for(u32 x=0; x<proc->out.count; x+=1){
	    AST_Type *type = (AST_Type*)proc->out[x];
	    bf.emit(type->type);
	};
	bf.emit(Bytecode::PROC_START);
	see.push(procSE);
	for(u32 x=proc->inCount; x<proc->body.count; x+=1){
	    compileToBytecode(proc->body[x], lexer, see, bca, bf);
	};
	see.pop()->uninit();
	mem::free(procSE);
	bca.pop().uninit();
	
	bf.emit(Bytecode::PROC_END);
    }break;
    default:
	UNREACHABLE;
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

#if(DBG)

#define DUMP_NEXT_BYTECODE dumpBytecode(getBytecode(pbuc, x), pbuc, x);

#define DUMP_REG dumpReg(getBytecode(pbuc, x));

#define DUMP_TYPE dumpType(getBytecode(pbuc, x));

namespace dbg{
    void dumpReg(Bytecode id){
	printf(" %%%d ", id);
    };
    void dumpType(Bytecode bc){
	printf(" ");
	switch(bc){
	case (Bytecode)Type::XE_VOID: printf("void");break;
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
    };
    Bytecode getBytecode(BytecodeBucket **pbuc, u32 &x){
	BytecodeBucket *buc = *pbuc;
	if(x == bytecodes_in_bucket){
	    x = 0;
	    buc = buc->next;
	};
	u32 v = x;
	x += 1;
	return buc->bytecodes[v];
    };
    void dumpBytecode(BytecodeBucket **pbuc, u32 &x){
	printf(" ");
	bool flag = true;
	Bytecode bc = getBytecode(pbuc, x);
	switch(bc){
	case Bytecode::NONE: *pbuc = nullptr; break;
	case Bytecode::NEXT_BUCKET:{
	    nextBucket(pbuc);
	}break;
	case Bytecode::MOV_CONSTS:{
	    printf("mov_consts");
	    DUMP_REG;
	    s64 num = getConstInt(pbuc, x);
	    printf("%lld", num);
	}break;
	case Bytecode::MOV_CONSTF:{
	    printf("mov_constf");
	    DUMP_REG;
	    f64 num = getConstDec(pbuc, x);
	    printf("%f", num);
	}break;
	case Bytecode::MOVS: printf("movs");flag = false;
	case Bytecode::MOVU: if(flag){printf("movu");flag = false;};
	case Bytecode::MOVF:{
	    if(flag){printf("movf");};
	    DUMP_REG;
	    DUMP_REG;	    
	}break;
	case Bytecode::ADDS: printf("adds");flag = false;
	case Bytecode::ADDU: if(flag){printf("addu");flag = false;};
	case Bytecode::ADDF:{
	    if(flag){printf("addf");};
	    DUMP_REG;
	    DUMP_REG;
	    DUMP_REG;
	}break;
	case Bytecode::SUBS: printf("subs");flag = false;
	case Bytecode::SUBU: if(flag){printf("subu");flag = false;};
	case Bytecode::SUBF:{
	    if(flag){printf("subf");};
	    DUMP_REG;
	    DUMP_REG;
	    DUMP_REG;;
	}break;
	case Bytecode::MULS: printf("muls");flag = false;
	case Bytecode::MULU: if(flag){printf("mulu");flag = false;};
	case Bytecode::MULF:{
	    if(flag){printf("mulf");};
	    DUMP_REG;
	    DUMP_REG;
	    DUMP_REG;
	}break;
	case Bytecode::DIVS: printf("divs");flag = false;
	case Bytecode::DIVU: if(flag){printf("divu");flag = false;};
	case Bytecode::DIVF:{
	    if(flag){printf("divf");};
	    DUMP_REG;
	    DUMP_REG;
	    DUMP_REG;
	}break;
	case Bytecode::CMPS: printf("cmps");flag = false;
	case Bytecode::CMPU: if(flag){printf("cmpu");flag = false;};
	case Bytecode::CMPF:{
	    if(flag){printf("cmpf");};
	    DUMP_REG;
	    DUMP_REG;
	    DUMP_REG;
	}break;
	case Bytecode::SETG:  printf("setg"); flag = false;
	case Bytecode::SETL:  if(flag){printf("setl");  flag = false;};
	case Bytecode::SETE:  if(flag){printf("sete");  flag = false;};
	case Bytecode::SETGE: if(flag){printf("setge"); flag = false;};
	case Bytecode::SETLE:{
	    if(flag){printf("setle"); flag = false;};
	    DUMP_REG;
	    DUMP_REG;
	}break;
	case Bytecode::JMPS:{
	    printf("jmps");
	    DUMP_REG;
	    printf("%p", getPointer(pbuc, x));
	}break;
	case Bytecode::JMPNS:{
	    printf("jmpns");
	    DUMP_REG;
	    printf("%p", getPointer(pbuc, x));
	}break;
	case Bytecode::JMP:{
	    printf("jmp");
	    printf(" %p", getPointer(pbuc, x));
	}break;
	case Bytecode::CAST:{
	    printf("cast");
	    DUMP_TYPE;
	    DUMP_REG;
	    DUMP_TYPE;
	    DUMP_REG;
	}break;
	case Bytecode::PROC_GIVES:
	case Bytecode::PROC_START:
	case Bytecode::PROC_END:break;
	case Bytecode::DEF:{
	    printf("def _");
	    Bytecode bc = getBytecode(pbuc, x);
	    printf("%d(", (u32)bc);
	    u8 comma = 0;
	    bc = getBytecode(pbuc, x);
	    while(bc != Bytecode::PROC_GIVES){
		dumpType(bc);
		DUMP_REG;
		bc = getBytecode(pbuc, x);
	        comma += 1;
		if(comma == 2){
		    comma = 0;
		    printf(",");
		};
	    };
	    printf(")");
	    bc = getBytecode(pbuc, x);
	    if(bc != Bytecode::PROC_START){
		printf(" -> (");
		while(bc != Bytecode::PROC_START){
		    dumpType(bc);
		    bc = getBytecode(pbuc, x);
		};
		printf(")");
	    };
	    
	    printf("\n{");
	    
	    x -= 1;
	    while(bc != Bytecode::PROC_END){
		dumpBytecode(pbuc, x);
		BytecodeBucket *buc = *pbuc;
		bc = buc->bytecodes[x];
		printf("\n");
	    };
	    x += 1;
	    
	    printf("}");
	    
	}break;
	case Bytecode::NEG:{
	    printf("neg");
	    DUMP_TYPE;
	    DUMP_REG;
	}break;
	default:
	    UNREACHABLE;
	};
    };
    void dumpBytecodeFile(BytecodeFile &bf){
	printf("\n\n[DUMPING BYTECODE FILE]\n");
	BytecodeBucket *buc = bf.firstBucket;
	u32 x = 0;
	while(buc){
	    dumpBytecode(&buc, x);
	    printf("\n");
	};
	printf("\n[FINISHED DUMPING BYTECODE FILE]\n\n");
    };
};
#endif
