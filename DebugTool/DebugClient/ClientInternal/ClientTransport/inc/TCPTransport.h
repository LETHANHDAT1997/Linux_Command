#ifndef TCPTRANSPORT_H
#define TCPTRANSPORT_H

#include "ITransport.h"

class TCPTransport : public ITransport {
public:
    TCPTransport();
    ~TCPTransport() override;
    void setstdinFd(int fd) override;  
    void setstdoutFd(int fd) override;
    void connect(const std::string& host, uint16_t port) override;
    void send(const std::string& data) override;
    std::string receive() override;
    void disconnect() override;
private:
    int socket_fd_;
};

#endif // TCPTRANSPORT_H