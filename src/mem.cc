namespace mem {
#if(XE_DBG)
    u64 notFreed = 0;
    u32 calls = 0;
#endif
    //TODO: write an allocator
    void *alloc(u64 size) {
	void *mem;
#if(XE_DBG)
	if(size == 0){
	    printf("\n[ERROR]: trying to allocate memory of size 0");
	    return nullptr;
	};
	calls += 1;
	notFreed += size;
	mem = malloc(size + sizeof(u64));
	u64 *num = (u64*)mem;
	*num = size;
	mem = (char*)mem + sizeof(u64);
#endif
	return mem;
    };
    void free(void *mem) {
#if(XE_DBG)
	if (mem == nullptr) {
	    printf("\n[ERROR]: trying to free a nullptr\n");
	};
	calls -= 1;
	u64 *num = (u64*)((char*)mem - sizeof(u64));
	notFreed -= *num;
	mem = num;
#endif
	::free(mem);
    };

    //stack based allocator. Contents stay alive till the compiler exits
    char sMem[1024];
    char *sTop = sMem;
    
    void *salloc(u64 size) {
	void *mem = (void*)sTop;
	sTop += size;
	return mem;
    };
    void sfree() {sTop = (char*)sMem;};
};
