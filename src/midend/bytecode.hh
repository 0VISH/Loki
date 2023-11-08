#pragma once

#include "../frontend/type.hh"

#define REGISTER_COUNT      100

/*
  $d  global   //TODO:
  %d  register
  @d  proc
*/
enum class Bytecode : u16{
    NONE = 0,
    TYPE,
    REG,
    GLOBAL,
    CAST,
    STORE,
    LOAD,
    MOV,
    MOV_CONST,
    ADD,
    SUB,
    MUL,
    DIV,
    CMP,
    SETG,
    SETL,
    SETE,
    SETGE,
    SETLE,
    JMPNS,        //jumps if given register is 0
    JMPS,         //jumps if given register is not 0
    JMP,
    DEF,
    RET,
    NEG,
    LABEL,
    BLOCK_START,
    BLOCK_END,
    ALLOC,
    NEXT_BUCKET,
    COUNT,
};

const u16 const_in_stream = sizeof(s64) / sizeof(Bytecode);
const u16 pointer_in_stream = sizeof(Bytecode*) / sizeof(Bytecode);
const u16 bytecodes_in_bucket = 30;
const u16 register_in_stream = 2;

struct BytecodeBucket{
    BytecodeBucket *next;
    BytecodeBucket *prev;
    Bytecode bytecodes[bytecodes_in_bucket+1];     //padding with NEXT_BUCKET
};
struct Expr{
    Type type;
    u16 reg;
};

struct BytecodeFile{
    DynamicArray<Bytecode*>   labels;
    BytecodeBucket           *firstBucket;
    BytecodeBucket           *curBucket;
    u16                       cursor;

    void init();
    void uninit();
    void newBucketAndUpdateCurBucket();
    void reserve(u16 reserve);
    void emit(Bytecode bc);
    void emit(u16 bc);
    void emit(Type type);
    Bytecode *getCurBytecodeAdd();
    void emitConstInt(s64 num);
    void emitConstDec(f64 num);
    void alloc(Type type, u16 reg);
    void store(u16 dest, u16 src);
    void load(Type type, u16 dest, u16 src);
    void label(u16 label);
    void blockStart();
    void blockEnd();
    void jmp(u16 label);
    void jmp(Bytecode op, u16 checkReg, u16 label);
    void mov(Type type, u16 dest, u16 src);
    void movConst(u16 reg, s64 num);
    void movConst(u16 outputReg, f64 num);
    void binOp(Bytecode op, Type type, u16 outputReg, u16 lhsReg, u16 rhsReg);
    void cast(Type finalType, u16 finalReg, Type type, u16 reg);
    void set(Bytecode op, u16 outputReg, u16 inputReg);
    void neg(Type type, u16 newReg, u16 reg);
    void ret(u16 reg, bool isVoid=false);
};
