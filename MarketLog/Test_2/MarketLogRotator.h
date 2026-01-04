#ifndef MARKET_LOG_ROTATOR_H
#define MARKET_LOG_ROTATOR_H

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
#include "MarketLogContext.h"
#include "JKQueue.h"
#include "JKSync.h"

class MarKetLog;

class LogRotator
{
public:
    LogRotator(MarKetLog* npMarKetLog);
    ~LogRotator();
    void start();
    void stop();
    void SetMaxFiles(int maxFiles);
    void SetLogContext(LogContext& ctx);

    /* Giao tiếp với Rotator */
    void NotifyToRotator(ROTATOR_FILES_e &npMs);

private:
    std::string RotateFileName(const std::string& filepath);
    void rotate();
    void DeleteFilesByFileCountLimit(void);
    void RotateFiles(void);
    void printdebug();

    /* Con trỏ đến LogContext của MarKetLog */
    LogContext* m_ctx                        = nullptr;

    /* Con trỏ đến MarKetLog */
    MarKetLog*  m_pMarKetLogShareToRotator   = nullptr;

    /* Luồng chính của Rotator */
    std::thread m_LogRotatorThread;

    /* Số lượng tệp hỗ trợ */
    uint32_t m_maxFiles;
    
    /* Thử nghiệm JKQueue */
    JKQueue<ROTATOR_FILES_e> m_eQueue;
    void RunQueue(void);

    /* Thử nghiệm JKSync */
    void ClockMutexLogContext(void);

    /* Thử nghiệm JKQueue */
    JKQueue<ROTATOR_FILES_e>::EN_STS WaitReceiveQueueSignal(ROTATOR_FILES_e &npMs);
    JKQueue<ROTATOR_FILES_e>::EN_STS SendQueueSignal(ROTATOR_FILES_e &npMs);

};

#endif
