# Chat Conversation

Note: _This is purely the output of the chat conversation and does not contain any raw data, codebase snippets, etc. used to generate the output._

### User Input

socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT); giải thích

### Planner Response

Dòng lệnh `socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);` được sử dụng trong lập trình hệ thống Linux để **tạo một socket Netlink nhằm lắng nghe các sự kiện thay đổi trạng thái của kernel kobject (thường gọi là uevents)** từ kernel gửi lên user-space. Đây là cơ chế phổ biến nhất để phát hiện thiết bị cắm nóng (hotplug) như thẻ nhớ SD, USB, cáp mạng, v.v.

Dưới đây là giải thích chi tiết từng tham số trong hàm `socket()`:

---

### 1. `PF_NETLINK` (Protocol Family Netlink)
* **Ý nghĩa:** Chỉ định họ giao thức (Protocol Family) sẽ sử dụng là **Netlink**.
* **Chi tiết:** Netlink là một giao thức đặc thù của Linux (không có trên các hệ thống Unix khác như BSD/macOS) dùng để giao tiếp hai chiều (IPC) giữa không gian nhân (Kernel-space) và không gian người dùng (User-space). Thay vì dùng các cơ chế như `/proc` hay `/sys` (vốn hoạt động theo cơ chế pull/polling), Netlink cho phép gửi sự kiện bất đồng bộ theo cơ chế push (kernel tự động gửi thông báo khi có sự kiện xảy ra).

### 2. `SOCK_DGRAM` (Socket Datagram)
* **Ý nghĩa:** Định nghĩa kiểu truyền dữ liệu là dạng gói tin không hướng kết nối (Connectionless).
* **Chi tiết:** Netlink chỉ hỗ trợ truyền dạng gói tin độc lập (datagram) và không hỗ trợ truyền dạng luồng dữ liệu (stream như TCP). Do đó, `SOCK_DGRAM` hoặc `SOCK_RAW` là các giá trị hợp lệ cho Socket Netlink. Khi dùng `SOCK_DGRAM`, nhân Linux sẽ đảm bảo đóng gói các thông tin sự kiện thành các gói tin riêng biệt và chuyển giao tới ứng dụng.

### 3. `NETLINK_KOBJECT_UEVENT`
* **Ý nghĩa:** Xác định giao thức con (protocol) cụ thể trong họ Netlink mà socket này sẽ kết nối vào.
* **Chi tiết:** Nhân Linux có nhiều kênh Netlink khác nhau phục vụ các mục đích khác nhau (như `NETLINK_ROUTE` cho cấu hình mạng, `NETLINK_SELINUX` cho bảo mật, v.v.). `NETLINK_KOBJECT_UEVENT` là kênh chuyên biệt để kernel phát đi các thông điệp **uevent** (User-space Event) khi các đối tượng kernel (`kobject`) thay đổi trạng thái.
  * Các sự kiện này chủ yếu mô tả trạng thái phần cứng thay đổi: cắm thiết bị mới (`add`), rút thiết bị (`remove`), thay đổi cấu hình (`change`), v.v.

---

### Cách thức hoạt động trong ứng dụng thực tế
Sau khi tạo socket này, chương trình thường thực hiện các bước tiếp theo:
1. **Liên kết Socket (`bind`):** Bind socket này với cấu trúc địa chỉ `struct sockaddr_nl` để đăng ký lắng nghe multicast group của uevent (thường là group `1` đại diện cho các sự kiện quảng bá uevent toàn hệ thống).
2. **Lắng nghe sự kiện (`recv` / `read`):** Sử dụng các hàm đọc dữ liệu như `recv()` hoặc `recvmsg()` trong một vòng lặp để đợi gói tin từ kernel.
3. **Phân tích dữ liệu (Parse uevent):** Dữ liệu nhận được sẽ là một chuỗi ký tự chứa thông tin chi tiết về thiết bị, ví dụ:
   ```text
   add@/devices/platform/soc/3f202000.mmc/mmc_host/mmc0/mmc0:e624/block/mmcsd0
   ACTION=add
   DEVPATH=/devices/platform/soc/3f202000.mmc/mmc_host/mmc0/mmc0:e624/block/mmcsd0
   SUBSYSTEM=block
   MAJOR=179
   MINOR=0
   DEVNAME=mmcsd0
   DEVTYPE=disk
   ```
   *Nhờ vào chuỗi thông tin này, các ứng dụng như `sd_monitor` có thể ngay lập tức phát hiện thẻ nhớ SD được cắm vào và thực hiện mount nó lên hệ thống.*

### User Input

"Thay vì dùng các cơ chế như /proc hay /sys" -> tôi không  biết nó là gì?

*Viewed [socket_default_server.c](file:///home/ledat/Documents/Linux_Command/IPC/socket_default/socket_default_server.c) *

### Planner Response

Trong hệ điều hành Linux, **/proc** và **/sys** là hai **hệ thống tệp ảo (virtual filesystems)** cực kỳ quan trọng. Chúng không chiếm bất kỳ dung lượng nào trên ổ cứng của bạn, mà thực chất là giao diện giao tiếp trực tiếp giữa Kernel (nhân hệ điều hành) và User-space (không gian người dùng).

Khi bạn tương tác với các tệp tin trong hai thư mục này, thực chất bạn đang tương tác trực tiếp với các biến và cấu trúc dữ liệu bên trong bộ nhớ RAM của Kernel.

---

### 1. `/proc` (Process Information Filesystem)
* **Ý nghĩa:** Đây là nơi lưu trữ thông tin về các tiến trình (Processes) đang chạy và thông tin tài nguyên hệ thống nói chung.
* **Cách tổ chức:**
  * Mỗi tiến trình đang chạy sẽ có một thư mục riêng mang tên PID (Process ID) của nó. Ví dụ: `/proc/1234/` chứa thông tin về tiến trình có PID là 1234 (như lượng RAM tiêu thụ, file đang mở, biến môi trường...).
  * Ngoài ra, nó chứa các thông số hệ thống chung như thông tin CPU, RAM, ổ cứng:
    * `/proc/cpuinfo`: Thông tin về CPU.
    * `/proc/meminfo`: Thông tin về bộ nhớ RAM (lệnh `free` thực chất là đọc từ file này).
    * `/proc/mounts`: Danh sách các phân vùng ổ đĩa đang được mount vào hệ thống.
* **Ví dụ thực tế:** Khi bạn chạy lệnh `cat /proc/meminfo`, Kernel sẽ tự động thu thập trạng thái RAM hiện tại và trả về kết quả dưới dạng văn bản.

---

### 2. `/sys` (Sysfs Filesystem)
* **Ý nghĩa:** Được thiết kế để quản lý và hiển thị thông tin chi tiết về **cấu trúc phần cứng**, driver, thiết bị ngoại vi và các Kernel Objects (`kobjects`).
* **Cách tổ chức:** `/sys` sắp xếp thiết bị rất khoa học theo dạng cây phân cấp (theo bus như PCI, USB, hoặc theo phân lớp thiết bị như block, net...).
  * Mỗi tệp tin trong `/sys` thường chỉ chứa **một giá trị duy nhất** đại diện cho một thuộc tính của thiết bị.
* **Ví dụ thực tế:**
  * `/sys/class/power_supply/BAT0/capacity`: Chứa phần trăm pin hiện tại của laptop (ví dụ: file chứa số `85`).
  * `/sys/class/thermal/thermal_zone0/temp`: Chứa nhiệt độ hiện tại của CPU.

---

### So sánh cơ chế giao tiếp: `/proc`/`/sys` vs Netlink

Để hiểu tại sao lại so sánh hai cơ chế này trong việc giám sát thiết bị cắm nóng (như thẻ nhớ SD), bạn có thể hình dung qua bảng so sánh sau:

| Đặc điểm | Qua `/proc` hoặc `/sys` (Cơ chế Polling) | Qua Netlink Socket (Cơ chế Event-driven) |
| :--- | :--- | :--- |
| **Cách hoạt động** | **Pull (Kéo):** Ứng dụng phải tự chủ động hỏi. Bạn phải tạo một vòng lặp liên tục đọc file `/proc/mounts` để xem có thiết bị mới xuất hiện hay không. | **Push (Đẩy):** Kernel tự chủ động thông báo. Ứng dụng chỉ cần nằm im chờ (`recv()` block) cho đến khi Kernel gửi tín hiệu có thiết bị mới. |
| **Tốn tài nguyên** | **Tốn CPU:** Đọc file liên tục (ví dụ 1 giây/lần) sẽ làm CPU không được nghỉ ngơi, hao pin. | **Không tốn CPU:** Ứng dụng ở trạng thái ngủ (Sleep) cho đến khi có sự kiện, tiết kiệm tài nguyên tuyệt đối. |
| **Độ trễ** | **Cao:** Có độ trễ tối đa bằng chu kỳ vòng lặp quét của bạn. | **Gần như bằng 0:** Kernel phát hiện thiết bị là gửi thông báo ngay lập tức. |

### User Input

ok, thôi bỏ  qua đi, trở lại với socket,  ngoài Netlink thì còn các dạng nào khác nữa?

*Viewed [socket_default_server.c](file:///home/ledat/Documents/Linux_Command/IPC/socket_default/socket_default_server.c) *

### Planner Response

Trong lập trình Linux, khi tạo một Socket bằng hàm `socket(domain, type, protocol)`, tham số thứ nhất (`domain` - hay còn gọi là **Address Family / Protocol Family**) và tham số thứ hai (`type` - loại truyền dữ liệu) sẽ xác định dạng socket.

Ngoài Netlink (`AF_NETLINK`), dưới đây là các dạng socket phổ biến nhất được phân chia theo hai khía cạnh: **Phạm vi giao tiếp (Domain)** và **Cách thức truyền dữ liệu (Type)**.

---

### 1. Phân loại theo phạm vi giao tiếp (Domain / Address Family)

Đây là tham số thứ nhất truyền vào hàm `socket()`. Có 3 họ socket phổ biến nhất:

* **`AF_UNIX` (hoặc `AF_LOCAL` - Unix Domain Socket):**
  * **Đặc điểm:** Dùng để giao tiếp giữa các tiến trình **trên cùng một máy vật lý** (Inter-Process Communication - IPC).
  * **Địa chỉ sử dụng:** Đường dẫn tệp tin trên hệ điều hành (ví dụ: `/tmp/my_socket`).
  * **Hiệu năng:** Rất nhanh và hiệu quả vì dữ liệu được sao chép trực tiếp trong bộ nhớ RAM của nhân, không cần đi qua ngăn xếp giao thức mạng (TCP/IP).
  * *Lưu ý: Đây chính là dạng socket mà bạn đang sử dụng trong file code `socket_default_server.c` đang mở (`AF_LOCAL`).*

* **`AF_INET` (IPv4 Internet Protocols):**
  * **Đặc điểm:** Dùng để giao tiếp giữa các thiết bị **qua môi trường mạng** sử dụng giao thức IPv4 (có thể giao tiếp nội bộ qua IP `127.0.0.1` hoặc mạng Internet).
  * **Địa chỉ sử dụng:** Kết hợp giữa Địa chỉ IP (ví dụ: `192.168.1.5`) và Số hiệu cổng (Port, ví dụ: `8080`).

* **`AF_INET6` (IPv6 Internet Protocols):**
  * Tương tự như `AF_INET` nhưng sử dụng định dạng địa chỉ IPv6 để giao tiếp mạng.

* **Các họ socket nâng cao khác:**
  * `AF_PACKET`: Cho phép giao tiếp trực tiếp với tầng liên kết dữ liệu mạng (Layer 2 / Network device driver), thường dùng cho các công cụ bắt gói tin như Wireshark, tcpdump.
  * `AF_CAN`: Dùng trong lập trình nhúng để giao tiếp với mạng CAN bus (Controller Area Network) trên ô tô hoặc thiết bị công nghiệp.

---

### 2. Phân loại theo cách thức truyền dữ liệu (Socket Type)

Đây là tham số thứ hai truyền vào hàm `socket()`, quyết định cách thức dữ liệu được gửi đi:

* **`SOCK_STREAM` (Stream Socket):**
  * **Đặc điểm:** Hướng kết nối (Connection-oriented), truyền dữ liệu dạng luồng tuần tự liên tục, tin cậy, không mất mát và đúng thứ tự (đảm bảo bởi giao thức TCP khi dùng mạng, hoặc cơ chế nhân khi dùng Unix Domain Socket).
  * **Ứng dụng:** HTTP (Web), FTP, SSH, và hầu hết các ứng dụng IPC cần tính chính xác cao.

* **`SOCK_DGRAM` (Datagram Socket):**
  * **Đặc điểm:** Không hướng kết nối (Connectionless), dữ liệu truyền đi theo từng gói tin (datagram) độc lập. Không đảm bảo gói tin sẽ đến nơi, không đảm bảo đúng thứ tự, nhưng tốc độ truyền rất nhanh (đảm bảo bởi UDP khi dùng mạng).
  * **Ứng dụng:** DNS, Livestreaming, VoIP, Game online, hoặc nhận sự kiện Netlink uevent.

* **`SOCK_RAW` (Raw Socket):**
  * **Đặc điểm:** Cho phép ứng dụng bỏ qua tầng vận chuyển (TCP/UDP) để can thiệp sâu hơn, tự đóng gói header của gói tin IP hoặc gói tin cấp thấp. Thường yêu cầu quyền root/admin để chạy.
  * **Ứng dụng:** Viết lệnh `ping` (gửi gói tin ICMP), hoặc công cụ quét mạng (Nmap).

### User Input

Ý tôi là Phân loại theo phạm vi giao tiếp, nó có vô tận không? Liệt kê ra tất cả dùm tôi được không?

*User accepted the command `grep -E "#define[[:space:]]+AF_" /usr/include/x86_64-linux-gnu/bits/socket.h || grep -E "#define[[:space:]]+AF_" /usr/include/sys/socket.h || true`*

### Planner Response

Các họ giao thức/địa chỉ (Address Family - `AF_`) **không vô tận**. Chúng là các giá trị số nguyên cố định (hằng số `const`) được định nghĩa cụ thể trong mã nguồn của nhân Linux và thư viện GNU C (`glibc`). 

Số lượng này là **hữu hạn** (hiện tại trên hệ thống của bạn có khoảng **45** họ giao thức đang được hỗ trợ).

Dưới đây là danh sách toàn bộ các họ địa chỉ `AF_` được lấy trực tiếp từ file cấu hình socket (`/usr/include/x86_64-linux-gnu/bits/socket.h`) trên máy của bạn, được chia theo các nhóm mục đích sử dụng:

---

### 1. Các nhóm phổ biến và quan trọng nhất (Dùng hàng ngày)
* **`AF_UNIX` / `AF_LOCAL` / `AF_FILE`**: Unix Domain Socket, dùng để truyền thông điệp cục bộ giữa các tiến trình trên cùng 1 máy.
* **`AF_INET`**: Giao thức mạng IPv4 (TCP, UDP).
* **`AF_INET6`**: Giao thức mạng IPv6.
* **`AF_NETLINK` / `AF_ROUTE`**: Giao tiếp giữa Kernel và User-space (như giám sát phần cứng, cấu hình IP/Routing).
* **`AF_PACKET`**: Cho phép đọc/ghi trực tiếp các gói tin thô ở tầng vật lý (Driver mạng), dùng cho Wireshark/Tcpdump.

### 2. Các nhóm giao thức mạng cổ điển (Hầu như không còn dùng)
Đây là các giao thức mạng phổ biến ở những thập niên 80-90 nhưng hiện tại đã bị thay thế hoàn toàn bởi TCP/IP:
* **`AF_APPLETALK`**: Giao thức mạng của hệ máy Apple Macintosh cũ.
* **`AF_IPX`**: Giao thức mạng Novell NetWare.
* **`AF_AX25` / `AF_NETROM` / `AF_ROSE`**: Giao thức dành cho mạng vô tuyến nghiệp dư (Amateur Radio / Packet Radio).
* **`AF_DECnet`**: Giao thức mạng của hãng Digital Equipment Corporation (DEC).
* **`AF_SNA`**: Kiến trúc mạng của IBM (Systems Network Architecture).
* **`AF_ECONET`**: Giao thức mạng của máy tính Acorn cũ.

### 3. Các nhóm phục vụ phần cứng & Công nghiệp nhúng
* **`AF_BLUETOOTH`**: Giao tiếp thông qua Bluetooth.
* **`AF_CAN`**: Giao tiếp thông qua mạng CAN Bus (Controller Area Network) - cực kỳ phổ biến trong công nghiệp ô tô và tự động hóa để các ECU giao tiếp với nhau.
* **`AF_NFC`**: Giao tiếp trường gần (Near Field Communication).
* **`AF_IRDA`**: Giao tiếp qua cổng hồng ngoại (Infrared).

### 4. Các nhóm chuyên dụng & Hiện đại (Mới được thêm vào sau này)
* **`AF_VSOCK`**: Dùng để giao tiếp siêu tốc giữa **Máy ảo (Virtual Machine)** và **Máy vật lý (Hypervisor)** mà không cần cấu hình mạng TCP/IP thông thường.
* **`AF_XDP`** (eXpress Data Path): Dạng socket hiệu năng cao cho phép bypass (bỏ qua) phần lớn network stack của nhân Linux để xử lý gói tin với tốc độ cực nhanh (thường dùng trong lọc mã độc DDoS).
* **`AF_ALG`**: Giao diện cho phép ứng dụng user-space sử dụng các thuật toán mã hóa (Crypto) chạy trực tiếp trong nhân Kernel để tăng tốc độ.
* **`AF_TIPC`** (Transparent Inter-Process Communication): Thiết kế riêng cho giao tiếp giữa các node trong một cụm máy chủ (Cluster).
* **`AF_IB`** (InfiniBand): Dành cho kết nối mạng băng thông cực cao và độ trễ cực thấp trong các siêu máy tính (HPC).

### 5. Các giao thức viễn thông & Khác
* `AF_ISDN`: Mạng điện thoại tích hợp số.
* `AF_PHONET`: Giao thức mạng cho thiết bị di động cũ của Nokia.
* `AF_CAIF`: Giao thức giao tiếp với modem di động trong cấu trúc CPU ST-Ericsson.
* `AF_IEEE802154`: Dùng cho các mạng không dây công suất thấp như Zigbee.
* `AF_PPPOX`: Giao thức PPP over Ethernet/ATM (thường dùng trong quay số cấu hình Modem ADSL/Cáp quang).

### User Input

tất cả đều dùng cấu trúc địa chỉ sockaddr_nl hả?

*Viewed [socket_default_server.c](file:///home/ledat/Documents/Linux_Command/IPC/socket_default/socket_default_server.c) *

### Planner Response

**Không phải**. Mỗi họ địa chỉ (`AF_`) sẽ có một **cấu trúc địa chỉ riêng biệt** phù hợp với cách định danh của giao thức đó. 

Chỉ có `AF_NETLINK` mới sử dụng `struct sockaddr_nl` (`nl` viết tắt của Netlink).

Dưới đây là các cấu trúc tương ứng cho các họ địa chỉ phổ biến:

| Họ địa chỉ (`AF_`) | Cấu trúc địa chỉ (`struct`) | Các trường thông tin chính bên trong |
| :--- | :--- | :--- |
| **`AF_NETLINK`** | **`struct sockaddr_nl`** | ID tiến trình (`nl_pid`), Nhóm multicast (`nl_groups`) |
| **`AF_UNIX` / `AF_LOCAL`** | **`struct sockaddr_un`** | Đường dẫn file socket (`sun_path`) <br>*(như bạn thấy ở dòng 81 trong file code của bạn)* |
| **`AF_INET`** (IPv4) | **`struct sockaddr_in`** | Địa chỉ IP 32-bit (`sin_addr`), Cổng/Port (`sin_port`) |
| **`AF_INET6`** (IPv6) | **`struct sockaddr_in6`** | Địa chỉ IP 128-bit (`sin6_addr`), Cổng/Port (`sin6_port`) |
| **`AF_PACKET`** | **`struct sockaddr_ll`** | Chỉ số card mạng (`sll_ifindex`), Giao thức (`sll_protocol`) |
| **`AF_CAN`** | **`struct sockaddr_can`** | Chỉ số mạng CAN (`can_ifindex`), ID nhận diện (`can_addr`) |

---

### Tại sao chúng lại khác nhau?
Bởi vì cách xác định "địa chỉ đích" của mỗi loại giao tiếp là hoàn toàn khác nhau:
* Để gửi dữ liệu qua **mạng IPv4**, bạn cần biết **IP và Port** (`sockaddr_in`).
* Để gửi dữ liệu cho một **tiến trình khác cùng máy**, bạn cần chỉ định **đường dẫn một file trên đĩa** (`sockaddr_un`).
* Để gửi dữ liệu tới **Kernel Netlink**, bạn cần chỉ định **ID tiến trình (PID) hoặc nhóm sự kiện** (`sockaddr_nl`).

---

### Điểm chung: Cấu trúc chung `struct sockaddr` (Casting)
Mặc dù các cấu trúc trên khác nhau về kích thước và các trường bên trong, nhưng tất cả các hàm hệ thống của Linux (như `bind()`, `accept()`, `connect()`) đều chỉ nhận một kiểu con trỏ chung duy nhất là:
```c
struct sockaddr *
```

Đây là cách lập trình C mô phỏng tính chất **đa hình (Polymorphism)**. Trong code của bạn (dòng 57 và 85):
* Bạn khai báo địa chỉ cụ thể là `struct sockaddr_un client_addr;` (vì bạn đang dùng UNIX domain socket).
* Nhưng khi truyền vào hàm `accept4()`, bạn phải ép kiểu nó về dạng chung: **`(struct sockaddr *)&client_addr`**. 
* Hệ điều hành sẽ nhìn vào trường đầu tiên của cấu trúc (`sun_family` hoặc `nl_family`) để tự động hiểu và ép kiểu ngược lại về cấu trúc thực tế phù hợp.

### User Input

ok, trở lại với socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);  -> vậy tôi sẽ nhận dữ liệu  thế nào đây?

*Viewed [socket_default_server.c](file:///home/ledat/Documents/Linux_Command/IPC/socket_default/socket_default_server.c) *

### Planner Response

Để nhận dữ liệu từ socket `NETLINK_KOBJECT_UEVENT`, bạn cần thực hiện 3 bước chính trong mã nguồn C:

1. **Khởi tạo và cấu hình địa chỉ Netlink (`bind`)** để đăng ký lắng nghe sự kiện.
2. **Nhận gói tin (`recv`)** từ socket vào một bộ đệm (buffer).
3. **Phân tích cú pháp (Parse dữ liệu)** vì gói tin uevent được cấu trúc bởi nhiều chuỗi ký tự kết thúc bằng ký tự `\0` (NULL) nối tiếp nhau.

Dưới đây là đoạn mã C hoàn chỉnh, đơn giản và dễ hiểu để bạn chạy thử nghiệm:

### Mã nguồn C mẫu lắng nghe Uevent

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/netlink.h> // Cần thiết cho struct sockaddr_nl và các hằng số Netlink

#define UEVENT_BUFFER_SIZE 2048

int main() {
    int sock_fd;
    struct sockaddr_nl src_addr;
    char buffer[UEVENT_BUFFER_SIZE];

    // 1. Tạo Netlink Socket
    sock_fd = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_KOBJECT_UEVENT);
    if (sock_fd < 0) {
        perror("socket");
        return -1;
    }

    // 2. Cấu hình địa chỉ nguồn sockaddr_nl
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid(); // Sử dụng PID của tiến trình hiện tại làm ID
    src_addr.nl_groups = 1;     // Lắng nghe Multicast group 1 (bắt buộc đối với uevent)

    // 3. Liên kết socket (bind)
    if (bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr)) < 0) {
        perror("bind");
        close(sock_fd);
        return -1;
    }

    printf("Đang lắng nghe sự kiện cắm/rút thiết bị (Uevents). Hãy thử cắm/rút USB hoặc thẻ nhớ...\n");

    // 4. Vòng lặp nhận dữ liệu
    while (1) {
        memset(buffer, 0, UEVENT_BUFFER_SIZE);
        
        // recv() sẽ block (ngủ) tại đây cho đến khi có sự kiện từ Kernel
        ssize_t len = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);
        if (len < 0) {
            perror("recv");
            break;
        }

        // 5. Phân tích dữ liệu nhận được
        printf("\n--- NHẬN ĐƯỢC SỰ KIỆN UEVENT (Độ dài: %ld bytes) ---\n", len);
        
        /* 
         * LƯU Ý QUAN TRỌNG: Dữ liệu trả về không phải là 1 chuỗi dài thông thường,
         * mà là tập hợp nhiều chuỗi ngắn được phân tách bằng ký tự null '\0'.
         * Ví dụ: "add@/dev...\0ACTION=add\0DEVNAME=sdb\0"
         */
        char *ptr = buffer;
        while (ptr < buffer + len) {
            if (strlen(ptr) > 0) {
                printf("%s\n", ptr); // In ra từng dòng cấu hình
            }
            ptr += strlen(ptr) + 1; // Nhảy con trỏ qua ký tự '\0' kế tiếp
        }
    }

    close(sock_fd);
    return 0;
}
```

---

### Giải thích cách xử lý dữ liệu ở bước 5:
Đoạn code:
```c
char *ptr = buffer;
while (ptr < buffer + len) {
    printf("%s\n", ptr);
    ptr += strlen(ptr) + 1;
}
```
* **Tại sao không `printf("%s", buffer)`?** Vì nếu chỉ in `buffer` trực tiếp bằng `%s`, trình biên dịch sẽ chỉ in đến ký tự `\0` đầu tiên rồi dừng lại. Bạn sẽ bỏ lỡ toàn bộ thông tin quan trọng phía sau (như `ACTION=...`, `DEVNAME=...`).
* **Cách giải quyết:** Đoạn code trên duyệt từ đầu bộ đệm đến cuối gói tin, cứ in xong một chuỗi thì cộng độ dài của chuỗi đó cộng thêm `1` (để bỏ qua ký tự `\0`) để chuyển sang chuỗi tiếp theo.

### User Input

là nhận từ recv() , còn cách nào khác không?

### Planner Response

Ngoài hàm `recv()` thông dụng, vì socket trong Linux thực chất vẫn là một **File Descriptor (FD)**, bạn có rất nhiều cách khác để nhận dữ liệu, từ mức độ cơ bản đến nâng cao. 

Dưới đây là các phương pháp thay thế:

---

### 1. Dùng hàm đọc file tiêu chuẩn: `read()`
Do socket cũng là một file descriptor, bạn có thể đọc nó giống như đọc một file thông thường trên ổ đĩa.
```c
ssize_t len = read(sock_fd, buffer, sizeof(buffer) - 1);
```
* **So sánh với `recv()`:** `read()` hoàn toàn tương đương với việc gọi `recv(sock_fd, buffer, len, 0)` (tức là gọi `recv` với flag bằng 0). 
* **Hạn chế:** `read()` không có tham số `flags` như `recv()` nên bạn không thể cấu hình các chế độ đọc đặc biệt (ví dụ: `MSG_DONTWAIT` để đọc không chặn, hoặc `MSG_PEEK` để xem trước dữ liệu mà không xóa khỏi hàng đợi).

---

### 2. Dùng hàm lấy thông tin người gửi: `recvfrom()`
Hàm này thường dùng cho giao thức không hướng kết nối (như UDP hoặc Netlink). Nó cho phép bạn vừa đọc dữ liệu vừa lấy thông tin của bên gửi.
```c
struct sockaddr_nl sender_addr;
socklen_t addr_len = sizeof(sender_addr);

ssize_t len = recvfrom(sock_fd, buffer, sizeof(buffer) - 1, 0,
                       (struct sockaddr *)&sender_addr, &addr_len);
```
* **Ứng dụng:** Bạn có thể kiểm tra xem gói tin uevent có thực sự đến từ Kernel hay không bằng cách check:
  ```c
  if (sender_addr.nl_pid == 0) { // PID = 0 nghĩa là gói tin gửi từ Kernel
      // Xử lý gói tin tin cậy
  }
  ```

---

### 3. Dùng hàm vạn năng: `recvmsg()`
Đây là hàm nhận dữ liệu nâng cao và mạnh mẽ nhất trong lập trình Socket Linux. Nó sử dụng cấu trúc `struct msghdr`.
```c
struct iovec iov;
struct msghdr msg;
struct sockaddr_nl sender_addr;

iov.iov_base = buffer;
iov.iov_len = sizeof(buffer);

memset(&msg, 0, sizeof(msg));
msg.msg_name = &sender_addr;
msg.msg_namelen = sizeof(sender_addr);
msg.msg_iov = &iov;
msg.msg_iovlen = 1;

ssize_t len = recvmsg(sock_fd, &msg, 0);
```
* **Ưu điểm:** 
  * Cho phép đọc dữ liệu phân mảnh vào nhiều bộ đệm khác nhau (Scatter-Gather I/O).
  * Nhận được các thông tin điều khiển phụ (Ancillary data / Control messages) đi kèm với gói tin (ví dụ: thông tin chứng thực của tiến trình gửi).

---

### 4. Kết hợp với Cơ chế Giám sát I/O (Multiplexing)
Trong thực tế, người ta ít khi gọi trực tiếp `recv()` hay `read()` ở luồng chính (Main thread) vì nó sẽ làm treo chương trình khi không có sự kiện. Thay vào đó, họ sẽ kết hợp socket với:
* **`select()` / `poll()` / `epoll()`**: Các cơ chế này cho phép bạn giám sát cùng lúc nhiều File Descriptor (ví dụ: vừa đợi thẻ nhớ cắm vào qua Netlink, vừa đợi dữ liệu mạng TCP từ client, vừa đợi input từ bàn phím).
* **Cách chạy:** Bạn đăng ký Socket Netlink vào vòng lặp `poll()` hoặc `epoll()`. Khi nào Kernel báo có dữ liệu sẵn sàng trên socket này, bạn mới gọi `recv()` hoặc `read()` để lấy dữ liệu ra xử lý.

---

### 5. Cơ chế bất đồng bộ hiện đại nhất: `io_uring`
Với các nhân Linux mới gần đây, bạn có thể dùng `io_uring` để thực hiện việc đọc socket một cách bất đồng bộ hoàn toàn (Asynchronous). 
* Bạn chỉ cần gửi một yêu cầu đọc (read request) vào hàng đợi (Submission Queue), sau đó tiếp tục làm việc khác. 
* Khi có dữ liệu, Kernel sẽ tự động xử lý và báo về hàng đợi hoàn thành (Completion Queue). Đây là cơ chế nhanh nhất hiện nay trên Linux.