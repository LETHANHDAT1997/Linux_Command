Dưới đây là **tóm tắt chi tiết và chính xác nội dung chính** của bài viết *“Những lưu ý sống còn khi làm việc với callback function của Software Timer”* trên Viblo — tập trung vào những ý quan trọng người làm embedded/RTOS cần biết: ([Viblo][1])

---

## 🧠 1. **Software Timer trong FreeRTOS hoạt động như thế nào**

* FreeRTOS có một thành phần gọi là **Software Timer** dùng để gọi hàm khi timer hết hạn.
* Các callback này được thực thi bởi một task đặc biệt gọi là **Timer Service Task** (hay *Timer Task*) chứ không chạy ở ngữ cảnh task người dùng hay ISR.
* *Timer Task* chỉ có **một instance duy nhất** và xử lý các callback theo thứ tự FIFO. ([Viblo][1])

---

## ⛔ 2. **Không được block trong callback function**

Callback timer **tuyệt đối không được block** — tức không được chờ, delay, hay đợi tài nguyên trong callback vì hậu quả rất nghiêm trọng: ([Viblo][1])

### Ví dụ hành vi “block” trong callback

* Gọi các API có thể block như `vTaskDelay()`, `xQueueReceive()` với timeout không 0, `xSemaphoreTake()`…
* Chờ mutex hoặc tài nguyên khác.
* Xử lý vòng lặp nặng, thực hiện các thao tác tốn thời gian lâu. ([Viblo][1])

---

## ⚠️ 3. **Hậu quả nếu callback bị block**

Nếu callback bị block, *Timer Task* sẽ dừng lại và:

### ⏱️ Mất tính thời gian thực (Real-Time)

* Các timer khác sẽ **bị trễ hoặc bị bỏ lỡ** thời hạn.
* Tác vụ định kỳ, watchdog, check sensor… đều mất độ chính xác thời gian. ([Viblo][1])

### 🔒 Có thể gây deadlock hệ thống

* Nếu callback block đợi semaphore/mutex mà không ai release, *Timer Task* sẽ bị treo và các timer khác **không bao giờ chạy nữa**. ([Viblo][1])

### 💻 Ảnh hưởng đến CPU & tải hệ thống

* Callback chiếm CPU quá lâu làm lệch tải, jitter tăng, các callback khác chạy muộn. ([Viblo][1])

---

## ✅ 4. **Thiết kế đúng cho callback**

Thay vì xử lý nặng ngay trong timer callback, bài viết khuyến nghị các nguyên tắc thiết kế sau: ([Viblo][1])

### ✔️ Callback chỉ như *trigger*

* Trong callback chỉ làm những việc rất **nhẹ và không block**, ví dụ:

  ```c
  void vSafeCallback(TimerHandle_t xTimer) {
    xTaskNotifyGive(xMainTaskHandle);  // gửi notify cho task xử lý
  }
  ```
* Logic phức tạp để riêng trong một **task riêng**. ([Viblo][1])

### ✔️ Task xử lý logic

* Tạo một task có **độ ưu tiên phù hợp** để xử lý công việc thực sự.
* Timer callback chỉ kích hoạt task bằng notify/queue/event group. ([Viblo][1])

### ✔️ Gửi dữ liệu không block

* Dùng các API non-blocking như `xQueueSendToBackFromISR()` nếu cần truyền dữ liệu ra task. ([Viblo][1])

### ✔️ Nếu cần delay/chờ

* Thiết kế task chuyên biệt để thực hiện delay/chờ — **không làm điều này trong callback**. ([Viblo][1])

---

## 🏁 5. **Tóm lại – nguyên tắc “sống còn”**

➡️ Callback phải **ngắn, không block, không xử lý logic hoặc delay nặng**.
➡️ Hãy dùng callback như một **trigger để chuyển việc thực thi sang task chính**.
➡️ Tuân thủ nguyên tắc này để đảm bảo hệ thống **freeRTOS chạy ổn định, đúng thời gian và đáng tin cậy**. ([Viblo][1])

---
