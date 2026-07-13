# 📷 Hướng dẫn: Kiểm tra Camera trên Linux (Raspberry Pi)

> **Hệ thống:** Raspberry Pi 5 — Linux kernel 6.18.34  
> **Camera:** OV5647 (Raspberry Pi Camera Module v1, 5MP)  
> **Thời gian thực hiện:** 2026-07-13

---

## Mục lục

1. [Tổng quan quy trình](#tổng-quan-quy-trình)
2. [Bước 1 — Kiểm tra device file trong /dev](#bước-1--kiểm-tra-device-file-trong-dev)
3. [Bước 2 — Dùng v4l2-ctl để liệt kê các camera](#bước-2--dùng-v4l2-ctl-để-liệt-kê-các-camera)
4. [Bước 3 — Xem thông tin chi tiết của device](#bước-3--xem-thông-tin-chi-tiết-của-device)
5. [Bước 4 — Kiểm tra kernel log dmesg](#bước-4--kiểm-tra-kernel-log-dmesg)
6. [Bước 5 — Dùng rpicam-hello để xác nhận camera sensor](#bước-5--dùng-rpicam-hello-để-xác-nhận-camera-sensor)
7. [Bước 6 — Chụp ảnh bằng rpicam-still](#bước-6--chụp-ảnh-bằng-rpicam-still)
8. [Các lỗi gặp phải và cách xử lý](#các-lỗi-gặp-phải-và-cách-xử-lý)
9. [Kiến thức nền — Camera Stack trên Raspberry Pi](#kiến-thức-nền--camera-stack-trên-raspberry-pi)
10. [Tóm tắt lệnh hay dùng](#tóm-tắt-lệnh-hay-dùng)

---

## Tổng quan quy trình

Khi cắm camera vào hệ thống Linux, để kiểm tra và sử dụng, ta đi theo luồng sau:

```
Cắm camera
    │
    ▼
Kiểm tra device file (/dev/video*)
    │
    ▼
Liệt kê chi tiết bằng v4l2-ctl
    │
    ▼
Kiểm tra kernel log (dmesg) để xem driver đã load chưa
    │
    ▼
Dùng rpicam-hello / libcamera để xác nhận sensor
    │
    ▼
Chụp ảnh bằng rpicam-still
```

---

## Bước 1 — Kiểm tra device file trong `/dev`

### Lệnh

```bash
ls /dev/video* 2>/dev/null && echo "--- Camera devices found ---" || echo "No video devices found"
```

### Giải thích từng phần

| Phần lệnh | Ý nghĩa |
|---|---|
| `ls /dev/video*` | Liệt kê tất cả file thiết bị video (camera, webcam...) |
| `2>/dev/null` | Nếu không có file nào, ẩn thông báo lỗi `No such file or directory` |
| `&&` | Nếu lệnh trước thành công thì chạy lệnh tiếp theo |
| `\|\|` | Nếu lệnh trước thất bại thì chạy lệnh này |

### Kết quả nhận được

```
/dev/video0   /dev/video21  /dev/video26  /dev/video30  /dev/video35
/dev/video1   /dev/video22  /dev/video27  /dev/video31  /dev/video4
/dev/video19  /dev/video23  /dev/video28  /dev/video32  /dev/video5
/dev/video2   /dev/video24  /dev/video29  /dev/video33  /dev/video6
/dev/video20  /dev/video25  /dev/video3   /dev/video34  /dev/video7
--- Camera devices found ---
```

### Phân tích

- Có rất nhiều `/dev/video*` — đây là điều bình thường trên Raspberry Pi 5.
- Không phải tất cả đều là camera vật lý! Nhiều node là các **virtual subdevice** của pipeline xử lý ảnh (ISP).
- Cần dùng công cụ khác để phân biệt.

> **Lưu ý:** Trên máy tính thường (laptop/PC) với webcam USB, thường chỉ có `/dev/video0` và `/dev/video1` (một capture, một metadata). Trên Raspberry Pi số lượng nhiều hơn vì có ISP phần cứng.

---

## Bước 2 — Dùng `v4l2-ctl` để liệt kê các camera

### Lệnh

```bash
v4l2-ctl --list-devices
```

### Công cụ `v4l2-ctl` là gì?

`v4l2-ctl` là công cụ dòng lệnh để tương tác với **V4L2 (Video4Linux2)** — kernel subsystem quản lý tất cả thiết bị video trên Linux. Nó cho phép:
- Liệt kê camera
- Xem/đặt thông số (độ sáng, tương phản...)
- Capture frame thô

### Kết quả nhận được

```
pispbe (platform:1000880000.pisp_be):
        /dev/video20 đến /dev/video35
        /dev/media2, /dev/media3

rp1-cfe (platform:1f00110000.csi):
        /dev/video0 đến /dev/video7
        /dev/media0

rpi-hevc-dec (platform:rpi-hevc-dec):
        /dev/video19
        /dev/media1
```

### Phân tích

| Driver | Ý nghĩa |
|---|---|
| `rp1-cfe` | **Camera Front End** — nhận dữ liệu RAW từ sensor qua CSI-2 |
| `pispbe` | **PiSP Back End** — chip ISP xử lý ảnh (debayer, denoise...) |
| `rpi-hevc-dec` | Bộ giải mã video HEVC (H.265) — không liên quan đến camera |

→ Camera vật lý thực sự nằm phía sau `rp1-cfe` (CSI interface).

---

## Bước 3 — Xem thông tin chi tiết của device

### Lệnh

```bash
v4l2-ctl --device=/dev/video0 --all
```

### Kết quả (trích)

```
Driver Info:
    Driver name      : rp1-cfe
    Card type        : rp1-cfe
    Bus info         : platform:1f00110000.csi
    Driver version   : 6.18.34
    Capabilities     : 0xaca00001
            Video Capture
            Metadata Capture
            I/O MC
            Streaming
            Extended Pix Format
```

### Ý nghĩa các Capabilities

| Capability | Ý nghĩa |
|---|---|
| `Video Capture` | Có thể capture video/ảnh |
| `I/O MC` | Dùng Media Controller để cấu hình pipeline |
| `Streaming` | Hỗ trợ streaming liên tục |
| `Extended Pix Format` | Hỗ trợ nhiều format pixel nâng cao |

---

## Bước 4 — Kiểm tra kernel log dmesg

### Lệnh

```bash
dmesg | grep -i "camera\|imx\|ov\|ar\|sensor\|csi" | tail -20
```

### Giải thích

| Phần lệnh | Ý nghĩa |
|---|---|
| `dmesg` | Hiển thị kernel ring buffer (log của kernel từ khi boot) |
| `grep -i` | Tìm kiếm không phân biệt hoa/thường |
| `"camera\|imx\|ov\|ar\|sensor\|csi"` | Tìm từ khóa liên quan đến camera sensor phổ biến (`imx` = Sony, `ov` = OmniVision, `ar` = onsemi) |
| `tail -20` | Chỉ lấy 20 dòng cuối (mới nhất) |

### Kết quả quan trọng

```
[    3.156524] rp1-cfe 1f00110000.csi: Registered [rp1-cfe-fe_image1] node ...
[  692.706403] rp1-cfe 1f00110000.csi: csi2_ch0 node link is not enabled.
```

### Phân tích — phát hiện cảnh báo

Dòng `csi2_ch0 node link is not enabled` xuất hiện.
Đây là dấu hiệu khi camera chưa được dùng lần nào (link chưa được enable). **Không phải lỗi**, chỉ là trạng thái khởi tạo.

> **Mẹo debug:** Nếu camera không hoạt động, chạy `dmesg | grep -i "error\|fail\|camera"` để tìm lỗi cụ thể hơn.

---

## Bước 5 — Dùng `rpicam-hello` để xác nhận camera sensor

### Lần thử đầu — THẤT BẠI ❌

```bash
libcamera-hello --list-cameras
```

**Lỗi gặp phải:**
```
bash: line 1: libcamera-hello: command not found
```

**Nguyên nhân:**  
Trên Raspberry Pi OS mới (Bookworm trở lên), Raspberry Pi đã **đổi tên** bộ công cụ:
- ❌ Cũ: `libcamera-hello`, `libcamera-still`, `libcamera-vid`
- ✅ Mới: `rpicam-hello`, `rpicam-still`, `rpicam-vid`

**Cách xử lý:** Thử lại với tên mới.

### Lần thử sau — THÀNH CÔNG ✅

```bash
rpicam-hello --list-cameras
```

**Kết quả:**
```
Available cameras
-----------------
0 : ov5647 [2592x1944 10-bit GBRG] (/base/axi/pcie@1000120000/rp1/i2c@88000/ov5647@36)
    Modes: 'SGBRG10_CSI2P' : 640x480 [62.50 fps - (16, 0)/2560x1920 crop]
                             1296x972 [46.34 fps - (0, 0)/2592x1944 crop]
                             1920x1080 [32.81 fps - (348, 434)/1928x1080 crop]
                             2592x1944 [15.63 fps - (0, 0)/2592x1944 crop]
```

### Phân tích kết quả

| Thông tin | Giá trị | Ý nghĩa |
|---|---|---|
| `ov5647` | Tên sensor | OmniVision OV5647 — chip camera 5MP |
| `2592x1944` | Độ phân giải tối đa | 5 Megapixel |
| `10-bit GBRG` | Bit depth & Bayer pattern | Raw 10-bit, màu sắp xếp theo GBRG |
| `SGBRG10_CSI2P` | Format truyền | Serial GBRG 10-bit qua CSI-2 Packed |
| `i2c@88000/ov5647@36` | Địa chỉ I2C | Sensor giao tiếp qua I2C bus, địa chỉ 0x36 |

---

## Bước 6 — Chụp ảnh bằng `rpicam-still`

### Lệnh

```bash
rpicam-still -o /home/ledat/Documents/Linux_Command/photo_$(date +%Y%m%d_%H%M%S).jpg \
             --width 1920 \
             --height 1080 \
             -t 3000
```

### Giải thích các tham số

| Tham số | Ý nghĩa |
|---|---|
| `-o <file>` | Output file — tên file ảnh đầu ra |
| `$(date +%Y%m%d_%H%M%S)` | Tạo timestamp động, ví dụ: `20260713_114425` |
| `--width 1920` | Chiều rộng ảnh (pixels) |
| `--height 1080` | Chiều cao ảnh (pixels) |
| `-t 3000` | Thời gian preview trước khi chụp (ms) — 3 giây để auto-exposure ổn định |

### Cảnh báo xuất hiện trong output (WARNING — không phải lỗi)

```
WARNING: Capture will not make use of temporal denoise
         Consider using the --zsl option for best results
```

**Giải thích:**  
- `temporal denoise` = khử nhiễu theo thời gian (dùng nhiều frame liên tiếp để giảm noise)
- `--zsl` = Zero Shutter Lag mode — chụp ảnh từ preview stream, cho phép dùng temporal denoise

**Cách xử lý:** Đây chỉ là gợi ý, không ảnh hưởng đến việc chụp. Ảnh vẫn được chụp bình thường. Lần sau dùng:
```bash
rpicam-still --zsl -o ~/anh.jpg
```

### Output xác nhận thành công

```
Still capture image received
```

### Kết quả file

```
-rw-rw-r-- 1 ledat ledat 318K Jul 13 11:44 photo_20260713_114425.jpg
```

Ảnh 1920×1080 nặng 318KB — bình thường với ảnh JPEG chụp trong nhà.

---

## Các lỗi gặp phải và cách xử lý

### Bảng tổng hợp

| STT | Lỗi | Nguyên nhân | Cách xử lý |
|---|---|---|---|
| 1 | `libcamera-hello: command not found` | RPi OS Bookworm đổi tên tool | Dùng `rpicam-hello` thay thế |
| 2 | `csi2_ch0 node link is not enabled` (dmesg) | Link pipeline chưa được kích hoạt lần đầu | Không cần xử lý — libcamera tự kích hoạt khi chụp |
| 3 | WARNING về temporal denoise | Không dùng ZSL mode | Thêm `--zsl` vào lệnh chụp |
| 4 | `Unsupported V4L2 pixel format Nc30, Nc12` | Format nội bộ PiSP chưa được V4L2 core support | Không ảnh hưởng — format nội bộ của Raspberry Pi |

### Cách debug khi camera không hoạt động

```bash
# 1. Kiểm tra camera có được nhận không
rpicam-hello --list-cameras

# 2. Kiểm tra kernel log
dmesg | grep -i "error\|fail\|camera\|ov5647"

# 3. Kiểm tra I2C (camera giao tiếp qua I2C)
i2cdetect -y 10   # Bus số có thể khác, thử 0, 1, 10

# 4. Kiểm tra quyền truy cập
ls -la /dev/video0
groups $USER       # User có trong group 'video' không?

# 5. Thêm user vào group video nếu thiếu
sudo usermod -aG video $USER
```

---

## Kiến thức nền — Camera Stack trên Raspberry Pi

### Sơ đồ pipeline

```
+--------------------------------------------------+
|         Ứng dụng (rpicam-still)                  |
+--------------------+-----------------------------+
                     | libcamera API
+--------------------v-----------------------------+
|                  libcamera                       |
|      (quản lý pipeline, IPA, tuning)             |
+--------+---------------------------+-------------+
         | V4L2 subdev               | IPA
+--------v--------+        +---------v-----------+
|    rp1-cfe      |        |  pisp IPA module    |
|   (CSI-2 RX)   |        |  (ov5647.json)      |
+--------+--------+        +---------------------+
         | MIPI CSI-2
+--------v--------+
|    OV5647       |  <-- Camera sensor vật lý
|  (I2C 0x36)    |
+-----------------+
```

### Giải thích các thành phần

| Thành phần | Vai trò |
|---|---|
| **OV5647** | Sensor vật lý — thu nhận ánh sáng, xuất dữ liệu RAW Bayer |
| **CSI-2** | Giao thức truyền dữ liệu tốc độ cao (Camera Serial Interface 2) |
| **rp1-cfe** | Driver kernel nhận dữ liệu từ CSI-2, expose ra V4L2 |
| **pispbe** | ISP phần cứng — chuyển RAW sang JPEG/YUV (debayer, denoise, AWB...) |
| **libcamera** | Thư viện userspace quản lý toàn bộ pipeline |
| **IPA** | Image Processing Algorithm — thuật toán AE, AWB, AF chạy trên CPU |
| **rpicam-apps** | Bộ tool dòng lệnh: `rpicam-still`, `rpicam-vid`, `rpicam-hello` |

### Bayer Pattern là gì?

Sensor màu dùng bộ lọc Bayer (Color Filter Array) để mỗi pixel chỉ thu một màu:
```
G B G B G B
R G R G R G
G B G B G B
R G R G R G
```
**GBRG** nghĩa là hàng đầu tiên bắt đầu bằng G-B, hàng thứ hai bắt đầu bằng R-G.
ISP sẽ "debayer" (hay "demosaic") để tính màu đầy đủ RGB cho mỗi pixel.

---

## Tóm tắt lệnh hay dùng

```bash
# Kiểm tra camera có nhận không
rpicam-hello --list-cameras

# Chụp ảnh đơn giản
rpicam-still -o anh.jpg

# Chụp ảnh chất lượng cao (khuyến nghị)
rpicam-still --zsl -o anh.jpg

# Chụp ảnh độ phân giải tối đa
rpicam-still --zsl --width 2592 --height 1944 -o anh_max.jpg

# Quay video 10 giây
rpicam-vid -t 10000 -o video.h264

# Xem preview trực tiếp (Ctrl+C để dừng)
rpicam-hello -t 0

# Liệt kê tất cả camera device
v4l2-ctl --list-devices

# Xem chi tiết một device
v4l2-ctl --device=/dev/video0 --all

# Xem kernel log liên quan đến camera
dmesg | grep -i "camera\|csi\|ov5647"
```

---

*Tài liệu được tạo bởi Antigravity AI — 2026-07-13*
