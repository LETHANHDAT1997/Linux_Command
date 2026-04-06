#pragma once
#include "Types.h"
#include <atomic>

class DebugStateMachine {
public:
    DebugState state() const;
    void transition(DebugState next);
private:
    std::atomic<DebugState> state_{DebugState::Idle};
};