#pragma once
#include "MIRecord.h"
#include <functional>
#include <thread>
#include <atomic>

// Owns a dedicated reader thread that blocks on GDB's stdout pipe.
// For each complete MI line, calls GDBMIParser::parseLine() and dispatches
// the result via the DispatchFn callback — always from the reader thread.

class GDBMIReader {
public:
    using DispatchFn = std::function<void(MIRecord)>;

    ~GDBMIReader() { stop(); }

    // Starts the reader thread.
    void start(int stdoutFd, DispatchFn dispatch);

    // Signals the thread to exit and joins. Safe to call multiple times.
    void stop();

private:
    void readerLoop();

    int               fd_{-1};
    DispatchFn        dispatch_;
    std::thread       thread_;
    std::atomic<bool> running_{false};
};
