Dưới đây là **tổng hợp chi tiết, rõ ràng và có ví dụ cụ thể** nội dung chính của bài *“Ứng dụng của Task Idle Hook trong thực tế — Theo dõi CPU rảnh với biến `count`”* từ Viblo, diễn giải theo kiến thức kỹ thuật FreeRTOS cho bạn dễ hiểu và áp dụng. ([Viblo][1])

---

## 📌1. Mục đích của **Idle Hook** trong FreeRTOS

Trong **FreeRTOS**, *Idle Task* là task hệ thống có **ưu tiên thấp nhất**, luôn chạy khi không còn task khác Ready để chạy. ([Viblo][1])

**Idle Hook** là một hàm callback do người dùng định nghĩa (thường là `vApplicationIdleHook()`), được kernel gọi mỗi lần Idle Task chạy khi CPU rảnh. ([Viblo][1])

Để dùng Idle Hook bạn cần:

```c
#define configUSE_IDLE_HOOK 1 // bật Idle Hook trong FreeRTOSConfig.h
```

và định nghĩa hàm:

```c
void vApplicationIdleHook(void) {
    // code chạy khi CPU rảnh
}
```

---

## 🧠2. Ứng dụng chính của Idle Hook

### ✅2.1. **Đo thời gian CPU nhàn rỗi (CPU idle time)**

Ứng dụng nổi bật nhất của Idle Hook là đo **CPU bận rỗi bao nhiêu phần trăm** trong một khoảng thời gian nhất định bằng cách **đếm số lần Idle Hook được gọi** trong một giây. ([Viblo][1])

📌 Ý tưởng:

* Mỗi khi CPU rảnh, scheduler chạy Idle Task ⇒ Idle Hook được gọi ⇒ biến đếm (`idleCounter`) tăng.
* Số lần gọi trong 1 giây phản ánh mức độ nhàn rỗi của CPU.

Ví dụ:

```c
volatile uint32_t idleCounter = 0;

void vApplicationIdleHook(void) {
    idleCounter++;
}
```

Nếu sau 1 giây `idleCounter` rất lớn ⇒ CPU rảnh nhiều ⇒ tải hệ thống thấp.
Nếu `idleCounter` nhỏ ⇒ CPU bận nhiều ⇒ tải lớn. ([Viblo][1])

### 📊 2.2. **Đánh giá mức độ sử dụng CPU**

Bạn có thể dùng Idle Hook để tính **CPU Usage**:

```c
// trong một task monitor
uint32_t lastIdle = 0;
for (;;) {
    uint32_t now = idleCounter;
    uint32_t delta = now - lastIdle;
    lastIdle = now;

    float idlePercent = (delta / (float)EXPECTED_IDLE_PER_SEC) * 100;
    float cpuUsage = 100 - idlePercent;
    printf("CPU Usage: %.2f%%\n", cpuUsage);

    vTaskDelay(pdMS_TO_TICKS(1000)); // mỗi 1 giây
}
```

Trong đó `EXPECTED_IDLE_PER_SEC` là số đếm Idle Hook trung bình khi CPU hoàn toàn rảnh.

---

### 🔋 2.3. **Tiết kiệm điện (Power-Saving)**

Idle Hook là nơi lý tưởng để cho vi điều khiển **vào chế độ sleep**, giảm tiêu thụ năng lượng khi không có công việc phải làm:

```c
void vApplicationIdleHook(void) {
    __WFI(); // CPU sleep đến khi có ngắt mới đánh thức
}
```

Điều này rất hữu ích với các thiết bị **IoT, cảm biến pin, wearable**. ([Viblo][1])

---

### 🔍2.4. **Giám sát hệ thống và công việc nền nhẹ**

Idle Hook cũng có thể:

* **Kiểm tra trạng thái hệ thống** (ví dụ queue có đầy không)
* **Ghi log nhẹ**
* **Xóa dữ liệu tạm không cần thiết**
* Thực hiện các công việc nền **rất nhẹ** khi CPU rảnh mà không ảnh hưởng đến task khác. ([Viblo][1])

---

## 📈3. Cách đo CPU rảnh bằng biến `count`

Biến `count` (thường `volatile uint32_t`) dùng để đếm số lần Idle Hook được gọi trong một khoảng thời gian:

* Khi **CPU hoàn toàn rảnh**, Idle Hook được gọi liên tục rất nhiều lần (tùy tần số CPU).
* Khi **CPU bận**, số lần gọi ít hơn. ([Viblo][1])

Ví dụ minh họa:

```c
volatile uint32_t idleCount = 0;

void vApplicationIdleHook(void) {
    idleCount++; // mỗi lần CPU rảnh
}
```

Kết quả sau mỗi 1 giây sẽ phản ánh CPU đang rảnh hay bận, và nếu CPU rảnh **liên tục 10 giây**, giá trị đếm sẽ tăng liên tục trong suốt khoảng đó (không chỉ tăng 1 lần). ([Viblo][1])

---

## ⚠️4. Lưu ý khi dùng Idle Hook

🔹 **Không nên làm việc nặng** trong Idle Hook vì nó sẽ làm chậm các task khác. ([Viblo][1])
🔹 **Không block CPU** bằng các API như `vTaskDelay` hoặc `xQueueReceive` với timeout trong Idle Hook – Idle Task cần liên tục chạy để giải phóng tài nguyên hoặc cho CPU rảnh. ([Viblo][1])
🔹 Các xử lý trong Idle Hook **phải rất nhẹ** và nhanh. ([Viblo][1])

---

## 📌5. Ví dụ tổng hợp

Đo CPU rảnh và in ra console mỗi giây:

```c
volatile uint32_t idleCounter = 0;

void vApplicationIdleHook(void) {
    idleCounter++;
}

void vMonitorCPU(void *pvParameters) {
    uint32_t last = 0;
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // 1 giây
        uint32_t now = idleCounter;
        printf("IdleCount in last 1s: %lu\n", now - last);
        last = now;
    }
}

int main(void) {
    xTaskCreate(vMonitorCPU, "Monitor", 256, NULL, 1, NULL);
    vTaskStartScheduler();
    while (1);
}
```

Khi CPU rảnh nhiều ⇒ `IdleCount` lớn, và ngược lại. ([Viblo][1])

---

## 🧾6. Kết luận

Idle Hook trong FreeRTOS là một công cụ rất hữu ích để:

✔ **Theo dõi mức độ rảnh/bận của CPU**
✔ **Đánh giá CPU Usage** bằng biến đếm `count`
✔ **Tiết kiệm điện năng** khi CPU không có việc
✔ **Thực hiện công việc nền nhẹ** khi CPU rảnh

Việc đặt biến `count` trong Idle Hook và đo trong khoảng thời gian nhất định giúp bạn hiểu rõ hiệu suất hệ thống nhúng một cách trực quan và chính xác. ([Viblo][1])

---
