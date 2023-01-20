namespace mem {
	//TODO: write an allocator
	void *alloc(u64 size) {return malloc(size);};
	void free(void *mem) {
#if(XE_DBG)
		if (mem == nullptr) {
			printf("\n[ERROR]: trying to free a nullptr\n");
		};
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
