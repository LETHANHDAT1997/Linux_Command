// ring_buffer.h
#ifndef RING_BUFFER_H
#define RING_BUFFER_H
#ifdef __cplusplus
 extern "C" {
#endif

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Cấu trúc ring buffer đơn giản dùng cho byte.
 *
 * Dùng kỹ thuật head/tail với cờ full để phân biệt giữa đầy và rỗng.
 */
typedef struct 
{
    unsigned char *buffer;   /**< Bộ đệm dữ liệu */
    size_t capacity;         /**< Kích thước bộ đệm */
    size_t head;             /**< Con trỏ ghi dữ liệu */
    size_t tail;             /**< Con trỏ đọc dữ liệu */
    bool full;               /**< Trạng thái buffer đầy */
} ring_buffer_t;

/**
 * @brief Khởi tạo ring buffer.
 *
 * @param rb       Con trỏ đến đối tượng ring_buffer_t.
 * @param buffer   Bộ nhớ dùng để chứa dữ liệu (đã cấp phát trước).
 * @param capacity Kích thước bộ nhớ buffer.
 */
void rb_init(ring_buffer_t *rb, unsigned char *buffer, size_t capacity);

/**
 * @brief Reset ring buffer về trạng thái rỗng.
 *
 * @param rb Con trỏ đến ring buffer.
 */
void rb_reset(ring_buffer_t *rb);

/**
 * @brief Kiểm tra buffer đầy.
 *
 * @param rb Con trỏ đến ring buffer.
 * @return true nếu buffer đầy, false nếu không.
 */
bool rb_is_full(const ring_buffer_t *rb);

/**
 * @brief Kiểm tra buffer rỗng.
 *
 * @param rb Con trỏ đến ring buffer.
 * @return true nếu rỗng, false nếu không.
 */
bool rb_is_empty(const ring_buffer_t *rb);

/**
 * @brief Lấy số byte đang có trong buffer.
 *
 * @param rb Con trỏ đến ring buffer.
 * @return Số phần tử hiện có.
 */
size_t rb_size(const ring_buffer_t *rb);

/**
 * @brief Lấy số byte còn trống trong buffer.
 *
 * @param rb Con trỏ đến ring buffer.
 * @return Số byte còn có thể ghi.
 */
size_t rb_available(const ring_buffer_t *rb);

/**
 * @brief Ghi 1 byte vào buffer.
 *
 * @param rb   Con trỏ đến ring buffer.
 * @param data Byte cần ghi.
 * @return true nếu ghi thành công, false nếu buffer đầy.
 */
bool rb_put(ring_buffer_t *rb, unsigned char data);

/**
 * @brief Đọc 1 byte từ buffer.
 *
 * @param rb   Con trỏ đến ring buffer.
 * @param data Con trỏ nơi lưu byte đọc được.
 * @return true nếu đọc thành công, false nếu buffer rỗng.
 */
bool rb_get(ring_buffer_t *rb, unsigned char *data);

/**
 * @brief Ghi nhiều byte vào buffer.
 *
 * @param rb     Con trỏ đến ring buffer.
 * @param data   Mảng dữ liệu cần ghi.
 * @param length Số byte muốn ghi.
 * @return Số byte thực sự ghi được.
 */
size_t rb_write(ring_buffer_t *rb, const unsigned char *data, size_t length);

/**
 * @brief Đọc nhiều byte từ buffer.
 *
 * @param rb     Con trỏ đến ring buffer.
 * @param data   Mảng để lưu dữ liệu đọc được.
 * @param length Số byte muốn đọc.
 * @return Số byte thực sự đọc được.
 */
size_t rb_read(ring_buffer_t *rb, unsigned char *data, size_t length);

/**
 * @brief Peek dữ liệu mà không tăng tail.
 *
 * @param rb     Con trỏ đến ring buffer.
 * @param data   Byte đọc được.
 * @param offset Vị trí tính từ tail.
 * @return true nếu đọc được, false nếu offset vượt phạm vi.
 */
bool rb_peek(const ring_buffer_t *rb, unsigned char *data, size_t offset);

#ifdef __cplusplus
    }
#endif

#endif // RING_BUFFER_H
