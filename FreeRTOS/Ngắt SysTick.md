Dưới đây là **tổng hợp rõ ràng và chi tiết** các nội dung chính của bài viết *“Hoạt động của ngắt SysTick trong FreeRTOS: hiểu rõ cơ chế và ưu tiên ngắt”* từ Viblo (Vietnamese), kèm giải thích ngắn gọn, dễ hiểu hơn dựa trên nguyên lý FreeRTOS và ARM Cortex-M — tất cả nội dung đều dựa trên bài gốc đã mở. ([Viblo][1])

---

## 📌1. Vai trò của ngắt **SysTick** trong FreeRTOS

* **SysTick** là bộ định thời 24-bit tích hợp trên các vi điều khiển **ARM Cortex-M**. Nó dùng để tạo một ngắt định kỳ (thường là **1 ms**) để cung cấp *time base* cho hệ thống. ([Viblo][1])
* Trong **FreeRTOS**, ngắt này phục vụ:
  📌 Tăng biến đếm thời gian hệ thống (`xTickCount`)
  📌 Kiểm tra các timeout (delay, queue, semaphore, mutex)
  📌 Kích hoạt scheduler để chuyển task nếu cần (preemptive scheduling) ([Viblo][1])

---

## 📌2. Cách FreeRTOS xử lý ngắt **SysTick**

### 🧠 ISR xử lý SysTick

FreeRTOS định nghĩa hàm ngắt như sau:

```c
void SysTick_Handler(void)
{
    if (xTaskIncrementTick() != pdFALSE)
    {
        portYIELD_FROM_ISR();
    }
}
```

Giải thích:

* `xTaskIncrementTick()` *tăng số tick*, kiểm tra xem có task hết timeout, hay task ưu tiên cao hơn đã sẵn sàng chưa.
* Nếu có, `portYIELD_FROM_ISR()` *gợi ý scheduler thực hiện context switch* sau khi ISR kết thúc. ([Viblo][1])

---

## 📌3. Cơ chế ưu tiên ngắt và nested interrupts (ngắt lồng nhau)

### 🧱 3.1 NVIC và ưu tiên ngắt

ARM Cortex-M dùng NVIC để quản lý ngắt: mỗi ngắt có một mức ưu tiên số.

* **0** là mức ưu tiên **cao nhất**.
* Ngắt có ưu tiên cao hơn có thể *lồng vào* ngắt đang chạy. ([Viblo][1])

### 🌀 3.2 Khi có nhiều ngắt xảy ra chung thời điểm

| Tình huống                                       | Hành vi CPU                               | Tick có bị mất không?      |
| ------------------------------------------------ | ----------------------------------------- | -------------------------- |
| Ngắt khác đang chạy, SysTick đến                 | SysTick *chờ* cho ngắt hiện tại xong      | ❌ Không mất, chỉ bị trễ    |
| SysTick đang chạy, ngắt khác ưu tiên cao hơn đến | CPU tạm dừng SysTick để xử lý ngắt mới    | ❌ Không mất, chỉ tạm ngừng |
| Ngắt khác ưu tiên thấp đến khi SysTick đang chạy | Ngắt thấp phải chờ cho SysTick xử lý xong | ❌ Không mất ([Viblo][1])   |

👉 Như vậy: **Tick không bao giờ bị bỏ qua**, nhưng có thể bị *trễ do ưu tiên ngắt khác*. ([Viblo][1])

---

## 📌4. Ưu tiên *SysTick* trong FreeRTOS

### 🧩 Mức ưu tiên

* FreeRTOS thường cấu hình **SysTick ở mức ưu tiên thấp** (dưới `configMAX_SYSCALL_INTERRUPT_PRIORITY`).
* Điều này cho phép các ngắt quan trọng hơn (UART, ADC, …) *chạy trước SysTick* mà không bị chặn. ([Viblo][1])

⚠️ Lưu ý quan trọng:

* Nếu đặt SysTick quá cao so với `configMAX_SYSCALL_INTERRUPT_PRIORITY`, việc gọi API FreeRTOS trong ISR khác có thể *lỗi* vì quy định ngắt gọi API phải có ưu tiên hợp lệ. ([Viblo][1])

Các tài liệu chính thức FreeRTOS còn ghi rằng **PendSV nhất thiết phải là mức thấp nhất**, và SysTick *khuyến khích* đặt thấp để chừa ưu tiên cho các ISR khác khi cần. ([FreeRTOS][2])

---

## 📌5. Tương tác SysTick với scheduler và các task

* Mỗi lần ngắt SysTick xảy ra, FreeRTOS gọi `xTaskIncrementTick()`: đây là bước then chốt để hệ thống RTOS biết rằng một “tick” đã trôi qua. ([Viblo][1])
* Nếu tick này khiến một task *timeout, hết delay, hay sẵn sàng chạy* và có *ưu tiên cao hơn đang chạy*, scheduler sẽ được yêu cầu *chuyển context ngay sau khi ISR xong*. ([Viblo][1])

---

## 📌6. Tóm tắt nhanh

✔ **SysTick là trái tim thời gian** của FreeRTOS – định thời và điều phối scheduler. ([Viblo][1])
✔ **ISR SysTick** tăng tick count và yêu cầu scheduler nếu cần. ([Viblo][1])
✔ **Ưu tiên ngắt** quyết định thứ tự xử lý: SysTick đặt thấp để không làm chậm ISR khác. ([Viblo][1])
✔ **Tick không bao giờ mất**, chỉ có thể *bị trễ* nếu ngắt khác ưu tiên cao hơn chiếm CPU. ([Viblo][1])
✔ Scheduler hoạt động ngay sau ISR khi cần chuyển task. ([Viblo][1])

---

