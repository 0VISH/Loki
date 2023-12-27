//@ignore
#if(__clang__)
#pragma clang diagnostic ignored "-Wwritable-strings"
#pragma clang diagnostic ignored "-Wswitch"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wmicrosoft-include"
#pragma clang diagnostic ignored "-Wmicrosoft-goto"
#pragma clang diagnostic ignored "-Wswitch"
#pragma clang diagnostic ignored "-Wnull-arithmetic"
#pragma clang diagnostic ignored "-Wformat"
#pragma clang diagnostic ignored "-Wnull-conversion"
#endif

#include "basic.hh"
#include "config.hh"
Config config;
#include "include.hh"

enum class ArgType{
    ENTRYPOINT,
    FILE,
    OUTNAME,
    END,
    HELP,
    COUNT,
};
struct ArgData{
    char *arg;
    ArgType type;
    char *help;
};

u16 getSwitchLen(char *arg, u32 argLen){
    u32 off = 0;
    while(off < argLen){
	if(arg[off] == ':'){break;};
	off += 1;
    };
    return off;
};

s32 main(s32 argc, char **argv) {
    SEH_EXCEPTION_BLOCK_START;

    ArgData argsData[] = {
	{"entrypoint", ArgType::ENTRYPOINT, "execution begins from this function"},
	{"file", ArgType::FILE, "main file(file which is read first by the compiler)"},
	{"out", ArgType::OUTNAME, "name of output file"},
	{"end", ArgType::END, "end goal\n             1: exe\n             2: dll\n             3: check"},
	{"help", ArgType::HELP, "print all the switches available"}
    };

    
    if(argc < 2) {
    PRINT_HELP_AND_QUIT:
	printf("----------switches----------\n");
	for(u32 x=0; x<(u16)ArgType::COUNT; x+=1){
	    printf("%s: %s\n", argsData[x].arg, argsData[x].help);
	};
	return EXIT_SUCCESS;
    };
    
    os::initTimer();
    os::startTimer(TimeSlot::TOTAL);

    config.entryPoint   = "main";
    config.file         = "main.loki";
    config.out          = "out";
    config.end          = EndType::EXECUTABLE;
    
    HashmapStr argMap;
    argMap.init((u16)ArgType::COUNT);
    for(u32 i=0; i<(u16)ArgType::COUNT; i+=1){
	argMap.insertValue({(char*)argsData[i].arg, (u32)strlen(argsData[i].arg) }, (u16)argsData[i].type);
    };

    
    for(u32 x=1; x<argc; x+=1){
	char *arg = argv[x];
	u32 argLen = strlen(arg);
	u32 len = getSwitchLen(arg, argLen);
	
	u32 type;
	if(argMap.getValue({arg, len}, &type) == false){
	    printf("unkown arg: %.*s", len, arg);
	    argMap.uninit();
	    return EXIT_SUCCESS;
	};
	switch((ArgType)type){
	case ArgType::HELP:{
	    goto PRINT_HELP_AND_QUIT;
	}break;
	case ArgType::ENTRYPOINT:{
	    config.entryPoint = arg + len + 1;
	}break;
	case ArgType::FILE:{
	    config.file = arg + len + 1;
	}break;
	case ArgType::OUTNAME:{
	    config.out = arg + len + 1;
	}break;
	case ArgType::END:{
	    char *end = arg + len + 1;
	    if(strcmp("executable", end) == 0){
		config.end = EndType::EXECUTABLE;
	    }else if(strcmp("dynamic", end) == 0){
		config.end = EndType::DYNAMIC;
	    }else if(strcmp("static", end) == 0){
		config.end = EndType::STATIC;
	    }else if(strcmp("check", end) == 0){
		config.end = EndType::CHECK;
	    }else{
		printf("unkown end goal: %s", end);
		argMap.uninit();
		return EXIT_SUCCESS;
	    };
	}break;
	};
    };
    argMap.uninit();

    if(os::isFile(config.file) == false){
	printf("invalid file path: %s", config.file);
	return EXIT_SUCCESS;
    };

    Word::init(Word::keywords, Word::keywordsData, Word::keywordCount);
    Word::init(Word::poundwords, Word::poundwordsData, Word::poundwordCount);
    GlobalStrings::init();
    Dep::init();
    compile(config.file);
    Dep::uninit();
    GlobalStrings::uninit();
    Word::uninit(Word::poundwords);
    Word::uninit(Word::keywords);

    os::endTimer(TimeSlot::TOTAL);
    if(report::errorOff == 0){printf("\n");};
    dumpTimers(times);
#if(DBG)
    printf("\nCALLS NOT FREED: %d\n", mem::calls);
#endif

    printf("\nDONE :)\n");
    return EXIT_SUCCESS;
    
    SEH_EXCEPTION_BLOCK_END;
    printf("LKSDJFKLSDJF");
};


//@ignore
static_assert(sizeof(u8) == 1, "u8 is not 1 byte");
static_assert(sizeof(u16) == 2, "u16 is not 2 byte");
static_assert(sizeof(u32) == 4, "u32 is not 4 byte");
static_assert(sizeof(u64) == 8, "u64 is not 8 byte");
static_assert(sizeof(s8) == 1, "s8 is not 1 byte");
static_assert(sizeof(s16) == 2, "s16 is not 2 byte");
static_assert(sizeof(s32) == 4, "s32 is not 4 byte");
static_assert(sizeof(s64) == 8, "s64 is not 8 byte");
static_assert(sizeof(b8) == 1, "b8 is not 1 byte");
static_assert(sizeof(b16) == 2, "b16 is not 2 byte");
static_assert(sizeof(b32) == 4, "b32 is not 4 byte");
static_assert(sizeof(b64) == 8, "b64 is not 8 byte");
static_assert(sizeof(f32) == 4, "f32 is not 4 byte");
static_assert(sizeof(f64) == 8, "f64 is not 8 byte");
