#include "TCPTransport.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

TCPTransport::TCPTransport() : socket_fd_(-1) {}

TCPTransport::~TCPTransport() 
{
    disconnect();
}

void TCPTransport::setstdinFd(int fd) 
{
    // Not used for TCP transport --- IGNORE ---
}

void TCPTransport::setstdoutFd(int fd) 
{
    // Not used for TCP transport --- IGNORE ---
}

void TCPTransport::connect(const std::string& host, uint16_t port) 
{
    socket_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd_ < 0) 
    {
        std::cerr << "Failed to create socket\n";
        return;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) 
    {
        std::cerr << "Invalid address\n";
        close(socket_fd_);
        socket_fd_ = -1;
        return;
    }
    if (::connect(socket_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) 
    {
        std::cerr << "Connection failed\n";
        close(socket_fd_);
        socket_fd_ = -1;
    }
}

void TCPTransport::send(const std::string& data) 
{
    if (socket_fd_ != -1) 
    {
        ::send(socket_fd_, data.c_str(), data.size(), 0);
    }
}

std::string TCPTransport::receive() 
{
    if (socket_fd_ == -1) return "";
    char buffer[1024];
    int len = recv(socket_fd_, buffer, sizeof(buffer) - 1, 0);
    if (len > 0) {
        buffer[len] = '\0';
        return std::string(buffer);
    }
    return "";
}

void TCPTransport::disconnect() 
{
    if (socket_fd_ != -1) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
}
