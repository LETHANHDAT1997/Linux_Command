Dưới đây là tổng hợp chi tiết nội dung từ bài viết **"Khái niệm về multitasking trên vi điều khiển"** trên Viblo, giúp bạn nắm bắt cốt lõi của việc chuyển đổi từ tư duy lập trình tuần tự sang đa tác vụ (multitasking) trong hệ thống nhúng.

### 1. Bối cảnh và hai phương pháp tiếp cận chính

Vi điều khiển (Microcontroller) được ví như trái tim của các thiết bị nhúng (đồng hồ thông minh, robot, thiết bị đo lường...). Khi phát triển phần mềm cho chúng, lập trình viên thường đứng trước hai lựa chọn phương pháp:

* **Lập trình tuần tự (Sequential Programming)**
* **Lập trình đa tác vụ (Multitasking)** - thường đi kèm với Hệ điều hành thời gian thực (RTOS).

---

### 2. Chi tiết về Lập trình tuần tự (Sequential Programming)

Đây là phương pháp truyền thống và cơ bản nhất.

* **Cơ chế hoạt động:** Các dòng lệnh được thực thi lần lượt từ trên xuống dưới trong một vòng lặp vô hạn (thường là `while(1)`). Tại một thời điểm, vi điều khiển chỉ làm đúng một việc duy nhất.
* **Ưu điểm:**
* Dễ viết, dễ hiểu, phù hợp cho người mới bắt đầu.
* Cấu trúc chương trình đơn giản, không cần cấu hình phức tạp.


* **Nhược điểm:**
* **Phản hồi chậm:** Nếu có một tác vụ tốn nhiều thời gian (ví dụ: tính toán phức tạp, chờ độ trễ `delay`, hoặc cập nhật màn hình), toàn bộ hệ thống sẽ bị "đóng băng" tại đó. Các tác vụ khác phải xếp hàng chờ đợi.
* **Khó mở rộng:** Khi chương trình lớn lên, việc quản lý luồng chạy trở nên rối rắm và khó bảo trì.



---

### 3. Chi tiết về Multitasking (Đa tác vụ)

Multitasking giải quyết nhược điểm của lập trình tuần tự bằng cách chia nhỏ chương trình thành các tác vụ độc lập.

* **Bản chất thực tế:** Đa số vi điều khiển chỉ có **một lõi CPU**, nên về mặt vật lý chúng **không thể** chạy song song 2 việc cùng lúc.
* **Cơ chế "Song song ảo":**
* Hệ thống sử dụng một **Hệ điều hành thời gian thực (RTOS)** (ví dụ: FreeRTOS).
* RTOS có một thành phần gọi là **Scheduler (Bộ lập lịch)**. Bộ này sẽ chia nhỏ thời gian CPU và cho phép chuyển đổi qua lại giữa các tác vụ (Task) cực kỳ nhanh chóng.
* Sự chuyển đổi này nhanh đến mức mắt thường hoặc cảm nhận của người dùng thấy như các tác vụ đang chạy đồng thời.


* **Ví dụ minh họa trong bài viết:**
Giả sử hệ thống cần:
1. Đọc cảm biến (mỗi 100ms).
2. Cập nhật màn hình (mỗi 500ms).
3. Gửi dữ liệu UART (mỗi 1000ms).
*-> RTOS sẽ tự động phân chia khe thời gian để đảm bảo cả 3 việc này đều được thực hiện đúng chu kỳ mà không việc nào chặn việc nào.*


* **Ưu điểm:**
* **Phản hồi nhanh (Real-time):** Hệ thống không bị chặn bởi các tác vụ chậm.
* **Dễ bảo trì & Mở rộng:** Có thể thêm tính năng mới bằng cách thêm một Task mới mà ít ảnh hưởng đến các Task cũ.
* Tận dụng tốt tài nguyên của các vi điều khiển hiện đại.



---

### 4. Khi nào nên dùng phương pháp nào?

| Tiêu chí | Lập trình tuần tự | Multitasking (RTOS) |
| --- | --- | --- |
| **Độ phức tạp ứng dụng** | Đơn giản (nháy LED, đọc 1 cảm biến) | Phức tạp (Robot, IoT, báo cháy, màn hình tương tác) |
| **Yêu cầu xử lý** | Đơn luồng, không khắt khe về thời gian | Đa luồng, cần chạy song song nhiều chức năng |
| **Đối tượng** | Người mới học, dự án nhỏ | Dự án chuyên nghiệp, yêu cầu real-time |

### 5. Kết luận

Bài viết nhấn mạnh rằng dù lập trình tuần tự phù hợp cho bước đầu làm quen, nhưng **Multitasking với RTOS** đang dần trở thành tiêu chuẩn bắt buộc cho các hệ thống nhúng hiện đại nhờ khả năng xử lý mạnh mẽ, giá thành vi điều khiển ngày càng rẻ và yêu cầu ngày càng cao về tốc độ phản hồi của người dùng.