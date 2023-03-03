struct VariableEntity{
    String name;
    Type type;
    u8 flag;
};
struct ProcEntity {
    String name;
};

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
String buildString(Lexer &lexer, u32 x) {
    String str;
    str.mem = lexer.fileContent + lexer.tokenOffsets[x].off;
    str.len = lexer.tokenOffsets[x].len;
    return str;
};
bool checkVariablePresentInScopeElseReg(Lexer &lexer, Type type, u32 off, ScopeEntities &se, Flag flag) {
    BRING_TOKENS_TO_SCOPE;
    String name = buildString(lexer, off);
    if (se.varMap.getValue(name) != -1) {
	lexer.emitErr(tokOffs[off].off, "Variable redecleration");
	return false;
    };
    se.varMap.insertValue(name, se.varCount);
    VariableEntity entity;
    entity.name = name;
    entity.type = type;
    entity.flag = flag;
    se.varEntities[se.varCount] = entity;
    se.varCount += 1;
    return true;
};
bool checkVariableEntity(ASTlr *lr, u32 x, Lexer &lexer, ScopeEntities &se, b8 t_known, b8 multi) {
    BRING_TOKENS_TO_SCOPE;
    Flag flag;
    Type treeType = getTreeType(lr->rhs, flag);
    Type givenType;
    if (t_known) {
	switch (tokTypes[x-1]) {
	case Token_Type::K_U8:  givenType = Type::U_8; break;
	case Token_Type::K_U16: givenType = Type::U_16; break;
	case Token_Type::K_U32: givenType = Type::U_32; break;
	case Token_Type::K_S8:  givenType = Type::S_8; break;
	case Token_Type::K_S16: givenType = Type::S_16; break;
	case Token_Type::K_S32: givenType = Type::S_32; break;
	};
	if (treeType != Type::COMP_VOID && treeType != Type::COMP_DECIMAL && treeType != Type::COMP_INTEGER) {
	    if (givenType != treeType) {
		lexer.emitErr(tokOffs[lr->tokenOff].off, "Tree type and given type do not match");
		return false;
	    };
	};
    };
    if (multi) {
	DynamicArray<ASTBase*> *table = getTableLR(lr);
	for (u8 x = 0; x < table->count; x += 1) {
	    u32 off = table->getElement(x)->tokenOff;
	    if (checkVariablePresentInScopeElseReg(lexer, givenType, off, se, flag) == false) {
		return false;
	    };
	};
    } else {
	if (checkVariablePresentInScopeElseReg(lexer, givenType, lr->lhs->tokenOff, se, flag) == false) {
	    return false;
	};
    };
    return true;
};
void goThroughEntitiesAndInitMaps(DynamicArray<ASTBase*> &entities, ScopeEntities &se) {
    u32 procCount = 0;
    u32 varCount = 0;
    for (u32 x = 0; x < entities.count; x += 1) {
	ASTBase *node = entities[x];
	if (node->type == ASTType::PROC_DEFENITION) {
	    procCount += 1;
	} else if (node->type >= ASTType::UNI_DECLERATION_T_KNOWN && node->type <= ASTType::UNI_ASSIGNMENT_T_KNOWN) {
	    varCount += 1;
	} else if (node->type >= ASTType::MULTI_DECLERATION_T_KNOWN && node->type <= ASTType::MULTI_ASSIGNMENT_T_KNOWN){
	    DynamicArray<ASTBase*> *table = getTableLR((ASTlr*)node);
	    varCount += table->count;
	};
    };
    se.varMap.init(varCount);
    se.procMap.init(procCount);
    se.varEntities = (VariableEntity*)mem::alloc(sizeof(VariableEntity) * varCount);
    se.procEntities = (ProcEntity*)mem::alloc(sizeof(ProcEntity) * procCount);
    se.varCount = 0;
    se.procCount = 0;
};
bool checkEntities(DynamicArray<ASTBase*> &entities, Lexer &lexer, ScopeEntities &se){
    TIME_BLOCK;
    BRING_TOKENS_TO_SCOPE;
    goThroughEntitiesAndInitMaps(entities, se);
    for (u32 x=0; x<entities.count; x+=1) {
	ASTBase *node = entities[x];
	ASTlr   *lr = (ASTlr*)node;
	u8 t_known = false;
	u8 multi = false;
	switch (node->type) {
	case ASTType::PROC_DEFENITION: {
	    String name = buildString(lexer, node->tokenOff);
	    if (se.procMap.getValue(name) != -1) {
		lexer.emitErr(tokOffs[node->tokenOff].off, "Procedure redecleration");
		return false;
	    };
	    se.procMap.insertValue(name, se.procCount);
	    ProcEntity entity;
	    entity.name = name;
	    se.procEntities[se.procCount] = entity;
	    se.procCount += 1;
	    ScopeEntities pse;
	    DynamicArray<ASTBase*> *table = getTableLR(lr);
	    if (checkEntities(*table, lexer, pse) == false) { return false; };
	} break;
	case ASTType::UNI_DECLERATION_T_KNOWN:
	case ASTType::MULTI_DECLERATION_T_KNOWN: {
	} break;
	case ASTType::MULTI_ASSIGNMENT_T_KNOWN:
	    t_known = true;
	case ASTType::MULTI_ASSIGNMENT_T_UNKNOWN: {
	    multi = true;
	    if (checkVariableEntity(lr, x, lexer, se, t_known, multi) == false) { return false; };
	} break;
	case ASTType::UNI_ASSIGNMENT_T_KNOWN:
	    t_known = true;
	case ASTType::UNI_ASSIGNMENT_T_UNKNOWN: {
	    if (checkVariableEntity(lr, x, lexer, se, t_known, multi) == false) { return false; };
	} break;
	default: DEBUG_UNREACHABLE;
	};
    };
    return true;
};
