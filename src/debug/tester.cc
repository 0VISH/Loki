namespace tester{
    char buff[1024];
    DynamicArray<char*> files;
#if(PLAT_WIN)
    void getAllTestFiles() {
	files.init();
	WIN32_FIND_DATA data;
	HANDLE hFind = FindFirstFile("test/*.xe", &data);

	u32 off = 0;
	if ( hFind != INVALID_HANDLE_VALUE ) {
	    do {
		char *start = buff + off;
		files.push(start);
		u32 len = strlen(data.cFileName)+1;
		strcpy(start, "test/");
		off += strlen("test/");
		memcpy(buff+off, data.cFileName, len);
		off += len;
	    } while (FindNextFile(hFind, &data));
	    FindClose(hFind);
	}
    };
#endif
    bool runTests() {
	printf("\n****STARTING****\n");
	const u8 dots = 40;
	dbg::initTimer();
	initKeywords();
	DynamicArray<bool> stats;
	stats.init(files.count);
	for (u32 x = 0; x < files.count; x += 1) {
	    if (compile(files[x]) == false) { stats.push(false); }
	    else { stats.push(true); };
	    dbg::dumpBlockTimes();
	};
	uninitKeywords();
	printf("\n****DONE****\n");
	printf("\n[RESULTS]\n");
	for (u32 x=0; x<files.count; x+=1) {
	    printf("%s", files[x]);
	    for (u8 start = (u8)strlen(files[x]); start<dots; start += 1) { printf("."); };
	    printf("%s", stats[x]?"OK":"FAIL");
	};
	stats.uninit();
	files.uninit();
	return true;
    };
};
