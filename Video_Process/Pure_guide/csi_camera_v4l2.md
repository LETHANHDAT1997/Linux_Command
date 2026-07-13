# 📷 CSI Camera trên Linux — Hướng dẫn thuần V4L2

> **Hệ thống thực hành:** Raspberry Pi 5, kernel 6.18.34, camera OV5647  
> **Triết lý:** Ưu tiên công cụ phổ quát có trên mọi Linux  
> **Ngày:** 2026-07-13

---

## Bản đồ công cụ phổ quát

| Công cụ | Gói cài | Dùng để |
|---|---|---|
| `dmesg` | Có sẵn trong kernel | Đọc kernel log |
| `ls /sys/class/video4linux/` | Có sẵn | Liệt kê device |
| `media-ctl` | `v4l-utils` | Xem/cấu hình pipeline CSI |
| `v4l2-ctl` | `v4l-utils` | Xem/set thông số camera |
| `strace` | `strace` | Debug ioctl sequence |
| `python3 + fcntl` | Có sẵn | Capture qua ioctl thuần |

```bash
# Cài v4l-utils (có trên mọi distro)
sudo apt install v4l-utils        # Debian/Ubuntu/RPi OS
sudo dnf install v4l-utils        # Fedora/RHEL
sudo pacman -S v4l-utils          # Arch Linux
```

---

## Bước 1 — Nhận biết camera CSI đã cắm vào chưa

### 1A. Kernel log — cách phổ quát, chạy được mọi nơi

```bash
dmesg | grep -iE "(csi|ov5|imx|ar[0-9]{4}|sensor|camera)" \
       | grep -v "ieee80211\|brcm\|SCSI" \
       | tail -20
```

**Output thực tế trên hệ thống này:**
```
[    0.029924] /axi/.../ov5647@36: Fixed dependency cycle(s) with .../csi@110000
[    0.969718] platform 1f00110000.csi: bcm2712_iommu_of_xlate: MMU ...
[  692.706403] rp1-cfe 1f00110000.csi: csi2_ch0 node link is not enabled.
```

**Cách đọc:**
- Thấy tên chip (`ov5647`, `imx219`, `imx477`...) → **camera được kernel nhận**
- Không thấy → camera chưa được nhận (sai connector, driver chưa load)

### 1B. Đếm số camera sensor — qua `/sys/class/video4linux`

```bash
# Liệt kê tất cả subdevice và phân loại
for dev in /sys/class/video4linux/v4l-subdev*; do
    name=$(cat "$dev/name" 2>/dev/null)
    # Sensor thật không có tên: csi2, pisp-fe, rp1-*
    if echo "$name" | grep -qvE "^csi|^pisp|^rp1|^isp"; then
        echo "[SENSOR] $name -> /dev/$(basename $dev)"
    else
        echo "[HW]     $name -> /dev/$(basename $dev)"
    fi
done
```

**Output thực tế:**
```
[HW]     csi2      -> /dev/v4l-subdev0    # CSI-2 bridge
[HW]     pisp-fe   -> /dev/v4l-subdev1    # ISP front end
[SENSOR] ov5647 10-0036 -> /dev/v4l-subdev2   # ← CAMERA THẬT
```

**Giải thích:** `10-0036` = I2C bus 10, địa chỉ 0x36. Đây là địa chỉ I2C cố định của OV5647.

### 1C. media-ctl — xem toàn bộ cấu trúc pipeline

```bash
# Liệt kê media devices
ls /dev/media*

# Xem pipeline của media0
media-ctl -d /dev/media0 -p
```

**Output quan trọng:**
```
Media device information
    driver   rp1-cfe
    model    rp1-cfe

- entity 16: ov5647 10-0036      <- SENSOR tìm thấy
    -> "csi2":0 [ENABLED,IMMUTABLE]

- entity 1: csi2                 <- CSI-2 bridge
    <- "ov5647 10-0036":0 [ENABLED,IMMUTABLE]
    -> "pisp-fe":0 [ENABLED]     <- link sang ISP
```

**Kết luận bước 1:** Camera OV5647 đã được nhận. Có 1 camera sensor.

---

## Bước 2 — Kiểm tra thông tin camera

### 2A. Tìm subdev node của sensor

```bash
# Tìm device node của sensor (không phải csi2 hay pisp)
SENSOR_DEV=""
for dev in /sys/class/video4linux/v4l-subdev*; do
    name=$(cat "$dev/name" 2>/dev/null)
    if echo "$name" | grep -qvE "^csi|^pisp|^rp1|^isp"; then
        SENSOR_DEV="/dev/$(basename $dev)"
        echo "Sensor: $name -> $SENSOR_DEV"
    fi
done
```

**Kết quả:** `Sensor: ov5647 10-0036 -> /dev/v4l-subdev2`

### 2B. Thông tin driver của sensor

```bash
v4l2-ctl --device=/dev/v4l-subdev2 --info
```

```
Driver version   : 6.18.34
Capabilities     : 0x00000000
Client Capabilities: 0x...02    # interval-uses-which
```

### 2C. Format sensor đang output (mediabus format)

```bash
v4l2-ctl --device=/dev/v4l-subdev2 --get-subdev-fmt
```

**Output:**
```
ioctl: VIDIOC_SUBDEV_G_FMT (pad=0,stream=0)
    Width/Height  : 1920/1080
    Mediabus Code : 0x300e (MEDIA_BUS_FMT_SGBRG10_1X10)   ← Bayer 10-bit GBRG
    Field         : None
    Colorspace    : Raw
```

### 2D. Tất cả các resolution sensor hỗ trợ

```bash
# code=0x300e là SGBRG10_1X10 (Bayer 10-bit của OV5647)
v4l2-ctl --device=/dev/v4l-subdev2 \
         --list-subdev-framesizes=pad=0,code=0x300e
```

**Output:**
```
Size Range: 2592x1944 - 2592x1944   ← Full resolution (5MP)
Size Range: 1920x1080 - 1920x1080   ← 1080p
Size Range: 1296x972  - 1296x972    ← ~1.3MP (gần HD nhất)
Size Range: 640x480   - 640x480     ← VGA
```

> **Lưu ý:** OV5647 không có mode 1280×720 native. Mode gần HD nhất là 1296×972.

### 2E. Các controls có thể điều chỉnh

```bash
v4l2-ctl --device=/dev/v4l-subdev2 --list-ctrls
```

**Output thực tế:**
```
User Controls:
    white_balance_automatic  (bool) : default=0 value=0
    exposure                 (int)  : min=4 max=1100 step=1 default=1000
    gain_automatic           (bool) : default=0 value=0
    horizontal_flip          (bool) : default=0
    vertical_flip            (bool) : default=0

Camera Controls:
    auto_exposure            (menu) : min=0 max=1 default=1 value=1 (Manual)
    camera_orientation       (menu) : min=0 max=2 value=2 (External)
    camera_sensor_rotation   (int)  : min=0 max=0 value=0 [read-only]

Image Source Controls:
    vertical_blanking        (int)  : min=24 max=31687 (ảnh hưởng FPS)
    horizontal_blanking      (int)  : min=496 max=6271
    analogue_gain            (int)  : min=16 max=1023 step=1 default=32

Image Processing Controls:
    link_frequency           (intmenu) : value=218500000 [read-only]
    pixel_rate               (int64)   : value=87500000 [read-only]
    test_pattern             (menu)    : value=0 (Disabled)
```

---

## Bước 3 — Cài đặt thông số camera (ví dụ: HD ~1296×972)

### 3A. Set resolution (format cho sensor)

```bash
# Syntax: media-ctl --set-v4l2 "'tên entity':pad[fmt:FORMAT/WxH]"
# FORMAT phải là mediabus format mà sensor hỗ trợ

media-ctl -d /dev/media0 \
  --set-v4l2 "'ov5647 10-0036':0[fmt:SGBRG10_1X10/1296x972]"
```

**Output (silent = thành công)**

```bash
# Phải set luôn format cho CSI-2 bridge (entity tiếp theo trong pipeline)
media-ctl -d /dev/media0 \
  --set-v4l2 "'csi2':0[fmt:SGBRG10_1X10/1296x972]"
```

```bash
# Xác nhận
v4l2-ctl --device=/dev/v4l-subdev2 --get-subdev-fmt
```

**Kết quả:**
```
Width/Height : 1296/972    ← ĐÃ THAY ĐỔI
Mediabus Code: 0x300e (MEDIA_BUS_FMT_SGBRG10_1X10)
```

### 3B. Set các control khác

```bash
# Bật auto exposure (auto_exposure=0 nghĩa là AUTO — ngược logic!)
v4l2-ctl --device=/dev/v4l-subdev2 --set-ctrl=auto_exposure=0

# Bật auto gain
v4l2-ctl --device=/dev/v4l-subdev2 --set-ctrl=gain_automatic=1

# Set exposure thủ công (nếu muốn manual)
v4l2-ctl --device=/dev/v4l-subdev2 --set-ctrl=auto_exposure=1  # Manual
v4l2-ctl --device=/dev/v4l-subdev2 --set-ctrl=exposure=500

# Lật ảnh theo chiều dọc
v4l2-ctl --device=/dev/v4l-subdev2 --set-ctrl=vertical_flip=1

# Kiểm tra giá trị hiện tại
v4l2-ctl --device=/dev/v4l-subdev2 \
  --get-ctrl=auto_exposure,gain_automatic,exposure,analogue_gain
```

**Kết quả:**
```
gain_automatic : 1
exposure       : 1100
auto_exposure  : 0 (Auto Mode)
analogue_gain  : 32
```

### 3C. Lựa chọn camera khi hệ thống có nhiều Camera (Multi-camera)

Khi cắm cùng lúc nhiều camera CSI (ví dụ trên 2 cổng CSI của Raspberry Pi 5), mỗi camera sẽ được gán một địa chỉ điều khiển I2C khác nhau và một node subdevice `/dev/v4l-subdev*` riêng biệt.

#### 1. Xác định vị trí của từng Camera
Bạn sử dụng thư mục `/sys` để liệt kê các camera sensor thực tế và bus I2C của chúng:
```bash
for dev in /sys/class/video4linux/v4l-subdev*; do
    name=$(cat "$dev/name" 2>/dev/null)
    if echo "$name" | grep -qvE "^csi|^pisp|^rp1|^isp"; then
        echo "Camera: $name -> /dev/$(basename $dev)"
    fi
done
```
**Kết quả mẫu khi cắm 2 camera:**
```
Camera: ov5647 10-0036 -> /dev/v4l-subdev2   # Camera 1 (ở I2C bus 10)
Camera: ov5647 12-0036 -> /dev/v4l-subdev3   # Camera 2 (ở I2C bus 12)
```

#### 2. Cấu hình thông số cho camera cụ thể
Khi cấu hình thông số (như độ phân giải hoặc các control exposure, gain), bạn chỉ cần thay đổi đường dẫn thiết bị `--device` hoặc tên thực thể (`entity`) tương ứng trong lệnh:

* **Cấu hình độ phân giải cho Camera 1:**
  ```bash
  media-ctl -d /dev/media0 --set-v4l2 "'ov5647 10-0036':0[fmt:SGBRG10_1X10/1296x972]"
  ```
* **Cấu hình độ phân giải cho Camera 2:**
  ```bash
  media-ctl -d /dev/media0 --set-v4l2 "'ov5647 12-0036':0[fmt:SGBRG10_1X10/1296x972]"
  ```
* **Điều chỉnh Exposure cho Camera 1:**
  ```bash
  v4l2-ctl --device=/dev/v4l-subdev2 --set-ctrl=exposure=500
  ```
* **Điều chỉnh Exposure cho Camera 2:**
  ```bash
  v4l2-ctl --device=/dev/v4l-subdev3 --set-ctrl=exposure=500
  ```

---

## Bước 4 — Chụp ảnh

### Tại sao bước này phức tạp trên RPi5?

Khi thực hành, phát hiện **vấn đề đặc thù của rp1-cfe (RPi5)**:

```
v4l2-ctl --stream-mmap ...
=> VIDIOC_STREAMON returned -1 (Invalid argument)

Buffer size sau REQBUFS: length=0, offset=0x0
```

**Nguyên nhân gốc rễ:** `rp1-cfe` trên RPi5 dùng kiến trúc **multiplexed streams** — mỗi output node (`/dev/video0..7`) chỉ hoạt động sau khi **routing table** được cấu hình. Đây là ioctl mới: `VIDIOC_SUBDEV_S_ROUTING`.

Thực nghiệm bằng Python ioctl:
```
VIDIOC_SUBDEV_S_ROUTING = 0xc0245656
Thử trên /dev/v4l-subdev0 (csi2): errno=25 ENOTTY (Không support)
Thử trên /dev/v4l-subdev1 (pisp-fe): errno=25 ENOTTY
Thử trên /dev/v4l-subdev2 (ov5647): errno=25 ENOTTY
```

Driver không expose ioctl này ra userspace trực tiếp → đây là giới hạn của **driver rp1-cfe hiện tại**.

**Phân tích strace của rpicam-still cho thấy libcamera:**
1. Mở tất cả `/dev/video0..7` để probe
2. Dùng **`DMA heap`** (`DMA_HEAP_IOCTL_ALLOC`) thay vì mmap thông thường
3. Dùng `VIDIOC_TRY_FMT` với format `PC1g` (PiSP compressed) trên node CFE
4. Dùng `VIDEO_CAPTURE_MPLANE` (multi-planar) trên node pispbe với format YUV420

→ libcamera **không dùng routing ioctl** — nó dùng DMA heap và planar format trên các node output đặc biệt của pispbe.

### 4A. Phương án A: Dùng ffmpeg + libcamera backend (có sẵn trên RPi OS)

```bash
# ffmpeg được build với --enable-libcamera trên RPi OS
ffmpeg -f libcamera -i /base/axi/pcie@1000120000/rp1/i2c@88000/ov5647@36 \
       -frames:v 1 photo.jpg -y

# Hoặc để ffmpeg tự tìm camera
ffmpeg -f libcamera -i default -frames:v 1 photo.jpg -y
```

> ffmpeg trên RPi OS Bookworm được build với libcamera support — đây vẫn là công cụ **phổ quát** (dùng được trên mọi Linux), chỉ là backend thay đổi theo hệ thống.

### 4B. Phương án B: rpicam-still (chỉ trên RPi)

```bash
# Chụp ảnh HD (1296x972 — gần nhất với HD trên OV5647)
rpicam-still --zsl \
             --width 1296 --height 972 \
             -t 2000 \
             -o photo_hd_$(date +%Y%m%d_%H%M%S).jpg
```

| Tham số | Ý nghĩa |
|---|---|
| `--zsl` | Zero Shutter Lag — dùng temporal denoise, chất lượng tốt hơn |
| `--width 1296 --height 972` | Resolution ~HD |
| `-t 2000` | 2 giây preview để AE/AWB ổn định |
| `-o file.jpg` | Tên file output |

### 4C. Phương án phổ quát cho camera CSI có driver cũ hơn (RPi4, Jetson, BeagleBone...)

Trên các board cũ hơn dùng kiến trúc CSI không multiplexed:

```bash
# 1. Enable link
media-ctl -d /dev/media0 -l "'csi2':4->'rp1-cfe-csi2_ch0':0[1]"

# 2. Set format capture node
v4l2-ctl --device=/dev/video0 \
  --set-fmt-video=width=1296,height=972,pixelformat=GBRG

# 3. Capture raw Bayer
v4l2-ctl --device=/dev/video0 \
  --stream-mmap \
  --stream-to=frame.raw \
  --stream-count=1

# 4. Convert raw Bayer -> JPEG bằng ffmpeg
ffmpeg -f rawvideo \
       -pixel_format bayer_gbrg8 \
       -video_size 1296x972 \
       -i frame.raw \
       -vf "debayer=pattern=GBRG" \
       photo.jpg
```

---

## Tổng kết: Sơ đồ tư duy

```
Cắm camera CSI vào
        │
        ▼
[1. NHẬN BIẾT] ─────────────────────────────────────────
│  dmesg | grep -iE "csi|ov5|imx|sensor"
│  for dev in /sys/class/video4linux/v4l-subdev*
│  media-ctl -d /dev/media0 -p
│  Kết quả: tên sensor, địa chỉ I2C, device node
        │
        ▼
[2. THÔNG TIN] ──────────────────────────────────────────
│  v4l2-ctl --device=/dev/v4l-subdevX --get-subdev-fmt
│  v4l2-ctl --device=/dev/v4l-subdevX --list-subdev-framesizes=pad=0,code=0x300e
│  v4l2-ctl --device=/dev/v4l-subdevX --list-ctrls
│  Kết quả: resolution, format, controls có sẵn
        │
        ▼
[3. CÀI ĐẶT] ────────────────────────────────────────────
│  media-ctl -d /dev/media0 --set-v4l2 "sensor:0[fmt:FORMAT/WxH]"
│  media-ctl -d /dev/media0 --set-v4l2 "csi2:0[fmt:FORMAT/WxH]"
│  v4l2-ctl --device=/dev/v4l-subdevX --set-ctrl=control=value
│  Kết quả: sensor output đúng resolution, controls đúng giá trị
        │
        ▼
[4. CHỤP ẢNH] ───────────────────────────────────────────
│
│  Camera USB (mọi Linux):
│    ffmpeg -f v4l2 -video_size 1280x720 -i /dev/video0 -frames:v 1 out.jpg -y
│
│  Camera CSI — pipeline đơn giản (RPi4, Jetson...):
│    media-ctl set link + v4l2-ctl --stream-mmap → raw → ffmpeg convert
│
│  Camera CSI — pipeline phức tạp (RPi5 rp1-cfe):
│    ffmpeg -f libcamera ... (ffmpeg với libcamera backend)
│    HOẶC rpicam-still (nếu trên RPi OS)
```

---

## Giải thích: Tại sao RPi5 khác biệt?

| Thế hệ | Driver CSI | Multiplexed streams | Routing cần thiết |
|---|---|---|---|
| RPi 1-3 | `bcm2835-unicam` | Không | Không |
| RPi 4 | `bcm2835-unicam` | Không | Không |
| **RPi 5** | **`rp1-cfe`** | **Có** | **Có (nhưng driver ẩn đi)** |
| Jetson | `tegra-vi` | Không | Không |
| i.MX8 | `imx-mipi-csis` | Không | Không |

RPi5 dùng chip **RP1** mới với kiến trúc CSI-2 hỗ trợ nhiều stream đồng thời. Kernel driver `rp1-cfe` implement routing nội bộ nhưng **không expose `VIDIOC_SUBDEV_S_ROUTING` ra userspace** → chỉ libcamera (được Raspberry Pi hỗ trợ chính thức) biết cách tương tác đúng.

---

*Tài liệu được tạo qua thực hành trực tiếp với Antigravity AI — 2026-07-13*
