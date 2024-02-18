#pragma once

#include "../frontend/type.hh"

#define REGISTER_COUNT      100

/*
  %d   register
  $d   global
  @d   proc
*/
enum class Bytecode : u16{
    NONE = 0,
    CAST,
    STORE,
    LOAD,
    MOV,
    MOV_CONST,
    ADD,
    SUB,
    MUL,
    DIV,
    SHL,
    SHR,
    CMP,
    SETG,
    SETL,
    SETE,
    SETGE,
    SETLE,
    JMPS,         //jumps if given register is not 0
    JMP,
    DEF,
    RET,
    NEG,
    LABEL,
    PROC_END,
    ALLOC,
    GLOBAL,
    CALL,
    STRUCT,
    GET_MEMBER,
    GET_ELEMENT,
    _TEXT_STARTUP_START,
    _TEXT_STARTUP_END,
    NEXT_BUCKET,
    COUNT,
};
typedef s16 Reg;

const u16 const_in_stream = sizeof(s64) / sizeof(Bytecode);
const u16 pointer_in_stream = sizeof(Bytecode*) / sizeof(Bytecode);
const u16 bytecodes_in_bucket = 30;

struct BytecodeBucket{
    BytecodeBucket *next;
    BytecodeBucket *prev;
    Bytecode bytecodes[bytecodes_in_bucket+1];     //padding with NEXT_BUCKET
};
struct Expr{
    Type type;
    Reg reg;
    bool isPtr;
};
struct VariableContext{
    Hashmap<u32, u32> varIDToOff;
    DynamicArray<Expr> varRegAndTypes;
    Scope scope;

    void init(Scope scopeOfContext, u32 varlen = 10);
    void uninit();
    void registerVar(Reg reg, Type type, u32 id);
    Expr getVar(u32 id);
};
