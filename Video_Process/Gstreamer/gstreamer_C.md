# 🎬 GStreamer C API cho Camera CSI — Hướng dẫn Thực tế

Tài liệu này hướng dẫn cách chuyển đổi các lệnh pipeline GStreamer từ [gstreamer_csi_camera.md](file:///home/ledat/Documents/Linux_Command/Video_Process/Gstreamer/gstreamer_csi_camera.md) sang mã nguồn ngôn ngữ **C**.

---

## 1. Chuẩn bị môi trường & Biên dịch

### 1.1 Cài đặt thư viện phát triển (Development Packages)
Trước khi lập trình GStreamer bằng C, bạn cần cài đặt các gói tiêu đề (`.h`) và thư viện (`.so`):

```bash
sudo apt update
# Cài đặt thư viện phát triển GStreamer và glib2.0
sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libglib2.0-dev
```

### 1.2 Lệnh biên dịch chuẩn (Compile Command)
Mọi chương trình GStreamer viết bằng C đều biên dịch qua `gcc` kết hợp với lệnh `pkg-config` để cấu hình thư viện:

```bash
gcc -o my_gst_app my_gst_app.c $(pkg-config --cflags --libs gstreamer-1.0)
```
> **Giải thích:**
> * `$(pkg-config --cflags gstreamer-1.0)`: Tự động thêm đường dẫn tới các file tiêu đề `.h` của GStreamer và GLib.
> * `$(pkg-config --libs gstreamer-1.0)`: Liên kết mã nguồn với các thư viện shared object tương ứng để chạy chương trình.

---

## 2. Các ví dụ mã nguồn C thực hành

### 📁 Bước 1: Nhận diện và liệt kê Camera (`GstDeviceMonitor`)
Tương ứng với mục **1B** và **2D** trong tài liệu CLI (dùng `gst-device-monitor-1.0 Video/Source`). Chương trình này dò quét các phần cứng video và in thông tin các camera đang được hệ thống kết nối.

Tạo file `list_cameras.c`:
```c
#include <gst/gst.h>

int main(int argc, char *argv[]) {
    GstDeviceMonitor *monitor;
    GList *devices, *l;
    int count = 0;

    /* Khởi tạo GStreamer */
    gst_init(&argc, &argv);

    /* Tạo bộ giám sát thiết bị */
    monitor = gst_device_monitor_new();

    /* Chỉ lọc các thiết bị cung cấp luồng Video (Video/Source) */
    gst_device_monitor_add_filter(monitor, "Video/Source", NULL);

    /* Bắt đầu dò quét thiết bị */
    gst_device_monitor_start(monitor);
    devices = gst_device_monitor_get_devices(monitor);

    g_print("==================================================\n");
    g_print("   DANH SÁCH CAMERA ĐƯỢC PHÁT HIỆN QUA GSTREAMER  \n");
    g_print("==================================================\n");

    for (l = devices; l != NULL; l = l->next) {
        GstDevice *device = GST_DEVICE(l->data);
        gchar *display_name = gst_device_get_display_name(device);
        gchar *device_class = gst_device_get_device_class(device);
        
        g_print("%d. Thiết bị: %s\n", ++count, display_name);
        g_print("   - Phân loại: %s\n", device_class);
        
        /* Đọc cấu trúc Capabilities (độ phân giải, format) hỗ trợ */
        GstCaps *caps = gst_device_get_caps(device);
        if (caps) {
            gchar *caps_str = gst_caps_to_string(caps);
            g_print("   - Khả năng xử lý (Caps):\n     %s\n", caps_str);
            g_free(caps_str);
            gst_caps_unref(caps);
        }
        
        g_free(display_name);
        g_free(device_class);
        g_print("--------------------------------------------------\n");
    }

    if (count == 0) {
        g_print("Không tìm thấy camera nào trên hệ thống của bạn!\n");
    }

    /* Dọn dẹp bộ nhớ */
    g_list_free_full(devices, gst_object_unref);
    gst_device_monitor_stop(monitor);
    gst_object_unref(monitor);

    return 0;
}
```

---

### 📷 Bước 2: Chụp 1 ảnh tĩnh (`gst_parse_launch`)
Tương ứng với mục **4A** trong tài liệu CLI:
```bash
gst-launch-1.0 libcamerasrc ! "video/x-raw,format=NV12,width=1296,height=972" ! videoconvert ! jpegenc snapshot=true ! filesink location=photo.jpg
```
Sử dụng hàm `gst_parse_launch` để nhanh chóng phân tích chuỗi pipeline dạng text trực tiếp trong code C.

Tạo file `capture_photo.c`:
```c
#include <gst/gst.h>

int main(int argc, char *argv[]) {
    GstElement *pipeline;
    GstBus *bus;
    GstMessage *msg;
    GError *error = NULL;

    /* Khởi tạo GStreamer */
    gst_init(&argc, &argv);

    /* 
     * Khởi tạo pipeline bằng chuỗi text (giống hệt lệnh gst-launch-1.0).
     * jpegenc snapshot=true giúp tự gửi tín hiệu kết thúc (EOS) sau khi encode 1 frame.
     */
    pipeline = gst_parse_launch(
        "libcamerasrc ! video/x-raw,format=NV12,width=1296,height=972 ! "
        "videoconvert ! jpegenc snapshot=true ! filesink location=photo_from_c.jpg",
        &error
    );

    if (error) {
        g_printerr("Không thể dựng pipeline: %s\n", error->message);
        g_error_free(error);
        return -1;
    }

    g_print("Đang khởi động camera và chụp ảnh...\n");
    
    /* Chuyển trạng thái pipeline sang PLAYING */
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    /* Lấy bus của pipeline để theo dõi các sự kiện lỗi hoặc kết thúc luồng (EOS) */
    bus = gst_element_get_bus(pipeline);
    
    /* Chờ vô hạn cho tới khi nhận được thông điệp lỗi hoặc EOS */
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    /* Xử lý thông điệp trả về */
    if (msg != NULL) {
        GError *err;
        gchar *debug_info;

        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                g_printerr("Lỗi từ phần tử %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
                g_printerr("Thông tin debug: %s\n", debug_info ? debug_info : "N/A");
                g_clear_error(&err);
                g_free(debug_info);
                break;
            case GST_MESSAGE_EOS:
                g_print("Chụp ảnh thành công! Đã lưu file: photo_from_c.jpg\n");
                break;
            default:
                break;
        }
        gst_message_unref(msg);
    }

    /* Giải phóng tài nguyên và đưa camera về trạng thái nghỉ (NULL) */
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}
```

---

### 🖥️ Bước 3: Live Preview thời gian thực (Xây dựng Pipeline thủ công)
Tương ứng với mục **4C** trong tài liệu CLI (sử dụng `glimagesink` để render mượt mà, không bị nhấp nháy trên Wayland/Raspberry Pi 5). 
Thay vì viết một chuỗi text dài, ở đây chúng ta tạo các element riêng lẻ bằng code C và liên kết chúng.

Tạo file `live_preview.c`:
```c
#include <gst/gst.h>
#include <glib.h>
#include <glib-unix.h>  /* Thư viện để bắt tín hiệu Ctrl+C trên Linux */

static GMainLoop *loop;

/* Callback xử lý tín hiệu ngắt Ctrl+C từ terminal */
static gboolean handle_interrupt(gpointer data) {
    g_print("\n[HỆ THỐNG] Nhận tín hiệu ngắt (Ctrl+C). Đang tắt preview...\n");
    g_main_loop_quit(loop);
    return FALSE; /* Chỉ chạy một lần rồi tự hủy callback */
}

/* Callback lắng nghe sự kiện trên Bus (nhận lỗi hoặc EOS từ các luồng phụ) */
static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    GMainLoop *loop = (GMainLoop *)data;

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("[BUS] Kết thúc luồng dữ liệu (EOS).\n");
            g_main_loop_quit(loop);
            break;
        case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *error;
            gst_message_parse_error(msg, &error, &debug);
            g_printerr("[BUS LỖI] %s\n", error->message);
            g_free(debug);
            g_error_free(error);
            g_main_loop_quit(loop);
            break;
        }
        default:
            break;
    }
    return TRUE;
}

int main(int argc, char *argv[]) {
    GstElement *pipeline;
    GstElement *source, *filter, *queue1, *conv, *queue2, *sink;
    GstCaps *caps;
    GstBus *bus;
    guint bus_watch_id;

    /* Khởi tạo */
    gst_init(&argc, &argv);
    loop = g_main_loop_new(NULL, FALSE);

    /* 1. Khởi tạo Pipeline cha */
    pipeline = gst_pipeline_new("camera-preview-pipeline");

    /* 2. Tạo các Elements con */
    source = gst_element_factory_make("libcamerasrc", "camera-source");
    filter = gst_element_factory_make("capsfilter", "caps-filter");
    queue1 = gst_element_factory_make("queue", "queue-1");
    conv   = gst_element_factory_make("videoconvert", "video-converter");
    queue2 = gst_element_factory_make("queue", "queue-2");
    sink   = gst_element_factory_make("glimagesink", "display-sink");

    if (!pipeline || !source || !filter || !queue1 || !conv || !queue2 || !sink) {
        g_printerr("Lỗi: Không thể khởi tạo một vài elements. Hãy kiểm tra các plugins.\n");
        return -1;
    }

    /* 3. Cấu hình Caps (Capabilities) cho filter để chọn định dạng camera */
    /* libcamerasrc bắt buộc cần format=NV12 để tránh lỗi negotiation */
    caps = gst_caps_new_simple("video/x-raw",
                               "format", G_TYPE_STRING, "NV12",
                               "width", G_TYPE_INT, 640,
                               "height", G_TYPE_INT, 480,
                               NULL);
    g_object_set(G_OBJECT(filter), "caps", caps, NULL);
    gst_caps_unref(caps); /* Giải phóng caps sau khi đã gán vào filter */

    /* 4. Thêm các phần tử vào trong container Pipeline */
    gst_bin_add_many(GST_BIN(pipeline), source, filter, queue1, conv, queue2, sink, NULL);

    /* 5. Liên kết các phần tử lại với nhau theo thứ tự:
     * source -> filter -> queue1 -> conv -> queue2 -> sink
     */
    if (!gst_element_link_many(source, filter, queue1, conv, queue2, sink, NULL)) {
        g_printerr("Lỗi: Không thể liên kết các elements lại với nhau!\n");
        gst_object_unref(pipeline);
        return -1;
    }

    /* 6. Thiết lập bus lắng nghe sự kiện */
    bus = gst_element_get_bus(pipeline);
    bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);

    /* 7. Đăng ký bắt tín hiệu Ctrl+C của hệ điều hành Linux */
    g_unix_signal_add(SIGINT, handle_interrupt, NULL);

    /* 8. Chạy Pipeline và khởi động Vòng lặp sự kiện chính (GMainLoop) */
    g_print("Đang khởi động live preview bằng OpenGL (glimagesink)...\n");
    g_print("Nhấn Ctrl+C trong terminal để dừng ứng dụng.\n");
    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    
    g_main_loop_run(loop); /* Chương trình sẽ dừng chặn (block) tại đây */

    /* 9. Dọn dẹp tài nguyên khi thoát vòng lặp */
    g_print("Đang tắt camera và giải phóng tài nguyên...\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline));
    g_source_remove(bus_watch_id);
    g_main_loop_unref(loop);

    return 0;
}
```

---

### 🎥 Bước 4: Quay video 10 giây và lưu file
Tương ứng với mục **4D** trong tài liệu CLI (sử dụng `x264enc` để mã hóa và `mp4mux` đóng gói thành file mp4).

> [!IMPORTANT]
> **Quy tắc vàng khi quay phim bằng GStreamer C:**
> Khi dừng ghi hình, bạn **không được** chuyển trực tiếp pipeline sang trạng thái `GST_STATE_NULL` một cách đột ngột. Nếu làm vậy, file `.mp4` sẽ bị lỗi cấu trúc (corrupted) vì muxer (`mp4mux`) chưa kịp ghi file header và đóng metadata (`moov` atom).
>
> Cách đúng là:
> 1. Gửi sự kiện `EOS` (End of Stream) đến pipeline: `gst_element_send_event(pipeline, gst_event_new_eos())`.
> 2. Đợi cho đến khi nhận được thông báo `EOS` trên Bus (cho thấy mọi buffer đã lưu hoàn toàn xuống đĩa cứng).
> 3. Sau đó mới chuyển pipeline sang `GST_STATE_NULL` và giải phóng bộ nhớ.

Tạo file `record_video.c`:
```c
#include <gst/gst.h>
#include <glib.h>

static GMainLoop *loop;
static GstElement *pipeline;

/* Callback được gọi sau 10 giây để dừng ghi hình một cách an toàn */
static gboolean stop_recording_callback(gpointer data) {
    g_print("\n[HỆ THỐNG] Đã quay đủ 10 giây. Đang gửi sự kiện EOS để lưu file an toàn...\n");
    
    /* Gửi sự kiện EOS đi vào pipeline */
    gst_element_send_event(pipeline, gst_event_new_eos());
    
    return FALSE; /* Trả về FALSE để bộ đếm giờ không gọi lại hàm này nữa */
}

/* Callback xử lý thông điệp bus */
static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    GMainLoop *loop = (GMainLoop *)data;

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("[BUS] Đã nhận tín hiệu EOS. Đóng file thành công!\n");
            g_main_loop_quit(loop);
            break;
        case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *error;
            gst_message_parse_error(msg, &error, &debug);
            g_printerr("[BUS LỖI] %s\n", error->message);
            g_free(debug);
            g_error_free(error);
            g_main_loop_quit(loop);
            break;
        }
        default:
            break;
    }
    return TRUE;
}

int main(int argc, char *argv[]) {
    GstBus *bus;
    guint bus_watch_id;
    GError *error = NULL;

    /* Khởi tạo */
    gst_init(&argc, &argv);
    loop = g_main_loop_new(NULL, FALSE);

    /* Tạo pipeline bằng chuỗi lệnh (x264enc + mp4mux) */
    pipeline = gst_parse_launch(
        "libcamerasrc ! video/x-raw,format=NV12,width=1296,height=972 ! "
        "videoconvert ! x264enc ! mp4mux ! filesink location=recorded_from_c.mp4",
        &error
    );

    if (error) {
        g_printerr("Lỗi khởi tạo pipeline: %s\n", error->message);
        g_error_free(error);
        return -1;
    }

    /* Thiết lập bus để chờ EOS */
    bus = gst_element_get_bus(pipeline);
    bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);

    g_print("Bắt đầu quay video 10 giây (lưu vào: recorded_from_c.mp4)...\n");
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    /* Cài đặt bộ hẹn giờ 10,000 miligiây (10 giây) */
    g_timeout_add(10000, stop_recording_callback, NULL);

    /* Chạy vòng lặp chính */
    g_main_loop_run(loop);

    /* Dọn dẹp */
    g_print("Đang giải phóng tài nguyên...\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_source_remove(bus_watch_id);
    g_main_loop_unref(loop);

    return 0;
}
```

---

## 3. Các lưu ý cốt lõi khi lập trình GStreamer C

1. **Rò rỉ bộ nhớ (Memory Leaks)**:
   * Mọi đối tượng kế thừa từ `GstObject` (như element, pipeline, bus) hoặc cấu trúc dữ liệu như `GstCaps`, `GstMessage` đều có cơ chế đếm tham chiếu (Reference counting).
   * Bạn **phải giải phóng** các tham chiếu này bằng `gst_object_unref()` hoặc `gst_caps_unref()` khi không còn dùng nữa để tránh rò rỉ RAM trên thiết bị nhúng.

2. **Coi chừng sự khác nhau của các Element tùy theo Camera**:
   * Nếu dùng **CSI camera trên Raspberry Pi 5**, sử dụng `libcamerasrc` và cấu hình caps `format=NV12`.
   * Nếu dùng **USB Camera / Webcam**, đổi phần tử nguồn thành `v4l2src` và chọn caps phù hợp (ví dụ: `format=YUY2` hoặc `image/jpeg`).

3. **GMainLoop**:
   * Khi ứng dụng cần chạy tương tác liên tục (như Live Preview hoặc stream luồng mạng), bạn phải dùng `GMainLoop`. Nó xử lý việc điều phối các luồng nền chạy bất đồng bộ bên dưới của GStreamer và kết xuất hiển thị.

*Tài liệu được tạo bởi Antigravity AI — 2026-07-13*
