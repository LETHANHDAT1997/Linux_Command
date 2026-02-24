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
#include "MarketLogContext.h"

class SingleFileHandler
{
public:
    SingleFileHandler();
    ~SingleFileHandler();
    void start();
    void stop();
    void SetMaxSizeLog(uint32_t maxSize);
    void SetLogContext(LogContext& ctx);

private:
    void run();
    void ExampleMakeFile(const std::string& dir);
    LogContext *m_ctx = nullptr;
    uint32_t    m_MaxSizeLog = 0;
    std::thread m_SingleFileHandlerThread;
    std::atomic<bool> m_running{false};
};