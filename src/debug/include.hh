#pragma once

#if(DBG)

#include "timer.cc"
#include "exception.cc"

#else

#define SEH_EXCEPTION_BLOCK_START
#define SEH_EXCEPTION_BLOCK_END

#define TIME_BLOCK_(name)
#define TIME_BLOCK

#endif
