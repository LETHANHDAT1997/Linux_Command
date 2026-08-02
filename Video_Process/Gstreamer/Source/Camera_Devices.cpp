#include "Camera_Devices.h"

#include <cstdio>   /* printf */
#include <cstdlib>  /* atoi */
#include <cstring>  /* strlen */

/**
 * @brief Chuyển đổi VideoSourceType sang tên GStreamer element tương ứng.
 * @param arg_type  Loại nguồn video.
 * @return Tên plugin GStreamer dưới dạng C-string.
 */
static const char *VideoSourceType_To_String(VideoSourceType arg_type)
{
    switch (arg_type)
    {
        case VideoSourceType::LibCamera: return "libcamerasrc";
        case VideoSourceType::V4L2:      return "v4l2src";
        default:                          return "libcamerasrc";
    }
}

/**
 * @brief Chuyển đổi VideoSinkType sang tên GStreamer element tương ứng.
 * @param arg_type  Loại sink video.
 * @return Tên plugin GStreamer dưới dạng C-string.
 */
static const char *VideoSinkType_To_String(VideoSinkType arg_type)
{
    switch (arg_type)
    {
        case VideoSinkType::GlImageSink:   return "glimagesink";
        case VideoSinkType::AutoVideoSink: return "autovideosink";
        case VideoSinkType::FakeSink:      return "fakesink";
        case VideoSinkType::AppSink:       return "appsink";
        default:                            return "glimagesink";
    }
}

/**
 * @brief Chuyển đổi VideoConverterType sang tên GStreamer element tương ứng.
 * @param arg_type  Loại converter.
 * @return Tên plugin GStreamer dưới dạng C-string.
 */
static const char *VideoConverterType_To_String(VideoConverterType arg_type)
{
    switch (arg_type)
    {
        case VideoConverterType::VideoConvert: return "videoconvert";
        case VideoConverterType::V4L2Convert:  return "v4l2convert";
        default:                                return "videoconvert";
    }
}


/**
 * @brief Trả về tên property để gán đường dẫn camera cho từng loại source.
 *
 * libcamerasrc dùng "camera-name"; v4l2src dùng "device".
 * @param arg_type  Loại nguồn video.
 * @return Tên property dưới dạng C-string.
 */
static const char *Camera_Source_Property(VideoSourceType arg_type)
{
    switch (arg_type)
    {
        case VideoSourceType::LibCamera: return "camera-name";
        case VideoSourceType::V4L2:      return "device";
        default:                          return "camera-name";
    }
}

/* NOTE: Các khai báo Pipeline_Config đã chuyển sang Camera_Devices.h
 * Phần dưới đây chỉ là phần triển khai (implementation) của GStreamer_Glue.
 * Bắt đầu với opening brace của Pipeline_Config để khớp cú pháp file gốc:
 */
/* ────────────────────────────────────────────────────────────────────────────
 * TRIỂN KHAI (Implementation) — bắt đầu tại đây
 * ────────────────────────────────────────────────────────────────────────── */

/* Dummy placeholder để giữ cấu trúc file; Pipeline_Config khai báo trong .h */
/* Toàn bộ khai báo class GStreamer_Glue đã chuyển sang Camera_Devices.h */

/* ────────────────────────────────────────────────────────────────────────────
 * TRIỂN KHAI — Constructor / Destructor
 * ────────────────────────────────────────────────────────────────────────── */

/*
 */
GStreamer_Glue::GStreamer_Glue(const std::string arg_filter_devices)
{
    /* Đảm bảo GStreamer đã được khởi tạo. gst_init() chỉ cần gọi một lần cho
     * toàn bộ tiến trình — nếu đã init từ trước (VD: tạo GStreamer_Glue thứ 2
     * cho camera sau trên dashcam 2 camera) thì đây là điều BÌNH THƯỜNG,
     * không phải lỗi, nên không in cảnh báo gì cả.
     * [FIX] Bản gốc in nhầm "Không thể khởi tạo!" đúng vào nhánh này (tức là
     * lúc mọi thứ đều ổn), khiến người dùng tưởng có lỗi trong khi không có. */
    if (!gst_is_initialized())
    {
        gst_init(nullptr, nullptr);
    }

    Private_Device_Monitor_Get_Devices(arg_filter_devices);
}

/*
 */
GStreamer_Glue::~GStreamer_Glue()
{
    /* [FIX Bug I] Cảnh báo nếu Start_Preview() vẫn đang chạy trên thread
     * khác. Đây gần như chắc chắn là lỗi sử dụng: destructor sắp sửa hủy
     * pipeline/GObject trong khi streaming thread của GStreamer có thể vẫn
     * đang truy cập chúng → use-after-free. Không thể tự chờ (join) ở đây vì
     * GStreamer_Glue không sở hữu thread đó, và việc chờ có thể deadlock
     * (Stop_Preview() có thể được gọi lại từ chính trong main loop, ví dụ
     * qua Private_Bus_Call/Default_Interrupt_Handler). Caller BẮT BUỘC phải
     * gọi Stop_Preview() rồi join() thread chạy Start_Preview() trước khi
     * để object này ra khỏi scope. */
    if (m_is_preview_running.load())
    {
        g_printerr(
            "[GStreamer_Glue] ⚠️  CẢNH BÁO NGHIÊM TRỌNG: object bị hủy trong khi\n"
            "  Start_Preview() vẫn đang chạy trên thread khác — nguy cơ cao\n"
            "  use-after-free/crash. Luôn gọi Stop_Preview() RỒI join() thread\n"
            "  chạy Start_Preview() TRƯỚC KHI GStreamer_Glue ra khỏi scope.\n");
    }

    /* ── Bước 1: Thoát main loop nếu đang chạy ────────────────────────────────
     * Phải làm trước mọi thứ để tránh deadlock khi pipeline đang stream. */
    if (GStreamer_Event_Handler_Structure.main_loop &&
        g_main_loop_is_running(GStreamer_Event_Handler_Structure.main_loop.get()))
    {
        g_main_loop_quit(GStreamer_Event_Handler_Structure.main_loop.get());
    }

    /* ── Bước 2: Các unique_ptr tự giải phóng theo thứ tự ngược khai báo ────────
     *   GStreamer_Event_Handler_Structure (reverse):
     *     main_loop      → g_main_loop_unref()
     *     bus_watch_id   → g_source_remove() + delete guint
     *     msg            → gst_message_unref()
     *     bus            → gst_object_unref()
     *   GStreamer_Pipeline_Structure (reverse):
     *     pipeline  → GstElementDeleter::operator():
     *                    gst_element_set_state(NULL)  ← giải phóng phần cứng
     *                    gst_object_unref()           ← giải phóng bộ nhớ
     *     (video_source / video_sink: raw ptr thuộc pipeline → tự hủy cùng)
     *   m_Camera_Devices_List    → std::vector tự giải phóng string/vector con
     *   m_interrupt_callbacks    → std::vector trivial pairs, tự giải phóng
     *   m_keyboard_callback      → function pointer, trivial */

    printf("[GStreamer_Glue] Đã giải phóng tài nguyên thành công.\n");
}

/* ────────────────────────────────────────────────────────────────────────────
 * TRIỂN KHAI — Quét thiết bị
 * ────────────────────────────────────────────────────────────────────────── */

/*
 */
void GStreamer_Glue::Private_Device_Monitor_Get_Devices(const std::string arg_filter_devices)
{
    /* Tạo bộ giám sát thiết bị (Device Monitor) */
    GstDeviceMonitorPtr local_device_monitor(gst_device_monitor_new());

    /* Chỉ lọc lấy các thiết bị theo class được truyền vào (VD "Video/Source").
     * [FIX] gst_device_monitor_add_filter("", NULL) KHÔNG giống add_filter
     * (NULL, NULL): chuỗi rỗng bị GLib tách thành mảng 1 phần tử rỗng và sẽ
     * không khớp thiết bị thật nào, trong khi NULL mới đúng nghĩa "mọi
     * class". Do arg_filter_devices mặc định là "", nếu không chuyển "" ->
     * nullptr ở đây thì gọi GStreamer_Glue() không đối số sẽ luôn âm thầm
     * ra danh sách thiết bị rỗng dù máy có camera thật. */
    const gchar *local_filter_classes =
        arg_filter_devices.empty() ? nullptr : arg_filter_devices.c_str();
    gst_device_monitor_add_filter(local_device_monitor.get(), local_filter_classes, NULL);

    /* Khởi động bộ dò quét thiết bị */
    gst_device_monitor_start(local_device_monitor.get());

    /* Lấy danh sách các thiết bị được tìm thấy */
    GstDeviceMonitorListPtr local_list_devices(gst_device_monitor_get_devices(local_device_monitor.get()));

    /* Parse thông tin thiết bị từ List */
    m_Camera_Devices_List = Private_Parse_Devices_Information(local_list_devices);
}

/*
 */
std::vector<Devices_Information> GStreamer_Glue::Private_Parse_Devices_Information(const GstDeviceMonitorListPtr &arg_list_devices)
{
    /* Khai báo biến return */
    std::vector<Devices_Information> local_list_devices;

    /* Duyệt qua từng thiết bị trong danh sách GList */
    for (GList *iter = arg_list_devices.get(); iter != NULL; iter = g_list_next(iter))
    {
        GstDevice *device = GST_DEVICE(iter->data);
        Devices_Information local_device_info;

        /* Lấy tên thiết bị (full path, VD: /base/axi/.../ov5647@36) */
        GCharPtr display_name(gst_device_get_display_name(device));
        local_device_info.Devices_Path = (display_name != nullptr) ? display_name.get() : "N/A";

        /* Trích xuất tên ngắn từ full path (VD: "ov5647@36" -> "ov5647") */
        if (display_name != nullptr)
        {
            std::string full_path = display_name.get();
            size_t last_slash = full_path.rfind('/');
            std::string short_name = (last_slash != std::string::npos) ? full_path.substr(last_slash + 1) : full_path;
            size_t at_pos = short_name.find('@');
            if (at_pos != std::string::npos)
            {
                short_name = short_name.substr(0, at_pos);
            }
            local_device_info.Devices_Name = short_name;
        }
        else
        {
            local_device_info.Devices_Name = "Unknown";
        }

        /* Lấy loại thiết bị */
        GCharPtr device_class(gst_device_get_device_class(device));
        local_device_info.Devices_Type = (device_class != nullptr) ? device_class.get() : "Unknown";

        /* Lấy danh sách Caps (metadata) của thiết bị */
        GstCapsPtr caps(gst_device_get_caps(device));
        if (caps != nullptr)
        {
            /* Tạo bộ lọc chỉ nhận các định dạng video thô hoặc nén thông dụng */
            GstCapsPtr filter(gst_caps_from_string("video/x-raw; image/jpeg; video/x-h264"));
            GstCapsPtr filtered_caps(gst_caps_intersect(caps.get(), filter.get()));

            if (filtered_caps != nullptr && !gst_caps_is_empty(filtered_caps.get()))
            {
                guint caps_size = gst_caps_get_size(filtered_caps.get());

                /* Cấp phát trước bộ nhớ để tránh reallocation trong vòng lặp */
                local_device_info.Metadata_List.reserve(caps_size);

                for (guint i = 0; i < caps_size; i++)
                {
                    /* GstStructure này thuộc về filtered_caps, không được free */
                    const GstStructure *cap_struct = gst_caps_get_structure(filtered_caps.get(), i);
                    Device_Metadata local_metadata;

                    /* Lấy MIME type (VD: video/x-raw, image/jpeg) */
                    local_metadata.Mime_Type = gst_structure_get_name(cap_struct);

                    /* Lấy Pixel Format */
                    const gchar *format = gst_structure_get_string(cap_struct, "format");
                    local_metadata.Pixel_Format = (format != NULL) ? format : "N/A";

                    /* Lấy Width và Height */
                    gst_structure_get_int(cap_struct, "width", &local_metadata.Width);
                    gst_structure_get_int(cap_struct, "height", &local_metadata.Height);

                    /* Lấy Framerate — tồn tại dưới 3 dạng trong GstCaps:
                     *   Dạng 1: fraction đơn  → framerate=(fraction)30/1
                     *   Dạng 2: list          → framerate=(fraction){ 30/1, 15/1 }
                     *   Dạng 3: range         → framerate=(fraction)[1/1,120/1]
                     * Cả 3 dạng đều được thu thập vào Framerate_List. */
                    const GValue *fps_val = gst_structure_get_value(cap_struct, "framerate");
                    if (fps_val != nullptr)
                    {
                        if (GST_VALUE_HOLDS_FRACTION(fps_val))
                        {
                            /* Dạng 1: fraction đơn */
                            int fps_n = gst_value_get_fraction_numerator(fps_val);
                            int fps_d = gst_value_get_fraction_denominator(fps_val);
                            local_metadata.Framerate_List.push_back({fps_n, fps_d});
                        }
                        else if (GST_VALUE_HOLDS_LIST(fps_val))
                        {
                            /* Dạng 2: list các fraction */
                            guint list_size = gst_value_list_get_size(fps_val);
                            local_metadata.Framerate_List.reserve(list_size);
                            for (guint j = 0; j < list_size; j++)
                            {
                                const GValue *item = gst_value_list_get_value(fps_val, j);
                                int fps_n = gst_value_get_fraction_numerator(item);
                                int fps_d = gst_value_get_fraction_denominator(item);
                                local_metadata.Framerate_List.push_back({fps_n, fps_d});
                            }
                        }
                        else if (GST_VALUE_HOLDS_FRACTION_RANGE(fps_val))
                        {
                            /* Dạng 3: range — lưu min và max */
                            const GValue *fps_min = gst_value_get_fraction_range_min(fps_val);
                            const GValue *fps_max = gst_value_get_fraction_range_max(fps_val);
                            int min_n = gst_value_get_fraction_numerator(fps_min);
                            int min_d = gst_value_get_fraction_denominator(fps_min);
                            int max_n = gst_value_get_fraction_numerator(fps_max);
                            int max_d = gst_value_get_fraction_denominator(fps_max);
                            local_metadata.Framerate_List.push_back({min_n, min_d});
                            local_metadata.Framerate_List.push_back({max_n, max_d});
                        }
                    }

                    local_device_info.Metadata_List.push_back(local_metadata);
                }
            }
        }

        local_list_devices.push_back(local_device_info);
    }

    return local_list_devices;
}

/*
 */
const std::vector<Devices_Information> &GStreamer_Glue::Get_Devices_List() const
{
    return m_Camera_Devices_List;
}

/* ────────────────────────────────────────────────────────────────────────────
 * TRIỂN KHAI — Xây dựng Pipeline
 * ────────────────────────────────────────────────────────────────────────── */

/*
 */
bool GStreamer_Glue::Private_Build_Video_Pipeline(const Pipeline_Config &arg_config)
{
    if (m_Camera_Devices_List.empty())
    {
        g_printerr("[GStreamer_Glue] Lỗi: Không có camera nào trong danh sách.\n");
        return false;
    }

    if (arg_config.camera_index < 0 ||
        arg_config.camera_index >= static_cast<int>(m_Camera_Devices_List.size()))
    {
        g_printerr("[GStreamer_Glue] Lỗi: camera_index=%d nằm ngoài danh sách (%zu camera).\n",
                   arg_config.camera_index, m_Camera_Devices_List.size());
        return false;
    }

    /* Lưu cấu hình vào struct để Switch_Camera() có thể dùng lại */
    GStreamer_Pipeline_Structure.active_config = arg_config;

    /* Tạo Pipeline cha */
    GStreamer_Pipeline_Structure.pipeline.reset(gst_pipeline_new("camera-preview-pipeline"));
    if (!GStreamer_Pipeline_Structure.pipeline)
    {
        g_printerr("[GStreamer_Glue] Lỗi: Không thể tạo pipeline.\n");
        return false;
    }

    Pipeline_Elements elems;
    if (!Private_Step_Create_Elements(arg_config, elems)) return false;
    Private_Step_Build_Caps(arg_config, elems.filter);
    if (!Private_Step_Add_And_Link(elems))                return false;
    Private_Step_Configure_Sink(arg_config);
    Private_Step_Setup_Bus_And_Loop();
    return true;
}

/*
 */
bool GStreamer_Glue::Private_Step_Create_Elements(const Pipeline_Config &arg_config,
                                                   Pipeline_Elements &arg_out)
{
    const std::string &camera_path  = m_Camera_Devices_List[arg_config.camera_index].Devices_Path;
    const char *source_plugin       = VideoSourceType_To_String(arg_config.source_type);
    const char *source_property     = Camera_Source_Property(arg_config.source_type);
    const char *sink_plugin         = VideoSinkType_To_String(arg_config.sink_type);

    /* Chọn converter theo enum VideoConverterType */
    const char *conv_plugin = VideoConverterType_To_String(arg_config.converter_type);

    GStreamer_Pipeline_Structure.video_source = gst_element_factory_make(source_plugin, "video-source");
    arg_out.filter                            = gst_element_factory_make("capsfilter",   "caps-filter");
    arg_out.queue1                            = gst_element_factory_make("queue",        "queue-1");
    arg_out.conv                              = gst_element_factory_make(conv_plugin,    "video-converter");
    arg_out.queue2                            = gst_element_factory_make("queue",        "queue-2");
    GStreamer_Pipeline_Structure.video_sink   = gst_element_factory_make(sink_plugin,    "display-sink");

    /* [CPU] videorate + rate_filter: giới hạn FPS xuống converter.
     * Camera vẫn capture ở tốc độ đầy đủ nhưng chỉ display_max_fps frame/giây
     * đi qua videoconvert (đoạn tốn CPU nhất). Không dùng nếu display_max_fps=0. */
    if (arg_config.display_max_fps > 0)
    {
        arg_out.rate        = gst_element_factory_make("videorate",    "fps-limiter");
        arg_out.rate_filter = gst_element_factory_make("capsfilter",   "fps-caps");
        if (!arg_out.rate || !arg_out.rate_filter)
        {
            g_printerr("[GStreamer_Glue] Cảnh báo: không tạo được videorate, bỏ display_max_fps.\n");
            /* Dọn sạch để không dùng */
            if (arg_out.rate)        { gst_object_unref(arg_out.rate);        arg_out.rate = nullptr; }
            if (arg_out.rate_filter) { gst_object_unref(arg_out.rate_filter); arg_out.rate_filter = nullptr; }
        }
        else
        {
            arg_out.display_max_fps = arg_config.display_max_fps;
        }
    }

    if (!GStreamer_Pipeline_Structure.video_source || !arg_out.filter || !arg_out.queue1 ||
        !arg_out.conv || !arg_out.queue2 || !GStreamer_Pipeline_Structure.video_sink)
    {
        g_printerr("[GStreamer_Glue] Lỗi: Không thể tạo một vài elements.\n"
                   "  source=%s  conv=%s  sink=%s\n",
                   source_plugin, conv_plugin, sink_plugin);
        return false;
    }

    /* Với V4L2Convert, cần tắt output-io-mode mặc định để tránh deadlock */
    if (arg_config.converter_type == VideoConverterType::V4L2Convert)
        g_object_set(G_OBJECT(arg_out.conv), "output-io-mode", 5 /*dmabuf-import*/, NULL);

    /* Gán camera theo đường dẫn đầy đủ */
    g_object_set(G_OBJECT(GStreamer_Pipeline_Structure.video_source),
                 source_property, camera_path.c_str(), NULL);
    g_print("[GStreamer_Glue] Nguồn: %-14s property=%-12s → %s\n",
            source_plugin, source_property, camera_path.c_str());
    g_print("[GStreamer_Glue] Sink  : %s\n", sink_plugin);
    g_print("[GStreamer_Glue] Conv  : %s%s\n",
            conv_plugin,
            arg_config.converter_type == VideoConverterType::V4L2Convert
                ? " (hardware M2M)"
                : " (software CPU)");
    if (arg_config.display_max_fps > 0 && arg_out.rate)
        g_print("[GStreamer_Glue] Rate  : giới hạn xuống %d fps trước converter\n",
                arg_config.display_max_fps);
    return true;
}

/*
 */
void GStreamer_Glue::Private_Step_Build_Caps(const Pipeline_Config &arg_config, GstElement *arg_filter)
{
    /* Tạo GstCaps từ config và gán vào capsfilter.
     * caps unref tự động khi ra khỏi scope (GstCapsPtr). */
    GstCapsPtr caps;
    if (arg_config.framerate_n > 0)
    {
        caps.reset(gst_caps_new_simple("video/x-raw",
                                       "format",    G_TYPE_STRING,     arg_config.pixel_format.c_str(),
                                       "width",     G_TYPE_INT,        arg_config.width,
                                       "height",    G_TYPE_INT,        arg_config.height,
                                       "framerate", GST_TYPE_FRACTION, arg_config.framerate_n,
                                                                       arg_config.framerate_d,
                                       NULL));
        g_print("[GStreamer_Glue] Caps  : %s %dx%d @ %d/%d fps\n",
                arg_config.pixel_format.c_str(),
                arg_config.width, arg_config.height,
                arg_config.framerate_n, arg_config.framerate_d);
    }
    else
    {
        caps.reset(gst_caps_new_simple("video/x-raw",
                                       "format", G_TYPE_STRING, arg_config.pixel_format.c_str(),
                                       "width",  G_TYPE_INT,    arg_config.width,
                                       "height", G_TYPE_INT,    arg_config.height,
                                       NULL));
        g_print("[GStreamer_Glue] Caps  : %s %dx%d (framerate: tự động)\n",
                arg_config.pixel_format.c_str(),
                arg_config.width, arg_config.height);
    }
    g_object_set(G_OBJECT(arg_filter), "caps", caps.get(), NULL);
}

/*
 */
bool GStreamer_Glue::Private_Step_Add_And_Link(const Pipeline_Elements &arg_elems)
{
    /* Thêm tất cả elements vào pipeline bin.
     * Sau lời gọi này pipeline nắm ownership của tất cả raw pointers. */
    if (arg_elems.rate && arg_elems.rate_filter)
    {
        /* Pipeline có rate limiter:
         * source → filter → queue1 → [rate → rate_filter] → conv → queue2 → sink */
        gst_bin_add_many(GST_BIN(GStreamer_Pipeline_Structure.pipeline.get()),
                         GStreamer_Pipeline_Structure.video_source,
                         arg_elems.filter,
                         arg_elems.queue1,
                         arg_elems.rate,
                         arg_elems.rate_filter,
                         arg_elems.conv,
                         arg_elems.queue2,
                         GStreamer_Pipeline_Structure.video_sink,
                         NULL);

        /* Ghi caps giới hạn fps vào rate_filter */
        GstCaps *rate_caps = gst_caps_new_simple(
            "video/x-raw",
            "framerate", GST_TYPE_FRACTION,
                arg_elems.display_max_fps, 1,
            NULL);
        g_object_set(G_OBJECT(arg_elems.rate_filter), "caps", rate_caps, NULL);
        gst_caps_unref(rate_caps);

        if (!gst_element_link_many(GStreamer_Pipeline_Structure.video_source,
                                   arg_elems.filter,
                                   arg_elems.queue1,
                                   arg_elems.rate,
                                   arg_elems.rate_filter,
                                   arg_elems.conv,
                                   arg_elems.queue2,
                                   GStreamer_Pipeline_Structure.video_sink,
                                   NULL))
        {
            g_printerr("[GStreamer_Glue] Lỗi: Không thể liên kết các elements (với rate).\n");
            return false;
        }
    }
    else
    {
        /* Pipeline không có rate limiter (chuẩn):
         * source → filter → queue1 → conv → queue2 → sink */
        gst_bin_add_many(GST_BIN(GStreamer_Pipeline_Structure.pipeline.get()),
                         GStreamer_Pipeline_Structure.video_source,
                         arg_elems.filter,
                         arg_elems.queue1,
                         arg_elems.conv,
                         arg_elems.queue2,
                         GStreamer_Pipeline_Structure.video_sink,
                         NULL);

        if (!gst_element_link_many(GStreamer_Pipeline_Structure.video_source,
                                   arg_elems.filter,
                                   arg_elems.queue1,
                                   arg_elems.conv,
                                   arg_elems.queue2,
                                   GStreamer_Pipeline_Structure.video_sink,
                                   NULL))
        {
            g_printerr("[GStreamer_Glue] Lỗi: Không thể liên kết các elements.\n");
            return false;
        }
    }
    return true;
}

/*
 */
void GStreamer_Glue::Private_Step_Configure_Sink(const Pipeline_Config &arg_config)
{
    switch (arg_config.sink_type)
    {
        case VideoSinkType::AppSink:
        {
            GstAppSink *appsink = GST_APP_SINK(GStreamer_Pipeline_Structure.video_sink);

            /* [FIX Bug A] Ép định dạng ĐẦU RA của appsink về một giá trị cố
             * định (mặc định "RGB16" = RGB565). Nếu không có bước này,
             * videoconvert phía trước không có lý do gì để chuyển đổi (không
             * ai ở phía sau yêu cầu định dạng khác) và sẽ pass-through
             * nguyên định dạng của camera (VD: NV12 — dữ liệu YUV, LVGL
             * không hiểu được) thẳng vào appsink. Đã kiểm chứng bằng thực
             * nghiệm: thiếu bước này, appsink nhận nguyên NV12 (640x480 →
             * 460.800 byte) thay vì định dạng RGB mong đợi. */
            std::string target_format = arg_config.appsink_pixel_format.empty()
                                             ? "RGB16"
                                             : arg_config.appsink_pixel_format;
            GstCapsPtr appsink_caps(gst_caps_new_simple("video/x-raw",
                                                        "format", G_TYPE_STRING, target_format.c_str(),
                                                        NULL));
            g_object_set(G_OBJECT(appsink), "caps", appsink_caps.get(), NULL);
            g_print("[GStreamer_Glue] AppSink: ép định dạng đầu ra = %s "
                    "(phải khớp LV_COLOR_DEPTH bên LVGL)\n", target_format.c_str());

            g_object_set(G_OBJECT(appsink),
                         "emit-signals", FALSE,
                         "max-buffers",  (guint)arg_config.appsink_max_buffers,
                         "drop",         TRUE,
                         "sync",         TRUE,
                         NULL);

            /* [FIX Bug E] Context là thành viên của instance (không phải
             * static function-local) — mỗi GStreamer_Glue có vùng nhớ riêng,
             * an toàn khi chạy song song nhiều instance (VD: 2 camera trước/
             * sau, mỗi camera một thread). */
            m_appsink_callback_context.self   = this;
            m_appsink_callback_context.config = &GStreamer_Pipeline_Structure.active_config;

            m_appsink_callbacks = {};
            m_appsink_callbacks.eos         = nullptr;
            m_appsink_callbacks.new_preroll = nullptr;
            m_appsink_callbacks.new_sample  = [](GstAppSink *sink, gpointer user_data) -> GstFlowReturn
            {
                auto *ctx = static_cast<AppSink_Callback_Context *>(user_data);

                GstSample *sample = gst_app_sink_pull_sample(sink);
                if (!sample) return GST_FLOW_ERROR;

                GstBuffer *buffer = gst_sample_get_buffer(sample);
                GstCaps   *caps   = gst_sample_get_caps(sample);

                if (!buffer)
                {
                    /* Phòng thủ: về lý thuyết không xảy ra với sample hợp lệ,
                     * nhưng tránh chắc chắn hơn là gọi gst_buffer_map(NULL). */
                    gst_sample_unref(sample);
                    return GST_FLOW_ERROR;
                }

                int w = 0, h = 0;
                if (caps)
                {
                    const GstStructure *st = gst_caps_get_structure(caps, 0);
                    gst_structure_get_int(st, "width",  &w);
                    gst_structure_get_int(st, "height", &h);
                }

                GstMapInfo map;
                if (gst_buffer_map(buffer, &map, GST_MAP_READ))
                {
                    /* Đường 1 — callback thô cho use case nâng cao.
                     * ⚠️ Chạy trên GStreamer streaming thread — xem cảnh báo
                     * trong Doxygen của Frame_Callback. Không gọi LVGL ở đây. */
                    if (ctx->config->appsink_frame_callback)
                    {
                        ctx->config->appsink_frame_callback(
                            static_cast<const uint8_t *>(map.data),
                            map.size,
                            w, h,
                            ctx->config->appsink_user_data);
                    }

                    /* [FIX Bug B] Đường 2 — buffer an toàn đa luồng cho LVGL.
                     * Luôn cập nhật bất kể có appsink_frame_callback hay
                     * không, để Try_Get_Latest_Frame() luôn hoạt động được. */
                    if (ctx->self)
                    {
                        std::lock_guard<std::mutex> lock(ctx->self->m_latest_frame_mutex);
                        const uint8_t *bytes = static_cast<const uint8_t *>(map.data);
                        ctx->self->m_latest_frame_data.assign(bytes, bytes + map.size);
                        ctx->self->m_latest_frame_width   = w;
                        ctx->self->m_latest_frame_height  = h;
                        ctx->self->m_latest_frame_updated = true;
                    }

                    gst_buffer_unmap(buffer, &map);
                }

                gst_sample_unref(sample);
                return GST_FLOW_OK;
            };

            gst_app_sink_set_callbacks(appsink, &m_appsink_callbacks, &m_appsink_callback_context, nullptr);

            if (arg_config.appsink_frame_callback != nullptr)
            {
                g_print("[GStreamer_Glue] AppSink: frame callback thô đã được đăng ký.\n");
            }
            g_print("[GStreamer_Glue] AppSink: Try_Get_Latest_Frame() sẵn sàng dùng "
                    "(an toàn gọi từ thread LVGL).\n");
            break;
        }

        case VideoSinkType::GlImageSink:
        case VideoSinkType::AutoVideoSink:
        case VideoSinkType::FakeSink:
        default:
            /* Các sink này không yêu cầu cấu hình bổ sung sau khi link. */
            break;
    }
}

/*
 */
void GStreamer_Glue::Private_Step_Setup_Bus_And_Loop()
{
    /* Thiết lập Bus watch.
     * bus chỉ cần trong block này; watch_id được lưu vào struct. */
    {
        GstBusPtr bus(gst_element_get_bus(GStreamer_Pipeline_Structure.pipeline.get()));
        guint *watch_id = new guint(gst_bus_add_watch(bus.get(), Private_Bus_Call, this));
        GStreamer_Event_Handler_Structure.bus_watch_id.reset(watch_id);
    }

    /* Tạo Main Loop */
    GStreamer_Event_Handler_Structure.main_loop.reset(g_main_loop_new(NULL, FALSE));
}

/* ────────────────────────────────────────────────────────────────────────────
 * TRIỂN KHAI — Callbacks (static)
 * ────────────────────────────────────────────────────────────────────────── */

/*
 */
gboolean GStreamer_Glue::Private_Bus_Call(GstBus *bus, GstMessage *msg, gpointer data)
{
    GStreamer_Glue *self = static_cast<GStreamer_Glue *>(data);

    switch (GST_MESSAGE_TYPE(msg))
    {
        case GST_MESSAGE_EOS:
            g_print("[BUS] Kết thúc luồng dữ liệu (EOS).\n");
            self->Stop_Preview();
            break;
        case GST_MESSAGE_ERROR:
        {
            gchar *debug;
            GError *error;
            gst_message_parse_error(msg, &error, &debug);
            g_printerr("[BUS LỖI] %s\n", error->message);
            g_free(debug);
            g_error_free(error);
            self->Stop_Preview();
            break;
        }
        case GST_MESSAGE_STREAM_START:
        {
            g_print("Start Stream........\n");
            break;
        }
        default:
            break;
    }
    return TRUE;
}

/*
 */
gboolean GStreamer_Glue::Default_Interrupt_Handler(gpointer data)
{
    GStreamer_Glue *self = static_cast<GStreamer_Glue *>(data);
    g_print("\n[GStreamer_Glue] Nhận tín hiệu ngắt (Ctrl+C). Đang tắt preview...\n");
    self->Stop_Preview();
    return FALSE; /* Chỉ chạy một lần rồi tự hủy */
}

/*
 */
gboolean GStreamer_Glue::Default_Keyboard_Handler(GIOChannel *channel, GIOCondition cond, gpointer data)
{
    gchar *str = NULL;
    gsize size = 0;
    GError *err = NULL;
    GIOStatus status = g_io_channel_read_line(channel, &str, &size, NULL, &err);

    if (status == G_IO_STATUS_NORMAL)
    {
        g_strstrip(str);
        if (g_ascii_strcasecmp(str, "s") == 0 ||
            g_ascii_strcasecmp(str, "switch") == 0 ||
            strlen(str) == 0)
        {
            GStreamer_Glue *self = static_cast<GStreamer_Glue *>(data);
            self->Switch_Camera();
        }
        g_free(str);
    }
    return TRUE;
}

/* ────────────────────────────────────────────────────────────────────────────
 * TRIỂN KHAI — Public API: Preview & Switch
 * ────────────────────────────────────────────────────────────────────────── */

/*
 */
void GStreamer_Glue::Set_Interrupt_Callback(std::pair<int, GSourceFunc> arg_signal_callback)
{
    /* Kiểm tra signal đã được đăng ký chưa để tránh lặp */
    for (const auto &entry : m_interrupt_callbacks)
    {
        if (entry.first == arg_signal_callback.first)
        {
            g_printerr("[GStreamer_Glue] Cảnh báo: Signal %d đã được đăng ký. Bỏ qua.\n",
                       arg_signal_callback.first);
            return;
        }
    }

    m_interrupt_callbacks.push_back(arg_signal_callback);
}

/*
 */
void GStreamer_Glue::Set_Keyboard_Callback(GIOFunc arg_callback)
{
    m_keyboard_callback = arg_callback;
}

/*
 */
bool GStreamer_Glue::Start_Preview(const Pipeline_Config &arg_config)
{
    if (!Private_Build_Video_Pipeline(arg_config))
    {
        return false;
    }

    /* Đăng ký tất cả các signal đã được cấu hình qua Set_Interrupt_Callback() */
    for (const auto &entry : m_interrupt_callbacks)
    {
        if (entry.second != nullptr)
        {
            g_unix_signal_add(entry.first, entry.second, this);
        }
    }

    /* Đăng ký lắng nghe phím bấm từ stdin nếu người dùng đã cấu hình callback
     * channel chỉ cần để đăng ký watch, unref ngay sau đó. */
    if (m_keyboard_callback != nullptr)
    {
        GIOChannel *channel = g_io_channel_unix_new(0); /* stdin fd = 0 */
        g_io_add_watch(channel, G_IO_IN, m_keyboard_callback, this);
        g_io_channel_unref(channel);
    }

    const char *sink_name = VideoSinkType_To_String(arg_config.sink_type);
    g_print("\n========================================================\n");
    g_print("  Đang khởi động live preview (sink: %s)...\n", sink_name);
    g_print("========================================================\n\n");

    gst_element_set_state(GStreamer_Pipeline_Structure.pipeline.get(), GST_STATE_PLAYING);

    /* [FIX Bug I] Đánh dấu đang chạy TRƯỚC khi block ở g_main_loop_run().
     * Nếu Start_Preview() được gọi trên thread riêng (mô hình bắt buộc khi
     * tích hợp LVGL, vì hàm này blocking), destructor có thể kiểm tra cờ
     * này để cảnh báo nếu bị hủy trước khi thread đó join() xong. */
    m_is_preview_running.store(true);

    /* Blocking cho đến khi Stop_Preview() được gọi */
    g_main_loop_run(GStreamer_Event_Handler_Structure.main_loop.get());

    /* Sau khi main loop kết thúc: đặt pipeline về NULL để giải phóng tài nguyên */
    gst_element_set_state(GStreamer_Pipeline_Structure.pipeline.get(), GST_STATE_NULL);

    m_is_preview_running.store(false);

    return true;
}

/*
 */
void GStreamer_Glue::Set_Pipeline_Config(const Pipeline_Config &arg_config)
{
    GStreamer_Pipeline_Structure.active_config = arg_config;
}

/*
 */
void GStreamer_Glue::Stop_Preview()
{
    if (GStreamer_Event_Handler_Structure.main_loop &&
        g_main_loop_is_running(GStreamer_Event_Handler_Structure.main_loop.get()))
    {
        g_main_loop_quit(GStreamer_Event_Handler_Structure.main_loop.get());
    }
}

/*
 */
void GStreamer_Glue::Switch_Camera()
{
    const int camera_count = static_cast<int>(m_Camera_Devices_List.size());

    if (camera_count <= 1)
    {
        g_print("[GStreamer_Glue] Chỉ phát hiện 1 camera. Không thể chuyển đổi.\n");
        return;
    }

    g_print("[GStreamer_Glue] Đang chuyển đổi camera...\n");

    /* 1. Đặt pipeline về NULL để giải phóng camera hiện tại hoàn toàn */
    gst_element_set_state(GStreamer_Pipeline_Structure.pipeline.get(), GST_STATE_NULL);

    /* 2. Cập nhật chỉ số camera (round-robin) trong active_config */
    Pipeline_Config &cfg = GStreamer_Pipeline_Structure.active_config;
    cfg.camera_index = (cfg.camera_index + 1) % camera_count;

    const std::string &next_camera = m_Camera_Devices_List[cfg.camera_index].Devices_Path;
    const char *source_property    = Camera_Source_Property(cfg.source_type);

    g_print("[GStreamer_Glue] Chuyển sang Camera %d: %s\n",
            cfg.camera_index + 1, next_camera.c_str());

    /* 3. Gán camera mới cho video_source (property đúng theo source type) */
    g_object_set(G_OBJECT(GStreamer_Pipeline_Structure.video_source),
                 source_property, next_camera.c_str(), NULL);

    /* 4. Khởi chạy lại pipeline với camera mới */
    gst_element_set_state(GStreamer_Pipeline_Structure.pipeline.get(), GST_STATE_PLAYING);
}

/*
 */
bool GStreamer_Glue::Try_Get_Latest_Frame(std::vector<uint8_t> &arg_out, int &arg_width, int &arg_height)
{
    std::lock_guard<std::mutex> lock(m_latest_frame_mutex);

    if (!m_latest_frame_updated)
    {
        return false;
    }

    arg_out    = m_latest_frame_data;   /* copy — an toàn để dùng ngoài khóa */
    arg_width  = m_latest_frame_width;
    arg_height = m_latest_frame_height;
    m_latest_frame_updated = false;

    return true;
}

#ifndef CAMERA_DEVICES_NO_MAIN
/*******************************************************************************
 * HÀM MAIN - TEST LIVE PREVIEW & SWITCH CAMERA
 * Guard này được kích hoạt khi build cùng main_pthread.cpp (CMakeLists.txt
 * định nghĩa CAMERA_DEVICES_NO_MAIN khi USE_PTHREAD=ON) để tránh conflict
 * "multiple definition of main".
 ******************************************************************************/
int main(int argc, char *argv[])
{
    printf("==============================================\n");
    printf("  🎥 TEST LIVE PREVIEW CAMERA (GStreamer C++)\n");
    printf("==============================================\n\n");

    /* Khởi tạo GStreamer_Glue với filter "Video/Source" để quét Camera */
    GStreamer_Glue gstreamer_glue("Video/Source");

    /* Lấy và in danh sách thiết bị đã quét */
    const std::vector<Devices_Information> &devices = gstreamer_glue.Get_Devices_List();

    if (devices.empty())
    {
        printf("⚠️  Không tìm thấy thiết bị Camera nào!\n");
        return -1;
    }

    printf("📋 Tìm thấy %zu thiết bị Camera:\n\n", devices.size());

    for (size_t i = 0; i < devices.size(); i++)
    {
        printf("----------------------------------------------\n");
        printf("  📷 Thiết bị #%zu\n", i + 1);
        printf("    Tên  : %s\n", devices[i].Devices_Name.c_str());
        printf("    Loại : %s\n", devices[i].Devices_Type.c_str());
        printf("    Path : %s\n", devices[i].Devices_Path.c_str());

        if (!devices[i].Metadata_List.empty())
        {
            printf("    Định dạng hỗ trợ (%zu):\n", devices[i].Metadata_List.size());
            for (size_t j = 0; j < devices[i].Metadata_List.size(); j++)
            {
                const Device_Metadata &meta = devices[i].Metadata_List[j];

                /* In thông tin cơ bản */
                printf("      [%zu] %s | Format: %-6s | %dx%d",
                       j + 1,
                       meta.Mime_Type.c_str(),
                       meta.Pixel_Format.c_str(),
                       meta.Width,
                       meta.Height);

                /* In framerate */
                if (meta.Framerate_List.empty())
                {
                    printf(" | FPS: N/A\n");
                }
                else if (meta.Framerate_List.size() == 1)
                {
                    /* Fraction đơn hoặc range với 1 giá trị */
                    printf(" | FPS: %d/%d\n",
                           meta.Framerate_List[0].first,
                           meta.Framerate_List[0].second);
                }
                else if (meta.Framerate_List.size() == 2)
                {
                    /* Có thể là range (min, max) hoặc list 2 phần tử */
                    printf(" | FPS: %d/%d ~ %d/%d\n",
                           meta.Framerate_List[0].first,
                           meta.Framerate_List[0].second,
                           meta.Framerate_List[1].first,
                           meta.Framerate_List[1].second);
                }
                else
                {
                    /* List nhiều framerate */
                    printf(" | FPS: ");
                    for (size_t k = 0; k < meta.Framerate_List.size(); k++)
                    {
                        if (k > 0) printf(", ");
                        printf("%d/%d",
                               meta.Framerate_List[k].first,
                               meta.Framerate_List[k].second);
                    }
                    printf("\n");
                }
            }
        }
        printf("\n");
    }

    /* ── Chọn test theo argv[1] ────────────────────────────────────────────
     * Không truyền arg → chỉ in device list rồi thoát.
     * argv[1] = "1" → TEST 1: glimagesink (dừng bằng Ctrl+C)
     * argv[1] = "2" → TEST 2: fakesink (dừng bằng Ctrl+C)
     * argv[1] = "3" → TEST 3: appsink (tự thoát sau 150 frame)
     * ───────────────────────────────────────────────────────────────────── */
    if (argc < 2)
    {
        printf("==============================================\n");
        printf("💡 Cách dùng: %s [1|2|3]\n", argv[0]);
        printf("   1 = glimagesink  (Ctrl+C để thoát)\n");
        printf("   2 = fakesink     (Ctrl+C để thoát)\n");
        printf("   3 = appsink      (tự thoát sau 150 frame)\n");
        printf("==============================================\n");
        return 0;
    }

    /* Đăng ký signal handler */
    gstreamer_glue.Set_Interrupt_Callback({SIGINT,  GStreamer_Glue::Default_Interrupt_Handler});
    gstreamer_glue.Set_Interrupt_Callback({SIGTERM, GStreamer_Glue::Default_Interrupt_Handler});

    int test_mode = atoi(argv[1]);

    if (test_mode == 1)
    {
        printf("==============================================\n");
        printf("🚀 TEST 1: glimagesink — Ctrl+C để thoát\n");
        printf("==============================================\n");

        gstreamer_glue.Set_Keyboard_Callback(GStreamer_Glue::Default_Keyboard_Handler);
        Pipeline_Config config_gl;
        bool result = gstreamer_glue.Start_Preview(config_gl);

        printf("==============================================\n");
        printf("%s TEST 1 %s.\n", result ? "✅" : "❌", result ? "thành công" : "thất bại");
        printf("==============================================\n");
    }
    else if (test_mode == 2)
    {
        printf("==============================================\n");
        printf("🚀 TEST 2: fakesink — Ctrl+C để thoát\n");
        printf("==============================================\n");

        Pipeline_Config config_fake;
        config_fake.camera_index = 0;
        config_fake.source_type  = VideoSourceType::LibCamera;
        config_fake.pixel_format = "NV12";
        config_fake.width        = 640;
        config_fake.height       = 480;
        config_fake.framerate_n  = 0;
        config_fake.sink_type    = VideoSinkType::FakeSink;

        bool result = gstreamer_glue.Start_Preview(config_fake);
        printf("==============================================\n");
        printf("%s TEST 2 %s.\n", result ? "✅" : "❌", result ? "thành công" : "thất bại");
        printf("==============================================\n");
    }
    else if (test_mode == 3)
    {
        printf("==============================================\n");
        printf("🚀 TEST 3: appsink — tự thoát sau 150 frame\n");
        printf("==============================================\n");

        int frame_count = 0;

        Pipeline_Config config_app;
        config_app.camera_index        = 0;
        config_app.source_type         = VideoSourceType::LibCamera;
        config_app.pixel_format        = "NV12";      /* định dạng CAMERA xuất ra */
        config_app.width               = 640;
        config_app.height              = 480;
        config_app.framerate_n         = 0;
        config_app.sink_type           = VideoSinkType::AppSink;
        config_app.appsink_pixel_format = "RGB16";     /* [FIX] định dạng appsink TRẢ VỀ (RGB565, khớp LVGL 16-bit) */
        config_app.appsink_max_buffers = 2;
        config_app.appsink_user_data   = &gstreamer_glue;

        config_app.appsink_frame_callback =
            [&frame_count](const uint8_t *data, size_t size, int w, int h, gpointer user_data)
            {
                (void)data;
                frame_count++;

                if (frame_count % 10 == 0)
                {
                    /* [FIX] In kích thước THẬT (từ GstMapInfo::size) thay vì
                     * giả định sai "RGBA / w*h*4" như bản gốc — bản gốc cấu
                     * hình NV12 nhưng lại tính như thể là RGBA, lệch 2.67 lần. */
                    printf("  [AppSink] Frame #%3d  %dx%d  (%zu bytes thực nhận, "
                           "RGB565 => kỳ vọng %d bytes)\n",
                           frame_count, w, h, size, w * h * 2);
                    fflush(stdout);
                }

                if (frame_count >= 150)
                {
                    GStreamer_Glue *glue = static_cast<GStreamer_Glue *>(user_data);
                    printf("  [AppSink] Đã nhận đủ 150 frame. Dừng pipeline.\n");
                    fflush(stdout);
                    glue->Stop_Preview();
                }
            };

        bool result = gstreamer_glue.Start_Preview(config_app);
        printf("==============================================\n");
        printf("%s TEST 3 %s. Tổng frame nhận: %d\n",
               result ? "✅" : "❌",
               result ? "thành công" : "thất bại",
               frame_count);
        printf("==============================================\n");
    }
    else
    {
        printf("❌ Lựa chọn không hợp lệ: %d (dùng 1, 2 hoặc 3)\n", test_mode);
        return 1;
    }

    return 0;
}
#endif /* CAMERA_DEVICES_NO_MAIN */

/* Không remove */
// class Camera_Devices
// {
// public:
//     /* Xóa các method tránh lỗi không mong muốn */
//     Camera_Devices& operator=(const Camera_Devices& other) = delete;
//     Camera_Devices(const Camera_Devices& other)            = delete;
//     Camera_Devices(Camera_Devices&& other)                 = delete;
//     Camera_Devices& operator=(Camera_Devices&& other)      = delete;
//
//     /**
//      * @brief Khởi tạo đối tượng quản lý thiết bị thiết bị với dung lượng
//      định trước.
//      */
//     Camera_Devices(void);
//
//     /**
//      * @brief Hủy đối tượng và dọn dẹp tài nguyên.
//      *
//      * Giải phóng toàn bộ bộ nhớ đã cấp phát động, đóng các kết nối thiết bị
//      đang hoạt động
//      * (nếu có) và đưa trạng thái hệ thống về an toàn.
//      */
//     ~Camera_Devices();
//
// private:
//
//     /*
//         Đối tượng quản lý GStreamer
//     */
//     GStreamer_Glue m_GStreamer_Glue;
// };