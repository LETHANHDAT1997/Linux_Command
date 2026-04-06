#pragma once
#include "IDebugEventListener.h"
#include <vector>
#include <mutex>

class EventDispatcher {
public:
    void add(IDebugEventListener* l);
    void remove(IDebugEventListener* l);
    void dispatch(const DebugEvent& evt);

private:
    std::mutex mtx_;
    std::vector<IDebugEventListener*> listeners_;
};