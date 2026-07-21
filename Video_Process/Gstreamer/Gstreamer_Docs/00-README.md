# Bộ Tài Liệu Lập Trình C với GStreamer

Bộ tài liệu này biên soạn cho lập trình viên C muốn tìm hiểu và sử dụng **GStreamer** — framework xử lý đa phương tiện (multimedia) mã nguồn mở, viết bằng C, dựa trên nền GObject/GLib.

Nội dung áp dụng cho dòng API ổn định **GStreamer 1.x** (bản mới nhất tại thời điểm biên soạn: **1.28**, series **1.26** vẫn song song nhận bản vá lỗi). API/ABI của dòng 1.x đã ổn định từ năm 2012 và hiện chưa có kế hoạch phát hành 2.0, nên phần lớn nội dung trong tài liệu này có giá trị lâu dài, ít phụ thuộc phiên bản nhỏ.

## Cấu trúc bộ tài liệu

| File | Nội dung chính |
|---|---|
| [`01-kien-truc-va-thiet-ke.md`](./01-kien-truc-va-thiet-ke.md) | Ý tưởng thiết kế, triết lý kiến trúc, các khái niệm cốt lõi: Element, Pad, Bin/Pipeline, Buffer, Caps, Event, Bus/Message, State, Clock, kiến trúc plugin, mô hình threading |
| [`02-huong-dan-su-dung.md`](./02-huong-dan-su-dung.md) | Cài đặt môi trường, biên dịch, các mẫu code thực tế: pipeline tĩnh, pipeline động (dynamic pad), xử lý Bus/MainLoop, seek, debug, appsrc/appsink, ví dụ trình phát hoàn chỉnh |
| [`03-tai-lieu-api.md`](./03-tai-lieu-api.md) | Tra cứu API dạng bảng: hàm, macro, enum, các loại message/event/query, bảng element thông dụng |

## Đối tượng phù hợp

- Lập trình viên C đã quen cơ bản với con trỏ, cấu trúc, biên dịch bằng `gcc`/`pkg-config`.
- Không yêu cầu biết trước GObject/GLib — phần cần thiết được giải thích trong file 01.

## Cách đọc

- Mới hoàn toàn với GStreamer: đọc theo thứ tự **01 → 02 → 03**.
- Đã hiểu kiến trúc, chỉ cần tra nhanh một hàm/loại message cụ thể: vào thẳng **file 03**.
- Muốn code chạy được ngay để thử nghiệm: vào thẳng **file 02**, mục 3 và mục 10.

## Tài nguyên chính thức bổ sung

- Trang chủ / tài liệu API đầy đủ: https://gstreamer.freedesktop.org/documentation/
- Tra cứu property/caps của 1 element đã cài trên máy: `gst-inspect-1.0 <tên-element>`
