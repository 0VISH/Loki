namespace GlobalStrings{
    hashmap *globalStrings;
    u16 stringID;

    void init(){
	globalStrings = hashmap_create();
	stringID = 0;
    };
    void uninit(){
	hashmap_free(globalStrings);
    };
    u32 addEntryIfRequired(String str){
	uintptr_t id = stringID;
	if(hashmap_get_set(globalStrings, str.mem+1, str.len, &id)){
	    stringID += 1;
	};
	return id;
    };
};
