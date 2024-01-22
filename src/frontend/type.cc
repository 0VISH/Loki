#include "entity.hh"

Type typeID2Type(TypeID type) {
    if(type == 0){return Type::UNKOWN;};
    u16 x = 1;
    while (IS_BIT(type, x) == 0) { x += 1; };
    return (Type)x;
};
AST_Type getTreeTypeID(ASTBase *base, Flag &flag, DynamicArray<ScopeEntities*> &see, Lexer &lexer) {
    BRING_TOKENS_TO_SCOPE;
    TypeID id = 0;
    AST_Type type = {};
    switch (base->type) {
    case ASTType::BIN_GRT:
    case ASTType::BIN_GRTE:
    case ASTType::BIN_LSR:
    case ASTType::BIN_LSRE:
    case ASTType::BIN_EQU:
    case ASTType::BIN_ADD:
    case ASTType::BIN_MUL:
    case ASTType::BIN_DIV: {
	ASTBinOp *node = (ASTBinOp*)base;
	Flag lhsFlag = 0;
	Flag rhsFlag = 0;
	auto lhsTypeNode = getTreeTypeID(node->lhs, lhsFlag, see, lexer);
	TypeID lhsTypeID = lhsTypeNode.id;
	if(lhsTypeID == 0){return type;};
	auto rhsTypeNode = getTreeTypeID(node->rhs, rhsFlag, see, lexer);
	TypeID rhsTypeID = rhsTypeNode.id;
	if(rhsTypeID == 0){return type;};
	flag = lhsFlag & rhsFlag;
	Type lhsType = typeID2Type(lhsTypeID);
	Type rhsType = typeID2Type(rhsTypeID);
	if(isSameTypeRange(lhsType, rhsType) == false){
	    lexer.emitErr(tokOffs[node->tokenOff].off, "LHS type and RHS type are not in the same type range");
	    return type;
	};
	if(lhsTypeNode.pointerDepth != rhsTypeNode.pointerDepth){
	    lexer.emitWarning(tokOffs[node->tokenOff].off, "LHS pointer depth(%d) != RHS pointer depth(%d)", lhsTypeNode.pointerDepth, rhsTypeNode.pointerDepth);
	};
	type.id = (TypeID)((u32)lhsTypeID | (u32)rhsTypeID);
	return type;
    } break;
    case ASTType::NUM_INTEGER:{
	SET_BIT(flag, Flags::CONSTANT);
	SET_BIT(id, Type::COMP_INTEGER);
	type.id = id;
	return type;
    }break;
    case ASTType::NUM_DECIMAL:{
	SET_BIT(flag, Flags::CONSTANT);
	SET_BIT(id, Type::COMP_DECIMAL);
	type.id = id;
	return type;
    }break;
    case ASTType::STRING:{
	SET_BIT(flag, Flags::CONSTANT);
	SET_BIT(id, Type::STRING);
	type.id = id;
	return type;
    }break;
    case ASTType::UNI_NEG:{
	ASTUniOp *uniOp = (ASTUniOp*)base;
	return getTreeTypeID(uniOp->node, flag, see, lexer);
    }break;
    case ASTType::VARIABLE:{
	ASTVariable *var = (ASTVariable*)base;
	SET_BIT(id, (u32)var->varEntRef.ent->type);
	type.id = id;
	type.pointerDepth = var->varEntRef.ent->pointerDepth;
	return type;
    }break;
    };
    SET_BIT(id, Type::XE_VOID);
    type.id = id;
    return type;
};
Type getTreeType(ASTBase *base, Flag &flag, DynamicArray<ScopeEntities*> &see, Lexer &lexer){
    return typeID2Type(getTreeTypeID(base, flag, see, lexer).id);
};
StructEntity *getStructEntity(String name, DynamicArray<ScopeEntities*> &see);
Type tokenKeywordToType(u32 off, Lexer &lexer, DynamicArray<ScopeEntities*> &see){
    BRING_TOKENS_TO_SCOPE;
    switch (tokTypes[off]) {
    case Token_Type::K_U8:  return Type::U_8;
    case Token_Type::K_U16: return Type::U_16;
    case Token_Type::K_U32: return Type::U_32;
    case Token_Type::K_S8:  return Type::S_8;
    case Token_Type::K_S16: return Type::S_16;
    case Token_Type::K_S32: return Type::S_32;
    case Token_Type::K_F32: return Type::F_32;
    case Token_Type::K_F64: return Type::F_64;
    default:
	String name = makeStringFromTokOff(off, lexer);
	StructEntity *entity = getStructEntity(name, see);
	if(entity == nullptr){
	    lexer.emitErr(tokOffs[off].off, "Unkown type");
	    return Type::UNKOWN;
	};
	return (Type)entity->id;
    };
    return Type::UNKOWN;
};
