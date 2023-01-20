#define AST_PAGE_SIZE 1024

enum class ASTType {
	NUM_INTEGER,
	NUM_DECIMAL,
	BIN_ADD,
	BIN_SUB,
	BIN_MUL,
	BIN_DIV,
	VAR,
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

ASTBase *genASTOperand(Lexer &lexer, u32 &x, char *fileContent, ASTFile &file, s16 &bracket) {
	DynamicArray<Token_Type> &tokTypes = lexer.tokenTypes;
	DynamicArray<TokenOffset> &tokOffs = lexer.tokenOffsets;
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
	DynamicArray<Token_Type> &tokTypes = lexer.tokenTypes;
	DynamicArray<TokenOffset> &tokOffs = lexer.tokenOffsets;
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
	DynamicArray<Token_Type> &tokTypes = lexer.tokenTypes;
	DynamicArray<TokenOffset> &tokOff = lexer.tokenOffsets;
	ASTType type;
	switch (lexer.fileContent[tokOff[x].off]) {
		case '+': type = ASTType::BIN_ADD; break;
		case '-': type = ASTType::BIN_SUB; break;
		case '*': type = ASTType::BIN_MUL; break;
		case '/': type = ASTType::BIN_DIV; break;
		default: DEBUG_UNREACHABLE;
	};
	return (ASTlr*)allocAST(sizeof(ASTlr), type, x, file);
};
ASTlr *genRHSExpr(Lexer &lexer, ASTFile &file, u32 x, u32 y, u32 &curPos, s16 &bracket) {
	DynamicArray<Token_Type> &tokTypes = lexer.tokenTypes;
	DynamicArray<TokenOffset> &tokOff = lexer.tokenOffsets;
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
		if (x > y) { break; };
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
	DynamicArray<Token_Type> &tokTypes = lexer.tokenTypes;
	DynamicArray<TokenOffset> &tokOffs = lexer.tokenOffsets;
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

void dumpNodes(ASTBase *node, Lexer &lexer, u8 padding=0) {
	DynamicArray<Token_Type> &tokTypes = lexer.tokenTypes;
	DynamicArray<TokenOffset> &tokOffs = lexer.tokenOffsets;
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
			if (lr->lhs != nullptr) {dumpNodes(lr->lhs, lexer, padding + 1);};
			PAD;
			printf("RHS");
			if (lr->rhs != nullptr) {dumpNodes(lr->rhs, lexer, padding + 1);};
		}break;
		default: DEBUG_UNREACHABLE;
	};
};
#endif