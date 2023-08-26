#include <windows.h>

void doJob(TPool *pool);

namespace os{
    Timer times[TimeSlot::COUNT];
    u64 freq;
    
    char *getFileFullName(char *filePath) {
	char fullFilePathBuff[1024];
	u32 len = GetFullPathNameA(filePath, 1024, fullFilePathBuff, NULL);
	char *fullPath = (char*)mem::salloc(len);
	memcpy(fullPath, fullFilePathBuff, len + 1);
	return fullPath;
    };

    bool isFile(char *filePath){
	DWORD dwAttrib = GetFileAttributes(filePath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES && 
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
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
	times[(u16)slot].start = temp.QuadPart;
    };
    void endTimer(TimeSlot slot){
	LARGE_INTEGER temp;
	QueryPerformanceCounter(&temp);
	u64 diff = temp.QuadPart - times[(u16)slot].start;
	
	times[(u16)slot].time = diff / (f64)freq;
    };

    struct Thread{
	HANDLE hdle;
    };
    DWORD WINAPI WinThreadFunc(LPVOID lpParam){
	TPool *pool = (TPool*)lpParam;
	while(pool->alive){
	    if(pool->jobs.count != 0){
		doJob(pool);
	    }else{
		Sleep(100);      //@foodforthought: change time?
	    };
	};
	return 0;
    };
    void createThreads(TPool &pool){
	for(u32 x=0; x<pool.threads.len; x+=1){
	    Thread t;
	    t.hdle = CreateThread(NULL, 0, WinThreadFunc, &pool, 0, nullptr);
	    pool.threads[x] = t;
	};
    };
    void destroyThreads(TPool &pool){
	for(u32 x=0; x<pool.threads.len; x+=1){
	    CloseHandle(pool.threads[x].hdle);
	};
    };
}
