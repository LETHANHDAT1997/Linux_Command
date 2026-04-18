#pragma once
#include <string>
#include <chrono>
#include "IDebugServer.h"
#include "ProcessHandle.h"

class OpenOCD : public IDebugServer
{
public:
    OpenOCD();
    ~OpenOCD();
    bool ParseArgs(const std::vector<std::string>& argv) override;
    // Polls TCP port until OpenOCD is accepting connections, or timeout expires.
    // Returns false on timeout.
    bool WaitReady(std::chrono::milliseconds timeout = std::chrono::seconds{5}) override;
private:
    int m_port;
    std::string m_host;
};
