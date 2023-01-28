#define AST_PAGE_SIZE 1024

#define BRING_TOKENS_TO_SCOPE DynamicArray<Token_Type> &tokTypes = lexer.tokenTypes;DynamicArray<TokenOffset> &tokOffs = lexer.tokenOffsets;

enum class ASTType {
	NUM_INTEGER,
	NUM_DECIMAL,
	BIN_ADD,
	BIN_SUB,
	BIN_MUL,
	BIN_DIV,
	UNI_ASSIGNMENT,
	MULTI_ASSIGNMENT,
	PROC_DEFENITION,
	VARIABLE,
};

struct ASTBase {
	ASTType type;
	u32 tokenOff;
};
struct ASTlr : ASTBase {
	ASTBase *lhs;
	ASTBase *rhs;
};
struct ASTFile {
	DynamicArray<char*> memPages;
	ASTBase *root;
	u16 pageBrim;
};
struct ProcArgs {
	/*
		foo :: proc(x,y,z: u32) ...
		
		count = 3
		typeOff = *token offset of u32*
		
	*/
	s8 count;
	u32 typeOff;
};

ASTFile createASTFile() {
	ASTFile file;
	file.memPages.init(2);
	char *page = (char*)mem::alloc(AST_PAGE_SIZE);
	file.memPages.push(page);
	file.pageBrim = 0;
	file.root = nullptr;
	return file;
};
void destroyASTFile(ASTFile &file) {
	for (u16 i=0; i<file.memPages.count; i++) {
		mem::free(file.memPages[i]);
	};
	file.memPages.uninit();
};
ASTBase *allocAST(u32 nodeSize, ASTType type, u32 tokenOff, ASTFile &file) {
	if (file.pageBrim+nodeSize >= AST_PAGE_SIZE) {
		char *page = (char*)mem::alloc(AST_PAGE_SIZE);
		file.memPages.push(page);
		file.pageBrim = 0;
	};
	u8 memPageOff = file.memPages.count-1;
	ASTBase *node = (ASTBase*)(file.memPages[memPageOff] + file.pageBrim);
	file.pageBrim += nodeSize;
	node->type = type;
	node->tokenOff = tokenOff;
	return node;
};
//table memory is freed during bytecode compilation
ASTBase *allocASTWithTable(u32 nodeSize, ASTType type, u32 tokenOff, ASTFile &file) {
	return allocAST(nodeSize + sizeof(DynamicArray<ASTBase*>), type, tokenOff, file);
};
DynamicArray<ASTBase*> *getTable(void *ptr, u32 size) { return (DynamicArray<ASTBase*>*)(((char*)ptr) + size); };

ASTBase *genASTOperand(Lexer &lexer, u32 &x, char *fileContent, ASTFile &file, s16 &bracket) {
	BRING_TOKENS_TO_SCOPE;
	CHECK_TYPE_AST_OPERAND:
	Token_Type type = tokTypes[x];
	switch (type) {
		case Token_Type::INTEGER: {
			ASTBase *numNode = allocAST(sizeof(ASTBase), ASTType::NUM_INTEGER, x, file);
			x += 1;
			return (ASTBase*)numNode;
		} break;
		case (Token_Type)'(': {
			while (tokTypes[x] == (Token_Type)'(') {
				bracket += 10;
				x += 1;
			};
			goto CHECK_TYPE_AST_OPERAND;
		}break;
		default:
			emitErr(lexer.fileName,
			        getLineAndOff(lexer.fileContent, tokOffs[x].off),
			        "Invalid operand"
			        );
			return nullptr;
	};
	return nullptr;
};
s16 checkAndGetPrio(Lexer &lexer, u32 &x) {
	BRING_TOKENS_TO_SCOPE;
	switch (tokTypes[x]) {
		case (Token_Type)'+':
		case (Token_Type)'-':
			return 1;
		case (Token_Type)'*':
		case (Token_Type)'/':
			return 2;
		default:
			emitErr(lexer.fileName,
			        getLineAndOff(lexer.fileContent, tokOffs[x].off),
			        "Invalid operator"
			        );
			return -1;
	};
};
ASTlr *genASTOperator(Lexer &lexer, u32 x, ASTFile &file) {
	BRING_TOKENS_TO_SCOPE;
	ASTType type;
	switch (lexer.fileContent[tokOffs[x].off]) {
		case '+': type = ASTType::BIN_ADD; break;
		case '-': type = ASTType::BIN_SUB; break;
		case '*': type = ASTType::BIN_MUL; break;
		case '/': type = ASTType::BIN_DIV; break;
		default: DEBUG_UNREACHABLE;
	};
	return (ASTlr*)allocAST(sizeof(ASTlr), type, x, file);
};
ASTlr *genRHSExpr(Lexer &lexer, ASTFile &file, u32 x, u32 y, u32 &curPos, s16 &bracket) {
	BRING_TOKENS_TO_SCOPE;
	s16 prio = checkAndGetPrio(lexer, x);
	if (prio == -1) { return nullptr; };
	ASTlr *node = (ASTlr*)genASTOperator(lexer, x, file);
	ASTBase *operand = nullptr;
	ASTlr *binOperator = nullptr;
	ASTlr *previousOperator = node;
	x += 1;
	while (x < y) {
		operand = genASTOperand(lexer, x, lexer.fileContent, file, bracket);
		if (operand == nullptr) { return nullptr; };
		if (x >= y) { break; };
		if (tokTypes[x] == (Token_Type)')') {
			while (tokTypes[x] == (Token_Type)')') {
				bracket -= 10;
				x += 1;
			};
			break;
		};
		s16 curPrio = checkAndGetPrio(lexer, x);
		if (curPrio == -1) { return nullptr; };
		if (curPrio+bracket < prio) {break;};
		prio = curPrio;
		binOperator = genASTOperator(lexer , x, file);
		binOperator->lhs = operand;
		previousOperator->rhs = (ASTBase*)binOperator;
		previousOperator = binOperator;
		x += 1;
	};
	previousOperator->rhs = operand;
	curPos = x;
	return node;
};
//                                               begin off, end off+1
ASTBase *genASTExprTree(Lexer &lexer, ASTFile &file, u32 x, u32 y) {
	u32 start = x;
	BRING_TOKENS_TO_SCOPE;
	Token_Type type = tokTypes[x];
	s16 bracket = 0;
	ASTBase *lhs = genASTOperand(lexer, x, lexer.fileContent, file, bracket);
	if (lhs == nullptr) { return nullptr; };
	ASTlr *rhs;
	while (x<y) {
		rhs = genRHSExpr(lexer, file, x, y, x, bracket);
		if (rhs == nullptr) { return nullptr; };
		rhs->lhs = lhs;
		lhs = rhs;
	};
	if (bracket != 0) {
		if (bracket < 0) {
			bracket *= -1;
			emitErr(lexer.fileName,
			        getLineAndOff(lexer.fileContent, tokOffs[start].off),
			        "Missing %d opening brackets", bracket/10
			        );
		} else {
			emitErr(lexer.fileName,
			        getLineAndOff(lexer.fileContent, tokOffs[start].off),
			        "Missing %d closing brackets", bracket/10
			        );
		};
	};
	return (ASTBase*)lhs;
};
u32 getEndNewlineEOF(DynamicArray<Token_Type>& tokTypes, u32 x) {
	while (tokTypes[x] != (Token_Type)'\n' && tokTypes[x] != Token_Type::END_OF_FILE) {
		x += 1;
	};
	return x;
};

s8 varDeclAddTableEntries(Lexer &lexer, ASTFile &file, u32 &x, DynamicArray<ASTBase*> *table) {
	BRING_TOKENS_TO_SCOPE;
	s8 varCount = 0;
	while (true) {
		if (tokTypes[x] != Token_Type::IDENTIFIER) {
			emitErr(lexer.fileName,
			        getLineAndOff(lexer.fileContent, tokOffs[x].off),
			        "Expected an identifier");
			table->uninit();
			return -1;
		};
		varCount += 1;
		table->push(allocAST(sizeof(ASTBase), ASTType::VARIABLE, x, file));
		x += 1;
		if (tokTypes[x] == (Token_Type)',') {
			x += 1;
			continue;
		};
		return varCount;
	};
	return varCount;
};

ASTBase *parseStatement(Lexer &lexer, ASTFile &file, u32 x) {
	BRING_TOKENS_TO_SCOPE;
	while (tokTypes[x] == (Token_Type)'\n') { x += 1; };
	u32 start = x;
	switch (tokTypes[x]) {
		case Token_Type::IDENTIFIER: {
			x += 1;
			switch (tokTypes[x]) {
				case (Token_Type)':': {
					x += 1;
					switch (tokTypes[x]) {
						case (Token_Type)'=': {
							//single variable assignment
							ASTlr *assign = (ASTlr*)allocAST(sizeof(ASTlr), ASTType::UNI_ASSIGNMENT, x, file);
							assign->lhs = allocAST(sizeof(ASTBase), ASTType::VARIABLE, start, file);
							x += 1;
							u32 end = getEndNewlineEOF(tokTypes, x);
							assign->rhs = genASTExprTree(lexer, file, x, end);
							if (assign->rhs == nullptr) { return nullptr; };
							return (ASTBase*)assign;
						} break;
						case (Token_Type)':': {
							//procedure defenition
							x += 1;
							if (tokTypes[x] != Token_Type::K_PROC) {
								emitErr(lexer.fileName,
								        getLineAndOff(lexer.fileContent, tokOffs[x].off),
								        "Expected 'proc' keyword");
								return nullptr;
							};
							x += 1;
							if (tokTypes[x] != (Token_Type)'(') {
								emitErr(lexer.fileName,
								        getLineAndOff(lexer.fileContent, tokOffs[x].off),
								        "Expected '('");
								return nullptr;
							};
							ASTlr *proc = (ASTlr*)allocASTWithTable(sizeof(ASTlr), ASTType::PROC_DEFENITION, start, file);
							DynamicArray<ASTBase*> *table = getTable(proc, sizeof(ASTlr));
							table->init();
							DynamicArray<ProcArgs> *procArgs = (DynamicArray<ProcArgs>*)mem::alloc(sizeof(DynamicArray<ProcArgs>));
							proc->lhs = (ASTBase*)procArgs;
							procArgs->init();
							s8 y = 0;
							while (true) {
								x += 1;
								y += varDeclAddTableEntries(lexer, file, x, table);
								if (y == -1) { return nullptr; };
								if (tokTypes[x] != (Token_Type)':') {
									emitErr(lexer.fileName,
									        getLineAndOff(lexer.fileContent, tokOffs[x].off),
									        "Expected ':'");
									table->uninit();
									procArgs->uninit();
									return nullptr;
								};
								x += 1;
								if (isKeyword(tokTypes[x]) == false && tokTypes[x] != Token_Type::IDENTIFIER) {
									emitErr(lexer.fileName,
									        getLineAndOff(lexer.fileContent, tokOffs[x].off),
									        "Expected a type");
									table->uninit();
									procArgs->uninit();
									return nullptr;
								};
								procArgs->push({y,x});
								x += 1;
								if (tokTypes[x] == (Token_Type)',') { continue; }
								else if (tokTypes[x] == (Token_Type)')') { break; }
								else {
									emitErr(lexer.fileName,
									        getLineAndOff(lexer.fileContent, tokOffs[x].off),
									        "Expected ','");
									table->uninit();
									procArgs->uninit();
									return nullptr;
								};
							};
							return (ASTBase*)proc;
						} break;
					} break;
				} break;
				case (Token_Type)',': {
					//multiple variable assignment
					ASTlr *assign = (ASTlr*)allocASTWithTable(sizeof(ASTlr), ASTType::MULTI_ASSIGNMENT, NULL, file);
					DynamicArray<ASTBase*> *table = getTable(assign, sizeof(ASTlr));
					table->init(4);
					x = start;
					s8 y = varDeclAddTableEntries(lexer, file, x, table);
					if (y == -1) { return nullptr; };
					if (tokTypes[x] != (Token_Type)':') {
						emitErr(lexer.fileName,
						        getLineAndOff(lexer.fileContent, tokOffs[x].off),
						        "Expected ':'");
						table->uninit();
						return nullptr;
					};
					x += 1;
					if (tokTypes[x] != (Token_Type)'=') {
						emitErr(lexer.fileName,
						        getLineAndOff(lexer.fileContent, tokOffs[x].off),
						        "Expected '='");
						table->uninit();
						return nullptr;
					};
					assign->tokenOff = x;
					x += 1;
					u32 end = getEndNewlineEOF(tokTypes, x);
					assign->rhs = genASTExprTree(lexer, file, x, end);
					if (assign->rhs == nullptr) {
						table->uninit();
						return nullptr;
					};
					return (ASTBase*)assign;
				} break;
			} break;
		} break;
		default: {
			u32 end = getEndNewlineEOF(tokTypes, x);
			return genASTExprTree(lexer, file, x, end);
		} break;
	};
	return nullptr;
};

u32 pow(u32 base, u32 exp){
	u32 num = 1;
	while(exp != 0){
		num *= base;
		exp -= 1;
	};
	return num;
};
//TODO: Hacked together af. REWRITE(return u64)
s64 string2int(String &str){
	s64 num = 0;
	u32 len = str.len;
	for(u32 x=0; x<str.len; x+=1){
		char c = str[x];
		switch(c){
			case '.':
			case '_':
				len -= 1;
				continue;
		};
	};
	u32 y = 0;
	for(u32 x=0; x<str.len; x+=1){
		char c = str[x];
		switch(c){
			case '.':
			case '_':
				continue;
		};
		num += (c - '0') * pow(10, len-y-1);
		y += 1;
	};
	return num;
}

#if(XE_DBG)

#define PAD printf("\n"); for (u8 i = 0; i < padding; i++) { printf("    "); };

void __dumpNodesWithoutEndPadding(ASTBase *node, Lexer &lexer, u8 padding) {
	BRING_TOKENS_TO_SCOPE;
	PAD;
	printf("[NODE]");
	PAD;
	printf("tokenOff: %d", node->tokenOff);
	PAD;
	printf("type: ");
	ASTType type = node->type;
	char c = NULL;
	switch (type) {
		case ASTType::NUM_INTEGER: {
			printf("num_integer");
			TokenOffset off = tokOffs[node->tokenOff];
			String str;
			str.len = off.len;
			str.mem = lexer.fileContent + off.off;
			s64 num = string2int(str);
			PAD;
			printf("num: %lld", num);
		} break;
		case ASTType::NUM_DECIMAL: {
			printf("num_decimal");
			PAD;
			printf("num: %f", 69.69);
		} break;
		case ASTType::BIN_ADD: if (c == NULL) { c = '+'; printf("bin_add"); };
		case ASTType::BIN_SUB: if (c == NULL) { c = '-'; printf("bin_sub"); };
		case ASTType::BIN_MUL: if (c == NULL) { c = '*'; printf("bin_mul"); };
		case ASTType::BIN_DIV:{
			ASTlr *lr = (ASTlr*)node;
			if (c == NULL) { c = '/'; printf("bin_div"); };
			PAD;
			printf("lhs: %p", lr->lhs);
			PAD;
			printf("rhs: %p", lr->rhs);
			PAD;
			printf("op: %c", c);
			PAD;
			printf("LHS");
			if (lr->lhs != nullptr) {__dumpNodesWithoutEndPadding(lr->lhs, lexer, padding + 1);};
			PAD;
			printf("RHS");
			if (lr->rhs != nullptr) {__dumpNodesWithoutEndPadding(lr->rhs, lexer, padding + 1);};
		}break;
		case ASTType::VARIABLE: {
			printf("variable");
			PAD;
			u32 x = node->tokenOff;
			printf("name: %.*s", lexer.tokenOffsets[x].len, lexer.fileContent + lexer.tokenOffsets[x].off);
		}break;
		case ASTType::UNI_ASSIGNMENT: {
			ASTlr *lr = (ASTlr*)node;
			printf("uni_assignment");
			PAD;
			printf("LHS");
			if (lr->lhs != nullptr) {__dumpNodesWithoutEndPadding(lr->lhs, lexer, padding + 1);};
			PAD;
			printf("RHS");
			if (lr->rhs != nullptr) {__dumpNodesWithoutEndPadding(lr->rhs, lexer, padding + 1);};
		} break;
		case ASTType::MULTI_ASSIGNMENT: {
			ASTlr *lr = (ASTlr*)node;
			printf("uni_assignment");
			PAD;
			printf("LHS");
			DynamicArray<ASTBase*> *table = getTable(lr, sizeof(ASTlr));
			for (u8 x=0; x<table->count; x +=1) {
				__dumpNodesWithoutEndPadding(table->getElement(x), lexer, padding + 1);
				PAD;
			};
			PAD;
			printf("RHS");
			if (lr->rhs != nullptr) {__dumpNodesWithoutEndPadding(lr->rhs, lexer, padding + 1);};
		} break;
		case ASTType::PROC_DEFENITION: {
			ASTlr *lr = (ASTlr*)node;
			u32 x = node->tokenOff;
			printf("proc_defenition");
			PAD;
			printf("name: %.*s", lexer.tokenOffsets[x].len, lexer.fileContent + lexer.tokenOffsets[x].off);
			PAD;
			printf("[ARGS]");
			DynamicArray<ASTBase*> *table = getTable(lr, sizeof(ASTlr));
			DynamicArray<ProcArgs> *args = (DynamicArray<ProcArgs>*)lr->lhs;
			u8 argOff = 0;
			ProcArgs procArg = args->getElement(argOff);
			for (u8 x = 0; x < table->count; x += 1) {
				padding += 1;
				__dumpNodesWithoutEndPadding(table->getElement(x), lexer, padding);
				PAD;
				if (x == procArg.count) {
					argOff += 1;
					procArg = args->getElement(argOff);
				};
				u32 z = procArg.typeOff;
				printf("type: %.*s", lexer.tokenOffsets[z].len, lexer.fileContent + lexer.tokenOffsets[z].off);
				padding -= 1;
				PAD;
			};
		} break;
		default: DEBUG_UNREACHABLE;
	};
};
void dumpNodes(ASTBase *node, Lexer &lexer, u8 padding = 0){
	__dumpNodesWithoutEndPadding(node, lexer, padding);
	printf("\n----\n");
};
#endif