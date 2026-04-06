#pragma once
#include "MIRecord.h"
#include <thread>
#include <atomic>
#include <functional>

class GDBMIReader {
public:
    using DispatchFn = std::function<void(MIRecord)>;

    ~GDBMIReader();
    void start(int stdoutFd, DispatchFn dispatch);
    void stop();

private:
    void readerLoop();

    int fd_{-1};
    DispatchFn dispatch_;
    std::thread thread_;
    std::atomic<bool> running_{false};
};