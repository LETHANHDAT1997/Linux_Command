#ifndef GDBPROTOCOL_H
#define GDBPROTOCOL_H

#include "IProtocol.h"
#include <memory>

class GDBProtocol : public IProtocol 
{
public:
    GDBProtocol();
    void setstdinFd(int fd)                                 override;
    void setstdoutFd(int fd)                                override;
    void connect(const std::string& host, uint16_t port)    override;
    void disconnect()                                       override;
    int  sendCommand(const std::string& cmd)                override;
    std::string receive(void)                               override;
    void loadElf(const std::string& elfPath)                override;
    void setBreakpoint(const std::string& file, int line)   override;
    void run()                                              override;
    void halt()                                             override;
    void step()                                             override;
    void reset()                                            override;
    void resetandrun()                                      override;
private:
    int m_stdinFd;
    int m_stdoutFd;
};

#endif // GDBPROTOCOL_H