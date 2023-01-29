#if(XE_DBG)
#define BLOCK_COUNT 120
namespace dbg{
	struct BlockTimes {
		char *blockName;
		f64 microSeconds;
		u8 depth;
	};

	u8 depth = 0;
	u16 blockOff = 0;
	BlockTimes blocks[BLOCK_COUNT];

	void registerBlockTime(char *blockName, f64 time, u8 depth) {
		BlockTimes rep;
		rep.blockName = blockName;
		rep.microSeconds = time;
		rep.depth = depth;
		blocks[blockOff] = rep;
		blockOff += 1;
	};
	void dumpBlockTimes() {
		const u8 dots = 40;
		printf("\n[TIMES]\n");
		for (u16 r=0; r<blockOff; r += 1) {
			BlockTimes rep = blocks[r];
			for (u8 x=1; x<rep.depth; x+=1) { printf("    "); };
			printf("%s", rep.blockName);
			for (u8 start = (u8)strlen(rep.blockName); start<dots; start += 1) { printf("."); };
			printf("%f\n", rep.microSeconds);
		};
		printf("----\n");
	}
};
#endif

#if(XE_DBG && XE_PLAT_WIN)
namespace dbg{
	u64 freq;
	void initTimer() {
		LARGE_INTEGER temp;
		QueryPerformanceFrequency(&temp);
		freq = temp.QuadPart;
	};

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
			f64 time = diff / (f64)freq;
			registerBlockTime(blockName, time, depth);
			depth -= 1;
		};
	private:
		LARGE_INTEGER start;
		char *blockName;
	};
};
#endif

#if(XE_DBG)
#define TIME_BLOCK_(name) dbg::Timer timer(name);
#define TIME_BLOCK dbg::Timer timer(__FUNCTION__);
#else
#define TIME_BLOCK_(name)
#define TIME_BLOCK
#endif