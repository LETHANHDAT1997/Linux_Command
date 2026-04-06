#include "../../include/protocol/GDBMIReader.h"
#include "../../include/protocol/GDBMIParser.h"
#include <unistd.h>
#include <string>

void GDBMIReader::start(int stdoutFd, DispatchFn dispatch) 
{
    if (running_.exchange(true)) return; // already running
    fd_       = stdoutFd;
    dispatch_ = std::move(dispatch);
    thread_   = std::thread(&GDBMIReader::readerLoop, this);
}

void GDBMIReader::stop() {
    if (!running_.exchange(false)) return;
    // The reader thread is blocked inside read(). Closing the fd from outside
    // would be a race; instead we rely on GDB's process exiting (or detach()
    // closing the pipe) to make read() return 0. join() waits for that.
    if (thread_.joinable())
        thread_.join();
}

void GDBMIReader::readerLoop() {
    std::string lineBuf;
    lineBuf.reserve(512);
    char tmp[512];

    while (running_.load(std::memory_order_relaxed)) {
        ssize_t n = ::read(fd_, tmp, sizeof(tmp));
        if (n <= 0) {
            // EOF (n == 0) or error — GDB process has exited.
            running_.store(false, std::memory_order_relaxed);
            break;
        }

        lineBuf.append(tmp, static_cast<size_t>(n));

        // Process all complete lines in the buffer.
        size_t pos;
        while ((pos = lineBuf.find('\n')) != std::string::npos) {
            std::string line = lineBuf.substr(0, pos + 1);
            lineBuf.erase(0, pos + 1);

            // The GDB prompt "(gdb)" is not an MI record; skip it.
            if (line == "(gdb)\n" || line == "(gdb)\r\n")
                continue;

            try {
                MIRecord rec = GDBMIParser::parseLine(line);
                if (rec.type != MIRecord::Type::Unknown && dispatch_)
                    dispatch_(std::move(rec));
            } catch (...) {
                // Malformed line — best-effort skip, keep running.
            }
        }
    }
}
