#if(WIN && _MSC_VER)

#define SEH_EXCEPTION_BLOCK_START		\
    __try{					\

#define SEH_EXCEPTION_BLOCK_END			\
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
	return EXIT_FAILURE;			\
      };					\

#elif(LIN)

#include <signal.h>
#include <setjmp.h>

void segFault(int nSig,int nErrType,int */*pnReglist*/){
    printf("\n[EXCEPTION] access violation");
};
void illInstr(int nSig,int nErrType,int */*pnReglist*/){
    printf("\n[EXCEPTION] illegal instruction");
};
void divZero(int nSig,int nErrType,int */*pnReglist*/){
    printf("\n[EXCEPTION] division by 0");
};

#define SEH_EXCEPTION_BLOCK_START		\
    signal(SIGFPE,  (__sighandler_t)divZero);	\
    signal(SIGILL,  (__sighandler_t)illInstr);	\
    signal(SIGSEGV, (__sighandler_t)segFault);	\

#define SEH_EXCEPTION_BLOCK_END

#endif
