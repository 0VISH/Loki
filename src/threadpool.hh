#pragma once

namespace os{
    struct Thread;
};

struct TPool{
    Array<os::Thread> threads;
};
