//TODO: optimize these data structures

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
	u16 insertValue(String str, u16 value, b8 &inserted) {
		if (empty.count == empty.len) {
			inserted = false;
			return NULL;
		};
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
	u16 getValue(String str, b8 &found) {
		u32 startHash = fnv_hash_1a_32(str.mem, str.len) % empty.len;
		u32 hash = startHash;
		while (empty[hash] == false) {
			if (cmpString(str, keys[hash]) == true) { return hash; };
			hash += 1;
			if (hash == startHash){
				found = false;
				return NULL;
			};
			if (hash == empty.len) { hash = 0; };
		};
		found = false;
		return NULL;
	};
private:
	u32 fnv_hash_1a_32(char *key, u32 len){
		u32 h = 0x811c9dc5;
		for(u32 i=0; i<len; i+=1){h = (h^key[i]) * 0x01000193;};
		return h;
	}
};