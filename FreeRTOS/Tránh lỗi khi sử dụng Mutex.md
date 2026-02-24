# Tránh lỗi khi sử dụng Mutex trong RTOS – Những nguyên tắc vàng

## Giới thiệu nhanh

Bài viết tập trung vào việc sử dụng Mutex trong RTOS (như FreeRTOS) để bảo vệ tài nguyên dùng chung, đảm bảo chỉ một task truy cập tại một thời điểm. Tuy nhiên, sử dụng sai có thể dẫn đến deadlock, priority inversion hoặc vi phạm tài nguyên. Bài viết phân tích các lỗi phổ biến và đưa ra các nguyên tắc vàng để tránh, giúp hệ thống ổn định và dễ bảo trì.

---

## I. Tóm tắt nhanh về Mutex

- **Mutex (Mutual Exclusion)**: Cơ chế đảm bảo chỉ một task truy cập tài nguyên dùng chung.
- Hỗ trợ **priority inheritance** để giảm priority inversion.
- Chỉ task đã **take** mới được **give** Mutex.
- **Không dùng trong ISR** (ngắt).

---

## II. Các lỗi phổ biến khi dùng Mutex và cách tránh

### 1. Quên `give` Mutex sau khi `take`

Đây là lỗi thường gặp nhất, dẫn đến Mutex bị khóa vĩnh viễn, các task khác không thể truy cập.

#### Ví dụ lỗi:

```c
if (xSemaphoreTake(xMutex, 100) == pdTRUE) {
    // thao tác với tài nguyên
    // quên xSemaphoreGive(xMutex);
}
```

#### Cách tránh:
- Luôn đặt `xSemaphoreGive` ngay sau khối xử lý tài nguyên.
- Sử dụng `goto` hoặc cơ chế cleanup để xử lý thoát sớm.
- Với C++, áp dụng **RAII** để tự động `give`.

---

### 2. Dùng Mutex trong ISR

Mutex không an toàn trong ISR vì không có khái niệm "task sở hữu".

#### Sai:

```c
void ISR_Handler() {
    xSemaphoreGive(xMutex); // Sai vì gọi từ ISR
}
```

#### Cách tránh:
- Thay bằng **Binary Semaphore** hoặc **Task Notification**.

```c
xSemaphoreGiveFromISR(xBinarySemaphore, &xHigherPriorityTaskWoken);
```

---

### 3. Dùng sai Mutex khi chỉ cần Semaphore

Mutex dành cho bảo vệ tài nguyên, không phải đồng bộ sự kiện.

#### Cách tránh:
- Mutex: Bảo vệ tài nguyên (biến toàn cục, UART).
- Binary Semaphore: Đồng bộ ISR-task hoặc task-task.

---

### 4. Deadlock – Hai task chờ nhau vô tận

Xảy ra khi các task take Mutex theo thứ tự khác nhau.

#### Ví dụ:

```c
// Task A
xSemaphoreTake(Mutex1, portMAX_DELAY);
xSemaphoreTake(Mutex2, portMAX_DELAY);

// Task B
xSemaphoreTake(Mutex2, portMAX_DELAY);
xSemaphoreTake(Mutex1, portMAX_DELAY);
```

#### Cách tránh deadlock:
1. **Thống nhất thứ tự take Mutex** (ví dụ: luôn Mutex1 trước Mutex2).
2. **Sử dụng timeout** thay vì `portMAX_DELAY`:

```c
if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    // xử lý
} else {
    // timeout, tránh deadlock
}
```

3. Giải phóng Mutex nếu lỗi xảy ra sớm.

---

### 5. Ưu tiên đảo ngược (Priority Inversion)

Task ưu tiên thấp giữ Mutex, task cao chờ; task trung bình chen ngang gây trễ.

#### Cách tránh:
- Sử dụng Mutex (hỗ trợ priority inheritance) thay vì Binary Semaphore.
- Không tắt priority inheritance trừ khi cần thiết.

---

## III. Nguyên tắc vàng khi sử dụng Mutex

### 1. **Thời gian giữ Mutex càng ngắn càng tốt**
- Giảm nghẽn bằng cách xử lý nhanh và `give` sớm.
- Tránh delay, blocking call hoặc vòng lặp dài trong khi giữ Mutex.

### 2. **Không chồng lệnh take mà không give**
- Nếu cần take nhiều lần trong cùng task, dùng **Recursive Mutex**:

```c
xSemaphoreTakeRecursive(xMutex, portMAX_DELAY);
// gọi lồng nhau
xSemaphoreGiveRecursive(xMutex);
```

### 3. **Không chia sẻ Mutex với các task không biết quy tắc**
- Đóng gói Mutex trong module riêng, tránh bên ngoài gọi trực tiếp.

### 4. **Debug dễ hơn với tên mô tả**
- Gán tên cho Mutex để dễ trace:

```c
xMutexUart = xSemaphoreCreateMutex();
vQueueAddToRegistry(xMutexUart, "UART Mutex");
```

### 5. **Sử dụng công cụ kiểm tra runtime**
- Sử dụng FreeRTOS Trace hoặc Percepio Tracealyzer để theo dõi thời gian giữ, phát hiện nghẽn và deadlock.

---

## IV. Sơ đồ các lỗi và cách phòng tránh

| **Lỗi**                     | **Cách tránh**                              |
|-----------------------------|---------------------------------------------|
| Quên give Mutex             | Luôn đặt give sau xử lý                     |
| Dùng Mutex trong ISR        | Dùng Binary Semaphore thay thế              |
| Dùng sai Mutex/Semaphore    | Xác định rõ mục đích sử dụng                |
| Deadlock                    | Thống nhất thứ tự - timeout                 |
| Priority Inversion          | Dùng Mutex, enable inheritance              |
| Giữ Mutex quá lâu           | Tối ưu logic bên trong Mutex                |

---

## V. Một số mẫu code đúng chuẩn

### Mẫu dùng Mutex đúng cách:

```c
void TaskSafeWrite(void *pvParameters) {
    for (;;) {
        if (xSemaphoreTake(xMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            write_to_resource();
            xSemaphoreGive(xMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

### Dùng `goto` để đảm bảo giải phóng (không khuyến nghị nhưng hữu ích):

```c
void critical_op() {
    if (xSemaphoreTake(xMutex, 100) != pdTRUE) return;

    if (!init()) goto cleanup;

    // Xử lý chính
    if (!process()) goto cleanup;

cleanup:
    xSemaphoreGive(xMutex);
}
```

---

## VI. Kết luận

Mutex là công cụ thiết yếu trong RTOS để bảo vệ tài nguyên, nhưng sử dụng sai có thể gây lỗi nghiêm trọng. Áp dụng các nguyên tắc vàng: luôn give sau take, tránh dùng trong ISR, thống nhất thứ tự, sử dụng timeout, giữ ngắn gọn, và dùng công cụ debug. Những thói quen này giúp tránh lỗi khó debug sau này. Bài tiếp theo sẽ thảo luận về mẫu thiết kế module với Mutex và unit test deadlock.