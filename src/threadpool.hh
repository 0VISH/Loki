#pragma once

namespace os{
    struct Thread;
};

typedef int (*Work )(void*);
struct Job{
    Work work;
    void *argument;
};
struct TPool{
    Array<os::Thread> threads;
    DynamicArray<Job> jobs;
    std::mutex mut;
    bool alive;
};
