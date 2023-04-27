#include "frontend/entity.hh"

Type getType(TypeID type) {
    u16 x = 1;
    while (IS_BIT(type, x) == 0) { x += 1; };
    return (Type)x;
};
Type greaterType(Type t1, Type t2){
    if(t1 < t2){return t1;};
    return t2;
};

TypeID getTreeTypeID(ASTBase *base, Flag &flag, DynamicArray<ScopeEntities*> &see, Lexer &lexer) {
    TypeID id = 0;
    switch (base->type) {
    case ASTType::BIN_ADD:
    case ASTType::BIN_SUB:
    case ASTType::BIN_MUL:
    case ASTType::BIN_DIV: {
	ASTBinOp *node = (ASTBinOp*)base;
	Flag lhsFlag = 0;
	Flag rhsFlag = 0;
	TypeID lhsTypeID = getTreeTypeID(node->lhs, lhsFlag, see, lexer);
	if(lhsTypeID == 0){return 0;};
	TypeID rhsTypeID = getTreeTypeID(node->rhs, rhsFlag, see, lexer);
	if(rhsTypeID == 0){return 0;};
	flag = lhsFlag & rhsFlag;
	return (TypeID)((u16)lhsTypeID | (u16)rhsTypeID);
    } break;
    case ASTType::NUM_INTEGER:{
	SET_BIT(flag, Flags::CONSTANT);
	SET_BIT(id, Type::COMP_INTEGER);
	return id;
    }break;
    case ASTType::NUM_DECIMAL:{
	SET_BIT(flag, Flags::CONSTANT);
	SET_BIT(id, Type::COMP_DECIMAL);
	return id;
    }break;
    case ASTType::VARIABLE:{
	BRING_TOKENS_TO_SCOPE;
	ASTVariable *var = (ASTVariable*)base;
	Type type = Type::UNKOWN;
	for(u32 x=see.count; x>0; x-=1){
	    ScopeEntities *se = see[x-1];
	    s32 id = se->varMap.getValue(var->name);
	    if(id != -1){
		const VariableEntity &e = se->varEntities[id];
		type = e.type;
		flag &= e.flag;
		break;
	    };
	};
	if(type == Type::UNKOWN){
	    lexer.emitErr(tokOffs[var->tokenOff].off, "Variable not defined");
	    return (TypeID)0;
	};
	SET_BIT(id, (u32)type);
	return id;
    }break;
    };
    SET_BIT(id, Type::COMP_VOID);
    return id;
};
Type tokenKeywordToType(Lexer &lexer, u32 off){
    BRING_TOKENS_TO_SCOPE;
    switch (tokTypes[off]) {
    case Token_Type::K_U8:  return Type::U_8;
    case Token_Type::K_U16: return Type::U_16;
    case Token_Type::K_U32: return Type::U_32;
    case Token_Type::K_S8:  return Type::S_8;
    case Token_Type::K_S16: return Type::S_16;
    case Token_Type::K_S32: return Type::S_32;
    default:
	printf("%d", tokTypes[off]);
	DEBUG_UNREACHABLE;
        return Type::UNKOWN;
    };
    return Type::UNKOWN;
};

#if(XE_DBG)
char *Type2CString[] =  {
    "unkown",
    "comp_void",
    "f_64",
    "s_64",
    "u_64",
    "f_32",
    "s_32",
    "u_32",
    "f_16",
    "s_16",
    "u_16",
    "s_8",
    "u_8",
    "comp_decimal",
    "comp_integer",
};
static_assert((u16)Type::TYPE_COUNT == ARRAY_LENGTH(Type2CString), "Type and Type2CString not one-to-one");
namespace dbg {
    char *getTypeName(Type type) { return Type2CString[(u16)(type)-1]; };
};
#endif
