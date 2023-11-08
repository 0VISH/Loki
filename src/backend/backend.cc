#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "../basic.hh"
/*
#include "../mem.cc"
#include "../ds.cc"
*/
#include "backend.hh"

DynamicArray<char*> pages;
DynamicArray<u32>   watermarks;

void initBackend(){
    pages.init();
    watermarks.init();
    pages.push((char*)mem::alloc(PAGE_SIZE));
    watermarks.push(0);
};
void uninitBackend(){
    for(u32 x=0; x<pages.count; x+=1){
	mem::free(pages[x]);
    };
    pages.uninit();
    watermarks.uninit();
};

void write(char *fmt, ...){
 WRITE_START:
    char *curPage = pages[pages.count-1];
    u32  &watermark = watermarks[watermarks.count-1];
    va_list args;
    va_start(args, fmt);
    int res = vsnprintf(curPage+watermark, PAGE_SIZE - watermark, fmt, args);
    if(res < 0 || res >= PAGE_SIZE){
	char *pg = (char*)mem::alloc(PAGE_SIZE);
	pages.push(pg);
	watermarks.push(0);
	goto WRITE_START;
    };
    watermark += res;
    va_end(args);
};
s64 getConstIntAndUpdate(Bytecode *page, u32 &x){
	s64 *mem = (s64*)(page + x);
	x += const_in_stream;
	return *mem;
};
f64 getConstDecAndUpdate(Bytecode *page, u32 &x){
    f64 *mem = (f64*)(page + x);
    x += const_in_stream;
    return *mem;
};
Bytecode *getPointerAndUpdate(Bytecode *page, u32 &x){
    Bytecode *mem = (Bytecode*)(page + x);
    x += pointer_in_stream;
    return mem;
};
inline Bytecode getBytecode(Bytecode *bc, u32 &x){
    return bc[x++];
};
