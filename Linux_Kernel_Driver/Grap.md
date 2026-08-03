Viết driver cho Linux là một quá trình đòi hỏi sự hiểu biết về kiến trúc kernel, cách quản lý tài nguyên và giao tiếp với phần cứng. Dựa trên các tài liệu được cung cấp, dưới đây là các bước và khái niệm cốt lõi để tìm hiểu về cách viết một driver Linux:

### 1. Kiến trúc tổng quan của một Driver
Trong Linux, một driver thường giao tiếp với hai thành phần chính:
*   **Framework (Khung làm việc):** Cho phép driver phơi bày (expose) các tính năng của phần cứng tới các ứng dụng không gian người dùng (user space) thông qua một giao diện chuẩn (ví dụ: giao diện mạng, tty, v4l, v.v.).
*   **Bus infrastructure (Hạ tầng bus):** Là một phần của mô hình thiết bị (device model), dùng để phát hiện và giao tiếp với phần cứng ở mức vật lý (ví dụ: USB, PCI, I2C, SPI).

### 2. Cấu trúc cơ bản của một Kernel Module
Một driver thường được viết dưới dạng một kernel module (có đuôi `.ko`), cho phép tải (load) và gỡ (unload) động vào kernel mà không cần khởi động lại hệ thống. Cấu trúc cơ bản bao gồm:
*   **Khai báo thư viện:** Không sử dụng thư viện C chuẩn mà phải dùng các header của kernel, ví dụ `<linux/init.h>`, `<linux/module.h>`.
*   **Hàm khởi tạo (`__init`) và hàm dọn dẹp (`__exit`):** 
    *   Hàm `__init` được gọi khi module được tải vào kernel và thường trả về `0` nếu thành công.
    *   Hàm `__exit` được gọi khi module bị gỡ bỏ.
*   **Đăng ký module:** Sử dụng các macro **`module_init()`** và **`module_exit()`** để chỉ định hàm nào sẽ chạy khi khởi tạo và dọn dẹp.
*   **Siêu dữ liệu (Metadata):** Các macro như `MODULE_LICENSE("GPL")`, `MODULE_DESCRIPTION()`, và `MODULE_AUTHOR()` được dùng để mô tả thông tin và giấy phép của module.

### 3. Tương tác với không gian người dùng (User Space)
Hầu hết các driver giao tiếp với ứng dụng thông qua các tập tin thiết bị (device files) bằng cách hoạt động như một **Character Driver**.
*   **Giao diện `struct file_operations`:** Driver cần triển khai các hàm như `read`, `write`, `open`, `release`, và `unlocked_ioctl` để ứng dụng có thể dùng các system call tương ứng tương tác với thiết bị.
*   **Trao đổi dữ liệu an toàn:** Kernel không được phép truy cập trực tiếp bộ nhớ của user space vì lý do bảo mật và ổn định. Thay vào đó, driver **bắt buộc phải sử dụng các hàm đặc biệt** như `copy_to_user()`, `copy_from_user()`, `get_user()`, hoặc `put_user()` để di chuyển dữ liệu.

### 4. Mô hình Platform Driver và Device Tree
Đối với các thiết bị được tích hợp sẵn trên System-on-Chip (SoC) và không có khả năng tự phát hiện (như UART, I2C, SPI controllers), Linux sử dụng **Platform Bus**.
*   **Struct `platform_driver`:** Driver sẽ định nghĩa cấu trúc này, bao gồm hai hàm quan trọng là **`probe()`** và **`remove()`**.
    *   `probe()`: Được gọi khi kernel tìm thấy một thiết bị khớp với driver. Hàm này chịu trách nhiệm khởi tạo phần cứng, ánh xạ bộ nhớ (mapping I/O), đăng ký ngắt (interrupts) và đăng ký thiết bị vào framework.
    *   `remove()`: Thực hiện các công việc ngược lại để dọn dẹp khi thiết bị bị gỡ.
*   **Matching qua Device Tree (DT):** Driver liên kết với thiết bị phần cứng thông qua thuộc tính **`compatible`** (ví dụ: `compatible = "st,stm32-uart"`). Khi chuỗi `compatible` trong Device Tree khớp với danh sách `of_match_table` của driver, hàm `probe()` sẽ được kích hoạt.

### 5. Quản lý tài nguyên an toàn với Device Managed Allocations (devm_*)
Để tránh rò rỉ bộ nhớ hoặc lỗi quên giải phóng tài nguyên trong các hàm `probe()` (khi có lỗi xảy ra) và `remove()`, kernel cung cấp các hàm cấp phát tự động dọn dẹp bắt đầu bằng tiền tố **`devm_`**.
*   Ví dụ: Sử dụng **`devm_kzalloc()`** thay vì `kzalloc()` để cấp phát bộ nhớ, hoặc **`devm_request_irq()`** để đăng ký ngắt. Tài nguyên sẽ tự động được giải phóng khi thiết bị bị gỡ bỏ hoặc tách khỏi driver.

### 6. Biên dịch Driver (Compiling)
Có hai cách chính để biên dịch một driver:
*   **In-tree (Tích hợp trong mã nguồn kernel):** Tích hợp driver vào hệ thống Kconfig và Makefile của kernel. Bạn cần thêm file `.c` vào thư mục phù hợp, cập nhật file `Kconfig` (tạo tuỳ chọn `tristate`) và thêm lệnh `obj-$(CONFIG_...) += mydriver.o` vào `Makefile`.
*   **Out-of-tree (Bên ngoài mã nguồn):** Dùng một Makefile riêng sử dụng biến `M` trỏ tới thư mục mã nguồn module và gọi Makefile của kernel để biên dịch (`$(MAKE) -C $(KDIR) M=$$PWD`). Để làm điều này, bạn cần có kernel headers hoặc mã nguồn kernel đã được cấu hình.

Bạn có thể tham khảo trực tiếp các ví dụ mã nguồn thực tế trong nhân Linux ở thư mục `drivers/` để hiểu rõ hơn cách các API này được áp dụng (như `drivers/tty/serial/imx.c` hoặc `drivers/net/usb/rtl8150.c`).


Trong phát triển Linux kernel driver, hàm `probe()` thường phải cấp phát rất nhiều loại tài nguyên khác nhau như: bộ nhớ, ánh xạ các thanh ghi I/O, đăng ký bộ xử lý ngắt (interrupt handlers), v.v.. Trước đây, lập trình viên phải tự viết các đoạn mã dọn dẹp (giải phóng) các tài nguyên này trong trường hợp hàm `probe()` gặp lỗi giữa chừng, hoặc bên trong hàm `remove()` khi thiết bị bị gỡ bỏ. Điều này dễ dẫn đến rò rỉ bộ nhớ hoặc lỗi logic do mã dọn dẹp hiếm khi được kiểm thử đầy đủ.

Để giải quyết vấn đề này, Linux đã giới thiệu danh mục **Device Managed Allocations** (bao gồm các hàm có tiền tố **`devm_`**). Ý tưởng cốt lõi là **liên kết việc cấp phát tài nguyên với cấu trúc `struct device`** đại diện cho thiết bị. Kernel sẽ **tự động giải phóng** các tài nguyên này khi thiết bị biến mất hoặc khi thiết bị bị tách (unbound) khỏi driver.

Dưới đây là chi tiết về các nhóm hàm `devm_` phổ biến và cách chúng hoạt động dựa trên các tài liệu:

### 1. Nhóm hàm cấp phát bộ nhớ (Memory Allocation)
Thay vì dùng `kmalloc()` hay `kzalloc()`, driver sẽ dùng các hàm `devm_` với tham số đầu tiên luôn là con trỏ tới `struct device`:
*   **`devm_kmalloc()`** và **`devm_kzalloc()`**: Cấp phát bộ nhớ quản lý bởi thiết bị (trong đó `kzalloc` tự động khởi tạo vùng nhớ bằng 0).
*   **`devm_kcalloc()`**: Cấp phát bộ nhớ cho một mảng gồm `n` phần tử với kích thước `size` và khởi tạo bằng 0.
*   **`devm_kfree()`**: Dù tài nguyên sẽ được tự động giải phóng, bạn vẫn có thể dùng hàm này nếu muốn ép giải phóng một vùng nhớ ngay lập tức (ví dụ trong hàm `probe()` để tiết kiệm RAM khi không cần dùng nữa).

### 2. Nhóm hàm ánh xạ bộ nhớ I/O (I/O Memory Mapping)
*   **`devm_platform_ioremap_resource()`**: Kết hợp ba bước phức tạp (lấy địa chỉ vật lý bằng `platform_get_resource`, xin cấp phát vùng I/O bằng `request_mem_region`, và ánh xạ ảo bằng `ioremap`) thành một hàm duy nhất. Nếu thành công, hàm trả về con trỏ ảo có thể truy cập được, và vùng ánh xạ này sẽ tự hủy khi thiết bị gỡ bỏ.

### 3. Nhóm hàm quản lý Ngắt (Interrupts)
*   **`devm_request_irq()`**: Dùng để đăng ký một hàm xử lý ngắt (`irq_handler`).
*   **`devm_request_threaded_irq()`**: Dùng để đăng ký hàm xử lý ngắt theo cơ chế phân luồng (threaded IRQ). 
Khi thiết bị gỡ bỏ, ngắt sẽ tự động được huỷ đăng ký (unregister) mà không cần bạn phải tự gọi `free_irq()` trong hàm `remove()`.

### 4. Nhóm hàm quản lý Năng lượng, Xung nhịp và Đặt lại (Power, Clocks, Resets)
*   **`devm_clk_get()`**: Lấy tham chiếu đến một nguồn xung nhịp (clock) cho thiết bị từ Common Clock Framework.
*   **`devm_reset_control_get()`**: Lấy điều khiển cho các chân reset của phần cứng.
*   **`devm_regulator_get_enable()`**: Lấy và bật một bộ điều áp (regulator) để cấp nguồn cho thiết bị.

### 5. Nhóm hàm liên quan tới các Framework cụ thể
Các framework riêng biệt trong kernel cũng cung cấp hàm `devm_` để tự động dọn dẹp các cấu trúc đặc thù của chúng:
*   **`devm_iio_device_alloc()`**: Dùng để cấp phát thiết bị đo lường/cảm biến trong framework IIO.
*   **`devm_rtc_allocate_device()`**: Dùng để cấp phát thiết bị thời gian thực (RTC).

### ⚠️ Lưu ý quan trọng (Caveats) khi sử dụng `devm_`
Mặc dù rất tiện lợi, các tài liệu cũng cảnh báo một số điểm khi sử dụng nhóm hàm này:
1.  **Dọn dẹp gắn liền với chu kỳ sống của `struct device`:** Không có cơ chế đếm tham chiếu (reference counting) độc lập nào. Tài nguyên bị xóa sạch ngay khi `struct device` bị dọn dẹp.
2.  **Không dùng cho tài nguyên được truy cập bên ngoài thiết bị:** Nếu vùng nhớ `devm_` được dùng bởi một file thiết bị ở không gian người dùng (userspace), và file này vẫn đang được mở (open) sau khi thiết bị đã bị gỡ bỏ (remove), vùng nhớ đó có thể bị kernel giải phóng mất, dẫn đến lỗi truy cập.
3.  **Cẩn thận với vòng lặp tham chiếu (circular references):** Việc liên kết chéo các tài nguyên có thể gây ra lỗi khi giải phóng bộ nhớ.