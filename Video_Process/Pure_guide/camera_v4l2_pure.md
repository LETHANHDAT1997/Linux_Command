# 📷 Chụp ảnh camera KHÔNG dùng rpicam/libcamera — Thuần V4L2

> **Hệ thống:** Raspberry Pi 5 — Linux kernel 6.18.34 — Camera OV5647 (CSI-2)  
> **Mục tiêu:** Thay thế `rpicam-still` bằng các công cụ phổ quát hơn trên mọi Linux  
> **Ngày:** 2026-07-13

---

## Câu hỏi đặt ra

> *"rpicam và libcamera chỉ có trên Raspberry Pi phải không? Từ Bước 5 trở đi có cách nào khác không?"*

**Câu trả lời ngắn:**
- `rpicam-apps` (`rpicam-still`, `rpicam-vid`...): ✅ **CHỈ có trên Raspberry Pi**
- `libcamera` (thư viện): ❌ Không chỉ có trên RPi, nhưng chủ yếu dùng ở đó và cấu hình phức tạp
- `V4L2` (Video4Linux2): ✅ **Có trên MỌI Linux** — đây là tầng kernel chuẩn
- `ffmpeg`, `v4l2-ctl`: ✅ **Có trên mọi Linux** — dùng được với mọi camera qua V4L2

**Vấn đề thực tế:** Camera CSI trên Raspberry Pi 5 có pipeline phức tạp hơn camera USB thông thường. Điều này ảnh hưởng đến cách tiếp cận.

---

## Sơ đồ pipeline ĐÚNG

```
rpicam-apps (chỉ có trên RPi)
    |
    v
libcamera (quản lý pipeline tự động)
    |
    v
V4L2 + Media Controller  <-- Tầng này có trên MỌI Linux
    |         |
    v         v
rp1-cfe    pispbe (ISP)
(CSI-2)
    |
    v
OV5647 sensor (I2C)
```

**Kết luận:** Nếu bỏ `rpicam` và `libcamera`, ta cần tự làm những gì libcamera làm tự động:
- Cấu hình Media Controller pipeline thủ công
- Set format cho từng subdevice
- Kích hoạt đúng link giữa các node

---

## Các công cụ thay thế (phổ quát)

| Công cụ | Hệ thống | Trường hợp dùng |
|---|---|---|
| `v4l2-ctl` | Mọi Linux | Capture raw, kiểm tra thiết bị |
| `ffmpeg -f v4l2` | Mọi Linux | Capture JPEG/video từ camera |
| `fswebcam` | Mọi Linux | Chụp ảnh đơn giản (cài thêm) |
| `media-ctl` | Mọi Linux (có Media Controller) | Cấu hình pipeline CSI camera |
| Python + `v4l2py` | Mọi Linux | Capture bằng Python thuần |
| Python + OpenCV | Mọi Linux | Capture + xử lý ảnh |

---

## Thực hành: Kiểm tra từng công cụ

### Kiểm tra công cụ có sẵn

```bash
which ffmpeg v4l2-ctl media-ctl fswebcam python3 2>/dev/null
```

**Kết quả thực tế trên hệ thống này:**
```
/usr/bin/ffmpeg      ✅ có sẵn
/usr/bin/media-ctl   ✅ có sẵn
/usr/bin/python3     ✅ có sẵn
/usr/bin/v4l2-ctl    ✅ có sẵn
fswebcam             ❌ chưa cài
gst-launch-1.0       ❌ chưa cài (GStreamer)
```

---

## Phần 1: Camera USB (Webcam thông thường)

Đây là trường hợp ĐƠN GIẢN NHẤT. Camera USB tự expose V4L2 device và không cần cấu hình pipeline.

### Chụp ảnh bằng ffmpeg (phổ biến nhất)

```bash
# Xem camera có ở /dev/video0 không
ls /dev/video*

# Xem format camera hỗ trợ
v4l2-ctl --device=/dev/video0 --list-formats-ext

# Chụp 1 frame JPEG
ffmpeg -f v4l2 -video_size 1280x720 -i /dev/video0 -frames:v 1 photo.jpg -y
```

### Chụp ảnh bằng v4l2-ctl (raw, cần convert)

```bash
# Capture raw YUYV
v4l2-ctl --device=/dev/video0 \
  --set-fmt-video=width=1280,height=720,pixelformat=YUYV \
  --stream-mmap \
  --stream-to=photo.raw \
  --stream-count=1

# Convert sang JPEG bằng ffmpeg
ffmpeg -f rawvideo -pixel_format yuyv422 -video_size 1280x720 \
       -i photo.raw photo.jpg
```

### Chụp ảnh bằng fswebcam (cần cài)

```bash
sudo apt install fswebcam
fswebcam -r 1280x720 --no-banner photo.jpg
```

### Chụp ảnh bằng Python + OpenCV (cần cài opencv)

```bash
sudo apt install python3-opencv
```

```python
import cv2

cap = cv2.VideoCapture(0)          # 0 = /dev/video0

cap.set(cv2.CAP_PROP_FRAME_WIDTH,  1280)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 720)

ret, frame = cap.read()
if ret:
    cv2.imwrite('photo.jpg', frame)
    print("Chụp thành công!")
else:
    print("Không đọc được frame!")

cap.release()
```

---

## Phần 2: Camera CSI trên Raspberry Pi (phức tạp hơn)

### Tại sao CSI phức tạp hơn USB?

Camera USB tự hoàn chỉnh: sensor + ISP nằm trong cùng một thiết bị, driver USB expose ra một `/dev/video0` đơn giản.

Camera CSI trên RPi: sensor (OV5647), CSI receiver (rp1-cfe), ISP (pispbe) là 3 phần cứng **riêng biệt** được kết nối qua **Media Controller pipeline**. Mỗi phần phải được cấu hình thủ công.

```
[OV5647 sensor] --I2C control--> kernel driver (ov5647)
      |
      | MIPI CSI-2 data lanes (tốc độ cao)
      v
[rp1-cfe] = CSI-2 receiver trên RP1 chip
      |
      | (nội bộ bus)
      v
[pispbe] = ISP phần cứng
      |
      v
/dev/video20..35 (output frames đã xử lý)
```

### Bước 1: Xem cấu trúc pipeline

```bash
media-ctl -d /dev/media0 -p
```

**Output thực tế:**
```
- entity 1: csi2 (8 pads, 8 links)
    <- "ov5647 10-0036":0 [ENABLED,IMMUTABLE]
    -> "rp1-cfe-csi2_ch0":0 []          # disabled (bypass ISP)
    -> "pisp-fe":0 [ENABLED]             # enabled (qua ISP)

- entity 16: ov5647 10-0036 (/dev/v4l-subdev2)
    -> "csi2":0 [ENABLED,IMMUTABLE]

- entity 18: rp1-cfe-csi2_ch0 (/dev/video0)  # Raw Bayer output
    <- "csi2":4 []

- entity 34: rp1-cfe-fe_image0 (/dev/video4)  # Processed output (qua ISP)
    <- "pisp-fe":2 [ENABLED]
```

**Hiểu ngay:** Mặc định, data chạy qua ISP (`pisp-fe`) và ra `/dev/video4`. Muốn bypass ISP lấy raw thì phải đổi link sang `/dev/video0`.

### Bước 2: Cấu hình pipeline thủ công (media-ctl)

#### Cách A: Capture RAW Bayer (bypass ISP, không ISP)

```bash
# 1. Disable link qua ISP
media-ctl -d /dev/media0 -l "'csi2':4->'pisp-fe':0[0]"

# 2. Enable link thẳng ra csi2_ch0
media-ctl -d /dev/media0 -l "'csi2':4->'rp1-cfe-csi2_ch0':0[1]"

# 3. Set format cho sensor
media-ctl -d /dev/media0 \
  --set-v4l2 "'ov5647 10-0036':0[fmt:SGBRG10_1X10/1920x1080]"

# 4. Set format cho csi2
media-ctl -d /dev/media0 \
  --set-v4l2 "'csi2':0[fmt:SGBRG10_1X10/1920x1080]"

# 5. Capture bằng v4l2-ctl
v4l2-ctl --device=/dev/video0 \
  --stream-mmap --stream-to=capture.raw --stream-count=1
```

#### Cách B: Capture qua ISP (đã processed)

```bash
# Link pisp-fe -> fe_image0 đã ENABLED mặc định
# Chỉ cần capture từ /dev/video4
v4l2-ctl --device=/dev/video4 \
  --stream-mmap --stream-to=capture.raw --stream-count=1
```

### Bước 3: Lỗi gặp phải — ghi chép thực tế

---

## ❌ LỖI 1: `VIDIOC_STREAMON: Invalid argument` trên /dev/video0

### Triệu chứng

```
VIDIOC_STREAMON returned -1 (Invalid argument)
```

File output = 0 bytes.

### Nguyên nhân

Raspberry Pi 5 dùng kernel mới hỗ trợ **multiplexed streams** (nhiều stream trên 1 link vật lý). Điều này yêu cầu cấu hình **routing table** cho subdevice `csi2` trước khi stream.

`v4l2-ctl` phiên bản hiện tại (`1.30.1`) không có option `--set-subdev-routing`:
```
v4l2-ctl: unrecognized option '--set-subdev-routing=0/0->4/0:1'
```

### Thực chất

Đây là giới hạn của **RPi5 specific hardware** (`rp1-cfe` với multiplexed stream). Camera CSI trên RPi4 và trước đó không có vấn đề này — các node hoạt động trực tiếp hơn.

### Cách xử lý

**Option 1:** Dùng `libcamera` / `rpicam` (nó tự handle routing). ← Cách đơn giản nhất.

**Option 2:** Dùng Python với IOCTL thủ công để set routing:
```python
import fcntl, struct
# VIDIOC_SUBDEV_S_ROUTING = 0xC010560E (cần map đúng struct)
# Phức tạp, không phổ quát
```

**Option 3:** Dùng ffmpeg với libcamera backend (nếu ffmpeg build có):
```bash
ffmpeg -f libcamera -i /base/axi/... -frames:v 1 photo.jpg
```

---

## ❌ LỖI 2: `ioctl(VIDIOC_STREAMON): Invalid argument` trong ffmpeg

### Triệu chứng

```
[video4linux2,v4l2 @ 0x...] ioctl(VIDIOC_STREAMON): Invalid argument
[in#0 @ 0x...] Error opening input: Invalid argument
```

### Nguyên nhân

ffmpeg cũng gặp cùng vấn đề — driver không chấp nhận STREAMON vì routing chưa được cấu hình (RPi5 multiplexed stream issue).

ffmpeg debug log cho thấy nó đã cố thử nhiều format (YUYV, yuv420p...) nhưng đều bị driver báo không phù hợp với format hiện tại (`pBAA` = 10-bit Bayer GBRG Packed):
```
The V4L2 driver changed the pixel format from 0x32315559 to 0x41414270
```
Tức là driver tự reset về Bayer format, nhưng vẫn không stream được.

### Cách xử lý

Cùng nguyên nhân với Lỗi 1 — routing table chưa được set.

---

## ❌ LỖI 3: `VIDIOC_STREAMON: Broken pipe`

### Triệu chứng

```
VIDIOC_STREAMON returned -1 (Broken pipe)
```

### Nguyên nhân

Hai consumer cùng lúc: lúc đầu link `csi2 -> pisp-fe` đang ENABLED, nhưng ta lại cố enable thêm link `csi2 -> csi2_ch0`. CSI2 không thể stream ra 2 nơi cùng lúc mà không có routing table đúng.

### Cách xử lý

Disable link pisp-fe TRƯỚC, rồi mới enable link csi2_ch0:
```bash
media-ctl -d /dev/media0 -l "'csi2':4->'pisp-fe':0[0]"          # disable trước
media-ctl -d /dev/media0 -l "'csi2':4->'rp1-cfe-csi2_ch0':0[1]"  # enable sau
```

---

## Tổng kết: USB Camera vs CSI Camera

| Tiêu chí | USB Camera | CSI Camera (RPi) |
|---|---|---|
| **Cấu hình pipeline** | Tự động | Phải làm thủ công |
| **Dùng ffmpeg** | ✅ Trực tiếp | ⚠️ Cần setup trước |
| **Dùng v4l2-ctl stream** | ✅ Dễ | ⚠️ Phức tạp (routing) |
| **Dùng fswebcam** | ✅ Dễ | ❌ Không hỗ trợ |
| **Dùng OpenCV** | ✅ Dễ | ⚠️ Phức tạp |
| **Dùng rpicam-still** | ❌ Không có | ✅ Tốt nhất |
| **Dùng libcamera** | ⚠️ Cần cài | ✅ Có sẵn trên RPi |

### Kết luận thực tế

Với **camera CSI trên Raspberry Pi 5**, libcamera/rpicam là lựa chọn tốt nhất vì:
1. Pipeline RPi5 dùng multiplexed streams — phức tạp để cấu hình thủ công
2. libcamera tự handle routing, AE, AWB, format negotiation
3. Không có công cụ nào phổ quát nào làm được tương đương mà không phức tạp

Với **camera USB** trên bất kỳ Linux nào:
```bash
ffmpeg -f v4l2 -video_size 1280x720 -i /dev/video0 -frames:v 1 photo.jpg -y
```
→ Lệnh này hoạt động ngay, không cần cấu hình gì thêm.

---

## Lệnh nhanh tham khảo

```bash
# Xem pipeline đầy đủ
media-ctl -d /dev/media0 -p

# Xem format subdevice hỗ trợ
v4l2-ctl --device=/dev/video0 --list-formats-ext

# Xem format subdev đang dùng
v4l2-ctl --device=/dev/v4l-subdev2 --get-subdev-fmt

# Set link trong Media Controller
media-ctl -d /dev/media0 -l "'entity_name':pad_src->'entity_dst':pad_dst[1]"
# [1] = enable, [0] = disable

# Set format subdevice
media-ctl -d /dev/media0 --set-v4l2 "'entity_name':pad[fmt:FORMAT/WxH]"

# ffmpeg capture (USB camera)
ffmpeg -f v4l2 -video_size 1920x1080 -i /dev/video0 -frames:v 1 out.jpg -y

# v4l2-ctl capture raw (USB camera)
v4l2-ctl --device=/dev/video0 \
  --set-fmt-video=width=1280,height=720,pixelformat=YUYV \
  --stream-mmap --stream-to=out.raw --stream-count=1
```

---

*Tài liệu được tạo bởi Antigravity AI — 2026-07-13*
