
#pragma once
#include <string>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <unordered_map>
#include <memory>
#include <queue>
#include <filesystem>
#include <thread>
#include <iostream>
#include <fstream>
#include <random>      
#include <algorithm>  
#include <deque>
#include "MarketLogRotator.h"
#include "MarKetLogConfig.h"
#include "LogWatcher.h"

enum MARKETLOG_STATE_e
{
    MARKETLOG_STATE_NONE,
    MARKETLOG_STATE_ACC_ON,
    MARKETLOG_STATE_ACC_OFF
};

class MarKetLog
{
public:
    MarKetLog();
    ~MarKetLog();
    void start();
    void stop();
    void ScanExistingLogs(const std::string& logDir, const std::string& extension);

    /* Method MarKetLog giao tiếp với Rotator và FileHandle */
    void MarKetLogNotifyToRotator(ROTATOR_FILES_e &npMs);
    void MarKetLogSendNotify(MARKETLOG_STATE_e &npMs);

private:
    LogContext          m_Context;
    LogRotator          m_Rotator;
    LogWatcher          m_Watcher;
    std::thread         m_MarKetLogThread;
    MarKetLogConfig     m_CMarKetLogYAML;

    void StartThreadRotator(void);
    void StopThreadRotator(void);

    void StartThreadLogWatcher(void);
    void StopThreadLogWatcher(void);

    /* Thử nghiệm JCK */
    void MainThreadJKC(void);
    JKQueue<MARKETLOG_STATE_e> m_eQueue;

    /* Thử nghiệm JKSync */
    void MarKetLogClockMutexLogContext(void);

    /* Thử nghiệm JKQueue */
    JKQueue<MARKETLOG_STATE_e>::EN_STS MarKetLogWaitReceiveQueueSignal(MARKETLOG_STATE_e &npMs);
    JKQueue<MARKETLOG_STATE_e>::EN_STS MarKetLogSendQueueSignal(MARKETLOG_STATE_e &npMs);
};
