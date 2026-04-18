#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>

class IDebugServer
{
public:
    virtual ~IDebugServer() = default;
    // Parse command line arguments specific to the debug server implementation.
    virtual bool ParseArgs(const std::vector<std::string>& argv) = 0;
    // Wait until the server is ready to accept connections or timeout.
    virtual bool WaitReady(std::chrono::milliseconds timeout = std::chrono::seconds{5}) = 0;
};
