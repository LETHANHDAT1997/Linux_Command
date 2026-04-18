#include "GDBProtocol.h"
#include <unistd.h>
#include <string>
GDBProtocol::GDBProtocol():
m_stdinFd(-1),
m_stdoutFd(-1)
{

}

void GDBProtocol::setstdinFd(int fd) 
{
    m_stdinFd = fd;
}

void GDBProtocol::setstdoutFd(int fd) 
{
    m_stdoutFd = fd;
}

void GDBProtocol::connect(const std::string& host, uint16_t port) 
{
    std::string cmd = "-target-select remote " + host + ":" + std::to_string(port);
    ssize_t n = ::write(m_stdinFd, cmd.c_str(), cmd.size());
}

void GDBProtocol::disconnect() 
{
    sendCommand("-gdb-exit");
}

int GDBProtocol::sendCommand(const std::string& cmd) 
{
    ssize_t n = ::write(m_stdinFd, cmd.c_str(), cmd.size());
    return n;
}

std::string GDBProtocol::receive(void) 
{
    char buffer[1024];
    ssize_t n = ::read(m_stdoutFd, buffer, sizeof(buffer) - 1);
    if (n > 0) 
    {
        buffer[n] = '\0';
        return std::string(buffer);
    }
    return "";
}

void GDBProtocol:: loadElf(const std::string& elfPath) 
{
    std::string cmd = "-file-exec-and-symbols " + elfPath;
    sendCommand(cmd);
}

void GDBProtocol::setBreakpoint(const std::string& file, int line) 
{
    std::string cmd = "-break-insert " + file + ":" + std::to_string(line);
    sendCommand(cmd);
}

void GDBProtocol::run() 
{
    sendCommand("-exec-run");
}

void GDBProtocol::halt() 
{
    sendCommand("-exec-interrupt");
}

void GDBProtocol::step() 
{
    sendCommand("-exec-step");
}

void GDBProtocol::reset() 
{
    sendCommand("-interpreter-exec console \"monitor reset halt\"\n");
}

void GDBProtocol::resetandrun() 
{
    sendCommand("-interpreter-exec console \"monitor reset run\"\n");
}