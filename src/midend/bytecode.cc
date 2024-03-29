#include "bytecode.hh"

BytecodeBucket *newBucket(BytecodeBucket *curBucket){
    BytecodeBucket *nb = (BytecodeBucket*)mem::alloc(sizeof(BytecodeBucket));
    nb->next = nullptr;
    nb->prev = curBucket;
    nb->bytecodes[bytecodes_in_bucket] = Bytecode::NEXT_BUCKET;
    return nb;
};

struct BytecodeFile{
    DynamicArray<Bytecode*>   labels;
    DynamicArray<ASTBase*>    startupNodes;
    BytecodeBucket           *firstBucket;
    BytecodeBucket           *curBucket;
    Reg                       registerID;
    u16                       cursor;
    u8                        id;

    void init(s16 fileID){
	registerID = 1;
	labels.init();
	startupNodes.init();
	cursor = 0;
	id = fileID;
	firstBucket = newBucket(nullptr);
	curBucket = firstBucket;
    };
    void uninit(){
	BytecodeBucket *buc = firstBucket;
	while(buc){
	    BytecodeBucket *temp = buc;
	    buc = buc->next;
	    mem::free(temp);
	};
	startupNodes.uninit();
	labels.uninit();
    };
    void newBucketAndUpdateCurBucket(){
	ASSERT(curBucket);

	cursor = 0;
	BytecodeBucket *nb = newBucket(curBucket);
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
    void emit(Type type){
	curBucket->bytecodes[cursor] = (Bytecode)type;
	cursor += 1;
    };
    void emit(Reg reg){
	curBucket->bytecodes[cursor] = (Bytecode)reg;
	cursor += 1;
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
    void alloc(Type type, Reg reg){
	reserve(1 + 1 + 1);
	emit(Bytecode::ALLOC);
	emit(type);
	emit(reg);
    };
    void store(Type type, Reg dest, Reg src){
	reserve(1 + 1 + 1 + 1);
	emit(Bytecode::STORE);
	emit(type);
	emit(dest);
	emit(src);
    };
    void load(Type type, Reg dest, Reg src){
	reserve(1 + 1 + 1 + 1);
	emit(Bytecode::LOAD);
	emit(type);
	emit(dest);
	emit(src);
    };
    void label(u16 label){
	reserve(1 + 1);
	emit(Bytecode::LABEL);
	emit(label);
	if(label >= labels.len){
	    labels.realloc(label + 5);
	};
	Bytecode *mem = getCurBytecodeAdd();
	labels[label] = mem;
    };
    void procEnd(){
	reserve(1);
	emit(Bytecode::PROC_END);
    };
    void jmp(u16 label){
	reserve(1 + 1);
	emit(Bytecode::JMP);
	emit(label);
    };
    void jmp(Bytecode op, Reg checkReg, u16 labelT, u16 labelF){
	reserve(1 + 1 + 1 + 1);
	emit(op);
	emit(checkReg);
	emit(labelT);
	emit(labelF);
    };
    void cmp(Bytecode op, Type type, Reg des, Reg lhs, Reg rhs){
	reserve(1 + 1 + 1 + 1 + 1 + 1);
	emit(Bytecode::CMP);
	emit(op);
	emit(type);
	emit(des);
	emit(lhs);
	emit(rhs);
    };
    void mov(Type type, Reg dest, Reg src){
	reserve(1 + 1 + 1 + 1);
	emit(Bytecode::MOV);
	emit(type);
	emit(dest);
	emit(src);
    };
    void movConst(Type type, Reg reg, s64 num){
	reserve(1 + 1 + 1 + const_in_stream);
	emit(Bytecode::MOV_CONST);
	emit(type);
	emit(reg);
	emitConstInt(num);
    };
    void movConst(Type type, Reg outputReg, f64 num){
	reserve(1 + 1 + 1 + const_in_stream);
	emit(Bytecode::MOV_CONST);
	emit(type);
	emit(outputReg);
	emitConstDec(num);
    };
    void binOp(Bytecode op, Type type, Reg outputReg, Reg lhsReg, Reg rhsReg){
	reserve(1 + 1 + 1 + 1 + 1);
	emit(op);
	emit(type);
	emit(outputReg);
	emit(lhsReg);
	emit(rhsReg);
    };
    void cast(Type finalType, Reg finalReg, Type type, Reg reg){
	reserve(1 + 1 + 1 + 1 + 1);
	emit(Bytecode::CAST);
	emit(finalType);
	emit(finalReg);
	emit(type);
	emit(reg);
    };
    void set(Bytecode op, Reg outputReg, Reg inputReg){
	reserve(1 + 1 + 1);
	emit(op);
	emit(outputReg);
	emit(inputReg);
    };
    void neg(Type type, Reg newReg, Reg reg){
	reserve(1 + 1 + 1 + 1);
	emit(Bytecode::NEG);
	emit(type);
	emit(newReg);
	emit(reg);
    };
    void ret(Reg reg, bool isVoid){
	reserve(1 + 1 + 1);
	emit(Bytecode::RET);
	emit((Bytecode)isVoid);
	emit(reg);
    };
    void gbl(Reg reg, Type type, s64 num){
	reserve(1 + 1 + const_in_stream);
	emit(Bytecode::GLOBAL);
	emit(reg);
	emit(type);
	emitConstInt(num);
    };
    void gbl(Reg reg, Type type, f64 num){
	reserve(1 + 1 + const_in_stream);
	emit(Bytecode::GLOBAL);
	emit(reg);
	emit(type);
	emitConstDec(num);
    };
    void call(u32 fileID, u32 id, DynamicArray<AST_Type*> &argTypes, Array<Reg> argRegs, DynamicArray<AST_Type*> &retTypes){
	reserve(1 + 1 + 1 + 1 + retTypes.count + 1 + argTypes.count*2);
	emit(Bytecode::CALL);
	emit(fileID);
	emit(id);
	emit(retTypes.count);
	for(u32 x=0; x<retTypes.count; x+=1){
	    emit(retTypes[x]->varType);
	};
	emit(argTypes.count);
	for(u32 x=0; x<argTypes.count; x+=1){
	    emit(argTypes[x]->varType);
	    emit(argRegs[x]);
	};
    };
    void structure(u32 id, Array<Type> &types){
	reserve(1 + 1 + 1 + types.count);
	emit(Bytecode::STRUCT);
	emit(id);
	emit(types.count);
	for(u32 x=0; x<types.count; x+=1){
	    emit(types[x]);
	};
    };
    void getMember(Reg dest, Reg src, u32 id, u32 off){
	reserve(1 + 1 + 1 + 1 + 1);
	emit(Bytecode::GET_MEMBER);
	emit(id);
	emit(dest);
	emit(src);
	emit(off);
    };
    void getElement(Reg dest, Reg src, Type type, Type offType, u32 off){
	reserve(1 + 1 + 1 + 1);
	emit(Bytecode::GET_ELEMENT);
	emit(type);
	emit(dest);
	emit(src);
	emit(offType);
	emit(off);
    };
    Reg newReg(){
	Reg reg = registerID;
	registerID += 1;
	return reg;
    };
};

u16 newLabel(){
    static u16 labelID = 1;
    u16 lbl = labelID;
    labelID += 1;
    return lbl;
};

void VariableContext::init(Scope scopeOfContext, u32 varlen){
    scope = scopeOfContext;
    varRegAndTypes.init();
    varIDToOff.init();
};
void VariableContext::uninit(){
    varRegAndTypes.uninit();
    varIDToOff.uninit();
};
void VariableContext::registerVar(Reg reg, Type type, u32 id){
    u32 off;
    Expr *expr;
    if(varIDToOff.getValue(id, &off) == false){
	expr = &varRegAndTypes.newElem();
	off = varRegAndTypes.count - 1;
    }else{
	expr = &varRegAndTypes[off];
    };
    varIDToOff.insertValue(id, off);
    expr->reg = reg;
    expr->type = type;
};
Expr VariableContext::getVar(u32 id){
    u32 off;
    varIDToOff.getValue(id, &off);
    return varRegAndTypes[off];
};
Expr compileExprToBytecode(ASTBase *node, DynamicArray<ScopeEntities*> &see, DynamicArray<VariableContext> &bca, BytecodeFile &bf);
Expr emitBinOpBc(Bytecode binOp, ASTBase *node, DynamicArray<VariableContext> &bca, BytecodeFile &bf, DynamicArray<ScopeEntities*> &see){
    Expr out;
    ASTBinOp *op = (ASTBinOp*)node;
    VariableContext &bc = bca[bca.count-1];
    Expr lhs = compileExprToBytecode(op->lhs, see, bca, bf);
    Expr rhs = compileExprToBytecode(op->rhs, see, bca, bf);
    Type ansType = greaterType(lhs.type, rhs.type);
    Reg outputReg = bf.newReg();
    if(lhs.isPtr || rhs.isPtr){
	Reg reg;
	Type type;
	Reg offReg;
	Type offType;
	if(lhs.isPtr){
	    reg = lhs.reg;
	    type = lhs.type;
	    offReg = rhs.reg;
	    offType = rhs.type;
	}
	else if(rhs.isPtr){
	    reg = rhs.reg;
	    type = rhs.type;
	    offReg = lhs.reg;
	    offType = rhs.type;
	};
	Reg lreg = bf.newReg();
	bf.load(Type::PTR, lreg, reg);
	Reg dreg = bf.newReg();
	bf.getElement(dreg, lreg, type, offType, offReg);

	out.reg = dreg;
	out.type = type;
	out.isPtr = true;
	return out;
    };
    if(lhs.type != rhs.type){
	Reg casted = bf.newReg();
	if(ansType == lhs.type){
	    bf.cast(ansType, casted, rhs.type, rhs.reg);
	    rhs.reg = casted;
	}else{
	    bf.cast(ansType, casted, lhs.type, lhs.reg);
	    lhs.reg = casted;
	};
    };
    bf.binOp(binOp, ansType, outputReg, lhs.reg, rhs.reg);
    out.reg = outputReg;
    out.type = ansType;
    return out;
};

s64 getConstInt(Bytecode *page){
    s64 *mem = (s64*)page;
    return *mem;
};
f64 getConstDec(Bytecode *page){
    f64 *mem = (f64*)page;
    return *mem;
};
Bytecode *getPointer(Bytecode *page){
    Bytecode *mem = (Bytecode*)page;
    return mem;
};
Expr getVarExprFromID(u32 id, DynamicArray<VariableContext> &bca){
    u32 bcOff = bca.count;
    while(bcOff>0){
	bcOff -= 1;
	VariableContext *bcont = &bca[bcOff];
	u32 off;
	if(bcont->varRegAndTypes.count != 0){
	    bool res = bcont->varIDToOff.getValue(id, &off);
	    if(res){
		return bcont->varRegAndTypes[off];
	    };
	};
	if(bcont->scope == Scope::PROC){
	    bcont = &bca[0];
	    while(bcont->scope == Scope::GLOBAL){
		if(bcont->varRegAndTypes.count != 0){
		    bool res = bcont->varIDToOff.getValue(id, &off);
		    if(res){
			return bcont->varRegAndTypes[off];
		    };
		};
		bcont = &bca[bcOff];
		bcOff += 1;
	    };
	    break;
	};
    };
    Expr expr;
    expr.type = Type::UNKOWN;
    return expr;
};
Expr compileExprToBytecode(ASTBase *node, DynamicArray<ScopeEntities*> &see, DynamicArray<VariableContext> &bca, BytecodeFile &bf){
    Expr out = {};
    if(IS_BIT(node->flag, Flags::GLOBAL) && node->type != ASTType::PROC_DEFENITION && node->type != ASTType::DECLERATION && node->type != ASTType::INITIALIZATION_T_KNOWN && node->type != ASTType::INITIALIZATION_T_UNKNOWN){
	bf.startupNodes.push(node);
	return out;
    };
    ASTType type = node->type;
    VariableContext &bc = bca[bca.count - 1];
    Reg outputReg;
    switch(type){
    case ASTType::MULTI_VAR_RHS:{
	ASTMultiVarRHS *mvr = (ASTMultiVarRHS*)node;
	out.type = Type::UNKOWN;
	if(mvr->reg == 0){
	    auto rhs = compileExprToBytecode(mvr->rhs, see, bca, bf);
	    mvr->reg = rhs.reg;
	    out.type = rhs.type;
	};
	out.reg = mvr->reg;
	return out;
    }break;
    case ASTType::STRING:{
	ASTString *string = (ASTString*)node;
	GlobalStrings::addEntryIfRequired(string->str);
	//TODO: 
    }break;
    case ASTType::NUM_INTEGER:{
	const Type type = Type::COMP_INTEGER;
	outputReg = bf.newReg();
	ASTNumInt *numInt = (ASTNumInt*)node;
	s64 num = numInt->num;
	bf.movConst(type, outputReg, num);
	out.reg = outputReg;
	out.type = type;
	return out;
    }break;
    case ASTType::NUM_DECIMAL:{
	const Type type = Type::COMP_DECIMAL;
	outputReg = bf.newReg();
	ASTNumDec *numDec = (ASTNumDec*)node;
	f64 num = numDec->num;
	bf.movConst(type, outputReg, num);
	out.reg = outputReg;
	out.type = type;
	return out;
    }break;
    case ASTType::MODIFIER:{
	ASTModifier *mod = (ASTModifier*)node;
	EntityRef<VariableEntity> &ent = mod->varEntRef;
	Expr varExpr = getVarExprFromID(ent.id, bca);
	u32 rootReg = varExpr.reg;
	while(mod->child->type != ASTType::VARIABLE){
	    ASTModifier *modc = (ASTModifier*)mod->child;
	    StructEntity *structEnt = modc->structEntRef.ent;
	    u32 id  = structEnt->id;
	    u32 off = modc->structEntRef.id;
	    Reg memberReg = bf.newReg();
	    bf.getMember(memberReg, rootReg, id, off);
	    rootReg = memberReg;
	    mod = modc;
	};
	Reg memberReg = bf.newReg();
	ASTVariable *var = (ASTVariable*)mod->child;
	StructEntity *structEnt = var->structEntRef.ent;
	u32 id  = structEnt->id;
	u32 off = var->structEntRef.id;
	bf.getMember(memberReg, rootReg, id, off);
	out.reg = memberReg;
	out.type = structEnt->varToType[off];
	return out;
    }break;
    case ASTType::VARIABLE:{
	ASTVariable *var = (ASTVariable*)node;
	u32 id = var->varEntRef.id;
	u32 pAccessDepth = var->pAccessDepth;
	//NOTE: we do not need to check if off == -1 as the checker would have raised a problem
	Expr expr = getVarExprFromID(id, bca);
	Reg base = expr.reg;
	while(pAccessDepth != 0){
	    Reg temp = bf.newReg();
	    bf.load(Type::PTR, temp, base);
	    base = temp;
	    pAccessDepth -= 1;
	};
	//TODO: load contents
	expr.reg = base;
	expr.isPtr = var->varEntRef.ent->pointerDepth > 0;
	return expr;
    }break;
    case ASTType::BIN_ADD: return emitBinOpBc(Bytecode::ADD, node, bca, bf, see);
    case ASTType::BIN_MUL: return emitBinOpBc(Bytecode::MUL, node, bca, bf, see);
    case ASTType::BIN_DIV: return emitBinOpBc(Bytecode::DIV, node, bca, bf, see);
    case ASTType::BIN_SHL: return emitBinOpBc(Bytecode::SHL, node, bca, bf, see);
    case ASTType::BIN_SHR: return emitBinOpBc(Bytecode::SHR, node, bca, bf, see);
    case ASTType::BIN_GRT:
    case ASTType::BIN_GRTE:
    case ASTType::BIN_LSR:
    case ASTType::BIN_LSRE:
    case ASTType::BIN_EQU:{
        ASTBinOp *op = (ASTBinOp*)node;					
	auto lhs = compileExprToBytecode(op->lhs, see, bca, bf);	
	auto rhs = compileExprToBytecode(op->rhs, see, bca, bf);	
	Type ansType = greaterType(lhs.type, rhs.type);
	
	Bytecode set;
	switch(op->type){
	case ASTType::BIN_GRTE: set = Bytecode::SETGE; break;
	case ASTType::BIN_GRT:  set = Bytecode::SETG;  break;
	case ASTType::BIN_LSR:  set = Bytecode::SETL;  break;
	case ASTType::BIN_LSRE: set = Bytecode::SETLE; break;
	case ASTType::BIN_EQU:  set = Bytecode::SETE;  break;
	};
	Reg cmpOutReg = bf.newReg();
	bf.cmp(set, ansType, cmpOutReg, lhs.reg, rhs.reg);

	out.reg = cmpOutReg;
	out.type = ansType;
	return out;
    }break;
    case ASTType::UNI_NEG:{
	ASTUniOp *uniOp = (ASTUniOp*)node;
	auto operand = compileExprToBytecode(uniOp->node, see, bca, bf);
	Reg newReg;
	if(isIntU(operand.type)){
	    newReg = bf.newReg();
	}else{
	    newReg = bf.newReg();
	};
	bf.neg(operand.type, newReg, operand.reg);

	return operand;
    }break;
    default:
	printf("TYPE: %d", type);
	UNREACHABLE;
	break;
    };
    return out;
};
void compileToBytecode(ASTBase *node, ASTFile &file, DynamicArray<ScopeEntities*> &see, DynamicArray<VariableContext> &bca, BytecodeFile &bf){
    VariableContext &bc = bca[bca.count-1];
    ScopeEntities *se = see[see.count-1];
    ASTType type = node->type;
    switch(type){
    case ASTType::ASSIGNMENT:{
	ASTAssignment *ass = (ASTAssignment*)node;
	Expr lhs = compileExprToBytecode(ass->lhs, see, bca, bf);
	Expr rhs = compileExprToBytecode(ass->rhs, see, bca, bf);
	if(lhs.type != rhs.type){
	    Reg castedReg = bf.newReg();
	    bf.cast(lhs.type, castedReg, rhs.type, rhs.reg);
	    rhs.reg = castedReg;
	    rhs.type = lhs.type;
	};
	bf.store((rhs.isPtr)?Type::PTR:rhs.type, lhs.reg, rhs.reg);
    }break;
    case ASTType::RETURN:{
	ASTReturn *ret = (ASTReturn*)node;
	if(ret->expr == nullptr){
	    bf.ret(0, true);
	}else{
	    auto rhs = compileExprToBytecode(ret->expr, see, bca, bf);
	    bf.ret(rhs.reg, false);
	};
    }break;
    case ASTType::DECLERATION:{
	ASTUniVar *var = (ASTUniVar*)node;
	u32 id = var->entRef.id;
	VariableEntity *entity = var->entRef.ent;
	Reg reg = bf.newReg();
	Type type = entity->type;
	bf.alloc((entity->pointerDepth==0)?type:Type::PTR, reg);
	if(!IS_BIT(var->flag, Flags::UNINITIALIZED)){
	    if(IS_BIT(config.helps, Help::INITIALIZE_WITH_0)){
		Reg init = bf.newReg();
		if(isFloat(type)){
		    bf.movConst(type, init, (f64)0.0);
		    bf.store(type, reg, init);
		}else if(isInt(type)){
		    bf.movConst(type, init, (s64)0);
		    bf.store(type, reg, init);
		};
	    };
	};
	bc.registerVar(reg, type, id);
    }break;
    case ASTType::INITIALIZATION_T_KNOWN:
    case ASTType::INITIALIZATION_T_UNKNOWN:{
	ASTUniVar *var = (ASTUniVar*)node;
	u32 id = var->entRef.id;
	VariableEntity *entity = var->entRef.ent;
	Type type = entity->type;
	Flag flag = var->flag;
	Reg reg;
	if(IS_BIT(flag, Flags::GLOBAL)){
	    reg = bf.newReg() * -1;
	    if(IS_BIT(flag, Flags::UNINITIALIZED)){
		bf.gbl(reg, type, (s64)0);
	    }else if(var->rhs->type == ASTType::NUM_INTEGER){
		ASTNumInt *numInt = (ASTNumInt*)var->rhs;
		bf.gbl(reg, type, numInt->num);
	    }else if(var->rhs->type == ASTType::NUM_DECIMAL){
		ASTNumDec *numDec = (ASTNumDec*)var->rhs;
		bf.gbl(reg, type, numDec->num);
	    }else{
		bf.gbl(reg, type, (s64)0);
		ASTGblVarInit *gvi = (ASTGblVarInit*)allocAST(sizeof(ASTGblVarInit), ASTType::GLOBAL_VAR_INIT, file);
		gvi->reg = reg;
		gvi->rhs = var->rhs;
		gvi->varType = type;
		bf.startupNodes.push(gvi);
	    };
	}else if(IS_BIT(flag, Flags::UNINITIALIZED) || IS_BIT(flag, Flags::ALLOC)){
	    //NOTE: UNI_ASSIGNMENT_T_UNKOWN wont reach here as checking stage would have raised an error
	    reg = bf.newReg();
	    bf.alloc((entity->pointerDepth==0)?type:Type::PTR, reg);
	}else{
	    reg = bf.newReg();
	    bf.alloc((entity->pointerDepth==0)?type:Type::PTR, reg);
	    auto rhs = compileExprToBytecode(var->rhs, see, bca, bf);
	    if(type != rhs.type){
		Reg castedReg = bf.newReg();
		bf.cast(type, castedReg, rhs.type, rhs.reg);
		rhs.reg = castedReg;
	    };
	    bf.store(type, reg, rhs.reg);
	};
	bc.registerVar(reg, type, id);
    }break;
    case ASTType::PROC_CALL:{
	ASTProcCall *pc = (ASTProcCall*)node;
	ProcEntity *pe = pc->entRef.ent;
	u32 procID = pc->entRef.id;
	Array<Reg> argRegs(pc->args.count);
	for(u32 x=0; x<pc->args.count; x+=1){
	    Type procExpectedType = pe->argTypes[x]->varType;
	    auto rhs = compileExprToBytecode(pc->args[x], see, bca, bf);
	    Reg argReg = bf.newReg();
	    bf.cast(procExpectedType, argReg, rhs.type, rhs.reg);
	    argRegs.push(argReg);
	    
	};
	bf.call(pe->fileID, procID, pe->argTypes, argRegs, pe->retTypes);
    }break;
    case ASTType::GLOBAL_VAR_INIT:{
	ASTGblVarInit *gvi = (ASTGblVarInit*)node;
	auto rhs = compileExprToBytecode(gvi->rhs, see, bca, bf);
	bf.store(gvi->varType, gvi->reg, rhs.reg);
    }break;
    case ASTType::FOR:{
	ASTFor *For = (ASTFor*)node;
	ScopeEntities *ForSe = For->ForSe;
	VariableContext &blockBC = bca.newElem();
	blockBC.init(Scope::BLOCK);
	see.push(ForSe);
	switch(For->loopType){
	case ForType::FOR_EVER:{
	    u16 loopStartLbl = newLabel();
	    bf.label(loopStartLbl);
	    for(u32 x=0; x<For->body.count; x+=1){
		compileToBytecode(For->body[x], file, see, bca, bf);
	    };
	    bf.jmp(loopStartLbl);
	}break;
	case ForType::EXPR:{
	    u16 loopStartLbl = newLabel();
	    u16 loopBodyLbl = newLabel();
	    u16 loopEndLbl = newLabel();
	    bf.label(loopStartLbl);
	    Expr expr = compileExprToBytecode(For->expr, see, bca, bf);
	    bf.jmp(Bytecode::JMPS, expr.reg, loopBodyLbl, loopEndLbl);
	    bf.label(loopBodyLbl);
	    for(u32 x=0; x<For->body.count; x+=1){
		compileToBytecode(For->body[x], file, see, bca, bf);
	    };
	    bf.label(loopEndLbl);
	}break;
	case ForType::C_LES:
	case ForType::C_EQU:{
	    ASTUniVar *var = (ASTUniVar*)For->body[0];
	    u32 id;
	    ForSe->varMap.getValue(var->name, &id);
	    VariableEntity &ent = ForSe->varEntities[id];
	    u16 loopStartLbl = newLabel();
	    u16 loopBodyLbl = newLabel();
	    u16 loopEndLbl = newLabel();

	    u16 incrementReg;
	    if(For->increment == nullptr){
		incrementReg = bf.newReg();
		bf.movConst(Type::S_64, incrementReg, (s64)1);
	    };
	    compileToBytecode(For->body[0], file, see, bca, bf);
	    Reg varRegPtr = bf.registerID-1;    //FIXME: 
	    bf.label(loopStartLbl);
	    
	    auto cond = compileExprToBytecode(For->end, see, bca, bf);
	    Bytecode setOp;
	    if(For->loopType == ForType::C_EQU){
		setOp = Bytecode::SETE;
	    }else{
		setOp = Bytecode::SETL;
	    };
	    Reg cmpOutReg = bf.newReg();
	    Reg varVal1 = bf.newReg();
	    bf.load(ent.type, varVal1, varRegPtr);
	    bf.cmp(setOp, ent.type, cmpOutReg, varVal1, cond.reg);
	    bf.jmp(Bytecode::JMPS, cmpOutReg, loopBodyLbl, loopEndLbl);

	    bf.label(loopBodyLbl);
	    for(u32 x=1; x<For->body.count; x+=1){
		compileToBytecode(For->body[x], file, see, bca, bf);
	    };

	    if(For->increment != nullptr){
		incrementReg = compileExprToBytecode(For->increment, see, bca, bf).reg;
	    };
	    Reg tempRes = bf.newReg();
	    Reg varVal2  = bf.newReg();
	    bf.load(ent.type, varVal2, varRegPtr);
	    bf.binOp(Bytecode::ADD, ent.type, tempRes, varVal2, incrementReg);
	    bf.store(ent.type, varRegPtr, tempRes);

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
	u32 exprID = compileExprToBytecode(If->expr, see, bca, bf).reg;
	VariableContext &blockBC = bca.newElem();
	ScopeEntities *IfSe = If->IfSe;
	blockBC.init(Scope::BLOCK);
	see.push(IfSe);
	u16 inIfLbl = newLabel();
	u16 outIfLbl = newLabel();
	u16 inElseLbl = outIfLbl;
	if(If->elseBody.count != 0){
	    inElseLbl = newLabel();
	};
	bf.jmp(Bytecode::JMPS, exprID, inIfLbl, inElseLbl);
	for(u32 x=0; x<If->body.count; x+=1){
	    compileToBytecode(If->body[x], file, see, bca, bf);
	};
	see.pop();
	IfSe->uninit();
	mem::free(IfSe);
	bca.pop().uninit();
	if(If->elseBody.count != 0){
	    bf.jmp(outIfLbl);
	    bf.label(inElseLbl);
	    ScopeEntities *ElseSe = If->ElseSe;
	    see.push(ElseSe);
	    VariableContext &elseBC = bca.newElem();
	    elseBC.init(Scope::BLOCK);
	    for(u32 x=0; x<If->elseBody.count; x+=1){
		compileToBytecode(If->elseBody[x], file, see, bca, bf);
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
	ProcEntity *pe = proc->entRef.ent;
	u32 procBytecodeID = proc->entRef.id;
	ScopeEntities *procSE = proc->se;
	VariableContext &procBC = bca.newElem();
        procBC.init(Scope::PROC);
	//        DEF    OUTPUT_COUNT         OUTPUT       ID     INPUT_COUNT     INPUT*(Type + Reg)
	bf.reserve(1  + const_in_stream + proc->out.count + 1 + const_in_stream + (proc->inCount)*2);
	bf.emit(Bytecode::DEF);
	bf.emitConstInt(proc->out.count);
	for(u32 x=0; x<proc->out.count; x+=1){
	    AST_Type *type = (AST_Type*)proc->out[x];
	    bf.emit(type->varType);
	};
	bf.emit((Bytecode)procBytecodeID);
	bf.emitConstInt(proc->inCount);
	for(u32 x=0; x<proc->inCount;){
	    ASTBase *node = proc->body[x];
	    switch(node->type){
	    case ASTType::DECLERATION:{
		ASTUniVar *var = (ASTUniVar*)node;
		u32 id = var->entRef.id;
		VariableEntity *entity = var->entRef.ent;
		Reg reg = bf.newReg();
		bf.emit(entity->type);
		bf.emit(reg);
		procBC.registerVar(reg, entity->type, id);
		x += 1;
	    }break;
	    };
	};
	//RESERVE ENDS HERE
	see.push(procSE);
	for(u32 x=proc->inCount; x<proc->body.count; x+=1){
	    compileToBytecode(proc->body[x], file, see, bca, bf);
	};
	see.pop()->uninit();
	mem::free(procSE);
	bf.procEnd();
	bca.pop().uninit();
    }break;
    case ASTType::STRUCT_DEFENITION:{
	ASTStructDef *Struct = (ASTStructDef*)node;
	StructEntity *structEnt = Struct->entRef.ent;
	Array<Type> types(Struct->members.count);
	types.mem = structEnt->varToType;
	types.count = Struct->members.count;
	bf.structure(Struct->entRef.id, types);
    }break;
    case ASTType::IMPORT: break;
    default:
	printf("BYTECODE: %d\n", type);
	UNREACHABLE;
	break;
    };
};
void compileASTFileToBytecode(ASTFile &file, DynamicArray<ScopeEntities*> &see, DynamicArray<VariableContext> &bca, BytecodeFile &bf){
    for(u32 x=0; x<file.nodes.count; x+=1){
	ASTBase *node = file.nodes[x];
	compileToBytecode(node, file, see, bca, bf);
    };
    if(bf.startupNodes.count > 0){
	bf.emit(Bytecode::_TEXT_STARTUP_START);
	for(u32 x=0; x<bf.startupNodes.count; x+=1){
	    ASTBase *node = bf.startupNodes[x];
	    CLEAR_BIT(node->flag, Flags::GLOBAL);
	    compileToBytecode(node, file, see, bca, bf);
	};
	bf.emit(Bytecode::_TEXT_STARTUP_END);
    };
    bf.emit(Bytecode::NONE);
};
#if(DBG)

#define DUMP_NEXT_BYTECODE dumpBytecode(getBytecode(buc, x), pbuc, x);
#define DUMP_REG dumpReg((Reg)getBytecode(buc, x));
#define DUMP_TYPE dumpType((Type)getBytecode(buc, x));

namespace dbg{
    char *spaces = "    ";
    
    void dumpReg(Reg reg){
	if(reg < 0){
	    printf(" $%d ", reg * -1);
	}else{
	    printf(" %%%d ", reg);
	};
    };
    Type dumpType(Type type){
	printf(" ");
	switch(type){
	case Type::PTR:  printf("ptr");break;
	case Type::XE_VOID: printf("void");break;
	case Type::S_64: printf("s64");break;
	case Type::U_64: printf("u64");break;
	case Type::F_64: printf("f64");break;
	case Type::S_32: printf("s32");break;
	case Type::U_32: printf("u32");break;
	case Type::F_32: printf("f32");break;
	case Type::S_16: printf("s16");break;
	case Type::U_16: printf("u16");break;
	case Type::S_8: printf("s8");break;
	case Type::U_8: printf("u8");break;
	case Type::COMP_INTEGER: printf("comp_int");break;
	case Type::COMP_DECIMAL: printf("comp_dec");break;
	default:
	    printf("struct(%d)", type);
	    return (Type)type;
	};
	return type;
    };
    s64 getConstIntAndUpdate(Bytecode *page, u32 &x){
	s64 *mem = (s64*)(page + x);
	x += const_in_stream;
	return *mem;
    };
    f64 getConstDecAndUpdate(Bytecode *page, u32 &x){
	f64 *mem = (f64*)(page + x);
	x += const_in_stream;
	return *mem;
    };
    Bytecode *getPointerAndUpdate(Bytecode *page, u32 &x){
	Bytecode *mem = (Bytecode*)(page + x);
	x += pointer_in_stream;
	return mem;
    };
    inline Bytecode getBytecode(BytecodeBucket *buc, u32 &x){
	return buc->bytecodes[x++];
    };
    BytecodeBucket *dumpBytecode(BytecodeBucket *buc, u32 &x, DynamicArray<Bytecode*> &labels){
	printf("\n%p|%s    ", buc->bytecodes + x, spaces);
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
	case Bytecode::MOV_CONST:{
	    printf("mov_const");
	    DUMP_TYPE;
	    DUMP_REG;
	    s64 num = getConstIntAndUpdate(buc->bytecodes, x);
	    printf("%lld", num);
	}break;
	case Bytecode::MOV:{
	    printf("mov");
	    DUMP_TYPE;
	    DUMP_REG;
	    DUMP_REG;	    
	}break;
	case Bytecode::ADD:{
	    printf("add");
	    DUMP_TYPE;
	    DUMP_REG;
	    DUMP_REG;
	    DUMP_REG;
	}break;
	case Bytecode::SUB:{
	    printf("sub");
	    DUMP_TYPE;
	    DUMP_REG;
	    DUMP_REG;
	    DUMP_REG;;
	}break;
	case Bytecode::MUL:{
	    printf("mul");
	    DUMP_TYPE;
	    DUMP_REG;
	    DUMP_REG;
	    DUMP_REG;
	}break;
	case Bytecode::DIV:{
	    printf("div");
	    DUMP_TYPE;
	    DUMP_REG;
	    DUMP_REG;
	    DUMP_REG;
	}break;
	case Bytecode::SHL:{
	    printf("shl");
	    DUMP_TYPE;
	    DUMP_REG;
	    DUMP_REG;
	    DUMP_REG;
	}break;
	case Bytecode::SHR:{
	    printf("shr");
	    DUMP_TYPE;
	    DUMP_REG;
	    DUMP_REG;
	    DUMP_REG;
	}break;
	case Bytecode::CMP:{
	    printf("cmp ");
	    Bytecode bc = getBytecode(buc, x);
	    switch(bc){
	    case Bytecode::SETG: printf("setg");break;
	    case Bytecode::SETL: printf("setl");break;
	    case Bytecode::SETE: printf("sete");break;
	    case Bytecode::SETGE: printf("setge");break;
	    case Bytecode::SETLE: printf("setle");break;
	    };
	    DUMP_TYPE;
	    DUMP_REG;
	    DUMP_REG;
	    DUMP_REG;
	}break;
	case Bytecode::JMPS:{
	    printf("jmps");
	    DUMP_REG;
	    Bytecode bc = getBytecode(buc, x);
	    printf("%#010x(%p) ", bc, labels[(u16)bc]-2); //-2 because it will be easier to comprehend when pointer address is the same as labels
	    bc = getBytecode(buc, x);
	    printf("%#010x(%p)", bc, labels[(u16)bc]-2);
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
	case Bytecode::DEF:{
	    printf("def ");
	    s64 outCount = getConstIntAndUpdate(buc->bytecodes, x);
	    while(outCount != 0){
		bc = getBytecode(buc, x);
		dumpType((Type)bc);
		printf(",");
		outCount -= 1;
	    };
	    bc = getBytecode(buc, x);
	    printf("_%d(", (u32)bc);
	    s64 inCount = getConstIntAndUpdate(buc->bytecodes, x);
	    while(inCount != 0){
		bc = getBytecode(buc, x);
		dumpType((Type)bc);
		DUMP_REG;
		printf(",");
		inCount -= 1;
	    };
	    printf("){");
	    bc = buc->bytecodes[x];
	    while(bc != Bytecode::PROC_END){
		buc = dumpBytecode(buc, x, labels);
		bc = buc->bytecodes[x];
	    };
	    dumpBytecode(buc, x, labels);
	}break;
	case Bytecode::PROC_END:{
	    printf("}");
	}break;
	case Bytecode::NEG:{
	    printf("neg");
	    DUMP_TYPE;
	    DUMP_REG;
	    DUMP_REG;
	}break;
	case Bytecode::RET:{
	    printf("return");
	    if((bool)buc->bytecodes[x] == true){
		x += 1;
		printf(" void");
		x += 1;
	    }else{
		DUMP_REG;
	    };
	}break;
	case Bytecode::ALLOC:{
	    printf("alloc");
	    DUMP_TYPE;
	    DUMP_REG;
	}break;
	case Bytecode::STORE:{
	    printf("store");
	    DUMP_TYPE;
	    DUMP_REG;
	    DUMP_REG;
	}break;
	case Bytecode::LOAD:{
	    printf("load");
	    DUMP_TYPE;
	    DUMP_REG;
	    DUMP_REG;
	}break;
	case Bytecode::GLOBAL:{
	    printf("global");
	    DUMP_REG;
	    Type type = DUMP_TYPE;
	    if(isFloat(type)){
		f64 num = getConstDecAndUpdate(buc->bytecodes, x);
		printf(" %f", num);
	    }else{
		s64 num = getConstIntAndUpdate(buc->bytecodes, x);
		printf(" %lld", num);
	    };
	}break;
	case Bytecode::CALL:{
	    Bytecode fileID = getBytecode(buc, x);
	    Bytecode id = getBytecode(buc, x);
	    printf("call %d%d", fileID, id);
	    Bytecode retCount = getBytecode(buc, x);
	    for(u32 j=0; j<(u32)retCount; j+=1){
		DUMP_TYPE;
	    };
	    printf("(");
	    Bytecode inCount = getBytecode(buc, x);
	    for(u32 j=0; j<(u32)inCount; j+=1){
		DUMP_TYPE;
		DUMP_REG;
		printf(", ");
	    };
	    printf(")");
	}break;
	case Bytecode::STRUCT:{
	    Bytecode id = getBytecode(buc, x);
	    Bytecode count = getBytecode(buc, x);
	    printf("%d struct{", id);
	    for(u32 j=0; j<(u32)count; j+=1){
		DUMP_TYPE;
		printf(",");
	    };
	    printf("}");
	}break;
	case Bytecode::GET_ELEMENT:{
	    Bytecode type = getBytecode(buc, x);
	    Bytecode dest = getBytecode(buc, x);
	    Bytecode src  = getBytecode(buc, x);
	    Bytecode offType = getBytecode(buc, x);
	    Bytecode off  = getBytecode(buc, x);
	    printf("%%%d = get_element type:%d, src:%%%d, offType:%d, off:%%%d", dest, type, src, offType, off);
	}break;
	case Bytecode::GET_MEMBER:{
	    Bytecode id = getBytecode(buc, x);
	    Bytecode dest = getBytecode(buc, x);
	    Bytecode src  = getBytecode(buc, x);
	    Bytecode off  = getBytecode(buc, x);
	    printf("%%%d = get_member type:%d, src:%%%d, off:%d", dest, id, src, off);
	}break;
	case Bytecode::_TEXT_STARTUP_START:{
	    printf("_TEXT_STARTUP_START:");
	}break;
	case Bytecode::_TEXT_STARTUP_END:{
	    printf("_TEXT_STARTUP_END");
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
	    buc = dumpBytecode(buc, x, bf.labels);
	};
	printf("\n[FINISHED DUMPING BYTECODE FILE]\n\n");
    };
};
#endif
