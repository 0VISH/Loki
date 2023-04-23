#define BYTECODE_PAGE_COUNT 100

enum class Bytecode : u16{
    NONE,             //internal
    NEXT_PAGE,        //internal
    CONST_INT,
    BYTECODE_COUNT,   //internal
};

struct BytecodeFile{
    DynamicArray<Bytecode*> bytecodePages;
    u16 pageBrim;

    void init(){
	bytecodePages.init(2);
	pageBrim = 0;
    };
    void uninit(){
	bytecodePages.uninit();
    };
    void emit(Bytecode bc){
	if(pageBrim >= BYTECODE_PAGE_COUNT){
	    newBytecodePage();
	};
	Bytecode *page = bytecodePages[bytecodePages.count-1];
	page[pageBrim] = bc;
	pageBrim += 1;
    };
    //encoding constant into bytecode page cause why not?
    void emitConstInt(s64 num){
	const u16 sizeReq = sizeof(s64) / sizeof(u16);
	if(pageBrim+sizeReq >= BYTECODE_PAGE_COUNT){
	    if(pageBrim+1 < BYTECODE_PAGE_COUNT){
		Bytecode *page = bytecodePages[bytecodePages.count-1];
		page[pageBrim] = Bytecode::NEXT_PAGE;
	    };
	    newBytecodePage();
	};
	Bytecode *page = bytecodePages[bytecodePages.count-1];
	s64 *mem = (s64*)(page + pageBrim);
	*mem = num;
	pageBrim += sizeReq;
    };
private:
    void newBytecodePage(){
	Bytecode *page = (Bytecode*)mem::alloc(sizeof(Bytecode) * BYTECODE_PAGE_COUNT);
	bytecodePages.push(page);
	pageBrim = 0;
    };
};

void CompileToBytecode(ASTBase *node, BytecodeFile &bf){
    ASTType type = node->type;
    switch(type){
    case ASTType::NUM_INTEGER:{
	
    }break;
    default:
	DEBUG_UNREACHABLE;
	break;
    };
};

#if(XE_DBG)
char *Bytecode2CString[] = {
    "NONE",
    "NEXT_PAGE",
    "CONST_INT",
};
static_assert((u16)Bytecode::BYTECODE_COUNT == ARRAY_LENGTH(Bytecode2CString), "Bytecode and Bytecode2CString not one-to-one");
#endif
