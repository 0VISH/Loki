#define AST_PAGE_SIZE 1024
#define BRING_TOKENS_TO_SCOPE DynamicArray<Token_Type> &tokTypes = lexer.tokenTypes;DynamicArray<TokenOffset> &tokOffs = lexer.tokenOffsets;

enum class ASTType {
	NUM_INTEGER,
	NUM_DECIMAL,
	BIN_ADD,
	BIN_SUB,
	BIN_MUL,
	BIN_DIV,
	UNI_ASSIGNMENT_T_UNKNOWN,
	UNI_ASSIGNMENT_T_KNOWN,
	MULTI_ASSIGNMENT_T_UNKNOWN,
	MULTI_ASSIGNMENT_T_KNOWN,
	PROC_DEFENITION,
	UNI_DECLERATION_T_KNOWN,
	MULTI_DECLERATION_T_KNOWN,
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
	DynamicArray<ASTBase*> nodes;
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
struct Arguments {
	DynamicArray<ProcArgs> argsMeta;
	DynamicArray<ASTBase*> args;
};

ASTFile createASTFile() {
	ASTFile file;
	file.memPages.init(2);
	char *page = (char*)mem::alloc(AST_PAGE_SIZE);
	file.memPages.push(page);
	file.pageBrim = 0;
	file.nodes.init(10);
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
			lexer.emitErr(tokOffs[x].off, "Invalid operand");
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
			lexer.emitErr(tokOffs[x].off, "Invalid operator");
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
//                                                begin off, end off+1
ASTBase *genASTExprTree(Lexer &lexer, ASTFile &file, u32 &x, u32 y) {
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
			lexer.emitErr(tokOffs[start].off, "Missing %d opening brackets", bracket / 10);
		} else {
			lexer.emitErr(tokOffs[start].off, "Missing %d closing brackets", bracket/10);
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
			lexer.emitErr(tokOffs[x].off, "Expected an identifier");
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

bool isType(Token_Type type) {
	return (type>Token_Type::K_START && type<Token_Type::K_PROC) || type == Token_Type::IDENTIFIER;
};

ASTBase *parseBlock(Lexer &lexer, ASTFile &file, u32 &x) {
	BRING_TOKENS_TO_SCOPE;
	eatNewlines(tokTypes, x);
	u32 start = x;
	switch (tokTypes[x]) {
		case Token_Type::IDENTIFIER: {
			x += 1;
			switch (tokTypes[x]) {
				case (Token_Type)':': {
					x += 1;
					b8 flag = false;
					switch (tokTypes[x]) {
						case (Token_Type)'=': {
							//single variable assignment
							SINGLE_VARIABLE_ASSIGNMENT:
							ASTType type = (flag)?ASTType::UNI_ASSIGNMENT_T_KNOWN:ASTType::UNI_ASSIGNMENT_T_UNKNOWN;
							ASTlr *assign = (ASTlr*)allocAST(sizeof(ASTlr), type, x, file);
							assign->lhs = allocAST(sizeof(ASTBase), ASTType::VARIABLE, start, file);
							x += 1;
							u32 end = getEndNewlineEOF(tokTypes, x);
							assign->rhs = genASTExprTree(lexer, file, x, end);
							return (ASTBase*)assign;
						} break;
						case (Token_Type)':': {
							//procedure defenition
							x += 1;
							if (tokTypes[x] != Token_Type::K_PROC) {
								lexer.emitErr(tokOffs[x].off, "Expected 'proc' keyword");
								return nullptr;
							};
							x += 1;
							if (tokTypes[x] != (Token_Type)'(') {
								lexer.emitErr(tokOffs[x].off, "Expected '('");
								return nullptr;
							};
							x += 1;
							ASTlr *proc = (ASTlr*)allocASTWithTable(sizeof(ASTlr), ASTType::PROC_DEFENITION, start, file);
							DynamicArray<ASTBase*> *table = getTable(proc, sizeof(ASTlr));
							table->init();
							DynamicArray<ProcArgs> *procArgs = nullptr;
							if (tokTypes[x] == (Token_Type)')') {
								proc->lhs = nullptr;
								goto PARSE_AFTER_ARGS;
							};
							Arguments *arguments = (Arguments*)mem::alloc(sizeof(Arguments));
							procArgs = &(arguments->argsMeta);
							DynamicArray<ASTBase*> *args = &(arguments->args);
							proc->lhs = (ASTBase*)arguments;
							procArgs->init();
							args->init();
							s8 y = 0;
							while (true) {
								eatNewlines(tokTypes, x);
								y += varDeclAddTableEntries(lexer, file, x, args);
								if (y == -1) { return nullptr; };
								if (tokTypes[x] != (Token_Type)':') {
									lexer.emitErr(tokOffs[x].off, "Expected ':'");
									table->uninit();
									procArgs->uninit();
									args->uninit();
									return nullptr;
								};
								x += 1;
								if (isType(tokTypes[x]) == false) {
									lexer.emitErr(tokOffs[x].off, "Expected a type");
									table->uninit();
									procArgs->uninit();
									args->uninit();
									return nullptr;
								};
								procArgs->push({y,x});
								x += 1;
								if (tokTypes[x] == (Token_Type)',') { x += 1; continue; }
								else if (tokTypes[x] == (Token_Type)')') { break; }
								else {
									lexer.emitErr(tokOffs[x].off, "Expected ','");
									table->uninit();
									procArgs->uninit();
									args->uninit();
									return nullptr;
								};
							};
							PARSE_AFTER_ARGS:
							x += 1;
							eatNewlines(tokTypes, x);
							switch (tokTypes[x]) {
								case Token_Type::ARROW: {
									x += 1;
									DynamicArray<u32> *returnTypeOffs = (DynamicArray<u32>*)mem::alloc(sizeof(DynamicArray<u8>));
									proc->rhs = (ASTBase*)returnTypeOffs;
									returnTypeOffs->init();
									bool bracket = false;
									if (tokTypes[x] == (Token_Type)'(') { bracket = true; x += 1; };
									while (true) {
										if (isType(tokTypes[x]) == false) {
											lexer.emitErr(tokOffs[x].off, "Expected a type");
											table->uninit();
											if (procArgs == nullptr) { return nullptr; };
											procArgs->uninit();
											args->uninit();
											return nullptr;
										};
										returnTypeOffs->push(x);
										x += 1;
										eatNewlines(tokTypes, x);
										if (tokTypes[x] == (Token_Type)',') { x += 1; continue; };
										if (tokTypes[x] == (Token_Type)')') {
											if (bracket != false) {
												x += 1;
												eatNewlines(tokTypes, x);
												break;
											};
											lexer.emitErr(tokOffs[x].off, "Did not expect ')'");
											table->uninit();
											if (procArgs == nullptr) { return nullptr; };
											procArgs->uninit();
											args->uninit();
											return nullptr;
										};
										if (tokTypes[x] != (Token_Type)'{') {
											lexer.emitErr(tokOffs[x].off, "Expected '{'");
											table->uninit();
											if (procArgs == nullptr) { return nullptr; };
											procArgs->uninit();
											args->uninit();
											return nullptr;
										};
										if (bracket == true) {
											lexer.emitErr(tokOffs[x].off, "Expected ')'");
											table->uninit();
											if (procArgs == nullptr) { return nullptr; };
											procArgs->uninit();
											args->uninit();
											return nullptr;
										};
										break;
									};
									x += 1;
								} break;
								case (Token_Type)'{': {
									proc->rhs = nullptr;
									x += 1;
								} break;
								default: {
									lexer.emitErr(tokOffs[x].off, "Expected '{' or '->'");
									table->uninit();
									if (procArgs == nullptr) { return nullptr; };
									procArgs->uninit();
									args->uninit();
									return nullptr;
								} break;
							};
							eatNewlines(tokTypes, x);
							while (tokTypes[x] != (Token_Type)'}') {
								ASTBase *node = parseBlock(lexer, file, x);
								if (node == nullptr) {
									table->uninit();
									if (procArgs == nullptr) { return nullptr; };
									procArgs->uninit();
									args->uninit();
									return nullptr;
								};
								table->push(node);
								x += 1;
							};
							x += 1;
							return (ASTBase*)proc;
						} break;
						default: {
							if (isType(tokTypes[x])) {
								flag = true;
								x += 1;
								if (tokTypes[x] == (Token_Type)'=') { goto SINGLE_VARIABLE_ASSIGNMENT; };
								ASTlr *assign = (ASTlr*)allocAST(sizeof(ASTlr), ASTType::UNI_DECLERATION_T_KNOWN, x, file);
								assign->lhs = allocAST(sizeof(ASTBase), ASTType::VARIABLE, start, file);
								return (ASTBase*)assign;
							};
						} break;
					} break;
				} break;
				case (Token_Type)',': {
					//multiple variable decleration
					ASTlr *base = (ASTlr*)allocASTWithTable(sizeof(ASTlr), ASTType::MULTI_DECLERATION_T_KNOWN, NULL, file);
					DynamicArray<ASTBase*> *table = getTable(base, sizeof(ASTlr));
					table->init(4);
					x = start;
					s8 y = varDeclAddTableEntries(lexer, file, x, table);
					if (y == -1) { return nullptr; };
					if (tokTypes[x] != (Token_Type)':') {
						lexer.emitErr(tokOffs[x].off, "Expected ':'");
						table->uninit();
						return nullptr;
					};
					x += 1;
					if (isType(tokTypes[x])) {
						x += 1;
						if (tokTypes[x] != (Token_Type)'=') {
							base->type = ASTType::MULTI_DECLERATION_T_KNOWN;
							return (ASTBase*)base;
						};
						base->type = ASTType::MULTI_ASSIGNMENT_T_KNOWN;
					} else if (tokTypes[x] == (Token_Type)'=') {
						base->type = ASTType::MULTI_ASSIGNMENT_T_UNKNOWN;
					} else {
						lexer.emitErr(tokOffs[x].off, "Expected a type or '='");
						table->uninit();
						return nullptr;
					};
					base->tokenOff = x;
					x += 1;
					u32 end = getEndNewlineEOF(tokTypes, x);
					base->rhs = genASTExprTree(lexer, file, x, end);
					return (ASTBase*)base;
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

namespace dbg {
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
		u8 flag = false;
		ASTlr *lr = (ASTlr*)node;
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
			case ASTType::UNI_DECLERATION_T_KNOWN: {
				u32 x = node->tokenOff - 1;
				printf("uni_decleration_t_known");
				PAD;
				printf("type: %.*s", lexer.tokenOffsets[x].len, lexer.fileContent + lexer.tokenOffsets[x].off);
				PAD;
				printf("LHS");
				__dumpNodesWithoutEndPadding(lr->lhs, lexer, padding + 1);
			} break;
			case ASTType::UNI_ASSIGNMENT_T_KNOWN: {
				u32 x = node->tokenOff - 1;
				printf("uni_assignment_t_known");
				PAD;
				printf("type: %.*s", lexer.tokenOffsets[x].len, lexer.fileContent + lexer.tokenOffsets[x].off);
			};
			flag = true;
			case ASTType::UNI_ASSIGNMENT_T_UNKNOWN: {
				if (flag) { flag = false; }
				else { printf("uni_assignment_t_unkown"); };	
				PAD;
				printf("LHS");
				if (lr->lhs != nullptr) {__dumpNodesWithoutEndPadding(lr->lhs, lexer, padding + 1);};
				PAD;
				printf("RHS");
				if (lr->rhs != nullptr) {__dumpNodesWithoutEndPadding(lr->rhs, lexer, padding + 1);};
			} break;
			case ASTType::MULTI_ASSIGNMENT_T_KNOWN: {
				u32 x = node->tokenOff - 1;
				printf("multi_assignment_t_known");
				PAD;
				printf("type: %.*s", lexer.tokenOffsets[x].len, lexer.fileContent + lexer.tokenOffsets[x].off);
			};
			flag = true;
			case ASTType::MULTI_ASSIGNMENT_T_UNKNOWN: {
				if (flag) { flag = false; }
				else { printf("multi_assignment_t_unkown"); };
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
				u32 x = node->tokenOff;
				printf("proc_defenition");
				PAD;
				printf("name: %.*s", lexer.tokenOffsets[x].len, lexer.fileContent + lexer.tokenOffsets[x].off);
				PAD;
				printf("[ARGS]");
				DynamicArray<ASTBase*> *table = getTable(lr, sizeof(ASTlr));
				Arguments *arguments = (Arguments*)lr->lhs;
				if (arguments == nullptr) { 
					PAD;
					printf("null");
					goto BODY;
				};
				DynamicArray<ProcArgs> &procArgs = arguments->argsMeta;
				DynamicArray<ASTBase*> &args = arguments->args;
				u8 argOff = 0;
				ProcArgs procArg = procArgs[argOff];
				for (u8 v=0; v < args.count; v += 1) {
					padding += 1;
					__dumpNodesWithoutEndPadding(args[v], lexer, padding);
					PAD;
					if (v == procArg.count) {
						argOff += 1;
						procArg = procArgs[argOff];
					};
					u32 z = procArg.typeOff;
					printf("type: %.*s", lexer.tokenOffsets[z].len, lexer.fileContent + lexer.tokenOffsets[z].off);
					padding -= 1;
					PAD;
				};
				BODY:
				PAD;
				printf("[BODY]");
				for (u32 v=0; v < table->count; v += 1) {
					__dumpNodesWithoutEndPadding(table->getElement(v), lexer, padding + 1);
					PAD;
				};
				PAD;
				printf("[RETURN]");
				DynamicArray<u32> *returnTypeOffs = (DynamicArray<u32>*)lr->rhs;
				if (returnTypeOffs == nullptr) {
					PAD;
					printf("type: void");
				} else {
					for (u32 v=0; v<returnTypeOffs->count; v+=1) {
						u32 z = returnTypeOffs->getElement(v);
						PAD;
						printf("type: %.*s", lexer.tokenOffsets[z].len, lexer.fileContent + lexer.tokenOffsets[z].off);
					};
				};
			} break;
			default: DEBUG_UNREACHABLE;
		};
	};
	void dumpNodes(ASTBase *node, Lexer &lexer, u8 padding = 0) {
		__dumpNodesWithoutEndPadding(node, lexer, padding);
		printf("\n----\n");
	};
	void dumpASTFile(ASTFile &file, Lexer &lexer) {
		for (u32 x = 0; x < file.nodes.count; x += 1) {
			ASTBase *node = file.nodes[x];
			dumpNodes(node, lexer);
		};
	};
};
#endif