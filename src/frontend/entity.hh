#pragma once

enum class Scope{
    GLOBAL,
    PROC,
    BLOCK,
};
struct ScopeEntities;
struct Entity{
    u32  id;
    Flag flag;
};
struct VariableEntity : Entity{
    u64 size;
    Type type;
};
struct ProcEntity : Entity{
    DynamicArray<Type> argTypes;
    DynamicArray<Type> retTypes;
    Flag flag;
    u32 id;
};
struct StructEntity : Entity{
    HashmapStr varToOff;
    u64 size;
};
struct ScopeEntities{
    HashmapStr varMap;
    HashmapStr procMap;
    HashmapStr structMap;
    VariableEntity* varEntities;
    ProcEntity* procEntities;
    StructEntity *structEntities;
    Scope scope;

    void init(u32 varCount, u32 procCount, u32 structCount);
    void uninit();
};
struct ASTBase;
void freeNodeInternal(ASTBase *base);
