#pragma once
#include "../Types.h"
#include <atomic>
#include <stdexcept>

// Validates and tracks the debugger lifecycle state.
// transition() throws std::logic_error on illegal transitions.
//
// Allowed transitions:
//   Idle       → Connecting
//   Connecting → Connected | Error
//   Connected  → Running | Stopped | Idle
//   Running    → Stopped | Error | Idle
//   Stopped    → Running | Idle
//   Error      → Idle

class DebugStateMachine {
public:
    DebugState state() const { return state_.load(std::memory_order_acquire); }

    void transition(DebugState next);

private:
    static bool isAllowed(DebugState from, DebugState to);
    std::atomic<DebugState> state_{DebugState::Idle};
};
