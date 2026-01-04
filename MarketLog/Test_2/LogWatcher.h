#ifndef LOG_WATCHER_H
#define LOG_WATCHER_H

#include <string>
#include <thread>
#include <atomic>
#include <iostream>
#include <fstream>
#include "MarketLogContext.h"
#include "JKQueue.h"
#include "JKSync.h"
#include "MarketLogRotator.h"

class MarKetLog;

class LogWatcher
{
public:
    explicit LogWatcher(MarKetLog* npMarKetLog);
    ~LogWatcher();

    void start();
    void stop();
    void SetLogContext(LogContext& ctx);
private:
    /* =========================
     * Thread & main loop
     * ========================= */
    void threadLoop();
    void handleEvents();

    /* =========================
     * inotify lifecycle
     * ========================= */
    bool initInotify();
    bool addWatch();
    void removeWatch();
    void cleanupInotify();

private:
    void ClockMutexLogContext(void);
    std::string m_logDir;

    int m_inotifyFd{-1};
    int m_watchFd{-1};

    std::thread m_thread;
    std::atomic<bool> m_running{false};

    /* Con trỏ đến LogContext của MarKetLog */
    LogContext* m_ctx                           = nullptr;

    /* Con trỏ đến MarKetLog */
    MarKetLog*  m_pMarKetLogShareToLogWatcher   = nullptr;
};

#endif