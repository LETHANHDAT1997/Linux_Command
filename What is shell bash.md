## **Tệp `.bashrc` là gì?**
`.bashrc` là một **tệp cấu hình shell** dành riêng cho **Bash** (Bourne Again SHell).
Nó là một **script được chạy mỗi khi bạn mở một shell tương tác mới** (ví dụ mở terminal).

---

### Vị trí

* Thường nằm ở trong **thư mục home** của người dùng:

  ```
  ~/.bashrc
  ```
* Có thể có thêm `/etc/bash.bashrc` (áp dụng cho toàn hệ thống).

---

### Công dụng chính của `.bashrc`

* **Định nghĩa biến môi trường**:

  ```bash
  export PATH=$PATH:$HOME/bin
  export EDITOR=nano
  ```
* **Tạo alias (bí danh lệnh)**:

  ```bash
  alias ll='ls -alF'
  alias gs='git status'
  ```
* **Cấu hình prompt (PS1)**:

  ```bash
  PS1='\u@\h:\w\$ '
  ```
* **Chạy script hoặc chương trình khởi tạo** khi vào shell.
* **Cấu hình cho các công cụ** (như `nvm`, `pyenv`, `conda`…).

---

### Khi nào nó được thực thi?

* Khi bạn mở một **terminal tương tác** (ví dụ nhấn Ctrl+Alt+T).
* Không chạy khi bạn đăng nhập qua **login shell** (khi đó dùng `~/.bash_profile` hoặc `~/.profile`), nhưng trong nhiều hệ thống `.bash_profile` lại được viết để gọi `.bashrc`.

Ví dụ nội dung trong `~/.bash_profile`:

```bash
if [ -f ~/.bashrc ]; then
    . ~/.bashrc
fi
```

---

👉 Nói ngắn gọn: `.bashrc` là **nơi bạn tùy chỉnh Bash theo ý muốn**, giúp bạn có môi trường làm việc cá nhân hóa mỗi lần mở terminal.


---

## **Login shell là gì?**

* **Login shell** là phiên Bash đầu tiên khi bạn đăng nhập vào hệ thống (qua **TTY, SSH, hay khi vừa login vào máy**).
* Khi shell được gọi là **login shell**, Bash sẽ đọc các file khởi tạo khác với non-login shell.

Ví dụ:

* Bạn bật terminal ngay sau khi đăng nhập: terminal đó thường là **non-login shell**.
* Bạn SSH vào máy từ xa: bạn nhận được **login shell**.
* Bạn chạy `bash --login`: bạn khởi tạo **login shell** thủ công.

---

### 1. **Các tệp cấu hình liên quan**

#### a) `~/.bash_profile`

* Được thực thi **một lần** khi login shell bắt đầu.
* Thường dùng để thiết lập biến môi trường, đường dẫn, chạy các script cần thiết khi login.
* Chỉ dành riêng cho Bash.

#### b) `~/.profile`

* Là file cấu hình **chuẩn POSIX** cho login shell (sh, dash, ksh, Bash đều dùng được).
* Trên hệ thống không có `~/.bash_profile`, Bash login shell sẽ fallback sang `~/.profile`.
* Dùng khi muốn cấu hình chung cho nhiều shell, không chỉ Bash.

#### c) `~/.bashrc`

* Chạy **mỗi lần mở terminal (non-login shell)**.
* Thường để alias, hàm, config prompt…

---

### 2. **Khác biệt chính giữa `bash_profile` và `profile`**

| Đặc điểm             | `~/.bash_profile`                             | `~/.profile`                                      |
| -------------------- | --------------------------------------------- | ------------------------------------------------- |
| Dành cho             | Bash login shell                              | Tất cả shell POSIX login                          |
| Ưu tiên              | Được Bash dùng **trước tiên**                 | Chỉ được dùng khi `~/.bash_profile` không tồn tại |
| Nội dung thường dùng | Biến môi trường, PATH, lệnh khởi tạo cho Bash | Thiết lập biến chung cho nhiều shell              |

---

### 3. **Mối liên hệ giữa `.bash_profile` và `.bashrc`**

Thông lệ phổ biến là trong `~/.bash_profile` sẽ gọi `~/.bashrc` để đảm bảo khi login shell khởi tạo, bạn vẫn có alias, function... từ `.bashrc`.

Ví dụ thường gặp:

```bash
# ~/.bash_profile
if [ -f ~/.bashrc ]; then
    . ~/.bashrc
fi
```

---

👉 Tóm gọn:

* **Login shell** = shell khi bạn vừa đăng nhập.
* **`.bash_profile`** = dành riêng cho Bash login shell.
* **`.profile`** = dành cho mọi loại login shell (POSIX).
* **`.bashrc`** = dành cho non-login shell (terminal mới mở).

---

## **Thư mục ./config là gì**

Trong Linux (cũng như các hệ điều hành *Unix-like*), thư mục **`~/.config`** (lưu ý: nằm trong **home directory** của từng user) là nơi lưu trữ **cấu hình của các ứng dụng** theo chuẩn [XDG Base Directory Specification](https://specifications.freedesktop.org/basedir-spec/latest/).

Giải thích chi tiết:

* **Dấu `~`**: chỉ thư mục home của người dùng (ví dụ `/home/username/`).
* **`.config`**: thư mục ẩn (bắt đầu bằng dấu `.`), thường dùng để chứa các file hoặc thư mục con cấu hình của ứng dụng.
* **Vai trò**:

  * Các chương trình không nên lưu file rải rác trong home nữa, mà tập trung ở `~/.config`.
  * Ví dụ:

    * `~/.config/code/` → chứa cài đặt của VS Code
    * `~/.config/gtk-3.0/` → cấu hình GTK theme
    * `~/.config/pulse/` → cấu hình âm thanh PulseAudio
* **So sánh với các thư mục cấu hình khác**:

  * Trước đây nhiều ứng dụng lưu file kiểu `~/.appnamerc` hoặc `~/.*` (rải rác trong home).
  * Chuẩn XDG giúp gom chúng vào một chỗ để dễ quản lý.

👉 Nếu bạn thấy thư mục **`./config`** (có dấu `./`), thì nó có nghĩa khác:

* `./config` là đường dẫn **tương đối**, chỉ một thư mục con tên là `config` nằm trong **thư mục hiện tại**.
* Còn **`~/.config`** mới là thư mục cấu hình trong home của user.

### 1. `./config`

* `.` nghĩa là **thư mục hiện tại** (current working directory).
* `./config` tức là thư mục con tên là `config` nằm ngay trong thư mục bạn đang đứng.
* Ví dụ:

  ```bash
  pwd
  /home/dat/projects/myapp

  ls ./config
  settings.json  theme.yaml
  ```

  → Thư mục `config` này chỉ liên quan đến dự án/app bạn đang làm, không liên quan đến hệ thống Linux.

### 2. `~/.config`

* `~` nghĩa là **thư mục home của user** (ví dụ `/home/dat`).
* `~/.config` là thư mục cấu hình chuẩn XDG, dùng chung cho nhiều ứng dụng.
* Ví dụ:

  ```bash
  ls ~/.config
  code/  gtk-3.0/  pulse/  ranger/  mpv/
  ```

  → Mỗi thư mục con bên trong chứa cấu hình riêng của từng ứng dụng cài trên máy.

✅ Tóm lại:

* `./config` → thư mục `config` nằm ở **chỗ bạn đang đứng**.
* `~/.config` → thư mục cấu hình **toàn cục cho user**, được Linux và app dùng theo chuẩn.

---

