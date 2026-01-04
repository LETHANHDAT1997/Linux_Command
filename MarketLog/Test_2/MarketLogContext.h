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
#include "JKSync.h"

enum ROTATOR_FILES_e
{
    ROTATOR_FILES_STATE_ROTATOR_ACC_ON,
    ROTATOR_FILES_STATE_ROTATOR_ACC_OFF
};

/* Cấu hình cho các chức năng ring buffer */
constexpr const uint32_t    CAPACITY_DEFAULT_WRITE_LOG_BUFFER   = 32*1024;          // Đơn vị byte, tránh để quá nhỏ làm đọc buffer chậm hơn ghi vào buffer, tránh việc để quá lớn làm tốn tài nguyên
constexpr const uint32_t    CAPACITY_DEFAULT_LOG_BUFFER         = 1*1024*1024;      // Đơn vị byte, nên chọn mức dung lượng đảm bảo việc đọc và ghi buffer luôn đủ tải.

struct LogFileInfo
{
    std::string path; // đường dẫn
    std::size_t size; // kích thước tệp
};

class LogContext
{
public:
    /* Danh sách tệp log */
    std::deque<LogFileInfo> files;

    /* Thông tin config */
    double      m_VerSion;
    uint32_t    m_MaxLogFiles;
    uint32_t    m_MaxSizeLogFile;

    std::string m_DirPath;
    std::string m_FileNameWithoutIndex;
    std::string m_Extension;
    std::string m_TempFileName;

    /* Nghiên cứu JKC */
    JKSync m_cLogContextMutex;
};
