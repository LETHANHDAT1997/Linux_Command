Dưới đây là phần tổng hợp chi tiết và có hệ thống về nội dung bài viết "Quản lý ưu tiên và lập lịch tác vụ hiệu quả trong FreeRTOS" trên Viblo.

### 1. Tổng quan: Tại sao cần quản lý ưu tiên (Priority) và Lập lịch (Scheduling)?

Trong hệ thống nhúng sử dụng RTOS (như FreeRTOS), việc tạo ra nhiều tác vụ (Task) là bình thường. Tuy nhiên, nếu bạn gán mức độ ưu tiên cho các task này một cách ngẫu nhiên hoặc theo cảm tính, hệ thống sẽ gặp các vấn đề nghiêm trọng:

* Chương trình chạy không đúng luồng thiết kế.
* Một số task quan trọng bị lãng quên hoặc không bao giờ được chạy.
* Task không quan trọng lại chiếm hết tài nguyên CPU.

### 2. Cơ chế Lập lịch trong FreeRTOS

FreeRTOS sử dụng bộ lập lịch dựa trên mức độ ưu tiên (Priority-based Scheduler).

* **Nguyên tắc cơ bản:** Task có mức ưu tiên cao hơn sẽ luôn được CPU ưu tiên thực thi trước.
* **Mức ưu tiên:** Là một số nguyên không âm. Giá trị càng lớn thì mức ưu tiên càng cao.
* **Các trạng thái của Task:**
* `Running`: Đang thực thi.
* `Ready`: Sẵn sàng chạy (chờ CPU rảnh).
* `Blocked`: Đang chờ sự kiện (như chờ thời gian delay, chờ dữ liệu từ Queue, Semaphore...).
* `Suspended`: Bị tạm dừng hoàn toàn (bởi lệnh `vTaskSuspend()`).



**Cấu hình Scheduler:**

* **Preemptive Scheduling (`configUSE_PREEMPTION = 1`):** Hệ điều hành có quyền ngắt (preempt) một task đang chạy nếu có task khác có mức ưu tiên cao hơn chuyển sang trạng thái Ready. Đây là chế độ phổ biến nhất để đảm bảo tính thời gian thực.
* **Time Slicing:** Chia sẻ thời gian CPU cho các task có *cùng mức ưu tiên*.
* **Manual Yield:** Task tự nguyện nhường CPU (dùng `taskYIELD()`).

### 3. Chiến lược thiết kế mức ưu tiên hiệu quả

Việc chọn mức ưu tiên không nên tùy tiện mà cần dựa trên các tiêu chí:

1. **Tính thời gian thực:** Task cần phản hồi nhanh hay chậm?
2. **Thời lượng xử lý:** Task chạy nhanh hay tốn nhiều CPU?
3. **Tần suất:** Task chạy liên tục hay thỉnh thoảng mới chạy?

**Bảng gợi ý mức ưu tiên (từ cao xuống thấp):**

| Loại tác vụ (Task) | Mức ưu tiên gợi ý | Lý do |
| --- | --- | --- |
| **Xử lý ngắt (Defer processing task)** | **Rất cao** | Cần xử lý ngay lập tức để giải phóng ngắt phần cứng. |
| **Thu thập, xử lý dữ liệu khẩn cấp** | **Cao** | Ví dụ: Phát hiện vật cản, xử lý phanh xe. |
| **Giao tiếp cảm biến định kỳ** | **Trung bình** | Cần độ ổn định nhưng không quá gấp. |
| **Gửi/Nhận UART thông thường** | **Thấp** | Giao tiếp người dùng không cần phản hồi tức thì từng micro giây. |
| **Tác vụ nền (Background)** | **Rất thấp (Idle)** | Các việc không quan trọng, chạy khi hệ thống rảnh. |

### 4. Các lỗi thường gặp và giải pháp thực tế

#### 4.1. Tác vụ ưu tiên thấp chặn tác vụ ưu tiên cao (Sai lầm phổ biến)

* **Tình huống:** Bạn có Task 1 (xử lý dữ liệu khẩn cấp) và Task 2 (tác vụ nặng, tốn thời gian) có **cùng mức ưu tiên cao**.
* **Hậu quả:** Do cùng mức ưu tiên, nếu Task 2 chạy trước và chiếm dụng CPU trong thời gian dài (ví dụ: vòng lặp xử lý lớn), Task 1 sẽ phải chờ Task 2 chạy xong hết lượt (time slice) mới được chạy. Điều này làm mất tính thời gian thực của Task 1.
* **Giải pháp:**
* Hạ mức ưu tiên của Task 2 xuống thấp hơn Task 1.
* Hoặc chèn các khoảng nghỉ (`vTaskDelay`) hợp lý trong Task 2 để nhường CPU.



#### 4.2. Priority Inversion (Đảo ngược ưu tiên) * **Tình huống:**

```
1.  Task A (Ưu tiên thấp) đang giữ một tài nguyên (ví dụ: Mutex).
2.  Task B (Ưu tiên cao) muốn dùng Mutex đó nhưng phải chờ Task A nhả ra -> Task B bị Block.
3.  Task C (Ưu tiên trung bình) xuất hiện và chiếm CPU (vì ưu tiên cao hơn Task A).
4.  **Kết quả:** Task A không có CPU để chạy tiếp và nhả Mutex -> Task B (ưu tiên cao nhất) bị treo vĩnh viễn bởi Task C (ưu tiên trung bình).

```

* **Giải pháp:** Sử dụng **Mutex có tính năng Priority Inheritance** (Kế thừa ưu tiên). Khi Task B chờ, Task A sẽ tạm thời được nâng mức ưu tiên lên bằng Task B để chạy nhanh chóng và nhả Mutex.

#### 4.3. Starvation (Đói tài nguyên)

* **Tình huống:** Các task ưu tiên thấp không bao giờ được chạy vì các task ưu tiên cao luôn chiếm dụng CPU (luôn ở trạng thái Ready/Running mà không bao giờ Block).
* **Giải pháp:**
* Tránh thiết kế các task ưu tiên cao chạy liên tục (polling).
* Luôn sử dụng `vTaskDelay()` hoặc chờ sự kiện (Event/Semaphore) trong các task ưu tiên cao để chúng chuyển sang trạng thái Blocked, nhường CPU cho task thấp hơn.



### 5. Kết luận

Để hệ thống FreeRTOS hoạt động ổn định:

* Phải hiểu rõ tính chất của từng task để gán Priority đúng.
* Tránh lạm dụng mức ưu tiên cao cho các tác vụ xử lý nặng.
* Luôn kiểm soát việc tranh chấp tài nguyên (Mutex) để tránh lỗi Priority Inversion.
* Nên dùng các công cụ đo đạc (Trace) để kiểm tra hành vi thực tế thay vì chỉ đoán mò.