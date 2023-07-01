void ScopeEntities::init(u32 varCount, u32 procCount){
    varMap.len = 0;
    procMap.len = 0;
    varMap.count = 0;
    procMap.count = 0;
    if(varCount != 0){
	varMap.init(varCount);
	varEntities = (VariableEntity*)mem::alloc(sizeof(VariableEntity) * varCount);
    };
    if(procCount != 0){
	procMap.init(procCount);
	procEntities = (ProcEntity*)mem::alloc(sizeof(ProcEntity) * procCount);
    };
};
void ScopeEntities::uninit(){
    if(varMap.len != 0){
	varMap.uninit();
	mem::free(varEntities);
    };
    if(procMap.len != 0){
	procMap.uninit();
        mem::free(procEntities);
    };
};

ScopeEntities* pushNewScope(DynamicArray<ScopeEntities*> &see, Scope scope){
    ScopeEntities *se = (ScopeEntities*)mem::alloc(sizeof(ScopeEntities));
    se->scope = scope;
    see.push(se);
    return se;
};

bool checkVarEntityPresentInScopeElseReg(Lexer &lexer, String name, Flag flag, Type type, ScopeEntities *se) {
    Map &map = se->varMap;
    BRING_TOKENS_TO_SCOPE;
    if (map.getValue(name) != -1) {
	return false;
    };
    u32 id = map.count;
    map.insertValue(name,id);
    VariableEntity entity;
    entity.name = name;
    entity.type = type;
    entity.flag = flag;
    se->varEntities[id] = entity;
    return true;
};
bool checkProcEntityPresentInScopeElseReg(Lexer &lexer, String name, Flag flag, ScopeEntities *se, ScopeEntities *procSe) {
    Map &map = se->procMap;
    BRING_TOKENS_TO_SCOPE;
    if (map.getValue(name) != -1) {
	return false;
    };
    u32 id = map.count;
    map.insertValue(name,id);
    ProcEntity entity;
    entity.name = name;
    entity.flag = flag;
    entity.se = procSe;
    se->procEntities[id] = entity;
    return true;
};
void goThroughEntitiesAndInitScope(DynamicArray<ASTBase*> &entities, ScopeEntities *se) {
    u32 procCount = 0;
    u32 varCount = 0;
    for (u32 x = 0; x < entities.count; x += 1) {
	ASTBase *node = entities[x];
	if (node->type == ASTType::PROC_DEFENITION) {
	    procCount += 1;
	} else if (node->type >= ASTType::UNI_DECLERATION && node->type <= ASTType::UNI_ASSIGNMENT_T_KNOWN) {
	    varCount += 1;
	} else if (node->type >= ASTType::MULTI_DECLERATION && node->type <= ASTType::MULTI_ASSIGNMENT_T_KNOWN){
	    ASTMultiVar *multi = (ASTMultiVar*)node;
	    varCount += multi->names.count;
	};
    };
    se->init(varCount, procCount);
};
bool checkVarDecl(ASTBase *base, Lexer &lexer, ScopeEntities *se, bool isSingle){
    BRING_TOKENS_TO_SCOPE;
    if(isSingle){
	ASTUniVar *var = (ASTUniVar*)base;
	Type type = tokenKeywordToType(lexer, var->tokenOff+2);
	if(type == Type::UNKOWN){
	    lexer.emitErr(tokOffs[var->tokenOff+2].off, "Unkown type");
	    return false;
	};
	if(checkVarEntityPresentInScopeElseReg(lexer, var->name, var->flag, type, se) == false){
	    BRING_TOKENS_TO_SCOPE;
	    lexer.emitErr(tokOffs[var->tokenOff].off, "Variable redecleration");
	    return false;
	};
	return true;
    };
    ASTMultiVar *var = (ASTMultiVar*)base;
    Type type = tokenKeywordToType(lexer, var->tokenOff+2);
    if(type == Type::UNKOWN){
	lexer.emitErr(tokOffs[var->tokenOff+2].off, "Unkown type");
	return false;
    };
    for(u32 x=0; x!=var->names.count; x+=1){
	String &name = var->names[x];
	if(checkVarEntityPresentInScopeElseReg(lexer, name, var->flag, type, se) == false){
	    BRING_TOKENS_TO_SCOPE;
	    lexer.emitErr(tokOffs[var->tokenOff - var->names.count - var->names.count + x*2 + 2].off, "Variable redecleration");
	    return false;
	};
    };
    return true;
};
bool checkVarDef(ASTBase *base, Lexer &lexer, DynamicArray<ScopeEntities*> &see, bool tKown, bool isSingle){
    BRING_TOKENS_TO_SCOPE;
    ScopeEntities *se = see[see.count-1];
    if(isSingle){
	ASTUniVar *var = (ASTUniVar*)base;
	Type type;
	Flag &flag = var->flag;
	Flag treeFlag;
	Type treeType = getTreeType(var->rhs, treeFlag, see, lexer);
        if(treeType == Type::UNKOWN){return false;};
	if(tKown){
	    type = tokenKeywordToType(lexer, var->tokenOff+2);
	    if(type == Type::UNKOWN){
		lexer.emitErr(tokOffs[var->tokenOff+2].off, "Unkown type");
		return false;
	    };
	    if((u32)treeType < (u32)type){
		lexer.emitErr(tokOffs[var->tokenOff].off, "Type of expression tree does not match declared type");
		return false;
	    };
	}else{
	    type = treeType;
	};
	flag |= treeFlag;
	if(checkVarEntityPresentInScopeElseReg(lexer, var->name, flag, type, se) == false){
	    lexer.emitErr(tokOffs[var->tokenOff].off, "Variable redefenition");
	    return false;
	};
	return true;
    };
    ASTMultiVar *var = (ASTMultiVar*)base;
    Type type;
    Flag &flag = var->flag;
    Flag treeFlag = 0;
    Type treeType = getTreeType(var->rhs, treeFlag, see, lexer);
    if(treeType == Type::UNKOWN){return false;};
    if(tKown){
	type = tokenKeywordToType(lexer, var->tokenOff+2);
	if(type == Type::UNKOWN){
	    lexer.emitErr(tokOffs[var->tokenOff+2].off, "Unkown type");
	    return false;
	};
	if((u32)treeType < (u32)type){
	    lexer.emitErr(tokOffs[var->tokenOff].off, "Size of expression is greater than declared size");
	    return false;
	};
    }else{
	type = treeType;
    };
    flag |= treeFlag;
    DynamicArray<String> &names = var->names;
    for(u32 x=0; x<names.count; x+=1){
	if(checkVarEntityPresentInScopeElseReg(lexer, names[x], flag, type, se) == false){
	    lexer.emitErr(tokOffs[var->tokenOff - var->names.count - var->names.count + x*2 + 2].off, "Variable redefenition");
	    return false;
	};
    };
    return true;
};
bool checkType(ASTBase *node, Lexer &lexer, DynamicArray<ScopeEntities*> &see){
    BRING_TOKENS_TO_SCOPE;
    AST_Type *typeNode = (AST_Type*)node;
    Token_Type tokType = tokTypes[typeNode->tokenOff];
    Type type;
    switch(tokType){
    case Token_Type::K_U8: type = Type::U_8; break;
    case Token_Type::K_S8: type = Type::S_8; break;
    case Token_Type::K_U16: type = Type::U_16; break;
    case Token_Type::K_S16: type = Type::S_16; break;
    case Token_Type::K_F16: type = Type::F_16; break;
    case Token_Type::K_U32: type = Type::U_32; break;
    case Token_Type::K_S32: type = Type::S_32; break;
    case Token_Type::K_F32: type = Type::F_32; break;
    default: return false;
    };
    typeNode->type = type;
    return true;
};
VariableEntity *getVarEntity(String name, DynamicArray<ScopeEntities*> &see){
    for(u32 x=see.count; x>0; x-=1){
	x -= 1;
	ScopeEntities *se = see[x];
	s32 k = se->varMap.getValue(name);
	if(k != -1){return se->varEntities+k;};
	if(se->scope == Scope::PROC){return nullptr;};
    };
    return nullptr;
};
bool checkEntities(DynamicArray<ASTBase*> &entities, Lexer &lexer, DynamicArray<ScopeEntities*> &see);
bool checkEntity(ASTBase* node, Lexer &lexer, DynamicArray<ScopeEntities*> &see){
    ScopeEntities *se = see[see.count-1]; //current scope
    switch (node->type) {
    case ASTType::PROC_DEFENITION: {
	BRING_TOKENS_TO_SCOPE;
	ASTProcDef *proc = (ASTProcDef*)node;
	String name = proc->name;
	ScopeEntities *procSE = pushNewScope(see, Scope::PROC);
	if(checkProcEntityPresentInScopeElseReg(lexer, name, proc->flag, se, procSE) == false){
	    lexer.emitErr(tokOffs[proc->tokenOff].off, "Procedure redecleration");
	    return false;
	};
	for(u32 x=0; x<proc->out.count; x+=1){
	    ASTBase *node = proc->out[x];
	    switch(node->type){
	    case ASTType::TYPE:{
		checkType(node, lexer, see);
	    }break;
	    default:
		lexer.emitErr(tokOffs[proc->tokenOff].off, "Invalid output statement for procedure defenition");
		return false;
	    };
	};
	//NOTE: checking body checks the input also
	if(checkEntities(proc->body, lexer, see) == false) { return false; };
	see.pop();
    } break;
    case ASTType::UNI_DECLERATION:{
	if(checkVarDecl(node, lexer, se, true) == false){ return false;};
    }break;
    case ASTType::MULTI_DECLERATION:{
	if(checkVarDecl(node, lexer, se, false) == false){ return false;};
    }break;
    case ASTType::MULTI_ASSIGNMENT_T_KNOWN:{
	ASTMultiVar *var = (ASTMultiVar*)node;
	if(checkVarDef(node, lexer, see, true, false) == false){return false;};
	if(checkEntity(var->rhs, lexer, see) == false){return false;};
    }break;
    case ASTType::MULTI_ASSIGNMENT_T_UNKNOWN: {
	ASTMultiVar *var = (ASTMultiVar*)node;
	if(checkVarDef(node, lexer, see, false, false) == false){return false;};
	if(checkEntity(var->rhs, lexer, see) == false){return false;};
    } break;
    case ASTType::UNI_ASSIGNMENT_T_KNOWN:{
	ASTUniVar *var = (ASTUniVar*)node;
	if(checkVarDef(node, lexer, see, true, true) == false){return false;};
	if(checkEntity(var->rhs, lexer, see) == false){return false;};
    }break;
    case ASTType::UNI_ASSIGNMENT_T_UNKNOWN: {
	ASTUniVar *var = (ASTUniVar*)node;
	if(checkVarDef(node, lexer, see, false, true) == false){return false;};
	if(checkEntity(var->rhs, lexer, see) == false){return false;};
    } break;
    case ASTType::LOG_GRT:{
	ASTBinOp *op = (ASTBinOp*)node;
	Flag flag;
	Type lhsType = getTreeType(op->lhs, flag, see, lexer);
	if(lhsType == Type::UNKOWN){return false;};
	if(isTypeNum(lhsType) == false){return false;};
	Type rhsType = getTreeType(op->rhs, flag, see, lexer);
	if(rhsType == Type::UNKOWN){return false;};
	if(isTypeNum(rhsType) == false){return false;};
	op->lhsType = lhsType;
	op->rhsType = rhsType;
    }break;
    case ASTType::BIN_MUL:
    case ASTType::BIN_DIV:
    case ASTType::BIN_ADD:{
	ASTBinOp *op = (ASTBinOp*)node;
	Flag flag;
	Type lhsType = getTreeType(op->lhs, flag, see, lexer);
	if(lhsType == Type::UNKOWN){return false;};
	if(isTypeNum(lhsType) == false){return false;};
	Type rhsType = getTreeType(op->rhs, flag, see, lexer);
	if(rhsType == Type::UNKOWN){return false;};
	if(isTypeNum(rhsType) == false){return false;};
	op->lhsType = lhsType;
	op->rhsType = rhsType;
    }break;
    case ASTType::NUM_INTEGER:
    case ASTType::NUM_DECIMAL: break;
    case ASTType::VARIABLE:{
	ASTVariable *var = (ASTVariable*)node;
	if(getVarEntity(var->name, see) == nullptr){return false;};
    }break;
    case ASTType::UNI_NEG:{
	ASTUniOp *op = (ASTUniOp*)node;
        switch(op->node->type){
	case ASTType::NUM_INTEGER:
	case ASTType::NUM_DECIMAL: break;
	case ASTType::VARIABLE:{
	    ASTVariable *var = (ASTVariable*)op->node;
	    Type type = getVarEntity(var->name, see)->type;
	    if(isTypeNum(type) == false){return false;};
	}break;
	};
    }break;
    default: DEBUG_UNREACHABLE;return false;
    };
    return true;
};
bool checkEntities(DynamicArray<ASTBase*> &entities, Lexer &lexer, DynamicArray<ScopeEntities*> &see){
    TIME_BLOCK;
    BRING_TOKENS_TO_SCOPE;
    ScopeEntities *se = see[see.count-1];
    goThroughEntitiesAndInitScope(entities, se);
    for (u32 x=0; x<entities.count; x+=1) {
	ASTBase *node = entities[x];
	if(checkEntity(node, lexer, see) == false){return false;};
    };
    return true;
};
void freeNodeInternal(ASTBase *base){
    switch(base->type){
    case ASTType::MULTI_DECLERATION:
    case ASTType::MULTI_ASSIGNMENT_T_UNKNOWN:
    case ASTType::MULTI_ASSIGNMENT_T_KNOWN:{
	ASTMultiVar *mv = (ASTMultiVar*)base;
	mv->names.uninit();
    }break;
    case ASTType::PROC_DEFENITION:{
	ASTProcDef *proc = (ASTProcDef*)base;
        if(proc->body.len != 0){
	    for(u32 x=0; x<proc->body.count; x+=1){
		freeNodeInternal(proc->body[x]);
	    };
	    proc->body.uninit();
	};
	if(proc->out.len != 0){proc->out.uninit();};
    }break;
    };
};
