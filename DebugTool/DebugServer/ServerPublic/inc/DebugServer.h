#pragma once
#include <memory>
#include <string>
#include <vector>
#include "ProcessHandle.h"
#include "chrono"

class IDebugServer;

class DebugServer : public ProcessHandle
{
public:
    explicit DebugServer(std::unique_ptr<IDebugServer> protocol);
    ~DebugServer();

    bool Start(const std::vector<std::string>& argv);
    void Stop();
    bool IsRunning() const;
    bool WaitReady(std::chrono::milliseconds timeout = std::chrono::seconds{5});
    void StopProcess();
private:
    std::unique_ptr<IDebugServer> server_;
    int m_stdinFd;
    int m_stdoutFd;
};