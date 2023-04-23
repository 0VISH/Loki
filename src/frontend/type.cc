Type getType(TypeId type) {
    u16 x = 1;
    while (IS_BIT(type, x) == 0) { x += 1; };
    return (Type)x;
};

Type getTreeType(ASTBase *base, Flag &flag) {
    switch (base->type) {
    case ASTType::BIN_ADD:
    case ASTType::BIN_SUB:
    case ASTType::BIN_MUL:
    case ASTType::BIN_DIV: {
	ASTBinOp *node = (ASTBinOp*)base;
	Flag lhsFlag = 0;
	Flag rhsFlag = 0;
	Type lhsType = getTreeType(node->lhs, lhsFlag);
	Type rhsType = getTreeType(node->rhs, rhsFlag);
	flag = lhsFlag & rhsFlag;
	return (Type)((u16)lhsType | (u16)rhsType);
    } break;
    case ASTType::NUM_INTEGER:
	SET_BIT(flag, Flags::CONSTANT);
	return Type::COMP_INTEGER;
    case ASTType::NUM_DECIMAL:
	SET_BIT(flag, Flags::CONSTANT);
	return Type::COMP_DECIMAL;
    };
    return Type::COMP_VOID;
};
Type getType(Lexer &lexer, u32 off){
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
	break;
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
