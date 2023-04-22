static TPool pool;

void initTPool(u32 threadCount){
    pool.threads.len = threadCount;
    pool.threads.mem = (os::Thread*)mem::alloc(sizeof(os::Thread) * threadCount);
    os::createThreads(pool);
    pool.jobs.init(10);
    pool.mut.unlock();
    pool.alive = true;
};
void doJob(TPool *pool){
    while(pool->mut.try_lock() == false){};
    Job job = pool->jobs.pop();
    pool->mut.unlock();
    job.work(job.argument);
};
void uninitTPool(){
    while(pool.jobs.count != 0){
	doJob(&pool);
    };
    pool.alive = false;
    pool.jobs.uninit();
    os::destroyThreads(pool);
    mem::free(pool.threads.mem);
};
void addJob(Work work, void *arg){
    Job job;
    job.work = work;
    job.argument = arg;
    while(pool.mut.try_lock() == false){};
    pool.jobs.push(job);
    pool.mut.unlock();
};
