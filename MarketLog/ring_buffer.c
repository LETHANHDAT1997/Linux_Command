#include "ring_buffer.h"
#include <string.h>

/*
 * Khởi tạo ring buffer.
 * Buffer phải là bộ nhớ đã được cấp phát từ trước.
 */
void rb_init(ring_buffer_t *rb, unsigned char *buffer, size_t capacity) 
{
    rb->buffer = buffer;
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->full = false;
}

/*
 * Reset buffer về trạng thái ban đầu.
 */
void rb_reset(ring_buffer_t *rb) 
{
    rb->head = 0;
    rb->tail = 0;
    rb->full = false;
}

// Trả về true nếu buffer đầy
bool rb_is_full(const ring_buffer_t *rb) 
{
    return rb->full;
}

// Trả về true nếu buffer rỗng
bool rb_is_empty(const ring_buffer_t *rb) 
{
    return (!rb->full && (rb->head == rb->tail));
}

/*
 * Tính số phần tử đang có trong buffer.
 * Có 2 trường hợp:
 *  - head >= tail: dữ liệu liên tục
 *  - head < tail: bị vòng (wrap around)
 */
size_t rb_size(const ring_buffer_t *rb) 
{
    if (rb->full) 
    {
        return rb->capacity;
    }
    if (rb->head >= rb->tail) 
    {
        return rb->head - rb->tail;
    }
    return rb->capacity + rb->head - rb->tail;
}

/* Số byte còn trống trong buffer */
size_t rb_available(const ring_buffer_t *rb) 
{
    return rb->capacity - rb_size(rb);
}

/*
 * Ghi một byte vào buffer.
 * Trả về false nếu buffer đầy.
 */
bool rb_put(ring_buffer_t *rb, unsigned char data) 
{
    if (rb->full) 
    {
        return false; // Không ghi được
    }
    
    rb->buffer[rb->head] = data;

    // Tăng head và wrap-around nếu cần
    rb->head = (rb->head + 1) % rb->capacity;

    // Nếu head đuổi kịp tail → buffer đầy
    rb->full = (rb->head == rb->tail);
    
    return true;
}

/*
 * Đọc 1 byte từ buffer.
 * Trả về false nếu rỗng.
 */
bool rb_get(ring_buffer_t *rb, unsigned char *data) 
{
    if (rb_is_empty(rb)) 
    {
        return false;
    }
    
    *data = rb->buffer[rb->tail];

    // Tăng tail theo kiểu vòng
    rb->tail = (rb->tail + 1) % rb->capacity;

    // Đọc -> chắc chắn buffer không còn full
    rb->full = false;
    
    return true;
}

/*
 * Ghi nhiều byte vào buffer.
 * Dừng lại khi buffer đầy.
 */
size_t rb_write(ring_buffer_t *rb, const unsigned char *data, size_t length) 
{
    size_t written = 0;
    
    for (size_t i = 0; i < length; i++) 
    {
        if (!rb_put(rb, data[i])) 
        {
            break; // buffer đầy
        }
        written++;
    }
    
    return written;
}

/*
 * Đọc nhiều byte.
 * Dừng nếu không còn dữ liệu.
 */
size_t rb_read(ring_buffer_t *rb, unsigned char *data, size_t length) 
{
    size_t read = 0;
    
    for (size_t i = 0; i < length; i++) 
    {
        if (!rb_get(rb, &data[i])) 
        {
            break; // buffer rỗng
        }
        read++;
    }
    
    return read;
}

/*
 * Peek: đọc dữ liệu mà không thay đổi tail.
 * offset = 0 → byte kế tiếp
 */
bool rb_peek(const ring_buffer_t *rb, unsigned char *data, size_t offset) 
{
    if (offset >= rb_size(rb)) 
    {
        return false; // offset lớn hơn lượng dữ liệu
    }
    
    size_t pos = (rb->tail + offset) % rb->capacity;
    *data = rb->buffer[pos];
    
    return true;
}

/**
 * @brief Ví dụ sử dụng ring buffer cơ bản.
 *
 * Ví dụ này minh họa:
 *   - Khởi tạo ring buffer
 *   - Ghi một chuỗi dữ liệu
 *   - Đọc một phần dữ liệu
 *   - Peek dữ liệu còn lại
 *   - Đọc toàn bộ dữ liệu còn lại
 *
 * @code
 * #include <stdio.h>
 * #include "ring_buffer.h"
 *
 * int main(void)
 * {
 *     // Tạo buffer 16 byte
 *     unsigned char raw_buffer[16];
 *     ring_buffer_t rb;
 *
 *     // Khởi tạo ring buffer
 *     rb_init(&rb, raw_buffer, sizeof(raw_buffer));
 *
 *     // Chuỗi cần ghi
 *     const char *msg = "HelloWorld";
 *
 *     // Ghi vào buffer
 *     size_t written = rb_write(&rb, (const unsigned char*)msg, 10);
 *     printf("Đã ghi %zu byte\n", written);
 *
 *     // Đọc 5 byte đầu tiên
 *     unsigned char read_buf[16];
 *     size_t read = rb_read(&rb, read_buf, 5);
 *     read_buf[read] = '\0';
 *     printf("Đọc 5 byte: %s\n", read_buf);  // Hello
 *
 *     // Peek byte kế tiếp (không xóa)
 *     unsigned char peek_byte;
 *     rb_peek(&rb, &peek_byte, 0);
 *     printf("Peek ký tự tiếp theo: %c\n", peek_byte); // W
 *
 *     // Đọc toàn bộ còn lại
 *     read = rb_read(&rb, read_buf, sizeof(read_buf));
 *     read_buf[read] = '\0';
 *     printf("Đọc toàn bộ còn lại: %s\n", read_buf); // World
 *
 *     return 0;
 * }
 * @endcode
 */
