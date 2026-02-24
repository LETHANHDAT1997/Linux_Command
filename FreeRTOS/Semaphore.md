# Giới thiệu về Semaphore trong RTOS

## 1. Giới thiệu nhanh

Trong các hệ thống nhúng và hệ điều hành thời gian thực (RTOS), việc đồng bộ hóa giữa các tiến trình (process) hoặc tác vụ (task) là một vấn đề cốt lõi. Khi nhiều task cùng truy cập vào tài nguyên dùng chung như bộ nhớ, cổng I/O, cảm biến hay vùng dữ liệu toàn cục, nguy cơ xung đột và lỗi dữ liệu là rất cao. Để giải quyết bài toán này, **Semaphore** ra đời như một công cụ đồng bộ đơn giản nhưng mạnh mẽ.

Semaphore giúp đảm bảo rằng chỉ có một số lượng hạn chế task có thể truy cập vào tài nguyên tại cùng một thời điểm, từ đó tránh được xung đột và đảm bảo hệ thống hoạt động ổn định.

---

## 2. Khái niệm Semaphore là gì?

**Semaphore** là một biến đặc biệt được sử dụng để quản lý truy cập tới tài nguyên dùng chung trong môi trường đa luồng hoặc đa tác vụ. Nó hoạt động như một bộ đếm, xác định xem có bao nhiêu task được phép truy cập tài nguyên tại cùng một thời điểm.

### 2.1 Các loại Semaphore

Semaphore có hai loại chính:

| Loại Semaphore | Mô tả | Ứng dụng |
| --- | --- | --- |
| **Binary Semaphore** | Chỉ có hai giá trị: 0 và 1 | Đồng bộ giữa hai task hoặc ISR và task |
| **Counting Semaphore** | Có giá trị đếm >= 0 | Quản lý tài nguyên có số lượng giới hạn như buffer, connection slot... |

---

## 3. Cơ chế hoạt động

### 3.1 Nguyên tắc cơ bản

Semaphore hoạt động dựa trên 3 thao tác cơ bản:

* **Khởi tạo** ( xSemaphoreCreate... ): Tạo semaphore với giá trị ban đầu.
* **Take (P operation)** : Task "chiếm" semaphore, nếu semaphore bằng 0 thì task sẽ bị block (đợi).
* **Give (V operation)** : Task hoặc ISR "nhả" semaphore, tăng giá trị semaphore lên và đánh thức các task đang chờ (nếu có).

### 3.2 Minh họa hoạt động của Binary Semaphore

```
sequenceDiagram
    participant ISR
    participant Task_A
    participant Resource

    Note over ISR,Task_A: Semaphore khởi tạo với giá trị = 0

    Task_A->>Semaphore: xSemaphoreTake()
    Semaphore-->>Task_A: Block vì Semaphore = 0

    ISR->>Semaphore: xSemaphoreGive()
    Semaphore-->>Task_A: Unblock, tiếp tục

    Task_A->>Resource: Truy cập tài nguyên
```

---

## 4. Semaphore và Mutex có giống nhau không?

Mặc dù có điểm chung trong việc đồng bộ hóa, **Semaphore** và **Mutex** có một số khác biệt rõ rệt:

| Tiêu chí | Semaphore | Mutex |
| --- | --- | --- |
| Số lượng task có thể sở hữu | N task (tùy theo counting) | Chỉ 1 task |
| Hỗ trợ ưu tiên owner | Không | Có |
| Tái nhập (Recursive) | Không | Có (nếu tạo recursive mutex) |
| Dùng cho đồng bộ ISR-task | Có (binary semaphore) | Không |

---

## 5. Ứng dụng thực tế của Semaphore trong RTOS

### 5.1 Đồng bộ giữa ISR và Task

Khi một ngắt (ISR) xảy ra, nó không nên xử lý nhiều mà chỉ nên gửi tín hiệu cho một task xử lý hậu kỳ. Lúc này, **Binary Semaphore** là giải pháp tối ưu.

#### Ví dụ:

```c
// ISR
void IRAM_ATTR gpio_isr_handler(void* arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xBinarySem, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// Task xử lý ngắt
void vInterruptHandlerTask(void* pvParameters) {
    while (1) {
        if (xSemaphoreTake(xBinarySem, portMAX_DELAY) == pdTRUE) {
            // Xử lý sau ngắt
        }
    }
}
```

### 5.2 Quản lý truy cập tài nguyên giới hạn

Khi nhiều task cùng truy cập vào một tài nguyên hữu hạn (như buffer có 3 slot), **Counting Semaphore** sẽ giúp quản lý số lượng truy cập đồng thời.

```c
SemaphoreHandle_t xCountingSem;

xCountingSem = xSemaphoreCreateCounting(3, 3); // Buffer có 3 vị trí

// Trong mỗi task:
if (xSemaphoreTake(xCountingSem, portMAX_DELAY)) {
    // Truy cập buffer
    ...
    xSemaphoreGive(xCountingSem);
}
```

---

## 6. Cách sử dụng Semaphore trong FreeRTOS

### 6.1 Các hàm API chính

| Hàm | Mô tả |
| --- | --- |
| xSemaphoreCreateBinary() | Tạo binary semaphore |
| xSemaphoreCreateCounting(max, init) | Tạo counting semaphore |
| xSemaphoreTake(sem, timeout) | Task lấy semaphore |
| xSemaphoreGive(sem) | Task trả semaphore |
| xSemaphoreGiveFromISR(sem, *xHigherPriorityTaskWoken) | Trả từ ISR |

### 6.2 Tính năng timeout và blocking

Semaphore cho phép chỉ định thời gian chờ khi task không lấy được semaphore:

```c
// Task sẽ chờ 100ms nếu không lấy được semaphore
if (xSemaphoreTake(xSem, pdMS_TO_TICKS(100))) {
    // Có semaphore
} else {
    // Xử lý timeout
}
```

---

## 7. Ưu điểm và Nhược điểm

### Ưu điểm:

* Đơn giản, dễ sử dụng.
* Hiệu quả cho các bài toán đồng bộ ISR - task.
* Quản lý số lượng truy cập linh hoạt.

### Nhược điểm:

* Không tự xác định được task nào sở hữu semaphore.
* Dễ gây deadlock nếu dùng sai.
* Không có tính "ưu tiên chủ sở hữu" như mutex.

---

## 8. Sơ đồ minh họa tổng quan Semaphore

```
graph TD
    A[Task A] -->|xSemaphoreTake| B[Semaphore]
    C[Task B] -->|xSemaphoreTake| B
    B -->|Nếu có sẵn| D[Tài nguyên]
    B -->|Nếu không sẵn| E[Task Block]
    F[ISR] -->|xSemaphoreGiveFromISR| B
    B -->|Wakeup Task| E
```

---

## 9. Các lỗi thường gặp và cách khắc phục

| Lỗi | Nguyên nhân | Giải pháp |
| --- | --- | --- |
| Deadlock | Task A chờ semaphore mà Task B giữ nhưng không bao giờ nhả | Rà soát logic, tránh chờ lẫn nhau |
| Starvation | Task ưu tiên thấp không bao giờ lấy được semaphore | Cân bằng ưu tiên task |
| Gọi xSemaphoreGive() khi không có xSemaphoreTake() | Sai thứ tự thao tác | Đảm bảo cấu trúc Take -> sử dụng -> Give đúng logic |

---

## 10. Kết luận

Semaphore là một công cụ mạnh mẽ trong lập trình hệ thống, đặc biệt là hệ điều hành thời gian thực như FreeRTOS. Việc hiểu rõ cách hoạt động và sử dụng semaphore giúp nhà phát triển nhúng tránh được các lỗi khó phát hiện như race condition, deadlock và đảm bảo hệ thống hoạt động mượt mà và tin cậy.

Trong các bài học tiếp theo, bạn nên tìm hiểu sâu hơn về **mutex** , **event group** và **task notification** – các công cụ đồng bộ nâng cao hơn – để xây dựng hệ thống nhúng mạnh mẽ, mở rộng và tối ưu hơn.