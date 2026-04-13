#include "ProcessHandle.h"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdexcept>
#include <thread>
#include <chrono>

bool ProcessHandle::isRunning() const {
    if (pid_ <= 0) return false;
    return ::waitpid(pid_, nullptr, WNOHANG) == 0;
}

void ProcessHandle::terminate() {
    if (pid_ <= 0) return;

    ::kill(pid_, SIGTERM);

    // Poll up to 2 s before escalating to SIGKILL.
    for (int i = 0; i < 20; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        int status = 0;
        if (::waitpid(pid_, &status, WNOHANG) == pid_) {
            pid_ = -1;
            goto close_pipes;
        }
    }

    ::kill(pid_, SIGKILL);
    ::waitpid(pid_, nullptr, 0);
    pid_ = -1;

close_pipes:
    auto closefd = [](int& fd) {
        if (fd >= 0) { ::close(fd); fd = -1; }
    };
    closefd(stdinPipe_);
    closefd(stdoutPipe_);
    closefd(stderrPipe_);
}

bool ProcessHandle::spawnProcess(const std::vector<std::string>& argv) {
    if (argv.empty()) return false;

    int stdinP[2], stdoutP[2], stderrP[2];

    auto closePair = [](int p[2]) { ::close(p[0]); ::close(p[1]); };

    if (::pipe(stdinP) < 0) return false;
    if (::pipe(stdoutP) < 0) { closePair(stdinP); return false; }
    if (::pipe(stderrP) < 0) { closePair(stdinP); closePair(stdoutP); return false; }

    pid_t pid = ::fork();
    if (pid < 0) {
        closePair(stdinP); closePair(stdoutP); closePair(stderrP);
        return false;
    }

    if (pid == 0) {
        // ── Child process ────────────────────────────────────────────────────
        ::dup2(stdinP[0],  STDIN_FILENO);
        ::dup2(stdoutP[1], STDOUT_FILENO);
        ::dup2(stderrP[1], STDERR_FILENO);

        closePair(stdinP);
        closePair(stdoutP);
        closePair(stderrP);

        std::vector<const char*> cargv;
        cargv.reserve(argv.size() + 1);
        for (const auto& a : argv) cargv.push_back(a.c_str());
        cargv.push_back(nullptr);

        ::execvp(cargv[0], const_cast<char* const*>(cargv.data()));
        ::_exit(127); // exec failed
    }

    // ── Parent process ───────────────────────────────────────────────────────
    pid_       = pid;
    stdinPipe_ = stdinP[1];   // write end → child's stdin
    stdoutPipe_= stdoutP[0];  // read end  ← child's stdout
    stderrPipe_= stderrP[0];  // read end  ← child's stderr

    // Close the ends the parent does not use.
    ::close(stdinP[0]);
    ::close(stdoutP[1]);
    ::close(stderrP[1]);

    return true;
}
