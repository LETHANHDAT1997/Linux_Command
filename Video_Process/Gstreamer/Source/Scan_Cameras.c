#include <gst/gst.h>

/* Hàm in các cấu trúc Caps xuống dòng sau mỗi dấu chấm phẩy */
static void print_formatted_caps(const gchar *caps_str) {
    if (!caps_str) return;
    
    g_print("   - Định dạng hỗ trợ (Caps):\n     ");
    for (const gchar *p = caps_str; *p != '\0'; p++) {
        g_print("%c", *p);
        if (*p == ';') {
            // Kiểm tra xem ký tự tiếp theo có phải kết thúc không
            if (*(p + 1) != '\0') {
                g_print("\n     ");
                // Bỏ qua khoảng trắng tiếp theo sau dấu chấm phẩy nếu có
                if (*(p + 1) == ' ') {
                    p++;
                }
            }
        }
    }
    g_print("\n");
}

int main(int argc, char *argv[]) {
    GstDeviceMonitor *monitor;
    GList *devices, *l;
    int count = 0;

    /* Khởi tạo GStreamer */
    gst_init(&argc, &argv);

    /* Tạo bộ giám sát thiết bị (Device Monitor) */
    monitor = gst_device_monitor_new();

    /* Chỉ lọc lấy các thiết bị ghi hình (Video/Source) */
    gst_device_monitor_add_filter(monitor, "Video/Source", NULL);

    /* Khởi động bộ dò quét thiết bị */
    gst_device_monitor_start(monitor);
    devices = gst_device_monitor_get_devices(monitor);

    g_print("==================================================\n");
    g_print("   DANH SÁCH CAMERA ĐƯỢC PHÁT HIỆN QUA GSTREAMER  \n");
    g_print("==================================================\n");

    for (l = devices; l != NULL; l = l->next) {
        GstDevice *device = GST_DEVICE(l->data);
        gchar *display_name = gst_device_get_display_name(device);
        gchar *device_class = gst_device_get_device_class(device);
        
        g_print("%d. Tên hiển thị: %s\n", ++count, display_name);
        g_print("   - Phân loại: %s\n", device_class);
        
        /* Xem các định dạng/độ phân giải (Caps) hỗ trợ (Đã lọc bỏ video/x-bayer) */
        GstCaps *caps = gst_device_get_caps(device);
        if (caps) {
            /* Tạo bộ lọc chỉ nhận các định dạng video thô hoặc nén thông dụng */
            GstCaps *filter = gst_caps_from_string("video/x-raw; image/jpeg; video/x-h264");
            GstCaps *filtered_caps = gst_caps_intersect(caps, filter);

            if (filtered_caps && !gst_caps_is_empty(filtered_caps)) {
                gchar *caps_str = gst_caps_to_string(filtered_caps);
                print_formatted_caps(caps_str);
                g_free(caps_str);
            } else {
                g_print("   - Định dạng hỗ trợ (Caps): Không có định dạng thông thường phù hợp.\n");
            }

            if (filtered_caps) {
                gst_caps_unref(filtered_caps);
            }
            gst_caps_unref(filter);
            gst_caps_unref(caps);
        }
        
        g_free(display_name);
        g_free(device_class);
        g_print("--------------------------------------------------\n");
    }

    if (count == 0) {
        g_print("Không tìm thấy camera nào kết nối với hệ thống của bạn!\n");
    }

    /* Dọn dẹp tài nguyên để tránh rò rỉ bộ nhớ */
    g_list_free_full(devices, gst_object_unref);
    gst_device_monitor_stop(monitor);
    gst_object_unref(monitor);

    return 0;
}
