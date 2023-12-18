#include "entity.hh"

#define AST_PAGE_SIZE 1024

struct ASTBase;

struct ASTFile{
    DynamicArray<char*> memPages;
    DynamicArray<ASTBase*> nodes;
    ScopeEntities *scope;
    s16 id;
    u16 pageBrim;

    void init(s16 fileID){
	id = fileID;
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
	scope->uninit();
	mem::free(scope);
    };
};
