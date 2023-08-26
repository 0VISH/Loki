#define AST_PAGE_SIZE 1024

typedef s16 FileID;

struct ASTBase;
void freeNodeInternal(ASTBase *base);

struct ASTFile{
    DynamicArray<char*> memPages;
    DynamicArray<ASTBase*> nodes;
    DynamicArray<FileID> dependsOnMe;
    FileID id;
    u16 pageBrim;
    u16 IDependOn;

    void init(FileID fileID){
	id = fileID;
	pageBrim = 0;
	IDependOn = 0;
	memPages.init(2);
	char *page = (char*)mem::alloc(AST_PAGE_SIZE);
	memPages.push(page);
	nodes.init(10);
	dependsOnMe.init();
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
	dependsOnMe.uninit();
    };
};

namespace Dep{
    DynamicArray<ASTFile> files;
    hashmap *fileNameToID;
    FileID fileID;

    void init(){
	fileNameToID = hashmap_create();
	fileID = 1;
	files.init();
    };
    void uninit(){
	hashmap_free(fileNameToID);
	for(u32 x=0; x<files.count; x+=1){
	    files[x].uninit();
	};
	files.uninit();
    };

    FileID getFileID(char *fileName){
	uintptr_t temp = fileID;
	if(hashmap_get_set(fileNameToID, fileName, strlen(fileName), &temp)){
	    ASTFile &file = files.newElem();
	    file.init((FileID)temp);
	    fileID += 1;
	};
	return (FileID)temp;
    };
    ASTFile &getASTFile(FileID id){
	return files[(u32)id];
    };
};
