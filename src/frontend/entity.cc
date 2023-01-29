bool checkEntities(DynamicArray<ASTBase*> &entities, Lexer &lexer){
	BRING_TOKENS_TO_SCOPE;
	u8 flag = false;
	for (u32 x=0; x<entities.count; x+=1) {
		ASTBase *node = entities[x];
		ASTlr   *lr = (ASTlr*)node;
		switch (node->type) {
			case ASTType::PROC_DEFENITION:
			case ASTType::UNI_DECLERATION_T_KNOWN:
			case ASTType::MULTI_DECLERATION_T_KNOWN:
				continue;
			case ASTType::UNI_ASSIGNMENT_T_UNKNOWN:
			case ASTType::MULTI_ASSIGNMENT_T_UNKNOWN:
				flag = true;
			case ASTType::UNI_ASSIGNMENT_T_KNOWN:
			case ASTType::MULTI_ASSIGNMENT_T_KNOWN: {
				Type treeType = getTreeType(lr->rhs);
				if (treeType != Type::COMP_VOID && treeType != Type::COMP_DECIMAL && treeType != Type::COMP_INTEGER) {
					emitErr(lexer.fileName,
					        getLineAndOff(lexer.fileContent, tokOffs[node->tokenOff].off),
					        "Variable value not known at comptime"
					        );
					return false;
				};
				if (flag) { return true; };
				Type givenType;
				switch (tokTypes[x-1]) {
					case Token_Type::K_U8:  givenType = Type::U_8; break;
					case Token_Type::K_U16: givenType = Type::U_16; break;
					case Token_Type::K_U32: givenType = Type::U_32; break;
					case Token_Type::K_S8:  givenType = Type::S_8; break;
					case Token_Type::K_S16: givenType = Type::S_16; break;
					case Token_Type::K_S32: givenType = Type::S_32; break;
				};
				//TODO: register type to hashmap
			} break;
			default: {
				emitErr(lexer.fileName,
				        getLineAndOff(lexer.fileContent, tokOffs[node->tokenOff].off),
				        "At file scope you can declare/define\n1. procedures\n2. variables(if defined, value should be known at comptime)"
				        );
				return false;
			} break;
		};
	};
	return true;
};