#define MAX_REPORTS 30

namespace report{

	struct Error {
		u64   off;
		char *fileName;
		char *msg;
		char *fileContent;
	};

	Error errors[MAX_REPORTS];
	u8 errorOff = 0;
	char reportBuff[1024];
	u32 reportBuffTop = 0;

	char *getLineAndOff(char *mem, u64 offset, u32 &line, u32 &off) {
		for (u64 i = 0; i < offset; i += 1) {
			if (mem[i] == '\n') { line += 1; };
		};
		while (mem[offset-off] != '\n') {
			off += 1;
		};
		return mem + offset-off+1;
	};

	void flushReports() {
		if (errorOff == 0) { return; };
		u8 err = 0;
		//printf in FILO so that the usr can see first report first in the terminal
		for (u8 i = errorOff; i != 0;) {
			i -= 1;
			Error &rep = errors[i];
			u32 line = 1;
			u32 off = 1;
			char *beg = getLineAndOff(rep.fileContent, rep.off, line, off);
			u32 x = 0;
			while (beg[x] != '\n' && beg[x] != '\0') {
				if (beg[x] == '\t') { beg[x] = ' '; }; //replace tabs with spaces for ez reporting and reading
				x += 1;
			};
			beg[x] = '\0';
			printf("\n%s(%d:%d) Error: %s\n", rep.fileName, line, off, rep.msg);
			printf("    %s\n____", beg);
			while (off != 1) {
				printf("_");
				off -= 1;
			};
			printf("^\n");
		};
		printf("\n\nflushed %d error%c\n", err, (err == 1)?' ':'s');
	};
}