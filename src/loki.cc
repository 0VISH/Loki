ScopeEntities *parseFile(char *fileName, ASTFile &astFile, Lexer &lexer){
    lexer.init(fileName);
    if(lexer.genTokens() == false) {
	return nullptr;
    };
    bool wasEntryPointKnown = (config.entryPointFileID != -1);
    u32 off = 0;
    if(lexer.tokenTypes[off] == Token_Type::END_OF_FILE) {
	return nullptr;
    };
    while (lexer.tokenTypes[off] != Token_Type::END_OF_FILE) {
	bool result = parseBlock(lexer, astFile, astFile.nodes, off);
	if(result == false){return nullptr;};
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
    astFile.entities = fileScopeEntities;
    
    if(config.entryPointProcID != -1 && (wasEntryPointKnown == false)){
	config.entryPointFileID = astFile.idGiver.fileID;
    };
    return fileScopeEntities;
};
bool compile(char *fileName){
    os::startTimer(TimeSlot::FRONTEND);
    Dep::pushToStack(fileName);
    DynamicArray<Lexer> lexers;
    lexers.init();
    for(u32 x=0; x<Dep::fileStack.count; x+=1){
	char *filePath = Dep::fileStack[x];
	ASTFile &file = Dep::newASTFile();
	Lexer &lexer = lexers.newElem();
	file.entities = parseFile(filePath, file, lexer);
	if(file.entities == nullptr){
	    report::flushReports();
	    return false;
	};
	Dep::fileToId.insertValue({filePath, (u32)strlen(filePath)}, file.idGiver.fileID);
    };
    DynamicArray<ScopeEntities*> see;
    see.init();
    DEFER(see.uninit());
    
    for(u32 x=Dep::fileStack.count; x>0;){
	x -= 1;
	ASTFile &file = Dep::astFiles[x];
	//NOTE: import pass
	for(u32 j=0; j<file.nodes.count; j+=1){
	    if(file.nodes[j]->type == ASTType::IMPORT){
		ASTImport *imp = (ASTImport*)file.nodes[j];
		u32 off;
		Dep::fileToId.getValue({imp->fileName, (u32)strlen(imp->fileName)}, &off);
		ASTFile &importFile = Dep::astFiles[off];
		see.push(importFile.entities);
	    };
	};
	see.push(file.entities);
	if(checkEntities(file.nodes, lexers[x], see, file.idGiver) == false){
	    report::flushReports();
	    return false;
	};
	see.pop();
    };
    for(u32 x=0; x<Dep::fileStack.count; x+=1){
	lexers[x].uninit();
    };
    lexers.uninit();
    os::endTimer(TimeSlot::FRONTEND);
    ASSERT(Dep::fileStack.count != 0);
    os::startTimer(TimeSlot::MIDEND);
    DynamicArray<BytecodeContext> bca;
    bca.init(3);
    DEFER(bca.uninit());
    Array<BytecodeFile> bfs(Dep::fileStack.count);
    DEFER(bfs.uninit());
    for(u32 x=Dep::fileStack.count; x>0;){
	x -= 1;
	ASTFile &file = Dep::astFiles[x];
	ScopeEntities *fileEntities = file.entities;
	BytecodeFile &bf = bfs[x];
	bf.init(file.idGiver.fileID);
	BytecodeContext &bc = bca.newElem();
	bc.init(Scope::GLOBAL);
	see.push(fileEntities);
	compileASTFileToBytecode(file, see, bca, bf);
	see.pop();
	bca.pop().uninit();
	dbg::dumpBytecodeFile(bf);
	file.uninit();
    };
    os::endTimer(TimeSlot::MIDEND);
    os::startTimer(TimeSlot::BACKEND);
    initLLVMBackend();
    for(u32 x=Dep::fileStack.count; x>0;){
	x -= 1;
	BytecodeFile &bf = bfs[x];
	backendCompileStage1(bf.firstBucket, bf.id, &config);
	bf.uninit();
    };
    backendCompileStage2(&config);
    uninitLLVMBackend();
    os::endTimer(TimeSlot::BACKEND);
    return true;
};
