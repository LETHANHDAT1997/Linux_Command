// --- ProcessHandle.cpp (Thực thi cho ProcessHandle, Managers) ---
#include "ProcessHandle.h"
#include "DebugStateMachine.h"
#include "VariableManager.h"
#include "EventDispatcher.h"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <future>
#include <algorithm>

// --- ProcessHandle ---
ProcessHandle::~ProcessHandle() { terminate(); }

bool ProcessHandle::isRunning() const {
    if (pid_ <= 0) return false;
    int status;
    return waitpid(pid_, &status, WNOHANG) == 0;
}

void ProcessHandle::terminate() {
    if (isRunning()) {
        kill(pid_, SIGTERM);
        usleep(100000); 
        if (isRunning()) kill(pid_, SIGKILL);
        waitpid(pid_, nullptr, 0); 
    }
    if (stdinPipe_ >= 0) { close(stdinPipe_); stdinPipe_ = -1; }
    if (stdoutPipe_ >= 0) { close(stdoutPipe_); stdoutPipe_ = -1; }
    pid_ = -1;
}

bool ProcessHandle::spawnProcess(const std::vector<std::string>& argv) {
    int inPipe[2], outPipe[2], errPipe[2];
    if (pipe(inPipe) != 0 || pipe(outPipe) != 0 || pipe(errPipe) != 0) return false;

    pid_ = fork();
    if (pid_ == 0) {
        dup2(inPipe[0], STDIN_FILENO);
        dup2(outPipe[1], STDOUT_FILENO);
        dup2(errPipe[1], STDERR_FILENO);

        close(inPipe[0]); close(inPipe[1]);
        close(outPipe[0]); close(outPipe[1]);
        close(errPipe[0]); close(errPipe[1]);

        std::vector<char*> c_argv;
        for (const auto& arg : argv) c_argv.push_back(const_cast<char*>(arg.c_str()));
        c_argv.push_back(nullptr);

        execvp(c_argv[0], c_argv.data());
        exit(1);
    } else if (pid_ > 0) {
        stdinPipe_ = inPipe[1];
        stdoutPipe_ = outPipe[0];
        close(inPipe[0]); close(outPipe[1]); close(errPipe[1]); close(errPipe[0]);
        return true;
    }
    return false;
}

bool GDBProcess::start(const std::string& gdbPath) {
    return spawnProcess({gdbPath, "--interpreter=mi", "--quiet"});
}

bool OpenOCDProcess::start(const std::string& configFile, const std::string& path) {
    return spawnProcess({path, "-f", configFile});
}

bool OpenOCDProcess::waitReady(int seconds) {
    sleep(1); // Giả lập chờ kết nối
    return isRunning(); 
}

// --- DebugStateMachine ---
DebugState DebugStateMachine::state() const { return state_.load(); }
void DebugStateMachine::transition(DebugState next) { state_.store(next); }

// --- VariableManager ---
void VariableManager::attach(GDBMIChannel& ch) { channel_ = &ch; }

std::string VariableManager::evaluate(const std::string& expr) {
    return sendSync("-data-evaluate-expression " + expr).payload.str();
}

MIRecord VariableManager::sendSync(std::string_view cmd) {
    std::promise<MIRecord> promise;
    auto future = promise.get_future();
    channel_->sendCommand(cmd, [&promise](const MIRecord& rec) {
        promise.set_value(rec);
    });
    return future.get();
}

// --- EventDispatcher ---
void EventDispatcher::add(IDebugEventListener* l) {
    std::lock_guard<std::mutex> lock(mtx_);
    listeners_.push_back(l);
}

void EventDispatcher::remove(IDebugEventListener* l) {
    std::lock_guard<std::mutex> lock(mtx_);
    listeners_.erase(std::remove(listeners_.begin(), listeners_.end(), l), listeners_.end());
}

void EventDispatcher::dispatch(const DebugEvent& evt) {
    std::vector<IDebugEventListener*> copy;
    {
        std::lock_guard<std::mutex> lock(mtx_);
        copy = listeners_;
    }
    for (auto* l : copy) l->onDebugEvent(evt);
}