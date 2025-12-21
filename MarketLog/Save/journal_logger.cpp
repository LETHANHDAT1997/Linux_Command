// journal_logger.cpp
#include "journal_logger.h"
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>

JournalLogger::JournalLogger(const Config& config)
    : config_(config), last_flush_(std::chrono::steady_clock::now()) 
{
    
    // Create log directory
    mkdir(config_.log_directory.c_str(), 0755);
    
    // Start worker thread
    worker_ = std::thread(&JournalLogger::worker_thread, this);
}

JournalLogger::~JournalLogger() {
    running_.store(false, std::memory_order_release);
    
    if (worker_.joinable()) {
        worker_.join();
    }
    
    if (current_file_) {
        fflush(current_file_);
        fclose(current_file_);
    }
}

void JournalLogger::worker_thread() {
    sd_journal* journal = nullptr;
    
    // Open journal for reading
    int ret = sd_journal_open(&journal, SD_JOURNAL_LOCAL_ONLY);
    if (ret < 0) {
        fprintf(stderr, "Failed to open journal: %s\n", strerror(-ret));
        return;
    }
    
    // Add filters for your services
    for (const auto& identifier : config_.syslog_identifiers) {
        std::string match = "SYSLOG_IDENTIFIER=" + identifier;
        sd_journal_add_match(journal, match.c_str(), 0);
    }
    
    // If you want to filter by systemd unit instead
    if (!config_.match_unit.empty()) {
        std::string match = "_SYSTEMD_UNIT=" + config_.match_unit;
        sd_journal_add_match(journal, match.c_str(), 0);
    }
    
    // Seek to end (only new entries)
    sd_journal_seek_tail(journal);
    sd_journal_previous(journal);  // Move back one to avoid missing first entry
    
    // Poll for new entries
    while (running_.load(std::memory_order_acquire)) {
        ret = sd_journal_wait(journal, 100000);  // 100ms timeout
        
        if (ret < 0) {
            fprintf(stderr, "Journal wait failed: %s\n", strerror(-ret));
            break;
        }
        
        // Process all new entries
        while (sd_journal_next(journal) > 0) {
            const void* data;
            size_t length;
            
            // Extract fields
            const char* message = nullptr;
            const char* priority = nullptr;
            const char* identifier = nullptr;
            const char* timestamp = nullptr;
            uint64_t timestamp_us = 0;
            
            if (sd_journal_get_data(journal, "MESSAGE", &data, &length) >= 0) {
                message = (const char*)data + 8;  // Skip "MESSAGE="
            }
            
            if (sd_journal_get_data(journal, "PRIORITY", &data, &length) >= 0) {
                priority = (const char*)data + 9;  // Skip "PRIORITY="
            }
            
            if (sd_journal_get_data(journal, "SYSLOG_IDENTIFIER", &data, &length) >= 0) {
                identifier = (const char*)data + 18;  // Skip "SYSLOG_IDENTIFIER="
            }
            
            // Get realtime timestamp
            if (sd_journal_get_realtime_usec(journal, &timestamp_us) >= 0) {
                time_t sec = timestamp_us / 1000000;
                uint32_t usec = timestamp_us % 1000000;
                struct tm tm_info;
                localtime_r(&sec, &tm_info);
                
                static char ts_buffer[64];
                snprintf(ts_buffer, sizeof(ts_buffer),
                        "%04d-%02d-%02d %02d:%02d:%02d.%06u",
                        tm_info.tm_year + 1900, tm_info.tm_mon + 1, 
                        tm_info.tm_mday, tm_info.tm_hour, 
                        tm_info.tm_min, tm_info.tm_sec, usec);
                timestamp = ts_buffer;
            }
            
            if (message && identifier) {
                write_log_entry(timestamp, identifier, priority, message);
            }
        }
        
        // Periodic flush
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_flush_).count();
        
        if (current_file_ && elapsed >= config_.flush_interval_ms) {
            fflush(current_file_);
            last_flush_ = now;
        }
    }
    
    // Cleanup
    if (current_file_) {
        fflush(current_file_);
    }
    
    sd_journal_close(journal);
}

void JournalLogger::write_log_entry(const char* timestamp, 
                                     const char* identifier,
                                     const char* priority, 
                                     const char* message) {
    // Open file if needed
    if (!current_file_) {
        std::string filename = get_current_log_filename();
        current_file_ = fopen(filename.c_str(), "a");
        if (!current_file_) {
            return;
        }
        
        fseek(current_file_, 0, SEEK_END);
        current_file_size_ = ftell(current_file_);
    }
    
    // Map priority to log level
    const char* level_map[] = {
        "EMERG", "ALERT", "CRIT", "ERROR", 
        "WARN", "NOTICE", "INFO", "DEBUG"
    };
    
    int prio = priority ? atoi(priority) : 6;
    if (prio < 0 || prio > 7) prio = 6;
    
    // Write log line
    char buffer[2048];
    int len = snprintf(buffer, sizeof(buffer),
                      "[%s] [%s] [%s] %s\n",
                      timestamp ? timestamp : "unknown",
                      level_map[prio],
                      identifier ? identifier : "unknown",
                      message);
    
    if (len > 0) {
        size_t written = fwrite(buffer, 1, len, current_file_);
        current_file_size_ += written;
        bytes_written_.fetch_add(written, std::memory_order_relaxed);
        messages_logged_.fetch_add(1, std::memory_order_relaxed);
    }
    
    rotate_if_needed();
}

void JournalLogger::rotate_if_needed() {
    if (current_file_size_ >= config_.max_file_size) {
        rotate_logs();
    }
}

void JournalLogger::rotate_logs() {
    if (current_file_) {
        fflush(current_file_);
        fclose(current_file_);
        current_file_ = nullptr;
        current_file_size_ = 0;
    }
    
    // Rotate files
    for (int i = static_cast<int>(config_.max_files) - 2; i >= 0; --i) {
        std::string old_name = (i == 0) ? 
            get_current_log_filename() : get_rotated_log_filename(i);
        std::string new_name = get_rotated_log_filename(i + 1);
        rename(old_name.c_str(), new_name.c_str());
    }
    
    files_rotated_.fetch_add(1, std::memory_order_relaxed);
    cleanup_old_logs();
}

void JournalLogger::cleanup_old_logs() {
    for (size_t i = config_.max_files; i < config_.max_files + 10; ++i) {
        std::string filename = get_rotated_log_filename(i);
        remove(filename.c_str());
    }
}

std::string JournalLogger::get_current_log_filename() const {
    return config_.log_directory + "/" + config_.log_prefix + ".log";
}

std::string JournalLogger::get_rotated_log_filename(size_t index) const {
    return config_.log_directory + "/" + config_.log_prefix + 
           ".log." + std::to_string(index);
}

JournalLogger::Stats JournalLogger::get_stats() const {
    Stats stats;
    stats.messages_logged = messages_logged_.load(std::memory_order_acquire);
    stats.bytes_written = bytes_written_.load(std::memory_order_acquire);
    stats.files_rotated = files_rotated_.load(std::memory_order_acquire);
    return stats;
}

void JournalLogger::flush() {
    if (current_file_) {
        fflush(current_file_);
    }
}

// Example: How your services should log
// In your C++ services:
/*
#include <systemd/sd-journal.h>

void log_to_journal() {
    sd_journal_send(
        "MESSAGE=Application started",
        "PRIORITY=6",  // Info level
        "SYSLOG_IDENTIFIER=myapp1",
        NULL
    );
    
    // Or with printf-style formatting
    sd_journal_print(LOG_INFO, "Processing request ID: %d", request_id);
}
*/

// Main example
int main() {
    JournalLogger::Config config;
    config.log_directory = "./logs";
    config.log_prefix = "aggregated";
    config.max_file_size = 5 * 1024 * 1024;  // 5 MB
    config.max_files = 10;
    config.flush_interval_ms = 1000;
    
    // Only capture logs from your services
    config.syslog_identifiers = {"myapp1", "myapp2", "myservice3"};
    
    JournalLogger logger(config);
    
    printf("Journal Logger Service started. Press Ctrl+C to stop.\n");
    
    // Keep running (in real service, handle signals properly)
    while (true) 
    {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        
        auto stats = logger.get_stats();
        printf("Stats - Logged: %lu, Bytes: %lu, Rotations: %lu\n",
               stats.messages_logged, stats.bytes_written, 
               stats.files_rotated);
    }
    
    return 0;
}

/*
// Lưu log theo service
journalctl -u systemd-journald.service > /home/ledat/Documents/MarKetLog/temp.log

// xem các service dang chạy
systemctl list-units --type=service --state=running

// Lưu log theo user
journalctl _UID=$(id -u ledat) > /home/ledat/Documents/MarKetLog/temp.log

// Xem dung lượng systemd-MarketLog.service đã sử dụng
journalctl -u systemd-MarketLog.service --disk-usage

// Tính dung lượng log của một service cụ thể
journalctl -u MarketLog -o verbose | awk '/MESSAGE=/ {size += length($0)+1} END {print size}'

// Filed theo ident
journalctl SYSLOG_IDENTIFIER=MarketLog -o verbose | awk '/MESSAGE=/ {size += length($0)+1} END {print size}'

*/