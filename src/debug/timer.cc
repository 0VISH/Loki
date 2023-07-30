#define BLOCK_COUNT 120
namespace dbg{
    struct BlockTime {
	char *blockName;
	f64 microSeconds;
	u8 depth;
    };

    u8 depth = 0;
    u16 blockOff = 0;
    BlockTime blocks[BLOCK_COUNT];

    void registerBlockTime(char *blockName, f64 time, u8 depth) {
	BlockTime rep;
	rep.blockName = blockName;
	rep.microSeconds = time;
	rep.depth = depth;
	blocks[blockOff] = rep;
	blockOff += 1;
    };
    void dumpBlockTimes() {
	printf("\n[TIMES]\n");
	const u8 dots = 40;
	for (u16 r=0; r<blockOff; r += 1) {
	    BlockTime rep = blocks[r];
	    for (u8 x=0; x<rep.depth; x+=1) { printf("    "); };
	    printf("%s", rep.blockName);
	    for (u8 start = (u8)strlen(rep.blockName)+(rep.depth*4); start<dots; start += 1) { printf("."); };
	    printf("%f\n", rep.microSeconds);
	};
	printf("----\n");
    }
};

#if(PLAT_WIN)
namespace dbg{
    struct Timer{
	Timer(char *name) {
	    dbg::depth += 1;
	    blockName = name;
	    QueryPerformanceCounter(&start);
	};
	~Timer() {
	    LARGE_INTEGER end;
	    QueryPerformanceCounter(&end);
	    u64 diff = end.QuadPart - start.QuadPart;
	    diff *= 1000000;
	    LARGE_INTEGER freq;
	    QueryPerformanceFrequency(&freq);
	    f64 time = diff / (f64)freq.QuadPart;
	    dbg::depth -= 1;
	    registerBlockTime(blockName, time, dbg::depth);
	};
    private:
	LARGE_INTEGER start;
	char *blockName;
    };
};
#endif

#define TIME_BLOCK_(name) dbg::Timer timer(name);
#define TIME_BLOCK dbg::Timer timer(__FUNCTION__);
