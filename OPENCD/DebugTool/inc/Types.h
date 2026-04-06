#pragma once
#include <string>
#include <vector>
#include <variant>
#include <map>
#include <cstdint>

enum class DebugState { Idle, Connecting, Connected, Running, Stopped, Error };
enum class DebugEventType { Stopped, Running, BreakpointHit, Disconnected, ConsoleOutput, Error };

struct BreakpointInfo {
    int         id;
    std::string file;
    int         line;
    bool        enabled = true;
};

struct FrameInfo {
    int         level;
    uint32_t    addr;
    std::string function;
    std::string file;
    int         line;
};

struct DebugEvent {
    DebugEventType type;
    std::string    reason;
    FrameInfo      frame;
    int            bkptId = -1;
    std::string    message;
};