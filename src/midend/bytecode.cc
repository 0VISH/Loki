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
    LABEL,
    DECL_REG,
    COUNT,
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
    BytecodeBucket *prev;
    Bytecode bytecodes[bytecodes_in_bucket+1];     //padding with NEXT_BUCKET
};

struct BytecodeFile{
    DynamicArray<Bytecode*>   labels;
    BytecodeBucket           *firstBucket;
    BytecodeBucket           *curBucket;
    u16                       cursor;

    void init(){
	labels.init();
	cursor = 0;
	firstBucket = (BytecodeBucket*)mem::alloc(sizeof(BytecodeBucket));
	firstBucket->next = nullptr;
	firstBucket->prev = nullptr;
	firstBucket->bytecodes[bytecodes_in_bucket] = Bytecode::NEXT_BUCKET;
	curBucket = firstBucket;
    };
    void uninit(){
	BytecodeBucket *buc = firstBucket;
	while(buc){
	    BytecodeBucket *temp = buc;
	    buc = buc->next;
	    mem::free(temp);
	};
	labels.uninit();
    };
    void newBucketAndUpdateCurBucket(){
	ASSERT(curBucket);

	cursor = 0;
	BytecodeBucket *nb = (BytecodeBucket*)mem::alloc(sizeof(BytecodeBucket));
	nb->next = nullptr;
	nb->prev = curBucket;
	nb->bytecodes[bytecodes_in_bucket] = Bytecode::NEXT_BUCKET;
	curBucket->next = nb;
	curBucket = nb;
    };
    void reserve(u16 reserve){
	if(cursor + reserve >= bytecodes_in_bucket){
	    emit(Bytecode::NEXT_BUCKET);
	    newBucketAndUpdateCurBucket();
	};
    };
    void emit(Bytecode bc){
	curBucket->bytecodes[cursor] = bc;
	cursor += 1;
    };
    void emit(u16 bc){
	curBucket->bytecodes[cursor] = (Bytecode)bc;
	cursor += 1;
    };
    void emit(Type type){
	curBucket->bytecodes[cursor] = (Bytecode)type;
	cursor += 1;
    };
    void declReg(Type regType, u16 regID){
	emit(Bytecode::DECL_REG);
	emit(regType);
	emit(regID);
    };
    Bytecode *getCurBytecodeAdd(){
	return curBucket->bytecodes + cursor;
    };
    //encoding constant into bytecode page for cache
    void emitConstInt(s64 num){
	Bytecode *mem = getCurBytecodeAdd();
	u64 *memNum = (u64*)(mem);
	*memNum = num;
	cursor += const_in_stream;
    };
    void emitConstDec(f64 num){
	Bytecode *mem = getCurBytecodeAdd();
	f64 *memNum = (f64*)(mem);
	*memNum = num;
	cursor += const_in_stream;
    };
    void label(u16 label){
	reserve(1 + 1);
	emit(Bytecode::LABEL);
	emit(label);
	//@maybe remove this?
	if(label > labels.len){
	    labels.realloc(label);
	};
	Bytecode *mem = getCurBytecodeAdd();
	labels[label] = mem;
    };
    void jmp(u16 label){
	reserve(1 + 1);
	emit(Bytecode::JMP);
	emit(label);
    };
    void jmp(Bytecode op, u16 checkReg, u16 label){
	emit(op);
	emit(checkReg);
	emit(label);
    };
    void movConstS(u16 reg, s64 num){
	reserve(1 + 1 + const_in_stream);
	emit(Bytecode::MOV_CONSTS);
	emit(reg);
	emitConstInt(num);
    };
    void movConstF(u16 outputReg, f64 num){
	reserve(1 + 1 + const_in_stream);
	emit(Bytecode::MOV_CONSTF);
	emit(outputReg);
	emitConstDec(num);
    };
    void binOp(Bytecode op, u16 outputReg, u16 lhsReg, u16 rhsReg){
	reserve(1 + 1 + 1 + 1);
	emit(op);
	emit(outputReg);
	emit(lhsReg);
	emit(rhsReg);
    };
    void cast(Type finalType, u16 finalReg, Type type, u16 reg){
	reserve(1 + 1 + 1 + 1 + 1);
	emit(Bytecode::CAST);
	emit(finalType);
	emit(finalReg);
	emit(type);
	emit(reg);
    };
    void set(Bytecode op, u16 outputReg, u16 inputReg){
	reserve(1 + 1 + 1);
	emit(op);
	emit(outputReg);
	emit(inputReg);
    };
    void neg(Type type, u16 reg){
	reserve(1 + 1 + 1);
	emit(Bytecode::NEG);
	emit(type);
	emit(reg);
    };
    void mov(Bytecode op, u16 outputReg, u16 inputReg){
	reserve(1 + 1 + 1);
	emit(op);
	emit(outputReg);
	emit(inputReg);
    };
};

static u16 procID  = 0;
static u16 labelID = 1;

u16 newLabel(){
    u16 lbl = labelID;
    labelID += 1;
    return lbl;
};

struct BytecodeContext{
    Map procToID;
    u32 *varToReg;
    Type *types;
    u32 registerID;

    void init(u32 varCount, u32 procCount, u16 rgsID){
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
	Type type;
	u16 reg;
	if(lbt != abt){
	    type = lhsType;
	    reg = lhsReg;	
	    lhsReg = newReg;
	}else{
	    type = rhsType;
	    reg = rhsReg;
	    rhsReg = newReg;
	};
	bf.cast(ansType, newReg, type, reg);
    };
    Bytecode binOp;
    switch(abt){
    case BytecodeType::INTEGER_S:   binOp = s;  break;
    case BytecodeType::INTEGER_U:   binOp = u;  break;
    case BytecodeType::DECIMAL_S:   binOp = d;  break;
    };
    bf.binOp(binOp, outputReg, lhsReg, rhsReg);
    return outputReg;
};

s64 getConstIntAndUpdate(Bytecode *page, u32 &x){
    s64 *mem = (s64*)(page + x);
    x += const_in_stream;
    return *mem;
};
s64 getConstInt(Bytecode *page){
    s64 *mem = (s64*)page;
    return *mem;
};
f64 getConstDecAndUpdate(Bytecode *page, u32 &x){
    f64 *mem = (f64*)(page + x);
    x += const_in_stream;
    return *mem;
};
f64 getConstDec(Bytecode *page){
    f64 *mem = (f64*)page;
    return *mem;
};
Bytecode *getPointerAndUpdate(Bytecode *page, u32 &x){
    Bytecode *mem = (Bytecode*)(page + x);
    x += pointer_in_stream;
    return mem;
};
Bytecode *getPointer(Bytecode *page){
    Bytecode *mem = (Bytecode*)page;
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
	bf.movConstS(outputReg, num);
	return outputReg;
    }break;
    case ASTType::NUM_DECIMAL:{
	outputReg = bc.newReg(Type::COMP_DECIMAL);
	ASTNumDec *numDec = (ASTNumDec*)node;
	String str = makeStringFromTokOff(numDec->tokenOff, lexer);
	f64 num = string2float(str);
	bf.movConstF(outputReg, num);
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
	    Type type;
	    u16 reg;
	    if(lbt != abt){
		type = lhsType;
		reg = lhsReg;
		lhsReg = newReg;
	    }else{								
		type = rhsType;
		reg = rhsReg;
		rhsReg = newReg;
	    };
	    bf.cast(ansType, newReg, type, reg);
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

	bf.binOp(cmp, cmpOutReg, lhsReg, rhsReg);
	bf.set(set, outputReg, cmpOutReg);

	return outputReg;
    }break;
    case ASTType::UNI_NEG:{
	ASTUniOp *uniOp = (ASTUniOp*)node;
	u32 outputReg = compileExprToBytecode(uniOp->node, lexer, see, bca, bf);
	Type type = bc.types[outputReg];
	if(typeToBytecodeType(type) == BytecodeType::INTEGER_U){
	    bc.types[outputReg] = Type::S_64;
	};
	bf.neg(type, outputReg);
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
    case ASTType::UNI_DECLERATION:{
	ASTUniVar *var = (ASTUniVar*)node;
	u32 id = se->varMap.getValue(var->name);
	const VariableEntity &entity = se->varEntities[id];
	u16 reg = bc.newReg(entity.type);
	if(isFloat(entity.type)){
	    bf.movConstS(reg, 0);
	}else{
	    bf.movConstF(reg, 0);
	};
    }break;
    case ASTType::MULTI_DECLERATION:{
	ASTMultiVar *var = (ASTMultiVar*)node;
	DynamicArray<String> &names = var->names;
	u32 firstID = se->varMap.getValue(names[0]);
	const VariableEntity &firstEntityID = se->varEntities[firstID];
	Type firstEntityType = firstEntityID.type;
	for(u32 x=0; x<names.count; x+=1){
	    u32 reg = bc.newReg(firstEntityType);
	    if(isFloat(firstEntityType)){
		bf.movConstF(reg, 0);
	    }else{
		bf.movConstS(reg, 0);
	    };
	};
    }break;
    case ASTType::UNI_ASSIGNMENT_T_KNOWN:
    case ASTType::UNI_ASSIGNMENT_T_UNKNOWN:{
ASTUniVar *var = (ASTUniVar*)node;
	u32 id = se->varMap.getValue(var->name);
	const VariableEntity &entity = se->varEntities[id];
	Flag flag = entity.flag;
	u16 regID;
	if(IS_BIT(flag, Flags::UNINITIALIZED)){
	    regID = bc.newReg(entity.type);
	    bf.declReg(entity.type, regID);
	    bc.varToReg[id] = regID;
	}else{
	    regID = compileExprToBytecode(var->rhs, lexer, see, bca, bf);
	    Type regType = bc.types[regID];
	    if(isSameTypeRange(regType, entity.type) == false){
		u32 newReg = bc.newReg(entity.type);
		bf.cast(entity.type, newReg, regType, regID);
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
	    bf.mov(byte, regID, ansReg);
	};
    }break;
    case ASTType::FOR:{
	ASTFor *For = (ASTFor*)node;
	ScopeEntities *ForSe = (ScopeEntities*)For->ForSe;
	BytecodeContext &blockBC = bca.newElem();
	blockBC.init(ForSe->varMap.count, ForSe->procMap.count, bc.registerID);
	see.push(ForSe);
	switch(For->loopType){
	case ForType::FOR_EVER:{
	    u16 loopStartLbl = newLabel();
	    bf.label(loopStartLbl);
	    for(u32 x=0; x<For->body.count; x+=1){
		compileToBytecode(For->body[x], lexer, see, bca, bf);
	    };
	    bf.jmp(loopStartLbl);
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
		bf.movConstS(incrementReg, 1);
	    };
	    compileToBytecode(For->body[0], lexer, see, bca, bf);
	    u16 varReg = blockBC.registerID-1;    //FIXME: 
	    bf.label(loopStartLbl);
	    
	    u16 cond = compileExprToBytecode(For->end, lexer, see, bca, bf);
	    u16 cmpOutReg = blockBC.newReg(ent.type);
	    bf.binOp(cmp, cmpOutReg, varReg, cond);

	    Bytecode setOp;
	    if(For->loopType == ForType::C_EQU){
		setOp = Bytecode::SETE;
	    }else{
		setOp = Bytecode::SETL;
	    };
	    u16 outputReg = blockBC.newReg(ent.type);
	    bf.set(setOp, outputReg, cmpOutReg);

	    bf.jmp(Bytecode::JMPS, outputReg, loopEndLbl);

	    for(u32 x=1; x<For->body.count; x+=1){
		compileToBytecode(For->body[x], lexer, see, bca, bf);
	    };

	    if(For->increment != nullptr){
		incrementReg = compileExprToBytecode(For->increment, lexer, see, bca, bf);
	    };
	    bf.binOp(add, varReg, varReg, incrementReg);

	    bf.jmp(loopStartLbl);

	    bf.label(loopEndLbl);
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
	bf.jmp(Bytecode::JMPNS, exprID, inElseLbl);
	for(u32 x=0; x<If->body.count; x+=1){
	    compileToBytecode(If->body[x], lexer, see, bca, bf);
	};
	see.pop();
	IfSe->uninit();
	mem::free(IfSe);
	bca.pop().uninit();
	if(If->elseBody.count != 0){
	    bf.jmp(outIfLbl);
	    bf.label(inElseLbl);
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
	bf.label(outIfLbl);
    }break;
    case ASTType::PROC_DEFENITION:{
	ASTProcDef *proc = (ASTProcDef*)node;
	u32 procBytecodeID = procID;
	bc.procToID.insertValue(proc->name, procBytecodeID);
	procID += 1;
	u32 procID = se->procMap.getValue(proc->name);
	ScopeEntities *procSE = se->procEntities[procID].se;
	BytecodeContext &procBC = bca.newElem();
        procBC.init(procSE->varMap.count, procSE->procMap.count, bc.registerID);
	//        DEF   ID                   INPUT                          GIVES     OUTPUT       START
	bf.reserve(1  +  1 + (proc->uniInCount + proc->multiInInputCount)*2 + 1  + proc->out.count + 1);
	bf.emit(Bytecode::DEF);
	bf.emit(procBytecodeID);
	u32 inCount = proc->uniInCount + proc->multiInCount;
	for(u32 x=0; x<inCount;){
	    ASTBase *node = proc->body[x];
	    switch(node->type){
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
	//RESERVE ENDS HERE
	see.push(procSE);
	for(u32 x=inCount; x<proc->body.count; x+=1){
	    compileToBytecode(proc->body[x], lexer, see, bca, bf);
	};
	see.pop()->uninit();
	mem::free(procSE);
	bca.pop().uninit();

	bf.reserve(1);
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

#define DUMP_NEXT_BYTECODE dumpBytecode(getBytecode(buc, x), pbuc, x);

#define DUMP_REG dumpReg(getBytecode(buc, x));

#define DUMP_TYPE dumpType((Type)getBytecode(buc, x));

namespace dbg{
    char *spaces = "    ";
    
    void dumpReg(Bytecode id){
	printf(" %%%d ", id);
    };
    void dumpType(Type type){
	printf(" ");
	switch(type){
	case Type::XE_VOID: printf("void");break;
	case Type::S_64: printf("s64");break;
	case Type::U_64: printf("u64");break;
	case Type::F_64: printf("f64");break;
	case Type::S_32: printf("s32");break;
	case Type::U_32: printf("u32");break;
	case Type::F_32: printf("f32");break;
	case Type::S_16: printf("s16");break;
	case Type::U_16: printf("u16");break;
	case Type::F_16: printf("f16");break;
	case Type::S_8: printf("s8");break;
	case Type::U_8: printf("u8");break;
	case Type::COMP_INTEGER: printf("comp_int");break;
	case Type::COMP_DECIMAL: printf("comp_dec");break;
	default:
	    UNREACHABLE;
	    return;
	};
    };
    inline Bytecode getBytecode(BytecodeBucket *buc, u32 &x){
	return buc->bytecodes[x++];
    };
    BytecodeBucket *dumpBytecode(BytecodeBucket *buc, u32 &x, DynamicArray<Bytecode*> &labels){
	bool flag = true;
	Bytecode bc = getBytecode(buc, x);
	switch(bc){
	case Bytecode::NONE: return nullptr;
	case Bytecode::NEXT_BUCKET:{
	    printf("NEXT_BUCKET");
	    buc = buc->next;
	    x = 0;
	}break;
	case Bytecode::LABEL:{
	    printf("%#010x:", getBytecode(buc, x));
	}break;
	case Bytecode::MOV_CONSTS:{
	    printf("mov_consts");
	    DUMP_REG;
	    s64 num = getConstIntAndUpdate(buc->bytecodes, x);
	    printf("%lld", num);
	}break;
	case Bytecode::MOV_CONSTF:{
	    printf("mov_constf");
	    DUMP_REG;
	    f64 num = getConstDecAndUpdate(buc->bytecodes, x);
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
	    Bytecode bc = getBytecode(buc, x);
	    printf("%#010x(%p)", bc, labels[(u16)bc]-2); //-2 because it will be easier to comprehend when pointer address is the same as labels
	}break;
	case Bytecode::JMPNS:{
	    printf("jmpns");
	    DUMP_REG;
	    Bytecode bc = getBytecode(buc, x);
	    printf("%#010x(%p)", bc, labels[(u16)bc]-2); //-2 because it will be easier to comprehend when pointer address is the same as labels
	}break;
	case Bytecode::JMP:{
	    printf("jmp ");
	    Bytecode bc = getBytecode(buc, x);
	    printf("%#010x(%p)", bc, labels[(u16)bc]-2); //-2 because it will be easier to comprehend when pointer address is the same as labels
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
	    Bytecode bc = getBytecode(buc, x);
	    printf("%d(", (u32)bc);
	    bc = getBytecode(buc, x);
	    while(bc != Bytecode::PROC_GIVES){
		dumpType((Type)bc);
		DUMP_REG;
		printf(",");
		bc = getBytecode(buc, x);
	    };
	    printf(")");
	    bc = getBytecode(buc, x);
	    if(bc != Bytecode::PROC_START){
		printf(" -> (");
		while(bc != Bytecode::PROC_START){
		    dumpType((Type)bc);
		    bc = getBytecode(buc, x);
		};
		printf(")");
	    };
	    
	    printf("{");

	    bc = buc->bytecodes[x];
	    while(bc != Bytecode::PROC_END){
		printf("\n%p|%s   ", buc->bytecodes + x, spaces);
		buc = dumpBytecode(buc, x, labels);
		bc = buc->bytecodes[x];
	    };
	    x += 1;
	    
	    printf("\n%p|%s}", buc->bytecodes + x, spaces);
	    
	}break;
	case Bytecode::NEG:{
	    printf("neg");
	    DUMP_TYPE;
	    DUMP_REG;
	}break;
	case Bytecode::DECL_REG:{
	    printf("decl_reg");
	    DUMP_TYPE;
	    DUMP_REG;
	}break;
	default:
	    UNREACHABLE;
	};
	return buc;
    };
    void dumpBytecodeFile(BytecodeFile &bf){
	printf("\n\n[DUMPING BYTECODE FILE]");
	BytecodeBucket *buc = bf.firstBucket;
	u32 x = 0;
	while(buc){
	    printf("\n%p|%s", buc->bytecodes + x, spaces);
	    buc = dumpBytecode(buc, x, bf.labels);
	};
	printf("\n[FINISHED DUMPING BYTECODE FILE]\n\n");
    };
};
#endif
