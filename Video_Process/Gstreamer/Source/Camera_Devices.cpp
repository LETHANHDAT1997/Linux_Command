#include <gst/gst.h>
#include <glib.h>
#include <glib-unix.h>
#include <memory>
#include <string>
#include <utility>
#include <vector>

/**
 * @brief Loại nguồn video được sử dụng trong pipeline GStreamer.
 */
enum class VideoSourceType
{
    LibCamera,  /* libcamerasrc — dành cho camera CSI trên Raspberry Pi */
    V4L2,       /* v4l2src       — dành cho camera USB (Video4Linux2)   */
};

/**
 * @brief Loại sink video ở cuối pipeline.
 */
enum class VideoSinkType
{
    GlImageSink,    /* glimagesink   — OpenGL window (mặc định, live preview) */
    AutoVideoSink,  /* autovideosink — tự chọn sink phù hợp với hệ thống     */
    FakeSink,       /* fakesink      — không hiển thị, dùng để test headless  */
};

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
        default:                            return "glimagesink";
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

struct Device_Metadata
{
    std::string Mime_Type;
    std::string Pixel_Format;
    int Width;
    int Height;
    /* Danh sách framerate hỗ trợ dưới dạng (tử số, mẫu số).
     * VD: {30,1}=30fps, {15,1}=15fps.
     * Rỗng nếu caps không khai báo framerate. */
    std::vector<std::pair<int, int>> Framerate_List;
};

struct Devices_Information
{
    std::string Devices_Name;
    std::string Devices_Type;
    std::string Devices_Path;
    std::vector<Device_Metadata> Metadata_List;
};

/**
 * @brief Toàn bộ tham số cấu hình cho một pipeline video.
 *
 * Truyền vào Start_Preview(Pipeline_Config) hoặc Set_Pipeline_Config()
 * trước khi build pipeline. Mọi trường đều có default hợp lý —
 * chỉ set những gì cần thay đổi.
 */
struct Pipeline_Config
{
    /* ── Nguồn ──────────────────────────────────────────────────────── */
    VideoSourceType source_type  = VideoSourceType::LibCamera;
    int             camera_index = 0;

    /* ── Định dạng video ─────────────────────────────────────────────── */
    std::string     pixel_format = "NV12";  /* NV12, RGBA, I420, MJPEG... */
    int             width        = 640;
    int             height       = 480;
    int             framerate_n  = 0;       /* 0/1 = không ép framerate   */
    int             framerate_d  = 1;

    /* ── Sink ────────────────────────────────────────────────────────── */
    VideoSinkType   sink_type    = VideoSinkType::GlImageSink;
};

class GStreamer_Glue
{
public:
    /* Xóa các method tránh lỗi không mong muốn */
    GStreamer_Glue &operator=(const GStreamer_Glue &other) = delete;
    GStreamer_Glue(const GStreamer_Glue &other) = delete;
    GStreamer_Glue(GStreamer_Glue &&other) = delete;
    GStreamer_Glue &operator=(GStreamer_Glue &&other) = delete;

    /**
     * @brief Khởi tạo đối tượng GStreamer.
     *
     * Khởi tạo thư viện GStreamer, thiết lập các cấu hình ban đầu và chuẩn bị các
     * tài nguyên cần thiết.
     * @param arg_filter_devices String Filter loại thiết bị (VD: \"Video/Source\"
     * để lọc Camera, \"Audio/Source\" để lọc Audio)
     */
    GStreamer_Glue(const std::string arg_filter_devices = "");

    /**
     * @brief Hủy đối tượng và dọn dẹp tài nguyên.
     *
     * Giải phóng toàn bộ bộ nhớ đã cấp phát động, đóng các kết nối thiết bị đang
     * hoạt động (nếu có) và đưa trạng thái hệ thống về an toàn.
     */
    ~GStreamer_Glue();

    /**
     * @brief Lấy danh sách thiết bị đã quét.
     *
     * @return const std::vector<Devices_Information>& danh sách thiết bị
     */
    const std::vector<Devices_Information> &Get_Devices_List() const;

    /**
     * @brief Khởi tạo pipeline và bắt đầu live preview (blocking) với cấu hình tùy chọn.
     *
     * Hàm sẽ gọi g_main_loop_run() và chặn cho đến khi nhận được tín hiệu
     * dừng (Ctrl+C hoặc Stop_Preview()). Nhấn [ENTER] / [s] để switch camera.
     *
     * @param arg_config  Cấu hình đầy đủ cho pipeline (sink, resolution, format...).
     * @return true nếu pipeline được khởi tạo thành công, false nếu có lỗi.
     */
    bool Start_Preview(const Pipeline_Config &arg_config);

    /**
     * @brief Overload backward-compatible: camera index + source type.
     *
     * Tương đương gọi Start_Preview(Pipeline_Config{source_type, camera_index}).
     * Các tham số khác (sink, format, resolution) dùng default của Pipeline_Config.
     *
     * @param arg_camera_index  Chỉ số camera trong danh sách (mặc định: 0).
     * @param arg_source_type   Loại nguồn video (mặc định: LibCamera).
     * @return true nếu pipeline được khởi tạo thành công, false nếu có lỗi.
     */
    bool Start_Preview(int arg_camera_index = 0,
                       VideoSourceType arg_source_type = VideoSourceType::LibCamera);

    /**
     * @brief Dừng pipeline và thoát khỏi main loop.
     */
    void Stop_Preview();

    /**
     * @brief Chuyển sang camera tiếp theo trong danh sách (round-robin).
     *
     * Dừng pipeline hiện tại, cập nhật chỉ số camera, rồi khởi động lại pipeline.
     */
    void Switch_Camera();

    /**
     * @brief Thiết lập cấu hình pipeline trước khi gọi Start_Preview().
     *
     * Gọi trước Start_Preview() để cấu hình resolution, format, sink type...
     * Nếu không gọi, Start_Preview() dùng giá trị default trong Pipeline_Config.
     * @param arg_config  Cấu hình đầy đủ cho pipeline.
     */
    void Set_Pipeline_Config(const Pipeline_Config &arg_config);

    /**
     * @brief Đăng ký một cặp (signal, callback) được gọi khi nhận tín hiệu hệ thống.
     *
     * Mỗi lần gọi sẽ thêm một entry vào danh sách nội bộ để đăng ký với
     * g_unix_signal_add() khi Start_Preview() chạy. Hàm sẽ in cảnh báo và bỏ qua
     * nếu signal đó đã được đăng ký trước đó.
     *
     * @param arg_signal_callback  std::pair gồm:
     *   - .first  : số hiệu signal POSIX (VD: SIGINT, SIGTERM).
     *   - .second : callback kiểu GSourceFunc (gboolean (*)(gpointer)).
     *               nullptr được chấp nhận nhưng không có hiệu lực.
     */
    void Set_Interrupt_Callback(std::pair<int, GSourceFunc> arg_signal_callback);

    /**
     * @brief Đăng ký callback được gọi khi có phím bấm từ stdin.
     *
     * Mặc định là nullptr — không đọc stdin. Truyền Default_Keyboard_Handler
     * để dùng hành vi switch camera mặc định (ENTER / s), hoặc truyền hàm của
     * riêng bạn.
     * @param arg_callback  Con trỏ hàm kiểu GIOFunc
     *                      (gboolean (*)(GIOChannel*, GIOCondition, gpointer)).
     *                      Truyền nullptr để tắt tính năng này.
     */
    void Set_Keyboard_Callback(GIOFunc arg_callback);

    /**
     * @brief Callback mặc định cho SIGINT: gọi Stop_Preview() rồi tự hủy.
     *
     * Dùng trực tiếp với Set_Interrupt_Callback(GStreamer_Glue::Default_Interrupt_Handler).
     */
    static gboolean Default_Interrupt_Handler(gpointer data);

    /**
     * @brief Callback mặc định cho stdin: nhấn [ENTER] hoặc [s] để switch camera.
     *
     * Dùng trực tiếp với Set_Keyboard_Callback(GStreamer_Glue::Default_Keyboard_Handler).
     */
    static gboolean Default_Keyboard_Handler(GIOChannel *channel, GIOCondition cond, gpointer data);

private:
    /*
        Định nghĩa Functor struct
        Deleter cho GObject
    */
    struct GstObjectDeleter
    {
        template <typename T>
        void operator()(T *obj) const
        {
            if (obj)
            {
                gst_object_unref(GST_OBJECT(obj));
            }
        }
    };

    /*
        Định nghĩa Functor struct
        Deleter riêng cho GstElement (pipeline).
        Bắt buộc đưa về GST_STATE_NULL trước khi unref để giải phóng
        camera / phần cứng an toàn. Không dùng GstObjectDeleter cho pipeline
        vì các GstObject khác (Bus, Message...) không có gst_element_set_state().
    */
    struct GstElementDeleter
    {
        void operator()(GstElement *element) const
        {
            if (element)
            {
                gst_element_set_state(element, GST_STATE_NULL);
                gst_object_unref(GST_OBJECT(element));
            }
        }
    };

    /*
        Định nghĩa Functor struct
        Deleter cho Message
    */
    struct GstMessageDeleter
    {
        void operator()(GstMessage *msg) const
        {
            if (msg)
            {
                gst_message_unref(msg);
            }
        }
    };

    /*
        Định nghĩa Functor struct
        Deleter cho BusWatchID
    */
    struct GstBusWatchIDDeleter
    {
        void operator()(guint *id) const
        {
            if (id)
            {
                g_source_remove(*id);
                delete id;
            }
        }
    };

    /*
        Định nghĩa Functor struct
        Deleter cho GList trả về từ gst_device_monitor_get_devices
    */
    struct GstDeviceMonitorList
    {
        void operator()(GList *devices) const
        {
            g_list_free_full(devices, gst_object_unref);
        }
    };

    /*
        Định nghĩa Functor struct
        Deleter cho DeviceMonitor: stop trước khi unref
    */
    struct GstDeviceMonitorDeleter
    {
        void operator()(GstDeviceMonitor *monitor) const
        {
            if (monitor)
            {
                gst_device_monitor_stop(monitor);
                gst_object_unref(GST_OBJECT(monitor));
            }
        }
    };

    /*
        Định nghĩa Functor struct
        Deleter cho gchar (g_free)
    */
    struct GCharDeleter
    {
        void operator()(gchar *str) const
        {
            if (str)
            {
                g_free(str);
            }
        }
    };

    /*
        Định nghĩa Functor struct
        Deleter cho GstCaps (gst_caps_unref)
    */
    struct GstCapsDeleter
    {
        void operator()(GstCaps *caps) const
        {
            if (caps)
            {
                gst_caps_unref(caps);
            }
        }
    };

    /*
        Định nghĩa Functor struct
        Deleter cho GstStructure (gst_structure_free)
        Chỉ dùng khi GstStructure được cấp phát độc lập (VD:
       gst_device_get_properties) Không dùng cho GstStructure lấy từ
       gst_caps_get_structure (bộ nhớ thuộc về GstCaps)
    */
    struct GstStructureDeleter
    {
        void operator()(GstStructure *structure) const
        {
            if (structure)
            {
                gst_structure_free(structure);
            }
        }
    };

    /*
        Định nghĩa Functor struct
        Deleter cho GstDevice (gst_object_unref)
    */
    struct GstDeviceDeleter
    {
        void operator()(GstDevice *device) const
        {
            if (device)
            {
                gst_object_unref(GST_OBJECT(device));
            }
        }
    };

    /*
        Định nghĩa Functor struct
        Deleter cho GMainLoop (g_main_loop_unref)
        CHÚ Ý: GMainLoop không phải GstObject nên phải dùng g_main_loop_unref,
        không dùng gst_object_unref.
    */
    struct GstMainLoopDeleter
    {
        void operator()(GMainLoop *loop) const
        {
            if (loop)
            {
                g_main_loop_unref(loop);
            }
        }
    };

    /*
        Định nghĩa Functor struct
        Deleter cho GstBus (gst_object_unref)
    */
    struct GstBusDeleter
    {
        void operator()(GstBus *bus) const
        {
            if (bus)
            {
                gst_object_unref(GST_OBJECT(bus));
            }
        }
    };

    /*
        Custom deleter cho GObject
    */
    using GstElementPtr           = std::unique_ptr<GstElement,       GstElementDeleter>;
    using GstBusPtr               = std::unique_ptr<GstBus,           GstBusDeleter>;
    using GstMessagePtr           = std::unique_ptr<GstMessage,       GstMessageDeleter>;
    using GstBusWatchIDPtr        = std::unique_ptr<guint,            GstBusWatchIDDeleter>;
    using GstDeviceMonitorPtr     = std::unique_ptr<GstDeviceMonitor, GstDeviceMonitorDeleter>;
    using GstDeviceMonitorListPtr = std::unique_ptr<GList,            GstDeviceMonitorList>;
    using GCharPtr                = std::unique_ptr<gchar,            GCharDeleter>;
    using GstCapsPtr              = std::unique_ptr<GstCaps,          GstCapsDeleter>;
    using GstStructurePtr         = std::unique_ptr<GstStructure,     GstStructureDeleter>;
    using GstDevicePtr            = std::unique_ptr<GstDevice,        GstDeviceDeleter>;
    using GstMainLoopPtr          = std::unique_ptr<GMainLoop,        GstMainLoopDeleter>;

    /*
        Cấu trúc chứa các Element cần thiết cho GStreamer Pipeline.
        video_source và video_sink là raw pointer vì pipeline (unique_ptr) nắm
        ownership sau gst_bin_add_many() — không cần unique_ptr riêng cho chúng.
    */
    struct
    {
        /* Pipeline cha */
        GstElementPtr pipeline;

        /* Các phần tử Video */
        GstElement *video_source    = nullptr;
        GstElement *video_sink      = nullptr;

        /* Các phần tử Audio */
        GstElement *audio_source    = nullptr;
        GstElement *audio_convert   = nullptr;
        GstElement *audio_resample  = nullptr;
        GstElement *audio_sink      = nullptr;

        /* Cấu hình pipeline đang active (dùng lại bởi Switch_Camera()) */
        Pipeline_Config active_config;
    } GStreamer_Pipeline_Structure;

    struct
    {
        /* Bus Watch ID */
        GstBusWatchIDPtr bus_watch_id;

        /* Main Loop */
        GstMainLoopPtr main_loop;
    } GStreamer_Event_Handler_Structure;

    // /* Các elements cha */
    // GstElementPtr m_pipeline;
    // GstBusPtr m_bus;

    // /* Các elements con */
    // GstElementPtr m_source;
    // GstElementPtr m_convert;
    // GstElementPtr m_sink;
    // GstMessagePtr m_msg;
    // GstBusWatchIDPtr m_bus_watch_id;
    // GstMainLoopPtr m_loop;

    /* Danh sách các thiết bị được tìm thấy từ hệ thống */
    std::vector<Devices_Information> m_Camera_Devices_List;

    /**
     * @brief Parse danh sách các thiết bị Camera từ GList.
     *
     * @return std::vector<Devices_Information> danh sách thiết bị được phân tích
     */
    std::vector<Devices_Information> Private_Parse_Devices_Information(const GstDeviceMonitorListPtr &arg_list_devices);

    /**
     * @brief Lấy danh sách thiết bị từ Monitor.
     *
     * @param arg_filter_devices String Filter loại thiết bị (VD: "Video/Source"
     * để lọc Camera, "Audio/Source" để lọc Audio)
     */
    void Private_Device_Monitor_Get_Devices(const std::string arg_filter_devices);

    /**
     * @brief Khởi tạo toàn bộ elements, link pipeline và thiết lập bus watch.
     *
     * @param arg_config  Cấu hình đầy đủ (camera index, source type, sink, format, resolution).
     * @return true nếu thành công, false nếu có lỗi.
     */
    bool Private_Build_Video_Pipeline(const Pipeline_Config &arg_config);

    /**
     * @brief Callback xử lý message EOS / ERROR từ GStreamer Bus.
     */
    static gboolean Private_Bus_Call(GstBus *bus, GstMessage *msg, gpointer data);

    /* Danh sách các signal → callback được đăng ký qua Set_Interrupt_Callback().
     * Mỗi signal chỉ xuất hiện tối đa một lần. */
    std::vector<std::pair<int, GSourceFunc>> m_interrupt_callbacks;
    GIOFunc m_keyboard_callback = nullptr;
};

/* ────────────────────────────────────────────────────────────────────────────
 * TRIỂN KHAI — Constructor / Destructor
 * ────────────────────────────────────────────────────────────────────────── */

/*
 */
GStreamer_Glue::GStreamer_Glue(const std::string arg_filter_devices)
{
    /* Đảm bảo GStreamer đã được khởi tạo */
    if (!gst_is_initialized())
    {
        gst_init(nullptr, nullptr);
    }
    else
    {
        g_printerr("Không thể khởi tạo!\n");
    }

    Private_Device_Monitor_Get_Devices(arg_filter_devices);
}

/*
 */
GStreamer_Glue::~GStreamer_Glue()
{
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

    /* Chỉ lọc lấy các thiết bị ghi hình (Video/Source) */
    gst_device_monitor_add_filter(local_device_monitor.get(), arg_filter_devices.c_str(), NULL);

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

    const std::string &camera_path  = m_Camera_Devices_List[arg_config.camera_index].Devices_Path;
    const char *source_plugin       = VideoSourceType_To_String(arg_config.source_type);
    const char *source_property     = Camera_Source_Property(arg_config.source_type);
    const char *sink_plugin         = VideoSinkType_To_String(arg_config.sink_type);

    /* 1. Tạo Pipeline cha */
    GStreamer_Pipeline_Structure.pipeline.reset(gst_pipeline_new("camera-preview-pipeline"));
    if (!GStreamer_Pipeline_Structure.pipeline)
    {
        g_printerr("[GStreamer_Glue] Lỗi: Không thể tạo pipeline.\n");
        return false;
    }

    /* 2. Tạo các Elements con
     * filter, queue1, conv, queue2 chỉ cần trong giai đoạn khởi tạo:
     * sau gst_bin_add_many(), pipeline nắm ownership → dùng block cục bộ. */
    {
        GstElement *filter, *queue1, *conv, *queue2;

        GStreamer_Pipeline_Structure.video_source = gst_element_factory_make(source_plugin, "video-source");
        filter                                    = gst_element_factory_make("capsfilter",   "caps-filter");
        queue1                                    = gst_element_factory_make("queue",        "queue-1");
        conv                                      = gst_element_factory_make("videoconvert", "video-converter");
        queue2                                    = gst_element_factory_make("queue",        "queue-2");
        GStreamer_Pipeline_Structure.video_sink   = gst_element_factory_make(sink_plugin,    "display-sink");

        if (!GStreamer_Pipeline_Structure.video_source || !filter || !queue1 ||
            !conv || !queue2 || !GStreamer_Pipeline_Structure.video_sink)
        {
            g_printerr("[GStreamer_Glue] Lỗi: Không thể tạo một vài elements.\n"
                       "  source=%s  sink=%s — kiểm tra plugins đã cài đầy đủ chưa.\n",
                       source_plugin, sink_plugin);
            return false;
        }

        /* Gán camera theo đường dẫn đầy đủ.
         * libcamerasrc dùng "camera-name"; v4l2src dùng "device". */
        g_object_set(G_OBJECT(GStreamer_Pipeline_Structure.video_source),
                     source_property, camera_path.c_str(), NULL);
        g_print("[GStreamer_Glue] Nguồn: %-14s property=%-12s → %s\n",
                source_plugin, source_property, camera_path.c_str());
        g_print("[GStreamer_Glue] Sink  : %s\n", sink_plugin);

        /* 3. Xây dựng Caps từ Pipeline_Config.
         * Nếu framerate_n == 0: không ép framerate (camera tự chọn).
         * caps unref tự động khi ra khỏi block. */
        {
            GstCapsPtr caps;
            if (arg_config.framerate_n > 0)
            {
                caps.reset(gst_caps_new_simple("video/x-raw",
                                               "format",    G_TYPE_STRING,   arg_config.pixel_format.c_str(),
                                               "width",     G_TYPE_INT,      arg_config.width,
                                               "height",    G_TYPE_INT,      arg_config.height,
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
            g_object_set(G_OBJECT(filter), "caps", caps.get(), NULL);
        }

        /* 4. Thêm tất cả elements vào pipeline */
        gst_bin_add_many(GST_BIN(GStreamer_Pipeline_Structure.pipeline.get()),
                         GStreamer_Pipeline_Structure.video_source, filter,
                         queue1, conv, queue2,
                         GStreamer_Pipeline_Structure.video_sink, NULL);

        /* 5. Liên kết các elements */
        if (!gst_element_link_many(GStreamer_Pipeline_Structure.video_source, filter,
                                   queue1, conv, queue2,
                                   GStreamer_Pipeline_Structure.video_sink, NULL))
        {
            g_printerr("[GStreamer_Glue] Lỗi: Không thể liên kết các elements.\n");
            return false;
        }
    } /* filter, queue1, conv, queue2 hết phạm vi nhưng vẫn sống trong pipeline */

    /* 6. Thiết lập Bus watch
     * bus chỉ cần trong block này; watch_id được lưu vào struct. */
    {
        GstBusPtr bus(gst_element_get_bus(GStreamer_Pipeline_Structure.pipeline.get()));
        guint *watch_id = new guint(gst_bus_add_watch(bus.get(), Private_Bus_Call, this));
        GStreamer_Event_Handler_Structure.bus_watch_id.reset(watch_id);
    }

    /* 7. Tạo Main Loop */
    GStreamer_Event_Handler_Structure.main_loop.reset(g_main_loop_new(NULL, FALSE));

    return true;
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

    /* Blocking cho đến khi Stop_Preview() được gọi */
    g_main_loop_run(GStreamer_Event_Handler_Structure.main_loop.get());

    /* Sau khi main loop kết thúc: đặt pipeline về NULL để giải phóng tài nguyên */
    gst_element_set_state(GStreamer_Pipeline_Structure.pipeline.get(), GST_STATE_NULL);

    return true;
}

/*
 */
bool GStreamer_Glue::Start_Preview(int arg_camera_index, VideoSourceType arg_source_type)
{
    Pipeline_Config config;
    config.camera_index = arg_camera_index;
    config.source_type  = arg_source_type;
    return Start_Preview(config);
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

/*******************************************************************************
 * HÀM MAIN - TEST LIVE PREVIEW & SWITCH CAMERA
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

    /* ── TEST 1: Default config (backward-compat) ─────────────────────────── */
    printf("==============================================\n");
    printf("🚀 TEST 1: Start_Preview(int, VideoSourceType) — backward compat\n");
    printf("   → Camera 0, LibCamera, NV12, 640x480, glimagesink\n");
    printf("==============================================\n");

    gstreamer_glue.Set_Interrupt_Callback({SIGINT,  GStreamer_Glue::Default_Interrupt_Handler});
    gstreamer_glue.Set_Interrupt_Callback({SIGTERM, GStreamer_Glue::Default_Interrupt_Handler});
    gstreamer_glue.Set_Keyboard_Callback(GStreamer_Glue::Default_Keyboard_Handler);

    bool result = gstreamer_glue.Start_Preview(0, VideoSourceType::LibCamera);

    printf("==============================================\n");
    printf("%s TEST 1 %s.\n", result ? "✅" : "❌", result ? "thành công" : "thất bại");
    printf("==============================================\n\n");

    /* ── TEST 2: Pipeline_Config tường minh — FakeSink (headless) ──────────── */
    printf("==============================================\n");
    printf("🚀 TEST 2: Pipeline_Config — FakeSink (headless)\n");
    printf("   → Camera 0, LibCamera, NV12, 640x480, fakesink\n");
    printf("==============================================\n");

    {
        Pipeline_Config config_fake;
        config_fake.camera_index = 0;
        config_fake.source_type  = VideoSourceType::LibCamera;
        config_fake.pixel_format = "NV12";
        config_fake.width        = 640;
        config_fake.height       = 480;
        config_fake.framerate_n  = 0;   /* không ép framerate */
        config_fake.sink_type    = VideoSinkType::FakeSink;

        /* fakesink không hiển thị — tự thoát sau 3 giây qua SIGALRM không có ở đây.
         * Dùng Ctrl+C để thoát thủ công khi test. */
        bool result2 = gstreamer_glue.Start_Preview(config_fake);
        printf("==============================================\n");
        printf("%s TEST 2 %s.\n", result2 ? "✅" : "❌", result2 ? "thành công" : "thất bại");
        printf("==============================================\n\n");
    }

    printf("==============================================\n");
    printf("✅ Tất cả test đã hoàn tất.\n");
    printf("==============================================\n");

    return 0;
}

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