enum class BackendType{
    LLVM,
};

typedef void(*backendCompileStage1Type)(BytecodeBucket *buc, s16 id, Config *config);
typedef bool(*backendCompileStage2Type)(Config *config);
typedef void(*initializeBackendType)();
typedef void(*startStopBackendType)();

struct BackendRef{
#if(WIN)
    HMODULE code;
#elif(LIN)
    void *code;
#endif
    BackendType type;
    backendCompileStage1Type backendCompileStage1;
    backendCompileStage2Type backendCompileStage2;
    startStopBackendType initBackend;
    startStopBackendType uninitBackend;
};

BackendRef loadRef(BackendType type){
    BackendRef ref;
    ref.type = type;
    switch(type){
    case BackendType::LLVM:{
#if(WIN)
#if(DBG)
	char *path = "bin/win/dbg/llvmBackend.dll";
#else
	char *path = "llvmBackend.dll";
#endif
	ref.code = LoadLibrary(path);
	if(!ref.code){
	    printf("\n[ERROR]: Couldn't find backend file %s", path);
	    return ref;
	};
	ref.initBackend = (startStopBackendType)GetProcAddress(ref.code, "initLLVMBackend");
	ref.uninitBackend = (startStopBackendType)GetProcAddress(ref.code, "uninitLLVMBackend");
	ref.backendCompileStage1 = (backendCompileStage1Type)GetProcAddress(ref.code, "backendCompileStage1");
	ref.backendCompileStage2 = (backendCompileStage2Type)GetProcAddress(ref.code, "backendCompileStage2");
#endif
    }break;
    };
    return ref;
};
void unloadRef(BackendRef &ref){
#if(WIN)
    FreeLibrary(ref.code);
#endif
};
