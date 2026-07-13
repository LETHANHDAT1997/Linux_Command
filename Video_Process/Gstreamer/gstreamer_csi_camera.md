# 🎬 GStreamer cho Camera CSI — Hướng dẫn thực hành

> **Hệ thống thực hành:** Raspberry Pi 5, kernel 6.18.34, camera OV5647  
> **GStreamer version:** 1.26.2  
> **Ngày:** 2026-07-13 — **Đã kiểm chứng thực tế**

---

## GStreamer là gì và tại sao dùng được trên mọi hệ thống?

GStreamer là một **multimedia pipeline framework** — nó hoạt động theo mô hình:

```
[source] → [filter/transform] → [sink]
```

Mỗi khối gọi là một **element**. Bạn nối các element lại thành pipeline bằng `!`.

**Tại sao phổ quát:**
- Available trên: Linux, Windows, macOS, Android, iOS, embedded
- Package: `gstreamer1.0-tools` (mọi distro Linux đều có)
- Không cần viết code — pipeline được mô tả bằng text trên command line

**Điều quan trọng cần hiểu:** GStreamer là *lớp trên*, nó vẫn cần *backend bên dưới*:

```
gst-launch-1.0 (pipeline text)
       │
       ▼
GStreamer Framework
       │
       ├── v4l2src plugin → V4L2 kernel API → Camera USB / CSI (board cũ)
       ├── libcamerasrc plugin → libcamera → Camera CSI (RPi5, hiện đại)
       └── pipewiresrc plugin → PipeWire → Camera qua audio/video server
```

---

## Cài đặt

```bash
# Gói bắt buộc (chứa gst-launch-1.0, gst-inspect-1.0)
sudo apt install gstreamer1.0-tools

# Gói chứa gst-device-monitor-1.0
sudo apt install gstreamer1.0-plugins-base-apps

# Plugin cho camera CSI (RPi5)
sudo apt install gstreamer1.0-libcamera

# Plugin encode ảnh và USB camera
sudo apt install gstreamer1.0-plugins-good

# Kiểm tra sau cài
gst-launch-1.0 --version
gst-inspect-1.0 libcamerasrc | head -5
```

> **Lưu ý:** `gst-device-monitor-1.0` nằm trong package **`gstreamer1.0-plugins-base-apps`**.
> Nếu chưa cài, dùng cách thay thế ở Bước 1B bên dưới.

**Trên hệ thống thực hành:** gstreamer1.0-libcamera (0.7.1+rpt20260609) đã có sẵn trong apt repo.

---

## Bước 1 — Nhận biết camera bằng GStreamer

### 1A. Kiểm tra plugin libcamerasrc

```bash
gst-inspect-1.0 libcamerasrc
```

**Output quan trọng:**
```
Factory Details:
  Rank: primary (256)
  Long-name: libcamera Source
  Klass: Source/Video

Pad Templates:
  SRC template: 'src'
    Capabilities:
      video/x-raw
      image/jpeg
      video/x-bayer
    Pad Properties:
      stream-role: video-recording | still-capture | raw | view-finder

Element Properties:
  camera-name: Select by name which camera to use
  sensor-config: Desired sensor configuration (width, height, depth)
  ...
```

### 1B. Liệt kê cameras — cách đã kiểm chứng

```bash
# gst-device-monitor-1.0 cần cài gstreamer1.0-tools
# Nếu chưa có, dùng cách này (đã test, hoạt động):
timeout 3 gst-launch-1.0 libcamerasrc ! fakesink 2>&1 | grep -iE "Adding camera|ov5|imx|ar[0-9]"
```

**Output thực tế:**
```
[INFO] Camera camera_manager.cpp Adding camera '/base/axi/pcie@.../ov5647@36'
```
→ Nhìn vào dòng **Adding camera** để biết tên và đường dẫn camera.

### 1C. Kiểm tra plugin v4l2src (USB camera)

```bash
gst-inspect-1.0 v4l2src
```

---

## Bước 2 — Kiểm tra thông tin camera qua GStreamer

### 2A. Xem format libcamerasrc có thể negotiate

```bash
# Thử các format khác nhau để xem format nào camera chấp nhận
timeout 3 gst-launch-1.0 libcamerasrc ! "video/x-raw,format=NV12,width=1296,height=972" ! fakesink 2>&1 | grep -iE "configuring streams|Selected sensor|ERROR"
```

**Output thực tế (thành công):**
```
[INFO] Camera camera.cpp configuring streams: (0) 1296x972-NV12/Rec709
[INFO] RPI pisp.cpp Selected sensor format: 1296x972-SGBRG10_1X10/RAW
```

> **Điểm quan trọng:** Dòng `configuring streams` cho thấy camera đã negotiate thành công với format `NV12`. ISP tự động xử lý RAW Bayer → NV12.

### 2B. Các lỗi negotiation thường gặp

```bash
# LỖI: caps quá chung chung, libcamerasrc chọn RAW Bayer, videoconvert không hiểu
timeout 3 gst-launch-1.0 libcamerasrc ! videoconvert ! fakesink 2>&1 | grep -E "ERROR|negot"
# => ERROR: streaming stopped, reason not-negotiated (-4)
# => Nguyên nhân: libcamerasrc mặc định negotiate sang video/x-bayer (RAW)
# => Sửa: phải chỉ định format=NV12 hoặc format=YUY2 tường minh
```

### 2C. Xem tất cả format hỗ trợ

```bash
# Kiểm tra từng format: NV12, YUY2, RGB, RGBA...
for fmt in NV12 YUY2 RGB I420; do
  result=$(timeout 2 gst-launch-1.0 libcamerasrc ! "video/x-raw,format=$fmt" ! fakesink 2>&1 | grep -c "configuring streams")
  [ "$result" -gt 0 ] && echo "$fmt: OK" || echo "$fmt: FAIL"
done
```

### 2D. gst-device-monitor (cần cài gstreamer1.0-tools)

```bash
gst-device-monitor-1.0 Video/Source
```

Để biết camera của bạn hỗ trợ các kích thước ảnh (độ phân giải) nào nhằm tránh lỗi negotiation, phương pháp thuần GStreamer là sử dụng công cụ giám sát thiết bị (cần lọc theo chiều rộng/cao để xem được đầy đủ danh sách từ thấp đến cao, tránh bị giới hạn dòng bởi grep):

```bash
# Liệt kê tất cả độ phân giải được hỗ trợ, sắp xếp từ thấp đến cao
gst-device-monitor-1.0 Video/Source | grep -E "width|height" | sort -u
```

*(Lưu ý: Nếu muốn kiểm tra trực tiếp ở mức driver phần cứng bằng lệnh v4l2-ctl của V4L2, hãy tham khảo tài liệu csi_camera_v4l2.md)*

---


## Bước 3 — Cài đặt thông số camera qua GStreamer

Với GStreamer, thông số được chỉ định qua **caps (capabilities)**:

```
element ! caps-filter ! element
```

### Cú pháp caps filter

```
video/x-raw,format=NV12,width=1296,height=972,framerate=46/1
```

| Trường | Ý nghĩa | Ví dụ |
|---|---|---|
| `format` | Pixel format — **phải chỉ định rõ** | `NV12`, `YUY2`, `I420` |
| `width` | Chiều rộng pixel | `1296` |
| `height` | Chiều cao pixel | `972` |
| `framerate` | FPS dạng fraction | `46/1`, `30/1` |

> ⚠️ **Quan trọng:** Với `libcamerasrc`, **bắt buộc phải chỉ định `format=NV12`** (hoặc format cụ thể khác). Nếu chỉ viết `video/x-raw,width=...,height=...` mà không có `format`, libcamerasrc sẽ negotiate sang `video/x-bayer` (RAW Bayer) và pipeline sẽ lỗi `not-negotiated`.

### Với libcamerasrc — set resolution (đã kiểm chứng)

```bash
# Đúng: phải có format=NV12
gst-launch-1.0 libcamerasrc ! "video/x-raw,format=NV12,width=1296,height=972" ! fakesink

# SAI: không có format → lỗi not-negotiated
# gst-launch-1.0 libcamerasrc ! "video/x-raw,width=1296,height=972" ! fakesink
```

### Với v4l2src — set resolution (USB camera)

```bash
gst-launch-1.0 v4l2src device=/dev/video0 ! "video/x-raw,format=YUY2,width=1280,height=720,framerate=30/1" ! fakesink
```

### Set controls của camera (exposure, gain...)

```bash
# Dùng v4l2-ctl để set trước khi chạy GStreamer pipeline
v4l2-ctl --device=/dev/v4l-subdev2 --set-ctrl=auto_exposure=0
v4l2-ctl --device=/dev/v4l-subdev2 --set-ctrl=gain_automatic=1

# Với v4l2src, có thể set qua property extra-controls
gst-launch-1.0 v4l2src device=/dev/video0 extra-controls="c,exposure=500" ! "video/x-raw,format=YUY2" ! fakesink
```

### Lựa chọn camera khi hệ thống có nhiều Camera

Nếu hệ thống của bạn cắm cùng lúc nhiều camera (ví dụ: 2 camera CSI trên Raspberry Pi 5 hoặc 1 camera CSI + 1 USB webcam), bạn cần chỉ định camera muốn stream bằng các thuộc tính sau:

#### 1. Đối với Camera CSI (Sử dụng `libcamerasrc`)
Bạn sử dụng thuộc tính **`camera-name`**. Tên này chính là đường dẫn phần cứng (hardware path) của camera được in ra ở Bước 1B hoặc qua lệnh `gst-device-monitor-1.0`.

```bash
# Lấy danh sách tên các camera CSI đang kết nối
gst-device-monitor-1.0 Video/Source | grep -i "api.libcamera.path"
# Ví dụ output: api.libcamera.path = "/base/axi/pcie@1000120000/rp1/i2c@88000/ov5647@36"

# Chỉ định camera muốn stream (ví dụ chọn camera CSI số 1)
gst-launch-1.0 libcamerasrc camera-name="/base/axi/pcie@1000120000/rp1/i2c@88000/ov5647@36" ! video/x-raw,format=NV12,width=1296,height=972 ! queue ! videoconvert ! queue ! glimagesink
```

#### 2. Đối với Camera USB hoặc V4L2 (Sử dụng `v4l2src`)
Bạn sử dụng thuộc tính **`device`** để chỉ định đường dẫn thiết bị `/dev/video*`.

```bash
# Stream từ USB camera số 1
gst-launch-1.0 v4l2src device=/dev/video0 ! video/x-raw,format=YUY2,width=1280,height=720 ! queue ! videoconvert ! queue ! glimagesink

# Stream từ USB camera số 2
gst-launch-1.0 v4l2src device=/dev/video1 ! video/x-raw,format=YUY2,width=1280,height=720 ! queue ! videoconvert ! queue ! glimagesink
```

---

## Bước 4 — Chụp ảnh bằng GStreamer

### Pipeline cơ bản — Sơ đồ tư duy

```
[Camera Source] → [Caps filter] → [Convert] → [Encode JPEG] → [Lưu file]

libcamerasrc  video/x-raw,    videoconvert  jpegenc         filesink
              format=NV12,                  snapshot=true   location=out.jpg
              width=1296,
              height=972
```

### Các lỗi đã gặp và cách sửa

| Lỗi | Nguyên nhân | Sửa |
|---|---|---|
| `no property "num-buffers" in element "libcamerasrc"` | libcamerasrc không có property này | Dùng `jpegenc snapshot=true` để tự dừng sau 1 frame |
| `not-negotiated (-4)` | Caps filter thiếu `format=` | Thêm `format=NV12` vào caps |
| `gst-device-monitor-1.0: command not found` | Chưa cài `gstreamer1.0-tools` | Dùng `timeout 3 gst-launch-1.0 libcamerasrc ! fakesink` thay thế |

### 4A. Chụp 1 ảnh từ CSI camera (RPi5) — ĐÃ KIỂM CHỨNG ✅

```bash
# snapshot=true: jpegenc tự gửi EOS sau khi encode 1 frame → pipeline tự dừng
gst-launch-1.0 libcamerasrc ! "video/x-raw,format=NV12,width=1296,height=972" ! videoconvert ! jpegenc snapshot=true ! filesink location=photo_hd.jpg
```

**Output khi thành công:**
```
Setting pipeline to PLAYING ...
[INFO] configuring streams: (0) 1296x972-NV12/Rec709
Got EOS from element "pipeline0".
```

**File output:** ~42KB (1 frame JPEG đúng)

**Giải thích pipeline:**

| Element | Vai trò |
|---|---|
| `libcamerasrc` | Nguồn camera CSI qua libcamera |
| `"video/x-raw,format=NV12,width=1296,height=972"` | Caps filter: bắt buộc có `format=NV12` |
| `videoconvert` | Chuyển NV12 → YUV/RGB cho jpegenc |
| `jpegenc snapshot=true` | Encode JPEG, gửi EOS sau 1 frame để dừng tự động |
| `filesink location=photo.jpg` | Ghi ra file |

### 4B. Chụp ảnh từ USB camera — dùng `v4l2src`

```bash
# v4l2src có num-buffers (khác libcamerasrc)
gst-launch-1.0 v4l2src device=/dev/video0 num-buffers=1 ! "video/x-raw,format=YUY2,width=1280,height=720" ! videoconvert ! jpegenc ! filesink location=photo_usb.jpg
```

### 4C. Xem preview trực tiếp (live view) — Khắc phục lỗi chớp tắt trên Wayland (Raspberry Pi 5)

Mặc định, `autovideosink` thường sử dụng `xvimagesink` (XVideo extension của X11). Tuy nhiên, Raspberry Pi 5 sử dụng display server **Wayland**. Khi chạy `xvimagesink` qua Xwayland, nó gây lỗi đồng bộ frame dẫn đến **hiện tượng chớp tắt (alternating black frames)**.

Để khắc phục, bạn có hai lựa chọn thay thế:
1. **`waylandsink`**: Lựa chọn native cho Wayland. GStreamer sẽ vẽ trực tiếp lên bề mặt Wayland compositor.
2. **`glimagesink`**: Lựa chọn **khuyên dùng** nhờ các đặc điểm kỹ thuật sau:
   - **Tự động thích ứng**: `glimagesink` sử dụng lớp trừu tượng OpenGL (EGL trên Linux). Khi chạy trên Wayland, nó sẽ tự động kết nối trực tiếp với Wayland compositor thông qua EGL API để render mà **không đi qua Xwayland**.
   - **Tối ưu hóa GPU (Zero-copy)**: Nó cho phép upload trực tiếp buffer hình ảnh từ camera (`dma-buf`) lên GPU texture để xử lý và hiển thị, giải phóng CPU hoàn toàn.
   - **Tính phổ quát**: Khác với `waylandsink` chỉ chạy được trên Wayland, `glimagesink` có thể chạy trên mọi hệ thống (X11, Wayland, Windows, macOS) nhờ cơ chế driver OpenGL thích ứng của nó. Bạn có thể mang nguyên pipeline này sang máy tính chạy X11 hoặc Windows mà không cần sửa code.

```bash
# Preview dùng OpenGL (Rất mượt, không chớp tắt, chạy được cả Wayland và X11)
gst-launch-1.0 libcamerasrc ! video/x-raw,format=NV12,width=1296,height=972 ! queue ! videoconvert ! queue ! glimagesink

# Preview dùng Wayland native (Chỉ chạy được trên hệ thống sử dụng Wayland)
gst-launch-1.0 libcamerasrc ! video/x-raw,format=NV12,width=1296,height=972 ! queue ! videoconvert ! queue ! waylandsink
```


### 4D. Quay video và lưu



```bash
# Quay 10 giây video (dùng timeout để dừng)
timeout 10 gst-launch-1.0 libcamerasrc ! "video/x-raw,format=NV12,width=1296,height=972" ! videoconvert ! x264enc ! mp4mux ! filesink location=video.mp4

# Quay MJPEG (đơn giản, không cần x264enc)
timeout 10 gst-launch-1.0 libcamerasrc ! "video/x-raw,format=NV12,width=1296,height=972" ! videoconvert ! jpegenc ! avimux ! filesink location=video.avi
```

---

## So sánh: libcamerasrc vs v4l2src

| Tiêu chí | `libcamerasrc` | `v4l2src` |
|---|---|---|
| **Backend** | libcamera | V4L2 kernel API trực tiếp |
| **CSI camera RPi5** | ✅ Hoạt động | ❌ routing issue |
| **CSI camera RPi4/cũ** | ✅ Hoạt động | ✅ Hoạt động |
| **USB camera** | ⚠️ Không phải mục đích | ✅ Hoạt động tốt |
| **Hệ thống non-RPi** | ⚠️ Cần cài libcamera | ✅ Có trên mọi Linux |
| **Auto-exposure/WB** | ✅ Tự động (IPA) | ⚠️ Phải set thủ công |
| **Cài đặt** | `gstreamer1.0-libcamera` | `gstreamer1.0-plugins-good` |
| **Phức tạp** | Ít hơn (abstracted) | Cần biết media pipeline |

---

## Trả lời câu hỏi: GStreamer có thỏa mãn yêu cầu không?

### ✅ Những gì GStreamer làm được

| Yêu cầu | GStreamer tool | Ghi chú |
|---|---|---|
| Nhận biết camera | `gst-device-monitor-1.0 Video/Source` | Phổ quát |
| Số lượng camera | `gst-device-monitor-1.0` | Đếm được |
| Thông tin camera | `gst-inspect-1.0 libcamerasrc` | Caps, format, resolution |
| Cài đặt thông số | Caps filter trong pipeline | `width=X,height=Y,framerate=N/1` |
| Chụp ảnh | `libcamerasrc ! ... ! filesink` | Hoạt động |

### ⚠️ Điều cần lưu ý

1. **Plugin phụ thuộc vào hardware:**  
   - RPi5 CSI → cần `gstreamer1.0-libcamera`  
   - USB camera → `gstreamer1.0-plugins-good` (có sẵn mọi nơi)

2. **GStreamer = framework, không phải magic:**  
   Nó vẫn dùng libcamera hoặc V4L2 bên dưới. Nó giúp bạn không cần viết code C/Python, chỉ cần dùng text pipeline.

3. **libcamerasrc có trên mọi Linux không?**  
   Không — package `gstreamer1.0-libcamera` phụ thuộc vào `libcamera` và chủ yếu hữu ích trên RPi. Trên server thông thường không có camera CSI nên không cần.

---

## Lệnh tham khảo nhanh — ĐÃ KIỂM CHỨNG

```bash
# Kiểm tra plugin
gst-inspect-1.0 libcamerasrc
gst-inspect-1.0 v4l2src

# Liệt kê cameras (thay thế gst-device-monitor nếu chưa cài)
timeout 3 gst-launch-1.0 libcamerasrc ! fakesink 2>&1 | grep -iE "Adding camera|ov5|imx"

# Liệt kê cameras (nếu đã cài gstreamer1.0-tools đầy đủ)
gst-device-monitor-1.0 Video/Source

# Chụp 1 ảnh từ CSI camera (RPi5) — ĐÚNG
gst-launch-1.0 libcamerasrc ! "video/x-raw,format=NV12,width=1296,height=972" ! videoconvert ! jpegenc snapshot=true ! filesink location=photo.jpg

# Chụp 1 ảnh từ USB camera (mọi Linux)
gst-launch-1.0 v4l2src device=/dev/video0 num-buffers=1 ! "video/x-raw,format=YUY2,width=1280,height=720" ! videoconvert ! jpegenc snapshot=true ! filesink location=photo_usb.jpg

# Preview live (dùng OpenGL tăng tốc, không bị chớp tắt trên Wayland/RPi5)
gst-launch-1.0 libcamerasrc ! "video/x-raw,format=NV12,width=640,height=480" ! queue ! videoconvert ! queue ! glimagesink

# Quay video 10 giây
timeout 10 gst-launch-1.0 libcamerasrc ! "video/x-raw,format=NV12,width=1296,height=972" ! videoconvert ! x264enc ! mp4mux ! filesink location=video.mp4

# Debug: xem caps negotiation chi tiết
GST_DEBUG=2 timeout 3 gst-launch-1.0 libcamerasrc ! "video/x-raw,format=NV12" ! fakesink 2>&1 | grep -iE "caps|negot|format"
```

---

## Kiến trúc tổng thể sau khi có GStreamer

```
Ứng dụng của bạn
       │
       │  gst-launch-1.0 (command line)
       │  HOẶC GStreamer API (C, Python, Rust...)
       ▼
GStreamer Framework (cross-platform)
       │
       ├─── libcamerasrc ──→ libcamera ──→ rp1-cfe/pispbe ──→ OV5647 (CSI)
       │
       ├─── v4l2src ────────→ V4L2 ──────→ USB Camera / CSI board cũ
       │
       └─── pipewiresrc ────→ PipeWire ──→ Camera qua audio/video server
```

**Kết luận:** GStreamer là lựa chọn tốt vì:
- Cú pháp pipeline đơn giản, dễ học
- Cross-platform thực sự
- Dễ mở rộng: thêm filter, encode, stream qua network...
- Không cần code để làm các tác vụ cơ bản

*Tài liệu được tạo bởi Antigravity AI — 2026-07-13*
