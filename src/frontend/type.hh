enum class Type{
    UNKOWN = 0,
    COMP_VOID = 1,
    F_64,
    S_64,
    U_64,
    F_32,
    S_32,
    U_32,
    F_16,
    S_16,
    U_16,
    S_8,
    U_8,
    COMP_DECIMAL,
    COMP_INTEGER,
    TYPE_COUNT
};
enum class Flags {
    CONSTANT = 1,
    COMPTIME,
};

typedef u16 TypeID;
typedef u8 Flag;
