#pragma once

#define PAGE_MEM_SIZE  1000

struct Page{
    char *mem;
    u32   watermark;
};
