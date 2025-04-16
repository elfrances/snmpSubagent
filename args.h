#pragma once

typedef struct CmdArgs {
    const char *configFile;
    bool daemon;
    const char *dataFile;
    bool syslog;
} CmdArgs;

