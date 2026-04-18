#ifndef DEBUGCLIENT_H
#define DEBUGCLIENT_H

#include <memory>
#include <string>
#include <vector>
#include "ProcessHandle.h"

class IProtocol;

class DebugClient : public ProcessHandle
{
public:
    explicit DebugClient(std::unique_ptr<IProtocol> protocol);
    ~DebugClient();

    bool start(const std::vector<std::string>& argv);
    void connect(const std::string& host, uint16_t port);
    void importElf(const std::string& elfPath);
    void disconnect();
    void setBreakpoint(const std::string& file, int line);
    void run();
    void halt();
    void step();
    void stop();
    void stopProcess();
private:
    std::unique_ptr<IProtocol> protocol_;
    int m_stdinFd;
    int m_stdoutFd;
};

#endif // DEBUGCLIENT_H