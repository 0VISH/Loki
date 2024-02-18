#include "entity.hh"
#include "../midend/bytecode.hh"

#define AST_PAGE_SIZE 1024

struct ASTBase;
struct IDGiver{
    u16 procID;
    u16 varID;
    u16 gblID;
    u16 structID;
    u16 fileID;
};
struct ASTFile{
    DynamicArray<char*> memPages;
    DynamicArray<ASTBase*> nodes;
    DynamicArray<u32> importFiles;
    IDGiver idGiver;
    ScopeEntities *entities;
    u16 pageBrim;

    void init(s16 fileID){
	idGiver.fileID = fileID;
	idGiver.procID = 0;
	idGiver.varID = 0;
	idGiver.gblID = 0;
	idGiver.structID = (u32)(Type::TYPE_COUNT) + 1;
	pageBrim = 0;
	memPages.init(2);
	char *page = (char*)mem::alloc(AST_PAGE_SIZE);
	memPages.push(page);
	nodes.init(10);
	importFiles.init();
    };
    void uninit(){
	for(u32 x=0; x<nodes.count; x += 1){
	    freeNodeInternal(nodes[x]);
	};
	for(u16 i=0; i<memPages.count; i++) {
	    mem::free(memPages[i]);
	};
	memPages.uninit();
	nodes.uninit();
	entities->uninit();
	mem::free(entities);
	importFiles.uninit();
    };
};

namespace Dep{
    //NOTE: 1-to-1 relation
    DynamicArray<char*>   fileStack;
    DynamicArray<ASTFile> astFiles;
    VariableContext* varContexts;
    //NOTE: this hashmaps exist in case we only the filename
    HashmapStr fileToId;

    void init(){
	fileStack.init();
	astFiles.init();
	fileToId.init();
    };
    void uninit(){
	fileStack.uninit();
	astFiles.uninit();
	fileToId.uninit();
    };
    void initVariableContext(){
	varContexts = (VariableContext*)mem::alloc(sizeof(VariableContext)*astFiles.count);
    };
    void uninitVariableContext(){
	for(u32 x=0; x<astFiles.count; x+=1){
	    varContexts[x].uninit();
	};
	mem::free(varContexts);
    };
    void pushToStack(char *fileName){
	fileStack.push(fileName);
    };
    ASTFile &newASTFile(){
	u32 id = astFiles.count;
	ASTFile &astFile = astFiles.newElem();
	astFile.init(id);
	return astFile;
    };
};
