# Hướng Dẫn Parse Device Tree & Triết Lý Thiết Kế Subsystem Trong Linux Kernel Driver

Tài liệu này tổng hợp toàn bộ kiến thức về cách ánh xạ (parse) các thuộc tính (properties) từ **Device Tree (DTS)** sang các hàm API trong **C Kernel Driver**, phân tích bản chất giữa **Raw DT Parsing** và **Subsystem API**, cùng triết lý thiết kế của Linux Kernel.

---

## 1. Ánh Xạ DTS Property Sang Hàm C Kernel Driver

Dưới đây là bảng tổng hợp các thuộc tính phổ biến trong file DTS (ví dụ từ [ledat-demo-overlay.dts](file:///d:/Linux/Linux_Command/Linux_Kernel_Driver/ledat-demo-overlay.dts)) và hàm C Kernel tương ứng được sử dụng trong driver (ví dụ trong [ledat_demo.c](file:///d:/Linux/Linux_Command/Linux_Kernel_Driver/ledat_demo.c)):

| Thuộc tính trong DTS | Kiểu dữ liệu / Mục đích | Hàm API C trong Kernel tương ứng | Ghi chú & Cách hoạt động |
| :--- | :--- | :--- | :--- |
| `compatible = "ledat,demo-dev";` | Chuỗi định danh thiết bị | Tự động khớp qua `struct of_device_id match_table[]` hoặc kiểm tra `of_device_is_compatible(np, "...")` | Kernel dùng chuỗi này để tìm driver phù hợp khi kích hoạt hàm `probe()`. |
| `status = "okay";` | Trạng thái bật/tắt node | `of_device_is_available(np)` | Trả về `true` nếu status là `"okay"` hoặc `"ok"`. |
| `reg = <0x50>;` | Địa chỉ I2C / Register | Tự động gán vào `client->addr` (với I2C bus) hoặc `of_property_read_u32(np, "reg", &val)` | Với bus driver (như I2C, SPI, Platform), kernel bus core tự parse giá trị này. |
| `sample-rate-hz = <1000>;` | Số nguyên 32-bit (`u32`) | `of_property_read_u32(np, "sample-rate-hz", &val)` hoặc `device_property_read_u32(dev, "sample-rate-hz", &val)` | Đọc 1 số nguyên 32-bit đơn lẻ. |
| `threshold-levels = <10 20 30>;` | Mảng số 32-bit (`u32 array`) | `of_property_read_u32_array(np, "threshold-levels", array, count)` | Đọc mảng gồm `count` phần tử số nguyên 32-bit. |
| `label = "demo-sensor-0";` | Chuỗi ký tự (string) | `of_property_read_string(np, "label", &str_ptr)` | Trả về con trỏ tới chuỗi ký tự trong bộ nhớ Device Tree. |
| `wakeup-source;` | Cờ Boolean | `of_property_read_bool(np, "wakeup-source")` | Trả về `true` nếu property có tồn tại trong node (không cần chứa giá trị). |
| `reset-gpios = <&gpio 17 1>;` | Chân GPIO điều khiển | `devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW)` hoặc hàm cũ `of_get_named_gpio(np, "reset-gpios", 0)` | GPIOD Framework tự khớp tiền tố `"reset"` với suffix `-gpios`, xử lý logic active high/low và cấp phát `struct gpio_desc *`. |
| `interrupts = <27 2>;`<br>`interrupt-parent = <&gpio>;` | Cấu hình Ngắt (IRQ) | I2C Core tự gán vào `client->irq`. Đọc thủ công: `of_irq_get(np, 0)` hoặc `platform_get_irq(pdev, 0)` | Trả về số IRQ hệ thống (virtual IRQ number) để sử dụng với `devm_request_threaded_irq()`. |
| `clocks = <&clk_osc 0>;`<br>`clock-names = "ext-clk";` | Nguồn Xung nhịp (Clock) | `devm_clk_get(dev, "ext-clk")` hoặc `of_clk_get_by_name(np, "ext-clk")` | Tra tên `"ext-clk"` để lấy đối tượng điều khiển `struct clk *` đã sẵn sàng để gọi `clk_prepare_enable()`. |
| `vdd-supply = <&reg_3v3>;` | Nguồn cấp điện (Regulator) | `devm_regulator_get(dev, "vdd")` | Regulator framework tự động gắn thêm suffix `-supply` để tìm node `<&reg_3v3>` và trả về `struct regulator *`. |
| `ledat,aux-phandle = <&aux_ctrl>;` | Tham chiếu tới Node khác (Phandle) | `of_parse_phandle(np, "ledat,aux-phandle", 0)` | Trả về con trỏ `struct device_node *` trỏ trực tiếp tới node `&aux_ctrl`. |

---

## 2. Phân Biệt: Đọc Dữ Liệu Thô (Raw Property) vs Sử Dụng Hàm Subsystem API

Để dễ hình dung nhất, hãy coi **Device Tree** giống như một **Danh bạ điện thoại**:

* **Đọc Dữ Liệu Thô (`of_property_read_*`):** Giống như việc bạn tra danh bạ và chép ra giấy chuỗi chữ `"Nguyễn Văn A"` hoặc số điện thoại `"0901234567"`. Bạn chỉ có **mẩu thông tin (văn bản/con số)** trên tay, chứ điện thoại chưa tự động bấm số gọi.
* **Hàm Subsystem API (`devm_gpiod_get`, `devm_clk_get`...):** Giống như việc bạn nhấn nút **"GỌI"** trên smartphone. Kernel sẽ tự tra danh bạ, tự quay số và nối máy. Kết quả là bạn nhận được **cuộc gọi đang kết nối (`struct clk *`, `struct gpio_desc *`)** để làm việc trực tiếp với phần cứng!

---

### 2.1. Thí dụ 1: `clock-names = "ext-clk"` dùng `of_property_read_string` thì sao?

* **Được không?** 👉 **ĐỌC ĐƯỢC**, hàm trả về chuỗi chữ `"ext-clk"`.
* **Vấn đề ở đâu?** Chuỗi chữ `"ext-clk"` chỉ là một cái tên đại diện trên giấy. Cầm chữ `"ext-clk"` trong tay, bạn **không thể bật/tắt hay đổi tần số** của bộ dao động thạch anh!
* **Cách làm đúng:** Sử dụng `devm_clk_get(dev, "ext-clk")`.
  * Kernel sẽ tự làm chuỗi hành động: Đọc chữ `"ext-clk"` $\rightarrow$ Dóng sang node `clocks = <&clk_osc 0>` $\rightarrow$ Tìm driver điều khiển thạch anh $\rightarrow$ Trả về con trỏ đối tượng phần cứng `struct clk *`.
  * Lúc này, bạn chỉ cần gọi `clk_prepare_enable(clk)` là thạch anh phần cứng sẽ thực sự phát ra xung nhịp!

---

### 2.2. Thí dụ 2: `reset-gpios = <&gpio 17 1>` dùng `of_property_read_u32_array` thì sao?

* **Được không?** 👉 **ĐỌC ĐƯỢC** ra 3 số nguyên, nhưng **3 SỐ ĐÓ HOÀN TOÀN VÔ DỤNG** để điều khiển phần cứng.
* **Tại sao lại vô dụng?**
  Khai báo `<&gpio 17 1>` khi biên dịch thành file nhị phân (`.dtb`) sẽ biến thành mảng 3 số: `[ID_ngẫu_nhiên, 17, 1]`:
  1. `ID_ngẫu_nhiên` (Phandle ID): Là một con số ngẫu nhiên do máy tính tự gán để đánh dấu node `&gpio`. Số này KHÔNG PHẢI địa chỉ RAM hay thanh ghi.
  2. Số `17`: Là số thứ tự chân GPIO 17. Nhưng nó mới chỉ là con số trần trụi, chân 17 này chưa được xin cấp phép sử dụng, chưa được chọn làm chân Xuất (Output) hay Nhập (Input).
  3. Số `1`: Là cờ báo Active Low.
* **Cách làm đúng:** Sử dụng `devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW)`.
  * Kernel sẽ tự động: Tra cứu ID ngẫu nhiên $\rightarrow$ Tìm đúng bộ điều khiển chip GPIO $\rightarrow$ Xin cấp phép dùng chân 17 $\rightarrow$ Cấu hình hướng Output $\rightarrow$ Trả về con trỏ `struct gpio_desc *`.
  * Lúc này, bạn chỉ cần gọi `gpiod_set_value(gpio, 1)` là điện áp trên chân 17 thực sự thay đổi!

---

## 3. Triết Lý Thiết Kế Của Linux Kernel: Provider - Consumer

### Tại sao Kernel lại thiết kế "lằng nhằng" như vậy?
Nếu không có kiến trúc này, bạn sẽ phải tự đọc datasheet của từng con SoC (Raspberry Pi/Broadcom, STM32, Allwinner, NXP...) để tự ghi trực tiếp vào thanh ghi phần cứng (bit-shift register). Hậu quả là code driver của bạn **không thể mang sang board khác chạy được**.

### Mô hình phân chia lớp trong Kernel:
1. **Provider Driver (Nhà cung cấp):** Do hãng sản xuất chip (SoC vendor) viết. Chịu trách nhiệm thao tác với các thanh ghi vật lý phức tạp của bộ điều khiển GPIO, Clock, Power, Interrupt Controller...
2. **Consumer Driver (Người tiêu dùng - chính là Driver bạn viết):** Driver cho cảm biến, màn hình, thiết bị ngoại vi.
3. **Device Tree (Sơ đồ nối dây):** Đóng vai trò là cuốn sổ tay mô tả kết nối giữa thiết bị của Consumer và bộ điều khiển của Provider.

👉 **Lợi ích:** Khi bạn gọi `gpiod_set_value(reset_gpio, 1);`, đoạn code này sẽ **chạy đúng 100% trên tất cả các loại board nhúng** mà không cần sửa đổi 1 dòng code C nào.

---

## 4. Phương Pháp Học & Tra Cứu Linux Kernel Driver

### 4.1. Tập trung vào 5-6 Subsystem Cốt Lõi (Chiếm 90% nhu cầu)
Không cần học thuộc lòng toàn bộ API của Kernel. Hãy làm chủ các nhóm hàm sau:

1. **GPIOD Framework:** `devm_gpiod_get()`, `gpiod_set_value()`, `gpiod_get_value()`
2. **Clock Framework:** `devm_clk_get()`, `clk_prepare_enable()`, `clk_disable_unprepare()`
3. **Regulator Framework:** `devm_regulator_get()`, `regulator_enable()`, `regulator_disable()`
4. **Interrupt Framework:** `devm_request_threaded_irq()`, `free_irq()`
5. **Bus Framework (I2C/SPI):** `i2c_smbus_read_byte_data()`, `spi_sync()`

### 4.2. Tra cứu tài liệu Binding trong Kernel Source
Khi gặp thiết bị mới, tra cứu cách viết Device Tree tại:
📁 `Documentation/devicetree/bindings/` (các file quy định thuộc tính bắt buộc `required` và tùy chọn `optional`).

### 4.3. Học từ Driver mẫu có sẵn
Mọi lập trình viên Kernel đều học bằng cách tham khảo các driver tương tự có sẵn trong thư mục `drivers/` của Linux Kernel Tree:
* Cảm biến / Đo lường: `drivers/iio/`
* Giám sát phần cứng: `drivers/hwmon/`
* Màn hình / Hế thống hiển thị: `drivers/gpu/drm/` hoặc `drivers/video/`
* Bộ nhớ / EEPROM: `drivers/misc/eeprom/`
