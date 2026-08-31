# Hướng Dẫn Chi Tiết Về SysVInit Script Và Tích Hợp Yocto (`lvgl-app-init`)

## 1. SysVInit Là Gì? Tại Sao Tệp Này Lại Là SysVInit?

Trong các hệ điều hành Linux (đặc biệt là Linux nhúng Yocto), có 2 cơ chế quản lý dịch vụ khởi động (Init System):
1. **SysVinit (System V Init - Cổ điển / Siêu nhẹ)**:
   - Quản lý dịch vụ bằng các tệp **Shell Script** đặt trong thư mục `/etc/init.d/`.
   - Script hoạt động dựa trên các tham số truyền vào như `start`, `stop`, `restart`.
   - Rất nhẹ, khởi động nhanh, thích hợp cho hệ thống Linux nhúng dung lượng nhỏ (dùng BusyBox) như Raspberry Pi Zero.
2. **Systemd (Hiện đại)**:
   - Quản lý dịch vụ bằng các tệp cấu hình `.service` (ví dụ: `lvgl-app.service`).

Tệp `/home/ledat/yocto/meta-custom/recipes-apps/lvgl-app/files/lvgl-app-init` chính là một **SysVinit script**. Trong Yocto recipe, khi bạn khai báo `inherit update-rc.d`, Yocto sẽ tự động copy tệp này vào `/etc/init.d/lvgl-app` trên board để Linux tự động chạy ứng dụng LVGL ngay khi bật nguồn.

---

## 2. Giải Thích Chi Tiết Từng Dòng Code Trong `lvgl-app-init`

Dưới đây là nội dung chi tiết của tệp:

```sh
#!/bin/sh
### BEGIN INIT INFO
# Provides:          lvgl-app
# Required-Start:    $local_fs
# Required-Stop:     $local_fs
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: Start LVGL Application
### END INIT INFO

case "$1" in
  start)
    echo "Starting LVGL Application..."
    start-stop-daemon -S -b -a /usr/bin/lvgl-app
    ;;
  stop)
    echo "Stopping LVGL Application..."
    start-stop-daemon -K -n lvgl-app
    ;;
  restart)
    $0 stop
    $0 start
    ;;
  *)
    echo "Usage: $0 {start|stop|restart}"
    exit 1
    ;;
esac

exit 0
```

### 2.1 Đoạn 1: Khai báo Shell & Header Metadata (Dòng 1 - 9)
- `#!/bin/sh`: Khai báo trình thông dịch shell thực thi script.
- Block `### BEGIN INIT INFO`: Các dòng chú thích chứa metadata chuẩn LSB cho hệ thống:
  - `Provides: lvgl-app`: Tên định danh của dịch vụ.
  - `Required-Start: $local_fs`: Đảm bảo bộ nhớ cục bộ (`$local_fs`) đã mount xong mới cho chạy app.
  - `Default-Start: 2 3 4 5`: Tự động chạy ứng dụng ở các Runlevel từ 2 đến 5 (chế độ đa người dùng / đồ họa bình thường).
  - `Default-Stop: 0 1 6`: Tắt dịch vụ khi Tắt máy (0), Chế độ đơn người dùng (1), hoặc Khởi động lại (6).

### 2.2 Đoạn 2: Xử Lý Tham Số Đầu Vào (Dòng 11 - 28)

#### Ý nghĩa của các biến đặc biệt:
- **`$0`**: Đại diện cho **tên của chính tệp script** (`/etc/init.d/lvgl-app`).
- **`$1`**: Đại diện cho **tham số thứ nhất** được truyền vào sau tên tệp.

#### Cụ thể từng nhánh trong `case "$1" in`:

1. **Nhánh `start)` (Khi tham số `$1` = "start")**:
   ```sh
   echo "Starting LVGL Application..."
   start-stop-daemon -S -b -a /usr/bin/lvgl-app
   ```
   - `start-stop-daemon`: Tiện ích chuẩn của Linux chuyên quản lý các tiến trình ngầm (daemon).
   - `-S` (`--start`): Lệnh bắt đầu chạy tiến trình.
   - `-b` (`--background`): Chạy ứng dụng dưới nền background để không chặn tiến trình boot của Linux.
   - `-a /usr/bin/lvgl-app`: Đường dẫn thực tế đến file thực thi LVGL đã biên dịch trên board.

2. **Nhánh `stop)` (Khi tham số `$1` = "stop")**:
   ```sh
   echo "Stopping LVGL Application..."
   start-stop-daemon -K -n lvgl-app
   ```
   - `-K` (`--stop`): Lệnh dừng tiến trình.
   - `-n lvgl-app`: Tìm và tắt tiến trình có tên `lvgl-app`.

3. **Nhánh `restart)` (Khi tham số `$1` = "restart")**:
   ```sh
   $0 stop
   $0 start
   ```
   - Lấy `$0` (tên script) gọi lại chính nó với tham số `stop`, sau đó gọi lại chính nó với tham số `start` để khởi động lại ứng dụng.

4. **Nhánh `*)` (Khi nhập sai tham số)**:
   - In hướng dẫn sử dụng `Usage: ...` và thoát với lỗi `exit 1`.

---

## 3. Cơ Chế Yocto Tự Động Cài Đặt Khởi Động SysVinit Khi Boot

### 3.1 Yocto Làm Thế Nào Để App Tự Động Được Gọi Khi Boot?

Quá trình tự động cài đặt trải qua 3 bước:

1. **Khai báo trong Recipe (`lvgl-app_1.0.bb`)**:
   ```bitbake
   inherit update-rc.d
   INITSCRIPT_NAME = "lvgl-app"
   INITSCRIPT_PARAMS = "defaults 99"
   ```

2. **Yocto thực thi `update-rc.d.bbclass` trong quá trình Build RootFS**:
   Khi build image, Yocto sẽ tạo ra các **Symbolic Link** (đường dẫn tắt) nằm trong các thư mục Runlevel trên board:
   - `/etc/rc2.d/S99lvgl-app -> /etc/init.d/lvgl-app`
   - `/etc/rc3.d/S99lvgl-app -> /etc/init.d/lvgl-app`
   - `/etc/rc4.d/S99lvgl-app -> /etc/init.d/lvgl-app`
   - `/etc/rc5.d/S99lvgl-app -> /etc/init.d/lvgl-app`
   - `/etc/rc0.d/K99lvgl-app -> /etc/init.d/lvgl-app` (Kill khi Shutdown)
   - `/etc/rc6.d/K99lvgl-app -> /etc/init.d/lvgl-app` (Kill khi Reboot)

   > **Giải thích ký hiệu `S99`**:
   > - **`S`** = **Start** (Tự động thực thi lệnh `start` khi tiến vào Runlevel này).
   > - **`99`** = **Độ ưu tiên (Priority)** từ 00 đến 99. Số 99 nghĩa là chạy sau cùng (sau khi bộ nhớ, filesystem, mạng và driver đã sẵn sàng).
   > - **`K`** = **Kill** (Tự động thực thi lệnh `stop` khi thoát khỏi Runlevel).

3. **Thực thi lúc Boot**:
   Khi Raspberry Pi bật nguồn và tiến vào Runlevel 3 hoặc 5, tiến trình `/sbin/init` duyệt thư mục `/etc/rc3.d/`, thấy file `S99lvgl-app` trỏ về `/etc/init.d/lvgl-app`, và tự động gọi:
   ```bash
   /etc/init.d/lvgl-app start
   ```

---

## 4. Hướng Dẫn Chỉnh Sửa Yocto Nếu Muốn Gọi Ứng Dụng THỦ CÔNG (Tắt Tự Động Boot)

Nếu bạn muốn cài ứng dụng vào board nhưng **KHÔNG cho tự động chạy khi bật máy** (để bạn tự mở terminal gõ lệnh chạy thủ công khi cần debug), có 2 cách chỉnh sửa trong Yocto:

### **Cách 1: Xóa bỏ `update-rc.d` khỏi Recipe `lvgl-app_1.0.bb` (Khuyên dùng)**

Mở file `meta-custom/recipes-apps/lvgl-app/lvgl-app_1.0.bb` và thực hiện các chỉnh sửa sau:

```diff
SUMMARY = "LVGL Application for Raspberry Pi"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

- SRC_URI = "file://lvgl-app-init"

- inherit cmake update-rc.d externalsrc
+ inherit cmake externalsrc

EXTERNALSRC = "${TOPDIR}/../Simulator_Linux/lv_port_pc_vscode"
EXTERNALSRC_BUILD = "${WORKDIR}/build"

- INITSCRIPT_NAME = "lvgl-app"
- INITSCRIPT_PARAMS = "defaults 99"

do_install() {
    install -d ${D}${bindir}
    if [ -f ${EXTERNALSRC}/bin/main ]; then
        install -m 0755 ${EXTERNALSRC}/bin/main ${D}${bindir}/lvgl-app
    elif [ -f ${B}/bin/main ]; then
        install -m 0755 ${B}/bin/main ${D}${bindir}/lvgl-app
    else
        install -m 0755 ${B}/main ${D}${bindir}/lvgl-app
    fi

-   install -d ${D}${sysconfdir}/init.d
-   install -m 0755 ${WORKDIR}/lvgl-app-init ${D}${sysconfdir}/init.d/lvgl-app
}

FILES:${PN} += " \
    ${bindir}/lvgl-app \
-   ${sysconfdir}/init.d/lvgl-app \
"
```

#### Kết quả sau khi nạp firmware:
- File nhị phân `/usr/bin/lvgl-app` vẫn được cài vào board bình thường.
- Yocto không tạo bất kỳ đường dẫn tự khởi động nào trong `/etc/rc*.d/`.
- Khi bật máy, app **KHÔNG tự chạy**.
- Để chạy app thủ công qua SSH/Terminal, bạn chỉ cần gõ:
  ```bash
  lvgl-app
  ```
  hoặc
  ```bash
  /usr/bin/lvgl-app
  ```

---

### **Cách 2: Giữ file Init Script nhưng Tắt Tự Động Boot Bằng `INITSCRIPT_PARAMS`**

Nếu bạn vẫn muốn giữ file script `/etc/init.d/lvgl-app` trên board để dùng lệnh `/etc/init.d/lvgl-app start|stop`, nhưng không muốn Linux tự gọi nó lúc boot:

Sửa dòng `INITSCRIPT_PARAMS` trong `lvgl-app_1.0.bb`:
```bitbake
INITSCRIPT_NAME = "lvgl-app"
INITSCRIPT_PARAMS = "stop 99 2 3 4 5 ."
```
*(Tham số `stop` khiến Yocto không tạo các link `S99...` khởi động ở bất kỳ runlevel nào)*.

#### Thao tác tắt tự động chạy trực tiếp trên Board (không cần re-build Yocto):
Nếu board đã đang chạy, bạn chỉ cần SSH vào board và nhập lệnh hủy tự động boot:
```bash
update-rc.d -f lvgl-app remove
```

---

## 5. Bảng Tóm Tắt Tra Cứu Thao Tác

| Nhu cầu | Chỉnh sửa trong Recipe `lvgl-app_1.0.bb` | Lệnh thực thi trên Board |
|---|---|---|
| **Tự động chạy khi Boot (Mặc định)** | `inherit update-rc.d`<br>`INITSCRIPT_PARAMS = "defaults 99"` | Tự động chạy khi bật nguồn |
| **Gọi thủ công (Tắt Boot tự động)** | Bỏ `update-rc.d` và các dòng `INITSCRIPT_*` | Gõ lệnh: `lvgl-app` |
| **Bật/Tắt app bằng script thủ công** | Giữ `do_install` file `/etc/init.d/lvgl-app` | Gõ lệnh: `/etc/init.d/lvgl-app start` hoặc `stop` |
