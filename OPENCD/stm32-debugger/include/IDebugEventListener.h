#pragma once
#include "Types.h"

class IDebugEventListener {
public:
    virtual ~IDebugEventListener() = default;

    // Always called from the GDB reader thread.
    // Implementations MUST NOT touch UI widgets directly — post to the UI thread instead.
    virtual void onDebugEvent(const DebugEvent& evt) = 0;
};
