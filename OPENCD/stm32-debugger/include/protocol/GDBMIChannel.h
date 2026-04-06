#pragma once
#include "MIRecord.h"
#include "GDBMIWriter.h"
#include "GDBMIReader.h"
#include <functional>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <string_view>

// Central coordination hub for the GDB MI protocol.
//
// Owns GDBMIWriter (send) and GDBMIReader (receive).
// Token correlation: every command gets a unique integer token;
// when the reader receives a result record with that token, the
// registered callback is fired — from the reader thread.
//
// Thread safety:
//   sendCommand()  — safe to call from any thread.
//   dispatch()     — called only from the GDBMIReader thread.
//   Callbacks fire — always on the reader thread.

class GDBMIChannel {
public:
    using ResponseCb = std::function<void(const MIRecord&)>;
    using AsyncCb    = std::function<void(const MIRecord&)>;
    using StreamCb   = std::function<void(std::string_view)>;

    // Attach to live pipe fds; starts the reader thread.
    void attach(int stdinFd, int stdoutFd);

    // Stops the reader thread and detaches the writer.
    void detach();

    // Sends "token command\n" to GDB.
    // If onResult is non-null, it is called when GDB sends the matching ^result.
    // Returns the token used.
    int sendCommand(std::string_view command, ResponseCb onResult = nullptr);

    // Called for *exec-async, =notify-async, +status-async records.
    void setAsyncHandler(AsyncCb cb);

    // Called for ~console, @target, &log stream records.
    void setStreamHandler(StreamCb cb);

    // Called by GDBMIReader for every parsed record. Routes to the correct handler.
    void dispatch(MIRecord rec);

private:
    std::atomic<int>                       nextToken_{1};

    std::mutex                             pendingMtx_;
    std::unordered_map<int, ResponseCb>    pending_;

    std::mutex                             handlerMtx_;
    AsyncCb                                asyncHandler_;
    StreamCb                               streamHandler_;

    GDBMIWriter                            writer_;
    GDBMIReader                            reader_;
};
