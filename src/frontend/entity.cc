void ScopeEntities::init(u32 varCount, u32 procCount, u32 structCount){
    varMap.len = 0;
    procMap.len = 0;
    structMap.len = 0;
    structMap.count = 0;
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
    if(structCount != 0){
	structMap.init(structCount);
	structEntities = (StructEntity*)mem::alloc(sizeof(StructEntity) * structCount);
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
    if(structMap.len != 0){
	structMap.uninit();
	for(u32 x=0; x<structMap.count; x+=1){
	    StructEntity *entity = structEntities + x;
	    entity->varToOff.uninit();
	};
	mem::free(structEntities);
    };
};

ScopeEntities* allocScopeEntity(Scope scope){
    ScopeEntities *se = (ScopeEntities*)mem::alloc(sizeof(ScopeEntities));
    se->scope = scope;
    return se;
};

void goThroughEntitiesAndInitScope(DynamicArray<ASTBase*> &entities, ScopeEntities *se) {
    u32 procCount = 0;
    u32 varCount = 0;
    u32 structCount = 0;
    for (u32 x = 0; x < entities.count; x += 1) {
	ASTBase *node = entities[x];
	if (node->type == ASTType::PROC_DEFENITION) {
	    procCount += 1;
	} else if (node->type >= ASTType::DECLERATION && node->type <= ASTType::INITIALIZATION_T_KNOWN) {
	    varCount += 1;
	} else if(node->type == ASTType::STRUCT_DEFENITION){
	    structCount += 1;
	};
    };
    se->init(varCount, procCount, structCount);
};

#define GET_ENTITY_TEMPLATE(MAP, ENTITIES)			\
    u32 x=see.count;						\
    while(x>0){							\
	x -= 1;							\
	ScopeEntities *se = see[x];				\
	if(se->MAP.len != 0){					\
	    s32 k = se->MAP.getValue(name);			\
	    if(k != -1){return se->ENTITIES+k;};		\
	};							\
	if(se->scope == Scope::PROC){				\
	    ScopeEntities *globalScope = see[0];		\
	    if(globalScope->MAP.len != 0){			\
		s32 k = globalScope->MAP.getValue(name);	\
		if(k != -1){return globalScope->ENTITIES+k;};	\
	    };							\
	    return nullptr;					\
	};							\
    };								\
    return nullptr;						\


s32 getVarEntityScopeOff(String name, DynamicArray<ScopeEntities*> &see){
    u32 x = see.count;
    while(x>0){
	x -= 1;
	ScopeEntities *se = see[x];
	if(se->varMap.len != 0){
	    s32 k = se->varMap.getValue(name);
	    if(k != -1){return k;};
	};
	if(se->scope == Scope::PROC){
	    ScopeEntities *globalScope = see[0];
	    if(globalScope->varMap.len != 0){
		s32 k = globalScope->varMap.getValue(name);
		if(k != -1){return k;};
	    };
	    return -1;
	};
    };
    return -1;
};
ProcEntity *getProcEntity(String name, DynamicArray<ScopeEntities*> &see){
    GET_ENTITY_TEMPLATE(procMap, procEntities);
};
VariableEntity *getVarEntity(String name, DynamicArray<ScopeEntities*> &see){
    GET_ENTITY_TEMPLATE(varMap, varEntities);
};
StructEntity *getStructEntity(String name, DynamicArray<ScopeEntities*> &see){
    GET_ENTITY_TEMPLATE(structMap, structEntities);
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
    case ASTType::STRING:
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
    case Token_Type::K_U32: type = Type::U_32; break;
    case Token_Type::K_S32: type = Type::S_32; break;
    case Token_Type::K_F32: type = Type::F_32; break;
    default: return false;
    };
    typeNode->type = type;
    return true;
};

ProcEntity *checkProcEntityPresentElseReg(String name, Flag flag, ScopeEntities *procSe, DynamicArray<ScopeEntities*> &see) {
    if(getProcEntity(name, see) != nullptr){return nullptr;};

    ScopeEntities *se = see[see.count-1];
    Map &map = se->procMap;

    u32 id = map.count + 1;
    if(cmpString(name, config.entryPoint)){
	id = 0;
    };
    map.insertValue(name,id);
    ProcEntity entity;
    entity.name = name;
    entity.flag = flag;
    entity.id   = id;
    se->procEntities[id] = entity;
    return se->procEntities + id;
};
bool checkVarEntityPresentInScopeElseReg(String name, Flag flag, Type type, DynamicArray<ScopeEntities*> &see){
    if(getVarEntity(name, see) != nullptr){return false;};
    
    ScopeEntities *se = see[see.count - 1];
    Map &map = se->varMap;
    
    u32 id = map.count;
    map.insertValue(name,id);
    VariableEntity entity;
    entity.name = name;
    entity.type = type;
    entity.flag = flag;
    se->varEntities[id] = entity;
    return true;
};

#define CHECK_TREE_AND_MERGE_FLAGS					\
    ASTBase *rhs = var->rhs;						\
    if(rhs->type != ASTType::MULTI_VAR_RHS){				\
	if(checkExpression(rhs, lexer, see) == false){return false;};	\
    }else{rhs = ((ASTMultiVarRHS*)rhs)->rhs;};   /* WOW :) */		\
    Flag treeFlag = 0;							\
    Type treeType = getTreeType(rhs, treeFlag, see, lexer);		\
    if(treeType == Type::UNKOWN){return false;};			\
    flag |= treeFlag;							\

#define IS_EXPLICIT_CAST_REQUIRED					\
    if(treeType < type){						\
	lexer.emitErr(tokOffs[var->tokenOff].off, "Explicit casting required as potential loss of information may occur"); \
	return false;							\
    };									\

bool checkEntities(DynamicArray<ASTBase*> &entities, Lexer &lexer, DynamicArray<ScopeEntities*> &see);
bool checkEntity(ASTBase* node, Lexer &lexer, DynamicArray<ScopeEntities*> &see){
    BRING_TOKENS_TO_SCOPE;
    ScopeEntities *se = see[see.count-1]; //current scope
    switch (node->type) {
    case ASTType::STRUCT_DEFENITION:{
	ASTStructDef *Struct = (ASTStructDef*)node;
	if(getStructEntity(Struct->name, see) != nullptr){
	    lexer.emitErr(tokOffs[Struct->tokenOff].off, "Struct redefenition");
	};
	Map &map = se->structMap;
	u32 id = map.count;
	map.insertValue(Struct->name, id);

	StructEntity entity;
	entity.varToOff.init(Struct->memberCount);
	u64 size = 0;
	ScopeEntities *StructSe = allocScopeEntity(Scope::BLOCK);
	see.push(StructSe);
	if(checkEntities(Struct->body, lexer, see) == false){return false;};
	for(u32 x=0; x<Struct->body.count; x+=1){
	    if(Struct->body[x]->type == ASTType::DECLERATION){
		ASTUniVar *var = (ASTUniVar*)Struct->body[x];
		if(entity.varToOff.getValue(var->name) != -1){
		    lexer.emitErr(tokOffs[var->tokenOff].off, "Member name already in use");
		    return false;
		};
		entity.varToOff.insertValue(var->name, size);
	    }else{
		//TODO: 
	    };
	};
	see.pop();
	StructSe->uninit();
	mem::free(StructSe);
	entity.size = size;
	se->structEntities[id] = entity;
    }break;
    case ASTType::DECLERATION:{
	ASTUniVar *var = (ASTUniVar*)node;
	Type type = tokenKeywordToType(var->typeTokenOff, lexer, see);
	if(type == Type::UNKOWN){return false;};
	SET_BIT(var->flag, Flags::CONSTANT);
	if(checkVarEntityPresentInScopeElseReg(var->name, var->flag, type, see) == false){
	    lexer.emitErr(tokOffs[var->tokenOff].off, "Variable redecleration");
	    return false;
	};
    }break;
    case ASTType::MULTI_VAR_RHS:{
	ASTMultiVarRHS *mvr = (ASTMultiVarRHS*)node;
	if(checkExpression(mvr->rhs, lexer, see) == false){return false;};
    }break;
    case ASTType::IMPORT:{
	ASTImport *imp = (ASTImport*)node;
	Dep::pushToParseCheckStack(imp->fileName);
    }break;
    case ASTType::FOR:{
	ASTFor *For = (ASTFor*)node;
	switch(For->loopType){
	case ForType::FOR_EVER:{
	    ScopeEntities *ForSe = allocScopeEntity(Scope::BLOCK);
	    see.push(ForSe);
	    For->ForSe = ForSe;
	    if(checkEntities(For->body, lexer, see) == false){return false;};
	    see.pop();
	}break;
	case ForType::EXPR:{
	    if(checkExpression(For->expr, lexer, see) == false){return false;};
	    ScopeEntities *ForSe = allocScopeEntity(Scope::BLOCK);
	    see.push(ForSe);
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
	    ScopeEntities *ForSe = allocScopeEntity(Scope::BLOCK);
	    For->ForSe = ForSe;
	    see.push(ForSe);
	    if(checkEntities(For->body, lexer, see) == false){return false;};
	    see.pop();
	    ASTUniVar *var = (ASTUniVar*)For->body[0];
	    u32 id = ForSe->varMap.getValue(var->name);
	    VariableEntity &ent = ForSe->varEntities[id];
	    if(ent.type == Type::UNKOWN){
		ent.type = endTreeType;
	    }else if(isTypeNum(ent.type) == false){
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
        ScopeEntities *IfSe = allocScopeEntity(Scope::BLOCK);
	If->IfSe = IfSe;
	see.push(IfSe);
	if(checkEntities(If->body, lexer, see) == false) { return false; };
	see.pop();
	if(If->elseBody.count == 0){break;};
	ScopeEntities *ElseSe = allocScopeEntity(Scope::BLOCK);
	If->ElseSe = ElseSe;
	see.push(ElseSe);
	if(checkEntities(If->elseBody, lexer, see) == false) { return false; };
	see.pop();
    }break;
    case ASTType::PROC_CALL:{
	ASTProcCall *pc = (ASTProcCall*)node;
	ProcEntity *pe = getProcEntity(pc->name, see);
	if(pe == nullptr){
	    lexer.emitErr(tokOffs[pc->off].off, "Procedure not defined: %.*s", pc->name.len, pc->name.mem);
	    return false;
	};
	if(pc->args.count != pe->argTypes.count){
	    lexer.emitErr(tokOffs[pc->off].off, "Procedure definition has %d argument%s but while calling %d argument%shas been passed", pe->argTypes.count, (pe->argTypes.count==1)?",":"s,", pc->args.count, (pc->args.count==1)?" ":"s ");
	    return false;
	};
	for(u32 x=0; x<pc->args.count; x+=1){
	    if(checkExpression(pc->args[x], lexer, see) == false){return false;};
	    Flag flag;
	    Type argTreeType = getTreeType(pc->args[x], flag, see, lexer);
	    Type argType = pe->argTypes[x];
	    if(isSameTypeRange(argTreeType, argType) == false){
		lexer.emitErr(tokOffs[pc->argOffs[x]].off, "Type mismatch. They don't belong to the same range");
	    };
	    if(argTreeType > argType){
		lexer.emitErr(tokOffs[pc->argOffs[x]].off, "Type of argument given is greater than defined. Explicit cast required");
	    };
	};
    }break;
    case ASTType::PROC_DEFENITION: {
	ASTProcDef *proc = (ASTProcDef*)node;
	String name = proc->name;
	
	ScopeEntities *procSE = allocScopeEntity(Scope::PROC);
	proc->se = procSE;
	ProcEntity *pe = checkProcEntityPresentElseReg(name, proc->flag, procSE, see);
	if(pe == nullptr){
	    lexer.emitErr(tokOffs[proc->tokenOff].off, "Procedure redecleration");
	    return false;
	};
	pe->retTypes.init();
	for(u32 x=0; x<proc->out.count; x+=1){
	    ASTBase *node = proc->out[x];
	    if(node->type != ASTType::TYPE){
		lexer.emitErr(tokOffs[proc->tokenOff].off, "Invalid output type for procedure defenition");
		return false;
	    };
	    checkType(node, lexer, see);
	    AST_Type *typeNode = (AST_Type*)node;
	    pe->retTypes.push(typeNode->type);
	};
	//NOTE: checking body checks the input also
	see.push(procSE);
	if(checkEntities(proc->body, lexer, see) == false) { return false; };
	see.pop();
	pe->argTypes.init();
	for(u32 x=0; x<proc->inCount; x+=1){
	    ASTBase *arg = proc->body[x];
	    switch(arg->type){
	    case ASTType::DECLERATION:{
		ASTUniVar *var = (ASTUniVar*)arg;
		Type type = tokenKeywordToType(var->typeTokenOff, lexer, see);
		pe->argTypes.push(type);
	    }break;
	    default:
		lexer.emitErr("Procedure defenition only takes declerations");
		return false;
	    };
	};
    } break;
    case ASTType::INITIALIZATION_T_UNKNOWN:{
	ASTUniVar *var = (ASTUniVar*)node;
	Flag flag = var->flag;
	if(IS_BIT(flag, Flags::UNINITIALIZED)){
	    lexer.emitErr(tokOffs[var->tokenOff].off, "Cannot keep the variable uninitialized since the type is unkown.");
	    return false;
	};
	CHECK_TREE_AND_MERGE_FLAGS;
	if(checkVarEntityPresentInScopeElseReg(var->name, flag, treeType, see) == false){
	    lexer.emitErr(tokOffs[var->tokenOff].off, "Variable redefinition");
	    return false;
	};
    }break;
    case ASTType::INITIALIZATION_T_KNOWN:{
	ASTUniVar *var = (ASTUniVar*)node;
	Type type = tokenKeywordToType(var->tokenOff + 2, lexer, see);
	Flag flag = var->flag;
	if(!IS_BIT(flag, Flags::UNINITIALIZED)){
	    CHECK_TREE_AND_MERGE_FLAGS;
	    IS_EXPLICIT_CAST_REQUIRED;
	};
	if(checkVarEntityPresentInScopeElseReg(var->name, flag, type, see) == false){
	    lexer.emitErr(tokOffs[var->tokenOff].off, "Variable redefinition");
	    return false;
	};
    }break;
    case ASTType::RETURN:{
	//TODO: 
    }break;
    default:
	UNREACHABLE;
	return false;
    };
    return true;
};
bool checkEntities(DynamicArray<ASTBase*> &entities, Lexer &lexer, DynamicArray<ScopeEntities*> &see){
    BRING_TOKENS_TO_SCOPE;
    ScopeEntities *se = see[see.count-1];
    goThroughEntitiesAndInitScope(entities, se);
    for (u32 x=0; x<entities.count; x+=1) {
	ASTBase *node = entities[x];
	if(checkEntity(node, lexer, see) == false){return false;};
    };
    return true;
};
