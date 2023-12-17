ScopeEntities *parseCheckAndLoadEntities(char *fileName, ASTFile &astFile){
    Lexer lexer;
    lexer.init(fileName);
    DEFER(lexer.uninit());
    if(lexer.genTokens() == false) {
	return nullptr;
    };
    u32 off = 0;
    eatNewlines(lexer.tokenTypes, off);
    if(lexer.tokenTypes[off] == Token_Type::END_OF_FILE) {
	return nullptr;
    };
    while (lexer.tokenTypes[off] != Token_Type::END_OF_FILE) {
	ASTBase *base = parseBlock(lexer, astFile, off);
	if(base == nullptr){break;};
	astFile.nodes.push(base);
	eatNewlines(lexer.tokenTypes, off);
    };
    if(report::errorOff != 0){
	report::flushReports();
	return nullptr;
    };
    dbg::dumpASTFile(astFile, lexer);
    DynamicArray<ScopeEntities*> see;
    see.init(3);
    DEFER(see.uninit());
    ScopeEntities *fileScopeEntities = allocScopeEntity(Scope::GLOBAL);
    see.push(fileScopeEntities);
    if (checkEntities(astFile.nodes, lexer, see) == false) {
	return nullptr;
    } else {
	for (u16 x = 0; x < fileScopeEntities->varMap.count; x += 1) {
	    VariableEntity &entity = fileScopeEntities->varEntities[x];
	    if (!IS_BIT(entity.flag, Flags::CONSTANT)) {
		u32 nodeOff = 0;
		for (u32 varCount = 0; nodeOff < astFile.nodes.count; nodeOff += 1) {
		    if (varCount == x+1) { break; };
		    ASTBase *base = astFile.nodes[nodeOff];
		    if (base->type >= ASTType::UNI_INITIALIZATION_T_UNKNOWN && base->type <= ASTType::MULTI_INITIALIZATION_T_KNOWN) {
			varCount += 1;
		    };
		};
		ASTBase *node = astFile.nodes[nodeOff-1];
		u32 off;
		if(node->type >= ASTType::MULTI_DECLERATION && node->type <= ASTType::MULTI_INITIALIZATION_T_KNOWN){
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
    Dep::registerScopedEntities(astFile.id, fileScopeEntities);
    return fileScopeEntities;
};
bool compile(char *fileName){
    os::startTimer(TimeSlot::FRONTEND);
    Dep::pushToParseAndCheckStack(fileName);
    while(Dep::parseAndCheckStack.count != 0){
	Dep::IDAndName idf = Dep::parseAndCheckStack.pop();
	ASTFile &file = Dep::createASTFile(idf.id);
	file.scope  = parseCheckAndLoadEntities(idf.name, file);
	if(file.scope){
	    Dep::pushToCompileStack(&file);
	};
    };
    os::endTimer(TimeSlot::FRONTEND);
    if(report::errorOff != 0){return false;};
    os::startTimer(TimeSlot::MIDEND);
    DynamicArray<BytecodeContext> bca;
    bca.init(3);
    DEFER(bca.uninit());
    DynamicArray<ScopeEntities*> see;
    Array<BytecodeFile> bfs(Dep::compileStack.count);
    DEFER(bfs.uninit());
    see.init();
    for(u32 x=Dep::compileStack.count; x>0;){
	x -= 1;
	ASTFile *file = Dep::compileStack[x];
	ScopeEntities *se = file->scope;
	printf("SE: %d %d %d\n", se->varMap.len, se->procMap.len, se->structMap.len);
	BytecodeFile &bf = bfs[x];
	bf.init(file->id);
	BytecodeContext &bc = bca.newElem();
	bc.init(se->varMap.count, se->procMap.count, 0);
	see.push(se);
	compileASTNodesToBytecode(file->nodes, see, bca, bf);
	see.pop();
	se->uninit();
	mem::free(se);
	bca.pop().uninit();
	dbg::dumpBytecodeFile(bf);
    };
    see.uninit();
    os::endTimer(TimeSlot::MIDEND);
    os::startTimer(TimeSlot::BACKEND);
    initLLVMBackend();
    for(u32 x=Dep::compileStack.count; x>0;){
	x -= 1;
	BytecodeFile &bf = bfs[x];
	BackendCompileStage1(&bf, &config);
	bf.uninit();
    };
    BackendCompileStage2(&config);
    uninitLLVMBackend();
    os::endTimer(TimeSlot::BACKEND);
    return true;
};
