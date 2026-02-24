#pragma once

#include <systemd/sd-journal.h>
#include <string>
#include <vector>
#include <optional>

/**
 * Trusted fields
 * VD:
 *  _SYSTEMD_UNIT
 *  _PID
 *  _UID
 *  _GID
 *  _COMM
 * _EXE
 *  _BOOT_ID
 * DD:
 *  Bắt đầu bằng dấu gạch dưới _
 *  Do systemd / kernel tự gán
 *  Không thể giả mạo từ ứng dụng
 *  Dùng cho: filtering, security, auditing
 *  */ 

/**
 * Standard syslog fields
 * VD:
 *  SYSLOG_IDENTIFIER
 *  PRIORITY
 *  MESSAGE
 * DD:
 *  Tương thích syslog truyền thống
 *  Ứng dụng có thể set
 *  Journald hiểu ngữ nghĩa
 *  */ 

/**
 * Custom / Application-defined fields
 * VD:
 *  MY_APP_STATE=INIT
 *  SENSOR_ID=3
 *  REQ_ID=abc123
 * DD:
 *  Do ứng dụng tự định nghĩa
 *  Dạng KEY=value
 *  */ 

enum class JournalField
{
    SystemdUnit,        // Trusted
    Pid,                // Trusted
    SyslogIdentifier,   // Syslog
    Priority,           // Syslog
    Message,            // Syslog
    Custom              // Custom
};

/**
 * @brief Đại diện cho MỘT bản ghi (entry) trong systemd journal.
 *
 * JournalEntry là cấu trúc dữ liệu "giải mã" từ journal thô (key=value),
 * dùng để truyền dữ liệu ra ngoài một cách thân thiện với C++.
 *
 * Mỗi instance tương ứng đúng 1 entry, không cache, không lazy.
 */
struct JournalEntry
{
    /**
     * @brief Cursor duy nhất của journal entry.
     *
     * - Do systemd cấp phát.
     * - Dùng để:
     *   + Resume đọc journal sau khi restart process
     *   + Đảm bảo không đọc trùng log
     *
     * Lưu ý:
     * - Cursor là opaque string, không được parse hay chỉnh sửa.
     * - Có thể rỗng nếu không lấy được từ journal.
     */
    std::string cursor;

    /**
     * @brief Nội dung log chính (MESSAGE=).
     *
     * - Tương đương nội dung log mà syslog/syslog-ng hiển thị.
     * - Không phải entry nào cũng có MESSAGE.
     */
    std::string message;

    /**
     * @brief systemd unit phát sinh log (_SYSTEMD_UNIT=).
     *
     * Ví dụ:
     * - ssh.service
     * - NetworkManager.service
     *
     * Có thể rỗng với log không gắn với unit.
     */
    std::string unit;

    /**
     * @brief SYSLOG_IDENTIFIER của tiến trình.
     *
     * - Thường là tên binary hoặc module.
     * - Dùng để nhóm log theo source logic.
     */
    std::string identifier;

    /**
     * @brief Mức độ ưu tiên log (PRIORITY=).
     *
     * Giá trị dạng chuỗi số:
     *   0 = emerg
     *   1 = alert
     *   2 = crit
     *   3 = err
     *   4 = warning
     *   5 = notice
     *   6 = info
     *   7 = debug
     *
     * Ghi chú:
     * - Hiện tại lưu string để tránh parse.
     * - Production nên map sang enum/int.
     */
    std::string priority;

    /**
     * @brief Thời gian ghi log (microsecond từ UNIX epoch).
     *
     * - Dùng cho:
     *   + Sắp xếp
     *   + Lọc theo thời gian
     *   + Timestamp chính xác (không phụ thuộc timezone)
     */
    uint64_t    realtime_usec;
};

class JournalReader
{
public:
    /**
     * @brief Xác định nguồn journal được mở.
     *
     * OpenMode chỉ ảnh hưởng tới *nguồn dữ liệu*, không ảnh hưởng
     * tới logic đọc, filter hay seek.
     */
    enum class OpenMode
    {
        /**
         * @brief Mở journal hệ thống (system-wide).
         *
         * - Bao gồm log của tất cả service hệ thống.
         * - Cần quyền root hoặc capability phù hợp.
         */
        SYSTEM,

        /**
         * @brief Mở journal của user hiện tại.
         *
         * - Chỉ chứa log của user session.
         * - Không cần quyền root.
         */
        USER,

        /**
         * @brief Mở journal từ thư mục cụ thể.
         *
         * - Dùng cho:
         *   + Offline analysis
         *   + Embedded system không có journal daemon
         *   + Test / replay log
         *
         * - path phải trỏ tới thư mục chứa *.journal.
         */
        DIRECTORY
    };

    explicit JournalReader(OpenMode mode, const std::string& path = "");
    ~JournalReader();

    // Disable copy
    JournalReader(const JournalReader&) = delete;
    JournalReader& operator=(const JournalReader&) = delete;

    /**
     * @brief Thêm điều kiện lọc journal.
     *
     * @param match Chuỗi filter dạng "KEY=value".
     *
     * Ví dụ:
     * - "_SYSTEMD_UNIT=ssh.service"
     * - "PRIORITY=3"
     *
     * Lưu ý:
     * - Các filter là AND với nhau.
     * - Không reset vị trí đọc.
     */
    void AddMatch(const std::string& match);   // "KEY=value"
    void AddMatch(const JournalField& f, const std::string& value);

    /**
     * @brief Xóa toàn bộ filter đã thêm.
     *
     * Sau khi gọi:
     * - JournalReader trả về toàn bộ entry.
     */
    void ClearMatches();

    /**
     * @brief Seek về entry đầu tiên.
     */
    void SeekHead();

    /**
     * @brief Seek về cuối journal.
     *
     * Thường dùng để:
     * - Bỏ qua log cũ
     * - Chỉ đọc log mới sinh ra
     */
    void SeekTail();

    /**
     * @brief Seek theo timestamp (microsecond).
     *
     * @param usec UNIX epoch (µs).
     */
    void SeekRealtime(uint64_t usec);

    /**
     * @brief Seek tới vị trí được xác định bởi cursor.
     *
     * @param cursor Cursor đã lưu trước đó.
     *
     * @return true  nếu seek thành công
     * @return false nếu cursor không hợp lệ
     */
    bool SeekCursor(const std::string& cursor);

    /**
     * @brief Đọc entry tiếp theo trong journal.
     *
     * @param entry Struct nhận dữ liệu.
     *
     * @return true  nếu đọc thành công
     * @return false nếu hết log hoặc lỗi
     *
     * Đặc điểm:
     * - Không bao giờ trả lại entry cũ
     * - An toàn cho vòng lặp while
     */
    bool NextEntry(JournalEntry& entry);

    /**
     * @brief Chờ journal có entry mới.
     *
     * @param timeout_ms Thời gian chờ (ms).
     *
     * @return true  nếu có log mới
     * @return false nếu timeout
     */
    bool Wait(int timeout_ms);
    
private:
    sd_journal* m_journal = nullptr;
    std::string ReadEntryField(sd_journal* j, const char* field);
    const char* toFieldName(JournalField f);
};
