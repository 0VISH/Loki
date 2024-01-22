#pragma once

enum class Type : u16{
    UNKOWN = 0,
    XE_VOID = 1,
    PTR,
    STRING,
    F_64,
    S_64,
    U_64,
    F_32,
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
enum class Flags {
    CONSTANT = 1,
    COMPTIME,
    GLOBAL,
    UNINITIALIZED,
    ALLOC,
};
typedef u8 Flag;
typedef u32 TypeID;
