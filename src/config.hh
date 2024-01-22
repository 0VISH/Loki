#pragma once

#include "backend/target.hh"

enum class EndType{
    EXECUTABLE,
    STATIC,
    DYNAMIC,
    CHECK,
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
};
