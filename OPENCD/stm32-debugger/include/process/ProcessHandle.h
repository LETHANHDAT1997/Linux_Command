#pragma once
#include <string>
#include <vector>
#include <sys/types.h>

// RAII wrapper around a child process with stdin / stdout / stderr pipes.
// Derived classes call spawnProcess() and expose a typed start() method.

class ProcessHandle {
public:
    virtual ~ProcessHandle() { terminate(); }

    bool isRunning() const;

    // Sends SIGTERM; if the process does not exit within ~2 s, sends SIGKILL.
    // Closes all pipe fds and resets pid_ to -1.
    void terminate();

    int stdinFd()  const { return stdinPipe_;  }
    int stdoutFd() const { return stdoutPipe_; }
    int stderrFd() const { return stderrPipe_; }

protected:
    // Forks, creates stdin/stdout/stderr pipes, and execs argv[0].
    // Returns false if fork() or exec() fails.
    bool spawnProcess(const std::vector<std::string>& argv);

    pid_t pid_        = -1;
    int   stdinPipe_  = -1;
    int   stdoutPipe_ = -1;
    int   stderrPipe_ = -1;
};
