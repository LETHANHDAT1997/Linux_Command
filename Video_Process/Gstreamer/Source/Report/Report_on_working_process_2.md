# BÁO CÁO KỸ THUẬT & HƯỚNG DẪN SỬ DỤNG CHI TIẾT CLASS `GStreamer_Glue` (PHẦN 2)

---

## 1. Cây Tổ chức Kiến trúc sau Refactor & Diễn giải Chi tiết

Cấu trúc luồng thực thi trong `Private_Build_Video_Pipeline()` của lớp `GStreamer_Glue` được phân chia thành 5 helper functions chuyên biệt, giúp quản lý pipeline rõ ràng, mô đun hóa cao và dễ bảo trì:

```text
Private_Build_Video_Pipeline()            ← Coordinator ngắn gọn (~35 dòng)
    │
    ├── Private_Step_Create_Elements()     ← Step 1: Khởi tạo các GstElement & gán camera path
    │
    ├── Private_Step_Build_Caps()          ← Step 2: Xây dựng GstCaps từ Pipeline_Config & gán vào capsfilter
    │
    ├── Private_Step_Add_And_Link()        ← Step 3: Thêm elements vào bin & link theo thứ tự
    │
    ├── Private_Step_Configure_Sink()      ← Step 4: Cấu hình sink dùng switch-case (AppSink, FakeSink...)
    │
    └── Private_Step_Setup_Bus_And_Loop()  ← Step 5: Đăng ký Bus watch handler & tạo GMainLoop
```

### Diễn giải Chi tiết Nhiệm vụ của từng Nhánh trong Cây Tổ chức:

#### 1. `Private_Build_Video_Pipeline()` (Hàm Điều Phối - Coordinator)
* **Nhiệm vụ**: Đóng vai trò làm Controller trung tâm, đảm bảo tính tuần tự của quy trình khởi tạo.
* **Chi tiết công việc bên trong**:
  - Kiểm tra điều kiện biên (Validation): Kiểm tra danh sách `m_Camera_Devices_List` không rỗng và `camera_index` nằm trong phạm vi hợp lệ.
  - Lưu trữ trạng thái cấu hình hiện tại vào `GStreamer_Pipeline_Structure.active_config` để hỗ trợ runtime switching (`Switch_Camera()`).
  - Khởi tạo đối tượng Pipeline cha (`GstPipeline`) tên `"camera-preview-pipeline"` và quản lý qua con trỏ thông minh `GstElementPtr`.
  - Gọi lần lượt 5 hàm `Private_Step_*` theo thứ tự. Nếu bất kỳ bước nào báo lỗi (`false`), dừng quá trình và trả về `false`.

#### 2. `Private_Step_Create_Elements()` (Step 1 - Khởi tạo Element & Thiết lập Nguồn Camera)
* **Nhiệm vụ**: Cấp phát động toàn bộ các thành phần element phần cứng/phần mềm cần thiết cho luồng xử lý.
* **Chi tiết công việc bên trong**:
  - Xác định tên plugin nguồn dựa vào `source_type` (`libcamerasrc` cho CSI Raspberry Pi hoặc `v4l2src` cho USB V4L2).
  - Xác định tên plugin sink dựa vào `sink_type` (`glimagesink`, `fakesink`, `appsink`, `autovideosink`).
  - Gọi `gst_element_factory_make()` để khởi tạo 6 elements: `video_source`, `capsfilter`, `queue1`, `videoconvert`, `queue2`, `video_sink`.
  - Thiết lập thuộc tính đường dẫn thiết bị camera phù hợp (`"camera-name"` cho libcamerasrc hoặc `"device"` cho v4l2src) trỏ đến `camera_path`.
  - Đóng gói các element trung gian vào struct `Pipeline_Elements` trả về cho bước tiếp theo.

#### 3. `Private_Step_Build_Caps()` (Step 2 - Xây dựng & Đăng ký Định dạng Video Caps)
* **Nhiệm vụ**: Định hình cấu hình phân giải, màu sắc và tốc độ khung hình cho luồng video thô.
* **Chi tiết công việc bên trong**:
  - Khởi tạo `GstCaps` thông qua `gst_caps_new_simple()` với loại media `"video/x-raw"`.
  - Thiết lập thuộc tính format pixel (`pixel_format`: NV12, RGBA, RGB, YUY2...), chiều rộng (`width`), chiều cao (`height`).
  - Nếu `framerate_n > 0`: Bổ sung tham số fraction `framerate_n/framerate_d`. Nếu `= 0`: Bỏ qua ép framerate để camera tự động negotiate tối ưu.
  - Thắt chặt định dạng bằng cách gán `GstCaps` này vào thuộc tính `"caps"` của element `capsfilter`.
  - Bộ nhớ `GstCaps` được giải phóng tự động khi kết thúc hàm nhờ `GstCapsPtr`.

#### 4. `Private_Step_Add_And_Link()` (Step 3 - Đưa vào Bin & Tạo Chuỗi Liên kết Elements)
* **Nhiệm vụ**: Gắn kết tất cả các element đơn lẻ thành một pipeline hoàn chỉnh truyền dữ liệu liên tục.
* **Chi tiết công việc bên trong**:
  - Gọi `gst_bin_add_many()` để đưa `video_source`, `filter`, `queue1`, `conv`, `queue2`, `video_sink` vào trong `GstPipeline`. (Từ thời điểm này, `GstPipeline` nắm toàn bộ ownership bộ nhớ của các element này).
  - Gọi `gst_element_link_many()` để đấu nối các đầu cắm (Pads) theo đúng thứ tự luồng video:
    $$\text{video\_source} \longrightarrow \text{filter} \longrightarrow \text{queue1} \longrightarrow \text{conv} \longrightarrow \text{queue2} \longrightarrow \text{video\_sink}$$
  - Trả về `false` nếu có sự bất đồng bộ giữa các Pads liền kề.

#### 5. `Private_Step_Configure_Sink()` (Step 4 - Cấu hình Đặc thù cho Đầu ra Sink bằng `switch-case`)
* **Nhiệm vụ**: Thiết lập các tùy chỉnh sâu cho từng loại sink cụ thể trước khi phát luồng.
* **Chi tiết công việc bên trong**:
  - Sử dụng cấu trúc `switch (arg_config.sink_type)` linh hoạt:
  - **Trường hợp `VideoSinkType::AppSink`**:
    - Ép kiểu `video_sink` sang `GstAppSink*`.
    - Thiết lập các thuộc tính: `emit-signals=FALSE`, `max-buffers=arg_config.appsink_max_buffers`, `drop=TRUE` (bỏ frame cũ khi queue đầy), `sync=TRUE`.
    - Đăng ký struct `static GstAppSinkCallbacks` chứa lambda `new_sample`. Khi có frame mới, lambda này rút sample qua `gst_app_sink_pull_sample()`, map bộ nhớ buffer qua `gst_buffer_map()` và chuyển con trỏ pixel thô tới callback người dùng `appsink_frame_callback`.
  - **Trường hợp `GlImageSink` / `FakeSink` / `AutoVideoSink`**:
    - Chạy qua nhánh `default: break` (không cần thêm thao tác đặc thù).

#### 6. `Private_Step_Setup_Bus_And_Loop()` (Step 5 - Thiết lập Lắng nghe Sự kiện Bus & Main Loop)
* **Nhiệm vụ**: Chuẩn bị cơ chế lắng nghe lỗi hệ thống và khởi tạo vòng lặp sự kiện chính.
* **Chi tiết công việc bên trong**:
  - Lấy `GstBus` từ pipeline thông qua `gst_element_get_bus()`.
  - Đăng ký hàm tĩnh `Private_Bus_Call()` làm bus watch handler qua `gst_bus_add_watch()` để tự động bắt và xử lý các sự kiện `GST_MESSAGE_ERROR` (như camera bị chiếm dụng) hoặc `GST_MESSAGE_EOS` (kết thúc luồng).
  - Lưu con trỏ `bus_watch_id` vào `GStreamer_Event_Handler_Structure` để tự động gỡ bỏ handler khi pipeline bị hủy.
  - Cấp phát `GMainLoop` mới và lưu trữ trong con trỏ `GstMainLoopPtr`.

---

## 2. Tổng quan & Danh sách Chức năng của `GStreamer_Glue`

Lớp `GStreamer_Glue` được thiết kế nhằm mục đích đóng gói toàn bộ độ phức tạp của GStreamer C API thành một interface C++17 hướng đối tượng, an toàn bộ nhớ (sử dụng `std::unique_ptr` với custom deleters) và cực kỳ dễ tích hợp.

### Danh sách API Chức năng:

| Phương thức API | Mô tả & Trách nhiệm |
|---|---|
| `GStreamer_Glue("Video/Source")` | **Constructor**: Tự động gọi `gst_init()`, khởi tạo `GstDeviceMonitor` để tự động quét danh sách camera kết nối với hệ thống. |
| `Get_Devices_List()` | **Quét & Lấy thiết bị**: Trả về `const std::vector<Devices_Information>&` chứa tên, đường dẫn (`/dev/video*` hoặc media path), định dạng, độ phân giải & framerate hỗ trợ của từng camera. |
| `Start_Preview(config)` | **Khởi chạy Pipeline (Blocking)**: Xây dựng pipeline theo cấu hình `Pipeline_Config`, đưa pipeline sang `GST_STATE_PLAYING` và chạy `g_main_loop_run()`. |
| `Start_Preview(index, source_type)` | **Start Overload**: Phiên bản tương thích cũ, sử dụng các cấu hình mặc định (glimagesink, NV12, 640x480). |
| `Stop_Preview()` | **Dừng Pipeline**: Thoát `g_main_loop_run()`, đưa pipeline về `GST_STATE_NULL` để giải phóng phần cứng camera an toàn. |
| `Switch_Camera()` | **Chuyển Camera tức thì**: Tự động dừng camera hiện tại, cập nhật chỉ số camera tiếp theo (Round-Robin) và phát lại luồng video mới mà không làm rò rỉ bộ nhớ. |
| `Set_Interrupt_Callback(pair)` | **Đăng ký Tín hiệu Hệ thống**: Đăng ký các signal POSIX (như `SIGINT`, `SIGTERM`) với GStreamer Event Loop để xử lý tắt chương trình an toàn. |
| `Set_Keyboard_Callback(func)` | **Đăng ký Phím bấm từ Stdin**: Đọc phím bấm từ bàn phím (như phím Enter/`s` để chuyển camera) không chặn main loop. |

---

## 3. Quy trình các Step chuẩn để sử dụng `GStreamer_Glue` trong C++

Để sử dụng `GStreamer_Glue` trong ứng dụng của bạn (ví dụ: giao diện LVGL, ghi hình, xem camera), quy trình gồm **5 bước chuẩn**:

```text
┌─────────────────────────────────────────────────────────────────────────┐
│ Step 1: Khởi tạo GStreamer_Glue("Video/Source") & Quét danh sách Camera  │
└────────────────────────────────────┬────────────────────────────────────┘
                                     │
┌────────────────────────────────────▼────────────────────────────────────┐
│ Step 2: Khai báo & Cấu hình các thông số trong Pipeline_Config           │
└────────────────────────────────────┬────────────────────────────────────┘
                                     │
┌────────────────────────────────────▼────────────────────────────────────┐
│ Step 3: (Tùy chọn) Đăng ký Signal Interrupt & Keyboard Callback         │
└────────────────────────────────────┬────────────────────────────────────┘
                                     │
┌────────────────────────────────────▼────────────────────────────────────┐
│ Step 4: Gọi Start_Preview(config) để kích hoạt Pipeline & Loop           │
└────────────────────────────────────┬────────────────────────────────────┘
                                     │
┌────────────────────────────────────▼────────────────────────────────────┐
│ Step 5: Tương tác Runtime (Switch_Camera / Stop_Preview) & Giải phóng    │
└─────────────────────────────────────────────────────────────────────────┘
```

### Mã nguồn mẫu minh họa quy trình 5 bước:

```cpp
#include "Camera_Devices.cpp" // Hoặc bao gồm file header tương ứng

int main(int argc, char *argv[])
{
    // STEP 1: Khởi tạo & Lấy danh sách Camera đã quét
    GStreamer_Glue gstreamer_glue("Video/Source");
    const auto &devices = gstreamer_glue.Get_Devices_List();
    if (devices.empty()) return -1;

    // STEP 2: Cấu hình thông số Pipeline qua struct Pipeline_Config
    Pipeline_Config config;
    config.camera_index = 0;                        // Chọn camera đầu tiên
    config.source_type  = VideoSourceType::LibCamera; // Dùng libcamerasrc (CSI)
    config.pixel_format = "NV12";                   // Định dạng NV12
    config.width        = 640;
    config.height       = 480;
    config.sink_type    = VideoSinkType::AppSink;   // Nhận frame qua Callback
    config.appsink_max_buffers = 1;

    // Đăng ký callback nhận dữ liệu pixel
    config.appsink_frame_callback = [](const uint8_t *data, int w, int h, gpointer user_data) {
        // Dữ liệu pixel thô nằm tại 'data' (kích thước w * h * bytes_per_pixel)
        // Cập nhật bộ đệm này lên màn hình LVGL hoặc xử lý AI
    };

    // STEP 3: Đăng ký Signal xử lý Ctrl+C
    gstreamer_glue.Set_Interrupt_Callback({SIGINT, GStreamer_Glue::Default_Interrupt_Handler});

    // STEP 4: Bắt đầu phát luồng video (Blocking Main Loop)
    gstreamer_glue.Start_Preview(config);

    // STEP 5: Tự động dọn dẹp tài nguyên khi Destructor của gstreamer_glue được gọi
    return 0;
}
```

---

## 4. Các Thông số có thể Cấu hình (`Pipeline_Config`)

Bảng chi tiết toàn bộ các tham số trong `Pipeline_Config` có thể tùy chỉnh:

### 4.1. Cấu hình Nguồn Video (Source Config)

| Tham số | Kiểu dữ liệu | Giá trị mặc định | Ý nghĩa & Tùy chọn |
|---|---|---|---|
| `source_type` | `VideoSourceType` | `VideoSourceType::LibCamera` | `LibCamera`: Dùng `libcamerasrc` cho camera CSI Raspberry Pi.<br>`V4L2`: Dùng `v4l2src` cho camera USB chuẩn V4L2. |
| `camera_index` | `int` | `0` | Chỉ số camera trong danh sách quét `Get_Devices_List()` (0, 1, 2...). |

### 4.2. Cấu hình Định dạng Video (Caps Config)

| Tham số | Kiểu dữ liệu | Giá trị mặc định | Ý nghĩa & Tùy chọn |
|---|---|---|---|
| `pixel_format` | `std::string` | `"NV12"` | Format mã hóa pixel: `"NV12"`, `"RGBA"`, `"RGB"`, `"YUY2"`, `"I420"`, `"MJPEG"`. |
| `width` | `int` | `640` | Chiều rộng mong muốn (Pixel). |
| `height` | `int` | `480` | Chiều cao mong muốn (Pixel). |
| `framerate_n` | `int` | `0` | Tử số Framerate (VD: 30 cho 30fps). Nếu = `0`: Không ép framerate, camera tự chọn tối ưu. |
| `framerate_d` | `int` | `1` | Mẫu số Framerate (VD: 1 cho 30/1 fps). |

### 4.3. Cấu hình Đầu ra (Sink Config)

| Tham số | Kiểu dữ liệu | Giá trị mặc định | Ý nghĩa & Tùy chọn |
|---|---|---|---|
| `sink_type` | `VideoSinkType` | `VideoSinkType::GlImageSink` | **`GlImageSink`**: Hiển thị cửa sổ preview OpenGL window trực tiếp.<br>**`AutoVideoSink`**: Tự động chọn sink hiển thị phù hợp với OS.<br>**`FakeSink`**: Bỏ qua hiển thị (headless testing).<br>**`AppSink`**: Trả raw video frame về bộ nhớ RAM qua callback. |

### 4.4. Cấu hình riêng cho AppSink (`sink_type == AppSink`)

| Tham số | Kiểu dữ liệu | Giá trị mặc định | Ý nghĩa & Tùy chọn |
|---|---|---|---|
| `appsink_frame_callback` | `Frame_Callback` | `nullptr` | Hàm callback dạng `std::function<void(const uint8_t*, int, int, gpointer)>` được gọi tự động mỗi khi có video frame mới. |
| `appsink_user_data` | `gpointer` | `nullptr` | Con trỏ tùy chọn truyền vào `appsink_frame_callback` (ví dụ con trỏ màn hình GUI/LVGL). |
| `appsink_max_buffers` | `int` | `1` | Số lượng frame tối đa lưu trong hàng đợi queue. Tự động drop frame cũ khi đầy để đảm bảo luồng video realtime không bị trễ. |

---

## 5. Quá trình Hoạt động bên trong & Cấu trúc Refactor

Khi gọi `Start_Preview(config)`, `GStreamer_Glue` thực hiện luồng công việc qua **5 Step functions chuyên biệt**:

```text
Private_Build_Video_Pipeline(config)
  │
  ├──► [Step 1] Private_Step_Create_Elements()
  │    • Tạo video_source (libcamerasrc / v4l2src), capsfilter, queue1, videoconvert, queue2, video_sink.
  │    • Thiết lập thuộc tính đường dẫn thiết bị ("camera-name" hoặc "device").
  │
  ├──► [Step 2] Private_Step_Build_Caps()
  │    • Đóng gói format, width, height, framerate thành GstCaps.
  │    • Gán GstCaps vào element capsfilter.
  │    • Giải phóng bộ nhớ GstCaps tự động qua GstCapsPtr.
  │
  ├──► [Step 3] Private_Step_Add_And_Link()
  │    • Đưa tất cả elements vào GstPipeline bin (`gst_bin_add_many`).
  │    • Liên kết luồng: source ➔ filter ➔ queue1 ➔ videoconvert ➔ queue2 ➔ sink (`gst_element_link_many`).
  │
  ├──► [Step 4] Private_Step_Configure_Sink()
  │    • Kiểm tra sink_type bằng `switch-case`:
  │      - Nếu là AppSink: Set `max-buffers`, `drop=TRUE`, `sync=TRUE`, và thiết lập `GstAppSinkCallbacks` dạng `static`.
  │      - Nếu là GlImageSink / FakeSink: Không cần cấu hình thêm.
  │
  └──► [Step 5] Private_Step_Setup_Bus_And_Loop()
       • Đăng ký bus watch handler (`Private_Bus_Call`) để bắt lỗi `GST_MESSAGE_ERROR` hoặc `GST_MESSAGE_EOS`.
       • Tạo `GMainLoop` sẵn sàng thực thi.
```

### Chu kỳ chạy Runtime & Giải phóng:
1. `gst_element_set_state(pipeline, GST_STATE_PLAYING)`: Kích hoạt camera phát luồng.
2. `g_main_loop_run(main_loop)`: Chặn luồng chính để lắng nghe sự kiện bus & callback.
3. Khi ngắt (`Stop_Preview()` hoặc `Ctrl+C`):
   - `g_main_loop_quit()` kết thúc vòng lặp.
   - `gst_element_set_state(pipeline, GST_STATE_NULL)` giải phóng khóa phần একত্র camera an toàn.

---

## 6. Hướng dẫn Lệnh Biên dịch & Chạy Test

### 6.1. Chạy nhanh qua script `Build_and_Run.sh`
```bash
# Test Mode 1: Xem Live Preview cửa sổ OpenGL (Ctrl+C để dừng)
./Build_and_Run.sh 4 1

# Test Mode 2: Test Headless fakesink (Ctrl+C để dừng)
./Build_and_Run.sh 4 2

# Test Mode 3: Test AppSink Frame Callback (tự dừng sau 150 frame)
./Build_and_Run.sh 4 3
```

### 6.2. Lệnh biên dịch C++ bằng `g++` thủ công
```bash
g++ -std=c++17 -Wall -Wextra \
    $(pkg-config --cflags gstreamer-1.0 gstreamer-app-1.0 glib-2.0) \
    Camera_Devices.cpp \
    $(pkg-config --libs gstreamer-1.0 gstreamer-app-1.0 glib-2.0) \
    -o build/camera_devices_test
```
