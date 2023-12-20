#include <math.h>

#define BRING_TOKENS_TO_SCOPE DynamicArray<Token_Type> &tokTypes = lexer.tokenTypes;DynamicArray<TokenOffset> &tokOffs = lexer.tokenOffsets;

enum class ASTType {
    NUM_INTEGER,
    NUM_DECIMAL,
    STRING,
    BIN_ADD,
    BIN_MUL,
    BIN_DIV,
    BIN_GRT,
    BIN_GRTE,
    BIN_LSR,
    BIN_LSRE,
    BIN_EQU,
    UNI_DECLERATION,
    UNI_INITIALIZATION_T_UNKNOWN,
    UNI_INITIALIZATION_T_KNOWN,
    MULTI_DECLERATION,
    MULTI_INITIALIZATION_T_UNKNOWN,
    MULTI_INITIALIZATION_T_KNOWN,
    PROC_DEFENITION,
    VARIABLE,
    TYPE,
    UNI_NEG,
    IF,
    FOR,
    IMPORT,
    STRUCT_DEFENITION,
    MODIFIER,
    GLOBAL_VAR_INIT,
    RETURN
};
enum class ForType{
    FOR_EVER,
    C_LES,
    C_EQU,
    EXPR,
};

struct ScopeEntities;
struct ASTBase {
    ASTType type;
    Flag flag;
};
struct ASTGblVarInit : ASTBase{
    s16 reg;
    ASTBase *rhs;
    Type type;
};
struct ASTImport : ASTBase{
    char *fileName;
};
struct ASTNumInt : ASTBase{
    s64 num;
};
struct ASTNumDec : ASTBase{
    f64 num;
};
struct ASTString : ASTBase{
    u32 tokenOff;
    String str;
};
struct ASTUniVar : ASTBase{
    String name;
    ASTBase *rhs;
    u32 tokenOff;
    u32 typeTokenOff;
};
struct ASTBinOp : ASTBase{
    ASTBase *lhs;
    ASTBase *rhs;
    u32 tokenOff;
    Type lhsType;
    Type rhsType;
};
struct ASTMultiVar : ASTBase{
    DynamicArray<String> names;
    ASTBase *rhs;
    u64 size;
    u32 tokenOff;
};
struct ASTUniOp : ASTBase{
    ASTBase *node;
};
struct ASTIf : ASTBase{
    DynamicArray<ASTBase*> body;
    DynamicArray<ASTBase*> elseBody;
    ASTBase *expr;
    ScopeEntities *IfSe;                  //scope of the body
    ScopeEntities *ElseSe;                //scope of else body
    u32 tokenOff;
};
struct ASTProcDef : ASTBase {
    DynamicArray<ASTBase*> body;    //NOTE: also includes input
    DynamicArray<ASTBase*> out;
    String name;
    ScopeEntities *se;
    u32 tokenOff;
    u32 uniInCount;        //number of single input
    u32 multiInCount;      //number of multiple input
    u32 multiInInputCount; //total number of variables in multiple input
};
struct ASTVariable : ASTBase{
    String name;
    u32 tokenOff;
};
struct ASTModifier : ASTBase{
    String name;
    ASTBase *child;
    u32 tokenOff;
};
struct AST_Type : ASTBase{
    union{
	u32 tokenOff;
	Type type;
    };
};
struct ASTFor : ASTBase{
    DynamicArray<ASTBase*> body;
    union{
	ASTBase *end;
	ASTBase *expr;
    };
    ASTBase *increment;
    ScopeEntities    *ForSe;
    u32      endOff;
    u32      incrementOff;
    ForType  loopType;
};
struct ASTStructDef : ASTBase{
    DynamicArray<ASTBase*> body;
    String name;
    u32 tokenOff;
    u32 memberCount;
};
struct ASTReturn : ASTBase{
    ASTBase *expr;
};

void freeNodeInternal(ASTBase *base);
void freeBody(DynamicArray<ASTBase*> &body){
    if(body.len != 0){
	for(u32 x=0; x<body.count; x+=1){
	    freeNodeInternal(body[x]);
	};
	body.uninit();
    };
};
void freeNodeInternal(ASTBase *base){
    switch(base->type){
    case ASTType::MULTI_DECLERATION:
    case ASTType::MULTI_INITIALIZATION_T_UNKNOWN:
    case ASTType::MULTI_INITIALIZATION_T_KNOWN:{
	ASTMultiVar *mv = (ASTMultiVar*)base;
	mv->names.uninit();
    }break;
    case ASTType::PROC_DEFENITION:{
	ASTProcDef *proc = (ASTProcDef*)base;
        freeBody(proc->body);
	if(proc->out.len != 0){proc->out.uninit();};
    }break;
    case ASTType::STRUCT_DEFENITION:{
	ASTStructDef *Struct = (ASTStructDef*)base;
	freeBody(Struct->body);
    }break;
    case ASTType::IF:{
	ASTIf *If = (ASTIf*)base;
	freeBody(If->body);
	freeBody(If->elseBody);
    }break;
    case ASTType::FOR:{
	ASTFor *For = (ASTFor*)base;
	freeBody(For->body);
    }break;
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
    u32 start = x;
    switch (tokTypes[x]) {
    case (Token_Type)'-': x -= 1;//NOTE: sub is a uni operator
    case (Token_Type)'+': type = ASTType::BIN_ADD; x += 1; break;
    case (Token_Type)'*': type = ASTType::BIN_MUL; x += 1; break;
    case (Token_Type)'/': type = ASTType::BIN_DIV; x += 1; break;
    case (Token_Type)'=':{
	type = ASTType::BIN_EQU;
	x += 1;
	if(tokTypes[x] == (Token_Type)'='){
	    x += 1;
	}else{
	    lexer.emitErr(tokOffs[x].off, "Expected '='");
	};
    }break;
    case (Token_Type)'>':{
	type = ASTType::BIN_GRT;
	x += 1;
	if(tokTypes[x] == (Token_Type)'='){
	    type = ASTType::BIN_GRTE;
	    x += 1;
	};
    }break;
    case (Token_Type)'<':{
	type = ASTType::BIN_LSR;
	x += 1;
	if(tokTypes[x] == (Token_Type)'='){
	    type = ASTType::BIN_LSRE;
	    x += 1;
	};
    }break;
    default: UNREACHABLE;
    };
    ASTBinOp *binOp = (ASTBinOp*)allocAST(sizeof(ASTBinOp), type, file);
    binOp->tokenOff = start;
    return binOp;
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
ASTBase *genVariable(u32 &x, Lexer &lexer, ASTFile &file){
    BRING_TOKENS_TO_SCOPE;
    ASTBase *root = nullptr;
    bool childReq = false;
    ASTModifier *lastMod = nullptr;
    while(tokTypes[x] == Token_Type::IDENTIFIER){
	if(tokTypes[x+1] == (Token_Type)'.'){
	    ASTModifier *mod = (ASTModifier*)allocAST(sizeof(ASTModifier), ASTType::MODIFIER, file);
	    mod->name = makeStringFromTokOff(x, lexer);
	    mod->tokenOff = x;
	    childReq = true;
	    if(root == nullptr){root = mod;};
	    lastMod = mod;
	    x += 2;
	}else{
	    ASTVariable *var = (ASTVariable*)allocAST(sizeof(ASTVariable), ASTType::VARIABLE, file);
	    var->name = makeStringFromTokOff(x, lexer);
	    var->tokenOff = x;
	    childReq = false;
	    if(root == nullptr){root = var;};
	    if(lastMod != nullptr){lastMod->child = var;};
	    x += 1;
	};
    };
    if(childReq){
	lexer.emitErr(tokOffs[x].off, "Identifier required");
	return nullptr;
    };
    return root;
};
ASTBase *genASTOperand(Lexer &lexer, u32 &x, ASTFile &file, s16 &bracket) {
    BRING_TOKENS_TO_SCOPE;
 CHECK_TYPE_AST_OPERAND:
    Token_Type type = tokTypes[x];
    switch (type) {
    case Token_Type::DOUBLE_QUOTES:{
	ASTString *string = (ASTString*)allocAST(sizeof(ASTString), ASTType::STRING, file);
	string->tokenOff = x;
	string->str = makeStringFromTokOff(x, lexer);
	x += 1;
	return (ASTBase*)string;
    }break;
    case (Token_Type)'-':{
	ASTUniOp *uniOp = (ASTUniOp*)allocAST(sizeof(ASTUniOp), ASTType::UNI_NEG, file);
	x += 1;
	ASTBase *node = genASTOperand(lexer, x, file, bracket);
	uniOp->node = node;
	return (ASTBase*)uniOp;
    }break;
    case Token_Type::INTEGER: {
	ASTNumInt *numNode = (ASTNumInt*)allocAST(sizeof(ASTNumInt), ASTType::NUM_INTEGER, file);
	String str = makeStringFromTokOff(x, lexer);
	numNode->num = string2int(str);
	x += 1;
	return (ASTBase*)numNode;
    } break;
    case Token_Type::DECIMAL:{
	ASTNumDec *numNode = (ASTNumDec*)allocAST(sizeof(ASTNumDec), ASTType::NUM_DECIMAL, file);
	String str = makeStringFromTokOff(x, lexer);
	numNode->num = string2float(str);	
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
    case (Token_Type)'=':
    case (Token_Type)'<':
    case (Token_Type)'>':
	//NOTE: this takes care of <=, >=
	return 1;
    case (Token_Type)'+':
    case (Token_Type)'-':
	return 2;
    case (Token_Type)'*':
    case (Token_Type)'/':
	return 3;
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
    if(node == nullptr){return nullptr;};
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
	if(binOperator == nullptr){return nullptr;};
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

ASTBase* parseType(u32 x, Lexer &lexer, ASTFile &file){
    BRING_TOKENS_TO_SCOPE;
    AST_Type *type = (AST_Type*)allocAST(sizeof(AST_Type), ASTType::TYPE, file);
    type->tokenOff = x;
    return (ASTBase*)type;
};
s32 getTokenOffInLine(Token_Type tok, Lexer &lexer, u32 cur){
    BRING_TOKENS_TO_SCOPE;
    s32 x = cur;
    while(tokTypes[x] != tok){
	switch(tokTypes[x]){
	case Token_Type::END_OF_FILE:
	case (Token_Type)'\n':
	    return -1;
	};
	x += 1;
    };
    return x;
};
bool parseBlock(Lexer &lexer, ASTFile &file, DynamicArray<ASTBase*> &table, u32 &x) {
    BRING_TOKENS_TO_SCOPE;
    Flag flag = 0;
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
    case Token_Type::K_RETURN:{
	ASTReturn *ret = (ASTReturn*)allocAST(sizeof(ASTReturn), ASTType::RETURN, file);
	x += 1;
	if(tokTypes[x] == (Token_Type)'}' || tokTypes[x] == (Token_Type)'\n'){
	    ret->expr = nullptr;
	}else{
	    s32 end;
	    s32 bend = getTokenOffInLine((Token_Type)'{', lexer,  x);
	    s32 nend = getTokenOffInLine((Token_Type)'\n', lexer, x);
	    if(bend != -1){
		end = bend;
	    }else{
		end = nend;
	    };
	    ret->expr = genASTExprTree(lexer, file, x, end);
	};
	x += 1;
	table.push(ret);
	return true;
    }break;
    case Token_Type::P_IMPORT:{
	x += 1;
	if(tokTypes[x] != Token_Type::DOUBLE_QUOTES){
	    lexer.emitErr(tokOffs[x].off, "Expected file path as a string");
	    return false;
	};
	String fileName = makeStringFromTokOff(x, lexer);
	fileName.mem += 1;
	char c = fileName.mem[fileName.len];
	fileName.mem[fileName.len] = '\0';
	char *fullFileName = os::getFileFullName(fileName.mem);
	fileName.mem[fileName.len] = c;
	if(os::isFile(fullFileName) == false){
	    lexer.emitErr(tokOffs[x].off, "Invalid file path");
	    return false;
	};
	x += 1;
	ASTImport *Import = (ASTImport*)allocAST(sizeof(ASTImport), ASTType::IMPORT, file);
	Import->fileName = fullFileName;
	table.push(Import);
	return true;
    }break;
    case Token_Type::K_FOR:{
	ASTFor *For = (ASTFor*)allocAST(sizeof(ASTFor), ASTType::FOR, file);
	For->body.init();
	x += 1;
	switch(tokTypes[x]){
	case (Token_Type)'{':{
	    //for ever
	    For->loopType = ForType::FOR_EVER;
	    x += 1;
	    eatNewlines(lexer.tokenTypes, x);
	    while(tokTypes[x] != (Token_Type)'}'){
		bool result = parseBlock(lexer, file, For->body, x);
		if(result == false){return false;};
		eatNewlines(lexer.tokenTypes, x);
	    };
	    x += 1;
	    table.push(For);
	    return true;
	}break;
	case Token_Type::IDENTIFIER:{
	    if(tokTypes[x+1] != (Token_Type)':'){
		goto PARSE_FOR_EXPR;
	    };
	    ASTUniVar *var = (ASTUniVar*)allocAST(sizeof(ASTUniVar), ASTType::UNI_INITIALIZATION_T_UNKNOWN, file);
	    var->typeTokenOff = 0;
	    var->tokenOff = x;
	    var->name = makeStringFromTokOff(x, lexer);
	    var->flag = 0;
	    SET_BIT(var->flag, Flags::ALLOC);
	    For->body.push(var);
	    For->increment = nullptr;
	    x += 2;
	    if(tokTypes[x] == Token_Type::IDENTIFIER || isKeyword(tokTypes[x])){
		var->typeTokenOff = x;
		var->type = ASTType::UNI_INITIALIZATION_T_KNOWN;
		x += 1;
	    };
	    s32 end = getTokenOffInLine(Token_Type::TDOT, lexer, x);
	    if(end == -1){
		lexer.emitErr(tokOffs[x].off, "Expected '...' to specify the upper bound");
		return false;
	    };
	    var->rhs = genASTExprTree(lexer, file, x, end);
	    if(var->rhs == nullptr){return false;};
	    x = end + 1;
	    s32 dend = getTokenOffInLine(Token_Type::DDOT, lexer, x);
	    s32 bend = getTokenOffInLine((Token_Type)'{', lexer,  x);
	    s32 nend = getTokenOffInLine((Token_Type)'\n', lexer, x);
	    if(dend != -1){
		end = dend;
	    }else if(bend != -1){
		end = bend;
	    }else{
		end = nend;
	    };
	    switch(tokTypes[x]){
	    case (Token_Type)'<':{
		For->loopType = ForType::C_LES;
		x += 1;
	    }break;
	    default:{
		For->loopType = ForType::C_EQU;
	    }break;
	    };
	    For->end = genASTExprTree(lexer, file, x, end);
	    if(For->end == false){return false;};
	    For->endOff = end;
	    x = end;
	    if(dend != -1){
		x += 1;
		if(bend != -1){
		    end = bend;
		}else{
		    end = nend;
		};
		For->increment = genASTExprTree(lexer, file, x, end);
		if(For->increment == nullptr){return false;};
		For->incrementOff = end;
		x = end;
	    };
	    if(bend == -1){
		eatNewlines(lexer.tokenTypes, x);
	    };
	    if(tokTypes[x] != (Token_Type)'{'){
		lexer.emitErr(tokOffs[x].off, "Expected '{' here");
		return false;
	    };
	    x += 1;
	    eatNewlines(lexer.tokenTypes, x);
	    while(tokTypes[x] != (Token_Type)'}'){
		bool result = parseBlock(lexer, file, For->body, x);
		if(result == false){return false;};
		eatNewlines(lexer.tokenTypes, x);
	    };
	    x += 1;
	    table.push(For);
	    return true;
	}break;
	default:{
	    x += 1;
	    PARSE_FOR_EXPR:
	    For->loopType = ForType::EXPR;
	    s32 bend = getTokenOffInLine((Token_Type)'{', lexer,  x);
	    s32 nend = getTokenOffInLine((Token_Type)'\n', lexer, x);
	    if(bend == -1 && nend == -1){
		lexer.emitErr(tokOffs[x-1].off, "Line has to be finished with '{' or a newline");
		return false;
	    };
	    u32 end = (bend != -1)? bend : nend;
	    For->expr = genASTExprTree(lexer, file, x, end);
	    if(For->expr == nullptr){return false;};
	    x = end;
	    if(bend == -1){
		eatNewlines(lexer.tokenTypes, x);
	    };
	    if(tokTypes[x] != (Token_Type)'{'){
		lexer.emitErr(tokOffs[x].off, "Expected '{' here");
		return nullptr;
	    };
	    x += 1;
	    eatNewlines(lexer.tokenTypes, x);
	    For->body.init();
	    while(tokTypes[x] != (Token_Type)'}'){
		bool result = parseBlock(lexer, file, For->body, x);
		if(result == false){return false;};
		eatNewlines(lexer.tokenTypes, x);
	    };
	    x += 1;
	    table.push(For);
	    return true;
	}break;
	};
    }break;
    case Token_Type::K_IF:{
	ASTIf *If = (ASTIf*)allocAST(sizeof(ASTIf), ASTType::IF, file);
	If->tokenOff = x;
	x += 1;
	If->body.zero();
	If->elseBody.zero();
	s32 end;
	s32 bend = getTokenOffInLine((Token_Type)'{', lexer, x);
	s32 nend = getTokenOffInLine((Token_Type)'\n', lexer, x);
	if(bend == -1 && nend == -1){
	    lexer.emitErr(tokOffs[x-1].off, "Line has to be finished with '{' or a newline");
	    return nullptr;
	};
	if(bend == -1){
	    end = nend;
	}else{
	    end = bend;
	};
	If->expr = genASTExprTree(lexer, file, x, end);
	if(If->expr == nullptr){return false;};
	x = end;
	if(bend == -1){
	    x += 1;
	    eatNewlines(lexer.tokenTypes, x);
	};
	if(tokTypes[x] != (Token_Type)'{'){
	    lexer.emitErr(tokOffs[x].off, "Expected '{' here");
	    return false;
	};
	If->body.init();
	x += 1;
	eatNewlines(lexer.tokenTypes, x);
	while(tokTypes[x] != (Token_Type)'}'){
	    bool result = parseBlock(lexer, file, If->body, x);
	    if(result == false){return false;};
	    eatNewlines(lexer.tokenTypes, x);
	};
	x += 1;
	eatNewlines(lexer.tokenTypes, x);
	if(tokTypes[x] != Token_Type::K_ELSE){return (ASTBase*)If;};
	If->elseBody.init();
	x += 1;
	eatNewlines(lexer.tokenTypes, x);
	switch(tokTypes[x]){
	case (Token_Type)'{':{
	    x += 1;
	    eatNewlines(lexer.tokenTypes, x);
	    while(tokTypes[x] != (Token_Type)'}'){
		bool result = parseBlock(lexer, file, If->elseBody, x);
		if(result == false){return false;};
		eatNewlines(lexer.tokenTypes, x);
	    };
	    x += 1;
	    if(tokTypes[x] == Token_Type::K_ELSE){
		lexer.emitErr(tokOffs[x].off, "Unexpected 'else'");
		return false;
	    };
	}break;
	case Token_Type::K_IF:{
	    bool result = parseBlock(lexer, file, If->elseBody, x);
	    if(result == false){return false;};
	}break;
	};
	table.push(If);
	return false;
    }break;
    case Token_Type::IDENTIFIER: {
	x += 1;
	switch (tokTypes[x]) {
	case (Token_Type)':': {
	    x += 1;
	    ASTType uniVarType = ASTType::UNI_INITIALIZATION_T_UNKNOWN; 
	    switch (tokTypes[x]) {
	    case (Token_Type)'=': {
		//single variable assignment
		SINGLE_VARIABLE_ASSIGNMENT:
		ASTUniVar *assign = (ASTUniVar*)allocAST(sizeof(ASTUniVar), uniVarType, file);
		assign->tokenOff = start;
		assign->name = makeStringFromTokOff(start, lexer);
		x += 1;
		u32 end = getEndNewlineEOF(tokTypes, x);
		if(tokTypes[x] == Token_Type::TDOT){
		    SET_BIT(flag, Flags::UNINITIALIZED);
		    x += 1;
		}else{
		    assign->rhs = genASTExprTree(lexer, file, x, end);
		    if(assign->rhs == nullptr){return false;};
		};
		assign->flag = flag;
		table.push(assign);
		return true;
	    } break;
	    case (Token_Type)':': {
		x += 1;
		switch (tokTypes[x]){
		case Token_Type::K_STRUCT:{
		    x += 1;
		    eatNewlines(tokTypes, x);
		    if(tokTypes[x] != (Token_Type)'{'){
			lexer.emitErr(tokOffs[x].off, "Expected '{'");
			return false;
		    };
		    x += 1;
		    eatNewlines(tokTypes, x);
		    ASTStructDef *Struct = (ASTStructDef*)allocAST(sizeof(ASTStructDef), ASTType::STRUCT_DEFENITION, file);
		    Struct->name = makeStringFromTokOff(start, lexer);
		    Struct->body.init();
		    Struct->tokenOff = start;
		    u32 count = 0;
		    while (tokTypes[x] != (Token_Type)'}') {
			//parse body
			u32 start = x;
			bool result = parseBlock(lexer, file, Struct->body, x);
			if(result == false) {return false;};
			ASTBase *node = Struct->body[Struct->body.count - 1];
			switch(node->type){
			case ASTType::UNI_DECLERATION: count += 1;break;
			case ASTType::MULTI_DECLERATION:{
			    ASTMultiVar *multiVar = (ASTMultiVar*)node;
			    count += multiVar->names.count;
			}break;
			default:
			    lexer.emitErr(tokOffs[x].off, "Invalid statement. Struct only takes uni/multi decleration");
			    return nullptr;
			};
			eatNewlines(tokTypes, x);
		    };
		    Struct->memberCount = count;
		    x += 1;
		    table.push(Struct);
		    return true;
		}break;
		case Token_Type::K_PROC:{
		    //procedure defenition
		    x += 1;
		    if (tokTypes[x] != (Token_Type)'(') {
			lexer.emitErr(tokOffs[x].off, "Expected '('");
			return false;
		    };
		    ASTProcDef *proc = (ASTProcDef*)allocAST(sizeof(ASTProcDef), ASTType::PROC_DEFENITION, file);
		    proc->tokenOff = start;
		    proc->flag = flag;
		    proc->name = makeStringFromTokOff(start, lexer);
		    proc->body.init();
		    proc->out.zero();
		    u32 uniInCount = 0;
		    u32 multiInCount = 0;
		    u32 multiInInputCount = 0;
		    //parse input
		    x += 1;
		    if(tokTypes[x] == (Token_Type)')'){goto END_PROC_INPUT_PARSING_LOOP;};
		    while(true){
			eatNewlines(tokTypes, x);
			bool result = parseBlock(lexer, file, proc->body, x);
			if(result == false){return false;};
			ASTBase *base = proc->body[proc->body.count - 1];
			switch(base->type){
			case ASTType::MULTI_DECLERATION:{
			    ASTMultiVar *multiVar = (ASTMultiVar*)base;
			    multiInCount += 1;
			    multiInInputCount = multiVar->names.count;
			}break;
			case ASTType::UNI_DECLERATION:{
			    uniInCount += 1;
			}break;
			};
			switch(tokTypes[x]){
			case (Token_Type)')':{
			    goto END_PROC_INPUT_PARSING_LOOP;
			}break;
			case (Token_Type)',':{
			    x += 1;
			}break;
			default:
			    lexer.emitErr(tokOffs[x].off, "Expected ','");
			    return false;
			};
		    };
		    END_PROC_INPUT_PARSING_LOOP:
		    proc->uniInCount = uniInCount;
		    proc->multiInCount = multiInCount;
		    proc->multiInInputCount = multiInInputCount;
		    if(tokTypes[x] != (Token_Type)')'){
			lexer.emitErr(tokOffs[x].off, "Expected ')'");
			return false;
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
			    if(typeNode == nullptr){return false;};
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
				return false;
			    };
			    x += 1;
			    eatNewlines(tokTypes, x);
			} else if(bracket == true) {
			    lexer.emitErr(tokOffs[x].off, "Expected ')'");
			    return false;
			};
			if (tokTypes[x] != (Token_Type)'{') {
			    lexer.emitErr(tokOffs[x].off, "Expected '{'");
			    return false;
			};
			x += 1;
		    } break;
		    case (Token_Type)'{': {
			x += 1;
		    } break;
		    default: {
			lexer.emitErr(tokOffs[x].off, "Expected '{' or '->'");
			return false;
		    } break;
		    };
		    eatNewlines(tokTypes, x);
		    if (tokTypes[x] == (Token_Type)'}') {
			x += 1;
			table.push(proc);
			return true;
		    };
		    while (tokTypes[x] != (Token_Type)'}') {
			//parse body
			bool result = parseBlock(lexer, file, proc->body, x);
			if (result == false) {
			    return false;
			};
			eatNewlines(tokTypes, x);
		    };
		    x += 1;
		    table.push(proc);
		    return true;
		}break;
		};
		
	    } break;
	    default: {
		if (isType(tokTypes[x])) {
		    x += 1;
		    if (tokTypes[x] == (Token_Type)'=') {
			uniVarType = ASTType::UNI_INITIALIZATION_T_KNOWN;
			goto SINGLE_VARIABLE_ASSIGNMENT;
		    };
		    ASTUniVar *assign = (ASTUniVar*)allocAST(sizeof(ASTUniVar), ASTType::UNI_DECLERATION, file);
		    assign->tokenOff = start;
		    assign->name = makeStringFromTokOff(start, lexer);
		    assign->flag = flag;
		    table.push(assign);
		    return true;
		}else{
		    lexer.emitErr(tokOffs[x].off, "Expected a type");
		    return false;
		};
	    } break;
	    } break;
	} break;
	case (Token_Type)',': {
	    //multi var def/decl
	    u32 i = x;
	    x = start;
	    ASTType type;
	    u32 equalSignPos = 0;
	    while(true){
		switch(tokTypes[i]){
		case (Token_Type)':':{
		    i += 1;
		    if(tokTypes[i] == Token_Type::IDENTIFIER){
			if(tokTypes[i+1] == (Token_Type)'='){
			    type = ASTType::UNI_INITIALIZATION_T_KNOWN;
			    equalSignPos = i + 1;
			}else{
			    type = ASTType::UNI_DECLERATION;
			};
		    }else if(tokTypes[i] == (Token_Type)'='){
			type = ASTType::UNI_INITIALIZATION_T_UNKNOWN;
			equalSignPos = i + 1;
		    }else{
			lexer.emitErr(tokOffs[i].off, "Expected a type or '='");
			return false;
		    };
		    goto EXIT_LOOP_MULTI_VAR;
		}break;
		case Token_Type::END_OF_FILE:
		case (Token_Type)'\n':{
		    lexer.emitErr(tokOffs[i].off, "Expected ':'");
		    return false;
		}break;
		};
		i += 1;
	    };
	    EXIT_LOOP_MULTI_VAR:
	    Flag flag = 0;
	    ASTBase *rhs = nullptr;
	    u32 end = getEndNewlineEOF(tokTypes, equalSignPos);
	    if(equalSignPos != 0){
		if(tokTypes[equalSignPos] == Token_Type::TDOT){
		    SET_BIT(flag, Flags::UNINITIALIZED);
		}else{
		    rhs = genASTExprTree(lexer, file, equalSignPos, end);
		};
	    };
	    while(true){
		if(tokTypes[x] == Token_Type::IDENTIFIER){
		    ASTUniVar *uv = (ASTUniVar*)allocAST(sizeof(ASTUniVar), type, file);
		    uv->flag = flag;
		    uv->rhs = rhs;
		    uv->name = makeStringFromTokOff(x, lexer);
		    table.push(uv);
		}else{
		    lexer.emitErr(tokOffs[x].off, "Expected an identifier");
		    return false;
		};
		x += 1;
		if(tokTypes[x] == (Token_Type)':'){
		    break;
		};
		if(tokTypes[x] != (Token_Type)','){
		    lexer.emitErr(tokOffs[x].off, "Expected ','");
		    return false;
		};
		x += 1;
	    };
	    x = end;
	    return true;
	} break;
	} break;
    } break;
    default: {
	u32 end = getEndNewlineEOF(tokTypes, x);
	ASTBase *tree = genASTExprTree(lexer, file, x, end);
	if(tree == nullptr){return false;};
	table.push(tree);
    } break;
    };
    return false;
};

u32 pow(u32 base, u32 exp){
    u32 num = 1;
    while(exp != 0){
	num *= base;
	exp -= 1;
    };
    return num;
};
#if(DBG)

#define PAD printf("\n"); for (u8 i = 0; i < padding; i++) { printf("    "); };

namespace dbg {
    void __dumpNodesWithoutEndPadding(ASTBase *node, Lexer &lexer, u8 padding);
    void __dumpDynamicArrayNodes(DynamicArray<ASTBase*> body, Lexer &lexer, u8 padding){
	for(u32 x=0; x<body.count; x+=1){
	    ASTBase *node = body[x];
	    __dumpNodesWithoutEndPadding(node, lexer, padding + 1);
	};
    };
    void __dumpNodesWithoutEndPadding(ASTBase *node, Lexer &lexer, u8 padding) {
	BRING_TOKENS_TO_SCOPE;
	PAD;
	printf("[NODE]");
	PAD;
	printf("type: ");
	ASTType type = node->type;
	char c = NULL;
	u8 flag = false;
	u8 printEqual = false;
	switch (type) {
	case ASTType::MODIFIER:{
	    ASTModifier *mod = (ASTModifier*)node;
	    printf("modifier");
	    PAD;
	    printf("name: %.*s", mod->name.len, mod->name.mem);
	    __dumpNodesWithoutEndPadding(mod->child, lexer, padding+1);
	    PAD;
	}break;
	case ASTType::RETURN:{
	    printf("return");
	    PAD;
	    ASTReturn *ret = (ASTReturn*)node;
	    if(ret->expr == nullptr){printf("void");}
	    else{__dumpNodesWithoutEndPadding(ret->expr, lexer, padding + 1);};
	}break;
	case ASTType::IF:{
	    printf("if");
	    PAD;
	    printf("EXPR");
	    ASTIf *If = (ASTIf*)node;
	    if(If->expr){__dumpNodesWithoutEndPadding(If->expr, lexer, padding + 1);};
	    PAD;
	    printf("BODY");
	    __dumpDynamicArrayNodes(If->body, lexer, padding);
	    PAD;
	    printf("ELSE BODY");
	    PAD;
	    for(u32 x=0; x<If->elseBody.count; x+=1){
		ASTBase *node = If->elseBody[x];
		PAD;
		__dumpNodesWithoutEndPadding(node, lexer, padding + 1);
	    };
	}break;
	case ASTType::NUM_INTEGER: {
	    ASTNumInt *numInt = (ASTNumInt*)node;
	    printf("num_integer");
	    PAD;
	    printf("num: %lld", numInt->num);
	} break;
	case ASTType::NUM_DECIMAL: {
	    printf("num_decimal");
	    ASTNumDec *numDec = (ASTNumDec*)node;
	    PAD;
	    printf("num: %f", numDec->num);
	} break;
	case ASTType::BIN_GRT: c = '>'; printf("bin_grt");
	case ASTType::BIN_GRTE: if(c == NULL) { c = '>'; printf("bin_grte"); printEqual=true;};
	case ASTType::BIN_LSR:  if(c == NULL) { c = '<'; printf("bin_lsr");};
	case ASTType::BIN_LSRE: if(c == NULL) { c = '<'; printf("bin_lsre"); printEqual=true;};
	case ASTType::BIN_EQU:  if(c == NULL) { c = '='; printf("bin_equ");  printEqual=true;};
	case ASTType::BIN_ADD:  if(c == NULL) { c = '+'; printf("bin_add"); };
	case ASTType::BIN_MUL:  if(c == NULL) { c = '*'; printf("bin_mul"); };
	case ASTType::BIN_DIV:{
	    ASTBinOp *bin = (ASTBinOp*)node;
	    if (c == NULL) { c = '/'; printf("bin_div"); };
	    PAD;
	    printf("lhs: %p", bin->lhs);
	    PAD;
	    printf("rhs: %p", bin->rhs);
	    PAD;
	    printf("op: %c", c);
	    if(printEqual){printf("=");};
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
	case ASTType::UNI_INITIALIZATION_T_KNOWN: {
	    ASTUniVar *decl = (ASTUniVar*)node;
	    u32 x = decl->typeTokenOff;
	    printf("UNI_INITIALIZATION_T_KNOWN");
	    PAD;
	    printf("type: %.*s", lexer.tokenOffsets[x].len, lexer.fileContent + lexer.tokenOffsets[x].off);
	};
	    flag = true;
	case ASTType::UNI_INITIALIZATION_T_UNKNOWN: {
	    if (flag) { flag = false; }
	    else { printf("uni_initialization_t_unkown"); };	
	    PAD;
	    ASTUniVar *decl = (ASTUniVar*)node;
	    printf("name: %.*s", decl->name.len, decl->name.mem);
	    PAD;
	    printf("RHS");
	    if(IS_BIT(decl->flag, Flags::UNINITIALIZED)){
		padding += 1;
		PAD;
		printf("uninitialized");
	    }else{
		if (decl->rhs != nullptr) {__dumpNodesWithoutEndPadding(decl->rhs, lexer, padding + 1);};
	    };
	} break;
	case ASTType::MULTI_INITIALIZATION_T_KNOWN: {
	    ASTMultiVar *decl = (ASTMultiVar*)node;
	    u32 x = decl->tokenOff + 2;
	    printf("MULTI_INITIALIZATION_T_KNOWN");
	    PAD;
	    printf("type: %.*s", lexer.tokenOffsets[x].len, lexer.fileContent + lexer.tokenOffsets[x].off);
	};
	    flag = true;
	case ASTType::MULTI_INITIALIZATION_T_UNKNOWN: {
	    if (flag) { flag = false; }
	    else { printf("multi_initialization_t_unkown"); };
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
	    if(IS_BIT(multiAss->flag, Flags::UNINITIALIZED)){
		padding += 1;
		PAD;
		printf("uninitialized");
	    }else{
		if (multiAss->rhs != nullptr) {__dumpNodesWithoutEndPadding(multiAss->rhs, lexer, padding + 1);};
	    };
	} break;
	case ASTType::FOR:{
	    bool printed = false;
	    ASTFor *For = (ASTFor*)node;
	    printf("for");
	    PAD;
	    printf("type: ");
	    switch(For->loopType){
	    case ForType::FOR_EVER:{
		printf("for_ever");
	    }break;
	    case ForType::EXPR:{
		printf("expr");
		PAD;
		printf("EXPR");
		__dumpNodesWithoutEndPadding(For->expr, lexer, padding+1);
	    }break;
	    case ForType::C_LES:
		printf("c_les");
		printed = true;
	    case ForType::C_EQU:{
		if(printed == false){printf("c_equ");};
		PAD;
		printf("it");
		__dumpNodesWithoutEndPadding(For->body[0], lexer, padding+1);
		PAD;
		printf("END");
		__dumpNodesWithoutEndPadding(For->end, lexer, padding+1);
		PAD;
		if(For->increment != nullptr){
		    printf("INCREMENT");
		    __dumpNodesWithoutEndPadding(For->increment, lexer, padding+1);
		};
	    }break;
	    };
	    PAD;
	    printf("BODY");
	    __dumpDynamicArrayNodes(For->body, lexer, padding);
	}break;
	case ASTType::PROC_DEFENITION: {
	    ASTProcDef *proc = (ASTProcDef*)node;
	    printf("proc_defenition");
	    PAD;
	    printf("name: %.*s", proc->name.len, proc->name.mem);
	    PAD;
	    u32 inCount = proc->uniInCount + proc->multiInCount;
	    for(u32 x=0; x<inCount; x+=1){
		ASTBase *node = proc->body[x];
		__dumpNodesWithoutEndPadding(node, lexer, padding + 1);
	    };
	    PAD;
	    printf("OUT");
	    __dumpDynamicArrayNodes(proc->out, lexer, padding);
	    DynamicArray<ASTBase*> &table = proc->body;
	    PAD;
	    printf("BODY");
	    for (u32 v=inCount; v < table.count; v += 1) {
		__dumpNodesWithoutEndPadding(table[v], lexer, padding + 1);
	    };
	} break;
	case ASTType::TYPE:{
	    AST_Type *type = (AST_Type*)node;
	    printf("%d", type->type);
	    PAD;
	}break;
	case ASTType::STRING:{
	    printf("string");
	    PAD;
	    ASTString *string = (ASTString*)node;
	    printf("\"%.*s\"", string->str.len, string->str.mem+1);
	}break;
	case ASTType::STRUCT_DEFENITION:{
	    printf("struct_defenition");
	    PAD;
	    ASTStructDef *Struct = (ASTStructDef*)node;
	    printf("name: %.*s", Struct->name.len, Struct->name.mem);
	    PAD;
	    printf("member_count: %d", Struct->memberCount);
	    PAD;
	    printf("BODY");
	    for (u32 v=0; v < Struct->body.count; v += 1) {
		__dumpNodesWithoutEndPadding(Struct->body[v], lexer, padding + 1);
	    };
	}break;
	case ASTType::IMPORT:{
	    ASTImport *imp = (ASTImport*)node;
	    printf("import");
	    PAD;
	    printf("file_path: %s", imp->fileName);
	}break;
	default:{
	    printf("\nASTType: %d", type);
	    UNREACHABLE;
	}break;
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
