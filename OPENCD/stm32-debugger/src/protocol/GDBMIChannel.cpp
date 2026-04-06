#include "../../include/protocol/GDBMIChannel.h"

void GDBMIChannel::attach(int stdinFd, int stdoutFd) 
{
    writer_.attach(stdinFd);
    reader_.start(stdoutFd, [this](MIRecord rec) 
                            {
                                dispatch(std::move(rec));
                            }
                 );
}

void GDBMIChannel::detach() 
{
    reader_.stop();
    writer_.detach();
}

int GDBMIChannel::sendCommand(std::string_view command, ResponseCb onResult) 
{
    int tok = nextToken_.fetch_add(1, std::memory_order_relaxed);
    if (onResult) 
    {
        std::lock_guard<std::mutex> lk(pendingMtx_);
        pending_[tok] = std::move(onResult);
    }
    writer_.write(tok, command);
    return tok;
}

void GDBMIChannel::setAsyncHandler(AsyncCb cb) {
    std::lock_guard<std::mutex> lk(handlerMtx_);
    asyncHandler_ = std::move(cb);
}

void GDBMIChannel::setStreamHandler(StreamCb cb) {
    std::lock_guard<std::mutex> lk(handlerMtx_);
    streamHandler_ = std::move(cb);
}

void GDBMIChannel::dispatch(MIRecord rec) 
{
    using T = MIRecord::Type;

    // ── Result record: matches a pending command token ─────────────────────
    if (rec.type == T::Result) 
    {
        ResponseCb cb;
        {
            std::lock_guard<std::mutex> lk(pendingMtx_);
            auto it = pending_.find(rec.token);
            if (it != pending_.end()) 
            {
                cb = std::move(it->second);
                pending_.erase(it);
            }
        }
        // Invoke callback *after* releasing the lock to avoid re-entrancy.
        if (cb) cb(rec);
        return;
    }

    // ── Async records: execution state changes, notifications ──────────────
    if (rec.type == T::ExecAsync    ||
        rec.type == T::NotifyAsync  ||
        rec.type == T::StatusAsync
       )
    {
        AsyncCb cb;
        {
            std::lock_guard<std::mutex> lk(handlerMtx_);
            cb = asyncHandler_;
        }
        if (cb) cb(rec);
        return;
    }

    // ── Stream records: console / target / log text ────────────────────────
    if (rec.type == T::ConsoleStream ||
        rec.type == T::TargetStream  ||
        rec.type == T::LogStream) {
        StreamCb cb;
        {
            std::lock_guard<std::mutex> lk(handlerMtx_);
            cb = streamHandler_;
        }
        if (cb && rec.payload.isString())
            cb(rec.payload.str());
        return;
    }
}
