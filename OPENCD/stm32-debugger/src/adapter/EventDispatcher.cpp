#include "../../include/adapter/EventDispatcher.h"
#include <algorithm>

void EventDispatcher::add(IDebugEventListener* l) {
    std::lock_guard<std::mutex> lk(mtx_);
    listeners_.push_back(l);
}

void EventDispatcher::remove(IDebugEventListener* l) {
    std::lock_guard<std::mutex> lk(mtx_);
    listeners_.erase(
        std::remove(listeners_.begin(), listeners_.end(), l),
        listeners_.end());
}

void EventDispatcher::dispatch(const DebugEvent& evt) {
    // Copy the listener list under the lock, then call each listener
    // with the lock released — prevents deadlock if a listener calls
    // add/remove, and avoids holding the lock across slow UI callbacks.
    std::vector<IDebugEventListener*> snapshot;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        snapshot = listeners_;
    }
    for (auto* l : snapshot)
        l->onDebugEvent(evt);
}
