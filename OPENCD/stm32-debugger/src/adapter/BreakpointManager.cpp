#include "../../include/adapter/BreakpointManager.h"
#include <sstream>
#include <iomanip>

void BreakpointManager::attach(GDBMIChannel& ch) {
    channel_ = &ch;
}

// ─── Insert ──────────────────────────────────────────────────────────────────

void BreakpointManager::add(const std::string& file, int line, DoneCb cb) {
    std::string cmd = "-break-insert " + file + ":" + std::to_string(line);
    channel_->sendCommand(cmd, [this, cb](const MIRecord& rec) {
        onInsertResult(rec, cb);
    });
}

void BreakpointManager::addAtAddress(uint32_t addr, DoneCb cb) {
    std::ostringstream oss;
    oss << "-break-insert *0x" << std::hex << std::uppercase << addr;
    channel_->sendCommand(oss.str(), [this, cb](const MIRecord& rec) {
        onInsertResult(rec, cb);
    });
}

void BreakpointManager::onInsertResult(const MIRecord& rec, DoneCb cb) {
    if (rec.klass != "done") {
        if (cb) cb(-1);
        return;
    }

    // MI result: ^done,bkpt={number="1",type="breakpoint",addr="0x...",
    //             file="main.c",line="42",...}
    const MIValue& bkpt = rec.payload["bkpt"];
    if (!bkpt.isTuple()) {
        if (cb) cb(-1);
        return;
    }

    BreakpointInfo info;
    try {
        info.id = std::stoi(bkpt["number"].str());
    } catch (...) {
        if (cb) cb(-1);
        return;
    }

    info.enabled = true;
    if (bkpt["file"].isString()) {
        info.file = bkpt["file"].str();
    }
    if (bkpt["line"].isString()) {
        try { info.line = std::stoi(bkpt["line"].str()); } catch (...) {}
    }
    if (bkpt["addr"].isString()) {
        try {
            const std::string& a = bkpt["addr"].str();
            info.addr = static_cast<uint32_t>(
                std::stoul(a.substr(a.find("0x") != std::string::npos ? 0 : 0),
                           nullptr, 16));
        } catch (...) {}
    }

    {
        std::lock_guard<std::mutex> lk(mtx_);
        breakpoints_[info.id] = info;
    }

    if (cb) cb(info.id);
}

// ─── Remove / enable ─────────────────────────────────────────────────────────

void BreakpointManager::remove(int id, std::function<void()> cb) {
    channel_->sendCommand("-break-delete " + std::to_string(id),
        [this, id, cb](const MIRecord& rec) {
            if (rec.klass == "done") {
                std::lock_guard<std::mutex> lk(mtx_);
                breakpoints_.erase(id);
            }
            if (cb) cb();
        });
}

void BreakpointManager::enable(int id, bool on) {
    std::string cmd = on ? "-break-enable " : "-break-disable ";
    cmd += std::to_string(id);
    channel_->sendCommand(cmd, nullptr);
}

// ─── Queries (local cache) ────────────────────────────────────────────────────

const BreakpointInfo* BreakpointManager::find(int id) const {
    std::lock_guard<std::mutex> lk(mtx_);
    auto it = breakpoints_.find(id);
    return it != breakpoints_.end() ? &it->second : nullptr;
}

std::vector<BreakpointInfo> BreakpointManager::all() const {
    std::lock_guard<std::mutex> lk(mtx_);
    std::vector<BreakpointInfo> result;
    result.reserve(breakpoints_.size());
    for (const auto& [k, v] : breakpoints_)
        result.push_back(v);
    return result;
}
