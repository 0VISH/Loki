bool compile(char *fileName){
	Lexer lexer = createLexer(fileName);
	if (lexer.fileContent == nullptr) {
		printf("invalid file path: %s", lexer.fileName);
		return false;
	};
	u32 off = 0;
	b32 x = genTokens(lexer);
	eatNewlines(lexer.tokenTypes, off);
	if (lexer.tokenTypes[off] == Token_Type::END_OF_FILE) {
		destroyLexer(lexer);
		return true;
	};
	dbg::dumpLexerStat(lexer);
	dbg::dumpLexerTokens(lexer);
	ASTFile astFile = createASTFile();
	while (lexer.tokenTypes[off] != Token_Type::END_OF_FILE) {
		ASTBase *base = parseBlock(lexer, astFile, off);
		if (base == nullptr) { break; };
		astFile.nodes.push(base);
		eatNewlines(lexer.tokenTypes, off);
	};
	if (errorOff == 0) {
		dbg::dumpASTFile(astFile, lexer);
		ScopeEntities fileScopeEntities = createScopeEntities();
		if (checkEntities(astFile.nodes, lexer, fileScopeEntities) == false) {
			printf("\nchecking entites failed\n");
		} else {
			for (u16 x = 0; x < fileScopeEntities.varEntities.count; x += 1) {
				VariableEntity &entity = fileScopeEntities.varEntities[x];
				if (!IS_BIT(entity.flag, VarFlags::CONSTANT)) {
					u32 nodeOff = 0;
					for (u32 varCount = 0; nodeOff < astFile.nodes.count; nodeOff += 1) {
						if (varCount == x+1) { break; };
						ASTBase *base = astFile.nodes[nodeOff];
						if (base->type >= ASTType::UNI_ASSIGNMENT_T_UNKNOWN && base->type <= ASTType::MULTI_ASSIGNMENT_T_KNOWN) {
							varCount += 1;
						};
					};
					emitErr(lexer.fileName,
					        getLineAndOff(lexer.fileContent, lexer.tokenOffsets[astFile.nodes[nodeOff-1]->tokenOff].off),
					        "Variable at global scope is not comptime"
					        );
				};
			};
		};
	};
	flushReports();
	destroyASTFile(astFile);
	destroyLexer(lexer);
	return true;
};