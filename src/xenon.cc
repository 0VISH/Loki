bool compile(char *fileName){
    Lexer lexer;
    if(lexer.init(fileName) == false){
	printf("invalid file path: %s", lexer.fileName);
	return false;
    };
    DEFER(lexer.uninit());
    if(lexer.genTokens() == false) {
	report::flushReports();
	return false;
    };
    //check if file is empty
    u32 off = 0;
    eatNewlines(lexer.tokenTypes, off);
    if(lexer.tokenTypes[off] == Token_Type::END_OF_FILE) {
	return true;
    };
    //dbg::dumpLexerStat(lexer);
    //dbg::dumpLexerTokens(lexer);
    ASTFile astFile;
    astFile.init();
    DEFER(astFile.uninit());
    while (lexer.tokenTypes[off] != Token_Type::END_OF_FILE) {
	ASTBase *base = parseBlock(lexer, astFile, off);
	if (base == nullptr) { break; };
	astFile.nodes.push(base);
	eatNewlines(lexer.tokenTypes, off);
    };
    if (report::errorOff != 0) {
	report::flushReports();
	return false;
    };
    dbg::dumpASTFile(astFile, lexer);
    DynamicArray<ScopeEntities*> see;
    see.init(3);
    DEFER({
	    destroyScopes(see);
	    see.uninit();
	});
    ScopeEntities *fileScopeEntities = pushNewScope(see);
    if (checkEntities(astFile.nodes, lexer, see) == false) {
	printf("\nchecking entites failed\n");
	report::flushReports();
	return false;
    } else {
	for (u16 x = 0; x < fileScopeEntities->varMap.count; x += 1) {
	    VariableEntity &entity = fileScopeEntities->varEntities[x];
	    if (!IS_BIT(entity.flag, Flags::CONSTANT)) {
		u32 nodeOff = 0;
		for (u32 varCount = 0; nodeOff < astFile.nodes.count; nodeOff += 1) {
		    if (varCount == x+1) { break; };
		    ASTBase *base = astFile.nodes[nodeOff];
		    if (base->type >= ASTType::UNI_ASSIGNMENT_T_UNKNOWN && base->type <= ASTType::MULTI_ASSIGNMENT_T_KNOWN) {
			varCount += 1;
		    };
		};
		ASTBase *node = astFile.nodes[nodeOff-1];
		u32 off;
		if(node->type >= ASTType::MULTI_DECLERATION && node->type <= ASTType::MULTI_ASSIGNMENT_T_KNOWN){
		    ASTMultiVar *multiAss = (ASTMultiVar*)node;
		    off = multiAss->tokenOff;
		}else{
		    ASTUniVar *uniAss = (ASTUniVar*)node;
		    off = uniAss->tokenOff;
		};
		lexer.emitErr(lexer.tokenOffsets[off].off, "Variable at global scope is not comptime");
	    };
	    SET_BIT(entity.flag, Flags::GLOBAL);
	};
    };
    BytecodeFile bf;
    bf.init();
    DEFER(bf.uninit());
    DynamicArray<BytecodeContext> bca;
    bca.init(3);
    DEFER({
	    destroyBytecodeContexts(bca);
	    bca.uninit();
	});
    BytecodeContext &bc = bca.newElem();
    bc.init(fileScopeEntities->varMap.count, fileScopeEntities->procMap.count);
    compileASTNodesToBytecode(astFile.nodes, lexer, see, bca, bf);
    dbg::dumpBytecodeFile(bf);
    VM vm;
    vm.init(bf, 0);
    DEFER(vm.uninit());
    execBytecode(1000000, vm);
    report::flushReports();
    return true;
};
