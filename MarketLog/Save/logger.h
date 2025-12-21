// logger.h
#ifndef LOGGER_H
#define LOGGER_H

#include <atomic>
#include <thread>
#include <memory>
#include <string>
#include <cstdint>
#include <array>
#include <cstring>
#include <chrono>

// Configuration constants
constexpr size_t MAX_LOG_MESSAGE_SIZE = 512;
constexpr size_t LOG_QUEUE_SIZE = 4096;  // Must be power of 2
constexpr size_t MAX_LOG_FILE_SIZE = 10 * 1024 * 1024;  // 10 MB
constexpr size_t MAX_LOG_FILES = 5;

// Log levels
enum class LogLevel : uint8_t 
{
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
};

// Log entry structure (fixed size for lock-free queue)
struct alignas(64) LogEntry // Cache line aligned
{  
    std::array<char, MAX_LOG_MESSAGE_SIZE> message;
    LogLevel level;
    uint64_t timestamp_ns;
    uint32_t thread_id;
    uint16_t message_length;
    
    LogEntry() : level(LogLevel::INFO), timestamp_ns(0), 
                 thread_id(0), message_length(0) 
                 {
                    message.fill(0);
                 }
};

// Lock-free SPMC ring buffer (Single Producer Multiple Consumer per slot)
template<typename T, size_t Size>
class LockFreeQueue 
{
    static_assert((Size & (Size - 1)) == 0, "Size must be power of 2");
    
private:
    std::array<T, Size> buffer_;
    alignas(64) std::atomic<uint64_t> write_pos_{0};
    alignas(64) std::atomic<uint64_t> read_pos_{0};
    
public:
    LockFreeQueue() = default;
    
    // Non-blocking enqueue
    bool try_enqueue(const T& item) 
    {
        const uint64_t current_write = write_pos_.load(std::memory_order_relaxed);
        const uint64_t current_read = read_pos_.load(std::memory_order_acquire);
        
        // Check if queue is full
        if (current_write - current_read >= Size) 
        {
            return false;  // Queue full
        }
        
        // Write to buffer
        const size_t index = current_write & (Size - 1);
        buffer_[index] = item;
        
        // Commit write
        write_pos_.store(current_write + 1, std::memory_order_release);
        return true;
    }
    
    // Dequeue (only called by logger thread)
    bool try_dequeue(T& item) 
    {
        const uint64_t current_read = read_pos_.load(std::memory_order_relaxed);
        const uint64_t current_write = write_pos_.load(std::memory_order_acquire);
        
        if (current_read >= current_write) 
        {
            return false;  // Queue empty
        }
        
        const size_t index = current_read & (Size - 1);
        item = buffer_[index];
        
        read_pos_.store(current_read + 1, std::memory_order_release);
        return true;
    }
    
    size_t size() const 
    {
        const uint64_t write = write_pos_.load(std::memory_order_acquire);
        const uint64_t read = read_pos_.load(std::memory_order_acquire);
        return static_cast<size_t>(write - read);
    }
    
    bool empty() const 
    {
        return size() == 0;
    }
};

class Logger 
{
public:
    struct Config 
    {
        std::string log_directory = "./logs";
        std::string log_prefix = "app";
        size_t max_file_size = MAX_LOG_FILE_SIZE;
        size_t max_files = MAX_LOG_FILES;
        LogLevel min_level = LogLevel::INFO;
        bool sync_on_error = true;  // fsync on ERROR/FATAL
        uint32_t flush_interval_ms = 1000;
    };
    
    explicit Logger(const Config& config);
    ~Logger();
    
    // Delete copy/move
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;
    
    // Main logging interface
    void log(LogLevel level, const char* format, ...);
    void trace(const char* format, ...);
    void debug(const char* format, ...);
    void info(const char* format, ...);
    void warn(const char* format, ...);
    void error(const char* format, ...);
    void fatal(const char* format, ...);
    
    // Control methods
    void set_min_level(LogLevel level) 
    {
        min_level_.store(level, std::memory_order_release); 
    }
    
    LogLevel get_min_level() const 
    {
        return min_level_.load(std::memory_order_acquire); 
    }
    
    // Statistics
    struct Stats 
    {
        uint64_t messages_logged;
        uint64_t messages_dropped;
        uint64_t bytes_written;
        uint64_t files_rotated;
    };
    
    Stats get_stats() const;
    void flush();  // Force flush pending logs
    
private:
    void worker_thread();
    void write_log_entry(const LogEntry& entry);
    void rotate_if_needed();
    void rotate_logs();
    void cleanup_old_logs();
    std::string get_current_log_filename() const;
    std::string get_rotated_log_filename(size_t index) const;
    uint64_t get_timestamp_ns() const;
    uint32_t get_thread_id() const;
    
    Config config_;
    LockFreeQueue<LogEntry, LOG_QUEUE_SIZE> queue_;
    
    std::atomic<LogLevel> min_level_;
    std::atomic<bool> running_{true};
    std::thread worker_;
    
    // Worker thread state (no synchronization needed)
    FILE* current_file_{nullptr};
    size_t current_file_size_{0};
    
    // Statistics
    std::atomic<uint64_t> messages_logged_{0};
    std::atomic<uint64_t> messages_dropped_{0};
    std::atomic<uint64_t> bytes_written_{0};
    std::atomic<uint64_t> files_rotated_{0};
    
    // Flush control
    std::chrono::steady_clock::time_point last_flush_;
};

#endif // LOGGER_H

