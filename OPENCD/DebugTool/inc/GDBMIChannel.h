#pragma once
#include "GDBMIWriter.h"
#include "GDBMIReader.h"
#include <unordered_map>

class GDBMIChannel {
public:
    using ResponseCb = std::function<void(const MIRecord&)>;
    using AsyncCb = std::function<void(const MIRecord&)>;

    void attach(int stdinFd, int stdoutFd);
    void detach();
    
    int sendCommand(std::string_view command, ResponseCb onResult = nullptr);
    void setAsyncHandler(AsyncCb cb);
    void dispatch(MIRecord rec);
    void cancelCommand(int token);
    
private:
    std::atomic<int> nextToken_{1};
    std::mutex pendingMtx_;
    std::unordered_map<int, ResponseCb> pending_;
    AsyncCb asyncHandler_;

    GDBMIWriter writer_;
    GDBMIReader reader_;
};