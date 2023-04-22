void init(u32 threadCount, TPool &pool){
    pool.threads.len = threadCount;
    pool.threads.mem = (os::Thread*)mem::alloc(sizeof(os::Thread) * threadCount);
};
