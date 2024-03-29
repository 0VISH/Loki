namespace mem {
    u32 calls = 0;

    //TODO: write an allocator
    void *alloc(u64 size) {
	void *mem = malloc(size);
#if(DBG)	
	if(size == 0){
	    printf("\n[ERROR]: trying to allocate memory of size 0");
	    return nullptr;
	};
	calls += 1;
#endif
	return mem;
    };
    void free(void *mem) {
#if(DBG)
	if (mem == nullptr) {
	    printf("\n[ERROR]: trying to free a nullptr\n");
	    return;
	};
	calls -= 1;
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
