#pragma once
#include <mutex>
#include <string_view>

class GDBMIWriter {
public:
    void attach(int stdinFd);
    void write(int token, std::string_view command);
private:
    int fd_{-1};
    std::mutex mtx_;
};