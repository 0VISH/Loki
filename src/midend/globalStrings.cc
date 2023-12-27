namespace GlobalStrings{
    HashmapStr globalStrings;
    u16 stringID;

    void init(){
	globalStrings.init();
	stringID = 0;
    };
    void uninit(){
	globalStrings.uninit();
    };
    u32 addEntryIfRequired(String str){
	u32 temp = stringID;
	if(globalStrings.getValue(str, &temp) == false){
	    globalStrings.insertValue(str, stringID);
	    stringID += 1;
	};
	return temp;
    };
};
