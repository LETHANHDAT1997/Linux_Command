# Message Queue: Khám Phá Bí Mật Giao Tiếp Giữa Các Task Trong FreeRTOS

## Giới thiệu nhanh

Trong phần trước, bài viết đã giới thiệu về **Task trong FreeRTOS** – các đơn vị thực thi độc lập, hoạt động theo lịch của Scheduler. Tuy nhiên, để các task phối hợp hiệu quả, cần có cơ chế giao tiếp. **Queue** chính là giải pháp cho việc truyền dữ liệu an toàn giữa các task hoặc giữa **ISR** và task, thay vì chia sẻ biến toàn cục dễ gây lỗi.

---

## 1. Queue trong FreeRTOS là gì?

**Queue** trong FreeRTOS là một cơ chế **IPC (Inter-Process Communication)** hoạt động theo nguyên tắc **FIFO (First In First Out)**, cho phép truyền dữ liệu an toàn giữa:

- Các **task** với nhau.
- **ISR (Interrupt Service Routine)** và **task**.

Ví dụ:  
- **Task A**: thu thập dữ liệu từ cảm biến.  
- **Task B**: xử lý dữ liệu.  
Thay vì dùng biến toàn cục (nguy hiểm), Task A đưa dữ liệu vào queue, Task B lấy ra để xử lý theo thứ tự đến trước được xử lý trước.

---

## 2. Cách hoạt động của Queue

### 2.1 Tạo queue

Trước khi sử dụng, cần tạo queue bằng hàm:

```c
QueueHandle_t xQueue;
xQueue = xQueueCreate(10, sizeof(int));
```

- `10`: số lượng phần tử tối đa mà queue có thể chứa.
- `sizeof(int)`: kích thước của mỗi phần tử dữ liệu (ví dụ: kiểu `int`).

### 2.2 Gửi dữ liệu vào queue

#### Từ task:

```c
xQueueSend(xQueue, &value, portMAX_DELAY);
```

#### Từ ISR:

```c
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
xQueueSendFromISR(xQueue, &value, &xHigherPriorityTaskWoken);
portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
```

### 2.3 Nhận dữ liệu từ queue

```c
int value;
xQueueReceive(xQueue, &value, portMAX_DELAY);
```

---

## 3. Queue hoạt động như thế nào bên trong?

- Khi một task gọi `xQueueSend`, dữ liệu được sao chép vào một **buffer vòng (circular buffer)** bên trong queue.
- Task gọi `xQueueReceive` sẽ nhận phần tử **đầu tiên** theo thứ tự FIFO.
- Nếu queue **rỗng**: task nhận sẽ bị **block** (treo) cho đến khi có dữ liệu (nếu dùng `portMAX_DELAY`) hoặc timeout.
- Nếu queue **đầy**: task gửi có thể bị **block** hoặc trả về lỗi tùy theo cấu hình.

---

## 4. Khi nào nên dùng Queue?

| Tình huống | Có nên dùng queue? | Ghi chú |
|-----------|---------------------|--------|
| Task nhận dữ liệu từ cảm biến và task khác xử lý | Rất phù hợp | Đảm bảo thứ tự xử lý |
| ISR gửi tín hiệu trạng thái đơn giản | Không nên dùng queue | Nên dùng **Semaphore** |
| Gửi tin nhắn dạng chuỗi | Không phù hợp | Dùng **Message Queue** hoặc **Stream Buffer** |
| Truyền dữ liệu khối lớn | Không nên dùng queue | Xem xét dùng buffer trực tiếp + signal |

---

## 5. Ưu điểm của Queue trong FreeRTOS

- **An toàn luồng (thread-safe)**: Được bảo vệ bởi cơ chế khóa nội bộ (mutex).
- **Linh hoạt**: Dùng được cho giao tiếp **task <-> task** hoặc **ISR <-> task**.
- **Tự động block**: Task không cần **polling** (liên tục kiểm tra), tiết kiệm CPU.
- **Nhẹ và hiệu quả**: Phù hợp với hệ thống nhúng có tài nguyên hạn chế.

---

## 6. Các loại queue trong FreeRTOS

| Tên | Mô tả | Ví dụ sử dụng |
|-----|-------|----------------|
| **Queue thường** | Hàng đợi FIFO với kích thước cố định | Lưu trữ `int`, `struct` nhỏ |
| **Queue set** | Cho phép chờ nhận dữ liệu từ nhiều queue cùng lúc | 2 cảm biến gửi dữ liệu về |
| **Stream Buffer** | Xử lý dòng byte liên tục | Giao tiếp UART, đọc ADC |
| **Message Buffer** | Lưu trữ tin nhắn có độ dài thay đổi | Tin nhắn theo định dạng TLV |

---

## 7. Một số tình huống thực tế & ví dụ

### 7.1 Giao tiếp ISR với task

```c
// ISR
void ADC_IRQHandler() {
    int val = read_adc();
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(xQueue, &val, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```

```c
// Task xử lý
void vAdcProcessingTask(void *pvParameters) {
    int val;
    while (1) {
        if (xQueueReceive(xQueue, &val, portMAX_DELAY)) {
            process(val);
        }
    }
}
```

### 7.2 Task gửi nhận dữ liệu lẫn nhau

```c
// Task 1 gửi dữ liệu
xQueueSend(xQueue, &data, 0);

// Task 2 nhận dữ liệu
if (xQueueReceive(xQueue, &rx_data, 0)) {
    // Xử lý rx_data
}
```

---

Tóm lại, **Queue trong FreeRTOS** là công cụ mạnh mẽ và cần thiết để đảm bảo giao tiếp an toàn, có thứ tự và hiệu quả giữa các task và ISR, đặc biệt trong các hệ thống nhúng thời gian thực. Việc lựa chọn loại queue phù hợp sẽ giúp tối ưu hóa hiệu năng và độ tin cậy của hệ thống.