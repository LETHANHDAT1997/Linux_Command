#pragma once
#include "Types.h"
#include "IDebugEventListener.h"
#include <string>
#include <vector>
#include <cstdint>

class IDebugger {
public:
    virtual ~IDebugger() = default;

    // ── Lifecycle ────────────────────────────────────────────────────────────
    // Required call order:
    //   start() → connectToTarget() → loadElf() → run() / halt()
    virtual bool start() = 0;
    virtual void stop()  = 0;

    virtual bool connectToTarget(const std::string& host = "localhost",
                                  int port = 3333) = 0;
    virtual bool loadElf(const std::string& elfPath) = 0;
    virtual void reset(bool halt = true) = 0;

    // ── Execution control ────────────────────────────────────────────────────
    virtual void run()      = 0;   // -exec-continue
    virtual void halt()     = 0;   // -exec-interrupt
    virtual void stepInto() = 0;   // -exec-step      (source line, into calls)
    virtual void stepOver() = 0;   // -exec-next      (source line, over calls)
    virtual void stepOut()  = 0;   // -exec-finish    (run until caller returns)

    // ── Breakpoints ──────────────────────────────────────────────────────────
    // These block until GDB confirms insertion and return the assigned id.
    virtual int  addBreakpoint(const std::string& file, int line) = 0;
    virtual int  addBreakpointAtAddress(uint32_t addr)            = 0;
    virtual void removeBreakpoint(int id)                          = 0;
    virtual void enableBreakpoint(int id, bool enable)             = 0;

    // ── Inspection ───────────────────────────────────────────────────────────
    // These block the calling thread until GDB replies.
    // Do NOT call from the UI thread or from any IDebugEventListener callback.
    virtual std::string            evaluate(const std::string& expr)     = 0;
    virtual std::vector<uint8_t>   readMemory(uint32_t addr, size_t len) = 0;
    virtual std::string            readRegisters()                        = 0;
    virtual std::vector<FrameInfo> backtrace()                            = 0;

    // ── State ────────────────────────────────────────────────────────────────
    virtual DebugState state() const = 0;

    // ── Event listeners (thread-safe) ────────────────────────────────────────
    virtual void addListener(IDebugEventListener* l)    = 0;
    virtual void removeListener(IDebugEventListener* l) = 0;
};
