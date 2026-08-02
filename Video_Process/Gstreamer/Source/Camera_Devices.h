/**
 * @file Camera_Devices.h
 * @brief GStreamer Camera Pipeline Wrapper for LVGL Simulator
 *
 * File này chứa toàn bộ khai báo (declarations) của các kiểu dữ liệu,
 * struct, enum và class GStreamer_Glue — phần triển khai (implementation)
 * nằm trong Camera_Devices.cpp.
 */

#ifndef CAMERA_DEVICES_H
#define CAMERA_DEVICES_H

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <glib.h>
#include <glib-unix.h>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <mutex>
#include <atomic>
#include <cstdint>

/* ─────────────────────────────────────────────────────────────────────────────
 * Enum: Loại nguồn & sink video
 * ───────────────────────────────────────────────────────────────────────── */

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
    GlImageSink,    /* glimagesink   — OpenGL window (mặc định, live preview)         */
    AutoVideoSink,  /* autovideosink — tự chọn sink phù hợp với hệ thống           */
    FakeSink,       /* fakesink      — không hiển thị, dùng để test headless       */
    AppSink,        /* appsink       — nhận raw frame qua callback (LVGL / record) */
};

/**
 * @brief Loại element dùng để chuyển đổi định dạng pixel giữa source và sink.
 *
 * Chọn đúng loại phù hợp với phần cứng để giảm tải CPU.
 */
enum class VideoConverterType
{
    VideoConvert,  /* videoconvert — software, chạy trên CPU (ARM NEON). Tương thích mọi hệ thống. */
    V4L2Convert,   /* v4l2convert  — hardware M2M (Pi 5 PiSP converter). Giảm tải CPU convert. */
};

/* ─────────────────────────────────────────────────────────────────────────────
 * Struct: Thông tin thiết bị
 * ───────────────────────────────────────────────────────────────────────── */

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

/* ─────────────────────────────────────────────────────────────────────────────
 * Frame_Callback — Signature của callback nhận frame từ appsink
 * ───────────────────────────────────────────────────────────────────────── */

/**
 * @brief Signature của callback nhận frame từ appsink.
 *
 * ⚠️ QUAN TRỌNG — LUỒNG THỰC THI: callback này được GStreamer gọi trên
 * streaming thread nội bộ của pipeline (thread của queue/videoconvert phía
 * trước appsink), KHÔNG PHẢI thread đã gọi Start_Preview() và CHẮC CHẮN
 * không phải thread đang chạy LVGL. Tuyệt đối KHÔNG được gọi bất kỳ hàm
 * lv_...() nào trực tiếp bên trong callback này (LVGL không thread-safe) —
 * làm vậy sẽ gây lỗi ngầm dạng crash/vỡ hình ảnh ngẫu nhiên rất khó debug.
 * Nếu cần đưa frame sang LVGL, hãy dùng Try_Get_Latest_Frame() (an toàn đa
 * luồng, gọi định kỳ từ thread LVGL) thay vì xử lý trực tiếp ở đây.
 *
 * @param arg_data    Con trỏ tới dữ liệu pixel thô (định dạng cố định theo
 *                    Pipeline_Config::appsink_pixel_format khi sink_type ==
 *                    AppSink — KHÔNG dùng chung với pixel_format của camera).
 * @param arg_size    Kích thước THẬT (byte) của arg_data, lấy từ GstMapInfo.
 *                    Luôn dùng giá trị này thay vì tự suy ra từ width*height,
 *                    vì hai giá trị này có thể không khớp nếu cấu hình sai.
 * @param arg_width   Chiều rộng frame (pixel).
 * @param arg_height  Chiều cao frame (pixel).
 * @param arg_user_data  Con trỏ người dùng truyền vào Pipeline_Config.
 */
using Frame_Callback = std::function<void(const uint8_t *arg_data,
                                          size_t arg_size,
                                          int arg_width, int arg_height,
                                          gpointer arg_user_data)>;

/* ─────────────────────────────────────────────────────────────────────────────
 * Struct: Cấu hình Pipeline
 * ───────────────────────────────────────────────────────────────────────── */

struct Pipeline_Config
{
    /* ── Nguồn ──────────────────────────────────────────────────────── */
    VideoSourceType source_type  = VideoSourceType::LibCamera;
    int             camera_index = 0;

    /* ── Định dạng video (phía CAMERA / nguồn) ───────────────────────────
     * Đây là định dạng capsfilter sẽ ép camera phải xuất ra (VD: NV12,
     * YUY2...). KHÔNG liên quan tới định dạng appsink sẽ gửi cho callback —
     * xem appsink_pixel_format bên dưới. */
    std::string     pixel_format = "NV12";  /* NV12, RGBA, I420... */
    int             width        = 640;
    int             height       = 480;
    int             framerate_n  = 0;       /* 0/1 = không ép framerate */
    int             framerate_d  = 1;

    /* ── Sink ────────────────────────────────────────────────────────── */
    VideoSinkType   sink_type    = VideoSinkType::GlImageSink;

    /* ── AppSink (chỉ hiệu lực khi sink_type == AppSink) ─────────────── */
    /* [FIX] Định dạng dữ liệu appsink sẽ TRẢ VỀ qua Frame_Callback /
     * Try_Get_Latest_Frame() — khác với pixel_format (định dạng camera).
     * videoconvert trong pipeline sẽ tự chuyển đổi từ pixel_format sang
     * định dạng này. PHẢI khớp với LV_COLOR_DEPTH bên phía LVGL:
     *   - LV_COLOR_DEPTH == 16 (RGB565)         → "RGB16"
     *   - LV_COLOR_DEPTH == 32, lv_color32_t     → "BGRA" (thứ tự byte
     *     {blue,green,red,alpha} của LVGL khớp đúng với "BGRA" của GStreamer)
     * Mặc định "RGB16" vì đa số màn hình SPI/TFT nhúng dùng LVGL ở 16-bit.
     * Nếu để trống, GStreamer_Glue sẽ tự dùng lại "RGB16". */
    std::string     appsink_pixel_format = "RGB16";
    /* Callback được gọi mỗi khi có frame mới từ appsink — xem cảnh báo về
     * luồng thực thi trong Doxygen của Frame_Callback ở trên.
     * nullptr = không dùng đường callback thô (vẫn có thể dùng
     * Try_Get_Latest_Frame() bình thường). */
    Frame_Callback  appsink_frame_callback = nullptr;
    gpointer        appsink_user_data      = nullptr;
    /* Số frame tối đa trong queue. Drop frame cũ khi đầy (tránh lag). */
    int             appsink_max_buffers    = 1;

    /* ── Tối ưu CPU ──────────────────────────────────────────────── */
    /* [CPU] Dùng v4l2convert (hardware M2M Pi 5) thay videoconvert (software CPU).
     * Pipeline: ... ! queue ! v4l2convert ! video/x-raw,format=RGB16 ! queue ! appsink
     * Nếu VideoConvert (mặc định): dùng videoconvert (software, tích hợp với mọi
     * pipeline khác). Nếu v4l2convert không khả dụng trên hệ thống, pipeline
     * sẽ fail ở bước link. */
    VideoConverterType converter_type      = VideoConverterType::VideoConvert;

    /* [CPU] Giới hạn FPS gửi xuống converter/appsink qua `videorate`.
     * Camera vẫn capture ở framerate_n/framerate_d, nhưng chỉ N frames/giây
     * đi qua videoconvert (phần tốn CPU nhất) rồi vào LVGL canvas.
     * Ví dụ: camera=30fps, display_max_fps=15 → giảm 50% CPU convert.
     * 0 = không giới hạn (dùng toàn bộ framerate của camera). */
    int             display_max_fps        = 0;
};

/* ─────────────────────────────────────────────────────────────────────────────
 * Class: GStreamer_Glue
 * ───────────────────────────────────────────────────────────────────────── */

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
     *
     * ⚠️ AN TOÀN LUỒNG: nếu Start_Preview() đang chạy trên một thread khác
     * (mô hình bắt buộc khi tích hợp LVGL, vì Start_Preview() blocking), PHẢI
     * gọi Stop_Preview() rồi join() thread đó xong TRƯỚC KHI để object này ra
     * khỏi scope. Hủy object trong khi Start_Preview() vẫn chạy là use-after-
     * free tiềm ẩn; destructor sẽ in cảnh báo nếu phát hiện vi phạm (xem
     * Is_Preview_Running()) nhưng không thể tự chờ thay caller.
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
     * @brief Dừng pipeline và thoát khỏi main loop.
     */
    void Stop_Preview();

    /**
     * @brief Chuyển sang camera tiếp theo trong danh sách (round-robin).
     *
     * Dừng pipeline hiện tại, cập nhật chỉ số camera, rồi khởi động lại pipeline.
     *
     * An toàn khi gọi từ thread khác với thread đang chạy Start_Preview()
     * (VD: gọi từ callback nút bấm trên thread LVGL) — GStreamer tự khóa nội
     * bộ cho gst_element_set_state()/g_object_set(). Lưu ý: nếu camera tiếp
     * theo không hỗ trợ đúng pixel_format/width/height đã cấu hình, pipeline
     * sẽ báo lỗi qua Bus (xem Private_Bus_Call) và toàn bộ preview sẽ dừng —
     * hàm này không tự dò lại caps phù hợp cho từng camera khác nhau.
     */
    void Switch_Camera();

    /**
     * @brief [MỚI] Lấy bản sao của frame mới nhất, an toàn khi gọi từ thread khác.
     *
     * Khác với Frame_Callback (chạy trên GStreamer streaming thread),
     * hàm này AN TOÀN để gọi định kỳ từ thread đang chạy LVGL (VD: trong một
     * lv_timer 30–60Hz). Dữ liệu trả về có định dạng cố định theo
     * Pipeline_Config::appsink_pixel_format (mặc định RGB565 "RGB16").
     *
     * @param arg_out     [out] Buffer nhận dữ liệu, tự resize nếu cần.
     * @param arg_width   [out] Chiều rộng frame hiện có trong arg_out.
     * @param arg_height  [out] Chiều cao frame hiện có trong arg_out.
     * @return true nếu có frame MỚI kể từ lần gọi trước (đã copy vào arg_out);
     *         false nếu chưa có frame nào hoặc chưa có frame mới hơn.
     */
    bool Try_Get_Latest_Frame(std::vector<uint8_t> &arg_out, int &arg_width, int &arg_height);

    /**
     * @brief [MỚI] Start_Preview() có đang chạy (trên thread nào đó) hay không.
     *
     * Dùng để kiểm tra trước khi hủy object: PHẢI đảm bảo thread đã gọi
     * Start_Preview() đã join() xong (sau khi gọi Stop_Preview()) trước khi
     * để GStreamer_Glue ra khỏi scope — destructor sẽ in cảnh báo nếu phát
     * hiện vi phạm nhưng không thể tự chờ thay cho caller (có nguy cơ deadlock).
     */
    bool Is_Preview_Running() const { return m_is_preview_running.load(); }

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

    /*
        [MỚI] Ngữ cảnh truyền vào callback new_sample của AppSink.
        Là thành viên của từng instance (KHÔNG static) để mỗi GStreamer_Glue
        có vùng nhớ callback riêng — an toàn khi chạy nhiều instance song song
        (VD: 2 camera trước/sau trên dashcam, mỗi camera một thread riêng).
        Địa chỉ &m_appsink_callback_context ổn định trong suốt vòng đời
        object nên an toàn truyền cho gst_app_sink_set_callbacks() làm user_data.
    */
    struct AppSink_Callback_Context
    {
        GStreamer_Glue  *self   = nullptr;
        Pipeline_Config *config = nullptr;
    };
    AppSink_Callback_Context m_appsink_callback_context;
    GstAppSinkCallbacks      m_appsink_callbacks{};

    /*
        [MỚI] Buffer chứa frame mới nhất, bảo vệ bằng mutex.
        GStreamer streaming thread ghi (trong new_sample callback);
        thread LVGL/UI đọc ra bằng Try_Get_Latest_Frame(). Đây là đường AN
        TOÀN để đưa frame sang LVGL — không như Frame_Callback chạy thẳng
        trên streaming thread.
    */
    std::mutex            m_latest_frame_mutex;
    std::vector<uint8_t>  m_latest_frame_data;
    int                   m_latest_frame_width   = 0;
    int                   m_latest_frame_height  = 0;
    bool                  m_latest_frame_updated = false;

    /*
        [MỚI] true trong khoảng thời gian từ lúc Start_Preview() gọi
        g_main_loop_run() tới khi hàm đó return. Dùng để destructor phát hiện
        và cảnh báo nếu object bị hủy trong khi vòng lặp vẫn đang chạy trên
        thread khác (nguồn gốc kinh điển của use-after-free khi tích hợp đa
        luồng với LVGL).
    */
    std::atomic<bool> m_is_preview_running{false};

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
     * @brief Struct trung gian chứa các GstElement tạm (filter, queues, conv).
     *
     * Chỉ dùng trong quá trình build pipeline. Sau gst_bin_add_many() pipeline
     * nắm ownership toàn bộ raw pointers bên trong — không cần giải phóng thủ công.
     */
    struct Pipeline_Elements
    {
        GstElement *filter       = nullptr;
        GstElement *queue1       = nullptr;
        GstElement *conv         = nullptr;
        GstElement *queue2       = nullptr;
        /* [CPU] Tùy chọn: giới hạn FPS trước converter */
        GstElement *rate         = nullptr;  /* videorate element */
        GstElement *rate_filter  = nullptr;  /* capsfilter gắn FPS target */
        int         display_max_fps = 0;     /* copy từ Pipeline_Config.display_max_fps */
    };

    /**
     * @brief Khởi tạo toàn bộ elements, link pipeline và thiết lập bus watch.
     *
     * Hàm coordinator: validate → tạo pipeline cha → gọi các Private_Step_*.
     *
     * @param arg_config  Cấu hình đầy đủ (camera index, source type, sink, format, resolution).
     * @return true nếu thành công, false nếu có lỗi.
     */
    bool Private_Build_Video_Pipeline(const Pipeline_Config &arg_config);

    /**
     * @brief [Step 1] Tạo tất cả GstElement, gán camera path vào video_source.
     *
     * @param arg_config   Cấu hình pipeline.
     * @param arg_out      Output: các element tạm (filter, queue1, conv, queue2).
     * @return true nếu tạo thành công tất cả elements.
     */
    bool Private_Step_Create_Elements(const Pipeline_Config &arg_config,
                                      Pipeline_Elements &arg_out);

    /**
     * @brief [Step 2] Tạo GstCaps từ Pipeline_Config và gán vào capsfilter.
     *
     * Nếu framerate_n == 0: caps không ép framerate (camera tự chọn).
     *
     * @param arg_config  Cấu hình pipeline.
     * @param arg_filter  Con trỏ tới capsfilter element.
     */
    void Private_Step_Build_Caps(const Pipeline_Config &arg_config, GstElement *arg_filter);

    /**
     * @brief [Step 3] Thêm elements vào pipeline bin và link theo thứ tự.
     *
     * @param arg_elems  Các element tạm cần add+link.
     * @return true nếu link thành công.
     */
    bool Private_Step_Add_And_Link(const Pipeline_Elements &arg_elems);

    /**
     * @brief [Step 4] Cấu hình sink theo loại: switch-case trên VideoSinkType.
     *
     * - AppSink: set max-buffers/drop/sync và đăng ký GstAppSinkCallbacks.
     * - Các loại khác: không cần cấu hình thêm.
     *
     * @param arg_config  Cấu hình pipeline (dùng sink_type, appsink_*).
     */
    void Private_Step_Configure_Sink(const Pipeline_Config &arg_config);

    /**
     * @brief [Step 5] Thiết lập GstBus watch và tạo GMainLoop.
     */
    void Private_Step_Setup_Bus_And_Loop();

    /**
     * @brief Callback xử lý message EOS / ERROR từ GStreamer Bus.
     */
    static gboolean Private_Bus_Call(GstBus *bus, GstMessage *msg, gpointer data);

    /* Danh sách các signal → callback được đăng ký qua Set_Interrupt_Callback().
     * Mỗi signal chỉ xuất hiện tối đa một lần. */
    std::vector<std::pair<int, GSourceFunc>> m_interrupt_callbacks;
    GIOFunc m_keyboard_callback = nullptr;
};

#endif /* CAMERA_DEVICES_H */
