Dưới đây là **tóm tắt chi tiết và có cấu trúc** toàn bộ nội dung bài viết trên Viblo (đăng ngày 20/06/2025 bởi tác giả delinux). Tôi giữ nguyên cấu trúc gốc, thuật ngữ kỹ thuật, đoạn code, ví dụ minh họa và các điểm quan trọng để bạn dễ theo dõi mà không mất thông tin cốt lõi.

### 1. Giới thiệu
Bài viết tập trung vào lỗi **Stack Overflow** (tràn ngăn xếp) – một vấn đề phổ biến, nguy hiểm và khó phát hiện khi phát triển ứng dụng nhúng sử dụng **RTOS** (Real-Time Operating System).  
Nội dung bao gồm:  
- Khái niệm stack trong RTOS  
- Nguyên nhân gây tràn stack  
- Cách phát hiện lỗi  
- Các phương pháp xử lý & phòng tránh  
- Minh họa bằng biểu đồ và ví dụ thực tế trên ESP32/STM32.

### 2. Stack trong RTOS là gì?
Khi tạo một **task** trong RTOS, hệ thống cấp phát một vùng RAM riêng gọi là **stack** (ngăn xếp). Stack dùng để:  
- Lưu **biến cục bộ**  
- Lưu **địa chỉ trả về** của hàm  
- Lưu **ngữ cảnh task** khi context switching  

Mỗi task có stack riêng, kích thước được cấu hình khi tạo task (ví dụ: `xTaskCreate(..., 1024, ...)` → 1024 words).

### 3. Nguyên nhân gây Stack Overflow
- Sử dụng quá nhiều biến cục bộ hoặc gọi đệ quy sâu  
- Hàm quá nặng, gọi nhiều hàm con lồng nhau  
- Stack size cấu hình ban đầu quá nhỏ  
- Không bật cơ chế bảo vệ (stack sentinel/check)  

Trong kiến trúc ARM Cortex-M, stack phát triển từ địa chỉ cao xuống thấp → tràn sẽ ghi đè vùng nhớ khác, gây lỗi khó đoán (reset ngẫu nhiên, hard fault…).

### 4. Cách phát hiện Stack Overflow
#### 4.1. Sử dụng tính năng sẵn có của RTOS (FreeRTOS ví dụ)
```c
#define configCHECK_FOR_STACK_OVERFLOW 2   // 1 = chỉ sentinel, 2 = kiểm tra khi chuyển task
```
Khi phát hiện tràn, gọi hook:
```c
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    printf("Stack overflow tại task: %s\n", pcTaskName);
    // Xử lý: log flash, nháy LED, reset hệ thống...
}
```

#### 4.2. Kỹ thuật Canary / Fill Pattern
Đổ đầy stack bằng giá trị cố định (thường 0xA5A5A5A5) trước khi chạy task. Sau đó kiểm tra:
```c
size_t getStackUsed(uint32_t *stack, size_t stackSize) {
    size_t used = 0;
    for (size_t i = 0; i < stackSize; i++) {
        if (stack[i] != 0xA5A5A5A5) used++;
    }
    return used * sizeof(uint32_t);
}
```

#### 4.3. Công cụ debugger
- STM32CubeIDE, ESP-IDF, J-Link: xem mức stack usage trực tiếp  
- Set watchpoint ở đáy stack để bắt ghi đè

### 5. Phương pháp xử lý & phòng tránh
1. Xác định task nào dùng nhiều stack nhất (dùng `uxTaskGetStackHighWaterMark()`)  
2. Tăng stack size hợp lý:
   ```c
   xTaskCreate(taskFunc, "Task1", 1024, NULL, 1, NULL);  // Tăng lên 1536 hoặc 2048 nếu cần
   ```
3. Tối ưu code:
   - Tránh mảng lớn cục bộ → dùng heap (malloc) hoặc biến static/global  
   - Hạn chế đệ quy  
   - Tránh dùng thư viện stdio nặng trong task quan trọng  
4. Luôn bật hook xử lý tràn stack (log, cảnh báo, reset an toàn)  
5. Giám sát định kỳ:
   ```c
   UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(xHandle);
   if (uxHighWaterMark < 100) { /* Cảnh báo sớm */ }
   ```

### 6. Minh họa biểu đồ (mô tả)
- **Stack bình thường** (512 bytes): dùng 200 bytes, còn vùng fill pattern trống  
- **Stack tràn**: dùng 600 bytes → vượt giới hạn, ghi đè vùng nhớ khác  

### 7. Kinh nghiệm thực tế (ESP32 & STM32)
- Lỗi reset ngẫu nhiên → kiểm tra stack overflow trước tiên  
- Không dùng printf/printf-heavy trong task realtime  
- Luôn chạy debug build + đo high water mark trong giai đoạn phát triển

### 8. Checklist chống tràn stack
- Bật `configCHECK_FOR_STACK_OVERFLOW` → Done  
- Đo high water mark định kỳ → Done  
- Không dùng mảng lớn trong stack → Done  
- Có hook log lỗi khi tràn → Done  
- Tối ưu gọi hàm trong task → Done  

### 9. Kết luận
Stack Overflow là “cơn ác mộng” của lập trình viên RTOS nếu không kiểm soát.  
**Giải pháp cốt lõi**: chủ động đo lường, tối ưu code, cấu hình stack hợp lý và bật cơ chế bảo vệ.  
Câu nói nổi bật của bài:  
**“Stack không bao giờ là đủ nếu các bạn không kiểm soát nó.”**

Bài viết rất thực tế, dễ áp dụng ngay cho FreeRTOS/Zephyr/CMSIS-RTOS. Nếu bạn đang gặp vấn đề stack trên dự án cụ thể, có thể chia sẻ thêm để mình hỗ trợ chi tiết hơn!