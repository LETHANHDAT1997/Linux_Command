#ifndef IPROTOCOL_H
#define IPROTOCOL_H

#include <string>
#include <cstdint>
class IProtocol 
{
public:
    virtual ~IProtocol() = default;
    virtual void setstdinFd(int fd) = 0;
    virtual void setstdoutFd(int fd) = 0;
    virtual void connect(const std::string& host, uint16_t port) = 0;
    virtual void disconnect() = 0;
    virtual int  sendCommand(const std::string& cmd) = 0;
    virtual std::string receive(void) = 0;
    virtual void loadElf(const std::string& elfPath) = 0;
    virtual void setBreakpoint(const std::string& file, int line) = 0;
    virtual void run() = 0;
    virtual void halt() = 0;
    virtual void step() = 0;
    virtual void reset() = 0;
    virtual void resetandrun() = 0;

};

#endif // IPROTOCOL_H