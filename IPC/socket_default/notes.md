# So Sánh Các Kiểu Socket (SOCK_STREAM vs SOCK_DGRAM vs SOCK_SEQPACKET)

Tài liệu này ghi chú lại sự khác biệt cốt lõi về **Độ tin cậy (Reliability)** và **Ranh giới dữ liệu (Message Boundary)** giữa các kiểu socket, đồng thời giải thích các hiểu lầm thường gặp về UDP trong ứng dụng Streaming.

---

## 1. Bảng So Sánh Tổng Quan

| Tiêu chí | SOCK_STREAM (TCP-like) | SOCK_DGRAM (UDP-like) | SOCK_SEQPACKET (Hybrid) |
| :--- | :--- | :--- | :--- |
| **Hướng kết nối** | Connection-oriented (Cần kết nối) | Connectionless (Không cần kết nối) | Connection-oriented (Cần kết nối) |
| **Độ tin cậy** | **Rất cao** (Không mất gói, đúng thứ tự) | **Thấp** (Có thể mất gói, sai thứ tự) | **Rất cao** (Không mất gói, đúng thứ tự) |
| **Ranh giới dữ liệu** | **Không có** (Byte Stream - Luồng byte) | **Có** (Datagram - Gói tin độc lập) | **Có** (Giữ nguyên ranh giới gói tin) |
| **Ứng dụng chính** | Truyền file, dữ liệu lớn, giao thức text/JSON | Gửi lệnh ngắn, thông báo sự kiện, video/audio | Giao tiếp IPC dạng Request-Response |

---

## 2. Ranh giới dữ liệu (Message Boundary) là gì?

Đây là cách hệ điều hành bàn giao dữ liệu từ bộ đệm (buffer) của hệ thống lên ứng dụng của bạn.

### Ví dụ cụ thể:
Giả sử tiến trình gửi thực hiện 2 lệnh gửi liên tiếp:
* **Lần 1:** Gửi gói dữ liệu `A` chứa 100 bytes.
* **Lần 2:** Gửi gói dữ liệu `B` chứa 50 bytes.

#### Với `SOCK_STREAM` (Luồng byte - TCP):
* Hệ điều hành coi toàn bộ dữ liệu là một dòng chảy liên tục `150 bytes` (`A` nối tiếp `B`).
* Phía nhận gọi `read(..., 1000)` có thể nhận được cả `150 bytes` trong **một lần đọc duy nhất**. Ranh giới giữa gói `A` và gói `B` bị xóa bỏ.
* Nếu muốn phân tách, ứng dụng phải tự thiết kế giao thức (ví dụ: thêm header chỉ định chiều dài của từng gói).

#### Với `SOCK_DGRAM` (Gói tin - UDP):
* Hệ điều hành tôn trọng tuyệt đối ranh giới của từng gói tin gửi đi.
* Phía nhận **phải gọi nhận 2 lần** để lấy ra gói `A` (100 bytes) và gói `B` (50 bytes) riêng biệt.
* **Đặc biệt:** Nếu phía nhận chuẩn bị buffer quá nhỏ (ví dụ 80 bytes) để nhận gói `A` (100 bytes), hệ thống sẽ trả về 80 bytes đầu tiên và **hủy bỏ hoàn toàn 20 bytes còn lại**. Ở lần gọi nhận tiếp theo, bạn sẽ nhận được gói `B`, phần dữ liệu thừa của gói `A` đã mất vĩnh viễn.

---

## 3. Giải mã hiểu lầm: "Tại sao UDP không an toàn nhưng lại phù hợp cho Streaming?"

Nhiều người lầm tưởng UDP phù hợp cho streaming (truyền phát video, audio, cuộc gọi thoại) vì nó *"không quan tâm đến gói dữ liệu"*. Thực tế hoàn toàn ngược lại:

* **UDP cực kỳ quan tâm đến ranh giới gói tin:** Mỗi gói âm thanh/hình ảnh được đóng gói rất chặt chẽ trong các giao thức thời gian thực (như RTP chạy trên nền UDP).
* **Lý do thực sự khiến UDP phù hợp với Streaming:**
  1. **Chấp nhận mất dữ liệu hơn là bị trễ (No Retransmission):** Trong video call, nếu một khung hình bị mất, ta chấp nhận bỏ qua (màn hình bị giật/nhiễu nhẹ) để xem tiếp khung hình mới nhất. Nếu dùng TCP, khi mất gói, toàn bộ hệ thống sẽ dừng lại để chờ truyền lại (gọi là *Head-of-Line blocking*), gây ra hiện tượng đứng hình và tích lũy độ trễ.
  2. **Tốc độ tối đa, không kiểm soát luồng (No Congestion/Flow Control):** UDP đẩy dữ liệu đi ngay lập tức mà không cần bắt tay kết nối phức tạp hay đợi gói tin phản hồi (ACK) từ phía nhận.

---

## 4. Giải pháp lai tối ưu cho IPC trên cùng máy: `SOCK_SEQPACKET`

Khi lập trình giao tiếp giữa các tiến trình (IPC) trên cùng một máy (nơi mà đường truyền bộ nhớ là tuyệt đối an toàn, không lo mất gói vật lý), chúng ta có lựa chọn thứ ba: **`SOCK_SEQPACKET`**.

* **Kế thừa từ `SOCK_STREAM`:** Giao tiếp có kết nối, đảm bảo dữ liệu truyền đi đúng thứ tự và không bao giờ bị mất mát.
* **Kế thừa từ `SOCK_DGRAM`:** Giữ nguyên ranh giới gói dữ liệu khi truyền nhận.
* **Lợi ích:** Bạn không cần phải viết code phân tách luồng byte phức tạp (như khi dùng `SOCK_STREAM`), cũng không lo lắng việc tràn/mất gói dữ liệu (như khi dùng `SOCK_DGRAM`).
