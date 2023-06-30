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
struct ScopeEntities{
    Map varMap;
    Map procMap;
    VariableEntity* varEntities;
    ProcEntity* procEntities;
    Scope scope;

    void init(u32 varCount, u32 procCount);
    void uninit();
};
void freeNodeInternal(ASTBase *base);
