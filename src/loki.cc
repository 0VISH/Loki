bool compile(char *fileName){
    os::startTimer(TimeSlot::FRONTEND);
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
	    see[0]->uninit();
	    mem::free(see[0]);
	    see.uninit();
	});
    ScopeEntities *fileScopeEntities = allocScopeEntity(Scope::GLOBAL);
    see.push(fileScopeEntities);
    if (checkEntities(astFile.nodes, lexer, see) == false) {
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
    os::endTimer(TimeSlot::FRONTEND);
    os::startTimer(TimeSlot::MIDEND);
    BytecodeFile bf;
    bf.init();
    DEFER(bf.uninit());
    DynamicArray<BytecodeContext> bca;
    bca.init(3);
    DEFER({
	    bca[0].uninit();
	    bca.uninit();
	});
    BytecodeContext &bc = bca.newElem();
    bc.init(fileScopeEntities->varMap.count, fileScopeEntities->procMap.count, 0);
    compileASTNodesToBytecode(astFile.nodes, lexer, see, bca, bf);
    dbg::dumpBytecodeFile(bf);
    os::endTimer(TimeSlot::MIDEND);
    /*
    os::startTimer(TimeSlot::EXEC_BC);
    VM vm;
    vm.init(bf.firstBucket, &bf.labels, 0);
    DEFER(vm.uninit());
    if(execBytecode(vm) == false){
	printf("TODO: VM ERRORs");
    };
    os::endTimer(TimeSlot::EXEC_BC);
    */
    return true;
};
