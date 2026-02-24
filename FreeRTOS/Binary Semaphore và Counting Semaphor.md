✅ Dưới đây là **tóm tắt chi tiết và có cấu trúc** toàn bộ nội dung bài viết trên Viblo (đăng ngày 07/07/2025 bởi tác giả delinux). Tôi giữ nguyên cấu trúc gốc, bảng so sánh, code ví dụ, thuật ngữ kỹ thuật và các điểm quan trọng.

### 1. Giới thiệu
Bài viết so sánh **Binary Semaphore** và **Counting Semaphore** – hai công cụ đồng bộ phổ biến trong RTOS (như FreeRTOS).  
Mặc dù tên gọi tương đồng, hai loại này phục vụ mục đích khác nhau và có hành vi hoàn toàn khác biệt.

### 2. Khái niệm cơ bản

#### 2.1 Binary Semaphore (Semaphore nhị phân)
- Chỉ có **hai trạng thái**: `0` (locked) hoặc `1` (unlocked).  
- Ứng dụng chính:  
  - Đồng bộ giữa **task** và **ISR**  
  - Báo hiệu (signal) giữa các task  
  - Bảo vệ tài nguyên chỉ cho **một luồng** truy cập tại một thời điểm

#### 2.2 Counting Semaphore (Semaphore đếm)
- Giá trị có thể **> 1** (tùy khai báo).  
- Ứng dụng chính:  
  - Quản lý **số lượng tài nguyên hạn chế** (ví dụ: 3 cổng UART)  
  - Đồng bộ với nhiều nguồn tín hiệu  
  - Quản lý buffer vòng, hàng đợi

### 3. Bảng so sánh chi tiết

| Tiêu chí                  | Binary Semaphore                  | Counting Semaphore                          |
|---------------------------|-----------------------------------|---------------------------------------------|
| Giá trị tối đa            | 1                                 | Tùy ý (do người dùng khai báo)              |
| Mục đích chính            | Đồng bộ hóa                       | Quản lý số lượng tài nguyên                 |
| Bảo vệ tài nguyên         | Có thể, nhưng không tối ưu như mutex | Có thể, nhưng không phải mục tiêu chính     |
| Có thể “give” nhiều lần   | Không (luôn giữ giá trị 1)        | Có (giá trị tăng dần)                       |
| Giao tiếp Task ↔ ISR      | Rất phù hợp                       | Ít phổ biến                                 |
| Giải phóng bởi nhiều task | Không khuyến nghị                 | Có thể                                      |
| Hành vi như tín hiệu      | Tốt                               | Tốt, nếu cần đếm số lượng tín hiệu          |
| Cấu trúc bên trong        | Queue có 1 phần tử                | Queue có nhiều phần tử                      |

### 4. Ví dụ minh họa

#### 4.1 Binary Semaphore – Đồng bộ ISR & Task
```c
SemaphoreHandle_t xBinarySemaphore;

void ISR_Handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xBinarySemaphore, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void TaskWaitForISR(void *pvParameters)
{
    for (;;) {
        if (xSemaphoreTake(xBinarySemaphore, portMAX_DELAY)) {
            printf("Interrupt received!\n");
        }
    }
}
```
→ ISR “give” một lần → task đang đợi sẽ được đánh thức.

#### 4.2 Counting Semaphore – Quản lý 3 tài nguyên
```c
SemaphoreHandle_t xCountingSemaphore;

void ResourceTask(void *pvParameters)
{
    while (1) {
        if (xSemaphoreTake(xCountingSemaphore, portMAX_DELAY)) {
            printf("Task %d dùng tài nguyên\n", (int)pvParameters);
            vTaskDelay(pdMS_TO_TICKS(1000));
            xSemaphoreGive(xCountingSemaphore);
        }
    }
}

// Khởi tạo
xCountingSemaphore = xSemaphoreCreateCounting(3, 3);  // max = 3, init = 3
```
→ 4 task cạnh tranh → chỉ 3 task chạy cùng lúc, task thứ 4 phải chờ.

### 5. Ưu & Nhược điểm

**Binary Semaphore**  
**Ưu**: Đơn giản, hiệu quả với ISR-task, không cần đếm.  
**Nhược**: Không lưu được nhiều tín hiệu, dễ mất signal nếu ISR bắn liên tục.

**Counting Semaphore**  
**Ưu**: Quản lý nhiều tài nguyên/tín hiệu tốt, có khả năng tích lũy.  
**Nhược**: Phức tạp hơn, dễ lỗi nếu take/give không cân bằng.

### 6. Khi nào dùng loại nào?

| Trường hợp                              | Semaphore phù hợp          |
|-----------------------------------------|----------------------------|
| ISR báo hiệu cho Task                   | Binary                     |
| Bảo vệ tài nguyên độc quyền             | Mutex (ưu tiên)            |
| Nhiều tài nguyên giống nhau (5 UART…)   | Counting                   |
| Nhiều tín hiệu đến liên tiếp            | Counting                   |
| “Bật đèn xanh” cho task bắt đầu         | Binary                     |

### 7. Kết luận
- **Binary Semaphore**: dùng khi cần đồng bộ **một sự kiện duy nhất** (ISR ↔ Task).  
- **Counting Semaphore**: dùng khi cần quản lý **nhiều tài nguyên** hoặc **nhiều tín hiệu tích lũy**.  

Việc chọn đúng loại giúp tránh **race condition**, **deadlock**, **dropped signals** trong hệ thống RTOS.

Bài viết ngắn gọn, dễ hiểu, rất phù hợp cho người mới làm quen FreeRTOS. Nếu bạn đang dùng semaphore trong dự án cụ thể (ví dụ cần thay thế bằng mutex, hoặc xử lý lost signal), hãy chia sẻ thêm để mình hỗ trợ chi tiết hơn!