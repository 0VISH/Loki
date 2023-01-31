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
enum class VarFlags {
	CONSTANT = 1,
};

typedef u16 TypeId;
typedef u8 Flag;