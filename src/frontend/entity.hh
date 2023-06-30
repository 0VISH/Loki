#pragma once

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
    DynamicArray<Type> treeTypes;    //This stores the types returned by the tree which is being checked

    void init(u32 varCount, u32 procCount);
    void uninit();
};
