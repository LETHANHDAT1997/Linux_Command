#include <syslog.h>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <chrono> //thư viện đo thời gian thực hiện các tác vụ.

#define COLOR_RESET        "\x1B[0m"

// --- Màu cơ bản (Normal) ---
#define COLOR_BLACK        "\x1B[30m"
#define COLOR_RED          "\x1B[31m"
#define COLOR_GREEN        "\x1B[32m"
#define COLOR_YELLOW       "\x1B[33m"
#define COLOR_BLUE         "\x1B[34m"
#define COLOR_MAGENTA      "\x1B[35m"
#define COLOR_CYAN         "\x1B[36m"
#define COLOR_WHITE        "\x1B[37m"

/* --- Màu sáng (Bright) --- */
#define COLOR_BRIGHT_BLACK   "\x1B[90m"  // Xám đậm
#define COLOR_BRIGHT_RED     "\x1B[91m"
#define COLOR_BRIGHT_GREEN   "\x1B[92m"
#define COLOR_BRIGHT_YELLOW  "\x1B[93m"
#define COLOR_BRIGHT_BLUE    "\x1B[94m"
#define COLOR_BRIGHT_MAGENTA "\x1B[95m"
#define COLOR_BRIGHT_CYAN    "\x1B[96m"
#define COLOR_BRIGHT_WHITE   "\x1B[97m"

/* Mã màu cho việc hiển thị thông tin */
#define LOG_RESET(msg, ...)     do  {   \
                                        char buffer[256]; \
                                        snprintf(buffer, sizeof(buffer), COLOR_RESET "[RESET] [%s:%d][%s] " msg "\r\n" COLOR_RESET, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
                                        syslog(LOG_INFO,buffer); \
                                    } while(0)

#define LOG_INFOR(msg, ...)     do  {   \
                                        char buffer[256]; \
                                        snprintf(buffer, sizeof(buffer), COLOR_BRIGHT_GREEN "[INFOR] [%s:%d][%s] " msg "\r\n" COLOR_RESET, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
                                        syslog(LOG_INFO,buffer); \
                                    } while(0)

#define LOG_WARNING(msg, ...)   do  {   \
                                        char buffer[256]; \
                                        snprintf(buffer, sizeof(buffer), COLOR_YELLOW "[WARNI] [%s:%d][%s] " msg "\r\n" COLOR_RESET, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
                                        syslog(LOG_INFO,buffer); \
                                    } while(0)

#define LOG_ERROR(msg, ...)     do  {   \
                                        char buffer[256]; \
                                        snprintf(buffer, sizeof(buffer), COLOR_RED "[ERROR] [%s:%d][%s] " msg "\r\n" COLOR_RESET, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
                                        syslog(LOG_INFO,buffer); \
                                    } while(0)

#define LOG_DEBUG(msg, ...)     do  {   \
                                        char buffer[256]; \
                                        snprintf(buffer, sizeof(buffer), COLOR_RESET "[DEBUG] [%s:%d][%s] " msg "\r\n" COLOR_RESET, __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
                                        syslog(LOG_INFO,buffer); \
                                    } while(0)


int main(void)
{
    openlog("MarketLog", LOG_PID, LOG_USER);

    for (int i = 0; i < 20; ++i) 
    {
        LOG_INFOR("Hello from app_a, count=%d", i);
        sleep(1);
    }

    closelog();
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