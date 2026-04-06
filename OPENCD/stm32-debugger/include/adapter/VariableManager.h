#pragma once
#include "../Types.h"
#include "../protocol/GDBMIChannel.h"
#include "../protocol/MIRecord.h"
#include <string>
#include <vector>
#include <cstdint>

// Wraps GDB inspection commands (evaluate, memory, registers, backtrace)
// with a synchronous (blocking) API using std::promise/future.
//
// Callers MUST NOT invoke these from the GDB reader thread or from any
// IDebugEventListener callback — that would deadlock.

class VariableManager 
{
public:
    void attach(GDBMIChannel& ch);

    std::string            evaluate(const std::string& expr);
    std::vector<uint8_t>   readMemory(uint32_t addr, size_t len);
    std::string            readRegisters();
    std::vector<FrameInfo> backtrace();

private:
    // Sends a command and blocks until GDB replies (or 5 s timeout).
    MIRecord sendSync(std::string_view cmd);

    GDBMIChannel* channel_{nullptr};
};
