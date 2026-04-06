// --- GDBDebugger.cpp ---
#include "GDBDebugger.h"
#include <future>

bool GDBDebugger::start() 
{
    if (!gdb_.start()) return false;
    channel_.attach(gdb_.stdinFd(), gdb_.stdoutFd());
    variables_.attach(channel_);
    
    channel_.setAsyncHandler([this](const MIRecord& rec) { onAsyncRecord(rec); });
    
    channel_.sendCommand("-gdb-set target-async on");
    stateMachine_.transition(DebugState::Connecting);
    return true;
}

void GDBDebugger::stop() 
{
    channel_.detach();
    gdb_.terminate();
    openocd_.terminate();
    stateMachine_.transition(DebugState::Idle);
}

bool GDBDebugger::connectToTarget(const std::string& host, int port) 
{
    std::string cmd = "-target-select remote " + host + ":" + std::to_string(port);
    
    // Chờ phản hồi từ GDB
    MIRecord response = executeSync(cmd);
    printf("GDB Response: type=%d, klass=%s\n", static_cast<int>(response.type), response.klass.c_str());
    // Kiểm tra class của Result Record (thường là "connected" hoặc "done")
    if  (   (response.type == MIRecord::Type::Result) && 
            (response.klass == "connected" || response.klass == "done")
        )
    {
        stateMachine_.transition(DebugState::Connected);
        return true;
    }
    
    // Nếu GDB trả về ^error
    return false;
}

bool GDBDebugger::loadElf(const std::string& elfPath) 
{
    std::string cmd = "-file-exec-and-symbols " + elfPath;
    
    MIRecord response = executeSync(cmd);
    
    if (response.type == MIRecord::Type::Result && response.klass == "done") 
    {
        return true;
    }
    return false; // File không tồn tại hoặc lỗi định dạng
}

void GDBDebugger::run() { channel_.sendCommand("-exec-continue"); }
void GDBDebugger::halt() { channel_.sendCommand("-exec-interrupt"); }

DebugState GDBDebugger::state() const { return stateMachine_.state(); }

void GDBDebugger::addListener(IDebugEventListener* l) { dispatcher_.add(l); }
void GDBDebugger::removeListener(IDebugEventListener* l) { dispatcher_.remove(l); }

void GDBDebugger::onAsyncRecord(const MIRecord& rec) 
{
    if (rec.klass == "stopped") 
    {
        stateMachine_.transition(DebugState::Stopped);
        DebugEvent evt{DebugEventType::Stopped, "Target Halted", {}, -1, ""};
        dispatcher_.dispatch(evt);
    } 
    else if (rec.klass == "running") 
    {
        stateMachine_.transition(DebugState::Running);
        DebugEvent evt{DebugEventType::Running, "Target Running", {}, -1, ""};
        dispatcher_.dispatch(evt);
    }
}

MIRecord GDBDebugger::executeSync(std::string_view cmd, int timeoutMs) 
{
    // Dùng shared_ptr để đảm bảo an toàn vùng nhớ (memory safety) 
    // trong trường hợp callback chạy sau khi hàm này đã return do timeout
    auto promisePtr = std::make_shared<std::promise<MIRecord>>();
    auto future = promisePtr->get_future();
    
    // Gửi lệnh và lấy về token ID
    int token = channel_.sendCommand(cmd, [promisePtr](const MIRecord& rec) 
    {
        promisePtr->set_value(rec); // Gọi an toàn vì đã dùng shared_ptr
    });
    
    // Đợi kết quả với thời gian tối đa
    std::future_status status = future.wait_for(std::chrono::milliseconds(timeoutMs));
    
    if (status == std::future_status::ready) 
    {
        // Có kết quả trả về kịp thời
        return future.get();
    } 
    else 
    {
        // TIMEOUT!
        // Xóa callback trong channel để tránh lãng phí RAM (memory leak)
        channel_.cancelCommand(token);
        
        // Trả về một Record báo lỗi do Timeout
        MIRecord errorRec;
        errorRec.type = MIRecord::Type::Result;
        errorRec.klass = "error";
        // Giả lập thông báo lỗi để các hàm gọi (như connectToTarget) tự xử lý
        return errorRec; 
    }
}