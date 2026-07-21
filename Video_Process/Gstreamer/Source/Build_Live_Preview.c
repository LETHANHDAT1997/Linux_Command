#include <gst/gst.h>
#include <glib.h>
#include <glib-unix.h>  /* Thư viện để bắt tín hiệu Ctrl+C trên Linux */
#include <string.h>

#define MAX_CAMERAS 10
static gchar *camera_names[MAX_CAMERAS];
static gint camera_count = 0;
static gint current_camera_index = 0;
static GMainLoop *loop;

/* Chứa các tài nguyên tồn tại suốt vòng đời chương trình:
 *   - pipeline / source: cần giữ để switch_camera() có thể điều khiển.
 *   - bus_watch_id / io_watch_id: cần giữ để g_source_remove() khi thoát. */
typedef struct 
{
    GstElement *pipeline;
    GstElement *source;
    guint bus_watch_id;
    guint io_watch_id;
} AppData;

/* Callback xử lý tín hiệu ngắt Ctrl+C từ terminal */
static gboolean handle_interrupt(gpointer data) 
{
    g_print("\n[HỆ THỐNG] Nhận tín hiệu ngắt (Ctrl+C). Đang tắt preview...\n");
    g_main_loop_quit(loop);
    return FALSE; /* Chỉ chạy một lần rồi tự hủy callback */
}

/* Callback lắng nghe sự kiện trên Bus (nhận lỗi hoặc EOS từ các luồng phụ) */
static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) 
{
    GMainLoop *loop = (GMainLoop *)data;

    switch (GST_MESSAGE_TYPE(msg)) 
    {
        case GST_MESSAGE_EOS:
            g_print("[BUS] Kết thúc luồng dữ liệu (EOS).\n");
            g_main_loop_quit(loop);
            break;
        case GST_MESSAGE_ERROR: 
        {
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

/* Dò quét các thiết bị camera có sẵn */
static void scan_cameras(void) 
{
    GstDeviceMonitor *monitor;
    GList *devices, *l;

    monitor = gst_device_monitor_new();
    gst_device_monitor_add_filter(monitor, "Video/Source", NULL);
    gst_device_monitor_start(monitor);
    devices = gst_device_monitor_get_devices(monitor);

    for (l = devices; l != NULL && camera_count < MAX_CAMERAS; l = l->next) 
    {
        GstDevice *device = GST_DEVICE(l->data);
        gchar *display_name = gst_device_get_display_name(device);
        if (display_name) 
        {
            camera_names[camera_count++] = display_name;
        }
    }

    g_list_free_full(devices, gst_object_unref);
    gst_device_monitor_stop(monitor);
    gst_object_unref(monitor);
}

/* Thực hiện chuyển đổi camera */
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

/* Lắng nghe phím nhấn từ terminal */
static gboolean on_keyboard_input(GIOChannel *source, GIOCondition cond, gpointer data) 
{
    gchar *str = NULL;
    gsize size = 0;
    GError *err = NULL;
    GIOStatus status = g_io_channel_read_line(source, &str, &size, NULL, &err);

    if (status == G_IO_STATUS_NORMAL) 
    {
        g_strstrip(str);
        if (g_ascii_strcasecmp(str, "s") == 0 || g_ascii_strcasecmp(str, "switch") == 0 || strlen(str) == 0) 
        {
            AppData *app_data = (AppData *)data;
            switch_camera(app_data->pipeline, app_data->source);
        }
        g_free(str);
    }
    return TRUE;
}

int main(int argc, char *argv[]) 
{
    AppData app_data = {0};

    /* Khởi tạo GStreamer */
    gst_init(&argc, &argv);
    loop = g_main_loop_new(NULL, FALSE);

    /* Quét danh sách camera */
    scan_cameras();
    if (camera_count == 0) 
    {
        g_printerr("[HỆ THỐNG] Lỗi: Không tìm thấy camera nào kết nối với hệ thống!\n");
        return -1;
    }

    g_print("[HỆ THỐNG] Tìm thấy %d camera:\n", camera_count);
    for (gint i = 0; i < camera_count; i++) 
    {
        g_print("  %d) %s\n", i + 1, camera_names[i]);
    }

    /* 1. Khởi tạo Pipeline cha */
    app_data.pipeline = gst_pipeline_new("camera-preview-pipeline");

    /* 2. Tạo các Elements con
     * filter, queue1, conv, queue2, sink chỉ cần trong giai đoạn khởi tạo:
     * sau khi gst_bin_add_many() và gst_element_link_many(), pipeline sẽ
     * nắm quyền sở hữu (ownership) và quản lý vòng đời của chúng.
     * Vì vậy ta đặt chúng trong block cục bộ để làm rõ phạm vi sử dụng. */
    {
        GstElement *filter, *queue1, *conv, *queue2, *sink;

        app_data.source = gst_element_factory_make("libcamerasrc", "camera-source");
        filter  = gst_element_factory_make("capsfilter",   "caps-filter");
        queue1  = gst_element_factory_make("queue",        "queue-1");
        conv    = gst_element_factory_make("videoconvert", "video-converter");
        queue2  = gst_element_factory_make("queue",        "queue-2");
        sink    = gst_element_factory_make("glimagesink",  "display-sink");

        if (!app_data.pipeline || !app_data.source || !filter || !queue1 || !conv || !queue2 || !sink) 
        {
            g_printerr("Lỗi: Không thể khởi tạo một vài elements. Hãy kiểm tra các plugins.\n");
            return -1;
        }

        /* Gán camera mặc định ban đầu */
        g_object_set(G_OBJECT(app_data.source), "camera-name", camera_names[current_camera_index], NULL);
        g_print("[HỆ THỐNG] Camera khởi tạo mặc định: %s\n", camera_names[current_camera_index]);

        /* 3. Cấu hình Caps (Capabilities) cho filter để chọn định dạng camera
         * caps chỉ dùng một lần rồi unref ngay sau khi gán vào filter. */
        {
            GstCaps *caps = gst_caps_new_simple("video/x-raw",
                                                "format", G_TYPE_STRING, "NV12",
                                                "width",  G_TYPE_INT,    640,
                                                "height", G_TYPE_INT,    480,
                                                NULL);
            g_object_set(G_OBJECT(filter), "caps", caps, NULL);
            gst_caps_unref(caps);
        }

        /* 4. Thêm các phần tử vào trong container Pipeline */
        gst_bin_add_many(GST_BIN(app_data.pipeline), app_data.source, filter, queue1, conv, queue2, sink, NULL);

        /* 5. Liên kết các phần tử lại với nhau */
        if (!gst_element_link_many(app_data.source, filter, queue1, conv, queue2, sink, NULL)) 
        {
            g_printerr("Lỗi: Không thể liên kết các elements lại với nhau!\n");
            gst_object_unref(app_data.pipeline);
            return -1;
        }
    } /* filter, queue1, conv, queue2, sink hết phạm vi nhưng vẫn sống trong pipeline */

    /* 6. Thiết lập bus lắng nghe sự kiện
     * bus chỉ cần để đăng ký watch, sau đó unref ngay;
     * watch_id được lưu lại trong app_data để g_source_remove() khi cleanup. */
    {
        GstBus *bus = gst_element_get_bus(app_data.pipeline);
        app_data.bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
        gst_object_unref(bus);
    }

    /* 7. Đăng ký bắt tín hiệu Ctrl+C của hệ điều hành Linux */
    g_unix_signal_add(SIGINT, handle_interrupt, NULL);

    /* Thiết lập lắng nghe phím bấm từ terminal để switch camera
     * channel chỉ cần để đăng ký watch, sau đó unref ngay;
     * watch_id được lưu lại trong app_data để g_source_remove() khi cleanup. */
    {
        GIOChannel *channel = g_io_channel_unix_new(0); /* standard input fd = 0 */
        app_data.io_watch_id = g_io_add_watch(channel, G_IO_IN, on_keyboard_input, &app_data);
        g_io_channel_unref(channel);
    }

    /* 8. Chạy Pipeline và khởi động Vòng lặp sự kiện chính (GMainLoop) */
    g_print("\n========================================================\n");
    g_print("Đang khởi động live preview bằng OpenGL (glimagesink)...\n");
    g_print("Nhấn phím [ENTER] hoặc [s] + [ENTER] để chuyển đổi giữa các camera.\n");
    g_print("Nhấn Ctrl+C trong terminal để dừng ứng dụng.\n");
    g_print("========================================================\n\n");

    gst_element_set_state(app_data.pipeline, GST_STATE_PLAYING);
    g_main_loop_run(loop);

    /* 9. Dọn dẹp tài nguyên khi thoát vòng lặp */
    g_print("Đang tắt camera và giải phóng tài nguyên...\n");
    gst_element_set_state(app_data.pipeline, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(app_data.pipeline));
    g_source_remove(app_data.bus_watch_id);
    g_source_remove(app_data.io_watch_id);

    /* Giải phóng bộ nhớ tên camera */
    for (gint i = 0; i < camera_count; i++) 
    {
        g_free(camera_names[i]);
    }

    g_main_loop_unref(loop);

    return 0;
}
