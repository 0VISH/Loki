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
VariableEntity *getVarEntity(String name, DynamicArray<ScopeEntities*> &see){
    for(u32 x=see.count; x>0; x-=1){
	x -= 1;
	ScopeEntities *se = see[x];
	s32 k = se->varMap.getValue(name);
	if(k != -1){return se->varEntities+k;};
	if(se->scope == Scope::PROC){
	    ScopeEntities *globalScope = see[0];
	    k = globalScope->varMap.getValue(name);
	    if(k != -1){return globalScope->varEntities+k;};
	    return nullptr;
	};
    };
    return nullptr;
};
bool checkExpression(ASTBase *node, Lexer &lexer, DynamicArray<ScopeEntities*> &see){
    BRING_TOKENS_TO_SCOPE;
    ScopeEntities *se = see[see.count-1];
    switch(node->type){
    case ASTType::BIN_GRT:
    case ASTType::BIN_GRTE:
    case ASTType::BIN_LSR:
    case ASTType::BIN_LSRE:
    case ASTType::BIN_EQU:
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
    default:
	UNREACHABLE;
	return false;
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
bool checkEntities(DynamicArray<ASTBase*> &entities, Lexer &lexer, DynamicArray<ScopeEntities*> &see);
bool checkEntity(ASTBase* node, Lexer &lexer, DynamicArray<ScopeEntities*> &see){
    BRING_TOKENS_TO_SCOPE;
    ScopeEntities *se = see[see.count-1]; //current scope
    switch (node->type) {
    case ASTType::FOR:{
	ASTFor *For = (ASTFor*)node;
	switch(For->loopType){
	case ForType::FOR_EVER:{
	    ScopeEntities *ForSe = pushNewScope(see, Scope::BLOCK);
	    For->ForSe = ForSe;
	    if(checkEntities(For->body, lexer, see) == false){return false;};
	    see.pop();
	}break;
	case ForType::C_LES:
	case ForType::C_EQU:{
	    if(checkExpression(For->end, lexer, see) == false){return false;};
	    Flag endTreeFlag = 0;
	    Type endTreeType = getTreeType(For->end, endTreeFlag, see, lexer);
	    if(isTypeNum(endTreeType) == false){
		lexer.emitErr(tokOffs[For->endOff].off, "Upper bound type is not a number");
		return false;
	    };
	    Type incrementTreeType = Type::UNKOWN;
	    if(For->increment != nullptr){
		if(checkExpression(For->increment, lexer, see) == false){return false;};
		Flag incrementTreeFlag = 0;
		incrementTreeType = getTreeType(For->increment, endTreeFlag, see, lexer);
		if(isTypeNum(incrementTreeType) == false){
		    lexer.emitErr(tokOffs[For->incrementOff].off, "Upper bound type is not a number");
		    return false;
		};
	    };
	    ScopeEntities *ForSe = pushNewScope(see, Scope::BLOCK);
	    For->ForSe = ForSe;
	    if(checkEntities(For->body, lexer, see) == false){return false;};
	    see.pop();
	    ASTUniVar *var = (ASTUniVar*)For->body[0];
	    u32 id = ForSe->varMap.getValue(var->name);
	    VariableEntity &ent = ForSe->varEntities[id];
	    if(isTypeNum(ent.type) == false){
		lexer.emitErr(tokOffs[For->endOff].off, "Iterator should have a numeric type");
		return false;
	    };
	    if(isSameTypeRange(ent.type, endTreeType) == false){
		lexer.emitErr(tokOffs[For->endOff].off, "Iterator is not the same type as upper bound expression. Explicit cast required");
		return false;
	    };
	    if(incrementTreeType != Type::UNKOWN){
		if(isSameTypeRange(ent.type, incrementTreeType) == false){
		    lexer.emitErr(tokOffs[For->endOff].off, "Iterator is not the same type as increment expression. Explicit cast required");
		    return false;
		};
	    };
	}break;
	};
    }break;
    case ASTType::IF:{
	BRING_TOKENS_TO_SCOPE;
	ASTIf *If = (ASTIf*)node;
	if(checkExpression(If->expr, lexer, see) == false){return false;};
	Flag treeFlag;
	Type treeType = getTreeType(If->expr, treeFlag, see, lexer);
	if(treeType == Type::UNKOWN){return false;};
	switch(treeType){
	case Type::XE_VOID:
	    lexer.emitErr(tokOffs[If->tokenOff].off, "Expression under 'if' cannot be void");
	    return false;
	};
        ScopeEntities *IfSe = pushNewScope(see, Scope::BLOCK);
	If->IfSe = IfSe;
	if(checkEntities(If->body, lexer, see) == false) { return false; };
	see.pop();
	if(If->elseBody.count == 0){break;};
	ScopeEntities *ElseSe = pushNewScope(see, Scope::BLOCK);
	If->ElseSe = ElseSe;
	if(checkEntities(If->elseBody, lexer, see) == false) { return false; };
	see.pop();
    }break;
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
    default:
	UNREACHABLE;
	return false;
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
