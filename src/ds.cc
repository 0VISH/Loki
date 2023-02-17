//TODO: optimize these data structures

//array whose length is known at compiletime
template<typename T, u32 len>
struct StaticArray {
	T mem[len];

	T& operator[](u32 index) {
#if(XE_DBG == true)
		if (index >= len) {
			printf("\n[ERROR]: abc(static_array) failed for type %s. index = %d\n", typeid(T).name(), index);
		};
#endif
		return mem[index];
	};
};

//array whose length is not known at comptime
template<typename T>
struct Array {
	T *mem;
	u32 len;

	T& operator[](u32 index) {
#if(XE_DBG == true)
		if (index >= len) {
			printf("\n[ERROR]: abc(array) failed for type %s. index = %d\n", typeid(T).name(), index);
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
			printf("\n[ERROR]: abc(string) failed. mem: %p, len: %d, index = %d\n", mem, len, index);
		};
#endif
		return mem[index];
	};
};
bool cmpString(String str1, String str2) {
	if (str1.len != str2.len) { return false; };
	return memcmp(str1.mem, str2.mem, str1.len) == 0;
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
	T &getElement(u32 index) {
#if(XE_DBG == true)
		if (index >= len) {
			printf("\n[ERROR]: abc(dynamic_array) failed for type %s. index = %d\n", typeid(T).name(), index);
		};
#endif
		return mem[index];
	};
	T &operator[](u32 index) { return getElement(index); };
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

//hash map
struct Map {
private:
	DynamicArray<u16> values;
	DynamicArray<String> keys;
	DynamicArray<b8>  empty;
public:
	void init(u32 count=5) {
		keys.init(count);
		values.init(count);
		empty.init(count);
		memset(empty.mem, true, sizeof(u8)*count);
	};
	void uninit() {
		keys.uninit();
		values.uninit();
		empty.uninit();
	};
	s32 insertValue(String str, u16 value) {
		if (empty.count == empty.len) { return -1; };
		u32 startHash = fnv_hash_1a_32(str.mem, str.len) % empty.len;
		u32 hash = startHash;
		while (empty[hash] == false) {
			hash += 1;
			if (hash >= empty.len) {
				hash = 0;
				continue;
			};
		}
		empty[hash] = false;
		values[hash] = value;
		keys[hash] = str;
		return hash;
	};
	s32 getValue(String str) {
		u32 startHash = fnv_hash_1a_32(str.mem, str.len) % empty.len;
		u32 hash = startHash;
		while (empty[hash] == false) {
			if (cmpString(str, keys[hash]) == true) { return values[hash]; };
			if (hash == startHash){ return -1; };
			hash += 1;
			if (hash == empty.len) { hash = 0; };
		};
		return -1;
	};
	bool isFull() { return empty.count == empty.len; };
private:
	u32 fnv_hash_1a_32(char *key, u32 len){
		u32 h = 0x811c9dc5;
		for(u32 i=0; i<len; i+=1){h = (h^key[i]) * 0x01000193;};
		return h;
	}
};