#pragma once

enum class Scope{
    GLOBAL,
    PROC,
    BLOCK,
};
struct ScopeEntities;
struct Entity{
    String name;
    Flag flag;
};
struct VariableEntity : Entity{
    u64 size;
    Type type;
};
struct ProcEntity : Entity{
    Flag flag;
};
struct StructEntity : Entity{
    Map varToOff;
    u64 size;
};
struct ScopeEntities{
    Map varMap;
    Map procMap;
    Map structMap;
    VariableEntity* varEntities;
    ProcEntity* procEntities;
    StructEntity *structEntities;
    Scope scope;

    void init(u32 varCount, u32 procCount, u32 structCount);
    void uninit();
};
struct ASTBase;
void freeNodeInternal(ASTBase *base);
