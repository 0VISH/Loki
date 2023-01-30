bool compile(char *fileName){
	Lexer lexer = createLexer(fileName);
	if (lexer.fileContent == nullptr) {
		printf("invalid file path: %s", lexer.fileName);
		destroyLexer(lexer);
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
	b8 error = false;
	while (lexer.tokenTypes[off] != Token_Type::END_OF_FILE) {
		ASTBase *base = parseBlock(lexer, astFile, off);
		if (base == nullptr) { error = true; break; };
		astFile.nodes.push(base);
		eatNewlines(lexer.tokenTypes, off);
	};
	if (error == false) {
		dbg::dumpASTFile(astFile, lexer);
		ScopeEntities fileScopeEntities = createScopeEntities();
		if (checkEntities(astFile.nodes, lexer, fileScopeEntities) == false) {
			printf("\nchecking entites failed\n");
		};
	};
	flushReports();
	destroyASTFile(astFile);
	destroyLexer(lexer);
	return true;
};