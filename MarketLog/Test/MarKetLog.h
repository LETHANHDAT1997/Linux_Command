
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
#include "MarKetLogFileHandle.h"

class MarKetLog
{
public:
    MarKetLog();
    ~MarKetLog();
    uint32_t ExtractLogIndex(const std::string& fullpath);
    void ScanExistingLogs(const std::string& logDir, const std::string& extension);
    void StartThreadRotator(void);
    void StopThreadRotator(void);
    void StartThreadFileHandle(void);
    void StopThreadFileHandle(void);

private:
    LogContext          m_Context;
    LogRotator          m_Rotator;
    SingleFileHandler   m_FileHandler;
    std::thread m_MarKetLogThread;
    std::thread m_FileHandleThread;
};