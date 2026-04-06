#include "../../include/adapter/DebugStateMachine.h"

void DebugStateMachine::transition(DebugState next) {
    DebugState cur = state_.load(std::memory_order_acquire);
    if (!isAllowed(cur, next)) {
        throw std::logic_error(
            "DebugStateMachine: illegal transition from " +
            std::to_string(static_cast<int>(cur)) + " to " +
            std::to_string(static_cast<int>(next)));
    }
    state_.store(next, std::memory_order_release);
}

bool DebugStateMachine::isAllowed(DebugState from, DebugState to) {
    using S = DebugState;
    switch (from) {
        case S::Idle:       return to == S::Connecting;
        case S::Connecting: return to == S::Connected || to == S::Error;
        case S::Connected:  return to == S::Running   || to == S::Stopped || to == S::Idle;
        case S::Running:    return to == S::Stopped   || to == S::Error   || to == S::Idle;
        case S::Stopped:    return to == S::Running   || to == S::Idle;
        case S::Error:      return to == S::Idle;
    }
    return false;
}
