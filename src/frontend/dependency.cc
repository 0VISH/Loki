#include "entity.hh"

#define AST_PAGE_SIZE 1024

struct ASTBase;
struct IDGiver{
    u32 procID;
    u32 varID;
    u32 structID;
};
struct ASTFile{
    DynamicArray<char*> memPages;
    DynamicArray<ASTBase*> nodes;
    IDGiver idGiver;
    ScopeEntities *entities;
    s16 id;
    u16 pageBrim;

    void init(s16 fileID){
	id = fileID;
	idGiver.procID = 0;
	idGiver.varID = 0;
	idGiver.structID = (u32)(Type::TYPE_COUNT) + 1;
	pageBrim = 0;
	memPages.init(2);
	char *page = (char*)mem::alloc(AST_PAGE_SIZE);
	memPages.push(page);
	nodes.init(10);
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
    };
};

namespace Dep{
    //NOTE: 1-to-1 relation
    DynamicArray<char*>   fileStack;
    DynamicArray<ASTFile> astFiles;
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
