#include "../../include/protocol/GDBMIWriter.h"
#include <unistd.h>
#include <cerrno>
#include <stdexcept>
#include <string>

void GDBMIWriter::attach(int stdinFd) {
    std::lock_guard<std::mutex> lk(mtx_);
    fd_ = stdinFd;
}

void GDBMIWriter::detach() {
    std::lock_guard<std::mutex> lk(mtx_);
    fd_ = -1;
}

void GDBMIWriter::write(int token, std::string_view command) {
    std::lock_guard<std::mutex> lk(mtx_);
    if (fd_ < 0) return;

    // Build: "token command\n"
    std::string line;
    line.reserve(command.size() + 16);
    line += std::to_string(token);
    line += command;
    line += '\n';

    const char* ptr       = line.data();
    size_t      remaining = line.size();

    while (remaining > 0) {
        ssize_t n = ::write(fd_, ptr, remaining);
        if (n < 0) {
            if (errno == EINTR) continue;
            // Pipe is broken — GDB exited. Silently drop the write.
            return;
        }
        ptr       += static_cast<size_t>(n);
        remaining -= static_cast<size_t>(n);
    }
}
