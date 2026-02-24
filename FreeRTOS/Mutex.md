# Hiểu rõ về Mutex trong FreeRTOS!!!

## 1. Giới thiệu nhanh
- Bài viết giới thiệu **Mutex** như một cơ chế đồng bộ và bảo vệ tài nguyên dùng chung trong FreeRTOS, khác biệt so với **Semaphore**.
- Mục tiêu: Giải thích **Mutex là gì**, **khác biệt với Semaphore**, **khi nào nên dùng**, và **cách sử dụng đúng cách**.

---

## 2. Mutex là gì?
- **Mutex** (Mutual Exclusion) – “loại trừ lẫn nhau”.
- Đảm bảo **chỉ một task duy nhất truy cập tài nguyên tại một thời điểm**.
- Tài nguyên có thể là:
  - Biến dùng chung giữa các task.
  - Thiết bị ngoại vi (ví dụ: UART).
  - Hàm xử lý **không reentrant** (không an toàn khi gọi đồng thời).

---

## 3. Mutex hoạt động như thế nào?
- Hoạt động tương tự **Binary Semaphore**, nhưng có **2 điểm khác biệt quan trọng**:

### a. Khái niệm “chủ sở hữu”
- Chỉ **task đã `take` Mutex thành công** mới được phép `give` lại.
- Không giống Semaphore – bất kỳ task nào cũng có thể `give`.

### b. Hỗ trợ **Priority Inheritance** (thừa kế ưu tiên)
- Ngăn **priority inversion** (đảo ngược ưu tiên).
- Ví dụ:
  - Task A (ưu tiên thấp) giữ Mutex.
  - Task B (ưu tiên cao) đang đợi.
  - FreeRTOS **tạm thời nâng ưu tiên Task A** lên bằng Task B để giảm thời gian chờ.

---

## 4. Minh họa cơ bản về Mutex (Sơ đồ hoạt động)

```
+-----------+       +-----------+       +-----------+
| Task Low  |       | Task Med  |       | Task High |
| (prio 1)  |       | (prio 2)  |       | (prio 3)  |
+-----------+       +-----------+       +-----------+
     |                   |                   |
     |--- take Mutex ---->                   |
     |<-- Mutex acquired --                  |
     |          ... (đang dùng tài nguyên)   |
     |                   |                   |
     |                   |--- ready -------->|
     |                   |                   |
     |     Task High muốn take Mutex         |
     |<----- blocked trên Mutex -------------|
     |     Priority Inheritance kicks in     |
     |      Task Low tạm nâng lên prio 3     |
     |      Task Low kết thúc sử dụng        |
     |---- give Mutex ---------------------> |
     |      Task High tiếp tục chạy          |
```

### Kết luận từ sơ đồ:
- Nếu dùng **Semaphore**: Task High có thể chờ lâu nếu Task Low không trả kịp.
- Với **Mutex**: **Priority Inheritance** giúp giảm thời gian chờ không cần thiết.

---

## 5. Cách dùng Mutex trong FreeRTOS

### Tạo Mutex:
```c
SemaphoreHandle_t xMutex = xSemaphoreCreateMutex();
```

### Task sử dụng Mutex:
```c
void Task1(void *pvParameters)
{
    for(;;)
    {
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE)
        {
            // Dùng tài nguyên
            // ...
            xSemaphoreGive(xMutex);
        }
    }
}
```

### Recursive Mutex (Mutex đệ quy)
- Dùng khi một task cần `take` nhiều lần liên tiếp (gọi lồng nhau).
- Tạo bằng:
```c
xSemaphoreCreateRecursiveMutex();
```
- API tương ứng: `xSemaphoreTakeRecursive`, `xSemaphoreGiveRecursive`.

---

## 6. Mutex vs Semaphore – So sánh chi tiết

| Tiêu chí | Mutex | Binary Semaphore |
|--------|-------|------------------|
| Mục đích chính | Bảo vệ tài nguyên dùng chung | Đồng bộ giữa các task/ISR |
| Ai được `give` | Chỉ task đã `take` | Bất kỳ task/ISR nào |
| Hỗ trợ Priority Inheritance | Có | Không |
| Gọi từ ISR | Không | Có thể |
| Hỗ trợ Recursive | Có (Recursive Mutex) | Không |
| Trọng lượng bộ nhớ | Nặng hơn | Nhẹ hơn |

### Khi nào dùng cái nào?
- Dùng **Mutex** để:
  - Bảo vệ tài nguyên dùng chung (UART, I2C, biến toàn cục…).
- Dùng **Binary Semaphore** để:
  - Đồng bộ giữa các task hoặc giữa ISR và task.

---

## 7. Ví dụ thực tế: Bảo vệ UART khi ghi từ nhiều task

Nếu nhiều task cùng ghi log qua UART mà không có Mutex → dữ liệu bị trộn lẫn.

```c
void vLogTask(void *pvParam)
{
    for(;;)
    {
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE)
        {
            printf("Task %d: Hello\n", (int)pvParam);
            xSemaphoreGive(xMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

Trong `main()`:
```c
xMutex = xSemaphoreCreateMutex();
xTaskCreate(vLogTask, "Log1", 1024, (void*)1, 1, NULL);
xTaskCreate(vLogTask, "Log2", 1024, (void*)2, 1, NULL);
xTaskCreate(vLogTask, "Log3", 1024, (void*)3, 1, NULL);
```

---

## 8. Lưu ý khi sử dụng Mutex

### Không dùng trong ISR
- Mutex **không an toàn** khi dùng từ ISR.
- Không có context để lưu thông tin “ai đang sở hữu”.
- Thay bằng: **Binary Semaphore** hoặc **Direct To Task Notification**.

### Luôn `give` sau khi `take`
- Nếu quên `give` → **deadlock** (các task khác không thể truy cập mãi mãi).

### Tránh lạm dụng
- Chỉ dùng khi thực sự cần bảo vệ tài nguyên dùng chung.
- Dùng thừa → tốn RAM và CPU.

---

## 9. Tình huống nguy hiểm: Deadlock (Khóa chết)

Xảy ra khi 2 task giữ từng cái một và đợi lẫn nhau:

```c
// Task A
xSemaphoreTake(Mutex1, portMAX_DELAY);
xSemaphoreTake(Mutex2, portMAX_DELAY); // Kẹt nếu Mutex2 bị Task B giữ

// Task B
xSemaphoreTake(Mutex2, portMAX_DELAY);
xSemaphoreTake(Mutex1, portMAX_DELAY); // Kẹt nếu Mutex1 bị Task A giữ
```

### Giải pháp:
- Quy định **thứ tự take Mutex** thống nhất giữa các task.
- Dùng **timeout** khi `take`.
- Dùng **watchdog** để phát hiện deadlock.

---

## 10. Khi nào nên dùng Recursive Mutex?

Dùng khi:
- Một hàm đã `take` Mutex, và gọi một hàm khác cũng cần `take` cùng Mutex.
- Nếu không dùng Recursive → **deadlock**.

Ví dụ:
```c
void foo() {
    xSemaphoreTakeRecursive(xMutex, portMAX_DELAY);
    // ...
    xSemaphoreGiveRecursive(xMutex);
}

void bar() {
    xSemaphoreTakeRecursive(xMutex, portMAX_DELAY);
    foo(); // Không deadlock nhờ Recursive Mutex
    xSemaphoreGiveRecursive(xMutex);
}
```

---

## 11. Tổng kết

Mutex là công cụ quan trọng trong FreeRTOS để:

- **Bảo vệ tài nguyên dùng chung** (Mutual Exclusion).
- **Hỗ trợ Priority Inheritance** → tránh priority inversion.
- **Chỉ task sở hữu mới được `give`**.
- **Không dùng trong ISR**.
- **Có phiên bản Recursive Mutex** cho các trường hợp gọi lồng nhau.

Dùng đúng Mutex giúp hệ thống:
- Ổn định hơn.
- Tránh deadlock.
- Tối ưu hiệu suất khi nhiều task truy cập tài nguyên chung.