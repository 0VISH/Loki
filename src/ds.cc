//array
template<typename T, u32 len>
struct Array {
	T mem[len];

	T& operator[](u32 index) {
#if(XE_DBG == true)
		if (index >= len) {
			printf("\n[ERROR]: abc(array) failed for type %s\n", typeid(T).name());
		};
#endif
		return mem[index];
	};
};

//string
struct String {
	char *mem;
	u32 len;

	char operator[](u32 index) {
#if(XE_DBG == true)
		if (index >= len) {
			printf("\n[ERROR]: abc(string) failed. mem: %p and len: %d\n", mem, len);
		};
#endif
		return mem[index];
	};
};

//dynamic array
template<typename T>
struct DynamicArray {
	T *mem;
	u32 count;
	u32 len;

	void realloc(u32 newCap) {
		void *newMem = mem::alloc(sizeof(T) * newCap);
		memcpy(newMem, mem, sizeof(T) * len);
		mem::free(mem);
		mem = (T*)newMem;
		len = newCap;
	};
	T &operator[](u32 index) {
#if(XE_DBG == true)
		if (index >= len) {
			printf("\n[ERROR]: abc(dynamic array) failed for type %s\n", typeid(T).name());
		};
#endif
		return mem[index];
	};
	void init(u32 startCount = 5) {
		count = 0;
		len = startCount;
		mem = (T*)mem::alloc(sizeof(T) * startCount);
	};
	void uninit() { mem::free(mem); };
	void push(const T &t) {
		if (count == len) { realloc(len + len / 2); };
		mem[count] = t;
		count += 1;
	};
#if(XE_DBG)
	void dumpStat() {
		printf("\n[DYNAMIC_ARRAY] mem: %p; count: %d; len: %d\n", mem, count, len);
	};
#endif
};
