# Report on Working Process: GStreamer Live Preview & Multi-Camera Pipeline Architecture

**Ngày báo cáo:** 2026-07-21  
**Dự án:** Video Processing with GStreamer & libcamera (Linux / Raspberry Pi 5)  
**Tác giả:** Antigravity AI & Lê Thành Đạt  
**Mã nguồn liên quan:** 
- [Build_Live_Preview.c](file:///home/ledat/Documents/Linux_Command/Video_Process/Gstreamer/Source/Build_Live_Preview.c)
- [Camera_Devices.cpp](file:///home/ledat/Documents/Linux_Command/Video_Process/Gstreamer/Source/Camera_Devices.cpp)
- [Scan_Cameras.c](file:///home/ledat/Documents/Linux_Command/Video_Process/Gstreamer/Source/Scan_Cameras.c)
- [Build_and_Run.sh](file:///home/ledat/Documents/Linux_Command/Video_Process/Gstreamer/Source/Build_and_Run.sh)

---

## 📋 MỤC LỤC

1. [Bối Cảnh & Mục Tiêu Dự Án](#1-bối-cảnh--mục-tiêu-dự-án)
2. [Chi Tiết Tiến Trình Thực Hiện (Step-by-Step Implementation)](#2-chi-tiết-tiến-trình-thực-hiện-step-by-step-implementation)
   - 2.1. Xây dựng ứng dụng Live Preview với OpenGL (`Build_Live_Preview.c`)
   - 2.2. Tính năng quét thiết bị & Chuyển đổi Camera động (Multi-Camera Switching)
   - 2.3. Thiết kế kiến trúc C++ RAII & Smart Pointers (`Camera_Devices.cpp`)
   - 2.4. Tích hợp Script tự động hóa (`Build_and_Run.sh`)
   - 2.5. Tuân thủ Quy chuẩn Format Code (`AGENTS.md`)
3. [Phân Tích Sâu Các Thắc Mắc Kỹ Thuật & Lý Thuyết GStreamer](#3-phân-tích-sâu-các-thắc-mắc-kỹ-thuật--lý-thuyết-gstreamer)
   - 3.1. Cơ chế Tắt Camera & Tối ưu hóa Hiệu năng ở mức Phần cứng Driver
   - 3.2. Kiến trúc Tích hợp Luồng Âm Thanh (Audio) & Thiết kế `AppData` Tập Trung
   - 3.3. Lý do luồng Audio cần các phần tử Chuyển đổi (`audioconvert`, `audioresample`)
   - 3.4. Bản chất & Chức năng Cốt lõi của Pipeline Cha (`GstPipeline` / `GstBin`)
   - 3.5. Cơ chế Bộ nhớ "Floating Reference" & Chuyển giao Ownership (`gst_bin_add`)
   - 3.6. Phân tích & Đánh giá Cấu trúc Smart Pointer C++ (`GStreamer_Pipeline_Structure`)
   - 3.7. Cơ chế Bộ đếm Tham chiếu (Reference Counting) của `GstBus` & `gst_object_unref`
4. [Trích Xuất Mã Nguồn & Sơ Đồ Hoạt Động](#4-trích-xuất-mã-nguồn--sơ-đồ-hoạt-động)
5. [Hướng Phát Triển Tiếp Theo](#5-hướng-phát-triển-tiếp-theo)

---

## 1. BỐI CẢNH & MỤC TIÊU DỰ ÁN

Trong các hệ thống xử lý video thời gian thực trên bo mạch nhúng (như Raspberry Pi 5 chạy hệ điều hành Linux), việc tương tác với phần cứng camera thông qua `libcamera` / `libcamerasrc` đòi hỏi ứng dụng phải đạt được các tiêu chuẩn khắt khe:
1. **Độ trễ cực thấp (Low Latency)** và **Hiệu năng hiển thị cao** thông qua gia tốc phần cứng OpenGL.
2. **Khả năng điều khiển linh hoạt**: Cho phép tự động dò tìm các camera CSI/USB kết nối với hệ thống và chuyển đổi luồng hiển thị (Switching) trực tiếp khi ứng dụng đang chạy.
3. **Tối ưu hóa tài nguyên phần cứng**: Ngừng đọc cảm biến camera không hoạt động để tránh lãng phí băng thông bus CSI/Media Controller và CPU/GPU.
4. **An toàn bộ nhớ (Memory Safety)**: Không xảy ra Memory Leak hay Double Free khi tương tác giữa ngôn ngữ C (GObject/GStreamer) và C++ (RAII Smart Pointers).

---

## 2. CHI TIẾT TIẾN TRÌNH THỰC HIỆN (STEP-BY-STEP IMPLEMENTATION)

### 2.1. Xây dựng ứng dụng Live Preview với OpenGL (`Build_Live_Preview.c`)

Ứng dụng được thiết kế dựa trên kiến trúc **Pipeline GStreamer thủ công (Manual Bin Creation)**:
- **`libcamerasrc`**: Phần tử nguồn (Source) đóng vai trò giao tiếp trực tiếp với stack `libcamera` trên Raspberry Pi.
- **`capsfilter`**: Bộ lọc năng lực. Đặt cố định cấu hình `video/x-raw, format=NV12, width=640, height=480`. Định dạng `NV12` là bắt buộc đối với `libcamerasrc` để tránh lỗi thương lượng định dạng (Caps Negotiation Error).
- **`queue` (1 & 2)**: Các hàng đợi bộ đệm chạy trên các Thread riêng biệt để tránh tình trạng nghẽn luồng giữa việc nhận ảnh từ camera và render hình ảnh lên màn hình.
- **`videoconvert`**: Bộ chuyển đổi không gian màu từ `NV12` sang định dạng RGBA mà card đồ họa yêu cầu.
- **`glimagesink`**: Phần tử đích (Sink) sử dụng OpenGL để render video trực tiếp lên cửa sổ hiển thị màn hình với hiệu năng phần cứng tối đa.

### 2.2. Tính năng quét thiết bị & Chuyển đổi Camera động (Multi-Camera Switching)

- **Dò quét thiết bị bằng `GstDeviceMonitor`**:
  ```c
  monitor = gst_device_monitor_new();
  gst_device_monitor_add_filter(monitor, "Video/Source", NULL);
  gst_device_monitor_start(monitor);
  devices = gst_device_monitor_get_devices(monitor);
  ```
  Hệ thống phát hiện thành công 2 camera CSI trên Raspberry Pi 5:
  1. Camera 1: `/base/axi/pcie@1000120000/rp1/i2c@88000/ov5647@36`
  2. Camera 2: `/base/axi/pcie@1000120000/rp1/i2c@70000/ov5647@36`

- **Lắng nghe phím bấm Terminal qua `GIOChannel`**:
  Tích hợp `GIOChannel` vào vòng lặp sự kiện `GMainLoop` của GLib để đọc dữ liệu nhập từ `stdin` (fd = 0) theo dạng asynchronous (không chặn Main Thread):
  ```c
  channel = g_io_channel_unix_new(0);
  io_watch_id = g_io_add_watch(channel, G_IO_IN, on_keyboard_input, &app_data);
  ```

- **Quy trình Chuyển đổi Camera (Hàm `switch_camera`)**:
  1. Đưa pipeline về trạng thái `GST_STATE_NULL` $\rightarrow$ giải phóng hoàn toàn camera hiện tại.
  2. Cập nhật chỉ số camera `current_camera_index = (current_camera_index + 1) % camera_count`.
  3. Gán thuộc tính tên camera mới: `g_object_set(G_OBJECT(source), "camera-name", next_camera, NULL);`.
  4. Khởi động lại pipeline: `gst_element_set_state(pipeline, GST_STATE_PLAYING);`.

### 2.3. Thiết kế kiến trúc C++ RAII & Smart Pointers (`Camera_Devices.cpp`)

Để đưa toàn bộ logic GStreamer vào môi trường C++ hiện đại, chúng tôi đã tạo các Custom Deleters cho `std::unique_ptr`:
```cpp
struct GstObjectDeleter {
    void operator()(GstElement *ptr) const {
        if (ptr) gst_object_unref(GST_OBJECT(ptr));
    }
};

using GstElementPtr = std::unique_ptr<GstElement, GstObjectDeleter>;
using GstBusPtr     = std::unique_ptr<GstBus, GstBusDeleter>;
```

Xây dựng cấu trúc chứa Pipeline hợp chuẩn:
```cpp
struct GStreamer_Pipeline_Structure 
{
    GstElementPtr pipeline; // Quản lý gốc RAII
    
    /* Con trỏ thô trỏ đến các element con đã được add vào Bin */
    GstElement *video_source    = nullptr;
    GstElement *video_sink      = nullptr;
    GstElement *audio_source    = nullptr;
    GstElement *audio_convert   = nullptr;
    GstElement *audio_resample  = nullptr;
    GstElement *audio_sink      = nullptr;
};
```

### 2.4. Tích hợp Script tự động hóa (`Build_and_Run.sh`)

Cập nhật kịch bản Shell `Build_and_Run.sh`:
- Biên dịch tự động kiểm tra `pkg-config --cflags --libs gstreamer-1.0`.
- Thêm **Option 5** vào menu hiển thị và nhận diện các tham số dòng lệnh CLI (`5`, `preview`, `live`).

### 2.5. Tuân thủ Quy chuẩn Format Code (`AGENTS.md`)

Áp dụng tuyệt đối các quy tắc định dạng C/C++:
1. All curly braces `{}` MUST be placed on their own separate line and aligned with the block header.
2. Code block body MUST be indented 4 spaces deeper than the braces.
3. Simple assignments written on one line.
4. Function signatures and function calls with less than 5 parameters kept on one line without unnecessary line breaks.

---

## 3. PHÂN TÍCH SÂU CÁC THẮC MẮC KỸ THUẬT & LÝ THUYẾT GSTREAMER

---

### 3.1. Cơ chế Tắt Camera & Tối ưu hóa Hiệu năng ở mức Phần cứng Driver

#### ❓ Thắc mắc:
*Khi switch giữa 2 camera, việc chuyển camera còn lại về trạng thái tắt có làm được không và có thực sự giúp tăng hiệu năng không?*

#### 🔬 Phân tích chi tiết:
**HOÀN TOÀN CÓ THỂ VÀ RẤT NÊN LÀM.**

1. **Các Trạng thái của GStreamer Pipeline (Pipeline States)**:
   - `GST_STATE_NULL`: Trạng thái mặc định ban đầu hoặc khi đã dừng hoàn toàn. Chưa cấp phát bất kỳ tài nguyên phần cứng nào.
   - `GST_STATE_READY`: Đã mở các thiết bị phần cứng (open file descriptors `/dev/video*`, `/dev/media*`), nhưng chưa mở stream I/O.
   - `GST_STATE_PAUSED`: Đã mở stream I/O, đã Preroll (lấy sẵn 1 frame dữ liệu vào đệm), nhưng Clock chưa chạy.
   - `GST_STATE_PLAYING`: Clock đang chạy, dữ liệu luân chuyển liên tục.

2. **Tác động ở mức phần cứng khi đưa về `GST_STATE_NULL`**:
   - Khi gọi `gst_element_set_state(pipeline, GST_STATE_NULL)`, driver `libcamerasrc` / `v4l2` sẽ thực hiện lời gọi hệ thống (ioctl) `VIDIOC_STREAMOFF` và `close()`.
   - Cảm biến camera dừng phát xung Clock, ngắt truyền tín hiệu MIPI CSI-2.
   - Bộ nhớ DMA Buffers (Direct Memory Access) cấp phát cho camera đó được giải phóng hoàn toàn về cho Hệ điều hành.
   - **Kết quả**: Giải phóng 100% tài nguyên CPU/GPU và băng thông phần cứng bus CSI cho camera đang hiển thị.

---

### 3.2. Kiến trúc Tích hợp Luồng Âm Thanh (Audio) & Thiết kế `AppData` Tập Trung

#### ❓ Thắc mắc:
*Nếu ứng dụng có thêm luồng âm thanh (Audio), có nên tạo một cấu trúc `AppData` mới riêng cho Audio không?*

#### 🔬 Phân tích chi tiết:
**KHÔNG NÊN TẠO `AppData` MỚI, MÀ NÊN DÙNG CHUNG (EXTEND) MỘT STRUCT `AppData` DUY NHẤT.**

```
               ┌────────────────────────────────────────────────────────┐
               │                  AppData (Struct chung)                │
               │                                                        │
               │ ┌────────────────────────────────────────────────────┐ │
               │ │                  pipeline (Parent)                 │ │
               │ └─────────────────────────┬──────────────────────────┘ │
               │                           │                            │
               │        ┌──────────────────┴──────────────────┐         │
               │        ▼                                     ▼         │
               │ ┌──────────────┐                      ┌──────────────┐ │
               │ │  Video Branch│                      │  Audio Branch│ │
               │ └──────────────┘                      └──────────────┘ │
               └────────────────────────────────────────────────────────┘
```

**Các lý do kỹ thuật bắt buộc**:
1. **Nguyên tắc Quản lý Tập trung (Single Source of Truth)**:
   - Âm thanh và Hình ảnh thuộc về cùng một luồng nghiệp vụ. Khi người dùng bấm dừng hoặc switch thiết bị, ứng dụng cần thay đổi trạng thái của cả luồng Video và Audio cùng một lúc.
2. **Ràng buộc con trỏ Callback trong GLib/GStreamer**:
   - Các hàm đăng ký sự kiện như `g_io_add_watch(channel, G_IO_IN, callback, user_data)` hay `gst_bus_add_watch(bus, bus_call, user_data)` chỉ cho phép truyền **duy nhất 1 con trỏ `gpointer user_data`**.
   - Nếu bạn tách thành nhiều struct `VideoAppData` và `AudioAppData`, hàm callback sẽ không thể truy cập đồng thời cả 2 nhánh để điều khiển.
3. **Đồng bộ hóa thời gian (A/V Synchronization)**:
   - Dùng chung 1 Pipeline giúp GStreamer sử dụng 1 Đồng hồ chung (`GstClock`), tự động cân bằng Lip-sync giữa tiếng và hình.

---

### 3.3. Lý do luồng Audio cần các phần tử Chuyển đổi (`audioconvert`, `audioresample`)

#### ❓ Thắc mắc:
*Tại sao luồng Audio lại cần nhiều phần tử trung gian như `audioconvert` và `audioresample`? Rút gọn có được không?*

#### 🔬 Phân tích chi tiết:
Có thể rút gọn nếu môi trường là cố định 100%, nhưng **trong thực tế GStreamer bắt buộc phải có 2 element này để tránh crash ứng dụng**.

1. **`audioconvert` (Tương tự `videoconvert`)**:
   - Microphone phần cứng ghi âm ở các định dạng thô khác nhau: Interleaved vs Non-interleaved, Signed Integer 16-bit (S16LE), Float 32-bit (F32LE), Mono (1 channel) hoặc Stereo (2 channels).
   - Card âm thanh đầu ra (Loa/ALSA/PulseAudio) có thể chỉ nhận `S16LE Stereo`.
   - `audioconvert` tự động biến đổi định dạng mẫu thô giữa đầu vào và đầu ra.

2. **`audioresample`**:
   - Tần số lấy mẫu (Sample Rate) của phần cứng ghi âm có thể là $44.1\,\text{kHz}$ ($44100\,\text{Hz}$), trong khi hệ thống âm thanh mặc định trên Linux (PulseAudio/PipeWire) yêu cầu $48\,\text{kHz}$ ($48000\,\text{Hz}$).
   - `audioresample` tính toán lại các điểm mẫu tín hiệu (interpolation) để tránh hiện tượng bị méo tiếng, giật tiếng hoặc lệch tốc độ phát.

---

### 3.4. Bản chất & Chức năng Cốt lõi của Pipeline Cha (`GstPipeline` / `GstBin`)

#### ❓ Thắc mắc:
*Ý nghĩa thực sự của một Pipeline cha là gì?*

#### 🔬 Phân tích chi tiết:
Pipeline cha (`GstPipeline`) kế thừa từ `GstBin`. Nó đóng vai trò là **Nhà máy quản lý cấp cao nhất**:

```
                       ┌──────────────────────────────────────────┐
                       │          Parent Pipeline (GstBin)        │
                       │                                          │
                       │   ┌────────┐    ┌────────┐    ┌────────┐ │
                       │   │ Source │───►│ Filter │───►│  Sink  │ │
                       │   └────────┘    └────────┘    └────────┘ │
                       │       ▲             ▲             ▲      │
                       │       └─────────────┼─────────────┘      │
                       │              State / Clock               │
                       └──────────────────────────────────────────┘
```

1. **Quản lý Trạng thái Đồng bộ Graph (State Management)**:
   - Khi bạn ra lệnh `gst_element_set_state(pipeline, GST_STATE_PLAYING)`, Pipeline sẽ tự động duyệt cây đồ thị (Graph) và gửi thông điệp chuyển trạng thái đến từng phần tử con theo thứ tự từ Sink ngược về Source.
2. **Cung cấp Đồng hồ Hệ thống Chung (Global Clock Provider)**:
   - Pipeline tự chọn một `GstClock` đáng tin cậy nhất (thường lấy từ Audio Sink hoặc Monotonic System Clock) và phân phát đồng hồ này cho toàn bộ các phần tử con để các khung hình và mẫu âm thanh được hiển thị đúng timestamp.
3. **Kênh Truyền Tin Tập Trung (`GstBus`)**:
   - Mọi thông báo lỗi (Error), Cảnh báo (Warning), Kết thúc luồng (EOS) từ bất kỳ phần tử con nào đều được đẩy lên Bus của Pipeline cha. Bạn chỉ cần nghe ở 1 nơi duy nhất.

---

### 3.5. Cơ chế Bộ nhớ "Floating Reference" & Chuyển giao Ownership (`gst_bin_add`)

#### ❓ Thắc mắc:
*Sau khi gọi `gst_bin_add`, các phần tử con này có bị giải phóng không? Tại sao con trỏ thô của ứng dụng vẫn truy cập bình thường?*

#### 🔬 Phân tích chi tiết:

```
[Khởi tạo] 
  gst_element_factory_make() ──► Creates Element with FLOATING REF (ref_count = 1, owner = NONE)

[Thêm vào Bin] 
  gst_bin_add(pipeline, source) ──► Bin Sinks Floating Ref ──► Ref Count = 1 (owner = PIPELINE)

[Ứng dụng sử dụng]
  g_object_set(source, ...) ──► Con trỏ source vẫn trỏ đúng ô nhớ trong RAM ──► HỢP LỆ!

[Dọn dẹp]
  gst_object_unref(pipeline) ──► Pipeline unref tất cả element con ──► Ref Count = 0 ──► FREED!
```

1. **Floating Reference (`GInitiallyUnowned`)**:
   - Khi tạo một `GstElement`, nó có trạng thái gọi là *Floating Reference*. Nghĩa là nó có bộ đếm tham chiếu bằng 1, nhưng chưa thuộc quyền sở hữu của ai.
2. **Chuyển giao Quyền sở hữu (Ownership Transfer)**:
   - Khi bạn gọi `gst_bin_add(GST_BIN(pipeline), source);`, Pipeline sẽ gọi hàm `g_object_ref_sink()`. Hàm này chuyển *Floating Reference* thành tham chiếu chính thức do Pipeline nắm giữ.
3. **Tại sao con trỏ thô `source` vẫn dùng được?**:
   - Đối tượng `source` vẫn nằm nguyên tại vị trí địa chỉ RAM đó.
   - Vì `pipeline` vẫn đang sống, nó giữ cho `source` không bị hủy.
   - Do đó, các câu lệnh như `g_object_set(G_OBJECT(source), ...)` hoàn toàn an toàn.
4. **CẢNH BÁO LỖI DOUBLE FREE**:
   - Vì Pipeline đã là chủ sở hữu của `source`, bạn **KHÔNG ĐƯỢC** gọi `gst_object_unref(source)` ở cuối chương trình. Nếu tự gọi, khi `pipeline` bị hủy nó lại gọi `unref(source)` lần nữa $\rightarrow$ Gây ra crash chương trình ngay lập tức (Double Free Memory Bug).

---

### 3.6. Phân tích & Đánh giá Cấu trúc Smart Pointer C++ (`GStreamer_Pipeline_Structure`)

#### ❓ Thắc mắc:
*Đánh giá cấu trúc kết hợp C++ Smart Pointer và Raw Pointer dưới đây:*

```cpp
struct GStreamer_Pipeline_Structure 
{
    GstElementPtr pipeline;
    
    GstElement *video_source    = nullptr;
    GstElement *video_sink      = nullptr;
    GstElement *audio_source    = nullptr;
    GstElement *audio_convert   = nullptr;
    GstElement *audio_resample  = nullptr;
    GstElement *audio_sink      = nullptr;
};
```

#### 🔬 Phân tích chi tiết:
**ĐÂY LÀ MỘT THIẾT KẾ C++ RAII CỰC KỲ CHUẨN XÁC KHI BỌC LỚP THƯ VIỆN C GOBJECT.**

- **`GstElementPtr pipeline` (Smart Pointer)**:
  - Giữ vai trò Quản lý Vòng đời (Lifetime Management). Khi biến chứa struct này ra khỏi phạm vi (out-of-scope), Custom Deleter `GstObjectDeleter` sẽ tự động gọi `gst_object_unref(GST_OBJECT(pipeline.get()))`.
- **Các phần tử con là `GstElement*` (Raw Pointer)**:
  - Tuyệt đối không được dùng `std::unique_ptr` cho các phần tử con này vì quyền sở hữu của chúng đã được trao cho `pipeline` (như đã phân tích ở Mục 3.5).
  - Dùng con trỏ thô `GstElement*` giúp ứng dụng truy cập nhanh để đọc/ghi thuộc tính mà không vi phạm nguyên tắc Single Ownership của C++ Smart Pointers.
- **Giá trị mặc định `= nullptr`**:
  - Đảm bảo tính an toàn trong C++11 trở lên, tránh con trỏ rác (Dangling/Wild Pointers).

---

### 3.7. Cơ chế Bộ đếm Tham chiếu (Reference Counting) của `GstBus` & `gst_object_unref`

#### ❓ Thắc mắc:
*Tại sao ở dòng 200 trong `Build_Live_Preview.c` lại phải gọi `gst_object_unref(bus);` ngay sau khi lấy bus và add watch?*

```c
bus = gst_element_get_bus(pipeline);
bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
gst_object_unref(bus);
```

#### 🔬 Phân tích chi tiết:

Hãy theo dõi biến động của **Reference Count (Bộ đếm tham chiếu)** đối với đối tượng Bus qua từng dòng lệnh:

| Bắt đầu câu lệnh | Thao tác | Tác động Ref Count | Ref Count Hiện tại | Giải thích |
| :--- | :--- | :---: | :---: | :--- |
| **Ban đầu** | Pipeline khởi tạo | +1 (bởi Pipeline) | **1** | Pipeline sở hữu Bus ban đầu. |
| `bus = gst_element_get_bus(pipeline);` | Gọi hàm `get_bus` | **+1** (Transfer Full) | **2** | Hàm `get_bus` tăng ref count để trao quyền sở hữu tạm thời cho biến `bus`. |
| `gst_bus_add_watch(bus, ...)` | Đăng ký Watch vào GLib | **+1** (Internal Watch) | **3** | GLib tự tăng ref count để đảm bảo Bus sống trong suốt thời gian Watch hoạt động. |
| `gst_object_unref(bus);` | **Giải phóng biến tạm** | **-1** | **2** | **HOÀN TRẢ THAM CHIẾU LẤY TỪ HÀM `get_bus`**. |
| **Thoát chương trình** | `gst_object_unref(pipeline)` | **-1** | **1** | Pipeline hủy tham chiếu của nó. |
| **Gỡ Watch** | `g_source_remove(bus_watch_id)` | **-1** | **0 (FREED)** | Ref count về 0, Bus được giải phóng an toàn hoàn toàn! |

#### ⚠️ Điều gì xảy ra nếu BỎ dọn dẹp `gst_object_unref(bus);`?
Nếu bỏ câu lệnh này, Ref Count sẽ bị dư 1 đơn vị. Khi ứng dụng kết thúc, Ref Count của Bus dừng lại ở mức 1 thay vì 0 $\rightarrow$ **Đối tượng Bus bị rò rỉ vĩnh viễn trong RAM (Memory Leak)**.

---

## 4. TRÍCH XUẤT MÃ NGUỒN & SƠ ĐỒ HOẠT ĐỘNG

### Mã nguồn chuyển đổi Camera an toàn trong `Build_Live_Preview.c`:

```c
static void switch_camera(GstElement *pipeline, GstElement *source) 
{
    if (camera_count <= 1) 
    {
        g_print("[HỆ THỐNG] Chỉ phát hiện 1 camera. Không thể chuyển đổi.\n");
        return;
    }

    g_print("[HỆ THỐNG] Đang chuyển đổi camera...\n");

    /* 1. Đặt trạng thái NULL cho pipeline để giải phóng camera hiện tại hoàn toàn */
    gst_element_set_state(pipeline, GST_STATE_NULL);

    /* 2. Cập nhật chỉ số camera */
    current_camera_index = (current_camera_index + 1) % camera_count;
    gchar *next_camera = camera_names[current_camera_index];
    g_print("[HỆ THỐNG] Chuyển sang Camera %d: %s\n", current_camera_index + 1, next_camera);

    /* 3. Gán camera mới cho nguồn libcamerasrc */
    g_object_set(G_OBJECT(source), "camera-name", next_camera, NULL);

    /* 4. Khởi chạy lại pipeline với camera mới */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
}
```

---

## 5. HƯỚNG PHÁT TRIỂN TIẾP THEO

1. **Xử lý Frame thô bằng OpenCV (`appsink`)**:
   - Chèn phần tử `appsink` song song với `glimagesink` thông qua phần tử `tee` để vừa xem preview vừa đưa ảnh về mảng `cv::Mat` xử lý AI.
2. **Ghi hình Video ra file (`v4l2h264enc` / `filesink`)**:
   - Thêm tính năng bấm nút trên Terminal để dynamically link thêm nhánh mã hóa H.264 và lưu thành file `.mp4`.
3. **Đóng gói hoàn thiện Class C++ (`GStreamer_Glue`)**:
   - Đóng gói toàn bộ logic C này vào một Class C++ chuẩn OOP để tích hợp vào dự án phần mềm tổng thể.
