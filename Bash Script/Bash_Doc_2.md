Dưới đây là danh sách chi tiết và đầy đủ nhất toàn bộ các lệnh, hàm, ký hiệu, toán tử và biến đặc biệt được liệt kê trong tài liệu của bạn:

### 1. Lệnh nội bộ Bash (Built-in commands)
* [cite_start]**Điều khiển luồng:** `if`, `then`, `else`, `elif`, `fi`, `case`, `esac`, `for`, `while`, `until`, `do`, `done`, `break`, `continue`[cite: 101, 110, 121].
* [cite_start]**Hàm & Scope:** `function`, `return`, `declare`, `local`, `readonly`[cite: 101].
* [cite_start]**Biến & Môi trường:** `export`, `unset`, `set`, `env`[cite: 101, 107].
* [cite_start]**I/O & Shell control:** `echo`, `printf`, `read`, `exec`, `exit`, `source`, `.`[cite: 101].
* [cite_start]**Job control:** `jobs`, `fg`, `bg`, `wait`, `kill`[cite: 101].

### 2. Lệnh hệ thống (External commands)
* [cite_start]**File & Thư mục:** `ls`, `cp`, `mv`, `rm`, `mkdir`, `touch`[cite: 102].
* [cite_start]**Xử lý văn bản:** `cat`, `grep`, `awk`, `sed`, `cut`, `sort`, `uniq`, `tr`[cite: 103, 120].
* [cite_start]**Hệ thống:** `ps`, `top`, `kill`, `df`, `du`, `free`[cite: 102, 119].
* [cite_start]**Mạng (Network):** `curl`, `wget`, `ping`, `ssh`[cite: 102].

### 3. Hàm cấp thấp (Hệ điều hành/C/Python)
* [cite_start]**Quản lý tiến trình:** `fork()` [cite: 113, 117, 184, 185][cite_start], `popen()`[cite: 113, 117, 184, 185, 192, 193, 194].
* [cite_start]**Quản lý tệp/I/O:** `open()` [cite: 115, 118, 169, 173, 176, 177, 179, 184, 188, 191, 192, 195][cite_start], `close()`[cite: 115, 118, 169, 178, 179, 190, 195].

### 4. Ký hiệu Điều hướng & Pipe (Redirection)
* [cite_start]`>` : Ghi đè[cite: 104, 124].
* `>>` : Ghi tiếp (append).
* [cite_start]`<` : Đọc input[cite: 104, 124].
* `2>` : Chuyển hướng stderr.
* `&>` : Chuyển hướng cả stdout và stderr.
* `<<` : Heredoc.
* [cite_start]`|` : Truyền output lệnh trước làm input lệnh sau[cite: 104, 124].
* `|&` : Truyền cả stderr qua pipe.

### 5. Các Toán tử (Operators)
* [cite_start]**Logic:** `&&` (AND), `||` (OR), `!` (NOT)[cite: 105, 125].
* **Số học:** `$(( ))`, `+`, `-`, `*`, `/`, `%`.
* **So sánh chuỗi:** `=`, `!=`, `<`, `>`.
* **So sánh số:** `-eq` (bằng), `-ne` (khác), `-lt` (nhỏ hơn), `-le` (nhỏ hơn hoặc bằng), `-gt` (lớn hơn), `-ge` (lớn hoặc bằng).
* **Kiểm tra file:** `-f` (file), `-d` (directory), `-e` (tồn tại), `-r` (đọc), `-w` (ghi), `-x` (thực thi).

### 6. Toàn bộ Biến đặc biệt (Special Variables)
* [cite_start]`$0` : Tên script đang chạy[cite: 108, 129].
* [cite_start]`$1..$9` : Tham số truyền vào từ 1 đến 9[cite: 108, 130].
* `$#` : Số lượng tham số truyền vào.
* [cite_start]`$@` : Tất cả tham số[cite: 108, 130].
* `$*` : Tất cả tham số (gom chung).
* [cite_start]`$?` : Exit code (mã trả về của lệnh vừa chạy)[cite: 108, 131].
* [cite_start]`$$` : PID của tiến trình Bash hiện tại[cite: 108, 132, 171].
* `$!` : PID của tiến trình chạy ngầm (background) gần nhất.
* [cite_start]`$RANDOM` : Số ngẫu nhiên[cite: 108, 132].
* `$SECONDS` : Số giây script đã chạy.
* `$EUID`    : Kiểm tra quyền thực thi của BASH script, nếu trả về 0 là quyền root/sudo.

### 7. Cú pháp Mở rộng (Expansion) & Globbing
* [cite_start]**Parameter:** `${var}`, `${var:-default}`, `${var:=default}`, `${var:+value}`, `${var:?error}`[cite: 109].
* [cite_start]**Command substitution:** `$(command)`, `` `command` ``[cite: 109, 112, 127].
* [cite_start]**Arithmetic:** `$((a + b))`[cite: 109].
* [cite_start]**Brace expansion:** `{1..10}`, `file{A,B}.txt`[cite: 109].
* [cite_start]**Globbing:** `*` (đại diện tất cả), `?` (đại diện 1 ký tự), `[abc]` (khớp 1 ký tự a, b, hoặc c)[cite: 109].

### 8. Ký hiệu Trích dẫn & Khác (Quoting & Advanced)
* [cite_start]`" "` : Ngoặc kép (giữ biến, giữ space)[cite: 106, 126].
* [cite_start]`' '` : Ngoặc đơn (chuỗi nguyên bản - literal)[cite: 106, 126].
* [cite_start]`\` : Backslash (escape)[cite: 106, 126].
* [cite_start]`[[ ... ]]` : Cú pháp kiểm tra điều kiện nâng cao[cite: 112, 128].
* [cite_start]`[ ... ]` : Cú pháp kiểm tra điều kiện cơ bản[cite: 112, 128].
* [cite_start]`( ... )` : Subshell (chạy trong tiến trình con)[cite: 111].
* [cite_start]`<( ... )` : Process substitution[cite: 111].

### 9. Cờ cài đặt (Set flags) & Tín hiệu (Signals)
* `set -e` : Thoát ngay khi có lệnh lỗi.
* `set -u` : Báo lỗi nếu gọi biến chưa khai báo.
* `set -x` : Chế độ debug (in ra từng lệnh).
* `set -o` : Trong pipeline (|), fail nếu bất kỳ lệnh nào trong chuỗi lệnh bị fail. VD grep "abc" file.txt | wc -l nếu grep fail thì toàn pipeline fail.
* [cite_start]`set -euo pipefail` : Best practice kết hợp[cite: 112, 122].
* [cite_start]`trap` : Lệnh bắt tín hiệu hệ thống (ví dụ bắt tín hiệu `SIGINT`)[cite: 111].

### 10. File Descriptor & Đường dẫn đặc biệt
* [cite_start]`0` : Standard Input (stdin)[cite: 114, 133, 155].
* [cite_start]`1` : Standard Output (stdout)[cite: 114, 134, 156].
* [cite_start]`2` : Standard Error (stderr)[cite: 114, 135, 157].
* [cite_start]`/proc/` : Chứa thông tin các tiến trình[cite: 136, 167].
* [cite_start]`/proc/<PID>/fd/` : Thư mục chứa các FD của một PID cụ thể[cite: 137, 168].
* [cite_start]`/proc/$$/fd/` : Thư mục chứa FD của terminal hiện tại[cite: 138, 171].
* [cite_start]`.pid` (vd `/var/run/nginx.pid`): Tệp chứa số định danh PID của phần mềm[cite: 139, 187].