#include "include.hh"

s32 main(s32 argc, char **argv) {
    if (argc < 2) {
	printf("main file path not provided");
	return EXIT_SUCCESS;
    };
    EXCEPTION_BLOCK_START
	if (strcmp(argv[1], "test") == 0) {
	    tester::getAllTestFiles();
	    tester::runTests();
	    return EXIT_SUCCESS;
	};
    dbg::initTimer();
    initKeywords();
    compile(argv[1]);
    uninitKeywords();
    dbg::dumpBlockTimes();
    EXCEPTION_BLOCK_END
#if(XE_DBG)
	printf("\ndone!");
#endif
    return EXIT_SUCCESS;
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
