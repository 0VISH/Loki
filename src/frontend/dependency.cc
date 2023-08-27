#include "entity.hh"

#define AST_PAGE_SIZE 1024

typedef s16 FileID;

struct ASTBase;

struct ASTFile{
    DynamicArray<char*> memPages;
    DynamicArray<ASTBase*> nodes;
    FileID id;
    u16 pageBrim;

    void init(FileID fileID){
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
	FileID id;
	char *name;
    };
    struct NodeAndScope{
	DynamicArray<ASTBase*> *nodes;
	ScopeEntities *se;
    };
    
    DynamicArray<ASTFile> files;
    DynamicArray<ScopeEntities*> fileIDToSE;
    hashmap *fileNameToID;
    FileID fileID;

    DynamicArray<IDAndName> parseAndCheckQueue;
    DynamicArray<NodeAndScope> compileToBytecodeQueue;

    void init(){
	fileNameToID = hashmap_create();
	fileID = 1;
	files.init(3);
	files.count += 1;        //0 -> main file scope
	parseAndCheckQueue.init();
	compileToBytecodeQueue.init();
	fileIDToSE.init();
    };
    void uninit(){
	hashmap_free(fileNameToID);
	for(u32 x=0; x<files.count; x+=1){
	    files[x].uninit();
	};
	files.uninit();
	parseAndCheckQueue.uninit();
	compileToBytecodeQueue.uninit();
	for(u32 x=0; x<fileIDToSE.count; x+=1){
	    ScopeEntities *se = fileIDToSE[x];
	    se->uninit();
	    mem::free(se);
	};
	fileIDToSE.uninit();
    };

    void registerScopedEntities(FileID id, ScopeEntities *se){
	if(id >= fileIDToSE.len){
	    fileIDToSE.realloc(id + 5);
	};
	fileIDToSE.count = (u32)id;
	fileIDToSE[(u32)id] = se;
    };
    FileID getFileID(char *fileName){
	uintptr_t temp = fileID;
	if(hashmap_get_set(fileNameToID, fileName, strlen(fileName), &temp) == false){
	    ASTFile &file = files.newElem();
	    file.init((FileID)temp);
	    fileID += 1;
	};
	return (FileID)temp;
    };
    ASTFile &getASTFile(FileID id){
	return files[(u32)id];
    };
    void pushToParseAndCheckQueue(char *name){
	FileID fid = getFileID(name);
	parseAndCheckQueue.push({fid, name});
    };
    void pushToCompileQueue(DynamicArray<ASTBase*> *nodes, ScopeEntities *se){
	compileToBytecodeQueue.push({nodes, se});
    };
};
