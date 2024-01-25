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
    DECLERATION,
    INITIALIZATION_T_UNKNOWN,
    INITIALIZATION_T_KNOWN,
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
    MULTI_VAR_RHS,
    PROC_CALL,
    RETURN,
    ASSIGNMENT,
};
enum class ForType{
    FOR_EVER,
    C_LES,
    C_EQU,
    EXPR,
};


template <typename T>
struct EntityRef{
    T  *ent;
    u32 id;
};

struct ScopeEntities;
struct ASTBase {
    ASTType type;
    Flag flag;
};
struct ASTMultiVarRHS : ASTBase{
    s16 reg;
    ASTBase *rhs;
};
struct ASTGblVarInit : ASTBase{
    s16 reg;
    ASTBase *rhs;
    Type varType;
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
struct ASTAssignment : ASTBase{
    ASTBase *lhs;
    ASTBase *rhs;
};
struct AST_Type : ASTBase{
    union{
	u32 tokenOff;
	Type varType;
	TypeID id; //only for the checker
    };
    u8 pointerDepth;
};
struct ASTUniVar : ASTBase{
    union{
	String name;
	EntityRef<VariableEntity> entRef;
    };
    ASTBase *rhs;
    u32 tokenOff;
    AST_Type varType;
};
struct ASTBinOp : ASTBase{
    ASTBase *lhs;
    ASTBase *rhs;
    u32 tokenOff;
    Type lhsType;
    Type rhsType;
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
    union{
	String name;
	EntityRef<ProcEntity> entRef;
    };
    ScopeEntities *se;
    u32 firstArgID;
    u32 inCount;
    u32 tokenOff;
};
struct ASTProcCall : ASTBase{
    DynamicArray<ASTBase*> args;
    DynamicArray<u32>      argOffs;
    union{
	String name;
	EntityRef<ProcEntity> entRef;
    };
    u32 off;
};
struct ASTVariable : ASTBase{
    union{
	String name;
	EntityRef<VariableEntity> varEntRef;
	EntityRef<StructEntity>   structEntRef;
    };
    u32 tokenOff;
    u8 pAccessDepth;
};
struct ASTModifier : ASTBase{
    union{
	String name;
	EntityRef<VariableEntity> varEntRef;
	EntityRef<StructEntity>   structEntRef;
    };
    ASTBase *child;
    u32 tokenOff;
    u8 pAccessDepth;
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
    DynamicArray<ASTBase*> members;
    union{
	String name;
	EntityRef<StructEntity> entRef;
    };
    u32 tokenOff;
};
struct ASTReturn : ASTBase{
    ASTBase *expr;
    u32 tokenOff;
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
    case ASTType::PROC_DEFENITION:{
	ASTProcDef *proc = (ASTProcDef*)base;
        freeBody(proc->body);
	if(proc->out.len != 0){proc->out.uninit();};
    }break;
    case ASTType::PROC_CALL:{
	ASTProcCall *pc = (ASTProcCall*)base;
	freeBody(pc->args);
	pc->argOffs.uninit();
    }break;
    case ASTType::STRUCT_DEFENITION:{
	ASTStructDef *Struct = (ASTStructDef*)base;
	Struct->members.uninit();
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
    node->flag = 0;
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
	u32 start = x;
	x += 1;
	u8 pointerDepth = 0;
	while(tokTypes[x] == (Token_Type)'^'){
	    pointerDepth += 1;
	    x += 1;
	};
	if(tokTypes[x] == (Token_Type)'.'){
	    ASTModifier *mod = (ASTModifier*)allocAST(sizeof(ASTModifier), ASTType::MODIFIER, file);
	    mod->name = makeStringFromTokOff(start, lexer);
	    mod->tokenOff = start;
	    mod->pAccessDepth = pointerDepth;
	    childReq = true;
	    if(root == nullptr){root = mod;};
	    if(lastMod){lastMod->child = mod;};
	    lastMod = mod;
	    x += 1;
	}else{
	    ASTVariable *var = (ASTVariable*)allocAST(sizeof(ASTVariable), ASTType::VARIABLE, file);
	    var->name = makeStringFromTokOff(start, lexer);
	    var->tokenOff = start;
	    var->pAccessDepth = pointerDepth;
	    childReq = false;
	    if(root == nullptr){root = var;};
	    if(lastMod){lastMod->child = var;};
	};
    };
    if(childReq){
	lexer.emitErr(tokOffs[x].off, "Identifier required");
	return nullptr;
    };
    return root;
};
s32 getTokenOff(Token_Type tok, DynamicArray<Token_Type> &tokTypes, u32 cur){
    s32 x = cur;
    while(tokTypes[x] != tok){
	switch(tokTypes[x]){
	case Token_Type::END_OF_FILE:
	    return -1;
	};
	x += 1;
    };
    return x;
};
u32 getEnd(DynamicArray<Token_Type>& tokTypes, u32 x) {
    //SIMD?
    u8 bra = 1;
    while(true){
	switch(tokTypes[x]){
	case (Token_Type)';': return x;
	case (Token_Type)',': return x;
	case (Token_Type)'(': bra += 1; break;
	case (Token_Type)')':{
	    bra -= 1;
	    if(bra == 0){return x;};
	}break;
	case Token_Type::END_OF_FILE: return x;
	};
	x += 1;
    };
    return x;
};
ASTBase *genASTExprTree(Lexer &lexer, ASTFile &file, u32 &x, u32 end);
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
	if(tokTypes[x+1] == (Token_Type)'('){
	    ASTProcCall *pc = (ASTProcCall*)allocAST(sizeof(ASTProcCall), ASTType::PROC_CALL, file);
	    pc->off = x;
	    pc->name = makeStringFromTokOff(x, lexer);
	    x += 2;
	    pc->args.init();
	    pc->argOffs.init();
	    s32 bend = getTokenOff((Token_Type)')', tokTypes, x);
	    if(bend == -1){
		lexer.emitErr(tokOffs[pc->off].off, "Expected ending ')'");
		return false;
	    };
	    while(true){
		s32 cend = getTokenOff((Token_Type)',', tokTypes, x);
		s32 end = cend;
		if(cend == -1){
		    end = bend;
		};
		pc->argOffs.push(x);
		ASTBase *arg = genASTExprTree(lexer, file, x, end);
		if(arg == false){return nullptr;};
		pc->args.push(arg);
		switch(tokTypes[x]){
		case (Token_Type)')': goto EXIT_LOOP_PROC_CALL;
		case (Token_Type)',': x+=1; break;
		default:
		    lexer.emitErr(tokOffs[x].off, "Expected ',' or ')'");
		    return nullptr;
		};
	    };
	EXIT_LOOP_PROC_CALL:
	    x += 1;
	    return (ASTBase*)pc;
	}else{
	    return genVariable(x, lexer, file);
	};
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
bool parseType(AST_Type *type, u32 &x, Lexer &lexer, ASTFile &file){
    BRING_TOKENS_TO_SCOPE;
    u8 pointerDepth = 0;
    while(tokTypes[x] == (Token_Type)'^'){
	pointerDepth += 1;
	x += 1;
    };
    if(isType(tokTypes[x]) == false && tokTypes[x] != Token_Type::IDENTIFIER){
	lexer.emitErr(tokOffs[x].off, "Expected a type");
	return false;
    };
    type->tokenOff = x;
    type->pointerDepth = pointerDepth;
    return true;
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
	ret->tokenOff = x;
	x += 1;
	if(tokTypes[x] == (Token_Type)'}' || tokTypes[x] == (Token_Type)'\n'){
	    ret->expr = nullptr;
	}else{
	    s32 end = getTokenOff((Token_Type)';', tokTypes, x);
	    if(end == -1){
		lexer.emitErr(tokOffs[start].off, "Expected ending ';'");
		return false;
	    };
	    ret->expr = genASTExprTree(lexer, file, x, end);
	};
	x += 1;
	table.push(ret);
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
	    while(tokTypes[x] != (Token_Type)'}'){
		bool result = parseBlock(lexer, file, For->body, x);
		if(result == false){return false;};
	    };
	    x += 1;
	    table.push(For);
	}break;
	case Token_Type::IDENTIFIER:{
	    if(tokTypes[x+1] != (Token_Type)':'){
		goto PARSE_FOR_EXPR;
	    };
	    ASTUniVar *var = (ASTUniVar*)allocAST(sizeof(ASTUniVar), ASTType::INITIALIZATION_T_UNKNOWN, file);
	    var->varType = {};
	    var->tokenOff = x;
	    var->name = makeStringFromTokOff(x, lexer);
	    var->flag = 0;
	    SET_BIT(var->flag, Flags::ALLOC);
	    For->body.push(var);
	    For->increment = nullptr;
	    x += 2;
	    if(tokTypes[x] == Token_Type::IDENTIFIER || isKeyword(tokTypes[x])){
		var->varType.tokenOff = x;
		var->type = ASTType::INITIALIZATION_T_KNOWN;
		x += 1;
	    };
	    s32 end = getTokenOff(Token_Type::TDOT, tokTypes, x);
	    if(end == -1){
		lexer.emitErr(tokOffs[x].off, "Expected '...' to specify the upper bound");
		return false;
	    };
	    var->rhs = genASTExprTree(lexer, file, x, end);
	    if(var->rhs == nullptr){return false;};
	    x = end + 1;
	    s32 dend = getTokenOff(Token_Type::DDOT, tokTypes, x);
	    s32 bend = getTokenOff((Token_Type)'{', tokTypes,  x);
	    if(dend != -1){
		end = dend;
	    }else if(bend != -1){
		end = bend;
	    }else{
		lexer.emitErr(tokOffs[start].off, "Expected '..' or '{'. Found nothing.");
		return false;
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
		For->increment = genASTExprTree(lexer, file, x, end);
		if(For->increment == nullptr){return false;};
		For->incrementOff = end;
		x = end;
	    };
	    if(tokTypes[x] != (Token_Type)'{'){
		lexer.emitErr(tokOffs[x].off, "Expected '{' here");
		return false;
	    };
	    x += 1;
	    while(tokTypes[x] != (Token_Type)'}'){
		bool result = parseBlock(lexer, file, For->body, x);
		if(result == false){return false;};
	    };
	    x += 1;
	    table.push(For);
	}break;
	default:{
	    x += 1;
	    PARSE_FOR_EXPR:
	    For->loopType = ForType::EXPR;
	    s32 end = getTokenOff((Token_Type)'{', tokTypes,  x);
	    if(end == -1){
		lexer.emitErr(tokOffs[start].off, "Expected '{' to define the body");
		return false;
	    };
	    For->expr = genASTExprTree(lexer, file, x, end);
	    if(For->expr == nullptr){return false;};
	    x = end;
	    if(tokTypes[x] != (Token_Type)'{'){
		lexer.emitErr(tokOffs[x].off, "Expected '{' here");
		return nullptr;
	    };
	    x += 1;
	    For->body.init();
	    while(tokTypes[x] != (Token_Type)'}'){
		bool result = parseBlock(lexer, file, For->body, x);
		if(result == false){return false;};
	    };
	    x += 1;
	    table.push(For);
	}break;
	};
    }break;
    case Token_Type::K_IF:{
	ASTIf *If = (ASTIf*)allocAST(sizeof(ASTIf), ASTType::IF, file);
	If->tokenOff = x;
	x += 1;
	If->body.zero();
	If->elseBody.zero();
	s32 end = getTokenOff((Token_Type)'{', tokTypes, x);
	if(end == -1){
	    lexer.emitErr(tokOffs[start].off, "Expected '{' to define the body");
	    return nullptr;
	};
	If->expr = genASTExprTree(lexer, file, x, end);
	if(If->expr == nullptr){return false;};
	x = end;
	if(tokTypes[x] != (Token_Type)'{'){
	    lexer.emitErr(tokOffs[x].off, "Expected '{' here");
	    return false;
	};
	If->body.init();
	x += 1;
	while(tokTypes[x] != (Token_Type)'}'){
	    bool result = parseBlock(lexer, file, If->body, x);
	    if(result == false){return false;};
	};
	x += 1;
	if(tokTypes[x] != Token_Type::K_ELSE){return (ASTBase*)If;};
	If->elseBody.init();
	x += 1;
	switch(tokTypes[x]){
	case (Token_Type)'{':{
	    x += 1;
	    while(tokTypes[x] != (Token_Type)'}'){
		bool result = parseBlock(lexer, file, If->elseBody, x);
		if(result == false){return false;};
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
    }break;
    case Token_Type::IDENTIFIER: {
	x += 1;
	ASTBase *lhs = nullptr;
	switch (tokTypes[x]) {
	case (Token_Type)'^':
	case (Token_Type)'.':
	case (Token_Type)'=':{
	    x = start;
	    ASTBase *lhs = genVariable(x, lexer, file);
	    if(tokTypes[x] != (Token_Type)'='){
		lexer.emitErr(tokOffs[x].off, "Expected '='");
		return false;
	    };
	    x += 1;
	    s32 end = getTokenOff((Token_Type)';', tokTypes, x);
	    if(end == -1){
		lexer.emitErr(tokOffs[start].off, "Expected ending ';'");
		return false;
	    };
	    ASTBase *rhs = genASTExprTree(lexer, file, x, end);
	    if(rhs == nullptr){return false;};
	    ASTAssignment *ass= (ASTAssignment*)allocAST(sizeof(ASTAssignment), ASTType::ASSIGNMENT, file);
	    ass->lhs = lhs;
	    ass->rhs = rhs;
	    table.push(ass);
	}break;
	case (Token_Type)':': {
	    x += 1;
	    ASTType uniVarType = ASTType::INITIALIZATION_T_UNKNOWN; 
	    switch (tokTypes[x]) {
	    case (Token_Type)'=': {
		//single variable assignment
		SINGLE_VARIABLE_ASSIGNMENT:
		ASTUniVar *assign = (ASTUniVar*)allocAST(sizeof(ASTUniVar), uniVarType, file);
		assign->tokenOff = start;
		assign->name = makeStringFromTokOff(start, lexer);
		assign->varType.pointerDepth = 0;
		x += 1;
		s32 end = getTokenOff((Token_Type)';', tokTypes, x);
		if(end == -1){
		    lexer.emitErr(tokOffs[start].off, "Expected ending ';'");
		    return false;
		};
		if(tokTypes[x] == Token_Type::TDOT){
		    SET_BIT(flag, Flags::UNINITIALIZED);
		    x += 1;
		}else{
		    assign->rhs = genASTExprTree(lexer, file, x, end);
		    if(assign->rhs == nullptr){return false;};
		};
		assign->flag = flag;
		table.push(assign);
	    } break;
	    case (Token_Type)':': {
		x += 1;
		switch (tokTypes[x]){
		case Token_Type::K_STRUCT:{
		    x += 1;
		    if(tokTypes[x] != (Token_Type)'{'){
			lexer.emitErr(tokOffs[x].off, "Expected '{'");
			return false;
		    };
		    x += 1;
		    ASTStructDef *Struct = (ASTStructDef*)allocAST(sizeof(ASTStructDef), ASTType::STRUCT_DEFENITION, file);
		    Struct->name = makeStringFromTokOff(start, lexer);
		    Struct->members.init();
		    Struct->tokenOff = start;
		    while (tokTypes[x] != (Token_Type)'}') {
			//parse body
			u32 start = x;
			bool result = parseBlock(lexer, file, Struct->members, x);
			if(result == false) {return false;};
			ASTBase *node = Struct->members[Struct->members.count - 1];
			switch(node->type){
			case ASTType::DECLERATION: break;
			default:
			    lexer.emitErr(tokOffs[x].off, "Invalid statement. Struct only takes uni/multi decleration");
			    return nullptr;
			};
		    };
		    x += 1;
		    table.push(Struct);
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
		    u32 inCount = 0;
		    //parse input
		    x += 1;
		    if(tokTypes[x] == (Token_Type)')'){goto END_PROC_INPUT_PARSING_LOOP;};
		    while(true){
			u32 argStart = x;
			bool result = parseBlock(lexer, file, proc->body, x);
			if(result == false){return false;};
			ASTBase *arg = proc->body[proc->body.count - 1];
			if(arg->type != ASTType::DECLERATION){
			    lexer.emitErr(tokOffs[argStart].off, "Procedure defenition only takes declerations");
			    return false;
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
		    proc->inCount = proc->body.count;
		    if(tokTypes[x] != (Token_Type)')'){
			lexer.emitErr(tokOffs[x].off, "Expected ')'");
			return false;
		    };
		    PARSE_AFTER_ARGS:
		    x += 1;
		    switch (tokTypes[x]) {
		    case Token_Type::ARROW: {
			x += 1;
			proc->out.init();
			bool bracket = false;
			if (tokTypes[x] == (Token_Type)'(') { bracket = true; x += 1; };
			//parse output
			while (true) {
			    ASTBase *typeNode = allocAST(sizeof(AST_Type), ASTType::TYPE, file);
			    if(parseType((AST_Type*)typeNode, x, lexer, file) == false){return false;};
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
		    u32 retCount = 0;
		    while (tokTypes[x] != (Token_Type)'}') {
			//parse body
			bool result = parseBlock(lexer, file, proc->body, x);
			if (result == false) {
			    return false;
			};
			if(proc->body[proc->body.count-1]->type == ASTType::RETURN){retCount += 1;};
		    };
		    if(retCount == 0 && proc->out.count == 0){
			ASTReturn *ret = (ASTReturn*)allocAST(sizeof(ASTReturn), ASTType::RETURN, file);
			ret->expr = nullptr;
			proc->body.push(ret);
		    };
		    x += 1;
		    table.push(proc);
		}break;
		};
	    }break;
	    default: {
		if (isType(tokTypes[x]) || tokTypes[x] == Token_Type::IDENTIFIER || tokTypes[x] == (Token_Type)'^') {
		    ASTUniVar *assign = (ASTUniVar*)allocAST(sizeof(ASTUniVar), ASTType::DECLERATION, file);
		    if(parseType(&assign->varType, x, lexer, file) == false){return false;};
		    x += 1;
		    if (tokTypes[x] == (Token_Type)'=') {
			uniVarType = ASTType::INITIALIZATION_T_KNOWN;
			goto SINGLE_VARIABLE_ASSIGNMENT;
		    };
		    assign->tokenOff = start;
		    assign->name = makeStringFromTokOff(start, lexer);
		    assign->flag = flag;
		    table.push(assign);
		}else{
		    lexer.emitErr(tokOffs[x].off, "Expected a type");
		    return false;
		};
	    } break;
	    } break;
	}break;
	case (Token_Type)',': {
	    //multi var def/decl
	    x = start;
	    ASTType type = ASTType::INITIALIZATION_T_UNKNOWN;
	    u32 equalSignPos = 0;
	    s32 end = getTokenOff((Token_Type)';', tokTypes, x);
	    if(end == -1){
		lexer.emitErr(tokOffs[start].off, "Expected ending ';'");
		return false;
	    };
	    Flag flag = 0;
	    ASTMultiVarRHS *mvr = (ASTMultiVarRHS*)allocAST(sizeof(ASTMultiVarRHS), ASTType::MULTI_VAR_RHS, file);
	    mvr->reg = 0;
	    s32 cend = getTokenOff((Token_Type)':', tokTypes, x);
	    if(cend == -1){
		lexer.emitErr(tokOffs[start].off, "Expected a ':'");
		return false;
	    };
	    AST_Type varType;
	    u32 ucend = cend;
	    if(tokTypes[cend] != (Token_Type)'='){
		type = ASTType::INITIALIZATION_T_KNOWN;
		parseType(&varType, ucend, lexer, file);
		cend = ucend;
	    };
	    if(tokTypes[cend] == (Token_Type)'='){
		cend += 1;
		if(tokTypes[cend] == Token_Type::TDOT){
		    SET_BIT(flag, Flags::UNINITIALIZED);
		    mvr->flag = flag;
		}else{
		    ucend = cend;
		    mvr->rhs = genASTExprTree(lexer, file, ucend, end);
		    if(mvr->rhs == nullptr){return false;}
		};
	    };
	    while(true){
		if(tokTypes[x] == Token_Type::IDENTIFIER){
		    ASTUniVar *uv = (ASTUniVar*)allocAST(sizeof(ASTUniVar), type, file);
		    uv->flag = flag;
		    uv->rhs = mvr;
		    uv->name = makeStringFromTokOff(x, lexer);
		    uv->varType = varType;
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
	}break;
	default:
	    goto PARSE_EXPR;
	}break;
    }break;
    default: {
	PARSE_EXPR:
	x = start;
	s32 end = getTokenOff((Token_Type)';', tokTypes, x);
	if(end == -1){
	    lexer.emitErr(tokOffs[start].off, "Expected ending ';'");
	    return false;
	};
	ASTBase *tree = genASTExprTree(lexer, file, x, end);
	if(tree == nullptr){return false;};
	table.push(tree);
	return true;
    } break;
    };
    if(tokTypes[x] != (Token_Type)';'){
	lexer.emitErr(tokOffs[x].off, "Expected ';'");
	return false;
    };
    x += 1;
    return true;
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
	    PAD;
	    printf("pAccessDepth: %d", mod->pAccessDepth);
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
	case ASTType::ASSIGNMENT:{
	    ASTAssignment *ass = (ASTAssignment*)node;
	    printf("assignemnt");
	    PAD;
	    printf("LHS");
	    __dumpNodesWithoutEndPadding(ass->lhs, lexer, padding+1);
	    PAD;
	    printf("RHS");
	    __dumpNodesWithoutEndPadding(ass->rhs, lexer, padding+1); 
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
	case ASTType::MULTI_VAR_RHS:{
	    ASTMultiVarRHS *mvr = (ASTMultiVarRHS*)node;
	    printf("multi_var_rhs");
	    PAD;
	    printf("rhs");
	    PAD;
	    __dumpNodesWithoutEndPadding(mvr->rhs, lexer, padding+1);
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
	    PAD;
	    printf("pAccessDepth: %d", var->pAccessDepth);
	}break;
	case ASTType::DECLERATION: {
	    ASTUniVar *decl = (ASTUniVar*)node;
	    u32 x = decl->varType.tokenOff;
	    printf("decleration");
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
	case ASTType::INITIALIZATION_T_KNOWN: {
	    ASTUniVar *decl = (ASTUniVar*)node;
	    u32 x = decl->varType.tokenOff;
	    printf("initialization_t_unkown");
	    PAD;
	    printf("type: %.*s", lexer.tokenOffsets[x].len, lexer.fileContent + lexer.tokenOffsets[x].off);
	};
	    flag = true;
	case ASTType::INITIALIZATION_T_UNKNOWN: {
	    if (flag) { flag = false; }
	    else { printf("initialization_t_unkown"); };	
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
	    for(u32 x=0; x<proc->inCount; x+=1){
		ASTBase *node = proc->body[x];
		__dumpNodesWithoutEndPadding(node, lexer, padding + 1);
	    };
	    PAD;
	    printf("OUT");
	    __dumpDynamicArrayNodes(proc->out, lexer, padding);
	    DynamicArray<ASTBase*> &table = proc->body;
	    PAD;
	    printf("BODY");
	    for (u32 v=proc->inCount; v < table.count; v += 1) {
		__dumpNodesWithoutEndPadding(table[v], lexer, padding + 1);
	    };
	} break;
	case ASTType::PROC_CALL:{
	    ASTProcCall *pc = (ASTProcCall*)node;
	    printf("proc_call");
	    PAD;
	    printf("name: %.*s", pc->name.len, pc->name.mem);
	    PAD;
	    for(u32 x=0; x<pc->args.count; x+=1){
		ASTBase *arg = pc->args[x];
		__dumpNodesWithoutEndPadding(arg, lexer, padding+1);
	    };
	}break;
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
	    printf("MEMBERS");
	    for (u32 v=0; v < Struct->members.count; v += 1) {
		__dumpNodesWithoutEndPadding(Struct->members[v], lexer, padding + 1);
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
