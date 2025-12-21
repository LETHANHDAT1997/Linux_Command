// journal_logger.h
#ifndef JOURNAL_LOGGER_H
#define JOURNAL_LOGGER_H

#include <systemd/sd-journal.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <memory>
#include <cstdio>
#include <chrono>

class JournalLogger {
public:
    struct Config {
        std::string log_directory = "./logs";
        std::string log_prefix = "app";
        size_t max_file_size = 10 * 1024 * 1024;  // 10 MB
        size_t max_files = 5;
        uint32_t flush_interval_ms = 1000;
        
        // Filter settings - only capture logs from these identifiers
        std::vector<std::string> syslog_identifiers;  // e.g., {"myapp1", "myapp2"}
        std::string match_unit;  // Optional: systemd unit filter
    };

    struct Stats 
    {
        uint64_t messages_logged;
        uint64_t bytes_written;
        uint64_t files_rotated;
    };

    explicit JournalLogger(const Config& config);
    ~JournalLogger();

    // Delete copy/move
    JournalLogger(const JournalLogger&) = delete;
    JournalLogger& operator=(const JournalLogger&) = delete;

    Stats get_stats() const;
    void flush();

private:
    void worker_thread();
    void write_log_entry(const char* timestamp, const char* identifier, 
                        const char* priority, const char* message);
    void rotate_if_needed();
    void rotate_logs();
    void cleanup_old_logs();
    std::string get_current_log_filename() const;
    std::string get_rotated_log_filename(size_t index) const;

    Config config_;
    std::atomic<bool> running_{true};
    std::thread worker_;
    
    // Worker thread state
    FILE* current_file_{nullptr};
    size_t current_file_size_{0};
    
    // Statistics
    std::atomic<uint64_t> messages_logged_{0};
    std::atomic<uint64_t> bytes_written_{0};
    std::atomic<uint64_t> files_rotated_{0};
    
    std::chrono::steady_clock::time_point last_flush_;
};

#endif // JOURNAL_LOGGER_H

