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

    s16     entryPointID;
    EndType end;
};
