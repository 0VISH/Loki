#include <windows.h>

namespace os{
    u64 freq;
    
    char *getFileFullName(char *filePath) {
	char fullFilePathBuff[1024];
	u32 len = GetFullPathNameA(filePath, 1024, fullFilePathBuff, NULL) + 1;
	char *fullPath = (char*)mem::salloc(len);
	memcpy(fullPath, fullFilePathBuff, len);
	return fullPath;
    };
    bool isFile(char *filePath){
	DWORD dwAttrib = GetFileAttributes(filePath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
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

    void initTimer(){
	memset(times, 0, sizeof(times)*(u16)TimeSlot::COUNT);
	LARGE_INTEGER fr;
	QueryPerformanceFrequency(&fr);
	freq = fr.QuadPart;
    };
    void startTimer(TimeSlot slot){
	LARGE_INTEGER temp;
	QueryPerformanceCounter(&temp);
	times[(u16)slot] = temp.QuadPart;
    };
    void endTimer(TimeSlot slot){
	LARGE_INTEGER temp;
	QueryPerformanceCounter(&temp);
	u64 diff = temp.QuadPart - times[(u16)slot];
	
	times[(u16)slot] = diff / (f64)freq;
    };
}
