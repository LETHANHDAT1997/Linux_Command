# Hướng dẫn về Tín hiệu (Signal) trong Linux và Phân biệt với epoll_event

Tài liệu này tổng hợp chi tiết sự khác biệt giữa hai cơ chế thông báo sự kiện phổ biến trong Linux: **Signal (Tín hiệu)** và **`epoll_event` (trong hệ thống `epoll`)**, đồng thời liệt kê đầy đủ các trường hợp thực tế nên sử dụng Signal.

---

## 1. Phân biệt `epoll_event` và Signal (sigaction)

| Đặc điểm | Signal (`sigaction`) | `epoll_event` (`epoll`) |
| :--- | :--- | :--- |
| **Mục đích chính** | Thông báo sự kiện hệ thống cấp tiến trình (lỗi bộ nhớ, tắt máy, tiến trình con...). | Giám sát trạng thái sẵn sàng I/O (đọc/ghi) của nhiều File Descriptor cùng lúc. |
| **Cơ chế kích hoạt** | **Bất đồng bộ (Asynchronous)**: Kernel ngắt tiến trình tại bất kỳ dòng code nào để chạy handler. | **Đồng bộ (Synchronous)**: Tiến trình chủ động đợi tại hàm block `epoll_wait()`. |
| **Phạm vi tác động** | Toàn bộ tiến trình (Process-wide). | Các File Descriptor cụ thể (Sockets, Pipes, Devices...). |
| **Độ an toàn** | Phức tạp. Chỉ được gọi các hàm an toàn với tín hiệu (**async-signal-safe**) để tránh deadlock. | Rất an toàn. Xử lý trực tiếp trong luồng chạy tuần tự của vòng lặp sự kiện (Event Loop). |
| **Hàng đợi (Queue)** | Mặc định không xếp hàng (nhiều tín hiệu trùng loại gửi liên tiếp sẽ bị gộp, trừ Real-time Signals). | Được quản lý hiệu quả dưới dạng cấu trúc cây Đỏ-Đen (Red-Black Tree) và Ready List trong Kernel. |

### Ví dụ ẩn dụ thực tế:
* **Signal** giống như **Còi báo cháy**: Bạn đang nấu ăn (chương trình chạy bình thường), còi báo cháy kêu (tín hiệu gửi tới). Bạn buộc phải dừng ngay lập tức việc đang làm (ngắt chương trình), chạy đi dập lửa (chạy handler), xong mới quay lại nấu tiếp. Bạn bị động và có thể bị ngắt ở bất kỳ thời điểm nào.
* **`epoll_event`** giống như **Chuông báo nước sôi**: Bạn đun 5 nồi nước (5 socket). Thay vì đứng canh từng nồi, bạn ngồi đợi (`epoll_wait`). Khi nồi 2 và 4 sôi, chuông báo cho bạn biết (`epoll_event`). Bạn chủ động đứng lên xử lý nồi 2 rồi đến nồi 4. Bạn hoàn toàn chủ động và không bị ngắt quãng bất ngờ.

---

## 2. 10 Trường hợp nên sử dụng Signal trong Linux

Dưới đây là danh sách đầy đủ các kịch bản thực tế mà lập trình viên hệ thống Linux cần sử dụng Signal:

### 1. Tắt tiến trình an toàn (Graceful Shutdown)
* **Tín hiệu**: `SIGINT` (nhấn `Ctrl+C`), `SIGTERM` (lệnh `kill` mặc định), `SIGHUP` (khi tắt terminal).
* **Ứng dụng**: Bắt các tín hiệu này để dọn dẹp tài nguyên trước khi chương trình thoát hoàn toàn:
  * Đóng và ghi nốt dữ liệu còn dang dở trên file/database.
  * Xóa các file socket tạm trên đĩa cứng (ví dụ: `unlink(SOCK_PATH)`).
  * Giải phóng các vùng nhớ dùng chung (Shared Memory).

### 2. Thu dọn tiến trình con (Tránh Zombie Process)
* **Tín hiệu**: `SIGCHLD`.
* **Ứng dụng**: Khi tiến trình cha sinh ra tiến trình con (`fork()`), khi tiến trình con kết thúc (`exit()`), Kernel sẽ gửi tín hiệu `SIGCHLD` về cho cha. Tiến trình cha cần bắt tín hiệu này và gọi `wait()` hoặc `waitpid()` để giải phóng tài nguyên của con, tránh biến nó thành Zombie Process chiếm dụng bảng tiến trình của hệ thống.

### 3. Load lại file cấu hình không cần khởi động lại (Reload Config)
* **Tín hiệu**: `SIGHUP` hoặc các tín hiệu người dùng tự định nghĩa (`SIGUSR1`, `SIGUSR2`).
* **Ứng dụng**: Rất phổ biến ở các Web Server/Daemon hệ thống (như Nginx, Apache). Khi quản trị viên thay đổi file cấu hình, họ sẽ gửi tín hiệu `SIGHUP` đến tiến trình. Tiến trình bắt được tín hiệu này sẽ load lại cấu hình mới mà không làm đứt quãng các kết nối hiện tại của khách hàng.

### 4. Ngăn chặn chương trình bị crash do ghi vào đường ống hỏng (Broken Pipe)
* **Tín hiệu**: `SIGPIPE`.
* **Ứng dụng**: Khi bạn cố tình ghi dữ liệu (`write`/`send`) vào một Socket hoặc Pipe đã bị phía bên kia đóng kết nối, mặc định Kernel sẽ gửi `SIGPIPE` để chấm dứt ngay chương trình của bạn.
* **Cách xử lý**: Bạn nên bỏ qua tín hiệu này (`SIG_IGN`) để tiến trình không bị dừng đột ngột. Khi đó, hàm ghi dữ liệu sẽ trả về lỗi `-1` kèm `errno = EPIPE` để bạn tự xử lý lỗi bằng code.

### 5. Ghi log sự cố trước khi crash (Crash Dump)
* **Tín hiệu**: `SIGSEGV` (lỗi truy cập bộ nhớ), `SIGFPE` (lỗi chia cho 0), `SIGILL` (chỉ thị CPU không hợp lệ).
* **Ứng dụng**: Khi chương trình gặp lỗi nghiêm trọng không thể tiếp tục chạy, bạn có thể bắt các tín hiệu này để ghi lại nhật ký lỗi (Stack trace, trạng thái thanh ghi) nhằm phục vụ công tác debug trước khi tiến trình tắt hoàn toàn.

### 6. Điều khiển trạng thái tác vụ (Job Control)
* **Tín hiệu**: `SIGSTOP` (tạm dừng ngay lập tức, không thể bị bỏ qua), `SIGTSTP` (phím `Ctrl+Z`), `SIGCONT` (cho phép tiến trình đang dừng tiếp tục chạy).
* **Ứng dụng**: Được sử dụng bởi các trình Shell (Bash/Zsh) để quản lý chạy ngầm (background) hoặc chạy nổi (foreground) thông qua các lệnh `bg` và `fg`.

### 7. Hẹn giờ và Định thời (Timers)
* **Tín hiệu**: `SIGALRM` (kích hoạt từ hàm `alarm()` hoặc `setitimer()`).
* **Ứng dụng**: Dùng để thiết lập thời gian chờ (timeout) cho các hàm bị chặn hoặc để thực hiện một tác vụ định kỳ sau mỗi khoảng thời gian định trước.

### 8. Gửi tín hiệu kèm theo dữ liệu (Real-time Signals)
* **Tín hiệu**: Từ `SIGRTMIN` đến `SIGRTMAX`.
* **Ứng dụng**: Khác với tín hiệu thông thường, Real-time Signals được xếp hàng gửi (không bị gộp/mất) và cho phép đính kèm một số nguyên hoặc một con trỏ vùng nhớ thông qua hàm `sigqueue()`. Được dùng cho các hệ thống IPC yêu cầu độ chính xác cao.

### 9. Bảo mật và Cô lập hệ thống (Sandboxing)
* **Tín hiệu**: `SIGSYS`.
* **Ứng dụng**: Sử dụng trong cơ chế bảo mật **Seccomp** (Secure Computing Mode). Khi một ứng dụng cố gọi một System Call lạ không nằm trong danh sách cho phép (dấu hiệu của mã độc hoặc bị hacker tấn công), Kernel sẽ gửi `SIGSYS` để chấm dứt ngay lập tức tiến trình đó.

### 10. Phục vụ công cụ gỡ lỗi (Debugging)
* **Tín hiệu**: `SIGTRAP`.
* **Ứng dụng**: Được dùng bởi các chương trình gỡ lỗi như `gdb`. Khi bạn đặt một Breakpoint, trình gỡ lỗi sẽ chèn lệnh tạo tín hiệu `SIGTRAP` vào tiến trình đang chạy để dừng nó lại và kiểm tra trạng thái bộ nhớ.
