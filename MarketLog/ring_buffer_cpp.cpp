// ring_buffer.hpp
#ifndef RING_BUFFER_HPP
#define RING_BUFFER_HPP

#include <array>
#include <cstddef>
#include <optional>

/**
 * @brief Template-based ring buffer sử dụng std::array.
 * 
 * @tparam T Kiểu dữ liệu của phần tử
 * @tparam N Kích thước buffer (số phần tử)
 */
template<typename T, std::size_t N>
class RingBuffer {
public:
    /**
     * @brief Constructor mặc định - khởi tạo buffer rỗng.
     */
    RingBuffer() : head_(0), tail_(0), full_(false) {}

    /**
     * @brief Reset buffer về trạng thái rỗng.
     */
    void reset() {
        head_ = 0;
        tail_ = 0;
        full_ = false;
    }

    /**
     * @brief Kiểm tra buffer đầy.
     * @return true nếu buffer đầy.
     */
    bool is_full() const {
        return full_;
    }

    /**
     * @brief Kiểm tra buffer rỗng.
     * @return true nếu buffer rỗng.
     */
    bool is_empty() const {
        return !full_ && (head_ == tail_);
    }

    /**
     * @brief Lấy số phần tử đang có trong buffer.
     * @return Số phần tử hiện có.
     */
    std::size_t size() const {
        if (full_) {
            return N;
        }
        if (head_ >= tail_) {
            return head_ - tail_;
        }
        return N + head_ - tail_;
    }

    /**
     * @brief Lấy số phần tử còn trống.
     * @return Số phần tử có thể ghi thêm.
     */
    std::size_t available() const {
        return N - size();
    }

    /**
     * @brief Lấy capacity của buffer.
     * @return Kích thước tối đa của buffer.
     */
    constexpr std::size_t capacity() const {
        return N;
    }

    /**
     * @brief Ghi một phần tử vào buffer.
     * @param data Phần tử cần ghi.
     * @return true nếu ghi thành công, false nếu buffer đầy.
     */
    bool put(const T& data) {
        if (full_) {
            return false;
        }
        
        buffer_[head_] = data;
        head_ = (head_ + 1) % N;
        full_ = (head_ == tail_);
        
        return true;
    }

    /**
     * @brief Ghi một phần tử vào buffer (move semantics).
     * @param data Phần tử cần ghi.
     * @return true nếu ghi thành công, false nếu buffer đầy.
     */
    bool put(T&& data) {
        if (full_) {
            return false;
        }
        
        buffer_[head_] = std::move(data);
        head_ = (head_ + 1) % N;
        full_ = (head_ == tail_);
        
        return true;
    }

    /**
     * @brief Đọc một phần tử từ buffer.
     * @return std::optional chứa phần tử nếu đọc được, std::nullopt nếu rỗng.
     */
    std::optional<T> get() {
        if (is_empty()) {
            return std::nullopt;
        }
        
        T data = std::move(buffer_[tail_]);
        tail_ = (tail_ + 1) % N;
        full_ = false;
        
        return data;
    }

    /**
     * @brief Peek phần tử tại offset từ tail mà không xóa.
     * @param offset Vị trí tính từ tail (0 = phần tử kế tiếp).
     * @return std::optional chứa phần tử nếu hợp lệ, std::nullopt nếu không.
     */
    std::optional<T> peek(std::size_t offset = 0) const {
        if (offset >= size()) {
            return std::nullopt;
        }
        
        std::size_t pos = (tail_ + offset) % N;
        return buffer_[pos];
    }

    /**
     * @brief Ghi nhiều phần tử vào buffer.
     * @param data Con trỏ đến mảng dữ liệu.
     * @param length Số phần tử cần ghi.
     * @return Số phần tử thực sự ghi được.
     */
    std::size_t write(const T* data, std::size_t length) {
        std::size_t written = 0;
        
        for (std::size_t i = 0; i < length; ++i) {
            if (!put(data[i])) {
                break;
            }
            ++written;
        }
        
        return written;
    }

    /**
     * @brief Đọc nhiều phần tử từ buffer.
     * @param data Con trỏ đến mảng để lưu dữ liệu.
     * @param length Số phần tử muốn đọc.
     * @return Số phần tử thực sự đọc được.
     */
    std::size_t read(T* data, std::size_t length) {
        std::size_t read_count = 0;
        
        for (std::size_t i = 0; i < length; ++i) {
            auto item = get();
            if (!item) {
                break;
            }
            data[i] = std::move(*item);
            ++read_count;
        }
        
        return read_count;
    }

private:
    std::array<T, N> buffer_;  ///< Bộ đệm dữ liệu
    std::size_t head_;         ///< Con trỏ ghi
    std::size_t tail_;         ///< Con trỏ đọc
    bool full_;                ///< Trạng thái đầy
};

// ============================================================================
// VÍ DỤ SỬ DỤNG
// ============================================================================

/**
 * @brief Ví dụ sử dụng RingBuffer với unsigned char.
 * 
 * @code
 * #include <iostream>
 * #include <cstring>
 * #include "ring_buffer.hpp"
 * 
 * int main() {
 *     // Tạo ring buffer 16 byte
 *     RingBuffer<unsigned char, 16> rb;
 *     
 *     // Chuỗi cần ghi
 *     const char* msg = "HelloWorld";
 *     
 *     // Ghi vào buffer
 *     std::size_t written = rb.write(
 *         reinterpret_cast<const unsigned char*>(msg), 
 *         std::strlen(msg)
 *     );
 *     std::cout << "Đã ghi " << written << " byte\n";
 *     
 *     // Đọc 5 byte đầu tiên
 *     unsigned char read_buf[16];
 *     std::size_t read_count = rb.read(read_buf, 5);
 *     read_buf[read_count] = '\0';
 *     std::cout << "Đọc 5 byte: " << read_buf << "\n";  // Hello
 *     
 *     // Peek byte kế tiếp (không xóa)
 *     auto peek_byte = rb.peek();
 *     if (peek_byte) {
 *         std::cout << "Peek ký tự tiếp theo: " 
 *                   << static_cast<char>(*peek_byte) << "\n";  // W
 *     }
 *     
 *     // Đọc toàn bộ còn lại
 *     read_count = rb.read(read_buf, sizeof(read_buf));
 *     read_buf[read_count] = '\0';
 *     std::cout << "Đọc toàn bộ còn lại: " << read_buf << "\n";  // World
 *     
 *     return 0;
 * }
 * @endcode
 */

/**
 * @brief Ví dụ sử dụng RingBuffer với kiểu dữ liệu tùy chỉnh.
 * 
 * @code
 * #include <iostream>
 * #include <string>
 * #include "ring_buffer.hpp"
 * 
 * struct Message {
 *     int id;
 *     std::string text;
 * };
 * 
 * int main() {
 *     RingBuffer<Message, 8> rb;
 *     
 *     // Ghi messages
 *     rb.put({1, "First message"});
 *     rb.put({2, "Second message"});
 *     rb.put({3, "Third message"});
 *     
 *     std::cout << "Buffer size: " << rb.size() << "\n";
 *     
 *     // Peek message đầu tiên
 *     auto msg = rb.peek();
 *     if (msg) {
 *         std::cout << "Peek: ID=" << msg->id 
 *                   << ", Text=" << msg->text << "\n";
 *     }
 *     
 *     // Đọc tất cả messages
 *     while (auto m = rb.get()) {
 *         std::cout << "Read: ID=" << m->id 
 *                   << ", Text=" << m->text << "\n";
 *     }
 *     
 *     return 0;
 * }
 * @endcode
 */

#endif // RING_BUFFER_HPP