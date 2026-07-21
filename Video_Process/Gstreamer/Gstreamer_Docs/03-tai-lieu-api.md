# Tài Liệu Tra Cứu API — GStreamer 1.x (C)

> Tài liệu tra cứu nhanh, tổ chức theo module. Xem [`01-kien-truc-va-thiet-ke.md`](./01-kien-truc-va-thiet-ke.md) để hiểu khái niệm, [`02-huong-dan-su-dung.md`](./02-huong-dan-su-dung.md) để xem API trong ngữ cảnh code hoàn chỉnh.

## 1. Khởi tạo và phiên bản

| Hàm | Mô tả |
|---|---|
| `void gst_init(int *argc, char **argv[])` | Khởi tạo GStreamer; **bắt buộc gọi trước** mọi hàm GStreamer khác. Tự xử lý tham số dòng lệnh riêng của GStreamer (vd `--gst-debug`). |
| `gboolean gst_init_check(int *argc, char **argv[], GError **error)` | Giống `gst_init` nhưng trả `FALSE` + set `GError` thay vì crash khi khởi tạo thất bại. |
| `void gst_deinit(void)` | Giải phóng tài nguyên toàn cục (thường không cần gọi trong chương trình đơn giản). |
| `const gchar *gst_version_string(void)` | Trả chuỗi phiên bản, vd `"GStreamer 1.28.5"`. |

## 2. `GstElement` — API element

| Hàm | Mô tả |
|---|---|
| `GstElement *gst_element_factory_make(const gchar *factoryname, const gchar *name)` | Tạo 1 element từ tên factory (vd `"filesrc"`). `name` có thể `NULL` để tự sinh. Trả `NULL` nếu factory không tồn tại (thiếu plugin). |
| `GstStateChangeReturn gst_element_set_state(GstElement *element, GstState state)` | Yêu cầu element (hoặc pipeline) chuyển sang trạng thái mới. |
| `GstStateChangeReturn gst_element_get_state(GstElement *element, GstState *state, GstState *pending, GstClockTime timeout)` | Lấy trạng thái hiện tại; dùng để chờ đồng bộ khi kết quả đổi trạng thái là `ASYNC`. |
| `gboolean gst_element_link(GstElement *src, GstElement *dest)` | Liên kết 2 element liền kề (tự tìm pad tương thích). |
| `gboolean gst_element_link_many(GstElement *e1, GstElement *e2, ..., NULL)` | Liên kết một chuỗi element, kết thúc bằng `NULL`. |
| `gboolean gst_element_link_filtered(GstElement *src, GstElement *dest, GstCaps *filter)` | Liên kết nhưng ép caps cụ thể tại điểm nối (thường dùng với `capsfilter`). |
| `GstPad *gst_element_get_static_pad(GstElement *element, const gchar *name)` | Lấy pad cố định theo tên (vd `"sink"`, `"src"`). |
| `GstPad *gst_element_request_pad_simple(GstElement *element, const gchar *name)` | Yêu cầu tạo pad theo mẫu — dùng cho element có pad theo yêu cầu như `tee`, `videomixer`. Thay thế hàm cũ `gst_element_get_request_pad()` (đã deprecated). |
| `gboolean gst_element_query_position(GstElement *element, GstFormat format, gint64 *cur)` | Hỏi vị trí phát hiện tại. |
| `gboolean gst_element_query_duration(GstElement *element, GstFormat format, gint64 *duration)` | Hỏi tổng thời lượng. |
| `gboolean gst_element_seek_simple(GstElement *element, GstFormat format, GstSeekFlags flags, gint64 seek_pos)` | Tua tới vị trí chỉ định (dạng đơn giản hoá của `gst_element_seek`). |
| `gboolean gst_element_send_event(GstElement *element, GstEvent *event)` | Gửi 1 event tuỳ chỉnh vào element. |
| `GstBus *gst_element_get_bus(GstElement *element)` | Lấy bus của pipeline (chỉ có ý nghĩa khi gọi trên `GstPipeline`). |

**`GstState`** (enum): `GST_STATE_VOID_PENDING`, `GST_STATE_NULL`, `GST_STATE_READY`, `GST_STATE_PAUSED`, `GST_STATE_PLAYING`.

**`GstStateChangeReturn`** (enum trả về khi đổi trạng thái):

| Giá trị | Ý nghĩa |
|---|---|
| `GST_STATE_CHANGE_FAILURE` | Chuyển trạng thái thất bại |
| `GST_STATE_CHANGE_SUCCESS` | Thành công ngay lập tức |
| `GST_STATE_CHANGE_ASYNC` | Đang xử lý bất đồng bộ (thường khi PAUSED→PLAYING cần preroll); chờ message `ASYNC_DONE` hoặc gọi `gst_element_get_state()` kèm timeout |
| `GST_STATE_CHANGE_NO_PREROLL` | Thành công nhưng element không preroll (vd live source) |

## 3. `GstBin` / `GstPipeline`

| Hàm | Mô tả |
|---|---|
| `GstElement *gst_pipeline_new(const gchar *name)` | Tạo pipeline rỗng. |
| `gboolean gst_bin_add(GstBin *bin, GstElement *element)` | Thêm 1 element vào bin/pipeline. |
| `gboolean gst_bin_add_many(GstBin *bin, GstElement *e1, ..., NULL)` | Thêm nhiều element cùng lúc, kết thúc bằng `NULL`. |
| `gboolean gst_bin_remove(GstBin *bin, GstElement *element)` | Gỡ element khỏi bin. |
| `GstElement *gst_bin_get_by_name(GstBin *bin, const gchar *name)` | Tìm lại element theo tên đã đặt lúc tạo. |
| `GstElement *gst_parse_launch(const gchar *pipeline_description, GError **error)` | Dựng cả pipeline từ 1 chuỗi mô tả kiểu `gst-launch-1.0`. |

Macro ép kiểu thường dùng: `GST_BIN(obj)`, `GST_ELEMENT(obj)`, `GST_PIPELINE(obj)` — chuyển con trỏ `GstElement*`/`GObject*` sang đúng kiểu con để gọi hàm chuyên biệt.

## 4. `GstPad`

| Hàm | Mô tả |
|---|---|
| `GstPadLinkReturn gst_pad_link(GstPad *srcpad, GstPad *sinkpad)` | Liên kết trực tiếp 2 pad cụ thể (dùng khi cần kiểm soát chi tiết, vd trong callback `pad-added`). |
| `gboolean gst_pad_unlink(GstPad *srcpad, GstPad *sinkpad)` | Gỡ liên kết. |
| `gboolean gst_pad_is_linked(GstPad *pad)` | Kiểm tra pad đã liên kết chưa. |
| `GstCaps *gst_pad_get_current_caps(GstPad *pad)` | Lấy caps đã chốt (sau negotiation) đang áp dụng trên pad. |
| `GstCaps *gst_pad_query_caps(GstPad *pad, GstCaps *filter)` | Hỏi tập caps khả dụng của pad. |
| `gulong gst_pad_add_probe(GstPad *pad, GstPadProbeType mask, GstPadProbeCallback callback, gpointer user_data, GDestroyNotify notify)` | Gắn "đầu dò" theo dõi/can thiệp buffer hoặc event đi qua pad — dùng để thống kê, chèn/sửa dữ liệu, chặn tạm thời luồng. |
| `void gst_pad_remove_probe(GstPad *pad, gulong id)` | Gỡ probe đã gắn. |

Signal thường gặp trên element có pad động: `"pad-added"` — `(GstElement *element, GstPad *new_pad, gpointer user_data)`; `"no-more-pads"` — báo đã tạo xong toàn bộ pad động.

## 5. `GstCaps` / `GstStructure`

| Hàm | Mô tả |
|---|---|
| `GstCaps *gst_caps_new_simple(const gchar *media_type, const gchar *fieldname, ...)` | Tạo caps từ danh sách field kiểu `key, GType, value, ..., NULL`. |
| `GstCaps *gst_caps_from_string(const gchar *string)` | Parse caps từ chuỗi văn bản (cùng cú pháp hiển thị bởi `gst-inspect-1.0`). |
| `void gst_caps_unref(GstCaps *caps)` | Giải phóng tham chiếu caps. |
| `GstStructure *gst_caps_get_structure(const GstCaps *caps, guint index)` | Lấy cấu trúc field bên trong caps. |
| `const gchar *gst_structure_get_name(const GstStructure *structure)` | Lấy media type, vd `"video/x-raw"`. |
| `gboolean gst_structure_get_int(const GstStructure *s, const gchar *field, gint *value)` | Đọc field kiểu int, vd `width`, `height`. |

## 6. `GstBuffer` / bộ nhớ

| Hàm / Macro | Mô tả |
|---|---|
| `GstBuffer *gst_buffer_new_allocate(GstAllocator *allocator, gsize size, GstAllocationParams *params)` | Cấp phát 1 buffer trống kích thước `size` byte. |
| `gboolean gst_buffer_map(GstBuffer *buffer, GstMapInfo *info, GstMapFlags flags)` | Ánh xạ vùng nhớ buffer để đọc/ghi trực tiếp (`GST_MAP_READ`, `GST_MAP_WRITE`). Kết quả ở `info.data`, `info.size`. |
| `void gst_buffer_unmap(GstBuffer *buffer, GstMapInfo *info)` | Bắt buộc gọi sau khi dùng xong map, đối xứng với `gst_buffer_map`. |
| `GST_BUFFER_PTS(buf)` / `GST_BUFFER_DTS(buf)` | Macro đọc/ghi trực tiếp Presentation/Decode Timestamp. |
| `GST_BUFFER_DURATION(buf)` | Macro đọc/ghi thời lượng buffer. |
| `void gst_buffer_unref(GstBuffer *buf)` | Giảm tham chiếu; buffer chỉ thực sự giải phóng khi về 0. |

## 7. `GstBus` / `GstMessage`

| Hàm | Mô tả |
|---|---|
| `GstBus *gst_pipeline_get_bus(GstPipeline *pipeline)` | Lấy bus (tương đương gọi `gst_element_get_bus` trên pipeline). |
| `guint gst_bus_add_watch(GstBus *bus, GstBusFunc func, gpointer user_data)` | Gắn bus vào `GMainLoop`; `func` được gọi mỗi khi có message mới, không chặn luồng. |
| `void gst_bus_add_signal_watch(GstBus *bus)` | Chuyển message thành GObject signal `"message"` — hữu ích khi muốn dùng nhiều `g_signal_connect` lọc theo loại message thay vì 1 hàm switch lớn. |
| `GstMessage *gst_bus_timed_pop_filtered(GstBus *bus, GstClockTime timeout, GstMessageType types)` | Chờ (chặn luồng) đến khi có message thuộc `types` hoặc hết `timeout` (`GST_CLOCK_TIME_NONE` = chờ vô hạn). |
| `GstMessage *gst_bus_pop(GstBus *bus)` | Lấy 1 message nếu có sẵn, không chặn (`NULL` nếu chưa có). |

Hàm parse message theo loại (nhận `GstMessage*`, ghi kết quả ra tham số output):

| Hàm parse | Dùng cho loại message |
|---|---|
| `gst_message_parse_error(msg, &GError*, &gchar* debug)` | `GST_MESSAGE_ERROR` |
| `gst_message_parse_warning(msg, &GError*, &gchar* debug)` | `GST_MESSAGE_WARNING` |
| `gst_message_parse_state_changed(msg, &old, &new, &pending)` | `GST_MESSAGE_STATE_CHANGED` |
| `gst_message_parse_tag(msg, &GstTagList*)` | `GST_MESSAGE_TAG` |
| `gst_message_parse_buffering(msg, &gint percent)` | `GST_MESSAGE_BUFFERING` |

**Các `GstMessageType` thường gặp** (macro `GST_MESSAGE_TYPE(msg)` để lấy loại):

| Loại | Khi nào được post |
|---|---|
| `GST_MESSAGE_EOS` | Toàn bộ pipeline đã xử lý xong dữ liệu (hết luồng) |
| `GST_MESSAGE_ERROR` | Lỗi nghiêm trọng, pipeline không thể tiếp tục |
| `GST_MESSAGE_WARNING` | Cảnh báo, pipeline vẫn chạy tiếp được |
| `GST_MESSAGE_INFO` | Thông tin không nghiêm trọng |
| `GST_MESSAGE_STATE_CHANGED` | Một element (hoặc pipeline) đổi trạng thái |
| `GST_MESSAGE_BUFFERING` | Đang đệm dữ liệu (thường khi phát qua mạng); app nên tạm PAUSE cho tới 100% |
| `GST_MESSAGE_TAG` | Có metadata mới (tên bài hát, nghệ sĩ, codec...) |
| `GST_MESSAGE_DURATION_CHANGED` | Thời lượng tổng thay đổi/được xác định |
| `GST_MESSAGE_ASYNC_DONE` | Thao tác bất đồng bộ (đổi trạng thái, seek) đã hoàn tất |
| `GST_MESSAGE_ELEMENT` | Message tuỳ chỉnh do một element cụ thể định nghĩa |

## 8. `GstEvent`

| Loại event | Hướng | Ý nghĩa |
|---|---|---|
| `GST_EVENT_EOS` | downstream | Báo hết dữ liệu |
| `GST_EVENT_SEGMENT` | downstream | Xác định phạm vi thời gian/định dạng của dữ liệu sắp tới |
| `GST_EVENT_FLUSH_START` / `FLUSH_STOP` | cả hai | Yêu cầu xả (loại bỏ) dữ liệu đang xử lý dở, dùng khi seek |
| `GST_EVENT_CAPS` | downstream | Báo caps đã chốt trước khi buffer đầu tiên theo caps đó tới |
| `GST_EVENT_SEEK` | upstream | Yêu cầu tua |
| `GST_EVENT_QOS` | upstream | Báo phía sau xử lý không kịp (quality-of-service) |

Hàm tạo event hay dùng: `GstEvent *gst_event_new_seek(gdouble rate, GstFormat format, GstSeekFlags flags, GstSeekType start_type, gint64 start, GstSeekType stop_type, gint64 stop)`.

## 9. `GstQuery`

| Loại query | Ý nghĩa |
|---|---|
| `GST_QUERY_POSITION` | Vị trí hiện tại |
| `GST_QUERY_DURATION` | Tổng thời lượng |
| `GST_QUERY_SEEKING` | Pipeline có hỗ trợ tua không, phạm vi tua được |
| `GST_QUERY_CAPS` | Hỏi caps khả dụng |

Thường dùng gián tiếp qua hàm tiện ích cấp cao (`gst_element_query_position`, `gst_element_query_duration`) thay vì tự tạo `GstQuery` thủ công, trừ khi cần loại query đặc biệt.

## 10. GObject API dùng xuyên suốt GStreamer

| Hàm | Mô tả |
|---|---|
| `void g_object_set(gpointer object, const gchar *first_property, ..., NULL)` | Gán 1 hoặc nhiều thuộc tính, kết thúc bằng `NULL`. |
| `void g_object_get(gpointer object, const gchar *first_property, ..., NULL)` | Đọc 1 hoặc nhiều thuộc tính vào biến con trỏ tương ứng. |
| `gulong g_signal_connect(gpointer instance, const gchar *detailed_signal, GCallback c_handler, gpointer data)` | Đăng ký callback cho 1 signal (vd `"pad-added"`). |
| `gpointer gst_object_ref(gpointer object)` / `void gst_object_unref(gpointer object)` | Tăng/giảm tham chiếu cho các đối tượng GStreamer (kế thừa từ `GstObject`). |

## 11. Bảng element phổ biến (tra nhanh)

| Element | Vai trò |
|---|---|
| `filesrc` | Đọc dữ liệu từ file |
| `filesink` | Ghi dữ liệu ra file |
| `videotestsrc` / `audiotestsrc` | Sinh dữ liệu test (hình caro, tần số âm chuẩn) — hữu ích để thử pipeline không cần file thật |
| `decodebin` | Tự nhận diện định dạng và giải mã (tạo pad động) |
| `uridecodebin` | Giống `decodebin` nhưng nhận thẳng URI (đã bao gồm phần đọc nguồn) |
| `playbin` | Element cấp cao, tự lắp toàn bộ pipeline phát media chỉ từ 1 URI |
| `videoconvert` / `audioconvert` | Chuyển đổi định dạng màu / định dạng mẫu âm thanh giữa các element có caps khác nhau |
| `videoscale` / `audioresample` | Đổi độ phân giải video / tần số lấy mẫu audio |
| `capsfilter` | Ép caps cụ thể tại 1 điểm trong pipeline (thường dùng với property `caps`) |
| `queue` | Hàng đợi buffer, tạo ranh giới luồng streaming riêng |
| `tee` | Tách 1 luồng dữ liệu thành nhiều nhánh song song |
| `appsrc` / `appsink` | Điểm nối cho ứng dụng tự bơm/lấy dữ liệu |
| `x264enc` / `avdec_h264` | Mã hoá / giải mã H.264 (phần mềm) |
| `vaapih264enc`, `nvh264enc` | Mã hoá H.264 tăng tốc phần cứng (VA-API trên Linux Intel/AMD, NVENC trên GPU NVIDIA) |
| `rtph264pay` / `rtph264depay` | Đóng gói / gỡ gói H.264 theo chuẩn RTP để truyền qua mạng |
| `autovideosink` / `autoaudiosink` | Tự chọn video/audio sink phù hợp hệ thống đang chạy |

Tra cứu chi tiết thuộc tính (property) và khả năng (caps) của bất kỳ element nào đã cài trên máy bằng:

```bash
gst-inspect-1.0 <tên-element>
```
