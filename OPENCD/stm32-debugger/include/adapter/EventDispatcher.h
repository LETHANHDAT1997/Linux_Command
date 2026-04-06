#pragma once
#include "../Types.h"
#include "../IDebugEventListener.h"
#include <vector>
#include <mutex>

// Thread-safe event fan-out to all registered IDebugEventListener instances.
//
// dispatch() is called from the GDB reader thread.
// It copies the listener list under the lock, releases the lock, then calls
// each listener — so listeners are never called with the lock held, preventing
// re-entrancy deadlocks.

class EventDispatcher {
public:
    void add(IDebugEventListener* l);
    void remove(IDebugEventListener* l);

    void dispatch(const DebugEvent& evt);

private:
    std::mutex                        mtx_;
    std::vector<IDebugEventListener*> listeners_;
};
