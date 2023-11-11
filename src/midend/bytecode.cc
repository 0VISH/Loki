#include "bytecode.hh"

void BytecodeFile::init(s16 fileID){
    labels.init();
    cursor = 0;
    id = fileID;
    firstBucket = (BytecodeBucket*)mem::alloc(sizeof(BytecodeBucket));
    firstBucket->next = nullptr;
    firstBucket->prev = nullptr;
    firstBucket->bytecodes[bytecodes_in_bucket] = Bytecode::NEXT_BUCKET;
    curBucket = firstBucket;
};
void BytecodeFile::uninit(){
    BytecodeBucket *buc = firstBucket;
    while(buc){
	BytecodeBucket *temp = buc;
	buc = buc->next;
	mem::free(temp);
    };
    labels.uninit();
};
void BytecodeFile::newBucketAndUpdateCurBucket(){
    ASSERT(curBucket);

    cursor = 0;
    BytecodeBucket *nb = (BytecodeBucket*)mem::alloc(sizeof(BytecodeBucket));
    nb->next = nullptr;
    nb->prev = curBucket;
    nb->bytecodes[bytecodes_in_bucket] = Bytecode::NEXT_BUCKET;
    curBucket->next = nb;
    curBucket = nb;
};
void BytecodeFile::reserve(u16 reserve){
    if(cursor + reserve >= bytecodes_in_bucket){
	emit(Bytecode::NEXT_BUCKET);
	newBucketAndUpdateCurBucket();
    };
};
void BytecodeFile::emit(Bytecode bc){
    curBucket->bytecodes[cursor] = bc;
    cursor += 1;
};
void BytecodeFile::emit(Type type){
    curBucket->bytecodes[cursor] = (Bytecode)type;
    cursor += 1;
};
void BytecodeFile::emit(Reg reg){
    curBucket->bytecodes[cursor] = (Bytecode)reg;
    cursor += 1;
};
Bytecode *BytecodeFile::getCurBytecodeAdd(){
    return curBucket->bytecodes + cursor;
};
//encoding constant into bytecode page for cache
void BytecodeFile::emitConstInt(s64 num){
    Bytecode *mem = getCurBytecodeAdd();
    u64 *memNum = (u64*)(mem);
    *memNum = num;
    cursor += const_in_stream;
};
void BytecodeFile::emitConstDec(f64 num){
    Bytecode *mem = getCurBytecodeAdd();
    f64 *memNum = (f64*)(mem);
    *memNum = num;
    cursor += const_in_stream;
};
void BytecodeFile::alloc(Type type, Reg reg){
    reserve(1 + 1 + 1);
    emit(Bytecode::ALLOC);
    emit(type);
    emit(reg);
};
void BytecodeFile::store(Type type, Reg dest, Reg src){
    reserve(1 + 1 + 1 + 1);
    emit(Bytecode::STORE);
    emit(type);
    emit(dest);
    emit(src);
};
void BytecodeFile::load(Type type, Reg dest, Reg src){
    reserve(1 + 1 + 1 + 1);
    emit(Bytecode::LOAD);
    emit(type);
    emit(dest);
    emit(src);
};
void BytecodeFile::label(u16 label){
    reserve(1 + 1);
    emit(Bytecode::LABEL);
    emit(label);
    if(label >= labels.len){
	labels.realloc(label + 5);
    };
    Bytecode *mem = getCurBytecodeAdd();
    labels[label] = mem;
};
void BytecodeFile::procEnd(){
    reserve(1);
    emit(Bytecode::PROC_END);
};
void BytecodeFile::jmp(u16 label){
    reserve(1 + 1);
    emit(Bytecode::JMP);
    emit(label);
};
void BytecodeFile::jmp(Bytecode op, Reg checkReg, u16 labelT, u16 labelF){
    reserve(1 + 1 + 1 + 1);
    emit(op);
    emit(checkReg);
    emit(labelT);
    emit(labelF);
};
void BytecodeFile::cmp(Bytecode op, Type type, Reg des, Reg lhs, Reg rhs){
    reserve(1 + 1 + 1 + 1 + 1 + 1);
    emit(Bytecode::CMP);
    emit(op);
    emit(type);
    emit(des);
    emit(lhs);
    emit(rhs);
};
void BytecodeFile::mov(Type type, Reg dest, Reg src){
    reserve(1 + 1 + 1 + 1);
    emit(Bytecode::MOV);
    emit(type);
    emit(dest);
    emit(src);
};
void BytecodeFile::movConst(Reg reg, s64 num){
    reserve(1 + 1 + 1 + const_in_stream);
    emit(Bytecode::MOV_CONST);
    emit(Type::COMP_INTEGER);
    emit(reg);
    emitConstInt(num);
};
void BytecodeFile::movConst(Reg outputReg, f64 num){
    reserve(1 + 1 + 1 + const_in_stream);
    emit(Bytecode::MOV_CONST);
    emit(Type::COMP_DECIMAL);
    emit(outputReg);
    emitConstDec(num);
};
void BytecodeFile::binOp(Bytecode op, Type type, Reg outputReg, Reg lhsReg, Reg rhsReg){
    reserve(1 + 1 + 1 + 1 + 1);
    emit(op);
    emit(type);
    emit(outputReg);
    emit(lhsReg);
    emit(rhsReg);
};
void BytecodeFile::cast(Type finalType, Reg finalReg, Type type, Reg reg){
    reserve(1 + 1 + 1 + 1 + 1);
    emit(Bytecode::CAST);
    emit(finalType);
    emit(finalReg);
    emit(type);
    emit(reg);
};
void BytecodeFile::set(Bytecode op, Reg outputReg, Reg inputReg){
    reserve(1 + 1 + 1);
    emit(op);
    emit(outputReg);
    emit(inputReg);
};
void BytecodeFile::neg(Type type, Reg newReg, Reg reg){
    reserve(1 + 1 + 1 + 1);
    emit(Bytecode::NEG);
    emit(type);
    emit(newReg);
    emit(reg);
};
void BytecodeFile::ret(Reg reg, bool isVoid){
    reserve(1 + 1 + 1);
    emit(Bytecode::RET);
    emit((Bytecode)isVoid);
    emit(reg);
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
    u32  *varToReg;
    Type *varTypes;
    u32 registerID;

    void init(u32 varCount, u32 procCount, Reg rgsID){
	varToReg = nullptr;
	registerID = rgsID;
	if(varCount != 0){
	    varToReg = (u32*)mem::alloc(sizeof(u32)*varCount);
	    varTypes = (Type*)mem::alloc(sizeof(Type)*varCount);
	};
	procToID.len = 0;
	if(procCount != 0){
	    procToID.init(procCount);
	};
    };
    void uninit(){
	if(varToReg != nullptr){
	    mem::free(varToReg);
	    mem::free(varTypes);
	};
	if(procToID.len != 0){procToID.uninit();};
    };
    Reg newReg(Type type){
	Reg reg = registerID;
	registerID += 1;
	varTypes[reg] = type;
	return reg;
    };
};
Expr compileExprToBytecode(ASTBase *node, DynamicArray<ScopeEntities*> &see, DynamicArray<BytecodeContext> &bca, BytecodeFile &bf);
Expr emitBinOpBc(Bytecode binOp, ASTBase *node, DynamicArray<BytecodeContext> &bca, BytecodeFile &bf, DynamicArray<ScopeEntities*> &see){
    Expr out;
    ASTBinOp *op = (ASTBinOp*)node;
    BytecodeContext &bc = bca[bca.count-1];
    auto lhs = compileExprToBytecode(op->lhs, see, bca, bf);
    auto rhs = compileExprToBytecode(op->rhs, see, bca, bf);
    Type ansType = greaterType(lhs.type, rhs.type);
    Reg outputReg = bc.newReg(ansType);
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
Expr compileExprToBytecode(ASTBase *node, DynamicArray<ScopeEntities*> &see, DynamicArray<BytecodeContext> &bca, BytecodeFile &bf){
    Expr out = {};
    ASTType type = node->type;
    BytecodeContext &bc = bca[bca.count - 1];
    Reg outputReg;
    switch(type){
    case ASTType::STRING:{
	ASTString *string = (ASTString*)node;
	GlobalStrings::addEntryIfRequired(string->str);
	//TODO: 
    }break;
    case ASTType::NUM_INTEGER:{
	const Type type = Type::COMP_INTEGER;
	outputReg = bc.newReg(type);
	ASTNumInt *numInt = (ASTNumInt*)node;
	s64 num = numInt->num;
	bf.movConst(outputReg, num);
	out.reg = outputReg;
	out.type = type;
	return out;
    }break;
    case ASTType::NUM_DECIMAL:{
	const Type type = Type::COMP_DECIMAL;
	outputReg = bc.newReg(type);
	ASTNumDec *numDec = (ASTNumDec*)node;
	f64 num = numDec->dec;
	bf.movConst(outputReg, num);
	out.reg = outputReg;
	out.type = type;
	return out;
    }break;
    case ASTType::VARIABLE:{
	ASTVariable *var = (ASTVariable*)node;
	s32 off = getVarEntityScopeOff(var->name, see); //NOTE: we do not need to check if off == -1 as the checker would have raised a problem
	ScopeEntities *se = see[off];
	u32 id = se->varMap.getValue(var->name);
	BytecodeContext &correctBC = bca[off];
	out.reg = correctBC.varToReg[id];
	out.type = correctBC.varTypes[id];
	return out;
    }break;
    case ASTType::BIN_ADD:{									
	return emitBinOpBc(Bytecode::ADD, node, bca, bf, see);
    }break;
    case ASTType::BIN_MUL:{
	return emitBinOpBc(Bytecode::MUL, node, bca, bf, see);
    }break;
    case ASTType::BIN_DIV:{
	return emitBinOpBc(Bytecode::DIV, node, bca, bf, see);
    }break;
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
	Reg cmpOutReg = bc.newReg(ansType);					
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
	    newReg = bc.newReg((Type)((u32)operand.type-1));
	}else{
	    newReg = bc.newReg(operand.type);
	};
	bf.neg(operand.type, newReg, operand.reg);

	return operand;
    }break;
    default:
	UNREACHABLE;
	break;
    };
    return out;
};
void compileToBytecode(ASTBase *node, DynamicArray<ScopeEntities*> &see, DynamicArray<BytecodeContext> &bca, BytecodeFile &bf){
    BytecodeContext &bc = bca[bca.count-1];
    ScopeEntities *se = see[see.count-1];
    ASTType type = node->type;
    switch(type){
    case ASTType::RETURN:{
	ASTReturn *ret = (ASTReturn*)node;
	if(ret->expr == nullptr){
	    bf.ret(0, true);
	}else{
	    auto rhs = compileExprToBytecode(ret->expr, see, bca, bf);
	    bf.ret(rhs.reg, false);
	};
    }break;
    case ASTType::UNI_DECLERATION:{
	ASTUniVar *var = (ASTUniVar*)node;
	u32 id = se->varMap.getValue(var->name);
	const VariableEntity &entity = se->varEntities[id];
	//TODO: check which architecture we are building for
	if(entity.type == Type::STRUCT){
	    //TODO: allocate a new id for a new struct
	    Reg reg = bc.newReg(Type::STRUCT);
	    bf.alloc(Type::STRUCT, reg);
	}else{
	    Reg reg = bc.newReg(entity.type);
	    if(isFloat(entity.type)){
		bf.movConst(reg, (f64)0.0);
	    }else{
		bf.movConst(reg, (s64)0);
	    };
	};
    }break;
    case ASTType::MULTI_DECLERATION:{
	ASTMultiVar *var = (ASTMultiVar*)node;
	DynamicArray<String> &names = var->names;
	u32 firstID = se->varMap.getValue(names[0]);
	const VariableEntity &firstEntityID = se->varEntities[firstID];
	Type firstEntityType = firstEntityID.type;
	for(u32 x=0; x<names.count; x+=1){
	    Reg reg = bc.newReg(firstEntityType);
	    if(isFloat(firstEntityType)){
		bf.movConst(reg, (f64)0.0);
	    }else{
		bf.movConst(reg, (s64)0);
	    };
	};
    }break;
    case ASTType::UNI_INITIALIZATION_T_KNOWN:
    case ASTType::UNI_INITIALIZATION_T_UNKNOWN:{
ASTUniVar *var = (ASTUniVar*)node;
	u32 id = se->varMap.getValue(var->name);
	const VariableEntity &entity = se->varEntities[id];
	Flag flag = entity.flag;
	if(IS_BIT(flag, Flags::UNINITIALIZED) || IS_BIT(flag, Flags::ALLOC)){
	    //NOTE: UNI_ASSIGNMENT_T_UNKOWN wont reach here as checking stage would have raised an error
	    Reg rhsReg = bc.newReg(entity.type);
	    bf.alloc(entity.type, rhsReg);
	    bc.varToReg[id] = rhsReg;
	}else{
	    auto rhs = compileExprToBytecode(var->rhs, see, bca, bf);
	    bc.varToReg[id] = rhs.reg;
	};
    }break;
    case ASTType::MULTI_INITIALIZATION_T_KNOWN:
    case ASTType::MULTI_INITIALIZATION_T_UNKNOWN:{
	ASTMultiVar *var = (ASTMultiVar*)node;
	DynamicArray<String> &names = var->names;
	u32 firstID = se->varMap.getValue(names[0]);
	const VariableEntity &firstEntity = se->varEntities[firstID];
	Flag flag = firstEntity.flag;
	Type type = firstEntity.type;
	if(IS_BIT(flag, Flags::UNINITIALIZED) || IS_BIT(flag, Flags::ALLOC)){
	    //NOTE: MULTI_ASSIGNMENT_T_UNKOWN wont reach here as checking stage would have raised an error
	    for(u32 x=0; x<names.count; x+=1){
		Reg rhsReg = bc.newReg(type);
		bf.alloc(type, rhsReg);
		bc.varToReg[x] = rhsReg;
	    };
	}else{
	    auto rhs = compileExprToBytecode(var->rhs, see, bca, bf);
	    Bytecode byte;
	    for(u32 x=0; x<names.count; x+=1){
		u32 id = se->varMap.getValue(names[x]);
		const VariableEntity &entity = se->varEntities[id];
		Reg regID = bc.newReg(rhs.type);
		bc.varToReg[id] = regID;
		bc.varTypes[regID] = rhs.type;
		bf.mov(type, regID, rhs.reg);
	    };
	};
    }break;
    case ASTType::FOR:{
	ASTFor *For = (ASTFor*)node;
	ScopeEntities *ForSe = For->ForSe;
	BytecodeContext &blockBC = bca.newElem();
	blockBC.init(ForSe->varMap.count, ForSe->procMap.count, bc.registerID);
	see.push(ForSe);
	switch(For->loopType){
	case ForType::FOR_EVER:{
	    u16 loopStartLbl = newLabel();
	    bf.label(loopStartLbl);
	    for(u32 x=0; x<For->body.count; x+=1){
		compileToBytecode(For->body[x], see, bca, bf);
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
		compileToBytecode(For->body[x], see, bca, bf);
	    };
	    bf.label(loopEndLbl);
	}break;
	case ForType::C_LES:
	case ForType::C_EQU:{
	    ASTUniVar *var = (ASTUniVar*)For->body[0];
	    u32 id = ForSe->varMap.getValue(var->name);
	    VariableEntity &ent = ForSe->varEntities[id];
	    u16 loopStartLbl = newLabel();
	    u16 loopBodyLbl = newLabel();
	    u16 loopEndLbl = newLabel();

	    u16 incrementReg;
	    if(For->increment == nullptr){
		incrementReg = blockBC.newReg(ent.type);
		bf.movConst(incrementReg, (s64)1);
	    };
	    compileToBytecode(For->body[0], see, bca, bf);
	    Reg varRegPtr = blockBC.registerID-1;    //FIXME: 
	    bf.label(loopStartLbl);
	    
	    auto cond = compileExprToBytecode(For->end, see, bca, bf);
	    Bytecode setOp;
	    if(For->loopType == ForType::C_EQU){
		setOp = Bytecode::SETE;
	    }else{
		setOp = Bytecode::SETL;
	    };
	    Reg cmpOutReg = blockBC.newReg(ent.type);
	    Reg varVal1 = blockBC.newReg(ent.type);
	    bf.load(ent.type, varVal1, varRegPtr);
	    bf.cmp(setOp, ent.type, cmpOutReg, varVal1, cond.reg);
	    bf.jmp(Bytecode::JMPS, cmpOutReg, loopBodyLbl, loopEndLbl);

	    bf.label(loopBodyLbl);
	    for(u32 x=1; x<For->body.count; x+=1){
		compileToBytecode(For->body[x], see, bca, bf);
	    };

	    if(For->increment != nullptr){
		incrementReg = compileExprToBytecode(For->increment, see, bca, bf).reg;
	    };
	    Reg tempRes = blockBC.newReg(ent.type);
	    Reg varVal2  = blockBC.newReg(ent.type);
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
	BytecodeContext &blockBC = bca.newElem();
	ScopeEntities *IfSe = If->IfSe;
	blockBC.init(IfSe->varMap.count, IfSe->procMap.count, bc.registerID);
	see.push(IfSe);
	u16 inIfLbl = newLabel();
	u16 outIfLbl = newLabel();
	u16 inElseLbl = outIfLbl;
	if(If->elseBody.count != 0){
	    inElseLbl = newLabel();
	};
	bf.jmp(Bytecode::JMPS, exprID, inIfLbl, inElseLbl);
	for(u32 x=0; x<If->body.count; x+=1){
	    compileToBytecode(If->body[x], see, bca, bf);
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
	    BytecodeContext &elseBC = bca.newElem();
	    elseBC.init(ElseSe->varMap.count, ElseSe->procMap.count, bc.registerID);
	    for(u32 x=0; x<If->elseBody.count; x+=1){
		compileToBytecode(If->elseBody[x], see, bca, bf);
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
	ScopeEntities *procSE = proc->se;
	BytecodeContext &procBC = bca.newElem();
        procBC.init(procSE->varMap.count, procSE->procMap.count, bc.registerID);
	//        DEF    OUTPUT_COUNT         OUTPUT       ID     INPUT_COUNT                  INPUT
	bf.reserve(1  + const_in_stream + proc->out.count + 1 + const_in_stream + (proc->uniInCount + proc->multiInInputCount)*2);
	bf.emit(Bytecode::DEF);
	bf.emitConstInt(proc->out.count);
	for(u32 x=0; x<proc->out.count; x+=1){
	    AST_Type *type = (AST_Type*)proc->out[x];
	    bf.emit(type->type);
	};
	bf.emit((Bytecode)procBytecodeID);
	u32 inCount = proc->uniInCount + proc->multiInCount;
	bf.emitConstInt(inCount);
	for(u32 x=0; x<inCount;){
	    ASTBase *node = proc->body[x];
	    switch(node->type){
	    case ASTType::UNI_DECLERATION:{
		ASTUniVar *var = (ASTUniVar*)node;
		u32 id = procSE->varMap.getValue(var->name);
		const VariableEntity &entity = procSE->varEntities[id];
		Reg regID = procBC.newReg(entity.type);
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
		    Reg regID = procBC.newReg(entity.type);
		    procBC.varToReg[id] = regID;
		    bf.emit(entity.type);
		    bf.emit(regID);
		};
		x += var->names.count;
	    }break;
	    };
	};
	//RESERVE ENDS HERE
	see.push(procSE);
	for(u32 x=inCount; x<proc->body.count; x+=1){
	    compileToBytecode(proc->body[x], see, bca, bf);
	};
	see.pop()->uninit();
	bf.procEnd();
	mem::free(procSE);
	bca.pop().uninit();
    }break;
    case ASTType::STRUCT_DEFENITION:
    case ASTType::IMPORT: break;
    default:
	UNREACHABLE;
	break;
    };
};
void compileASTNodesToBytecode(DynamicArray<ASTBase*> &nodes, DynamicArray<ScopeEntities*> &see, DynamicArray<BytecodeContext> &bca, BytecodeFile &bf){
    for(u32 x=0; x<nodes.count; x+=1){
	ASTBase *node = nodes[x];
	compileToBytecode(node, see, bca, bf);
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
	case Type::S_8: printf("s8");break;
	case Type::U_8: printf("u8");break;
	case Type::COMP_INTEGER: printf("comp_int");break;
	case Type::COMP_DECIMAL: printf("comp_dec");break;
	default:
	    UNREACHABLE;
	    return;
	};
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
	case Bytecode::MOV_CONST:{
	    printf("mov_const");
	    DUMP_TYPE;
	    DUMP_REG;
	    s64 num = getConstIntAndUpdate(buc->bytecodes, x);
	    printf("%lld", num);
	}break;
	case Bytecode::MOV:{
	    if(flag){printf("mov");};
	    DUMP_TYPE;
	    DUMP_REG;
	    DUMP_REG;	    
	}break;
	case Bytecode::ADD:{
	    if(flag){printf("add");};
	    DUMP_TYPE;
	    DUMP_REG;
	    DUMP_REG;
	    DUMP_REG;
	}break;
	case Bytecode::SUB:{
	    if(flag){printf("sub");};
	    DUMP_TYPE;
	    DUMP_REG;
	    DUMP_REG;
	    DUMP_REG;;
	}break;
	case Bytecode::MUL:{
	    if(flag){printf("mul");};
	    DUMP_TYPE;
	    DUMP_REG;
	    DUMP_REG;
	    DUMP_REG;
	}break;
	case Bytecode::DIV:{
	    if(flag){printf("div");};
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
	    printf("def (");
	    s64 outCount = getConstIntAndUpdate(buc->bytecodes, x);
	    while(outCount != 0){
		bc = getBytecode(buc, x);
		dumpType((Type)bc);
		printf(",");
		outCount -= 1;
	    };
	    bc = getBytecode(buc, x);
	    printf(") _%d(", (u32)bc);
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
	    DUMP_TYPE
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
