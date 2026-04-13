#include "DebugClient.h"
#include "IProtocol.h"

DebugClient::DebugClient(std::unique_ptr<IProtocol> protocol)
    : protocol_(std::move(protocol))
    , m_stdinFd(-1)
    , m_stdoutFd(-1)
{
}

DebugClient::~DebugClient() = default;

bool DebugClient::start(const std::vector<std::string>& argv)
{
    bool lbResult = spawnProcess(argv);
    if (lbResult) 
    {
        m_stdinFd  = stdinFd();
        m_stdoutFd = stdoutFd();
        protocol_->setstdinFd(m_stdinFd);
        protocol_->setstdoutFd(m_stdoutFd);
    }
    return lbResult;
}

void DebugClient::connect(const std::string& host, uint16_t port) 
{
    protocol_->connect(host, port);
}

void DebugClient::importElf(const std::string& elfPath) 
{
    protocol_->loadElf(elfPath);
}

void DebugClient::disconnect() 
{
    protocol_->disconnect();
}

void DebugClient::setBreakpoint(const std::string& file, int line) 
{
    protocol_->setBreakpoint(file, line);
}

void DebugClient::run() 
{
    protocol_->run();
}

void DebugClient::halt() 
{
    protocol_->halt();
}

void DebugClient::step() 
{
    protocol_->step();
}