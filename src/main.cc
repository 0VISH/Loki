#include "include.hh"

s32 main(s32 argc, char **argv) {
	if (argc < 2) {
		printf("main file path not provided");
		return EXIT_SUCCESS;
	};
	EXCEPTION_BLOCK_START
	dbg::initTimer();
	initKeywords();
	Lexer lexer = createLexer(argv[1]);
	if (lexer.fileContent == nullptr) {
		printf("invalid file path: %s", lexer.fileName);
		goto UNINIT_L1;
	};
	u32 off = 0;
	b32 x = genTokens(lexer);
	eatNewlines(lexer.tokenTypes, off);
	if (lexer.tokenTypes[off] == Token_Type::END_OF_FILE) { goto UNINIT_L2; };
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
	ScopeEntities fileScopeEntities = createScopeEntities();
	if (error == false) {
		dbg::dumpASTFile(astFile, lexer);
		if (checkEntities(astFile.nodes, lexer, fileScopeEntities) == false) { printf("\nchecking entites failed\n"); };
	};
	flushReports();
	destroyASTFile(astFile);
	UNINIT_L2:
	destroyLexer(lexer);
	UNINIT_L1:
	uninitKeywords();
	dbg::dumpBlockTimes();
	EXCEPTION_BLOCK_END
#if(XE_DBG)
	printf("\ndone!");
#endif
	return EXIT_SUCCESS;
};


//@ignore
static_assert(sizeof(u8) == 1, "u8 is not 1 byte");
static_assert(sizeof(u16) == 2, "u16 is not 2 byte");
static_assert(sizeof(u32) == 4, "u32 is not 4 byte");
static_assert(sizeof(u64) == 8, "u64 is not 8 byte");
static_assert(sizeof(s8) == 1, "s8 is not 1 byte");
static_assert(sizeof(s16) == 2, "s16 is not 2 byte");
static_assert(sizeof(s32) == 4, "s32 is not 4 byte");
static_assert(sizeof(s64) == 8, "s64 is not 8 byte");
static_assert(sizeof(b8) == 1, "b8 is not 1 byte");
static_assert(sizeof(b16) == 2, "b16 is not 2 byte");
static_assert(sizeof(b32) == 4, "b32 is not 4 byte");
static_assert(sizeof(b64) == 8, "b64 is not 8 byte");
static_assert(sizeof(f32) == 4, "f32 is not 4 byte");
static_assert(sizeof(f64) == 8, "f64 is not 8 byte");
