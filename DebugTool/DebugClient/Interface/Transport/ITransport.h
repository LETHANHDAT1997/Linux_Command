#ifndef ITRANSPORT_H
#define ITRANSPORT_H

#include <string>
#include <cstdint>

class ITransport {
public:
    virtual ~ITransport() = default;
    virtual void setstdinFd(int fd) = 0;
    virtual void setstdoutFd(int fd) = 0;
    virtual void connect(const std::string& host, uint16_t port) = 0;
    virtual void send(const std::string& data) = 0;
    virtual std::string receive() = 0;
    virtual void disconnect() = 0;
};

#endif // ITRANSPORT_H