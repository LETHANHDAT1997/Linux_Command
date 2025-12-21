# C++ Logger Design Document

## 1. Architecture Overview

### 1.1 Threading Model
```
┌─────────────────────────────────────────────────────────┐
│                   Application Threads                    │
│  Thread 1    Thread 2    Thread 3    ...    Thread N    │
└────┬────────────┬────────────┬──────────────────┬───────┘
     │            │            │                  │
     │ log()      │ log()      │ log()            │ log()
     │ O(1)       │ O(1)       │ O(1)             │ O(1)
     └────┬───────┴────┬───────┴──────────────────┘
          │            │
          ▼            ▼
     ┌────────────────────────────┐
     │   Lock-Free Ring Buffer    │
     │     (Fixed Size Queue)     │
     └────────────┬───────────────┘
                  │
                  │ dequeue
                  ▼
          ┌──────────────────┐
          │  Logger Thread   │
          │  - Write to file │
          │  - Rotate files  │
          │  - Cleanup       │
          └──────────────────┘
                  │
                  ▼
          ┌──────────────────┐
          │  File System     │
          │  app.log         │
          │  app.log.1       │
          │  app.log.2       │
          └──────────────────┘
```

### 1.2 Key Components

**Lock-Free Queue**
- Single-writer-multiple-reader (SWMR) per slot
- Power-of-2 sized ring buffer for fast modulo operations
- Cache-line aligned to prevent false sharing
- Non-blocking enqueue with overflow detection

**Logger Thread**
- Dedicated background thread for I/O operations
- Batch processing of log entries
- Periodic flushing (configurable interval)
- Graceful shutdown with queue draining

**File Management**
- Rotation based on size threshold
- Numbered backup files (app.log → app.log.1 → app.log.2...)
- Automatic deletion of oldest files beyond max count
- Atomic file operations

## 2. Design Decisions & Rationale

### 2.1 Lock-Free Queue

**Why Lock-Free?**
- **Predictable Latency**: No lock contention or priority inversion
- **Real-Time Safe**: Bounded execution time for log calls
- **No Deadlock**: Cannot deadlock even if logger thread blocks
- **Cache Friendly**: Sequential writes to ring buffer

**Trade-offs:**
- Fixed queue size (but configurable at compile time)
- Messages can be dropped if queue fills (counter tracks drops)
- More complex implementation than mutex-based queue

### 2.2 Fixed-Size Log Entries

**Benefits:**
- Enables truly lock-free queue (no dynamic allocation)
- Predictable memory footprint
- Better cache locality
- Simpler memory management

**Implementation:**
```cpp
struct LogEntry {
    char message[512];  // Fixed size
    LogLevel level;
    uint64_t timestamp;
    uint32_t thread_id;
    // Total: ~576 bytes per entry
};
```

**Trade-off:** Messages truncated at MAX_LOG_MESSAGE_SIZE

### 2.3 Single Background Thread

**Advantages:**
- Simple synchronization (only one writer to files)
- Sequential I/O patterns (better for embedded systems)
- Easy to control I/O rate and buffering
- Predictable resource usage

**Consideration:** All file I/O happens on one thread, which is acceptable because:
- Disk I/O is inherently serialized anyway
- Most time is spent in formatting, not I/O
- Queue provides sufficient buffering

### 2.4 Rotation Strategy

**Size-Based Rotation:**
```
app.log          (current, 0-10MB)
app.log.1        (10MB)
app.log.2        (10MB)
app.log.3        (10MB)
app.log.4        (10MB)
app.log.5 → deleted when app.log rotates
```

**Why not time-based?**
- Size-based is more predictable for disk usage
- Simpler to implement without timer dependencies
- Better for embedded systems with limited storage

### 2.5 Memory Ordering

Critical atomic operations use appropriate memory ordering:

```cpp
// Fast path (producers)
if (level < min_level_.load(memory_order_acquire))
    return;  // O(1) filter

// Queue operations
write_pos_.store(pos + 1, memory_order_release);
read_pos_.load(memory_order_acquire);
```

**Benefits:**
- Relaxed ordering where safe (counters)
- Acquire-release for synchronization points
- No unnecessary memory barriers

## 3. Performance Characteristics

### 3.1 Time Complexity

| Operation | Complexity | Notes |
|-----------|------------|-------|
| log() call | O(1) | Lock-free enqueue |
| Level filter | O(1) | Single atomic load |
| Format message | O(n) | n = message length |
| Queue full check | O(1) | Atomic subtraction |
| Dequeue | O(1) | Single consumer |
| File write | O(1) | Buffered I/O |
| Rotation | O(k) | k = max_files (typically 5-10) |

### 3.2 Space Complexity

**Per-Logger Instance:**
```
Queue:        4096 entries × 576 bytes = ~2.3 MB
Overhead:     ~1 KB (atomics, config, state)
Total:        ~2.3 MB fixed allocation
```

**Disk Usage:**
```
Max disk usage = max_file_size × max_files
Example: 10 MB × 5 files = 50 MB maximum
```

### 3.3 Expected Latency

**Application Thread (log call):**
- Best case: ~50-100 ns (level filter + timestamp)
- Typical: ~200-500 ns (format + enqueue)
- Worst case: ~1-2 μs (queue full, message dropped)

**Logger Thread (per entry):**
- Format timestamp: ~100-200 ns
- File write: ~1-10 μs (buffered)
- Rotation: ~10-100 ms (infrequent)

## 4. Configuration Guidelines

### 4.1 Queue Size Selection

```cpp
constexpr size_t LOG_QUEUE_SIZE = 4096;  // Power of 2
```

**Sizing formula:**
```
queue_size = burst_rate × burst_duration / avg_processing_time

Example:
- 10,000 logs/sec burst
- 100ms burst duration  
- 50 μs processing time
→ Need ~1000 entries (use 2048 for safety)
```

### 4.2 File Size Selection

**Embedded Systems:**
- Small flash (< 1GB): 1-5 MB per file
- Medium flash (1-8GB): 5-20 MB per file
- Large storage: 20-100 MB per file

**File Count:**
- Minimum 3 (current + 2 rotated)
- Typical 5-10 for good history
- Maximum limited by directory entry limits

### 4.3 Real-Time Tuning

**For hard real-time systems:**
```cpp
config.max_file_size = 1 * 1024 * 1024;  // 1MB (faster rotation)
config.flush_interval_ms = 100;            // Frequent flush
config.sync_on_error = false;              // Avoid fsync() blocking
```

**For best-effort logging:**
```cpp
config.max_file_size = 50 * 1024 * 1024;  // 50MB
config.flush_interval_ms = 5000;           // Less frequent
config.sync_on_error = true;               // Ensure errors are saved
```

## 5. Thread Safety Guarantees

### 5.1 Safe Operations

**From any thread (lock-free):**
- `log()` and all level-specific methods
- `set_min_level()`
- `get_min_level()`
- `get_stats()`

**From any thread (wait-free):**
- Level filtering check

### 5.2 Synchronization Points

1. **Queue full detection**: Atomic compare of write/read positions
2. **Shutdown**: Atomic flag + queue draining
3. **Statistics**: Atomic counters (relaxed ordering)

### 5.3 Guarantees

- **No deadlock**: Lock-free design
- **No data races**: All shared state is atomic
- **Progress guarantee**: At least one thread makes progress (lock-free)
- **Message ordering**: FIFO per thread (across threads: best-effort)

## 6. Error Handling

### 6.1 Queue Overflow

When queue is full:
```cpp
if (!queue_.try_enqueue(entry)) {
    messages_dropped_.fetch_add(1);
    // Message is lost, but system continues
}
```

**Detection:** Check `get_stats().messages_dropped`

### 6.2 File System Errors

**File open failure:**
- Logger continues running
- Messages are dequeued but not written
- Retry on next log entry

**Disk full:**
- Write returns partial bytes
- Rotation triggered earlier
- Oldest logs deleted to make space

**Rotation failure:**
- Keep writing to current file
- Retry rotation on next size threshold

### 6.3 Resource Limits

**Open file descriptors:**
- Only 1 file open at a time
- Closed before rotation

**Memory:**
- Fixed allocation (no dynamic growth)
- Queue size is compile-time constant

## 7. Platform Considerations

### 7.1 Linux User-Space

**File Operations:**
- Standard POSIX: `fopen()`, `fwrite()`, `fsync()`
- Directory operations: `mkdir()`, `rename()`, `remove()`

**Threading:**
- `std::thread` with pthread backend
- `pthread_setname_np()` for debugging

### 7.2 Embedded Linux

**Considerations:**
- May have slower I/O (SD card, eMMC)
- Increase flush interval to reduce wear
- Consider sync_on_error=false for flash longevity
- Use smaller log files for faster rotation

**Example config:**
```cpp
config.max_file_size = 512 * 1024;     // 512 KB
config.flush_interval_ms = 5000;        // 5 seconds
config.sync_on_error = false;           // Reduce flash wear
```

### 7.3 Embedded RTOS

**Requirements:**
- C++11 or later with `<atomic>` and `<thread>`
- POSIX-like file system API
- At least 3-4 MB RAM for logger

**Adaptations needed:**
```cpp
// Replace std::thread with RTOS task
// Replace std::chrono with RTOS timer
// Replace pthread_self() with RTOS task ID
```

### 7.4 Windows Support

Already included with conditional compilation:
```cpp
#ifdef _WIN32
    #include <windows.h>
    #define mkdir(dir, mode) _mkdir(dir)
    GetCurrentThreadId()  // Thread ID
#endif
```

## 8. Usage Examples

### 8.1 Basic Setup

```cpp
#include "logger.h"

int main() {
    Logger::Config config;
    config.log_directory = "./logs";
    config.log_prefix = "myapp";
    config.max_file_size = 10 * 1024 * 1024;  // 10 MB
    config.max_files = 5;
    config.min_level = LogLevel::INFO;
    
    Logger logger(config);
    
    logger.info("Application started");
    logger.warn("This is a warning: %d", 42);
    logger.error("Error code: %s", "E123");
    
    return 0;
}
```

### 8.2 Multi-Threaded Application

```cpp
// Global or singleton logger
Logger* g_logger = nullptr;

void worker_thread(int id) {
    for (int i = 0; i < 10000; ++i) {
        g_logger->debug("Worker %d processing item %d", id, i);
        
        if (i % 100 == 0) {
            g_logger->info("Worker %d checkpoint: %d items", id, i);
        }
    }
}

int main() {
    Logger::Config config;
    Logger logger(config);
    g_logger = &logger;
    
    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back(worker_thread, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    logger.flush();
    auto stats = logger.get_stats();
    printf("Total logged: %lu, Dropped: %lu\n", 
           stats.messages_logged, stats.messages_dropped);
    
    return 0;
}
```

### 8.3 Runtime Level Control

```cpp
Logger logger(config);

// Start with INFO level
logger.set_min_level(LogLevel::INFO);

// Enable debug logging temporarily
logger.set_min_level(LogLevel::DEBUG);
run_diagnostic_tests();

// Back to INFO
logger.set_min_level(LogLevel::INFO);
```

### 8.4 Monitoring and Alerts

```cpp
void monitor_logger(Logger& logger) {
    while (running) {
        auto stats = logger.get_stats();
        
        if (stats.messages_dropped > 0) {
            // Alert: Queue overflow detected!
            send_alert("Logger dropping messages");
        }
        
        double drop_rate = static_cast<double>(stats.messages_dropped) /
                          static_cast<double>(stats.messages_logged + 
                                            stats.messages_dropped);
        
        if (drop_rate > 0.01) {  // More than 1% dropped
            // Increase queue size or reduce log volume
            logger.set_min_level(LogLevel::WARN);
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(60));
    }
}
```

## 9. Optimization Tips

### 9.1 Reduce Allocations

Current design already minimizes allocations:
- Fixed-size queue (allocated once)
- Stack-based LogEntry in application threads
- No heap allocation in hot path

### 9.2 Cache Optimization

```cpp
// LogEntry is cache-line aligned
struct alignas(64) LogEntry { ... };

// Atomic counters are on separate cache lines
alignas(64) std::atomic<uint64_t> write_pos_;
alignas(64) std::atomic<uint64_t> read_pos_;
```

### 9.3 Minimize String Formatting

```cpp
// Fast path: level check before formatting
if (level < min_level_) return;  // ~50ns

// Only format if needed
va_list args;
va_start(args, format);
vsnprintf(...);  // ~200-500ns
va_end(args);
```

### 9.4 Batch Processing

Logger thread processes all available entries before sleeping:
```cpp
while (queue_.try_dequeue(entry)) {
    write_log_entry(entry);  // Batch write
}
fflush(current_file_);  // Single flush for batch
```

## 10. Testing Strategy

### 10.1 Unit Tests

- Queue operations (enqueue/dequeue)
- Overflow handling
- Level filtering
- File rotation logic
- Thread ID capture
- Timestamp generation

### 10.2 Integration Tests

- Multi-threaded stress test
- File system full scenarios
- Rotation with concurrent logging
- Shutdown with pending messages
- Statistics accuracy

### 10.3 Performance Tests

```cpp
void benchmark_log_call() {
    Logger logger(config);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 1000000; ++i) {
        logger.info("Test message %d", i);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        end - start).count();
    
    printf("Average: %lu ns per log call\n", duration / 1000000);
}
```

### 10.4 Real-Time Analysis

Use tools like:
- `perf` on Linux for profiling
- `valgrind --tool=cachegrind` for cache analysis
- `cyclictest` for latency measurement
- Custom instrumentation for worst-case timing

## 11. Future Enhancements

### 11.1 Possible Extensions

1. **Network logging**: UDP/TCP output option
2. **Compression**: Compress rotated logs (gzip)
3. **Structured logging**: JSON format support
4. **Log filtering**: Regex-based filtering
5. **Multiple outputs**: File + syslog + network
6. **Dynamic configuration**: Runtime reload of settings
7. **Per-module levels**: Different levels per source file/module

### 11.2 Advanced Features

```cpp
// Custom formatter
logger.set_formatter([](const LogEntry& e) {
    return format_as_json(e);
});

// Log filtering
logger.add_filter([](const LogEntry& e) {
    return e.message.find("password") == std::string::npos;
});

// Multiple sinks
logger.add_sink(std::make_unique<FileSink>());
logger.add_sink(std::make_unique<SyslogSink>());
logger.add_sink(std::make_unique<NetworkSink>("10.0.0.1:514"));
```

## 12. Conclusion

This logger design provides:
- ✅ **Predictable latency** (lock-free, O(1) operations)
- ✅ **Thread safety** (safe for concurrent use)
- ✅ **Bounded memory** (fixed allocation)
- ✅ **Automatic rotation** (size-based with cleanup)
- ✅ **Production ready** (error handling, statistics)
- ✅ **Portable** (Linux, embedded Linux, RTOS with minor changes)
- ✅ **Zero dependencies** (only standard library)

The implementation balances real-time constraints with practical usability, making it suitable for embedded systems, real-time applications, and general-purpose use.