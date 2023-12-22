#pragma once

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
    s32   entryPointID;

    EndType end;
};
