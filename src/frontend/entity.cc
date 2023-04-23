struct Entity{
    String name;
    u8 flag;
};
struct VariableEntity : Entity{
    Type type;
};
struct ProcEntity : Entity{};

struct ScopeEntities{
    Map varMap;
    Map procMap;
    VariableEntity* varEntities;
    ProcEntity* procEntities;
    u16 varCount;
    u16 procCount;
};

void destroyFileEntities(ScopeEntities &se) {
    mem::free(se.varEntities);
    mem::free(se.procEntities);
    se.varMap.uninit();
    se.procMap.uninit();
};
bool checkVarEntityPresentInScopeElseReg(Lexer &lexer, String name, Flag flag, Type type, ScopeEntities &se) {
    Map &map = se.varMap;
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
    se.varEntities[id] = entity;
    return true;
};
bool checkProcEntityPresentInScopeElseReg(Lexer &lexer, String name, Flag flag, ScopeEntities &se) {
    Map &map = se.procMap;
    BRING_TOKENS_TO_SCOPE;
    if (map.getValue(name) != -1) {
	return false;
    };
    u32 id = map.count;
    map.insertValue(name,id);
    ProcEntity entity;
    entity.name = name;
    entity.flag = flag;
    se.procEntities[id] = entity;
    return true;
};
void goThroughEntitiesAndInitMaps(DynamicArray<ASTBase*> &entities, ScopeEntities &se) {
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
    se.varMap.init(varCount);
    se.procMap.init(procCount);
    se.varEntities = (VariableEntity*)mem::alloc(sizeof(VariableEntity) * varCount);
    se.procEntities = (ProcEntity*)mem::alloc(sizeof(ProcEntity) * procCount);
    se.varCount = 0;
    se.procCount = 0;
};
bool checkVarDecl(ASTBase *base, Lexer &lexer, ScopeEntities &se, bool isSingle){
    if(isSingle){
	ASTUniVar *var = (ASTUniVar*)base;
	Type type = getType(lexer, var->tokenOff+2);
	if(checkVarEntityPresentInScopeElseReg(lexer, var->name, var->flag, type, se) == false){
	    BRING_TOKENS_TO_SCOPE;
	    lexer.emitErr(tokOffs[var->tokenOff].off, "Variable redecleration");
	    return false;
	};
	return true;
    };
    ASTMultiVar *var = (ASTMultiVar*)base;
    Type type = getType(lexer, var->tokenOff+2);
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
bool checkVarDef(ASTBase *base, Lexer &lexer, ScopeEntities &se, bool tKown, bool isSingle){
    BRING_TOKENS_TO_SCOPE;
    if(isSingle){
	ASTUniVar *var = (ASTUniVar*)base;
	Type type;
	Flag &flag = var->flag;
	Flag treeFlag;
	Type treeType = getTreeType(var->rhs, treeFlag);
	if(tKown){
	    type = getType(lexer, var->tokenOff+2);
	    if((u32)treeType < (u32)type){
		lexer.emitErr(tokOffs[var->tokenOff].off, "Size of expression is greater than declared size");
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
    Type treeType = getTreeType(var->rhs, treeFlag);
    if(tKown){
	type = getType(lexer, var->tokenOff+2);
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
bool checkEntities(DynamicArray<ASTBase*> &entities, Lexer &lexer, ScopeEntities &se){
    TIME_BLOCK;
    BRING_TOKENS_TO_SCOPE;
    goThroughEntitiesAndInitMaps(entities, se);
    for (u32 x=0; x<entities.count; x+=1) {
	ASTBase *node = entities[x];
	u8 t_known = false;
	u8 multi = false;
	switch (node->type) {
	case ASTType::PROC_DEFENITION: {
	    ASTProcDef *proc = (ASTProcDef*)node;
	    String name = proc->name;
	    if(checkProcEntityPresentInScopeElseReg(lexer, name, NULL, se) == false){
		lexer.emitErr(tokOffs[proc->tokenOff].off, "Procedure redecleration");
		return false;
	    };
	    //TODO:check in and out
	    ScopeEntities pse;
	    if (checkEntities(proc->body, lexer, pse) == false) { return false; };
	} break;
	case ASTType::UNI_DECLERATION:{
	    if(checkVarDecl(node, lexer, se, true) == false){ return false;};
	}break;
	case ASTType::MULTI_DECLERATION:{
	    if(checkVarDecl(node, lexer, se, false) == false){ return false;};
	}break;
	case ASTType::MULTI_ASSIGNMENT_T_KNOWN:{
	    if(checkVarDef(node, lexer, se, true, false) == false){return false;};
	}break;
	case ASTType::MULTI_ASSIGNMENT_T_UNKNOWN: {
	    if(checkVarDef(node, lexer, se, false, false) == false){return false;};
	} break;
	case ASTType::UNI_ASSIGNMENT_T_KNOWN:{
	    if(checkVarDef(node, lexer, se, true, true) == false){return false;};
	}break;
	case ASTType::UNI_ASSIGNMENT_T_UNKNOWN: {
	    if(checkVarDef(node, lexer, se, false, true) == false){return false;};
	} break;
	case ASTType::BIN_SUB:
	case ASTType::BIN_ADD:{
	    ASTBinOp *op = (ASTBinOp*)node;
	    Flag flag;
	    Type lhsType = getTreeType(op->lhs, flag);
	    Type rhsType = getTreeType(op->rhs, flag);
	    op->lhsType = lhsType;
	    op->rhsType = rhsType;
	    //TODO: check if types are compatible?
	}break;
	default: DEBUG_UNREACHABLE;
	};
    };
    return true;
};
