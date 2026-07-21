# Hướng Dẫn Sử Dụng GStreamer trong C

> Các khái niệm Element/Pad/Bus/State nhắc tới trong file này được giải thích chi tiết ở [`01-kien-truc-va-thiet-ke.md`](./01-kien-truc-va-thiet-ke.md).

## 1. Cài đặt môi trường phát triển

### Ubuntu / Debian

```bash
sudo apt update
sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav gstreamer1.0-tools \
    gstreamer1.0-x gstreamer1.0-alsa pkg-config build-essential
```

### Fedora

```bash
sudo dnf install gstreamer1-devel gstreamer1-plugins-base-devel \
    gstreamer1-plugins-good gstreamer1-plugins-bad-free \
    gstreamer1-plugins-ugly-free gstreamer1-libav \
    gcc pkgconf-pkg-config
```

### macOS (Homebrew)

```bash
brew install gstreamer gst-plugins-base gst-plugins-good \
    gst-plugins-bad gst-plugins-ugly gst-libav pkg-config
```

### Windows

Dùng bộ cài đặt chính thức (runtime + development) từ trang tải GStreamer, hoặc MSYS2:

```bash
pacman -S mingw-w64-x86_64-gstreamer
```

### Kiểm tra cài đặt

```bash
gst-launch-1.0 --version
gst-inspect-1.0 videotestsrc      # xem chi tiết 1 element cụ thể
pkg-config --modversion gstreamer-1.0
```

## 2. Biên dịch chương trình C với GStreamer

GStreamer cung cấp file `.pc` cho `pkg-config`, tự động lấy đúng flag biên dịch/liên kết:

```bash
gcc -o my_app my_app.c $(pkg-config --cflags --libs gstreamer-1.0)
```

Cần thêm module riêng khi dùng API chuyên biệt, ví dụ `appsrc`/`appsink`:

```bash
gcc -o my_app my_app.c $(pkg-config --cflags --libs gstreamer-1.0 gstreamer-app-1.0)
```

Makefile mẫu:

```makefile
CC = gcc
CFLAGS = $(shell pkg-config --cflags gstreamer-1.0)
LIBS = $(shell pkg-config --libs gstreamer-1.0)

my_app: my_app.c
	$(CC) -o $@ $< $(CFLAGS) $(LIBS)

clean:
	rm -f my_app
```

## 3. Chương trình đầu tiên: pipeline bằng chuỗi mô tả

Cách nhanh nhất dựng pipeline là `gst_parse_launch()` — nhận chuỗi có cú pháp giống công cụ dòng lệnh `gst-launch-1.0`.

```c
#include <gst/gst.h>

int main(int argc, char *argv[]) {
    GstElement *pipeline;
    GstBus *bus;
    GstMessage *msg;

    gst_init(&argc, &argv);   /* Bắt buộc: khởi tạo trước mọi lời gọi GStreamer khác */

    /* Dựng pipeline: nguồn kiểm tra hình -> chuyển đổi màu -> hiển thị */
    pipeline = gst_parse_launch(
        "videotestsrc ! videoconvert ! autovideosink", NULL);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    /* Chặn (block) luồng hiện tại cho đến khi có lỗi hoặc hết luồng (EOS) */
    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    if (msg != NULL)
        gst_message_unref(msg);

    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);   /* luôn đưa về NULL để giải phóng tài nguyên */
    gst_object_unref(pipeline);

    return 0;
}
```

Mẹo: trước khi viết code, luôn thử chuỗi pipeline trực tiếp bằng `gst-launch-1.0` trên terminal:

```bash
gst-launch-1.0 videotestsrc ! videoconvert ! autovideosink
```

Nếu chuỗi này chạy đúng ngoài terminal thì `gst_parse_launch()` với cùng chuỗi hầu như chắc chắn cũng chạy đúng trong code.

## 4. Hai cách xây dựng pipeline

**Cách 1 — chuỗi mô tả** (`gst_parse_launch`): nhanh, gọn, phù hợp khi cấu trúc pipeline cố định, biết trước lúc viết code (mục 3 ở trên).

**Cách 2 — xây dựng thủ công từng element**: cần khi phải giữ con trỏ tới từng element để chỉnh thuộc tính lúc chạy, phải xử lý pad động, hoặc cấu trúc pipeline thay đổi tuỳ điều kiện runtime.

```c
#include <gst/gst.h>

int main(int argc, char *argv[]) {
    GstElement *pipeline, *source, *convert, *sink;
    GstBus *bus;
    GstMessage *msg;
    GstStateChangeReturn ret;

    gst_init(&argc, &argv);

    /* Tạo từng element riêng lẻ qua factory */
    source  = gst_element_factory_make("audiotestsrc", "source");
    convert = gst_element_factory_make("audioconvert", "convert");
    sink    = gst_element_factory_make("autoaudiosink", "sink");

    pipeline = gst_pipeline_new("test-pipeline");

    if (!pipeline || !source || !convert || !sink) {
        g_printerr("Không thể tạo một trong các element.\n");
        return -1;
    }

    /* Đưa element vào pipeline (pipeline là một GstBin) */
    gst_bin_add_many(GST_BIN(pipeline), source, convert, sink, NULL);

    /* Liên kết element theo đúng thứ tự dữ liệu chảy qua */
    if (gst_element_link_many(source, convert, sink, NULL) != TRUE) {
        g_printerr("Liên kết element thất bại.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    /* Chỉnh thuộc tính (property) của element qua GObject API */
    g_object_set(source, "freq", 440.0, NULL);

    ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Không thể chuyển sang PLAYING.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    if (msg != NULL) {
        GError *err;
        gchar *debug_info;

        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                g_printerr("Lỗi từ %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
                g_printerr("Chi tiết debug: %s\n", debug_info ? debug_info : "không có");
                g_clear_error(&err);
                g_free(debug_info);
                break;
            case GST_MESSAGE_EOS:
                g_print("Đã đến cuối luồng (EOS).\n");
                break;
            default:
                g_printerr("Message không mong đợi.\n");
                break;
        }
        gst_message_unref(msg);
    }

    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}
```

## 5. Bus không chặn luồng với `GMainLoop`

Cách dùng `gst_bus_timed_pop_filtered()` ở trên **chặn (block)** luồng gọi — chỉ phù hợp chương trình dòng lệnh đơn giản. Với ứng dụng thực tế (đặc biệt có giao diện), nên dùng `GMainLoop` kết hợp callback không chặn:

```c
#include <gst/gst.h>

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer user_data) {
    GMainLoop *loop = (GMainLoop *) user_data;

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("Kết thúc luồng.\n");
            g_main_loop_quit(loop);
            break;

        case GST_MESSAGE_ERROR: {
            GError *err;
            gchar *debug;
            gst_message_parse_error(msg, &err, &debug);
            g_printerr("Lỗi: %s\n", err->message);
            g_clear_error(&err);
            g_free(debug);
            g_main_loop_quit(loop);
            break;
        }

        case GST_MESSAGE_STATE_CHANGED: {
            GstState old_state, new_state;
            gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
            g_print("Trạng thái đổi: %s -> %s\n",
                gst_element_state_get_name(old_state),
                gst_element_state_get_name(new_state));
            break;
        }

        default:
            break;
    }
    return TRUE;   /* trả TRUE để tiếp tục nhận message tiếp theo */
}

int main(int argc, char *argv[]) {
    GstElement *pipeline;
    GstBus *bus;
    GMainLoop *loop;
    guint bus_watch_id;

    gst_init(&argc, &argv);
    loop = g_main_loop_new(NULL, FALSE);

    pipeline = gst_parse_launch("videotestsrc ! videoconvert ! autovideosink", NULL);

    bus = gst_element_get_bus(pipeline);
    bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    g_main_loop_run(loop);   /* chạy vòng lặp sự kiện, không chặn cứng như cách trên */

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_source_remove(bus_watch_id);
    g_main_loop_unref(loop);

    return 0;
}
```

`gst_bus_add_watch()` tích hợp bus vào vòng lặp sự kiện GLib — cách được khuyến nghị khi ứng dụng còn xử lý các nguồn sự kiện khác (input, timer, mạng...) song song với pipeline.

## 6. Pad động — ví dụ với `uridecodebin`

`uridecodebin`/`decodebin` không tạo src pad ngay lúc khởi tạo vì chưa biết trước file có bao nhiêu luồng (track) audio/video/phụ đề. Phải lắng nghe signal `"pad-added"` rồi tự liên kết khi pad xuất hiện.

```c
#include <gst/gst.h>

typedef struct {
    GstElement *pipeline;
    GstElement *convert;
    GstElement *sink;
} AppData;

static void on_pad_added(GstElement *src, GstPad *new_pad, AppData *data) {
    GstPad *sink_pad = gst_element_get_static_pad(data->convert, "sink");
    GstCaps *caps = NULL;
    GstStructure *str = NULL;
    const gchar *type = NULL;

    g_print("Pad mới xuất hiện: %s\n", GST_PAD_NAME(new_pad));

    if (gst_pad_is_linked(sink_pad)) {
        goto done;   /* đã nối rồi thì bỏ qua (decodebin có thể gọi lại nhiều lần) */
    }

    caps = gst_pad_get_current_caps(new_pad);
    str  = gst_caps_get_structure(caps, 0);
    type = gst_structure_get_name(str);

    /* Ví dụ này chỉ nối pad audio, bỏ qua video/phụ đề */
    if (!g_str_has_prefix(type, "audio/x-raw")) {
        g_print("Bỏ qua pad kiểu %s (không phải audio).\n", type);
        goto done;
    }

    if (GST_PAD_LINK_FAILED(gst_pad_link(new_pad, sink_pad))) {
        g_printerr("Liên kết pad động thất bại.\n");
    }

done:
    if (caps) gst_caps_unref(caps);
    gst_object_unref(sink_pad);
}

int main(int argc, char *argv[]) {
    AppData data;
    GstElement *source;
    GstBus *bus;
    GstMessage *msg;

    gst_init(&argc, &argv);

    source        = gst_element_factory_make("uridecodebin", "source");
    data.convert  = gst_element_factory_make("audioconvert", "convert");
    data.sink     = gst_element_factory_make("autoaudiosink", "sink");
    data.pipeline = gst_pipeline_new("dynamic-pipeline");

    if (!data.pipeline || !source || !data.convert || !data.sink) {
        g_printerr("Không thể tạo element.\n");
        return -1;
    }

    gst_bin_add_many(GST_BIN(data.pipeline), source, data.convert, data.sink, NULL);

    /* Chỉ nối trước convert -> sink; source sẽ nối vào convert LÚC CHẠY qua callback */
    if (!gst_element_link(data.convert, data.sink)) {
        g_printerr("Không thể nối convert -> sink.\n");
        return -1;
    }

    g_object_set(source, "uri",
        "https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm", NULL);

    g_signal_connect(source, "pad-added", G_CALLBACK(on_pad_added), &data);

    gst_element_set_state(data.pipeline, GST_STATE_PLAYING);

    bus = gst_element_get_bus(data.pipeline);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
    if (msg) gst_message_unref(msg);

    gst_object_unref(bus);
    gst_element_set_state(data.pipeline, GST_STATE_NULL);
    gst_object_unref(data.pipeline);

    return 0;
}
```

## 7. Truy vấn vị trí, thời lượng và tua (seek)

```c
gint64 position, duration;

if (gst_element_query_position(pipeline, GST_FORMAT_TIME, &position) &&
    gst_element_query_duration(pipeline, GST_FORMAT_TIME, &duration)) {
    g_print("Vị trí: %" GST_TIME_FORMAT " / %" GST_TIME_FORMAT "\n",
        GST_TIME_ARGS(position), GST_TIME_ARGS(duration));
}

/* Tua tới giây thứ 30 */
gst_element_seek_simple(pipeline, GST_FORMAT_TIME,
    GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, 30 * GST_SECOND);
```

`GST_SEEK_FLAG_FLUSH` xả sạch dữ liệu cũ còn trong pipeline trước khi tua (tránh phát lẫn dữ liệu trước/sau điểm tua); `GST_SEEK_FLAG_KEY_UNIT` yêu cầu tua tới keyframe gần nhất để tránh phải giải mã lại từ đầu GOP.

## 8. Gỡ lỗi

Biến môi trường `GST_DEBUG` bật log chi tiết theo category và mức độ (1=ERROR … 9=MEMDUMP):

```bash
GST_DEBUG=3 ./my_app
GST_DEBUG=videodecoder:5,*:2 ./my_app   # mức 5 riêng cho videodecoder, mức 2 cho phần còn lại
```

Xuất sơ đồ pipeline thực tế dạng `.dot` (mở bằng Graphviz) để trực quan hoá liên kết và caps đã thương lượng:

```bash
export GST_DEBUG_DUMP_DOT_DIR=/tmp/gst-dot
./my_app
dot -Tpng /tmp/gst-dot/*.dot -o pipeline.png
```

Dùng `gst-launch-1.0 -v` để in ra caps thương lượng ở từng điểm nối khi thử pipeline ngoài terminal — nên làm bước này trước khi viết code.

## 9. `appsrc` / `appsink` — đưa dữ liệu tuỳ ý vào/ra pipeline

Khi ứng dụng cần tự cấp dữ liệu (ví dụ frame ảnh tạo bằng thuật toán riêng) hoặc tự lấy dữ liệu đã xử lý ra để dùng tiếp (ví dụ đưa vào mô hình AI), dùng hai element đặc biệt:

- **`appsrc`**: gọi `gst_app_src_push_buffer()` để "bơm" `GstBuffer` tự tạo vào đầu pipeline.
- **`appsink`**: gọi `gst_app_sink_pull_sample()` (hoặc lắng nghe signal `"new-sample"`) để lấy dữ liệu đã xử lý ra khỏi pipeline.

Hai element này cần module riêng khi biên dịch: `pkg-config --cflags --libs gstreamer-app-1.0`. Chi tiết các hàm liên quan xem [`03-tai-lieu-api.md`](./03-tai-lieu-api.md).

## 10. Ví dụ hoàn chỉnh: trình phát media tối giản dùng `playbin`

Với nhu cầu phát media "tiêu chuẩn" (đọc URI, tự động chọn demux/decode/sink phù hợp), GStreamer cung cấp `playbin` — element cấp cao tự lắp ráp toàn bộ pipeline con bên trong, nên phần lớn ứng dụng phát media không cần tự tay xây pipeline chi tiết:

```c
#include <gst/gst.h>

int main(int argc, char *argv[]) {
    GstElement *playbin;
    GstBus *bus;
    GstMessage *msg;

    gst_init(&argc, &argv);

    if (argc < 2) {
        g_printerr("Dùng: %s <URI>\n", argv[0]);
        return -1;
    }

    playbin = gst_element_factory_make("playbin", "player");
    g_object_set(playbin, "uri", argv[1], NULL);

    gst_element_set_state(playbin, GST_STATE_PLAYING);

    bus = gst_element_get_bus(playbin);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE,
        GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    if (msg && GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        GError *err;
        gchar *debug;
        gst_message_parse_error(msg, &err, &debug);
        g_printerr("Lỗi: %s\n", err->message);
        g_clear_error(&err);
        g_free(debug);
    }

    if (msg) gst_message_unref(msg);
    gst_object_unref(bus);
    gst_element_set_state(playbin, GST_STATE_NULL);
    gst_object_unref(playbin);

    return 0;
}
```

Biên dịch và chạy:

```bash
gcc -o player player.c $(pkg-config --cflags --libs gstreamer-1.0)
./player https://gstreamer.freedesktop.org/data/media/sintel_trailer-480p.webm
```

Xem tiếp [`03-tai-lieu-api.md`](./03-tai-lieu-api.md) để tra cứu chi tiết từng hàm/API đã dùng ở trên.
