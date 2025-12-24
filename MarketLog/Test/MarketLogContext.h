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

constexpr int MAX_FILES_LOG = 15;
constexpr int MAX_SIZE_LOG  = 4096;
constexpr const char* DEFAULT_DIR_PATH  = "./stress_logs";
constexpr const char* DEFAULT_EXTENSION = ".log";

// enum class LogEvent
// {
//     FILE_UPDATED,
//     ROTATE_DONE
// };

struct LogFileInfo
{
    std::string path;
    std::size_t size;
};

class LogContext
{
public:
    std::deque<LogFileInfo> files;
    std::condition_variable cvRotate;
    std::condition_variable cvRotateDone;

    // trạng thái hệ thống
    std::atomic<bool> rotateRequested{false};
    std::atomic<bool> rotating{false};

    // event nhẹ (SPSC)
    // std::atomic<LogEvent> lastEvent{LogEvent::FILE_UPDATED};

    std::mutex filesMutex;
};