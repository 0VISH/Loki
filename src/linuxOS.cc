#include <time.h>

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

    void setPrintColorToWhite() {printf("\033[97m");};
    void printErrorInRed() {printf("\033[31mERROR\033[97m");};

    clock_t start;
    void compilerStartTimer(){
	start = clock();
    };
    f64 compilerEndTimer(){
	clock_t end = clock();
	return (f64)(end - start)/CLOCKS_PER_SEC;
    };
}
