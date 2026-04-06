// --- GDBMIProtocol.cpp (Gộp các hàm thực thi của Layer C) ---
#include "GDBMIParser.h"
#include "GDBMIWriter.h"
#include "GDBMIReader.h"
#include "GDBMIChannel.h"
#include <unistd.h>
#include <string>

// --- GDBMIParser ---
MIRecord GDBMIParser::parseLine(std::string_view line) 
{
    MIRecord rec;
    if (line.empty()) return rec;

    size_t pos = 0;
    while (pos < line.size() && std::isdigit(line[pos])) pos++;
    if (pos > 0) rec.token = std::stoi(std::string(line.substr(0, pos)));
    if (pos >= line.size()) return rec;

    char prefix = line[pos++];
    switch (prefix) 
    {
        case '^': rec.type = MIRecord::Type::Result; break;
        case '*': rec.type = MIRecord::Type::ExecAsync; break;
        case '=': rec.type = MIRecord::Type::NotifyAsync; break;
        case '~': rec.type = MIRecord::Type::ConsoleStream; break;
        default:  rec.type = MIRecord::Type::Unknown; break;
    }

    size_t comma = line.find(',', pos);
    if (comma != std::string_view::npos) 
    {
        rec.klass = std::string(line.substr(pos, comma - pos));
    } 
    else 
    {
        rec.klass = std::string(line.substr(pos));
    }
    return rec;
}

// --- GDBMIWriter ---
void GDBMIWriter::attach(int stdinFd) { fd_ = stdinFd; }
void GDBMIWriter::write(int token, std::string_view command) {
    if (fd_ < 0) return;
    std::string fullCmd = std::to_string(token) + std::string(command) + "\n";
    std::lock_guard<std::mutex> lock(mtx_);
    ::write(fd_, fullCmd.c_str(), fullCmd.size());
}

// --- GDBMIReader ---
GDBMIReader::~GDBMIReader() { stop(); }

void GDBMIReader::start(int stdoutFd, DispatchFn dispatch) 
{
    fd_ = stdoutFd;
    dispatch_ = dispatch;
    running_ = true;
    thread_ = std::thread(&GDBMIReader::readerLoop, this);
}

void GDBMIReader::stop() 
{
    if (running_) 
    {
        running_ = false;
        if (thread_.joinable()) thread_.join();
    }
}

void GDBMIReader::readerLoop() 
{
    char buffer[4096];
    std::string lineStr;
    while (running_ && fd_ >= 0) 
    {
        ssize_t bytes = ::read(fd_, buffer, sizeof(buffer) - 1);
        if (bytes <= 0) break;
        buffer[bytes] = '\0';
        lineStr += buffer;
        
        size_t pos;
        while ((pos = lineStr.find('\n')) != std::string::npos) 
        {
            std::string line = lineStr.substr(0, pos);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            lineStr.erase(0, pos + 1);
            
            if (line != "(gdb) ") 
            {
                dispatch_(GDBMIParser::parseLine(line));
            }
        }
    }
}

// --- GDBMIChannel ---
void GDBMIChannel::attach(int stdinFd, int stdoutFd) {
    writer_.attach(stdinFd);
    reader_.start(stdoutFd, [this](MIRecord rec) { dispatch(rec); });
}

void GDBMIChannel::detach() { reader_.stop(); }

int GDBMIChannel::sendCommand(std::string_view command, ResponseCb onResult) {
    int token = nextToken_++;
    if (onResult) {
        std::lock_guard<std::mutex> lock(pendingMtx_);
        pending_[token] = onResult;
    }
    writer_.write(token, command);
    return token;
}

void GDBMIChannel::setAsyncHandler(AsyncCb cb) { asyncHandler_ = cb; }

void GDBMIChannel::dispatch(MIRecord rec) 
{
    if (rec.type == MIRecord::Type::Result) 
    {
        ResponseCb cb = nullptr;
        {
            std::lock_guard<std::mutex> lock(pendingMtx_);
            auto it = pending_.find(rec.token);
            if (it != pending_.end()) {
                cb = it->second;
                pending_.erase(it);
            }
        }
        if (cb) cb(rec);
    } 
    else if (rec.type == MIRecord::Type::ExecAsync || rec.type == MIRecord::Type::NotifyAsync) 
    {
        if (asyncHandler_) asyncHandler_(rec);
    }
}

void GDBMIChannel::cancelCommand(int token) 
{
    std::lock_guard<std::mutex> lock(pendingMtx_);
    pending_.erase(token);
}