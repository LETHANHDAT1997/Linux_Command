# ISR Yield: Khám Phá Cơ Chế Tương Tác Giữa Ngắt Và Task Trong RTOS

## Giới thiệu nhanh

Bài viết tập trung vào khái niệm **ISR Yield** trong FreeRTOS – một hệ điều hành thời gian thực (RTOS) phổ biến cho các hệ thống nhúng. ISR Yield là cơ chế giúp quản lý tương tác giữa ngắt (ISR) và các task, đảm bảo rằng sau khi ISR đánh thức một task có độ ưu tiên cao hơn, hệ thống sẽ chuyển ngữ cảnh (context switch) ngay lập tức để chạy task đó. Điều này tránh độ trễ không cần thiết, đặc biệt quan trọng trong các ứng dụng thời gian thực trên vi điều khiển như ESP32 hoặc STM32.

---

## 1. ISR Yield là gì?

**ISR Yield** là quá trình yêu cầu lập lịch lại (context switch) từ bên trong ISR nếu có task có độ ưu tiên cao hơn task hiện tại đã chuyển sang trạng thái ready sau khi ISR xử lý.

Trong FreeRTOS, không nên gọi trực tiếp `vTaskSwitchContext()` trong ISR. Thay vào đó, sử dụng các hàm API phiên bản "FromISR" kết hợp với `portYIELD_FROM_ISR()` để yêu cầu chuyển đổi an toàn.

### Ví dụ code:

```c
BaseType_t xHigherPriorityTaskWoken = pdFALSE;
xQueueSendFromISR(queueHandle, &data, &xHigherPriorityTaskWoken);
portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
```

- `xQueueSendFromISR()` cập nhật `xHigherPriorityTaskWoken` để chỉ ra liệu có task ưu tiên cao hơn đã sẵn sàng.
- `portYIELD_FROM_ISR()` chỉ thực hiện yield nếu `xHigherPriorityTaskWoken` là `pdTRUE`.

---

## 2. Tại sao cần ISR Yield?

Không sử dụng ISR Yield có thể dẫn đến các vấn đề:

- **Bỏ qua task ưu tiên cao**: ISR đánh thức task B (ưu tiên cao) trong khi task A (ưu tiên thấp) đang chạy, nhưng task A tiếp tục sau ISR, gây trễ.
- **Vi phạm nguyên tắc ưu tiên**: RTOS sử dụng lập lịch ưu tiên preemptive; ISR Yield giúp duy trì điều này.

### Ví dụ minh họa:

- Task A (thấp) đang chạy.
- ISR xảy ra, đánh thức Task B (cao) → Task B ready.
- Không yield: Task A tiếp tục → trễ.
- Có yield: Chuyển sang Task B ngay sau ISR.

---

## 3. Cơ chế hoạt động bên trong

### 3.1. Vai trò của `xHigherPriorityTaskWoken`

Các API "FromISR" trả về `xHigherPriorityTaskWoken` (kiểu `BaseType_t`):

- `pdTRUE`: Có task ưu tiên cao hơn đã ready (ví dụ: nhận dữ liệu từ queue).
- `pdFALSE`: Không thay đổi ưu tiên, không cần yield.

Điều này tránh yield không cần thiết, giảm overhead.

### 3.2. Vai trò của `portYIELD_FROM_ISR`

Đây là macro/hàm port-specific:

- Đánh dấu cần context switch sau ISR.
- Kernel kiểm tra và thực hiện switch nếu cần.

#### Ví dụ trên các nền tảng:

**ARM Cortex-M:**

```c
#define portYIELD_FROM_ISR(x) if(x != pdFALSE) portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT
```

- Kích hoạt PendSV cho context switch.

**Xtensa (ESP32):**

```c
#define portYIELD_FROM_ISR() portYIELD()
```

- Gọi yield trực tiếp.

---

## 4. Ví dụ thực tế

### Mô hình: Xử lý dữ liệu UART qua ISR

UART nhận byte, ISR đọc và gửi vào queue. Task ưu tiên cao chờ xử lý.

```c
void uart_isr_handler(void *arg) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    char data = UART0.fifo.rw_byte;

    xQueueSendFromISR(uartQueue, &data, &xHigherPriorityTaskWoken);

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
```

- Nếu task chờ queue có ưu tiên cao hơn, yield kích hoạt switch.

### Biểu đồ thời gian (mô tả)

Luồng: ISR (top-half) chạy nhanh, đẩy dữ liệu; sau đó switch đến task (bottom-half) nếu cần.

---

## 5. Sai lầm phổ biến

| Sai lầm | Hậu quả |
|---------|---------|
| Không gọi `portYIELD_FROM_ISR()` sau API FromISR | Task ưu tiên cao không chạy ngay, vi phạm real-time. |
| Gọi `vTaskDelay()` trong ISR | Không hợp lệ; ISR phải ngắn, không gọi syscall thông thường. |
| Không kiểm tra `xHigherPriorityTaskWoken` | Yield thừa (giảm hiệu suất) hoặc thiếu (gây trễ). |

---

## 6. Tối ưu hóa ISR với ISR Yield

### Chiến lược:

- **Tách ISR**: Top-half (ISR nhanh, đẩy dữ liệu); Bottom-half (task xử lý).
- **Yield có điều kiện**: Chỉ khi cần, giảm context switch.
- **Kết hợp Task Notification**: Nhanh hơn queue cho báo hiệu đơn giản; dùng `xTaskNotifyFromISR()`.

---

## 7. Khi nào không cần ISR Yield?

- ISR báo hiệu task ưu tiên bằng/thấp hơn task hiện tại.
- Trì hoãn switch (ví dụ: critical section).
- ISR ưu tiên cao hơn tất cả task (hiếm).
- Nếu vượt `configMAX_SYSCALL_INTERRUPT_PRIORITY`, không gọi API FreeRTOS.

---

## 8. Kết luận

ISR Yield là cơ chế quan trọng trong FreeRTOS để giảm trễ, đảm bảo ưu tiên, và tăng phản ứng thời gian thực. Hiểu và áp dụng đúng giúp hệ thống nhúng ổn định, đặc biệt trên ESP32/STM32. Đây là yếu tố phân biệt kỹ sư chuyên nghiệp trong thiết kế RTOS.