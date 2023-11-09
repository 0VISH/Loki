Type greaterType(Type t1, Type t2){
    if(t1 < t2){return t1;};
    return t2;
};
bool isTypeNum(Type type){return (type > Type::XE_VOID) && (type <= Type::COMP_INTEGER);};
bool isFloat(Type type){
    return (type == Type::F_64 || type == Type::F_32 || type == Type::COMP_DECIMAL);
};
bool isIntS(Type type){
    return (type == Type::S_64 || type == Type::S_32 || type == Type::S_16 || type == Type::S_8 || type == Type::COMP_INTEGER);
};
bool isIntU(Type type){
    return (type == Type::U_64 || type == Type::U_32 || type == Type::U_16 || type == Type::U_8);
};
bool isSameTypeRange(Type type1, Type type2){
    if(isFloat(type1)){
	return isFloat(type2);
    };
    if(isIntS(type1)){
	return isIntS(type2);
    };
    if(isIntU(type1)){
	return isIntU(type2);
    };
    return false;
};
