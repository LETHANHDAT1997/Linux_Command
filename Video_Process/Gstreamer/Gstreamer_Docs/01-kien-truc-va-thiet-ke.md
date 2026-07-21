# Kiến Trúc và Ý Tưởng Thiết Kế của GStreamer

## 1. GStreamer là gì?

GStreamer là framework mã nguồn mở dùng để xây dựng ứng dụng xử lý dữ liệu đa phương tiện (âm thanh, video, hình ảnh, phụ đề...) bằng cách ghép nối các "khối xử lý" nhỏ thành một luồng xử lý hoàn chỉnh gọi là **pipeline**. Lõi framework viết bằng C, dựa trên hệ thống kiểu và đối tượng của GLib/GObject.

Thay vì tự viết một trình phát video hay bộ mã hoá audio từ đầu, GStreamer cung cấp sẵn hàng trăm "linh kiện" (elements) — đọc file, giải mã H.264, chuyển đổi định dạng màu, phát ra loa, đóng gói RTP để streaming... — để lắp ghép lại giống lắp Lego.

Framework này được dùng trong: trình phát media desktop (Totem, Rhythmbox), hệ thống streaming (RTSP/WebRTC server, camera IoT), pipeline thị giác máy tính/AI kết hợp inference theo thời gian thực, trình biên tập video phi tuyến (Pitivi), và cả nghiên cứu khoa học (LIGO dùng GStreamer — qua giao diện GstLAL — để xử lý dữ liệu sóng hấp dẫn).

## 2. Triết lý thiết kế cốt lõi

### 2.1. Mô hình pipe-and-filter (pipeline)

Ý tưởng nền tảng: **luồng dữ liệu chảy qua chuỗi bộ lọc** (dataflow / pipe-and-filter), tương tự triết lý Unix pipe (`cat file | grep x | sort`), nhưng áp dụng cho dữ liệu media với timing, đồng bộ và định dạng phức tạp hơn nhiều:

```
[Nguồn dữ liệu] → [Bộ giải mã] → [Bộ xử lý] → [Đích xuất]
    filesrc          decodebin     videoconvert   autovideosink
```

Dữ liệu (dưới dạng các `GstBuffer`) chảy từ trái sang phải qua từng element. Mỗi element chỉ biết làm đúng một việc — nó không cần biết element trước/sau nó là gì, miễn định dạng dữ liệu (caps) khớp nhau tại điểm nối.

### 2.2. Vì sao chọn kiến trúc này

- **Tách biệt mối quan tâm**: logic đọc file, giải mã, xử lý ảnh, phát âm thanh nằm ở các module độc lập, không đan xen vào nhau.
- **Tái sử dụng tối đa**: cùng một `decodebin` dùng được trong trình phát nhạc, công cụ chuyển đổi định dạng, hay pipeline machine learning.
- **Lắp ráp linh hoạt**: pipeline phát file cục bộ và pipeline streaming qua mạng chỉ khác nhau ở vài element đầu/cuối; phần giải mã/xử lý ở giữa giữ nguyên.
- **Trừu tượng hoá phần cứng**: `autovideosink` tự chọn sink phù hợp hệ thống đang chạy (X11, Wayland, KMS...); mã hoá H.264 chạy trên CPU (`x264enc`) hay GPU (`nvh264enc`, `vaapih264enc`) mà phần còn lại pipeline không cần biết.
- **Đa nền tảng**: cùng một cấu trúc pipeline logic chạy được trên Linux, Windows, macOS, Android, iOS.

### 2.3. So sánh với cách viết "thủ công"

Tự viết trình phát video từ đầu đòi hỏi tự demux container, gọi decoder riêng cho từng codec, đồng bộ audio/video theo timestamp, quản lý buffer, xử lý resize màu... GStreamer đóng gói toàn bộ các bước này thành các element có thể hoán đổi cho nhau; công việc của lập trình viên chuyển thành **định nghĩa đồ thị xử lý (graph)** thay vì viết vòng lặp xử lý byte-by-byte.

## 3. Các khái niệm cốt lõi

### 3.1. Element (`GstElement`)

Khối xây dựng cơ bản nhất — một "linh kiện" thực hiện đúng một chức năng:

- **Source**: sinh dữ liệu (đọc file, đọc mạng, sinh tín hiệu test).
- **Filter**: biến đổi dữ liệu (decoder, encoder, converter, scaler...).
- **Sink**: xuất dữ liệu ra ngoài (loa, màn hình, file, socket mạng).

Mỗi loại element đăng ký dưới một tên factory (`"filesrc"`, `"x264enc"`, `"autoaudiosink"`...) và khởi tạo bằng `gst_element_factory_make()`.

### 3.2. Pad — chân kết nối

Pad là điểm giao tiếp vào/ra của element:

- **sink pad**: nơi dữ liệu đi *vào*.
- **src pad**: nơi dữ liệu đi *ra*.

Hai element chỉ liên kết được khi src pad bên này nối vừa với sink pad bên kia, và **caps** (định dạng dữ liệu) của hai pad tương thích — quá trình này gọi là **caps negotiation**. Ví dụ src pad của `videoconvert` có thể xuất nhiều định dạng màu; nếu sink pad phía sau chỉ nhận `video/x-raw,format=NV12`, hai bên "thương lượng" để chốt định dạng chung duy nhất trước khi dữ liệu thật sự chảy qua.

Một số element có pad cố định ngay lúc khởi tạo (`videoconvert` luôn có đúng 1 sink + 1 src). Một số khác — như `decodebin` — chưa biết trước file chứa bao nhiêu luồng (audio/video/phụ đề) nên **tạo pad động** (dynamic pad) sau khi đã phân tích xong dữ liệu đầu vào; ứng dụng phải lắng nghe signal `pad-added` để kết nối kịp lúc.

### 3.3. Bin và Pipeline — container theo mẫu Composite

`GstBin` là container chứa nhiều element, và **bản thân nó cũng là một `GstElement`** — ứng dụng của mẫu thiết kế Composite: gom một cụm element phức tạp thành "hộp đen" rồi coi nó như 1 element đơn lẻ khi ghép vào pipeline lớn hơn.

`GstPipeline` là một `GstBin` đặc biệt ở cấp cao nhất: quản lý **đồng hồ chung (clock)** cho toàn đồ thị và sở hữu **bus** để nhận thông báo.

### 3.4. Buffer (`GstBuffer`) — đơn vị dữ liệu

Gói dữ liệu media thực sự chảy qua pipeline (một khung hình video, một đoạn mẫu âm thanh...). Mỗi buffer mang metadata quan trọng:

- **PTS** (Presentation Timestamp): thời điểm cần hiển thị/phát.
- **DTS** (Decode Timestamp): thời điểm cần giải mã (khác PTS khi có B-frame).
- **Duration**: thời lượng buffer chiếm.

Buffer dùng **đếm tham chiếu (reference counting)** thay vì copy dữ liệu qua từng element — nhiều element có thể cùng giữ tham chiếu tới vùng nhớ (`GstMemory`) bên dưới, giảm chi phí copy, tiệm cận mô hình zero-copy.

### 3.5. Caps (`GstCaps`) — mô tả định dạng

Mô tả "loại dữ liệu" một pad chấp nhận hoặc tạo ra, ví dụ:

```
video/x-raw, format=(string)I420, width=(int)1280, height=(int)720, framerate=(fraction)30/1
```

**Negotiation** diễn ra khi liên kết hai pad: GStreamer tìm giao của tập caps mà src pad có thể xuất và sink pad có thể nhận, chốt một caps cụ thể duy nhất trước khi dữ liệu chảy qua.

### 3.6. Event — tín hiệu điều khiển trong luồng dữ liệu

Khác buffer (mang dữ liệu media), **event** mang tín hiệu điều khiển, di chuyển xen kẽ trong cùng luồng dữ liệu, theo hai hướng:

- **Downstream** (theo chiều dữ liệu): `EOS`, `SEGMENT`, `FLUSH_START/STOP`, `CAPS`.
- **Upstream** (ngược chiều dữ liệu): `SEEK`, `QOS` (báo phía sau xử lý không kịp), `RECONFIGURE`.

### 3.7. Message và Bus — kênh thông báo bất đồng bộ

Element chạy xử lý dữ liệu trên **luồng streaming riêng**, tách khỏi luồng chính của ứng dụng. Để báo lỗi, cảnh báo, hết luồng, đổi trạng thái..., element **post một `GstMessage` lên `GstBus`** của pipeline thay vì gọi callback trực tiếp — nhờ vậy ứng dụng xử lý các sự kiện này an toàn trên luồng chính (ví dụ luồng UI) mà không lo tranh chấp dữ liệu.

Đây là điểm thiết kế quan trọng: **Bus tách biệt luồng xử lý media khỏi luồng ứng dụng**, giống mô hình message queue giữa các thread.

### 3.8. Query — hỏi thông tin pipeline

Khác event (đẩy tín hiệu một chiều), **query** dùng để hỏi và nhận câu trả lời đồng bộ ngay: vị trí phát hiện tại (`POSITION`), tổng thời lượng (`DURATION`), có hỗ trợ seek không (`SEEKING`).

### 3.9. State — máy trạng thái

Element (và cả pipeline) đi qua 4 trạng thái theo thứ tự:

```
NULL → READY → PAUSED → PLAYING
```

- **NULL**: chưa cấp phát tài nguyên.
- **READY**: đã cấp phát tài nguyên tĩnh (mở thiết bị, cấp bộ nhớ) nhưng chưa xử lý dữ liệu.
- **PAUSED**: sẵn sàng xử lý dữ liệu, đồng hồ dừng — ở đây pipeline thực hiện **preroll** (đẩy đủ dữ liệu để khung hình đầu tiên sẵn sàng hiển thị).
- **PLAYING**: đang chạy, đồng hồ chạy, dữ liệu chảy và render đúng nhịp thời gian.

Việc bắt buộc đi tuần tự qua từng trạng thái giúp mỗi element có cơ hội cấp phát/giải phóng tài nguyên đúng lúc, và giúp pipeline có bước preroll để tránh giật hình khi bắt đầu phát.

### 3.10. Clock — đồng bộ thời gian

`GstClock` cung cấp mốc thời gian chung để pipeline đồng bộ các luồng audio/video khác nhau. Thường một sink (ví dụ audio sink) được chọn làm nguồn cấp đồng hồ; các element khác so khớp PTS của buffer với đồng hồ này để quyết định "render buffer này ngay hay đợi thêm".

## 4. Nền tảng GObject/GLib

GStreamer không tự phát minh hệ thống kiểu — nó xây trên **GLib/GObject**, framework hướng đối tượng cho C có từ dự án GTK/GNOME. Điều này mang lại:

- **Property**: mọi element là GObject nên đọc/ghi thuộc tính qua `g_object_set()` / `g_object_get()` thay vì hàm setter riêng cho từng loại element.
- **Signal**: cơ chế callback chuẩn hoá (`g_signal_connect()`), dùng cho các sự kiện như `pad-added`, `no-more-pads`.
- **Reference counting**: quản lý vòng đời đối tượng qua `gst_object_ref()` / `gst_object_unref()` (tương tự `shared_ptr` trong C++).
- **Type introspection**: nhờ GObject Introspection, cùng thư viện lõi viết bằng C dùng trực tiếp được từ Python, JavaScript (gjs), C#, Rust... mà không cần viết binding thủ công.

## 5. Kiến trúc plugin

Gần như toàn bộ chức năng "thấy được" của GStreamer (từng loại decoder/source/sink cụ thể) **không nằm trong lõi**, mà nằm trong các **plugin** — thư viện chia sẻ nạp động khi cần. Lõi GStreamer chỉ cung cấp bộ khung: hệ thống pad, buffer, bus, state machine, registry.

Plugin nhóm thành các bộ, đặt tên theo lịch sử từ phim "The Good, the Bad and the Ugly":

| Bộ plugin | Ý nghĩa |
|---|---|
| `gst-plugins-base` | Element nền tảng, chất lượng cao, bảo trì tốt (`videotestsrc`, `audioconvert`, `playbin` cơ bản...) |
| `gst-plugins-good` | Chất lượng tốt, giấy phép rõ ràng, không vướng bằng sáng chế |
| `gst-plugins-bad` | Hữu ích nhưng chưa đủ ổn định/hoàn thiện để "tốt nghiệp" lên good |
| `gst-plugins-ugly` | Chất lượng tốt nhưng vướng vấn đề bằng sáng chế/giấy phép ở một số khu vực pháp lý |
| `gst-libav` | Wrapper quanh FFmpeg/libav, bổ sung rất nhiều codec |

Khi gọi `gst_element_factory_make("x264enc", ...)`, GStreamer tra trong **registry** (bộ nhớ đệm đã quét sẵn thông tin toàn bộ plugin cài trên máy) để tìm và nạp đúng plugin cung cấp element đó — nhờ vậy khởi động không phải quét lại toàn bộ thư mục plugin mỗi lần chạy.

## 6. Mô hình luồng dữ liệu & threading

Mặc định, pipeline đơn giản có thể chạy hoàn toàn trên **một luồng streaming** — buffer được "kéo" hoặc "đẩy" tuần tự qua từng element. Nhưng nhiều pipeline thực tế cần xử lý song song (ví dụ giải mã video không nên bị chặn bởi tốc độ ghi ra mạng chậm).

Element **`queue`** đóng vai trò ranh giới luồng: tạo một luồng streaming mới với hàng đợi buffer (giới hạn dung lượng) ở giữa, cho phép phần pipeline trước và sau `queue` chạy trên hai luồng khác nhau, giao tiếp qua hàng đợi đó. Đây là lý do hầu hết pipeline streaming/thời gian thực dùng nhiều `queue` để tách tốc độ xử lý giữa các giai đoạn (đọc mạng, giải mã, hiển thị), tránh giai đoạn chậm làm nghẽn toàn bộ.

Luồng ứng dụng (nơi gọi `gst_element_set_state()`, chạy `GMainLoop`) là luồng riêng, tách khỏi (các) luồng streaming — đây là lý do Bus/Message tồn tại: đưa thông tin an toàn từ luồng streaming về luồng ứng dụng.

## 7. Tổng kết

GStreamer đánh đổi một chút độ phức tạp ban đầu (phải hiểu pad, caps, state machine, bus) để đổi lấy: khả năng mở rộng gần như vô hạn (ai cũng viết plugin mới được), tái sử dụng logic giữa hàng trăm ứng dụng khác nhau, và hiệu năng tốt nhờ zero-copy buffer cùng khả năng tận dụng tăng tốc phần cứng qua plugin chuyên biệt mà không phải đổi kiến trúc pipeline tổng thể.

Phần tiếp theo ([`02-huong-dan-su-dung.md`](./02-huong-dan-su-dung.md)) áp dụng các khái niệm này vào code C thực tế.
