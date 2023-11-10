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
    JMPS,         //jumps if given register is not 0
    JMP,
    DEF,
    RET,
    NEG,
    LABEL,
    PROC_END,
    ALLOC,
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
    void emit(Type type);
    void emit(Reg reg);
    Bytecode *getCurBytecodeAdd();
    void emitConstInt(s64 num);
    void emitConstDec(f64 num);
    void alloc(Type type, Reg reg);
    void store(Type type, Reg dest, Reg src);
    void load(Type type, Reg dest, Reg src);
    void label(u16 label);
    void procEnd();
    void jmp(u16 label);
    void jmp(Bytecode op, Reg checkReg, u16 labelT, u16 labelF);
    void cmp(Bytecode op, Type type, Reg des, Reg lhs, Reg rhs);
    void mov(Type type, Reg dest, Reg src);
    void movConst(Reg reg, s64 num);
    void movConst(Reg outputReg, f64 num);
    void binOp(Bytecode op, Type type, Reg outputReg, Reg lhsReg, Reg rhsReg);
    void cast(Type finalType, Reg finalReg, Type type, Reg reg);
    void set(Bytecode op, Reg outputReg, Reg inputReg);
    void neg(Type type, Reg newReg, Reg reg);
    void ret(Reg reg, bool isVoid=false);
};
