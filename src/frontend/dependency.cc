#include "entity.hh"

#define AST_PAGE_SIZE 1024

struct ASTBase;

struct ASTFile{
    DynamicArray<char*> memPages;
    DynamicArray<ASTBase*> nodes;
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
    };
};

namespace Dep{
    struct IDAndName{
	s16 id;
	char *name;
    };
    struct NodeScope{
	DynamicArray<ASTBase*> *nodes;
	ScopeEntities *se;
	s16 id;
    };
    
    DynamicArray<ASTFile> files;
    DynamicArray<ScopeEntities*> fileIDToSE;
    hashmap *fileNameToID;
    s16 fileID;

    DynamicArray<IDAndName> parseAndCheckStack;
    DynamicArray<NodeScope> compileStack;

    void init(){
	fileNameToID = hashmap_create();
	fileID = 0;
	files.init(3);
	parseAndCheckStack.init();
	compileStack.init();
	fileIDToSE.init();
    };
    void uninit(){
	hashmap_free(fileNameToID);
	for(u32 x=0; x<files.count; x+=1){
	    files[x].uninit();
	};
	files.uninit();
	parseAndCheckStack.uninit();
	compileStack.uninit();
	for(u32 x=0; x<fileIDToSE.count; x+=1){
	    ScopeEntities *se = fileIDToSE[x];
	    se->uninit();
	    mem::free(se);
	};
	fileIDToSE.uninit();
    };

    void registerScopedEntities(s16 id, ScopeEntities *se){
	if(id >= fileIDToSE.len){
	    fileIDToSE.realloc(id + 5);
	};
	fileIDToSE.count = (u32)id;
	fileIDToSE[(u32)id] = se;
    };
    s16 getFileID(char *fileName){
	uintptr_t temp = fileID;
	if(hashmap_get_set(fileNameToID, fileName, strlen(fileName), &temp) == false){
	    ASTFile &file = files.newElem();
	    file.init((s16)temp);
	    fileID += 1;
	};
	return (s16)temp;
    };
    ASTFile &getASTFile(s16 id){
	return files[id];
    };
    void pushToParseAndCheckStack(char *name){
	s16 fid = getFileID(name);
	parseAndCheckStack.push({fid, name});
    };
    void pushToCompileStack(DynamicArray<ASTBase*> *nodes, ScopeEntities *se, s16 id){
	compileStack.push({nodes, se, id});
    };
};
