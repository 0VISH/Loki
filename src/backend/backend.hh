#pragma once

#include "../midend/bytecode.hh"
#include "../config.hh"

#if(WIN)
#define EXPORT extern "C" __declspec(dllexport)
#elif(LIN)
#define EXPORT extern "C"
#endif

#define PAGE_MEM_SIZE  1000

struct Page{
    char *mem;
    u32   watermark;
};
