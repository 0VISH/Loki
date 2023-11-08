#pragma once

#include "../midend/bytecode.hh"

#if(WIN)
#define EXPORT extern "C" __declspec(dllexport)
#endif

#define PAGE_SIZE  1000

struct Config{
};
