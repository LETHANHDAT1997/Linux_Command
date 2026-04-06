#pragma once
#include <string_view>
#include <mutex>

// Thread-safe serializer for GDB MI commands.
// Prepends the numeric token, appends a newline, and writes atomically to GDB's stdin fd.

class GDBMIWriter {
public:
    void attach(int stdinFd);
    void detach();

    // Writes "token command\n" to the attached fd.
    // Multiple threads may call this concurrently; writes are serialized.
    void write(int token, std::string_view command);

private:
    int        fd_{-1};
    std::mutex mtx_;
};
