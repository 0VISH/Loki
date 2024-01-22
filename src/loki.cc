ScopeEntities *parseCheckAndLoadEntities(char *fileName, ASTFile &astFile){
    Lexer lexer;
    lexer.init(fileName);
    DEFER(lexer.uninit());
    if(lexer.genTokens() == false) {
	return nullptr;
    };
    bool wasEntryPointKnown = (config.entryPointFileID != -1);
    u32 off = 0;
    eatNewlines(lexer.tokenTypes, off);
    if(lexer.tokenTypes[off] == Token_Type::END_OF_FILE) {
	return nullptr;
    };
    while (lexer.tokenTypes[off] != Token_Type::END_OF_FILE) {
	bool result = parseBlock(lexer, astFile, astFile.nodes, off);
	if(result == false){return nullptr;};
	eatNewlines(lexer.tokenTypes, off);
    };
    for(u32 x=0; x<astFile.nodes.count; x+=1){
	ASTBase *base = astFile.nodes[x];
	SET_BIT(base->flag, Flags::GLOBAL);
    };
    dbg::dumpASTFile(astFile, lexer);
    DynamicArray<ScopeEntities*> see;
    see.init(3);
    DEFER(see.uninit());
    ScopeEntities *fileScopeEntities = allocScopeEntity(Scope::GLOBAL);
    astFile.scope = fileScopeEntities;
    see.push(fileScopeEntities);
    if(checkEntities(astFile.nodes, lexer, see, astFile.idGiver) == false){
	return nullptr;
    };
    if(config.entryPointProcID != -1 && (wasEntryPointKnown == false)){
	config.entryPointFileID = astFile.id;
    };
    return fileScopeEntities;
};
bool compile(char *fileName){
    os::startTimer(TimeSlot::FRONTEND);
    Dep::pushToParseCheckStack(fileName);
    while(Dep::parseCheckStack.count != 0){
	char *filePath = Dep::parseCheckStack.pop();
	ASTFile &file = Dep::newASTFile();
	file.scope = parseCheckAndLoadEntities(filePath, file);
	if(file.scope == nullptr){break;};
	Dep::pushToCompileStack(file.id);
    };
    os::endTimer(TimeSlot::FRONTEND);
    report::flushReports();
    if(report::errorOff != 0){
	return false;
    };
    ASSERT(Dep::compileStack.count != 0);
    os::startTimer(TimeSlot::MIDEND);
    DynamicArray<BytecodeContext> bca;
    bca.init(3);
    DEFER(bca.uninit());
    DynamicArray<ScopeEntities*> see;
    see.init();
    DEFER(see.uninit());
    Array<BytecodeFile> bfs(Dep::compileStack.count);
    DEFER(bfs.uninit());
    for(u32 x=Dep::compileStack.count; x>0;){
	x -= 1;
	s16 fileID = Dep::compileStack[x];
	ASTFile &file = Dep::astFiles[fileID];
	ScopeEntities *fileScope = file.scope;
	BytecodeFile &bf = bfs[x];
	bf.init(file.id);
	BytecodeContext &bc = bca.newElem();
	bc.init(Scope::GLOBAL);
	see.push(fileScope);
	compileASTFileToBytecode(file, see, bca, bf);
	see.pop();
	bca.pop().uninit();
	dbg::dumpBytecodeFile(bf);
	file.uninit();
    };
    os::endTimer(TimeSlot::MIDEND);
    os::startTimer(TimeSlot::BACKEND);
    BackendRef backRef = loadRef(BackendType::LLVM);
    if(!backRef.code){
	return false;
    };
    backRef.initBackend();
    for(u32 x=Dep::compileStack.count; x>0;){
	x -= 1;
	BytecodeFile &bf = bfs[x];
	backRef.backendCompileStage1(bf.firstBucket, bf.id, &config);
	bf.uninit();
    };
    backRef.backendCompileStage2(&config);
    backRef.uninitBackend();
    unloadRef(backRef);
    os::endTimer(TimeSlot::BACKEND);
    return true;
};
