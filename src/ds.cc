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
    //TODO: SIMD??
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
	if (count == len) { realloc(len + len / 2 + 1); };
	mem[count] = t;
	count += 1;
    };
    T pop(){
	count -= 1;
	return mem[count];
    };
    T& newElem(){
	if (count == len) { realloc(len + len / 2 + 1); };
	count += 1;
	return mem[count-1];
    };
    void reserve(u32 rCount){
	if(count+rCount >= len){
	    realloc(count+rCount);
	}
    }
#if(XE_DBG)
    void dumpStat() {
	printf("\n[DYNAMIC_ARRAY] mem: %p; count: %d; len: %d\n", mem, count, len);
    };
#endif
};

//hash map
struct Map{
public:
    s32 insertValue(String str, u16 value){
	if(isFull()){return -1;};
	u32 hash = hashFunc(str) % len;
	while(status[hash] == true){
	    hash += 1;   //TODO: check if reprobing by someother value than 1 is faster
	    if(hash >= len){hash = 0;};
	};
	status[hash] = true;
	count += 1;
	keys[hash] = str;
	values[hash] = value;
	return hash;
    };
    s32 getValue(String str){
	u32 startHash = hashFunc(str) % len;
	u32 hash = startHash;
	while(status[hash] == true){
	    if(cmpString(str, keys[hash]) == true) { return values[hash]; };
	    hash += 1;
	    if(hash >= len){hash = 0;};
	    if (hash == startHash){ return -1; };
	};
	return -1;
    };
    void init(u32 length) {
	len = length;
	count = 0;
	char *mem = (char*)mem::alloc(length * (sizeof(String)+sizeof(u16)+sizeof(u8)));
	keys = (String*)mem;
	values = (u16*) ((char*)keys   + (length*sizeof(String)));
	status = (bool*)  ((char*)values + (length*sizeof(u16)));
	memset(status, false, sizeof(u8)*length);
    };
    void uninit() {
	mem::free(keys);
    };
    bool isFull() { return count == len; };
#if(XE_DBG)
    void dumpMap(){
	for(u32 x=0; x<len; x+=1){
	    printf("%d: ", x);
	    if(status[x] == false){printf("---\n", x);}
	    else{
		printf("\n  KEY: %.*s", keys[x].len, keys[x].mem);
		printf("\n  VAL: %d\n", values[x]);
	    };
	};
    };
#endif
public:
    String *keys;
    u16 *values;
    bool *status;
    u32 count;
    u32 len;
    u32 hashFunc(String &str){
	//fnv_hash_1a_32
	char *key = str.mem;
	u32 len = str.len;
	u32 h = 0x811c9dc5;
	for(u32 i=0; i<len; i+=1){h = (h^key[i]) * 0x01000193;};
	return h;
    }
};
