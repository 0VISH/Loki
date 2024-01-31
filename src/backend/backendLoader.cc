enum class BackendType{
    LLVM,
};

typedef void(*backendCompileStage1Type)(BytecodeBucket *buc, s16 id, Config *config);
typedef bool(*backendCompileStage2Type)(Config *config);
typedef void(*initializeBackendType)();
typedef void(*startStopBackendType)();

#if(WIN)
#if(DBG)
char *path = "bin/win/dbg/llvmBackend.dll";
#else
char *path = "llvmBackend.dll";
#endif
#define LOAD_LIB LoadLibrary(path)
#define GET_PROC GetProcAddress

#elif(LIN)
#include <dlfcn.h>
#if(DBG)
char *path = "bin/lin/dbg/llvmBackend";
#else
char *path = "llvmBackend";
#endif
#define LOAD_LIB dlopen(path, RTLD_LAZY)
#define GET_PROC dlsym

#endif

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
	ref.code = LOAD_LIB;
	if(!ref.code){
	    printf("\n[ERROR]: Couldn't find backend file %s", path);
	    return ref;
	};
	ref.initBackend = (startStopBackendType)GET_PROC(ref.code, "initLLVMBackend");
	ref.uninitBackend = (startStopBackendType)GET_PROC(ref.code, "uninitLLVMBackend");
	ref.backendCompileStage1 = (backendCompileStage1Type)GET_PROC(ref.code, "backendCompileStage1");
	ref.backendCompileStage2 = (backendCompileStage2Type)GET_PROC(ref.code, "backendCompileStage2");
    }break;
    };
    return ref;
};
void unloadRef(BackendRef &ref){
#if(WIN)
    FreeLibrary(ref.code);
#elif(LIN)
    dlclose(ref.code);
#endif
};
