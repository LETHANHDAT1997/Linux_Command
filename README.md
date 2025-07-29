# 🐧 Hướng Dẫn Toàn Diện về Dòng Lệnh Linux

Hướng dẫn này cung cấp một cái nhìn tổng quan toàn diện về dòng lệnh Linux, từ cấu trúc hệ thống tệp cơ bản đến các lệnh nâng cao và các khái niệm cốt lõi. Nó được thiết kế cho cả người mới bắt đầu và những người muốn củng cố kiến thức của mình để trở thành một người dùng thành thạo.

---

## 📂 Hệ Thống Phân Cấp Tệp Tin Linux

Hệ thống tệp của Linux có cấu trúc dạng cây bắt đầu từ thư mục gốc (`/`). Mọi tệp và thư mục trên hệ thống của bạn đều nằm dưới thư mục gốc duy nhất này.

| Thư mục | Mô tả |
| :--- | :--- |
| **`/`** | **Thư mục gốc (Root Directory)**: Thư mục cấp cao nhất của toàn bộ hệ thống tệp. |
| **`/bin`** | **Tệp nhị phân thiết yếu của người dùng (Essential User Binaries)**: Chứa các chương trình thực thi cần thiết để hệ thống hoạt động, có sẵn cho tất cả người dùng. |
| **`/boot`** | **Tệp của bộ tải khởi động (Boot Loader Files)**: Chứa các tệp cần thiết để khởi động hệ điều hành, bao gồm cả nhân Linux. |
| **`/etc`** | **Tệp cấu hình (Configuration Files)**: Lưu giữ các tệp cấu hình toàn hệ thống cho các chương trình và dịch vụ đã cài đặt. |
| **`/home`** | **Thư mục chính của người dùng (User Home Directories)**: Chứa một thư mục chính cho mỗi người dùng trên hệ thống (ví dụ: `/home/jason`). Đây là nơi người dùng lưu trữ các tệp cá nhân của họ. |
| **`/lib`** | **Thư viện chia sẻ thiết yếu (Essential Shared Libraries)**: Chứa các thư viện cần thiết cho các tệp nhị phân trong `/bin` và `/sbin`. |
| **`/media`** | **Thiết bị di động (Removable Media)**: Điểm gắn kết (mount point) tiêu chuẩn cho các thiết bị di động như ổ USB, CD-ROM và DVD. |
| **`/mnt`** | **Điểm gắn kết tạm thời (Temporary Mount Point)**: Một điểm gắn kết chung để gắn tạm thời các hệ thống tệp. |
| **`/opt`** | **Phần mềm tùy chọn (Optional Software)**: Được sử dụng để cài đặt phần mềm của bên thứ ba không đi kèm với hệ điều hành. |
| **`/proc`** | **Thông tin tiến trình (Process Information)**: Một hệ thống tệp ảo cung cấp thông tin thời gian thực về các tiến trình hệ thống (PID) và trạng thái nhân. |
| **`/root`** | **Thư mục chính của người dùng Root (Root User's Home Directory)**: Thư mục chính của người dùng `root` (siêu người dùng), khác với thư mục gốc của hệ thống (`/`). |
| **`/sbin`** | **Tệp nhị phân hệ thống (System Binaries)**: Chứa các tệp thực thi quản trị hệ thống thiết yếu, thường được sử dụng bởi người dùng `root`. |
| **`/tmp`** | **Tệp tạm thời (Temporary Files)**: Một thư mục để lưu trữ các tệp tạm thời. Các tệp này thường bị xóa khi hệ thống khởi động lại. |
| **`/usr`** | **Chương trình và dữ liệu người dùng (User Programs and Data)**: Một thư mục lớn chứa các chương trình, thư viện và tài liệu do người dùng cài đặt. |
| **`/var`** | **Tệp biến đổi (Variable Files)**: Lưu trữ các tệp có kích thước dự kiến sẽ tăng lên, chẳng hạn như nhật ký (`/var/log`), bộ đệm và các tệp chờ xử lý (spool files). |

---

## 💻 Hiểu về Dấu Nhắc Lệnh (Command Prompt)

Dấu nhắc lệnh cung cấp thông tin chính về phiên làm việc hiện tại của bạn.

-   **`[jason@linuxsvr ~]$`**:
    -   `jason`: Tên người dùng hiện tại.
    -   `linuxsvr`: Tên máy chủ (hostname) của hệ thống.
    -   `~`: Thư mục hiện tại. Dấu ngã (`~`) là một lối tắt cho thư mục chính của người dùng hiện tại (ví dụ: `/home/jason`).
    -   `$`: Cho biết đây là một phiên làm việc của người dùng thông thường.

-   **`[root@linuxsvr ~]#`**:
    -   `root`: Người dùng hiện tại là siêu người dùng (quản trị viên).
    -   `#`: Cho biết đây là một phiên làm việc của siêu người dùng (root), có các đặc quyền cao hơn.

---

## 🧭 Điều Hướng Cơ Bản và Quản Lý Tệp

Các lệnh này là nền tảng để tương tác với hệ thống tệp. Hãy nhớ, **lệnh Linux phân biệt chữ hoa và chữ thường**.

### `pwd` (Print Working Directory)
Hiển thị đường dẫn đầy đủ của thư mục hiện tại.

### `cd` (Change Directory)
Di chuyển giữa các thư mục.
-   `cd /home/jason/documents`: Di chuyển đến thư mục được chỉ định.
-   `cd ..`: Di chuyển đến thư mục cha.
-   `cd ~` hoặc `cd`: Quay trở lại thư mục chính của bạn.
-   `cd -`: Quay trở lại thư mục trước đó bạn đã ở.

### `ls` (List)
Liệt kê các tệp và thư mục.
-   `ls -l`: Liệt kê ở định dạng dài, hiển thị quyền, chủ sở hữu, kích thước và ngày sửa đổi.
-   `ls -a`: Liệt kê tất cả các tệp, bao gồm cả các tệp ẩn (bắt đầu bằng dấu `.`).
-   `ls -h`: Sử dụng với `-l`, hiển thị kích thước tệp ở định dạng dễ đọc (ví dụ: KB, MB, GB).
-   `ls -R`: Liệt kê nội dung của thư mục và tất cả các thư mục con của nó một cách đệ quy.
-   `ls -F`: Thêm một chỉ báo vào cuối tên (`/` cho thư mục, `*` cho tệp thực thi, `@` cho liên kết tượng trưng).

### `tree`
Hiển thị cấu trúc cây của các thư mục và tệp.
-   `tree -d`: Chỉ hiển thị các thư mục.
-   `tree -C`: Thêm màu sắc để dễ đọc hơn.

### `mkdir` (Make Directory)
Tạo thư mục mới.
-   `mkdir my_project`
-   `mkdir -p project/src/include`: Tạo toàn bộ đường dẫn thư mục, bao gồm cả các thư mục cha nếu chúng chưa tồn tại.

### `rmdir` (Remove Directory)
Xóa các thư mục **rỗng**.

### `rm` (Remove)
Xóa tệp và thư mục. **Sử dụng hết sức thận trọng.**
-   `rm myfile.txt`: Xóa một tệp.
-   `rm -r my_directory`: Xóa một thư mục và tất cả nội dung của nó một cách đệ quy.
-   `rm -f`: Buộc xóa mà không cần yêu cầu xác nhận.
-   `rm -rf /`: **Đây là một lệnh có thể gây thảm họa.** Nó sẽ cố gắng xóa mọi thứ trên hệ thống của bạn. **Không bao giờ chạy lệnh này.**

### `cp` (Copy)
Sao chép tệp và thư mục.
-   `cp source.txt destination.txt`: Sao chép một tệp.
-   `cp -r source_directory/ destination_directory/`: Sao chép một thư mục một cách đệ quy.
-   `cp -i`: Chế độ tương tác, hỏi trước khi ghi đè.

### `mv` (Move)
Di chuyển hoặc đổi tên tệp và thư mục.
-   `mv old_name.txt new_name.txt`: Đổi tên một tệp.
-   `mv my_file.txt /home/jason/documents/`: Di chuyển một tệp đến một thư mục khác.

---
## ✨ Ký Tự Đại Diện (Wildcards)

Wildcards là các ký tự đặc biệt giúp bạn khớp với tên tệp và thư mục, rất hữu ích khi làm việc với nhiều tệp cùng lúc.

| Ký tự | Tên | Mô tả | Ví dụ |
| :--- | :--- | :--- | :--- |
| **`*`** | Dấu sao (Asterisk) | Khớp với bất kỳ chuỗi ký tự nào (kể cả rỗng). | `ls *.log` (liệt kê tất cả các tệp có đuôi `.log`). |
| **`?`** | Dấu chấm hỏi | Khớp với một ký tự đơn lẻ bất kỳ. | `ls file?.txt` (khớp với `file1.txt`, `fileA.txt` nhưng không khớp `file10.txt`). |
| **`[ ]`** | Dấu ngoặc vuông | Khớp với bất kỳ ký tự nào bên trong ngoặc. | `ls report[1-3].pdf` (khớp với `report1.pdf`, `report2.pdf`, `report3.pdf`). |
| **`{ }`** | Dấu ngoặc nhọn | Mở rộng thành nhiều chuỗi riêng biệt. | `mkdir {source,build,docs}` (tạo ba thư mục `source`, `build`, và `docs`). |

---

## ⚙️ Biến Môi Trường và Tìm Kiếm Lệnh

### Biến Môi Trường (Environment Variables)
Đây là các biến động chứa thông tin mà các tiến trình có thể sử dụng. Tên biến thường được viết hoa.

#### `echo $VAR_NAME`
Sử dụng lệnh `echo` với tiền tố `$` để in ra giá trị của một biến môi trường.

#### `echo $PATH`
**`PATH`** là một trong những biến môi trường quan trọng nhất. Nó chứa một danh sách các thư mục được phân tách bằng dấu hai chấm (`:`). Khi bạn nhập một lệnh (ví dụ `ls`) mà không chỉ định đường dẫn đầy đủ (`/bin/ls`), shell sẽ tìm kiếm tệp thực thi của lệnh đó trong từng thư mục được liệt kê trong biến `PATH`, theo thứ tự từ trái sang phải.

-   **Ví dụ về cách hoạt động**:
    1.  Bạn gõ lệnh `ls` và nhấn Enter.
    2.  Shell kiểm tra biến `$PATH`. Giả sử giá trị là: `/usr/local/bin:/usr/bin:/bin`.
    3.  Shell tìm kiếm `ls` trong `/usr/local/bin`. Không tìm thấy.
    4.  Shell tìm kiếm `ls` trong `/usr/bin`. Không tìm thấy.
    5.  Shell tìm kiếm `ls` trong `/bin`. Tìm thấy tệp `/bin/ls`!
    6.  Shell thực thi `/bin/ls` và dừng tìm kiếm.

### `which [lệnh]`
Lệnh `which` cho bạn biết đường dẫn tuyệt đối đến tệp thực thi sẽ được chạy khi bạn gõ một lệnh. Nó thực hiện việc tìm kiếm trong các thư mục của biến `PATH` giống như shell.

-   **Ví dụ**:
    ```bash
    $ which ls
    /bin/ls

    $ which python3
    /usr/bin/python3
    ```
    Điều này rất hữu ích để xác định phiên bản nào của một chương trình đang được sử dụng nếu có nhiều phiên bản được cài đặt.

---

## 📜 Xem và Chỉnh Sửa Tệp

Các lệnh để xem nội dung của tệp.

### `cat` (Concatenate)
Đọc các tệp và in nội dung của chúng ra đầu ra tiêu chuẩn.
-   `cat file.txt`: Hiển thị nội dung của `file.txt`.
-   `cat file1.txt file2.txt`: Hiển thị nội dung của cả hai tệp.
-   `cat file1.txt file2.txt > new_file.txt`: Hợp nhất hai tệp thành một tệp mới.
-   `cat file1.txt >> existing_file.txt`: Nối nội dung của `file1.txt` vào cuối `existing_file.txt`.
-   `cat > new_file.txt`: Tạo một tệp mới; nhập văn bản của bạn và nhấn `CTRL + D` để lưu.

### `less`
Một trình xem trang cho phép bạn xem nội dung tệp từng màn hình một. Nó tiên tiến hơn `more`.
-   `less /var/log/syslog`: Sử dụng các phím mũi tên, Page Up/Down để điều hướng. Nhấn `/` để tìm kiếm và `q` để thoát.

### `head` & `tail`
Xem phần đầu hoặc phần cuối của một tệp.
-   `head -n 20 file.txt`: Hiển thị 20 dòng đầu tiên.
-   `tail -n 20 file.txt`: Hiển thị 20 dòng cuối cùng.
-   `tail -f /var/log/syslog`: Theo dõi tệp, hiển thị các dòng mới khi chúng được thêm vào trong thời gian thực. Cực kỳ hữu ích để xem các tệp nhật ký trực tiếp.

---

## 🔎 Tìm Kiếm Tệp và Văn Bản (Find & Grep)

### `find`
Một công cụ mạnh mẽ để tìm kiếm tệp và thư mục dựa trên nhiều tiêu chí.
-   `find [đường_dẫn] [biểu_thức]`
-   `find /home/jason -name "*.log"`: Tìm tất cả các tệp có đuôi `.log` trong thư mục chính của Jason.
-   `find . -type d`: Tìm tất cả các thư mục ở vị trí hiện tại.
-   `find / -size +1G`: Tìm tất cả các tệp lớn hơn 1 Gigabyte.
-   `find . -mtime -7`: Tìm các tệp đã được sửa đổi trong vòng 7 ngày qua.
-   `find . -name "*.c" -exec grep -l "main" {} \;`: Tìm tất cả các tệp `.c` và thực thi lệnh `grep` trên chúng để tìm những tệp chứa từ "main".

### `grep` (Global Regular Expression Print)
Tìm kiếm các mẫu văn bản cụ thể bên trong các tệp.
-   `grep "error" /var/log/syslog`: Tìm kiếm và hiển thị tất cả các dòng chứa từ "error" trong nhật ký hệ thống.
-   `grep -i "linux" notes.txt`: Tìm kiếm không phân biệt chữ hoa chữ thường.
-   `grep -r "API_KEY" /etc/`: Tìm kiếm đệ quy "API_KEY" trong tất cả các tệp dưới `/etc/`.
-   `grep -v "DEBUG"` app.log: Hiển thị tất cả các dòng KHÔNG chứa từ "DEBUG".
-   `ps aux | grep "nginx"`: Chuyển đầu ra của `ps` sang `grep` để lọc và tìm tiến trình nginx.

---

## 🚀 Chuyển Hướng Đầu Vào/Đầu Ra và Đường Ống (Pipes)

Kiểm soát nơi đầu vào của lệnh đến từ đâu và đầu ra đi đến đâu. Linux có 3 luồng dữ liệu tiêu chuẩn:
1.  **Standard Input (stdin)** - File Descriptor 0
2.  **Standard Output (stdout)** - File Descriptor 1
3.  **Standard Error (stderr)** - File Descriptor 2

| Toán tử | Tên | Mô tả | Ví dụ |
| :--- | :--- | :--- | :--- |
| **`>`** | Chuyển hướng stdout | Gửi đầu ra đến một tệp, **ghi đè** lên nó. | `ls -l > file_list.txt` |
| **`>>`** | Nối stdout | Gửi đầu ra đến một tệp, **nối** vào cuối. | `echo "Log entry" >> app.log` |
| **`<`** | Chuyển hướng stdin | Lấy đầu vào từ một tệp thay vì bàn phím. | `sort < unsorted_list.txt` |
| **`2>`** | Chuyển hướng stderr | Gửi thông báo lỗi (stderr) đến một tệp. | `bad_command 2> errors.log` |
| **`2>>`** | Nối stderr | Nối thông báo lỗi vào một tệp. | `bad_command 2>> errors.log` |
| **`&>`** | Chuyển hướng tất cả | Gửi cả stdout và stderr đến một tệp. | `script.sh &> output_and_errors.log` |
| **`|`** | Đường ống (Pipe) | Gửi stdout của một lệnh làm stdin cho lệnh khác. | `cat access.log | grep "404" | wc -l` |
| **`/dev/null`** | Thiết bị Null | Một tệp đặc biệt loại bỏ tất cả dữ liệu được ghi vào nó. | `cron_job.sh > /dev/null 2>&1` |

---

## 🔒 Quyền Đối Với Tệp và Thư Mục

Linux là một hệ thống đa người dùng, và quyền hạn xác định ai có thể làm gì với các tệp và thư mục.

### Hiểu về Quyền Hạn

Khi bạn chạy `ls -l`, bạn sẽ thấy một chuỗi như `drwxr-xr--`.

-   **Ký tự đầu tiên**: Loại tệp (`d` cho thư mục, `-` cho tệp thông thường, `l` cho liên kết tượng trưng).
-   **9 ký tự tiếp theo**: Quyền hạn, được nhóm thành ba:
    1.  **User (Chủ sở hữu)**: `rwx` (Đọc, Ghi, Thực thi)
    2.  **Group**: `r-x` (Đọc, Thực thi)
    3.  **Others**: `r--` (Chỉ đọc)

### `chmod` (Change Mode)
Sửa đổi quyền của tệp.
-   **Phương pháp tượng trưng (Symbolic Method)**:
    -   `u` (người dùng), `g` (nhóm), `o` (những người khác), `a` (tất cả)
    -   `+` (thêm), `-` (loại bỏ), `=` (thiết lập chính xác)
    -   `r` (đọc), `w` (ghi), `x` (thực thi)
    -   *Ví dụ*: `chmod u+x script.sh` (thêm quyền thực thi cho người dùng).
-   **Phương pháp bát phân (Octal Method)**:
    -   `4` = Đọc, `2` = Ghi, `1` = Thực thi
    -   Cộng các số để có quyền kết hợp (ví dụ: `rwx` = 4+2+1 = `7`).
    -   *Ví dụ*: `chmod 755 script.sh` tương đương với `rwxr-xr-x`.
-   **Tùy chọn đệ quy**: `chmod -R 644 my_directory/` áp dụng quyền cho tất cả tệp và thư mục con.

### `chown` (Change Owner)
Thay đổi quyền sở hữu người dùng và/hoặc nhóm của một tệp hoặc thư mục.
-   `sudo chown jason:developers script.sh`: Thay đổi chủ sở hữu thành `jason` và nhóm thành `developers`.
-   `sudo chown -R jason:jason /home/jason/my_project`: Thay đổi quyền sở hữu một cách đệ quy cho toàn bộ thư mục.

---

## 👥 Quản Lý Người Dùng và Nhóm

Đây là các lệnh cần thiết để quản lý tài khoản người dùng và nhóm. Hầu hết các lệnh này yêu cầu quyền `sudo`.

### `useradd` / `adduser`
Tạo một người dùng mới. `adduser` là một phiên bản thân thiện hơn của `useradd` trên các hệ thống Debian/Ubuntu.
- `sudo useradd -m -s /bin/bash -g users alice`
    - `-m`: Tạo thư mục nhà (`/home/alice`).
    - `-s /bin/bash`: Đặt shell đăng nhập mặc định.
    - `-g users`: Thêm người dùng vào nhóm chính `users`.
- `sudo adduser bob`: Lệnh này sẽ hỏi bạn một cách tương tác để đặt mật khẩu và các thông tin khác.

### `passwd`
Thay đổi mật khẩu cho người dùng.
- `sudo passwd alice`: Đặt hoặc thay đổi mật khẩu cho người dùng `alice`.
- `passwd`: Người dùng đang đăng nhập có thể tự thay đổi mật khẩu của mình.

### `usermod`
Sửa đổi thông tin của một người dùng hiện có.
- `sudo usermod -aG sudo bob`: Thêm người dùng `bob` vào nhóm `sudo` (cấp quyền quản trị). `-aG` có nghĩa là "append to group".
- `sudo usermod -l charlie bob`: Đổi tên đăng nhập từ `bob` thành `charlie`.

### `userdel`
Xóa một người dùng.
- `sudo userdel alice`: Xóa người dùng `alice`.
- `sudo userdel -r charlie`: Xóa người dùng `charlie` và cả thư mục nhà của anh ta (`/home/charlie`).

### `groupadd` & `groupdel`
Tạo hoặc xóa một nhóm người dùng.
- `sudo groupadd developers`
- `sudo groupdel developers`

---

## 📦 Quản Lý Gói Phần Mềm

Trình quản lý gói giúp bạn dễ dàng cài đặt, cập nhật, và gỡ bỏ phần mềm trên hệ thống.

### Debian/Ubuntu (sử dụng APT)
- **`sudo apt update`**: Cập nhật danh sách các gói có sẵn từ kho chứa.
- **`sudo apt upgrade`**: Nâng cấp tất cả các gói đã cài đặt lên phiên bản mới nhất.
- **`sudo apt install htop`**: Cài đặt một gói phần mềm (ví dụ: `htop`).
- **`sudo apt remove htop`**: Gỡ bỏ một gói phần mềm.
- **`sudo apt autoremove`**: Gỡ bỏ các gói phụ thuộc không còn cần thiết.
- **`apt search "web server"`**: Tìm kiếm các gói liên quan đến "web server".

### CentOS/Fedora/RHEL (sử dụng YUM/DNF)
(DNF là phiên bản hiện đại hơn của YUM)
- **`sudo dnf check-update`**: Kiểm tra các bản cập nhật có sẵn.
- **`sudo dnf upgrade`**: Nâng cấp tất cả các gói.
- **`sudo dnf install htop`**: Cài đặt một gói.
- **`sudo dnf remove htop`**: Gỡ bỏ một gói.
- **`sudo dnf autoremove`**: Gỡ bỏ các gói phụ thuộc không cần thiết.
- **`dnf search "web server"`**: Tìm kiếm một gói.

---

## 📈 Giám Sát và Phân Tích Hệ Thống

Các công cụ này giúp bạn theo dõi hiệu suất và chẩn đoán sự cố.

### `top` / `htop`
Hiển thị một cái nhìn tương tác, thời gian thực về các tiến trình hệ thống, mức sử dụng CPU, RAM. `htop` có giao diện màu sắc và dễ sử dụng hơn.

### `df` (Disk Free) & `du` (Disk Usage)
- `df -h`: Hiển thị dung lượng trống và đã sử dụng của các phân vùng đĩa ở định dạng dễ đọc.
- `du -sh /var/log`: Hiển thị tổng dung lượng của một thư mục cụ thể.

### `free`
Hiển thị lượng bộ nhớ (RAM) và swap đã sử dụng và còn trống.
- `free -h`: Hiển thị ở định dạng dễ đọc (MB, GB).

### `ps` (Process Status)
Liệt kê các tiến trình đang chạy.
- `ps aux`: Hiển thị tất cả các tiến trình đang chạy trên hệ thống.
- `ps -ef | grep nginx`: Một cách khác để tìm kiếm một tiến trình cụ thể.

### `kill`
Gửi tín hiệu đến một tiến trình, thường là để chấm dứt nó.
- `kill <PID>`: Gửi tín hiệu chấm dứt mặc định (TERM) đến tiến trình có ID là `<PID>`.
- `kill -9 <PID>`: Buộc chấm dứt một tiến trình không phản hồi (tín hiệu KILL).

### `dmesg`
In ra các thông điệp từ bộ đệm của nhân (kernel). Rất hữu ích để chẩn đoán các sự cố phần cứng hoặc driver ngay sau khi khởi động.
- `dmesg | grep -i "error"`: Lọc các thông điệp lỗi từ kernel.

### `lsof` (List Open Files)
Liệt kê các tệp đang được mở bởi các tiến trình.
- `sudo lsof -i :80`: Hiển thị tiến trình nào đang sử dụng cổng 80.

---

## 📦 Nén và Lưu Trữ

### `tar` (Tape Archive)
Gộp nhiều tệp thành một tệp lưu trữ duy nhất. Nó **không** nén theo mặc định.
-   `tar -cvf archive.tar file1 file2 dir1/`: **Tạo** một kho lưu trữ `.tar`.
-   `tar -xvf archive.tar`: **Giải nén** từ một kho lưu trữ `.tar`.
-   `tar -tvf archive.tar`: **Liệt kê** nội dung của một kho lưu trữ `.tar`.

### `gzip` & `gunzip`
Công cụ nén phổ biến nhất, thường được sử dụng với `tar`.
-   `gzip myfile.txt`: Nén tệp thành `myfile.txt.gz` (và xóa tệp gốc).
-   `gunzip myfile.txt.gz`: Giải nén tệp trở lại thành `myfile.txt`.

### Kết hợp `tar` và `gzip`
Rất phổ biến để tạo một tệp tar được nén bằng gzip (`.tar.gz` hoặc `.tgz`).
-   `tar -czvf archive.tar.gz directory/`: **Tạo** và **nén gzip** một kho lưu trữ.
-   `tar -xzvf archive.tar.gz`: **Giải nén** từ một kho lưu trữ đã được nén gzip.

---

## 🌐 Mạng (Networking)

Các lệnh cơ bản để quản lý và chẩn đoán kết nối mạng.

### `ip` / `ifconfig`
- `ip addr` hoặc `ifconfig`: Hiển thị thông tin giao diện mạng và địa chỉ IP. `ip` là lệnh hiện đại và được khuyến nghị hơn.
- `ip route`: Hiển thị bảng định tuyến của hệ thống.

### `ping`
Kiểm tra kết nối đến một máy chủ từ xa bằng cách gửi các gói tin ICMP.
- `ping google.com`
- `ping -c 4 8.8.8.8`: Chỉ gửi 4 gói tin đến địa chỉ IP 8.8.8.8.

### `netstat` / `ss`
Hiển thị các kết nối mạng, bảng định tuyến, thống kê giao diện... `ss` là lệnh thay thế hiện đại hơn cho `netstat`.
- `ss -tuln`: Hiển thị tất cả các cổng TCP (t) và UDP (u) đang lắng nghe (l) dưới dạng số (n).
- `netstat -tuln`: Tương tự như trên.

### `ssh` (Secure Shell)
Kết nối đến một máy chủ từ xa một cách an toàn.
- `ssh user@hostname`: Kết nối đến máy chủ `hostname` với tư cách người dùng `user`.
- `scp file.txt user@hostname:/remote/path/`: Sao chép an toàn tệp `file.txt` đến một máy chủ từ xa.

### `curl` / `wget`
Các công cụ dòng lệnh để tải xuống nội dung từ web.
- `curl http://example.com`: Hiển thị nội dung của URL ra màn hình.
- `wget http://example.com/file.zip`: Tải tệp `file.zip` về thư mục hiện tại.

---

## 🤖 Tự Động Hóa Tác Vụ với Cron

`cron` là một trình nền (daemon) cho phép bạn lập lịch chạy các lệnh hoặc script một cách tự động vào những thời điểm xác định.

### `crontab`
Lệnh để quản lý các công việc cron của người dùng.
- `crontab -e`: Chỉnh sửa tệp crontab của người dùng hiện tại.
- `crontab -l`: Liệt kê các công việc cron hiện có.
- `crontab -r`: Xóa tất cả các công việc cron.

### Cú pháp Crontab
Mỗi dòng trong tệp crontab đại diện cho một công việc và có 6 trường: