#define AST_PAGE_SIZE 1024
#define BRING_TOKENS_TO_SCOPE DynamicArray<Token_Type> &tokTypes = lexer.tokenTypes;DynamicArray<TokenOffset> &tokOffs = lexer.tokenOffsets;

enum class ASTType {
    NUM_INTEGER,
    NUM_DECIMAL,
    BIN_ADD,
    BIN_MUL,
    BIN_DIV,
    UNI_DECLERATION,
    UNI_ASSIGNMENT_T_UNKNOWN,
    UNI_ASSIGNMENT_T_KNOWN,
    MULTI_DECLERATION,
    MULTI_ASSIGNMENT_T_UNKNOWN,
    MULTI_ASSIGNMENT_T_KNOWN,
    PROC_DEFENITION,
    VARIABLE,
    TYPE,
    UNI_NEG,
};

struct ASTBase {
    ASTType type;
};
struct ASTNumInt : ASTBase{
    u32 tokenOff;
};
struct ASTNumDec : ASTBase{
    u32 tokenOff;
};
struct ASTBinOp : ASTBase{
    ASTBase *lhs;
    ASTBase *rhs;
    Type lhsType;
    Type rhsType;
};
struct ASTUniVar : ASTBase{
    String name;
    ASTBase *rhs;
    u32 tokenOff;
    u8 flag;
};
struct ASTMultiVar : ASTBase{
    DynamicArray<String> names;
    ASTBase *rhs;
    u32 tokenOff;
    u8 flag;
};
struct ASTUniOp : ASTBase{
    ASTBase *node;
};
struct ASTProcDef : ASTBase {
    DynamicArray<ASTBase*> body;    //NOTE: also includes input
    DynamicArray<ASTBase*> out;
    String name;
    u32 startProcInID;
    u32 tokenOff;
    u32 inCount;
    u32 inCommaCount;
    u8 flag;
};
struct ASTVariable : ASTBase{
    String name;
    u32 tokenOff;
};
struct AST_Type : ASTBase{
    union{
	u32 tokenOff;
	Type type;
    };
};
struct ASTFile {
    DynamicArray<char*> memPages;
    DynamicArray<ASTBase*> nodes;
    u16 pageBrim;

    void init(){
	memPages.init(2);
	char *page = (char*)mem::alloc(AST_PAGE_SIZE);
	memPages.push(page);
	pageBrim = 0;
	nodes.init(10);
    };
    void uninit(){
	for (u16 i=0; i<memPages.count; i++) {
	    mem::free(memPages[i]);
	};
	memPages.uninit();
	nodes.uninit();
    };
};

String makeStringFromTokOff(u32 x, Lexer &lexer){
    BRING_TOKENS_TO_SCOPE;
    TokenOffset off = tokOffs[x];
    String str;
    str.len = off.len;
    str.mem = lexer.fileContent + off.off;
    return str;
};
ASTBase *allocAST(u32 nodeSize, ASTType type, ASTFile &file) {
    if (file.pageBrim+nodeSize >= AST_PAGE_SIZE) {
	char *page = (char*)mem::alloc(AST_PAGE_SIZE);
	file.memPages.push(page);
	file.pageBrim = 0;
    };
    u8 memPageOff = file.memPages.count-1;
    ASTBase *node = (ASTBase*)(file.memPages[memPageOff] + file.pageBrim);
    file.pageBrim += nodeSize;
    node->type = type;
    return node;
};

ASTBinOp *genASTOperator(Lexer &lexer, u32 &x, ASTFile &file) {
    BRING_TOKENS_TO_SCOPE;
    ASTType type;
    switch (tokTypes[x]) {
    case (Token_Type)'-': x -= 1;//NOTE: sub is a uni operator
    case (Token_Type)'+': type = ASTType::BIN_ADD; x += 1; break;
    case (Token_Type)'*': type = ASTType::BIN_MUL; x += 1; break;
    case (Token_Type)'/': type = ASTType::BIN_DIV; x += 1; break;
    default: DEBUG_UNREACHABLE;
    };
    return (ASTBinOp*)allocAST(sizeof(ASTBinOp), type, file);
};
ASTBase *genASTOperand(Lexer &lexer, u32 &x, ASTFile &file, s16 &bracket) {
    BRING_TOKENS_TO_SCOPE;
 CHECK_TYPE_AST_OPERAND:
    Token_Type type = tokTypes[x];
    switch (type) {
    case (Token_Type)'-':{
	ASTUniOp *uniOp = (ASTUniOp*)allocAST(sizeof(ASTUniOp), ASTType::UNI_NEG, file);
	x += 1;
	ASTBase *node = genASTOperand(lexer, x, file, bracket);
	uniOp->node = node;
	return (ASTBase*)uniOp;
    }break;
    case Token_Type::INTEGER: {
	ASTNumInt *numNode = (ASTNumInt*)allocAST(sizeof(ASTNumInt), ASTType::NUM_INTEGER, file);
	numNode->tokenOff = x;
	x += 1;
	return (ASTBase*)numNode;
    } break;
    case Token_Type::DECIMAL:{
	ASTNumDec *numNode = (ASTNumDec*)allocAST(sizeof(ASTNumDec), ASTType::NUM_DECIMAL, file);
	numNode->tokenOff = x;
	x += 1;
	return (ASTBase*)numNode;
    }break;
    case (Token_Type)'(': {
	while (tokTypes[x] == (Token_Type)'(') {
	    bracket += 10;
	    x += 1;
	};
	goto CHECK_TYPE_AST_OPERAND;
    }break;
    case Token_Type::IDENTIFIER:{
	ASTVariable *var = (ASTVariable*)allocAST(sizeof(ASTVariable), ASTType::VARIABLE, file);
	var->name = makeStringFromTokOff(x, lexer);
	var->tokenOff = x;
	x += 1;
	return (ASTBase*)var;
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
ASTBinOp *genRHSExpr(Lexer &lexer, ASTFile &file, u32 &curPos, u32 y, s16 &bracket) {
    BRING_TOKENS_TO_SCOPE;
    u32 x = curPos;
    s16 prio = checkAndGetPrio(lexer, x);
    if (prio == -1) { return nullptr; };
    ASTBinOp *node = genASTOperator(lexer, x, file);
    ASTBinOp *previousOperator = node;
    ASTBinOp *binOperator = nullptr;
    ASTBase *operand = nullptr;
    while (x < y) {
	operand = genASTOperand(lexer, x, file, bracket);
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
    };
    previousOperator->rhs = operand;
    curPos = x;
    return node;
};
ASTBase *genASTExprTree(Lexer &lexer, ASTFile &file, u32 &x, u32 end) {
    u32 start = x;
    BRING_TOKENS_TO_SCOPE;
    Token_Type type = tokTypes[x];
    s16 bracket = 0;
    ASTBase *lhs = genASTOperand(lexer, x, file, bracket);
    if (lhs == nullptr) { return nullptr; };
    while (x<end) {
	ASTBinOp *rhs = genRHSExpr(lexer, file, x, end, bracket);
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
    //SIMD?
    while (tokTypes[x] != (Token_Type)'\n' && tokTypes[x] != Token_Type::END_OF_FILE) {
	x += 1;
    };
    return x;
};

s8 varDeclAddTableEntriesAST(Lexer &lexer, ASTFile &file, u32 &x, DynamicArray<ASTBase*> &table, bool typesIncluded=false) {
    BRING_TOKENS_TO_SCOPE;
    s8 varCount = 0;
    while (true) {
	bool a;
	if (typesIncluded) { a = isType(tokTypes[x]); }
	else { a = tokTypes[x] == Token_Type::IDENTIFIER; };
	if (a == false) {
	    lexer.emitErr(tokOffs[x].off, "Expected identifier%s", (typesIncluded)?" or a type":" ");
	    table.uninit();
	    return -1;
	};
	varCount += 1;
	ASTVariable *var = (ASTVariable*)allocAST(sizeof(ASTVariable), ASTType::VARIABLE, file);
	var->name = makeStringFromTokOff(x, lexer);
	table.push((ASTBase*)var);
	x += 1;
	if (tokTypes[x] == (Token_Type)',') {
	    x += 1;
	    continue;
	};
	return varCount;
    };
    return varCount;
};
s8 varDeclAddTableEntriesStr(Lexer &lexer, ASTFile &file, u32 &x, DynamicArray<String> &table) {
    BRING_TOKENS_TO_SCOPE;
    s8 varCount = 0;
    while (true) {
	if (tokTypes[x] != Token_Type::IDENTIFIER) {
	    lexer.emitErr(tokOffs[x].off, "Expected an identifier");
	    table.uninit();
	    return -1;
	};
	varCount += 1;
	table.push(makeStringFromTokOff(x, lexer));
	x += 1;
	if (tokTypes[x] == (Token_Type)',') {
	    x += 1;
	    continue;
	};
	return varCount;
    };
    return varCount;
};
void uninitProcDef(ASTProcDef *node){
    if(node->body.len != 0)  {node->body.uninit();};
    if(node->out.len != 0) {node->out.uninit();};
};
ASTBase *parseBlock(Lexer &lexer, ASTFile &file, u32 &x);
ASTBase* parseType(u32 x, Lexer &lexer, ASTFile &file){
    BRING_TOKENS_TO_SCOPE;
    AST_Type *type = (AST_Type*)allocAST(sizeof(AST_Type), ASTType::TYPE, file);
    type->tokenOff = x;
    return (ASTBase*)type;
};
ASTBase *parseBlockInner(Lexer &lexer, ASTFile &file, u32 &x, Flag &flag, u32 &flagStart) {
    BRING_TOKENS_TO_SCOPE;
    flagStart = x;
    while(tokTypes[x] == Token_Type::K_CONSTANT || tokTypes[x] == Token_Type::K_COMPTIME){
	if(tokTypes[x] == Token_Type::K_CONSTANT){
	    SET_BIT(flag, Flags::CONSTANT);
	}else if(tokTypes[x] == Token_Type::K_COMPTIME){
	    SET_BIT(flag, Flags::COMPTIME);
	};
	x += 1;
    };
    u32 start = x;
    switch (tokTypes[x]) {
    case Token_Type::IDENTIFIER: {
	x += 1;
	switch (tokTypes[x]) {
	case (Token_Type)':': {
	    x += 1;
	    ASTType uniVarType = ASTType::UNI_ASSIGNMENT_T_UNKNOWN; 
	    switch (tokTypes[x]) {
	    case (Token_Type)'=': {
		//single variable assignment
		SINGLE_VARIABLE_ASSIGNMENT:
		ASTUniVar *assign = (ASTUniVar*)allocAST(sizeof(ASTUniVar), uniVarType, file);
		assign->tokenOff = start;
		assign->name = makeStringFromTokOff(start, lexer);
		x += 1;
		u32 end = getEndNewlineEOF(tokTypes, x);
		assign->rhs = genASTExprTree(lexer, file, x, end);
		if(assign->rhs == nullptr){return nullptr;};
		assign->flag = flag;
		flag = 0;
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
		ASTProcDef *proc = (ASTProcDef*)allocAST(sizeof(ASTProcDef), ASTType::PROC_DEFENITION, file);
		proc->tokenOff = start;
		proc->flag = flag;
		flag = 0;
		proc->name = makeStringFromTokOff(start, lexer);
		proc->body.init();
		proc->out.len = 0;
		proc->inCommaCount = 0;
		if (tokTypes[x] == (Token_Type)')') {
		    goto PARSE_AFTER_ARGS;
		};
		u32 inCount = 0;
		u32 inCommaCount = 0;
		//parse input
		while (true) {
		    inCommaCount += 1;
		    eatNewlines(tokTypes, x);
		    ASTBase *base = parseBlock(lexer, file, x);
		    if(base == nullptr){
			uninitProcDef(proc);
			return nullptr;
		    };
		    switch(base->type){
		    case ASTType::UNI_DECLERATION: inCount += 1;break;
		    case ASTType::MULTI_DECLERATION:
			ASTMultiVar *multiVar = (ASTMultiVar*)base;
			inCount += multiVar->names.count;
			break;
		    };
		    proc->body.push(base);
		    if(tokTypes[x] == (Token_Type)','){
			x += 1;
			continue;
		    };
		    break;
		};
		proc->inCount = inCount;
		proc->inCommaCount = inCommaCount;
		if(tokTypes[x] != (Token_Type)')'){
		    lexer.emitErr(tokOffs[x].off, "Expected ')'");
		    return nullptr;
		};
		PARSE_AFTER_ARGS:
		x += 1;
		eatNewlines(tokTypes, x);
		switch (tokTypes[x]) {
		case Token_Type::ARROW: {
		    x += 1;
		    proc->out.init();
		    bool bracket = false;
		    if (tokTypes[x] == (Token_Type)'(') { bracket = true; x += 1; };
		    //parse output
		    while (true) {
			eatNewlines(tokTypes, x);
			ASTBase *typeNode = parseType(x, lexer, file);
			if(typeNode == nullptr){
			    uninitProcDef(proc);
			    return nullptr;
			};
			x += 1;
			proc->out.push(typeNode);
			if(tokTypes[x] == (Token_Type)','){
			    x += 1;
			    continue;
			};
			break;
		    };
		    if (tokTypes[x] == (Token_Type)')') {
			if (bracket == false) {
			    lexer.emitErr(tokOffs[x].off, "Did not expect ')'");
			    uninitProcDef(proc);
			    return nullptr;
			};
			x += 1;
			eatNewlines(tokTypes, x);
		    } else if(bracket == true) {
			lexer.emitErr(tokOffs[x].off, "Expected ')'");
		        uninitProcDef(proc);
			return nullptr;
		    };
		    if (tokTypes[x] != (Token_Type)'{') {
			lexer.emitErr(tokOffs[x].off, "Expected '{'");
		        uninitProcDef(proc);
			return nullptr;
		    };
		    x += 1;
		} break;
		case (Token_Type)'{': {
		    x += 1;
		} break;
		default: {
		    lexer.emitErr(tokOffs[x].off, "Expected '{' or '->'");
		    uninitProcDef(proc);
		    return nullptr;
		} break;
		};
		eatNewlines(tokTypes, x);
		if (tokTypes[x] == (Token_Type)'}') {
		    x += 1;
		    return (ASTBase*)proc;
		};
		while (tokTypes[x] != (Token_Type)'}') {
		    //parse body
		    ASTBase *node = parseBlock(lexer, file, x);
		    if (node == nullptr) {
		        uninitProcDef(proc);
			return nullptr;
		    };
		    proc->body.push(node);
		    x += 1;
		    eatNewlines(tokTypes, x);
		};
		x += 1;
		return (ASTBase*)proc;
	    } break;
	    default: {
		if (isType(tokTypes[x])) {
		    uniVarType = ASTType::UNI_ASSIGNMENT_T_KNOWN;
		    x += 1;
		    if (tokTypes[x] == (Token_Type)'=') { goto SINGLE_VARIABLE_ASSIGNMENT; };
		    ASTUniVar *assign = (ASTUniVar*)allocAST(sizeof(ASTUniVar), ASTType::UNI_DECLERATION, file);
		    assign->tokenOff = start;
		    assign->name = makeStringFromTokOff(start, lexer);
		    assign->flag = flag;
		    flag = 0;
		    return (ASTBase*)assign;
		}else{
		    lexer.emitErr(tokOffs[x].off, "Expected a type");
		    return nullptr;
		};
	    } break;
	    } break;
	} break;
	case (Token_Type)',': {
	    //multi var def/decl
	    ASTMultiVar *multiAss = (ASTMultiVar*)allocAST(sizeof(ASTMultiVar), ASTType::MULTI_DECLERATION, file);
	    DynamicArray<String> &names = multiAss->names;
	    names.init(3);
	    x = start;
	    s8 y = varDeclAddTableEntriesStr(lexer, file, x, names);
	    if (y == -1) { return nullptr; };
	    if (tokTypes[x] != (Token_Type)':') {
		lexer.emitErr(tokOffs[x].off, "Expected ':'");
		names.uninit();
		return nullptr;
	    };
	    multiAss->tokenOff = x-1;
	    x += 1;
	    if (isType(tokTypes[x])) {
		x += 1;
		if (tokTypes[x] != (Token_Type)'=') {
		    multiAss->type = ASTType::MULTI_DECLERATION;
		    return (ASTBase*)multiAss;
		};
		multiAss->type = ASTType::MULTI_ASSIGNMENT_T_KNOWN;
	    } else if (tokTypes[x] == (Token_Type)'=') {
		multiAss->type = ASTType::MULTI_ASSIGNMENT_T_UNKNOWN;
	    } else {
		lexer.emitErr(tokOffs[x].off, "Expected a type or '='");
		names.uninit();
		return nullptr;
	    };
	    x += 1;
	    u32 end = getEndNewlineEOF(tokTypes, x);
	    multiAss->rhs = genASTExprTree(lexer, file, x, end);
	    multiAss->flag = flag;
	    flag = 0;
	    return (ASTBase*)multiAss;
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
ASTBase *parseBlock(Lexer &lexer, ASTFile &file, u32 &x){
    Flag flag = 0;
    u32 flagStart = 0;
    ASTBase *base = parseBlockInner(lexer, file, x, flag, flagStart);
    if(flag != 0){
	BRING_TOKENS_TO_SCOPE;
	lexer.emitErr(tokOffs[flagStart].off, "Unexpected flags");
	return nullptr;
    };
    return base;
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
f64 string2float(String &str){
    u32 decimal = 0;
    while(str[decimal] != '.'){decimal += 1;};
    u32 postDecimalBadChar = 0;
    for(u32 x=decimal+1; x<str.len; x+=1){
	if(str[x] == '_'){postDecimalBadChar += 1;};
    };
    s64 num = string2int(str);
    return (f64)num/pow(10, str.len-decimal-1-postDecimalBadChar);
};

#if(XE_DBG)

#define PAD printf("\n"); for (u8 i = 0; i < padding; i++) { printf("    "); };

namespace dbg {
    void __dumpNodesWithoutEndPadding(ASTBase *node, Lexer &lexer, u8 padding) {
	BRING_TOKENS_TO_SCOPE;
	PAD;
	printf("[NODE]");
	PAD;
	printf("type: ");
	ASTType type = node->type;
	char c = NULL;
	u8 flag = false;
	switch (type) {
	case ASTType::NUM_INTEGER: {
	    ASTNumInt *numInt = (ASTNumInt*)node;
	    printf("num_integer");
	    String str = makeStringFromTokOff(numInt->tokenOff, lexer);
	    s64 num = string2int(str);
	    PAD;
	    printf("num: %lld", num);
	} break;
	case ASTType::NUM_DECIMAL: {
	    printf("num_decimal");
	    ASTNumDec *numDec = (ASTNumDec*)node;
	    String str = makeStringFromTokOff(numDec->tokenOff, lexer);
	    f64 num = string2float(str);
	    PAD;
	    printf("num: %f", num);
	} break;
	case ASTType::BIN_ADD: if (c == NULL) { c = '+'; printf("bin_add"); };
	case ASTType::BIN_MUL: if (c == NULL) { c = '*'; printf("bin_mul"); };
	case ASTType::BIN_DIV:{
	    ASTBinOp *bin = (ASTBinOp*)node;
	    if (c == NULL) { c = '/'; printf("bin_div"); };
	    PAD;
	    printf("lhs: %p", bin->lhs);
	    PAD;
	    printf("rhs: %p", bin->rhs);
	    PAD;
	    printf("op: %c", c);
	    PAD;
	    printf("LHS");
	    if (bin->lhs != nullptr) {__dumpNodesWithoutEndPadding(bin->lhs, lexer, padding + 1);};
	    PAD;
	    printf("RHS");
	    if (bin->rhs != nullptr) {__dumpNodesWithoutEndPadding(bin->rhs, lexer, padding + 1);};
	}break;
	case ASTType::VARIABLE: {
	    printf("variable");
	    PAD;
	    ASTVariable *var = (ASTVariable*)node;
	    String name = var->name;
	    printf("name: %.*s", name.len, name.mem);
	}break;
	case ASTType::UNI_DECLERATION: {
	    ASTUniVar *decl = (ASTUniVar*)node;
	    u32 x = decl->tokenOff+2;
	    printf("uni_decleration");
	    PAD;
	    printf("type: %.*s", lexer.tokenOffsets[x].len, lexer.fileContent + lexer.tokenOffsets[x].off);
	    PAD;
	    printf("name: %.*s", decl->name.len, decl->name.mem);
	} break;
	case ASTType::UNI_NEG:{
	    printf("uni_neg");
	    PAD;
	    printf("NODE");
	    ASTUniOp *uniOp = (ASTUniOp*)node;
	    __dumpNodesWithoutEndPadding(uniOp->node, lexer, padding + 1);
	}break;
	case ASTType::MULTI_DECLERATION:{
	    ASTMultiVar *decl = (ASTMultiVar*)node;
	    u32 x = decl->tokenOff+2;
	    printf("multi_decleration");
	    PAD;
	    printf("type: %.*s", lexer.tokenOffsets[x].len, lexer.fileContent + lexer.tokenOffsets[x].off);
	    PAD;
	    printf("names:");
	    ASTMultiVar *multiAss = (ASTMultiVar*)node;
	    DynamicArray<String> &names = multiAss->names;
	    for (u8 x=0; x<names.count; x +=1) {
		String name = names[x];
		PAD;
		printf("      %.*s", name.len, name.mem);
	    };
	}break;
	case ASTType::UNI_ASSIGNMENT_T_KNOWN: {
	    ASTUniVar *decl = (ASTUniVar*)node;
	    u32 x = decl->tokenOff + 2;
	    printf("uni_assignment_t_known");
	    PAD;
	    printf("type: %.*s", lexer.tokenOffsets[x].len, lexer.fileContent + lexer.tokenOffsets[x].off);
	};
	    flag = true;
	case ASTType::UNI_ASSIGNMENT_T_UNKNOWN: {
	    if (flag) { flag = false; }
	    else { printf("uni_assignment_t_unkown"); };	
	    PAD;
	    ASTUniVar *decl = (ASTUniVar*)node;
	    printf("name: %.*s", decl->name.len, decl->name.mem);
	    PAD;
	    printf("RHS");
	    if (decl->rhs != nullptr) {__dumpNodesWithoutEndPadding(decl->rhs, lexer, padding + 1);};
	} break;
	case ASTType::MULTI_ASSIGNMENT_T_KNOWN: {
	    ASTMultiVar *decl = (ASTMultiVar*)node;
	    u32 x = decl->tokenOff + 2;
	    printf("multi_assignment_t_known");
	    PAD;
	    printf("type: %.*s", lexer.tokenOffsets[x].len, lexer.fileContent + lexer.tokenOffsets[x].off);
	};
	    flag = true;
	case ASTType::MULTI_ASSIGNMENT_T_UNKNOWN: {
	    if (flag) { flag = false; }
	    else { printf("multi_assignment_t_unkown"); };
	    PAD;
	    printf("names:");
	    ASTMultiVar *multiAss = (ASTMultiVar*)node;
	    DynamicArray<String> &names = multiAss->names;
	    for (u8 x=0; x<names.count; x +=1) {
		String name = names[x];
		PAD;
		printf("      %.*s", name.len, name.mem);
	    };
	    PAD;
	    printf("RHS");
	    if (multiAss->rhs != nullptr) {__dumpNodesWithoutEndPadding(multiAss->rhs, lexer, padding + 1);};
	} break;
	case ASTType::PROC_DEFENITION: {
	    ASTProcDef *proc = (ASTProcDef*)node;
	    printf("proc_defenition");
	    PAD;
	    printf("name: %.*s", proc->name.len, proc->name.mem);
	    PAD;
	    printf("[IN]");
	    for(u32 x=0; x<proc->inCommaCount; x+=1){
		ASTBase *node = proc->body[x];
		PAD;
		__dumpNodesWithoutEndPadding(node, lexer, padding + 1);
	    };
	    PAD;
	    printf("[OUT]");
	    for(u32 x=0; x<proc->out.count; x+=1){
		ASTBase *node = proc->out[x];
		PAD;
		__dumpNodesWithoutEndPadding(node, lexer, padding + 1);
	    };
	    PAD;
	    DynamicArray<ASTBase*> &table = proc->body;
	    printf("[BODY]");
	    for (u32 v=proc->inCommaCount; v < table.count; v += 1) {
		PAD;
		__dumpNodesWithoutEndPadding(table[v], lexer, padding + 1);
	    };
	    PAD;
	} break;
	case ASTType::TYPE:{
	    AST_Type *type = (AST_Type*)node;
	    printf("%d", type->type);
	    PAD;
	}break;
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
