#include "../../include/adapter/GDBDebugger.h"
#include <future>
#include <sstream>
#include <iomanip>
#include <memory>

// ─── Configuration ────────────────────────────────────────────────────────────

void GDBDebugger::configure(std::string openocdConfig,
                             std::string gdbPath,
                             std::string openocdPath) {
    openocdConfig_ = std::move(openocdConfig);
    gdbPath_       = std::move(gdbPath);
    openocdPath_   = std::move(openocdPath);
}

// ─── Lifecycle ────────────────────────────────────────────────────────────────

bool GDBDebugger::start() {
    // 1. Start OpenOCD first — it must be listening before GDB connects.
    if (!openocd_.start(openocdConfig_, openocdPath_)) return false;
    if (!openocd_.waitReady(std::chrono::seconds{8})) {
        openocd_.terminate();
        return false;
    }

    // 2. Start GDB with MI interpreter.
    if (!gdb_.start(gdbPath_)) {
        openocd_.terminate();
        return false;
    }

    // 3. Attach the channel to GDB's pipes and register handlers.
    channel_.attach(gdb_.stdinFd(), gdb_.stdoutFd());
    channel_.setAsyncHandler([this](const MIRecord& rec) { onAsyncRecord(rec); });
    channel_.setStreamHandler([this](std::string_view text) { onStreamOutput(text); });

    // 4. Initial MI setup — must happen before any target commands.
    sendBlocking("-gdb-set target-async on",  3);
    sendBlocking("-gdb-set pagination off",   3);
    sendBlocking("-gdb-set confirm off",      3);
    sendBlocking("-gdb-set print pretty on",  3);

    breakpoints_.attach(channel_);
    variables_.attach(channel_);

    stateMachine_.transition(DebugState::Connecting);
    return true;
}

void GDBDebugger::stop() {
    // Ask GDB to quit gracefully; ignore errors (process may already be dead).
    channel_.sendCommand("-gdb-exit", nullptr);

    // Give GDB 1 s to exit cleanly, then force-kill.
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    channel_.detach();
    gdb_.terminate();
    openocd_.terminate();

    try { stateMachine_.transition(DebugState::Idle); } catch (...) {}
}

// ─── Connection ───────────────────────────────────────────────────────────────

bool GDBDebugger::connectToTarget(const std::string& host, int port) {
    std::string cmd = "-target-select remote " + host + ":" + std::to_string(port);

    auto promise = std::make_shared<std::promise<bool>>();
    auto future  = promise->get_future();

    channel_.sendCommand(cmd, [promise](const MIRecord& rec) {
        // GDB replies with ^connected or ^done on success, ^error on failure.
        bool ok = (rec.klass == "connected" || rec.klass == "done");
        promise->set_value(ok);
    });

    if (future.wait_for(std::chrono::seconds{10}) != std::future_status::ready)
        return false;

    bool ok = future.get();
    if (ok) {
        try { stateMachine_.transition(DebugState::Connected); } catch (...) {}
    } else {
        try { stateMachine_.transition(DebugState::Error); } catch (...) {}
    }
    return ok;
}

bool GDBDebugger::loadElf(const std::string& elfPath) {
    // -file-exec-and-symbols: sets the executable and loads debug symbols.
    return sendBlocking("-file-exec-and-symbols \"" + elfPath + "\"", 15);
}

void GDBDebugger::reset(bool halt) {
    // These are OpenOCD-specific monitor commands passed through GDB.
    if (halt)
        channel_.sendCommand("monitor reset halt", nullptr);
    else
        channel_.sendCommand("monitor reset run",  nullptr);
}

// ─── Execution control ────────────────────────────────────────────────────────

void GDBDebugger::run() {
    channel_.sendCommand("-exec-continue", nullptr);
    try { stateMachine_.transition(DebugState::Running); } catch (...) {}
}

void GDBDebugger::halt() {
    // -exec-interrupt sends SIGINT to the target.
    channel_.sendCommand("-exec-interrupt", nullptr);
}

void GDBDebugger::stepInto() {
    channel_.sendCommand("-exec-step", nullptr);
    try { stateMachine_.transition(DebugState::Running); } catch (...) {}
}

void GDBDebugger::stepOver() {
    channel_.sendCommand("-exec-next", nullptr);
    try { stateMachine_.transition(DebugState::Running); } catch (...) {}
}

void GDBDebugger::stepOut() {
    channel_.sendCommand("-exec-finish", nullptr);
    try { stateMachine_.transition(DebugState::Running); } catch (...) {}
}

// ─── Breakpoints (blocking wrappers around BreakpointManager) ─────────────────

int GDBDebugger::addBreakpoint(const std::string& file, int line) {
    auto promise = std::make_shared<std::promise<int>>();
    auto future  = promise->get_future();
    breakpoints_.add(file, line, [promise](int id) { promise->set_value(id); });
    if (future.wait_for(std::chrono::seconds{5}) != std::future_status::ready) return -1;
    return future.get();
}

int GDBDebugger::addBreakpointAtAddress(uint32_t addr) {
    auto promise = std::make_shared<std::promise<int>>();
    auto future  = promise->get_future();
    breakpoints_.addAtAddress(addr, [promise](int id) { promise->set_value(id); });
    if (future.wait_for(std::chrono::seconds{5}) != std::future_status::ready) return -1;
    return future.get();
}

void GDBDebugger::removeBreakpoint(int id) {
    auto promise = std::make_shared<std::promise<void>>();
    auto future  = promise->get_future();
    breakpoints_.remove(id, [promise]() { promise->set_value(); });
    future.wait_for(std::chrono::seconds{5});
}

void GDBDebugger::enableBreakpoint(int id, bool enable) {
    breakpoints_.enable(id, enable);
}

// ─── Inspection (delegated to VariableManager) ────────────────────────────────

std::string GDBDebugger::evaluate(const std::string& expr) {
    return variables_.evaluate(expr);
}

std::vector<uint8_t> GDBDebugger::readMemory(uint32_t addr, size_t len) {
    return variables_.readMemory(addr, len);
}

std::string GDBDebugger::readRegisters() {
    return variables_.readRegisters();
}

std::vector<FrameInfo> GDBDebugger::backtrace() {
    return variables_.backtrace();
}

// ─── Async record handler (reader thread) ─────────────────────────────────────

void GDBDebugger::onAsyncRecord(const MIRecord& rec) {
    if (rec.type != MIRecord::Type::ExecAsync) return;

    if (rec.klass == "stopped") {
        handleStopped(rec);
    } else if (rec.klass == "running") {
        try { stateMachine_.transition(DebugState::Running); } catch (...) {}
        DebugEvent evt;
        evt.type = DebugEventType::Running;
        dispatcher_.dispatch(evt);
    }
}

void GDBDebugger::onStreamOutput(std::string_view text) {
    DebugEvent evt;
    evt.type    = DebugEventType::ConsoleOutput;
    evt.message = std::string(text);
    dispatcher_.dispatch(evt);
}

// ─── Stopped event processing ────────────────────────────────────────────────

FrameInfo GDBDebugger::parseFrame(const MIValue& frameVal) {
    FrameInfo fi;
    if (!frameVal.isTuple()) return fi;
    try {
        if (frameVal["level"].isString()) fi.level    = std::stoi(frameVal["level"].str());
        if (frameVal["addr"].isString())  fi.addr     = static_cast<uint32_t>(
                                              std::stoul(frameVal["addr"].str(), nullptr, 16));
        if (frameVal["func"].isString())  fi.function = frameVal["func"].str();
        if (frameVal["file"].isString())  fi.file     = frameVal["file"].str();
        if (frameVal["line"].isString())  fi.line     = std::stoi(frameVal["line"].str());
    } catch (...) {}
    return fi;
}

void GDBDebugger::handleStopped(const MIRecord& rec) {
    try { stateMachine_.transition(DebugState::Stopped); } catch (...) {}

    DebugEvent evt;
    evt.frame = parseFrame(rec.payload["frame"]);

    const MIValue& reason = rec.payload["reason"];
    evt.reason = reason.isString() ? reason.str() : "unknown";

    if (evt.reason == "breakpoint-hit") {
        evt.type = DebugEventType::BreakpointHit;
        const MIValue& bkptno = rec.payload["bkptno"];
        if (bkptno.isString()) {
            try { evt.bkptId = std::stoi(bkptno.str()); } catch (...) {}
        }
    } else if (evt.reason == "exited" || evt.reason == "exited-normally" ||
               evt.reason == "exited-signalled") {
        evt.type = DebugEventType::Disconnected;
        const MIValue& sig = rec.payload["signal-name"];
        if (sig.isString()) evt.message = "signal: " + sig.str();
    } else {
        evt.type = DebugEventType::Stopped;
    }

    dispatcher_.dispatch(evt);
}

// ─── sendBlocking helper ──────────────────────────────────────────────────────

bool GDBDebugger::sendBlocking(std::string_view cmd, int timeoutSec) {
    auto promise = std::make_shared<std::promise<bool>>();
    auto future  = promise->get_future();

    channel_.sendCommand(cmd, [promise](const MIRecord& rec) {
        promise->set_value(rec.klass == "done" || rec.klass == "connected");
    });

    if (future.wait_for(std::chrono::seconds{timeoutSec}) != std::future_status::ready)
        return false;
    return future.get();
}
