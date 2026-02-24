Dưới đây là **tóm tắt chi tiết và có cấu trúc** toàn bộ nội dung bài viết trên Viblo (đăng ngày 08/07/2025 bởi tác giả delinux). Tôi giữ nguyên cấu trúc gốc, khái niệm chính, code ví dụ, bảng so sánh, lưu ý kỹ thuật và các điểm quan trọng.

### 1. Giới thiệu chung
Bài viết giải thích cơ chế hoạt động của **ngắt (Interrupt)** trong FreeRTOS, tập trung vào cách ngắt tương tác với **scheduler** để đảm bảo hệ thống thời gian thực. Nội dung bao gồm:  
- Cách ISR (Interrupt Service Routine) hoạt động mà không trực tiếp chuyển ngữ cảnh.  
- Vai trò của các API FromISR() trong việc gợi ý scheduler chuyển task.  
- Minh họa qua ví dụ, lưu ý thực tế và so sánh.

### 2. Ngắt trong FreeRTOS hoạt động như thế nào?
- Khi ngắt xảy ra, bộ vi điều khiển lưu trạng thái hiện tại và nhảy đến ISR.  
- ISR xử lý nhanh chóng, sau đó chương trình quay về điểm bị gián đoạn.  
- Trong FreeRTOS, ISR không gọi API thông thường (như xSemaphoreGive()), mà phải dùng phiên bản **FromISR()** để an toàn và gợi ý scheduler kiểm tra task ưu tiên cao hơn.

### 3. Scheduler hoạt động ra sao khi có ngắt?
- FreeRTOS sử dụng **preemptive scheduler** (lập lịch tiền nhiệm): task ưu tiên cao hơn có thể giành CPU ngay.  
- ISR gọi API FromISR() → kiểm tra nếu có task ưu tiên cao hơn được "giải phóng" (qua semaphore/queue/notify).  
- Nếu có, gọi `portYIELD_FROM_ISR()` để yêu cầu chuyển ngữ cảnh **ngay sau ISR kết thúc**.

### 4. Giải thích kỹ hơn qua ví dụ
Ví dụ: Hai task – Task A (ưu tiên thấp, đọc sensor mỗi 100ms) và Task B (ưu tiên cao, xử lý dữ liệu UART).  
- Task A đang chạy.  
- Ngắt UART xảy ra → ISR nhận dữ liệu và gọi `xSemaphoreGiveFromISR()`.  
- Nếu Task B được giải phóng, đặt `xHigherPriorityTaskWoken = pdTRUE`.  
- Gọi `portYIELD_FROM_ISR(xHigherPriorityTaskWoken)` → sau ISR, scheduler chuyển sang Task B ngay lập tức.

### 5. Các hàm API ISR cần biết
Các API dành riêng cho ISR (không dùng trong task thông thường):  
```c
xQueueSendFromISR();      // Gửi dữ liệu vào queue từ ISR
xSemaphoreGiveFromISR();  // Nhả semaphore từ ISR
xTaskNotifyFromISR();     // Gửi notify đến task từ ISR
portYIELD_FROM_ISR(x);    // Kích hoạt yield nếu cần chuyển task
```

### 6. Lưu ý khi viết ISR trong FreeRTOS
- Giữ ISR **ngắn gọn**, tránh xử lý phức tạp (không dùng malloc(), không truy cập biến toàn cục mà không bảo vệ).  
- Sử dụng **volatile** cho biến chia sẻ.  
- Bảo vệ tài nguyên bằng **critical section** hoặc semaphore để tránh race condition.  
- Không gọi API FreeRTOS thông thường trong ISR.

### 7. So sánh chuyển ngữ cảnh với và không có ngắt
| Trạng thái hệ thống | Khi không có ngắt | Khi có ngắt |
|---------------------|-------------------|-------------|
| Quản lý tác vụ      | Theo timer hoặc blocking | ISR đánh thức task ngay |
| Độ trễ kích hoạt    | Có thể cao       | Rất thấp (thời gian thực) |
| Kiểm soát CPU       | Thuần task       | Tương tác ISR-task |

### 8. Cơ chế Nested Interrupt (Ngắt lồng nhau)
- Hỗ trợ qua cấu hình `configMAX_SYSCALL_INTERRUPT_PRIORITY`.  
- Ngắt ưu tiên ≤ giá trị này: Có thể gọi API FromISR().  
- Ngắt ưu tiên cao hơn: Không được gọi API FreeRTOS (tránh xung đột).

### 9. Sơ đồ và hình ảnh (mô tả)
- Bài viết đề cập đến "Sơ Đồ Luồng Xử Lý Ngắt Trong FreeRTOS" và "Tái Hiện Sơ Đồ Dòng Thời Gian (Timing)", nhưng không hiển thị chi tiết (có thể là tham chiếu đến biểu đồ minh họa luồng từ ngắt đến scheduler, bao gồm ISR, API FromISR và yield).

### 10. Kết luận (Tổng kết)
- Ngắt trong FreeRTOS tối ưu thời gian phản hồi bằng cách phối hợp ISR + Scheduler + API FromISR().  
- Tuân thủ quy ước: Sử dụng FromISR(), giữ ISR ngắn, tránh race condition để hệ thống hoạt động an toàn và hiệu quả.  
- Nhấn mạnh: Sự tương tác này là chìa khóa cho lập trình thời gian thực.

Bài viết ngắn gọn, tập trung thực tế, phù hợp cho lập trình viên nhúng làm việc với FreeRTOS. Nếu bạn đang gặp vấn đề về ngắt hoặc scheduler trong dự án (ví dụ: debug race condition hoặc tối ưu ISR), hãy chia sẻ thêm để mình hỗ trợ chi tiết hơn!