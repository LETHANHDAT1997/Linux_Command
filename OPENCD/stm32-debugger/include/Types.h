#pragma once
#include <string>
#include <cstdint>

// ─── Debugger lifecycle state ────────────────────────────────────────────────

enum class DebugState {
    Idle,
    Connecting,
    Connected,
    Running,
    Stopped,
    Error
};

// ─── Breakpoint ──────────────────────────────────────────────────────────────

struct BreakpointInfo {
    int         id      = -1;
    std::string file;
    int         line    = 0;
    uint32_t    addr    = 0;
    bool        enabled = true;
};

// ─── Stack frame ─────────────────────────────────────────────────────────────

struct FrameInfo {
    int         level    = 0;
    uint32_t    addr     = 0;
    std::string function;
    std::string file;
    int         line     = 0;
};

// ─── Debug event ─────────────────────────────────────────────────────────────

enum class DebugEventType {
    Stopped,
    Running,
    BreakpointHit,
    Disconnected,
    ConsoleOutput,
    Error
};

struct DebugEvent {
    DebugEventType type    = DebugEventType::Error;
    std::string    reason;      // MI "reason" string, e.g. "breakpoint-hit"
    FrameInfo      frame;       // valid on Stopped / BreakpointHit
    int            bkptId  = -1;
    std::string    message;     // for ConsoleOutput / Error
};
