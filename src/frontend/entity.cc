struct VariableEntity{
	String name;
	Type type;
	u8 flag;
};
struct ProcEntity {
	DynamicArray<ProcArgs> *args;
	String name;
};

//TODO: store array of maps
struct ScopeEntities{
	Map varMap;
	DynamicArray<VariableEntity> varEntities;
	Map procMap;
	DynamicArray<ProcEntity>     procEntities;
};

ScopeEntities createScopeEntities() {
	ScopeEntities se;
	se.varMap.init(10);
	se.varEntities.init();
	se.procMap.init(10);
	se.procEntities.init(10);
	return se;
};
void destroyFileEntities(ScopeEntities &se) {
	se.varMap.uninit();
	se.varEntities.uninit();
	se.procMap.uninit();
	se.procEntities.uninit();
};
String buildString(Lexer &lexer, u32 x) {
	String str;
	str.mem = lexer.fileContent + lexer.tokenOffsets[x].off;
	str.len = lexer.tokenOffsets[x].len;
	return str;
};
bool checkVariablePresentInScope(Lexer &lexer, Type type, u32 off, ScopeEntities &se) {
	BRING_TOKENS_TO_SCOPE;
	String name = buildString(lexer, off);
	if (se.varMap.getValue(name) != -1) {
		emitErr(lexer.fileName,
		        getLineAndOff(lexer.fileContent, tokOffs[off].off),
		        "Variable redecleration"
		        );
		return false;
	};
	se.varMap.insertValue(name, se.varEntities.count);
	VariableEntity entity;
	entity.name = name;
	entity.type = type;
	entity.flag = NULL;
	se.varEntities.push(entity);
	return true;
};
bool checkVariableEntity(ASTlr *lr, u32 x, Lexer &lexer, ScopeEntities &se, b8 t_known, b8 multi) {
	BRING_TOKENS_TO_SCOPE;
	Type treeType = getTreeType(lr->rhs);
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
				emitErr(lexer.fileName,
				        getLineAndOff(lexer.fileContent, tokOffs[lr->tokenOff].off),
				        "Tree type and given type do not match"
				        );
				return false;
			};
		};
	};
	if (multi) {
		DynamicArray<ASTBase*> *table = getTable(lr, sizeof(ASTlr));
		for (u8 x = 0; x < table->count; x += 1) {
			u32 off = table->getElement(x)->tokenOff;
			if (checkVariablePresentInScope(lexer, givenType, off, se) == false) {
				return false;
			};
		};
	} else {
		if (checkVariablePresentInScope(lexer, givenType, lr->lhs->tokenOff, se) == false) {
			return false;
		};
	};
	return true;
};
bool checkEntities(DynamicArray<ASTBase*> &entities, Lexer &lexer, ScopeEntities &se){
	TIME_BLOCK;
	BRING_TOKENS_TO_SCOPE;
	for (u32 x=0; x<entities.count; x+=1) {
		ASTBase *node = entities[x];
		ASTlr   *lr = (ASTlr*)node;
		u8 t_known = false;
		u8 multi = false;
		switch (node->type) {
			case ASTType::PROC_DEFENITION: {
			} break;
			case ASTType::UNI_DECLERATION_T_KNOWN:
			case ASTType::MULTI_DECLERATION_T_KNOWN: {
			} break;
			case ASTType::MULTI_ASSIGNMENT_T_KNOWN:
				t_known = true;
			case ASTType::MULTI_ASSIGNMENT_T_UNKNOWN:
				multi = true;
				if (checkVariableEntity(lr, x, lexer, se, t_known, multi) == false) { return false; };
				break;
			case ASTType::UNI_ASSIGNMENT_T_KNOWN:
				t_known = true;
			case ASTType::UNI_ASSIGNMENT_T_UNKNOWN:
				if (checkVariableEntity(lr, x, lexer, se, t_known, multi) == false) { return false; };
				break;
			default: DEBUG_UNREACHABLE;
		};
	};
	return true;
};