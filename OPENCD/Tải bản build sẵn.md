Để tải bản build sẵn (pre-built) từ **xPack**, bạn có hai cách: tải thủ công bằng trình duyệt hoặc sử dụng dòng lệnh (phù hợp để viết vào script tự động hóa). xPack là nguồn cực kỳ uy tín vì họ duy trì các bản build ổn định cho nhiều nền tảng (Windows, Linux, macOS).

Dưới đây là cách thực hiện:

### 1. Tải thủ công từ GitHub (Cách nhanh nhất)

1. Truy cập vào trang [xPack OpenOCD Releases](https://github.com/xpack-dev-tools/openocd-xpack/releases).
2. Tìm phiên bản mới nhất (ví dụ: `v0.12.0-4`).
3. Trong phần **Assets**, hãy chọn file phù hợp với Linux:
   * File có dạng: `xpack-openocd-0.12.0-4-linux-x64.tar.gz`
4. Tải về và giải nén vào thư mục `.tools/` trong project của bạn.

---

### 2. Tải bằng dòng lệnh (Dùng cho Script hoặc Dockerfile)

Nếu bạn muốn tích hợp việc tải này vào quá trình setup dự án, hãy sử dụng `curl` hoặc `wget`:

```bash
# Tạo thư mục chứa công cụ
mkdir -p .tools
cd .tools

# Tải bản xPack OpenOCD (ví dụ bản 0.12.0-4)
wget https://github.com/xpack-dev-tools/openocd-xpack/releases/download/v0.12.0-4/xpack-openocd-0.12.0-4-linux-x64.tar.gz

# Giải nén
tar -xvf xpack-openocd-0.12.0-4-linux-x64.tar.gz

# Xóa file nén sau khi giải nén để tiết kiệm dung lượng
rm xpack-openocd-0.12.0-4-linux-x64.tar.gz

# Đổi tên thư mục cho gọn (tùy chọn)
mv xpack-openocd-0.12.0-4 openocd
```

---

### 3. Cách sử dụng OpenOCD từ thư mục dự án

Khi bạn đã để OpenOCD trong `.tools/openocd`, cấu trúc lệnh sẽ khác một chút vì bạn phải chỉ định đường dẫn đến file cấu hình (scripts) nằm bên trong thư mục đó.

**Lệnh mẫu để nạp code:**
```bash
#!/bin/bash
# flash_stm32_openocd.sh - Script nạp firmware cho STM32 bằng OpenOCD
set -e

echo "=== Flashing STM32 via OpenOCD ==="

# 1. Đường dẫn đến file .elf (giữ nguyên cấu trúc của bạn)
ELF_PATH="../../build/MyCProject.elf"

# 2. Đường dẫn đến OpenOCD và thư mục scripts
PROJECT_ROOT="/"
OPENOCD_BIN="openocd-0.12.0-7-linux-x64/xpack-openocd-0.12.0-7/bin/openocd"
OPENOCD_SCRIPTS="openocd-0.12.0-7-linux-x64/xpack-openocd-0.12.0-7/openocd/scripts"

# 3. Cấu hình phần cứng (Thay đổi TARGET_CFG cho phù hợp với chip của bạn)
INTERFACE_CFG="interface/stlink.cfg"
TARGET_CFG="target/stm32f4x.cfg" # Sửa lại thành stm32f1x.cfg, stm32g4x.cfg... nếu cần

echo "[INFO] Sử dụng OpenOCD tại: $OPENOCD_BIN"
echo "[INFO] Đang nạp file: $ELF_PATH"

# ---------------------------------------------------------
# KIỂM TRA VÀ CẤP QUYỀN THỰC THI CHO OPENOCD
# ---------------------------------------------------------
# Kiểm tra xem file có tồn tại không trước khi cấp quyền
if [ ! -f "$OPENOCD_BIN" ]; then
    echo "[ERROR] Không tìm thấy file thực thi OpenOCD tại: $OPENOCD_BIN"
    exit 1
fi

# Kiểm tra quyền thực thi
if [ ! -x "$OPENOCD_BIN" ]; then
    echo "[INFO] OpenOCD chưa có quyền thực thi. Đang tiến hành cấp quyền..."
    chmod +x "$OPENOCD_BIN"
    echo "[OK] Đã cấp quyền thực thi (chmod +x) cho OpenOCD."
else
    echo "[INFO] OpenOCD đã có sẵn quyền thực thi. Bỏ qua bước cấp quyền."
fi
# ---------------------------------------------------------

# 4. Thực thi nạp firmware bằng OpenOCD
"$OPENOCD_BIN" \
    -s "$OPENOCD_SCRIPTS" \
    -f "$INTERFACE_CFG" \
    -f "$TARGET_CFG" \
    -c "program \"$ELF_PATH\" verify reset exit"

# Kiểm tra kết quả
if [ $? -ne 0 ]; then
    echo "[ERROR] Flash failed!"
    exit 1
fi

echo "[OK] Flash successful!"
```
> **Lưu ý quan trọng:** Flag `-s ./.tools/openocd/scripts` cực kỳ quan trọng. Nó chỉ cho OpenOCD biết nơi tìm các file cấu hình mạch nạp và chip (interface/target) mà bạn đã tải về kèm theo.

---

### 4. Thiết lập quyền truy cập USB (udev rules)

Mặc dù bạn chạy OpenOCD từ thư mục riêng, Ubuntu vẫn cần "giấy phép" để phần mềm đó chạm vào phần cứng qua cổng USB.

Trong thư mục bạn vừa tải về đã có sẵn file rule. Hãy chạy lệnh này một lần duy nhất trên máy Ubuntu của bạn:

```bash
#!/bin/bash
# setup_usb_rules.sh - Kiểm tra và cấp quyền truy cập USB cho OpenOCD
set -e

echo "=== Kiểm tra quyền truy cập USB (udev rules) ==="

RULE_FILENAME="60-openocd.rules"
DEST_PATH="/etc/udev/rules.d/$RULE_FILENAME"

# Cập nhật đường dẫn này trỏ tới file rules bên trong thư mục của bạn.
# Tùy vào phiên bản xPack, nó thường nằm ở share/openocd/contrib/ hoặc scripts/
SRC_PATH="openocd-0.12.0-7-linux-x64/xpack-openocd-0.12.0-7/openocd/contrib/$RULE_FILENAME"

# 1. Kiểm tra xem quy tắc udev đã có trong hệ thống chưa
if [ -f "$DEST_PATH" ]; then
    echo "[OK] Quyền truy cập USB cho OpenOCD đã được thiết lập ($DEST_PATH)."
    echo "Bỏ qua bước cài đặt."
    exit 0
fi

echo "[INFO] Chưa tìm thấy cấu hình udev cho OpenOCD. Tiến hành thiết lập..."

# 2. Yêu cầu quyền sudo vì việc ghi vào /etc/udev/rules.d/ cần quyền root
if [ "$(id -u)" -ne 0 ]; then 
    echo "[ERROR] Lệnh này cần quyền root để cài đặt file rules vào hệ thống."
    echo "Vui lòng chạy lại script bằng lệnh: sudo ./setup_usb_rules.sh"
    exit 1
fi

# 3. Kiểm tra xem file rules gốc (trong project) có tồn tại không
if [ ! -f "$SRC_PATH" ]; then
    echo "[ERROR] Không tìm thấy file rules gốc tại: $SRC_PATH"
    echo "Vui lòng kiểm tra lại cấu trúc thư mục .tools của bạn và sửa lại SRC_PATH trong script này."
    exit 1
fi

# 4. Thực hiện copy và khởi động lại dịch vụ udev
echo "[INFO] Đang copy $SRC_PATH -> $DEST_PATH..."
cp "$SRC_PATH" "$DEST_PATH"

echo "[INFO] Đang tải lại udev rules..."
udevadm control --reload-rules
udevadm trigger

echo "[OK] Thiết lập quyền truy cập USB thành công!"
echo ">> LƯU Ý: Nếu bạn đang cắm mạch ST-Link/J-Link, vui lòng rút ra và cắm lại để hệ thống nhận diện."
```

### Tại sao nên dùng xPack?
* **Standalone:** Nó chứa gần như đầy đủ các thư viện cần thiết, giảm thiểu lỗi thiếu `libusb` trên các hệ điều hành khác nhau.
* **Consistent:** Cả team của bạn sẽ dùng chung một phiên bản OpenOCD chính xác đến từng bit, loại bỏ lỗi do khác biệt phiên bản phần mềm.

### Trên Windows, nếu OpenOCD không nhận bộ nạp, bạn có thể cần dùng công cụ Zadig để chuyển driver của bộ nạp sang WinUSB hoặc libusb-win32