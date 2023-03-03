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
bool checkVariablePresentInScopeElseReg(Lexer &lexer, Type type, String name, u32 off, ScopeEntities &se, Flag flag) {
    BRING_TOKENS_TO_SCOPE;
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
bool checkVariableEntity(ASTBase *base, u32 x, Lexer &lexer, ScopeEntities &se, b8 t_known, b8 multi) {
    BRING_TOKENS_TO_SCOPE;
    Flag flag;
    Type treeType;
    if(multi){
	ASTMultiVar *multiAss = (ASTMultiVar*)base;
	treeType = getTreeType(multiAss->rhs, flag);
    }else{
	ASTUniVar *uniAss = (ASTUniVar*)base;
	treeType = getTreeType(uniAss->rhs, flag);
    };
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
		u32 off;
		if(multi){
		    ASTMultiVar *multiAss = (ASTMultiVar*)base;
		    off = multiAss->tokenOff;
		}else{
		    ASTUniVar *uniAss = (ASTUniVar*)base;
		    off = uniAss->tokenOff;
		};
		lexer.emitErr(tokOffs[off].off, "Tree type and given type do not match");
		return false;
	    };
	};
    };
    if (multi) {
	ASTMultiVar *multiAss = (ASTMultiVar*)base;
	DynamicArray<String> &names = multiAss->names;;
	for (u8 x = 0; x < names.count; x += 1) {
	    if (checkVariablePresentInScopeElseReg(lexer, givenType, names[x], x, se, flag) == false) {
		return false;
	    };
	};
    } else {
	ASTUniVar *uniAss = (ASTUniVar*)base;
	if (checkVariablePresentInScopeElseReg(lexer, givenType, uniAss->name, uniAss->tokenOff, se, flag) == false) {
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
	    if (se.procMap.getValue(name) != -1) {
		lexer.emitErr(tokOffs[proc->tokenOff].off, "Procedure redecleration");
		return false;
	    };
	    //TODO:checking in
	    if(proc->in.args.count != 0){
		Map inProc;
		inProc.init(proc->in.args.count);
		for(u16 x=0; x<proc->in.args.count; x+=1){
		    
		};
	    };
	    //TODO:checking out
	    if(proc->out.typeOffs.count != 0){
	    };
	    se.procMap.insertValue(name, se.procCount);
	    ProcEntity entity;
	    entity.name = name;
	    se.procEntities[se.procCount] = entity;
	    se.procCount += 1;
	    ScopeEntities pse;
	    if (checkEntities(proc->body, lexer, pse) == false) { return false; };
	} break;
	case ASTType::UNI_DECLERATION_T_KNOWN:
	case ASTType::MULTI_DECLERATION_T_KNOWN: {
	} break;
	case ASTType::MULTI_ASSIGNMENT_T_KNOWN:
	    t_known = true;
	case ASTType::MULTI_ASSIGNMENT_T_UNKNOWN: {
	    multi = true;
	    if (checkVariableEntity(node, x, lexer, se, t_known, multi) == false) { return false; };
	} break;
	case ASTType::UNI_ASSIGNMENT_T_KNOWN:
	    t_known = true;
	case ASTType::UNI_ASSIGNMENT_T_UNKNOWN: {
	    if (checkVariableEntity(node, x, lexer, se, t_known, multi) == false) { return false; };
	} break;
	default: DEBUG_UNREACHABLE;
	};
    };
    return true;
};
