#pragma once
#include "IDebugger.h"
#include <memory>
#include <string>

// DebugCore is the only object the UI layer should hold.
// It owns the backend (GDBDebugger or any future IDebugger impl)
// and provides a single high-level entry point for session management.

class DebugCore {
public:
    // Inject a pre-configured backend (for testing or custom backends).
    void setBackend(std::unique_ptr<IDebugger> backend);

    // Convenience: creates a GDBDebugger, calls start() → connectToTarget() → loadElf().
    // openocdConfig: path to the OpenOCD board/interface config file.
    bool startSession(const std::string& elfPath,
                      const std::string& openocdConfig,
                      const std::string& gdbPath = "arm-none-eabi-gdb",
                      const std::string& openocdPath = "openocd");

    void endSession();

    // Direct access to the backend for everything not in the facade.
    // Returns nullptr if no backend has been set.
    IDebugger* debugger();

    // Listener forwarding helpers.
    void addListener(IDebugEventListener* l);
    void removeListener(IDebugEventListener* l);

private:
    std::unique_ptr<IDebugger> backend_;
};
