#pragma once
#include <string>
#include <vector>
#include <sys/types.h>

class ProcessHandle {
public:
    virtual ~ProcessHandle();
    bool isRunning() const;
    void terminate();
    int stdinFd() const { return stdinPipe_; }
    int stdoutFd() const { return stdoutPipe_; }

protected:
    bool spawnProcess(const std::vector<std::string>& argv);
    pid_t pid_{-1};
    int stdinPipe_{-1};
    int stdoutPipe_{-1};
};

class GDBProcess : public ProcessHandle {
public:
    bool start(const std::string& gdbPath = "arm-none-eabi-gdb");
};

class OpenOCDProcess : public ProcessHandle {
public:
    bool start(const std::string& configFile, const std::string& path = "openocd");
    bool waitReady(int seconds = 5);
};