#pragma once
#include "ProcessHandle.h"
#include <string>

// Spawns arm-none-eabi-gdb (or a custom GDB binary) with --interpreter=mi.
// The MI interpreter flag is always added automatically.

class GDBProcess : public ProcessHandle {
public:
    // gdbPath: binary name on PATH or absolute path.
    bool start(const std::string& gdbPath = "arm-none-eabi-gdb");
};
