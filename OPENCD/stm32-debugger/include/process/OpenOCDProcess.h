#pragma once
#include "ProcessHandle.h"
#include <string>
#include <chrono>

// Spawns openocd and waits until it is listening on the GDB port.
// Must be started (and waitReady()) BEFORE GDBProcess::start().

class OpenOCDProcess : public ProcessHandle {
public:
    bool start(const std::string& configFile,
               const std::string& openocdPath = "openocd",
               int gdbPort = 3333);

    // Polls TCP port until OpenOCD is accepting connections, or timeout expires.
    // Returns false on timeout.
    bool waitReady(std::chrono::milliseconds timeout = std::chrono::seconds{5});

private:
    int port_{3333};
};
