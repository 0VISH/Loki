#pragma once

#include "backend/target.hh"

enum class EndType{
    EXECUTABLE,
    STATIC,
    DYNAMIC,
    CHECK,
};
enum class Help{
    INITIALIZE_WITH_0,
};

struct Config{
    char *entryPoint;
    char *file;
    char *out;
    s32   entryPointProcID;
    s32   entryPointFileID;

    Arch arch;
    OS os;
    EndType end;
    u32 helps;
};
