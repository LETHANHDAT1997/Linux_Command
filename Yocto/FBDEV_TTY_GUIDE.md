# Hướng Dẫn Khắc Phục Lỗi Xung Đột Linux Framebuffer (/dev/fb0) & TTY Console

## 1. Tóm Tắt Nhu Cầu & Vấn Đề

Khi chạy ứng dụng LVGL trực tiếp trên Linux Framebuffer (`/dev/fb0`) ở môi trường nhúng (như Raspberry Pi Zero 2W / Yocto Linux) mà không dùng Display Manager (X11 / Wayland), thường xuất hiện 2 lỗi hiển thị:
1. **Con trỏ TTY nhấp nháy**: Một hình chữ nhật nhỏ màu đen có gạch ngang bên dưới nhấp nháy đều đặn khoảng ~500ms tại một vị trí cố định trên màn hình (xuất hiện ở mọi UI).
2. **Vỡ UI khi gõ bàn phím**: Khi gõ phím trên bàn phím USB hoặc terminal, các ký tự, dấu xuống dòng và prompt của terminal sẽ vẽ đè lên `/dev/fb0`, làm hỏng giao diện LVGL.

---

## 2. Phân Tích Nguyên Nhân Gốc Rễ

### 2.1 Linux Framebuffer Console (`fbcon`)
Mặc định trên Linux, Kernel khởi tạo driver `fbcon` để hiển thị văn bản lên `/dev/fb0` cho các Virtual Terminal (`tty1`-`tty6`).

Khi ứng dụng LVGL ghi dữ liệu trực tiếp vào `/dev/fb0`:
- **TTY Console vẫn hoạt động ở `KD_TEXT` Mode**: Kernel không tự động biết rằng có một ứng dụng GUI đang làm chủ màn hình.
- **Vẽ con trỏ văn bản**: `fbcon` liên tục vẽ con trỏ (dạng khối đen / gạch ngang) đè lên `/dev/fb0` theo chu kỳ blink của terminal.
- **Terminal Echo**: Mọi phím gõ từ bàn phím kết nối với TTY active sẽ được driver console xử lý và in ký tự (echo) trực tiếp lên màn hình framebuffer, ghi đè lên vùng nhớ pixel của LVGL.

---

## 3. Giải Pháp C Code: Khởi Tạo `KD_GRAPHICS` Mode

Giải pháp chuẩn nhất ở cấp độ C code là chuyển TTY active sang chế độ **`KD_GRAPHICS`** bằng hàm `ioctl(fd, KDSETMODE, KD_GRAPHICS)`.

### 3.1 Tác Dụng Của `KD_GRAPHICS` Mode
- **Tắt xuất chữ của `fbcon`**: Ngăn Kernel vẽ bất kỳ văn bản nào lên `/dev/fb0`.
- **Tắt hoàn toàn con trỏ TTY**: Loại bỏ chấm đen / gạch ngang nhấp nháy.
- **Tắt Echo phím gõ**: Ngăn bàn phím gõ chữ làm đè/vỡ giao diện.
- **Giữ nguyên input `evdev`**: Các driver đầu vào của LVGL (`/dev/input/event*`) vẫn đọc dữ liệu cảm ứng, chuột, bàn phím bình thường.

### 3.2 Hiện Thực Trong C Code (`src/hal/hal.c`)

```c
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <stdlib.h>

static int s_tty_fd = -1;

/**
 * @brief Khôi phục TTY về chế độ văn bản (KD_TEXT) khi thoát ứng dụng.
 */
static void restore_tty_mode(void)
{
    if(s_tty_fd >= 0) {
        ioctl(s_tty_fd, KDSETMODE, KD_TEXT);
        close(s_tty_fd);
        s_tty_fd = -1;
    }
}

/**
 * @brief Chuyển TTY sang KD_GRAPHICS mode để tắt fbcon và tắt echo bàn phím.
 */
static void configure_tty_graphics_mode(void)
{
    const char * tty_paths[] = {"/dev/tty0", "/dev/tty1", "/dev/tty"};
    for(size_t i = 0; i < sizeof(tty_paths)/sizeof(tty_paths[0]); i++) {
        int fd = open(tty_paths[i], O_RDWR);
        if(fd >= 0) {
            if(ioctl(fd, KDSETMODE, KD_GRAPHICS) == 0) {
                s_tty_fd = fd;
                atexit(restore_tty_mode);
                printf("[HAL] Console %s đã chuyển sang KD_GRAPHICS mode (Tắt TTY text echo).\n", tty_paths[i]);
                return;
            }
            close(fd);
        }
    }
    printf("[HAL WARNING] Không thể chuyển TTY sang KD_GRAPHICS mode.\n");
}
```

---

## 4. Giải Pháp Cấu Hình Hệ Thống Yocto / Linux

Ngoài việc xử lý trong C code, bạn nên cấu hình hệ thống Yocto để chặn TTY ngay từ lúc boot.

### 4.1 Tham Số Kernel (`cmdline.txt` / U-Boot)
Ẩn con trỏ TTY toàn cục và chuyển log kernel ra cổng Serial TTL:

```text
console=ttyAMA0,115200 vt.global_cursor_default=0
```

### 4.2 Cấu Hình Service (Systemd / SysVinit)
Tắt dịch vụ đăng nhập (`getty`) trên `tty1` để không có tiến trình shell nào nhận input từ bàn phím:

#### Đối với Systemd:
```bash
systemctl disable getty@tty1.service
systemctl stop getty@tty1.service
```

#### Đối với SysVinit (`recipes-apps/lvgl-app/files/lvgl-app-init`):
Đảm bảo script khởi động ẩn con trỏ TTY trước khi chạy app:

```sh
case "$1" in
  start)
    echo "Starting LVGL Application..."
    # Ẩn con trỏ TTY1
    printf '\033[?25l' > /dev/tty1 2>/dev/null || true
    start-stop-daemon -S -b -a /usr/bin/lvgl-app
    ;;
```

---

## 5. Bảng Tóm Tắt Kiểm Tra

| Hiện tượng | Nguyên nhân | Cách khắc phục |
|---|---|---|
| Ô đen / gạch ngang nhấp nháy | Con trỏ văn bản của `fbcon` | Gọi `ioctl(fd, KDSETMODE, KD_GRAPHICS)` trong `hal.c` |
| Gõ bàn phím làm hỏng UI LVGL | TTY Echo chữ lên `/dev/fb0` | Dùng `KD_GRAPHICS` mode + Tắt `getty@tty1` |
| Màn hình bị chớp / lệch hình | Hardware double-buffering page flip | Ép `yres_virtual = yres` hoặc set `LV_LINUX_FBDEV_BUFFER_COUNT 0` |
