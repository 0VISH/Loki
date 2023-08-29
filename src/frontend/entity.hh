#pragma once

enum class Scope{
    GLOBAL,
    PROC,
    BLOCK,
};
struct ScopeEntities;
struct Entity{
    String name;
    u8 flag;
};
struct VariableEntity : Entity{
    Type type;
};
struct ProcEntity : Entity{
    ScopeEntities *se;
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
