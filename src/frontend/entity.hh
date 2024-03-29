#pragma once

enum class Scope{
    GLOBAL,
    PROC,
    BLOCK,
};
struct ScopeEntities;
struct Entity{
    u16  id;
    u16  fileID;
    Flag flag;
};
struct VariableEntity : Entity{
    u64 size;
    Type type;
    u8   pointerDepth;
};
struct AST_Type;
struct ProcEntity : Entity{
    DynamicArray<AST_Type*> argTypes;
    DynamicArray<AST_Type*> retTypes;
    Flag flag;
    u32 id;
};
struct StructEntity : Entity{
    HashmapStr varToOff;
    Type *varToType;
};
struct ScopeEntities{
    HashmapStr         varMap;
    HashmapStr         procMap;
    HashmapStr         structMap;
    Hashmap<u32, u32>  structIDMap;
    VariableEntity    *varEntities;
    ProcEntity        *procEntities;
    StructEntity      *structEntities;
    Scope              scope;

    void init(u32 varCount, u32 procCount, u32 structCount);
    void uninit();
};
struct ASTBase;
void freeNodeInternal(ASTBase *base);
