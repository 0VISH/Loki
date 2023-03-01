#if(XE_DBG && XE_PLAT_WIN)

#define EXCEPTION_BLOCK_START			\
    __try{					\

#define EXCEPTION_BLOCK_END			\
    } __except(EXCEPTION_EXECUTE_HANDLER) {	\
	printf("\n[EXCEPTION] ");		\
	auto code = GetExceptionCode();		\
	switch (code) {				\
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:	\
	case EXCEPTION_INT_DIVIDE_BY_ZERO:	\
	    printf("dividing by 0");		\
	    break;				\
	case EXCEPTION_ACCESS_VIOLATION:	\
	    printf("access violation");		\
	    break;				\
	case EXCEPTION_INT_OVERFLOW:		\
	    printf("int overflow");		\
	    break;				\
	};					\
      };					\

#else
#define EXCEPTION_BLOCK_START
#define EXCEPTION_BLOCK_END
#endif
