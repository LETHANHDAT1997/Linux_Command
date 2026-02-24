# Phân biệt các loại Delay khi sử dụng FreeRTOS

## Giới thiệu nhanh

Bài viết giải thích sự khác biệt giữa các hàm delay khi sử dụng FreeRTOS trong phát triển phần mềm nhúng, đặc biệt trên ESP32. Các hàm như `delay()`, `vTaskDelay()`, và `vTaskDelayUntil()` được phân tích về cách hoạt động, ảnh hưởng đến hệ thống và ứng dụng phù hợp. Mục tiêu giúp người đọc hiểu rõ cơ chế delay trong môi trường đa nhiệm (RTOS) so với lập trình đơn luồng.

---

## 1. delay() – Hồi ức của thời đơn luồng

- **Hành vi**: Hàm `delay(ms)` là vòng lặp bận (busy wait), giữ CPU không làm gì trong thời gian chờ.
- **Ví dụ code**:
  ```c
  void loop() {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);  // chờ 1 giây
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
  }
  ```
- **Vấn đề trong RTOS**: Làm nghẽn CPU, không nhường quyền cho task khác → không phù hợp với hệ thống đa nhiệm.

---

## 2. vTaskDelay() – bước chân đầu tiên vào thế giới RTOS

- **Hành vi**: Task tự nguyện ngủ (block) trong số tick nhất định, nhường CPU cho task khác.
- **Ví dụ code**:
  ```c
  void blinkTask(void *pvParameters) {
    while (1) {
      digitalWrite(LED_BUILTIN, HIGH);
      vTaskDelay(pdMS_TO_TICKS(1000));  // chờ 1 giây
      digitalWrite(LED_BUILTIN, LOW);
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
  }
  ```
- **Lưu ý**: Thời gian delay không chính xác tuyệt đối, có thể bị trễ do scheduler bận.

---

## 3. vTaskDelayUntil() – nghệ thuật giữ nhịp thời gian

- **Hành vi**: Duy trì chu kỳ định kỳ chính xác bằng cách theo dõi thời điểm đánh thức (`lastWakeTime`), đảm bảo task chạy đúng nhịp dù có trễ nhỏ.
- **Ví dụ code**:
  ```c
  void stableBlinkTask(void *pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(1000);

    while (1) {
      digitalWrite(LED_BUILTIN, HIGH);
      vTaskDelayUntil(&lastWakeTime, period / 2); // bật 500ms

      digitalWrite(LED_BUILTIN, LOW);
      vTaskDelayUntil(&lastWakeTime, period);     // tắt 500ms nữa
    }
  }
  ```
- **Ưu điểm**: Giữ nhịp ổn định, phù hợp cho các tác vụ định kỳ như đọc cảm biến, ghi log.

---

## 4. So sánh trực quan

| Hàm               | Hành vi                              | Ứng dụng phù hợp                     |
|-------------------|--------------------------------------|---------------------------------------|
| `delay()`         | Chặn CPU, không nhường               | Đơn luồng, demo                      |
| `vTaskDelay()`    | Ngủ mềm, nhường CPU                 | Task tự chạy định kỳ                 |
| `vTaskDelayUntil()` | Giữ nhịp định kỳ chuẩn             | Sensor sampling, log dữ liệu         |

### Sơ đồ dòng thời gian (Timeline - ticks)

```
Task A: | Run |------delay--------| Run |------delay--------| ...
Task B: |         Run         |         Run         |         Run ...
         ↑                    ↑
      vTaskDelay()       vTaskDelay()

Task C: | Run |------delay_until(1000)--------| Run |---delay_until(2000)---| ...
         ↑                       ↑
      vTaskDelayUntil()     (Duy trì nhịp 1 giây)
```

---

## 5. Thí nghiệm nhỏ: delay() vs vTaskDelay() vs vTaskDelayUntil()

- **Mô tả**: Chạy 3 task nhấp nháy LED trên ESP32 với 3 loại delay khác nhau.
- **Code ví dụ**:
  ```c
  void taskA(void *) {
    pinMode(2, OUTPUT);
    while (1) {
      digitalWrite(2, !digitalRead(2));
      delay(500);  // BLOCKING delay
    }
  }

  void taskB(void *) {
    pinMode(4, OUTPUT);
    while (1) {
      digitalWrite(4, !digitalRead(4));
      vTaskDelay(pdMS_TO_TICKS(500)); // cooperative delay
    }
  }

  void taskC(void *) {
    pinMode(5, OUTPUT);
    TickType_t lastWake = xTaskGetTickCount();
    while (1) {
      digitalWrite(5, !digitalRead(5));
      vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(500)); // rhythmic
    }
  }
  ```
- **Kết quả**:
  - LED 2: Có thể làm trễ cả hệ thống (do block CPU).
  - LED 4: Chớp bình thường nhưng không đều nếu hệ thống bận.
  - LED 5: Chớp đều, đúng chu kỳ, ổn định nhất.

---

## 6. Một số lưu ý nhỏ

- `vTaskDelayUntil()` phụ thuộc vào độ chính xác của tick (mặc định 100Hz → 10ms/tick trên ESP32).
- Nếu task bị block lâu do ưu tiên cao hơn, có thể bị trượt nhịp.
- Không phù hợp cho tác vụ realtime cực cao → cần dùng interrupt hoặc timer phần cứng.

---

## 7. Kết luận: delay không chỉ là chờ đợi

- Ba loại delay như ba phong cách sống của task:
  - `delay()`: Không quan tâm đến người khác (block).
  - `vTaskDelay()`: Biết nhường nhịn (cooperative).
  - `vTaskDelayUntil()`: Sống theo nhịp điệu riêng (periodic).
- Hiểu sâu về delay trong RTOS cần thực hành và thử nghiệm để nắm vững.
- Kiến thức này là nền tảng để tối ưu hóa scheduler trong tương lai.