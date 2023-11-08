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

DynamicArray<Page> pages;

void initPage(Page &pg){
    pg.mem = (char*)mem::alloc(PAGE_MEM_SIZE);
    pg.watermark = 0;
};
void initBackend(){
    pages.init();
    Page &pg = pages.newElem();
    initPage(pg);
};
void uninitBackend(){
    for(u32 x=0; x<pages.count; x+=1){
	mem::free(pages[x].mem);
    };
    pages.uninit();
};

void write(char *fmt, ...){
 WRITE_START:
    Page &pg = pages[pages.count-1];
    va_list args;
    va_start(args, fmt);
    int res = vsnprintf(pg.mem+pg.watermark, PAGE_MEM_SIZE - pg.watermark, fmt, args);
    if(res < 0 || res >= PAGE_MEM_SIZE){
	Page &newPage = pages.newElem();
	initPage(newPage);
	goto WRITE_START;
    };
    pg.watermark += res;
    va_end(args);
};
void BackendDump(char *fileName){
    FILE *f = fopen(fileName, "w");
    for(u32 x=0; x<pages.count; x+=1){
	Page &pg = pages[x];
	fwrite(pg.mem, 1, pg.watermark, f);
    };
    fclose(f);
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
