#include <windows.h>

namespace os{
    char *getFileFullName(char *filePath) {
	char fullFilePathBuff[1024];
	u32 len = GetFullPathNameA(filePath, 1024, fullFilePathBuff, NULL);
	char *fullPath = (char*)mem::salloc(len);
	memcpy(fullPath, fullFilePathBuff, len + 1);
	return fullPath;
    };

    void setPrintColorToWhite() {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN);
    };
    void printErrorInRed() {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, FOREGROUND_RED);
	printf("ERROR");
	SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN);
    };

    LARGE_INTEGER timeStart;
    void compilerStartTimer(){
	QueryPerformanceCounter(&timeStart);
    };
    f64 compilerEndTimer(){
	LARGE_INTEGER end;
	QueryPerformanceCounter(&end);
	u64 diff = end.QuadPart - timeStart.QuadPart;
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	f64 time = diff / (f64)freq.QuadPart;
	return time;
    };
}