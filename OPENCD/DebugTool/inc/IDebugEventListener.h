#pragma once
#include "Types.h"

class IDebugEventListener {
public:
    virtual ~IDebugEventListener() = default;
    virtual void onDebugEvent(const DebugEvent& evt) = 0;
};