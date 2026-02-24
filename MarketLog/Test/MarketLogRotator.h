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

class LogRotator
{
public:
    LogRotator();
    ~LogRotator();
    void start();
    void stop();
    void printdebug();
    void SetMaxFiles(int maxFiles);
    void SetLogContext(LogContext& ctx);

private:
    void run();
    void rotate();
    std::string GetDirectory(const std::string& path);
    std::string GetFilename(const std::string& path);
    std::string RotateFileName(const std::string& base);
    void SplitExtension(const std::string& filename, std::string& name, std::string& ext);
    void SplitNumericSuffix(const std::string& name, std::string& prefix, int& index);
    void DeleteFilesWhenExceedLimit(void);
    void RotateFiles(void);

    // std::size_t m_maxSize;
    LogContext* m_ctx = nullptr;
    int m_maxFiles;
    std::thread m_LogRotatorThread;
    std::atomic<bool> m_running{false};
};