#pragma once
#include "../Types.h"
#include "../protocol/GDBMIChannel.h"
#include <functional>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <string>
#include <cstdint>

// Manages the breakpoint lifecycle via MI commands.
// All public methods issue MI commands through GDBMIChannel; results arrive
// asynchronously via callbacks (always on the reader thread).
// The local cache (breakpoints_) is always up-to-date after a callback fires.

class BreakpointManager {
public:
    using DoneCb = std::function<void(int id)>; // id == -1 on failure

    void attach(GDBMIChannel& ch);

    void add(const std::string& file, int line, DoneCb cb = nullptr);
    void addAtAddress(uint32_t addr, DoneCb cb = nullptr);
    void remove(int id, std::function<void()> cb = nullptr);
    void enable(int id, bool on);

    // Thread-safe queries on the local cache.
    const BreakpointInfo*       find(int id) const;
    std::vector<BreakpointInfo> all()        const;

private:
    void onInsertResult(const MIRecord& rec, DoneCb cb);

    GDBMIChannel*                           channel_{nullptr};
    mutable std::mutex                      mtx_;
    std::unordered_map<int, BreakpointInfo> breakpoints_;
};
