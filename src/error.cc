#define MAX_REPORTS 30

enum class ReportType : u8 {
	ERROR_REP,
};

struct LineOff {
	u32 line;
	u32 off;
};
struct Error {
	char *fileName;
	char *msg;
	LineOff lo;
	ReportType reportType;
};

Error errors[MAX_REPORTS];
u8 errorOff = 0;
char reportBuff[1024];
u32 reportBuffTop = 0;

void emitErr(char *fileName, LineOff lo, char *fmt, ...) {
	if (errorOff == MAX_REPORTS) { return; };
	Error &rep = errors[errorOff];
	errorOff += 1;
	rep.fileName = fileName;
	rep.lo = lo;
	rep.reportType = ReportType::ERROR_REP;
	if (fmt != nullptr) { rep.msg = reportBuff + reportBuffTop; };
	va_list args;
	va_start(args, fmt);
	reportBuffTop += vsprintf(reportBuff, fmt, args);
	va_end(args);
};
void flushReports() {
	if (errorOff == 0) { return; };
	u8 err = 0;
	//printf in FILO so that the usr can see first report first in the terminal
	for (u8 i = errorOff; i != 0;) {
		i -= 1;
		Error &rep = errors[i];
		switch (rep.reportType) {
			case ReportType::ERROR_REP:
				err += 1;
				printf("\n[ERROR]");
				break;
		};
		printf("\n%s(%d:%d) %s", rep.fileName, rep.lo.line, rep.lo.off, rep.msg);
	};
	printf("\n\nflushed %d error%c\n", err, (err == 1)?' ':'s');
};

#if(XE_DBG)
void debugUnreachable(char *file, u32 line) {
	printf("\n[ERROR] unreachable area reached: %s(%d)", file, line);
};
#define DEBUG_UNREACHABLE debugUnreachable(__FILE__, __LINE__);
#else
#define DEBUG_UNREACHABLE
#endif