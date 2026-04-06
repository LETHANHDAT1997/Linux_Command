#pragma once
#include "../IDebugger.h"
#include "../protocol/GDBMIChannel.h"
#include "../process/GDBProcess.h"
#include "../process/OpenOCDProcess.h"
#include "BreakpointManager.h"
#include "VariableManager.h"
#include "DebugStateMachine.h"
#include "EventDispatcher.h"
#include <string>

class GDBDebugger : public IDebugger {
public:
    // Must be called before start().
    void configure(std::string openocdConfig,
                   std::string gdbPath     = "arm-none-eabi-gdb",
                   std::string openocdPath = "openocd");

    // ── IDebugger ─────────────────────────────────────────────────────────────
    bool start() override;
    void stop()  override;

    bool connectToTarget(const std::string& host, int port) override;
    bool loadElf(const std::string& elfPath) override;
    void reset(bool halt) override;

    void run()      override;
    void halt()     override;
    void stepInto() override;
    void stepOver() override;
    void stepOut()  override;

    int  addBreakpoint(const std::string& file, int line) override;
    int  addBreakpointAtAddress(uint32_t addr)            override;
    void removeBreakpoint(int id)                          override;
    void enableBreakpoint(int id, bool enable)             override;

    std::string            evaluate(const std::string& expr)     override;
    std::vector<uint8_t>   readMemory(uint32_t addr, size_t len) override;
    std::string            readRegisters()                        override;
    std::vector<FrameInfo> backtrace()                            override;

    DebugState state() const override { return stateMachine_.state(); }

    void addListener(IDebugEventListener* l)    override { dispatcher_.add(l); }
    void removeListener(IDebugEventListener* l) override { dispatcher_.remove(l); }

private:
    // Called from the GDB reader thread.
    void onAsyncRecord(const MIRecord& rec);
    void onStreamOutput(std::string_view text);

    void handleStopped(const MIRecord& rec);
    FrameInfo parseFrame(const MIValue& frameVal);

    // Helper: sends a command and blocks up to `timeoutSec` seconds.
    // Returns true if the result class is "done" or "connected".
    bool sendBlocking(std::string_view cmd, int timeoutSec = 5);

    std::string openocdConfig_;
    std::string gdbPath_     {"arm-none-eabi-gdb"};
    std::string openocdPath_ {"openocd"};

    OpenOCDProcess    openocd_;
    GDBProcess        gdb_;
    GDBMIChannel      channel_;

    BreakpointManager breakpoints_;
    VariableManager   variables_;
    DebugStateMachine stateMachine_;
    EventDispatcher   dispatcher_;
};
