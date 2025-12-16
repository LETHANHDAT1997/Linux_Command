// logger.cpp
#include <cstdio>
#include <cstdarg>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <algorithm>
#include <vector>
#include <ctime>
#include "logger.h"
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define mkdir(dir, mode) _mkdir(dir)
#else
#include <unistd.h>
#include <pthread.h>
#endif

Logger::Logger(const Config& config) 
:   config_(config), min_level_(config.min_level),
    last_flush_(std::chrono::steady_clock::now()) 
{
    
    // Create log directory if it doesn't exist
    mkdir(config_.log_directory.c_str(), 0755);
    
    // Start worker thread
    worker_ = std::thread(&Logger::worker_thread, this);
    
    // Set thread name for debugging
#ifdef __linux__
    pthread_setname_np(worker_.native_handle(), "LoggerWorker");
#endif
}

Logger::~Logger() 
{
    // Signal shutdown
    running_.store(false, std::memory_order_release);
    
    // Wait for worker to finish
    if (worker_.joinable()) 
    {
        worker_.join();
    }
    
    // Close current file
    if (current_file_) 
    {
        fflush(current_file_);
        fclose(current_file_);
    }
}

void Logger::log(LogLevel level, const char* format, ...) 
{
    // Quick filter check
    if (level < min_level_.load(std::memory_order_acquire)) 
    {
        return;
    }
    
    LogEntry entry;
    entry.level = level;
    entry.timestamp_ns = get_timestamp_ns();
    entry.thread_id = get_thread_id();
    
    // Format message
    va_list args;
    va_start(args, format);
    int written = vsnprintf(entry.message.data(), 
                           MAX_LOG_MESSAGE_SIZE - 1, 
                           format, args);
    va_end(args);
    
    if (written > 0) 
    {
        entry.message_length = std::min(written, 
                                       static_cast<int>(MAX_LOG_MESSAGE_SIZE - 1));
    }
    
    // Try to enqueue (non-blocking)
    if (!queue_.try_enqueue(entry)) 
    {
        messages_dropped_.fetch_add(1, std::memory_order_relaxed);
    }
}

void Logger::trace(const char* format, ...) 
{
    if (LogLevel::TRACE < min_level_.load(std::memory_order_acquire)) return;
    va_list args;
    va_start(args, format);
    LogEntry entry;
    entry.level = LogLevel::TRACE;
    entry.timestamp_ns = get_timestamp_ns();
    entry.thread_id = get_thread_id();
    vsnprintf(entry.message.data(), MAX_LOG_MESSAGE_SIZE - 1, format, args);
    va_end(args);
    if (!queue_.try_enqueue(entry)) messages_dropped_.fetch_add(1, std::memory_order_relaxed);
}

void Logger::debug(const char* format, ...) 
{
    if (LogLevel::DEBUG < min_level_.load(std::memory_order_acquire)) return;
    va_list args;
    va_start(args, format);
    LogEntry entry;
    entry.level = LogLevel::DEBUG;
    entry.timestamp_ns = get_timestamp_ns();
    entry.thread_id = get_thread_id();
    vsnprintf(entry.message.data(), MAX_LOG_MESSAGE_SIZE - 1, format, args);
    va_end(args);
    if (!queue_.try_enqueue(entry)) messages_dropped_.fetch_add(1, std::memory_order_relaxed);
}

void Logger::info(const char* format, ...) 
{
    if (LogLevel::INFO < min_level_.load(std::memory_order_acquire)) return;
    va_list args;
    va_start(args, format);
    LogEntry entry;
    entry.level = LogLevel::INFO;
    entry.timestamp_ns = get_timestamp_ns();
    entry.thread_id = get_thread_id();
    vsnprintf(entry.message.data(), MAX_LOG_MESSAGE_SIZE - 1, format, args);
    va_end(args);
    if (!queue_.try_enqueue(entry)) messages_dropped_.fetch_add(1, std::memory_order_relaxed);
}

void Logger::warn(const char* format, ...) 
{
    if (LogLevel::WARN < min_level_.load(std::memory_order_acquire)) return;
    va_list args;
    va_start(args, format);
    LogEntry entry;
    entry.level = LogLevel::WARN;
    entry.timestamp_ns = get_timestamp_ns();
    entry.thread_id = get_thread_id();
    vsnprintf(entry.message.data(), MAX_LOG_MESSAGE_SIZE - 1, format, args);
    va_end(args);
    if (!queue_.try_enqueue(entry)) messages_dropped_.fetch_add(1, std::memory_order_relaxed);
}

void Logger::error(const char* format, ...) 
{
    if (LogLevel::ERROR < min_level_.load(std::memory_order_acquire)) return;
    va_list args;
    va_start(args, format);
    LogEntry entry;
    entry.level = LogLevel::ERROR;
    entry.timestamp_ns = get_timestamp_ns();
    entry.thread_id = get_thread_id();
    vsnprintf(entry.message.data(), MAX_LOG_MESSAGE_SIZE - 1, format, args);
    va_end(args);
    if (!queue_.try_enqueue(entry)) messages_dropped_.fetch_add(1, std::memory_order_relaxed);
}

void Logger::fatal(const char* format, ...) 
{
    va_list args;
    va_start(args, format);
    LogEntry entry;
    entry.level = LogLevel::FATAL;
    entry.timestamp_ns = get_timestamp_ns();
    entry.thread_id = get_thread_id();
    vsnprintf(entry.message.data(), MAX_LOG_MESSAGE_SIZE - 1, format, args);
    va_end(args);
    if (!queue_.try_enqueue(entry)) messages_dropped_.fetch_add(1, std::memory_order_relaxed);
}

void Logger::worker_thread() 
{
    LogEntry entry;
    
    while (running_.load(std::memory_order_acquire) || !queue_.empty()) 
    {
        bool processed = false;
        
        // Process all available entries
        while (queue_.try_dequeue(entry)) 
        {
            write_log_entry(entry);
            processed = true;
        }
        
        // Periodic flush
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_flush_).count();
        
        if (current_file_ && elapsed >= config_.flush_interval_ms) 
        {
            fflush(current_file_);
            last_flush_ = now;
        }
        
        if (!processed) 
        {
            // Sleep briefly if queue is empty
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }
    
    // Final flush
    if (current_file_) 
    {
        fflush(current_file_);
    }
}

void Logger::write_log_entry(const LogEntry& entry) 
{
    // Open file if needed
    if (!current_file_) 
    {
        std::string filename = get_current_log_filename();
        current_file_ = fopen(filename.c_str(), "a");
        if (!current_file_) 
        {
            return;  // Failed to open file
        }
        
        // Get current file size
        fseek(current_file_, 0, SEEK_END);
        current_file_size_ = ftell(current_file_);
    }
    
    // Format timestamp
    uint64_t seconds = entry.timestamp_ns / 1000000000ULL;
    uint64_t nanos = entry.timestamp_ns % 1000000000ULL;
    time_t time_sec = static_cast<time_t>(seconds);
    struct tm tm_info;
    localtime_r(&time_sec, &tm_info);
    
    // Level string
    const char* level_str[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    const char* level = level_str[static_cast<int>(entry.level)];
    
    // Write log line
    char buffer[1024];
    int len = snprintf(buffer, sizeof(buffer),
                      "[%04d-%02d-%02d %02d:%02d:%02d.%09lu] [%s] [T%u] %s\n",
                      tm_info.tm_year + 1900, tm_info.tm_mon + 1, tm_info.tm_mday,
                      tm_info.tm_hour, tm_info.tm_min, tm_info.tm_sec,
                      nanos, level, entry.thread_id, entry.message.data());
    
    if (len > 0) 
    {
        size_t written = fwrite(buffer, 1, len, current_file_);
        current_file_size_ += written;
        bytes_written_.fetch_add(written, std::memory_order_relaxed);
        messages_logged_.fetch_add(1, std::memory_order_relaxed);
        
        // Sync on error/fatal if configured
        if  (   config_.sync_on_error && 
                (entry.level == LogLevel::ERROR || entry.level == LogLevel::FATAL)
            )
        {
            fflush(current_file_);
            fsync(fileno(current_file_));
        }
    }
    
    // Check if rotation needed
    rotate_if_needed();
}

void Logger::rotate_if_needed() 
{
    if (current_file_size_ >= config_.max_file_size) 
    {
        rotate_logs();
    }
}

void Logger::rotate_logs() 
{
    if (current_file_) 
    {
        fflush(current_file_);
        fclose(current_file_);
        current_file_ = nullptr;
        current_file_size_ = 0;
    }
    
    // Rename existing log files
    for (int i = static_cast<int>(config_.max_files) - 2; i >= 0; --i) 
    {
        std::string old_name = (i == 0) ? get_current_log_filename() : get_rotated_log_filename(i);
        std::string new_name = get_rotated_log_filename(i + 1);
        
        rename(old_name.c_str(), new_name.c_str());
    }
    
    files_rotated_.fetch_add(1, std::memory_order_relaxed);
    cleanup_old_logs();
}

void Logger::cleanup_old_logs() 
{
    // Delete files beyond max_files limit
    for (size_t i = config_.max_files; i < config_.max_files + 10; ++i) 
    {
        std::string filename = get_rotated_log_filename(i);
        remove(filename.c_str());
    }
}

std::string Logger::get_current_log_filename() const 
{
    return config_.log_directory + "/" + config_.log_prefix + ".log";
}

std::string Logger::get_rotated_log_filename(size_t index) const 
{
    return config_.log_directory + "/" + config_.log_prefix + 
           ".log." + std::to_string(index);
}

uint64_t Logger::get_timestamp_ns() const 
{
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
}

uint32_t Logger::get_thread_id() const 
{
#ifdef _WIN32
    return static_cast<uint32_t>(GetCurrentThreadId());
#else
    return static_cast<uint32_t>(pthread_self());
#endif
}

Logger::Stats Logger::get_stats() const 
{
    Stats stats;
    stats.messages_logged   = messages_logged_.load(std::memory_order_acquire);
    stats.messages_dropped  = messages_dropped_.load(std::memory_order_acquire);
    stats.bytes_written     = bytes_written_.load(std::memory_order_acquire);
    stats.files_rotated     = files_rotated_.load(std::memory_order_acquire);
    return stats;
}

void Logger::flush() 
{
    // Wait for queue to drain
    while (!queue_.empty()) 
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Note: Worker thread will flush periodically
}

// Example usage
// #ifdef LOGGER_EXAMPLE_MAIN
int main() 
{
    Logger::Config config;
    config.log_directory = "./logs";
    config.log_prefix = "myapp";
    config.max_file_size = 1024 * 1024;  // 1 MB
    config.max_files = 5;
    config.min_level = LogLevel::DEBUG;
    
    Logger logger(config);
    
    // Multi-threaded logging example
    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) 
    {
        threads.emplace_back([&logger, i]() 
        {
            for (int j = 0; j < 1000; ++j) 
            {
                logger.info("Thread %d: Message %d", i, j);
                logger.warn("Thread %d: Warning %d", i, j);
                logger.error("Thread %d: Error %d", i, j);
            }
        });
    }
    
    for (auto& t : threads) 
    {
        t.join();
    }
    
    logger.flush();
    
    auto stats = logger.get_stats();
    printf("Logged: %lu, Dropped: %lu, Bytes: %lu, Rotations: %lu\n",
           stats.messages_logged, stats.messages_dropped,
           stats.bytes_written, stats.files_rotated);
    
    return 0;
}
// #endif