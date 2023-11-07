#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace os{
    char *getFileFullName(char *filePath) {
	char fullFilePathBuff[1024];
	char *fullpath = realpath(filePath, fullFilePathBuff);
	if(fullpath == nullptr){return nullptr;};
	u32 len = strlen(fullpath);
	char *fullPath = (char*)mem::salloc(len);
	memcpy(fullPath, fullFilePathBuff, len + 1);
	return fullPath;
    };
    bool isFile(char *filePath){
	struct stat pathStat;
	if(stat(filePath, &pathStat) != 0){return false;};
	return S_ISREG(pathStat.st_mode);
    };

    void setPrintColorToWhite() {printf("\033[97m");};
    void printErrorInRed() {printf("\033[31mERROR\033[97m");};
    
    void initTimer(){};
    void startTimer(TimeSlot slot){
	times[(u16)slot] = (u64)clock();
    };
    void endTimer(TimeSlot slot){
	clock_t end = clock();
	times[(u16)slot] = (f64)(end - times[(u16)slot])/CLOCKS_PER_SEC;
    };
}
