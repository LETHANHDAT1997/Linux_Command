#include "DebugServer.h"
#include "IDebugServer.h"
#include <memory>

DebugServer::DebugServer(std::unique_ptr<IDebugServer> protocol)
    : server_(std::move(protocol))
{
}

DebugServer::~DebugServer()
{
    terminate();
}

bool DebugServer::Start(const std::vector<std::string>& argv)
{
    bool lbResult = server_->ParseArgs(argv);
    if (lbResult)
    {
        lbResult = spawnProcess(argv);
    }
    if (lbResult)
    {   
        m_stdinFd  = stdinFd();
        m_stdoutFd = stdoutFd();
    }
    return lbResult;
}

bool DebugServer::IsRunning() const
{
    return isRunning();
}

bool DebugServer::WaitReady(std::chrono::milliseconds timeout)
{
    return server_->WaitReady(timeout);
}

void DebugServer::StopProcess()
{
    terminate();
}