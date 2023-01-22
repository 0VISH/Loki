enum class Type{
	COMP_VOID = 1,
	S_64,
	U_64,
	S_32,
	U_32,
	S_16,
	U_16,
	S_8,
	U_8,
	COMP_DECIMAL,
	COMP_INTEGER,
	TYPE_COUNT
};

typedef u16 TypeId;

Type getType(TypeId type) {
	u16 x = 1;
	while (IS_BIT(type, x) == 0) { x += 1; };
	return (Type)x;
};

Type getTreeType(ASTBase *base) {
	switch (base->type) {
		case ASTType::BIN_ADD:
		case ASTType::BIN_SUB:
		case ASTType::BIN_MUL:
		case ASTType::BIN_DIV: {
			ASTlr *node = (ASTlr*)base;
			Type lhsType = getTreeType(node->lhs);
			Type rhsType = getTreeType(node->rhs);
			return (Type)((u16)lhsType | (u16)rhsType);
		} break;
		case ASTType::NUM_INTEGER: return Type::COMP_INTEGER;
		case ASTType::NUM_DECIMAL: return Type::COMP_DECIMAL;
	};
	return Type::COMP_VOID;
};

#if(XE_DBG)
char *Type2CString[] =  {
	"comp_void",
	"s_64",
	"u_64",
	"s_32",
	"u_32",
	"s_16",
	"u_16",
	"s_8",
	"u_8",
	"comp_decimal",
	"comp_integer",
};
static_assert((u16)Type::TYPE_COUNT-1 == ARRAY_LENGTH(Type2CString), "Type and Type2CString not one-to-one");
namespace dbg {
	char *getTypeName(Type type) { return Type2CString[(u16)(type)-1]; };
};
#endif