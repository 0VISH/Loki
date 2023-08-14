hashmap *globalStrings;
u16 stringID;

void initGlobalStrings(){
    globalStrings = hashmap_create();
    stringID = 0;
};
void uninitGlobalStrings(){
    hashmap_free(globalStrings);
};
u32 addEntryIfRequired(String str){
    uintptr_t id = stringID;
    if(hashmap_get_set(globalStrings, str.mem+1, str.len, &id)){
	//NOTE: is this correct? should we check for == false?
	stringID += 1;
    };
    return id;
};
