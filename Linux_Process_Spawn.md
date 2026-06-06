# Spawn Process trong Linux bằng C

> Tài liệu hướng dẫn các phương pháp tạo (spawn) tiến trình con trong Linux sử dụng ngôn ngữ C.

---

## Mục lục

1. [Tổng quan về Process](#1-tổng-quan-về-process)
2. [fork() — Tạo tiến trình con](#2-fork--tạo-tiến-trình-con)
3. [exec() — Thay thế tiến trình](#3-exec--thay-thế-tiến-trình)
4. [fork() + exec() — Mô hình chuẩn](#4-fork--exec--mô-hình-chuẩn)
5. [wait() / waitpid() — Thu hồi tiến trình con](#5-wait--waitpid--thu-hồi-tiến-trình-con)
6. [system() — Chạy lệnh shell](#6-system--chạy-lệnh-shell)
7. [Signal trong Linux — Bắn từ đâu ra?](#7-signal-trong-linux--bắn-từ-đâu-ra)
8. [posix_spawn() — Spawn hiện đại](#8-posix_spawn--spawn-hiện-đại)
9. [Zombie & Orphan Process](#9-zombie--orphan-process)
10. [So sánh các phương pháp](#10-so-sánh-các-phương-pháp)
11. [Ví dụ tổng hợp](#11-ví-dụ-tổng-hợp)

---

## 1. Tổng quan về Process

Trong Linux, mỗi chương trình đang chạy là một **process** (tiến trình). Mỗi process có:

| Thuộc tính | Mô tả |
|---|---|
| **PID** | Process ID — định danh duy nhất |
| **PPID** | Parent Process ID — PID của tiến trình cha |
| **Bộ nhớ riêng** | Mỗi process có không gian địa chỉ riêng (virtual memory) |
| **File Descriptor** | Bảng mô tả tệp (stdin=0, stdout=1, stderr=2) |

```
┌──────────────────────────────────┐
│           init (PID=1)           │
│    ┌───────────┬────────────┐    │
│    ▼           ▼            ▼    │
│  bash       systemd      sshd   │
│   │                        │    │
│   ▼                        ▼    │
│ my_app                   login  │
└──────────────────────────────────┘
```

> **Quy tắc**: Mọi process đều được tạo ra từ một process cha, ngoại trừ `init` (PID=1).

### File Descriptor (fd) là gì trong một tiến trình?

Trong mỗi process, **file descriptor (fd)** là một **số nguyên không âm** (0, 1, 2, 3, ...) đóng vai trò như **"tay nắm" (handle)** để process tương tác với tài nguyên I/O (file, pipe, socket, terminal...).

Kernel duy trì cho **mỗi process** một **bảng fd riêng** (file descriptor table). Mỗi ô trong bảng trỏ đến một **cấu trúc file mở** (open file description) trong kernel.

```
  Process (PID=100)
  ┌─────────────────────────────────────────────────┐
  │  File Descriptor Table (bảng fd riêng)          │
  │  ┌────┬──────────────────────────────────────┐  │
  │  │ fd │ Trỏ đến (open file description)      │  │
  │  ├────┼──────────────────────────────────────┤  │
  │  │  0 │ → terminal /dev/pts/0 (stdin)        │  │
  │  │  1 │ → terminal /dev/pts/0 (stdout)       │  │
  │  │  2 │ → terminal /dev/pts/0 (stderr)       │  │
  │  │  3 │ → file /tmp/log.txt                  │  │
  │  │  4 │ → socket 192.168.1.1:8080            │  │
  │  │  5 │ → pipe (write end)                   │  │
  │  └────┴──────────────────────────────────────┘  │
  └─────────────────────────────────────────────────┘
```

- `fd = 0, 1, 2` luôn được tạo sẵn khi process khởi chạy (stdin, stdout, stderr).
- Mỗi lần gọi `open()`, `socket()`, `pipe()`... kernel **tự cấp fd mới**, lấy số nhỏ nhất chưa dùng.
- fd chỉ là **một con số** — process dùng số này để yêu cầu kernel đọc/ghi, kernel tra bảng fd để biết tài nguyên thực sự.

```c
int fd = open("/tmp/data.txt", O_RDONLY);  /* kernel cấp fd = 3 (số nhỏ nhất trống) */
read(fd, buf, 100);    /* kernel tra bảng: fd 3 → file /tmp/data.txt → đọc 100 byte */
close(fd);             /* kernel xóa ô fd 3 khỏi bảng, giải phóng tài nguyên */
```

### Trong cùng một PID, các fd có thể trỏ đến cùng một tài nguyên không?

**CÓ** — nhiều fd (khác số) có thể cùng trỏ đến **một tài nguyên** (cùng file, cùng pipe...).

Lưu ý: Bản thân **số fd** trong cùng một process luôn **duy nhất** (không có 2 fd trùng số), nhưng chúng hoàn toàn có thể trỏ đến cùng một thứ:

```c
int fd1 = open("/tmp/log.txt", O_WRONLY);  /* fd1 = 3 */
int fd2 = open("/tmp/log.txt", O_WRONLY);  /* fd2 = 4 — khác số, nhưng cùng file */
int fd3 = dup(fd1);                        /* fd3 = 5 — bản sao của fd1 */
```

```
  Bảng fd (cùng 1 PID):
  ┌────┬────────────────────────────────────────────────┐
  │ fd │ Trỏ đến                                       │
  ├────┼────────────────────────────────────────────────┤
  │  3 │ → open file description A (log.txt, vị trí 0) │
  │  4 │ → open file description B (log.txt, vị trí 0) │  ← open() tạo entry MỚI
  │  5 │ → open file description A (log.txt, vị trí 0) │  ← dup() chia sẻ entry A
  └────┴────────────────────────────────────────────────┘
```

**Phân biệt `open()` 2 lần vs `dup()`:**

| Cách tạo | Kết quả | Chia sẻ vị trí đọc/ghi? |
|---|---|---|
| `open()` 2 lần cùng file | 2 fd, 2 open file description **riêng biệt** | **KHÔNG** — mỗi fd có con trỏ vị trí (offset) riêng |
| `dup(fd)` / `dup2(fd, newfd)` | 2 fd, **chia sẻ** 1 open file description | **CÓ** — cả 2 fd di chuyển offset cùng nhau |

### Nếu các tiến trình khác nhau thì fd có giống nhau không?

**CÓ — giá trị số fd có thể trùng nhau**, nhưng chúng **hoàn toàn độc lập** vì mỗi process có bảng fd riêng.

```
  Process A (PID=100)              Process B (PID=200)
  ┌────┬───────────────┐           ┌────┬───────────────┐
  │ fd │ Trỏ đến       │           │ fd │ Trỏ đến       │
  ├────┼───────────────┤           ├────┼───────────────┤
  │  0 │ → terminal    │           │  0 │ → terminal    │
  │  1 │ → terminal    │           │  1 │ → terminal    │
  │  2 │ → terminal    │           │  2 │ → terminal    │
  │  3 │ → /tmp/a.txt  │           │  3 │ → /tmp/b.txt  │  ← KHÁC FILE
  │  4 │ → socket:8080 │           │  4 │ → pipe        │  ← KHÁC LOẠI
  └────┴───────────────┘           └────┴───────────────┘
        │                                │
        └── fd=3 ở A ≠ fd=3 ở B ────────┘
```

- `fd = 3` của process A và `fd = 3` của process B là **hai thứ hoàn toàn khác nhau**.
- Giống như phòng 101 ở khách sạn A và phòng 101 ở khách sạn B — cùng số nhưng khác phòng.
- Hầu hết mọi process đều có `fd = 0, 1, 2` (stdin, stdout, stderr) — nhưng chúng có thể trỏ đến terminal, file, hoặc pipe khác nhau tùy cách chạy.

**Trường hợp đặc biệt — `fork()`:** Khi `fork()`, child **kế thừa bản sao** bảng fd của parent. Lúc này, cùng một số fd ở cả parent và child **trỏ đến cùng open file description** trong kernel:

```c
int fd = open("/tmp/shared.txt", O_WRONLY);  /* fd = 3 ở parent */
pid_t pid = fork();
/* Bây giờ:
 * Parent: fd=3 → open file description X
 * Child:  fd=3 → open file description X   ← CÙNG entry!
 */
```

### Nếu fd ở nhiều tiến trình cùng trỏ về một thứ — có gây lỗi không?

**Có thể gây lỗi** nếu không xử lý đúng. Đây là tình huống thường gặp sau `fork()` hoặc khi nhiều process cùng mở một file.

#### Tình huống 1: Sau `fork()` — chia sẻ open file description (NGUY HIỂM)

Sau `fork()`, parent và child chia sẻ **cùng con trỏ vị trí (offset)**. Nếu cả hai cùng ghi, dữ liệu bị **xen kẽ (interleave)**:

```c
int fd = open("log.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
pid_t pid = fork();

if (pid == 0) {
    write(fd, "CHILD\n", 6);   /* Ghi tại offset 0, offset tăng lên 6 */
} else {
    write(fd, "PARENT\n", 7);  /* Ghi tại offset 6?? hay 0?? → KHÔNG XÁC ĐỊNH */
}
```

```
  Kernel:
  ┌───────────────────────────────────────────┐
  │  Open File Description (log.txt)          │
  │  ┌─────────────┐                          │
  │  │ offset = ??? │ ← Parent VÀ Child cùng  │
  │  └─────────────┘   di chuyển con trỏ này  │
  │                                           │
  │  Parent ghi "PARENT\n" ──┐                │
  │  Child  ghi "CHILD\n"  ──┤ RACE CONDITION │
  │                          ↓                │
  │  Kết quả file: "PCHIARLEDN\nT\n"  ← LỖI! │
  └───────────────────────────────────────────┘
```

**Hậu quả**: **Race condition** — thứ tự ghi không xác định, dữ liệu bị hỏng.

**Cách khắc phục:**

```c
/* Cách 1: Dùng O_APPEND — mỗi write() luôn ghi ở cuối file (atomic) */
int fd = open("log.txt", O_WRONLY | O_CREAT | O_APPEND, 0644);

/* Cách 2: Mỗi process mở file riêng (open file description riêng) */
if (pid == 0) {
    int child_fd = open("child_log.txt", O_WRONLY | O_CREAT, 0644);
    write(child_fd, "CHILD\n", 6);
}

/* Cách 3: Dùng file lock */
struct flock fl = { .l_type = F_WRLCK, .l_whence = SEEK_SET };
fcntl(fd, F_SETLKW, &fl);   /* Khóa trước khi ghi */
write(fd, "SAFE\n", 5);
fl.l_type = F_UNLCK;
fcntl(fd, F_SETLK, &fl);    /* Mở khóa sau khi ghi */
```

#### Tình huống 2: Nhiều process cùng `open()` một file (ÍT NGUY HIỂM HƠN)

Nếu mỗi process tự gọi `open()` riêng (không qua `fork()`), mỗi bên có **offset riêng biệt**:

```c
/* Process A */
int fd = open("data.txt", O_WRONLY);  /* offset A = 0 */
write(fd, "AAAA", 4);                /* offset A = 4 */

/* Process B (chạy riêng, không phải fork) */
int fd = open("data.txt", O_WRONLY);  /* offset B = 0 */
write(fd, "BB", 2);                   /* offset B = 2 */
```

**Hậu quả**: Process B **ghi đè** lên dữ liệu của A vì cả hai bắt đầu từ offset 0.

**Khắc phục**: Dùng `O_APPEND` hoặc file lock (giống cách 1 và 3 ở trên).

#### Tình huống 3: Pipe/Socket — thiết kế để chia sẻ (AN TOÀN)

Pipe và socket được **thiết kế** để nhiều process cùng truy cập. Kernel đảm bảo:

- `write()` nhỏ hơn `PIPE_BUF` (thường 4096 byte) là **atomic** — không bị xen kẽ.
- Mỗi byte chỉ được `read()` **một lần** bởi **một** reader.

```c
int pipefd[2];
pipe(pipefd);
pid_t pid = fork();

if (pid == 0) {
    close(pipefd[0]);              /* Child đóng đầu đọc */
    write(pipefd[1], "hello", 5);  /* Ghi vào pipe — AN TOÀN */
    close(pipefd[1]);
} else {
    close(pipefd[1]);              /* Parent đóng đầu ghi */
    char buf[100];
    read(pipefd[0], buf, 100);    /* Đọc từ pipe — AN TOÀN */
    close(pipefd[0]);
}
```

#### Tóm tắt: Khi nào chia sẻ fd gây lỗi?

| Tình huống | Offset chung? | Nguy hiểm? | Cách xử lý |
|---|---|---|---|
| Sau `fork()`, cùng ghi file thường | **CÓ** | ⚠️ Race condition | `O_APPEND` hoặc file lock |
| Nhiều process tự `open()` cùng file | **KHÔNG** | ⚠️ Ghi đè lẫn nhau | `O_APPEND` hoặc file lock |
| Pipe / Socket | N/A | ✅ An toàn (atomic nếu ≤ PIPE_BUF) | Không cần xử lý thêm |
| Sau `fork()`, child gọi `exec()` | **CÓ** (nếu không close) | ⚠️ Rò rỉ fd | Dùng `O_CLOEXEC` hoặc `close()` trước `exec()` |

---

## 2. fork() — Tạo tiến trình con

### Header & Prototype

```c
#include <unistd.h>

pid_t fork(void);
```

### Giá trị trả về

| Trả về | Ngữ cảnh |
|---|---|
| `> 0` | Trong **parent** — giá trị là PID của child |
| `== 0` | Trong **child** |
| `< 0` | Lỗi — không tạo được process |

### Cơ chế hoạt động

```
  Parent (PID=100)
       │
    fork()
       │
  ┌────┴────┐
  │         │
Parent    Child (PID=101)
(trả về   (trả về 0)
  101)
```

- `fork()` tạo **bản sao** (copy) của process hiện tại.
- Child nhận bản sao toàn bộ bộ nhớ, file descriptor, biến môi trường.
- Hai process chạy **độc lập** sau `fork()`.
- Linux sử dụng **Copy-on-Write (COW)**: bộ nhớ chỉ thực sự copy khi một bên ghi.

### Ví dụ cơ bản

```c
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

int main(void)
{
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        /* ---- CHILD PROCESS ---- */
        printf("[Child]  PID=%d, PPID=%d\n", getpid(), getppid());
    } else {
        /* ---- PARENT PROCESS ---- */
        printf("[Parent] PID=%d, Child PID=%d\n", getpid(), pid);
    }

    return 0;
}
```

**Biên dịch & chạy:**

```bash
gcc -o fork_demo fork_demo.c
./fork_demo
```

**Output mẫu:**

```
[Parent] PID=1234, Child PID=1235
[Child]  PID=1235, PPID=1234
```

---

## 3. exec() — Thay thế tiến trình

Họ hàm `exec()` **thay thế** toàn bộ mã lệnh của process hiện tại bằng chương trình mới. Process giữ nguyên PID.

### Các biến thể

```c
#include <unistd.h>

/* Dùng danh sách tham số (list) */
int execl (const char *path, const char *arg0, ..., NULL);
int execlp(const char *file, const char *arg0, ..., NULL);
int execle(const char *path, const char *arg0, ..., NULL, char *const envp[]);

/* Dùng mảng tham số (vector) */
int execv (const char *path, char *const argv[]);
int execvp(const char *file, char *const argv[]);
int execve(const char *path, char *const argv[], char *const envp[]);
```

### Quy tắc đặt tên

| Hậu tố | Ý nghĩa |
|---|---|
| `l` | Tham số truyền dạng **list** (variadic) |
| `v` | Tham số truyền dạng **vector** (mảng `argv[]`) |
| `p` | Tìm chương trình trong **PATH** (không cần đường dẫn đầy đủ) |
| `e` | Truyền **environment** tùy chỉnh |

### Giải thích chi tiết từng đối số

#### `path` vs `file` — Đối số đầu tiên

- **`const char *path`** (dùng trong `execl`, `execv`, `execle`, `execve`): Đường dẫn **tuyệt đối hoặc tương đối** đến file thực thi. Kernel sẽ tìm **chính xác** tại đường dẫn này.

  ```c
  execl("/usr/bin/ls", "ls", NULL);   /* path = đường dẫn đầy đủ */
  execl("./my_app", "my_app", NULL);   /* path = đường dẫn tương đối */
  ```

- **`const char *file`** (dùng trong `execlp`, `execvp`): Chỉ cần tên chương trình. Kernel sẽ **tự tìm** trong các thư mục liệt kê bởi biến môi trường `$PATH` (ví dụ `/usr/bin:/usr/local/bin:...`).

  ```c
  execlp("ls", "ls", NULL);   /* file = chỉ cần tên, kernel tìm trong PATH */
  ```

#### `arg0, arg1, ..., NULL` — Danh sách tham số (biến thể `l`)

- **`arg0`**: Theo quy ước, đây là **tên chương trình** (giống `argv[0]` trong `main()`). Thường trùng với `path`/`file`.
- **`arg1, arg2, ...`**: Các tham số dòng lệnh truyền cho chương trình.
- **`NULL`**: **Bắt buộc** — đánh dấu kết thúc danh sách tham số.

```c
/* Tương đương chạy: ls -l -a /tmp */
execlp("ls",        /* file: tên chương trình, tìm trong PATH    */
       "ls",        /* arg0: tên chương trình (quy ước)          */
       "-l",        /* arg1: tham số thứ nhất                    */
       "-a",        /* arg2: tham số thứ hai                     */
       "/tmp",      /* arg3: tham số thứ ba                      */
       NULL);       /* Kết thúc danh sách — BẮT BUỘC             */
```

#### `argv[]` — Mảng tham số (biến thể `v`)

Thay vì liệt kê từng tham số, truyền một **mảng con trỏ chuỗi**, phần tử cuối là `NULL`.

```c
char *argv[] = {
    "ls",       /* argv[0]: tên chương trình (quy ước)   */
    "-l",       /* argv[1]: tham số thứ nhất             */
    "-a",       /* argv[2]: tham số thứ hai              */
    "/tmp",     /* argv[3]: tham số thứ ba               */
    NULL        /* Kết thúc mảng — BẮT BUỘC              */
};
execvp("ls", argv);
```

> **Khi nào dùng `l` vs `v`?**
> - Dùng `l` khi **biết trước** số lượng tham số lúc viết code.
> - Dùng `v` khi số lượng tham số **được xây dựng động** lúc runtime.

#### `envp[]` — Biến môi trường tùy chỉnh (biến thể `e`)

Mặc định, child kế thừa toàn bộ biến môi trường từ parent. Biến thể `e` cho phép truyền **bộ biến môi trường riêng**:

```c
char *my_env[] = {
    "PATH=/usr/bin",          /* Chỉ định PATH tùy chỉnh   */
    "MY_VAR=hello",           /* Biến tự định nghĩa        */
    "LANG=en_US.UTF-8",       /* Locale                    */
    NULL                      /* Kết thúc mảng — BẮT BUỘC  */
};

/* Child sẽ chỉ thấy 3 biến môi trường trên, KHÔNG kế thừa từ parent */
execle("/usr/bin/env", "env", NULL, my_env);
```

### Ví dụ

```c
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    printf("Trước exec — PID=%d\n", getpid());

    /* Thay thế process hiện tại bằng lệnh "ls -la" */
    execlp("ls", "ls", "-la", NULL);

    /* Dòng này CHỈ chạy nếu exec() thất bại */
    perror("exec failed");
    return 1;
}
```

> **Lưu ý quan trọng**: Sau khi `exec()` thành công, **không có dòng code nào phía sau được thực thi** vì toàn bộ chương trình đã bị thay thế.

---

## 4. fork() + exec() — Mô hình chuẩn

Đây là mô hình **chuẩn** và phổ biến nhất để spawn process trong Linux.

```
  Parent
    │
  fork() ──────────────► Child
    │                      │
    │                   exec("program")
    │                      │
  wait()               chạy "program"
    │                      │
    │ ◄──── exit status ───┘
    │
  tiếp tục
```

### Ví dụ hoàn chỉnh

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        /*
         * CHILD: thay thế bằng chương trình khác.
         * Ví dụ: chạy lệnh "ls -l /tmp"
         */
        char *args[] = {"ls", "-l", "/tmp", NULL};
        execvp("ls", args);

        /* Chỉ đến đây nếu exec thất bại */
        perror("execvp");
        _exit(127);  /* Dùng _exit() trong child sau fork */
    }

    /* PARENT: đợi child kết thúc */
    int status;
    waitpid(pid, &status, 0);

    if (WIFEXITED(status)) {
        printf("Child thoát với mã: %d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("Child bị kill bởi signal: %d\n", WTERMSIG(status));
    }

    return 0;
}
```

### Tại sao dùng `_exit()` thay vì `exit()` trong child?

| `exit()` | `_exit()` |
|---|---|
| Flush buffer của stdio | **Không** flush buffer |
| Gọi các `atexit()` handler | **Không** gọi handler |

Nếu dùng `exit()` trong child sau `fork()`, buffer có thể bị flush **hai lần** (cả parent và child), gây ra output trùng lặp.

---

## 5. wait() / waitpid() — Thu hồi tiến trình con

### Prototype

```c
#include <sys/wait.h>

pid_t wait(int *status);                          /* Đợi bất kỳ child nào */
pid_t waitpid(pid_t pid, int *status, int options); /* Đợi child cụ thể   */
```

### Giải thích chi tiết từng đối số

#### `wait(int *status)`

| Đối số | Kiểu | Ý nghĩa |
|---|---|---|
| `status` | `int *` | Con trỏ đến biến `int` để kernel ghi **trạng thái thoát** của child vào. Truyền `NULL` nếu không cần biết trạng thái. |

- **Giá trị trả về**: PID của child đã thoát, hoặc `-1` nếu lỗi.
- `wait()` sẽ **chặn (block)** cho đến khi **bất kỳ** child nào thoát.

#### `waitpid(pid_t pid, int *status, int options)`

| Đối số | Kiểu | Ý nghĩa |
|---|---|---|
| `pid` | `pid_t` | Xác định **child nào** cần đợi (xem bảng dưới) |
| `status` | `int *` | Con trỏ nhận trạng thái thoát (giống `wait`). Truyền `NULL` nếu không cần. |
| `options` | `int` | Cờ điều khiển hành vi (xem bảng dưới) |

**Tham số `pid`:**

| Giá trị | Ý nghĩa |
|---|---|
| `> 0` | Đợi child có **PID cụ thể** đó |
| `-1` | Đợi **bất kỳ** child nào (giống `wait()`) |
| `0` | Đợi bất kỳ child nào cùng **process group** với parent |
| `< -1` | Đợi bất kỳ child nào thuộc process group có ID = `|pid|` |

**Tham số `options`** (có thể kết hợp bằng OR `|`):

| Cờ | Ý nghĩa |
|---|---|
| `0` | Chặn (blocking) cho đến khi child thoát |
| `WNOHANG` | **Non-blocking** — trả về `0` ngay nếu chưa có child thoát, thay vì chờ |
| `WUNTRACED` | Cũng trả về khi child bị **stopped** (dừng bởi signal), không chỉ khi thoát |
| `WCONTINUED` | Cũng trả về khi child bị stopped trước đó được **tiếp tục** (bởi `SIGCONT`) |

### Biến `status` chứa gì? — Cấu trúc bên trong

Khi kernel ghi vào `status`, nó **đóng gói nhiều thông tin** vào một số `int` duy nhất (32 bit). Bạn **KHÔNG nên đọc trực tiếp** giá trị này mà phải dùng các macro để giải mã.

```
  Cấu trúc bit của status (đơn giản hoá):
  ┌────────────────────┬───────────────────┐
  │  byte cao (bit 15-8)│  byte thấp (bit 7-0)│
  ├────────────────────┼───────────────────┤
  │  exit code (0-255) │  0x00             │  ← child gọi exit()
  │  0x00              │  signal number    │  ← child bị kill bởi signal
  │  signal number     │  0x7F             │  ← child bị stopped
  └────────────────────┴───────────────────┘
```

### Macro kiểm tra status — Giải thích chi tiết

#### 1. `WIFEXITED(status)` — Child có thoát bình thường không?

- **Trả về**: `true` (khác 0) nếu child thoát bằng cách gọi `exit()`, `_exit()`, hoặc `return` từ `main()`.
- **Nghĩa**: Child **tự nguyện thoát**, không bị signal nào giết.

#### 2. `WEXITSTATUS(status)` — Lấy mã thoát (exit code)

- **Chỉ dùng khi** `WIFEXITED(status)` trả về `true`.
- **Trả về**: Giá trị mà child truyền vào `exit(code)` hoặc `return code` từ `main()`, nằm trong khoảng **0–255**.
- **Quy ước**: `0` = thành công, khác `0` = lỗi.

```c
if (WIFEXITED(status)) {
    int code = WEXITSTATUS(status);
    /* code = giá trị child truyền vào exit(), ví dụ:
     *   child gọi exit(0)   → code = 0  (thành công)
     *   child gọi exit(1)   → code = 1  (lỗi chung)
     *   child gọi exit(127) → code = 127 (command not found - quy ước) */
}
```

#### 3. `WIFSIGNALED(status)` — Child có bị giết bởi signal không?

- **Trả về**: `true` nếu child bị **kết thúc bất thường** do nhận một signal mà nó không xử lý (handle) được hoặc không bắt (catch).
- **Nghĩa**: Child **không tự thoát**, mà bị kernel hoặc process khác ép buộc dừng.

#### 4. `WTERMSIG(status)` — Lấy số hiệu signal đã giết child

- **Chỉ dùng khi** `WIFSIGNALED(status)` trả về `true`.
- **Trả về**: Số hiệu signal (ví dụ `9` cho `SIGKILL`, `11` cho `SIGSEGV`).

```c
if (WIFSIGNALED(status)) {
    int sig = WTERMSIG(status);
    /* sig = signal đã giết child, ví dụ:
     *   sig = 9  (SIGKILL)  → bị kill -9
     *   sig = 11 (SIGSEGV)  → segmentation fault
     *   sig = 6  (SIGABRT)  → child gọi abort()  */
}
```

#### 5. `WIFSTOPPED(status)` — Child có bị dừng tạm (stopped) không?

- **Chỉ có ý nghĩa khi** gọi `waitpid()` với cờ `WUNTRACED`.
- **Trả về**: `true` nếu child bị **tạm dừng** (chưa thoát, chỉ dừng lại, có thể tiếp tục sau).

#### 6. `WSTOPSIG(status)` — Lấy signal đã dừng child

- **Chỉ dùng khi** `WIFSTOPPED(status)` trả về `true`.
- **Trả về**: Số hiệu signal gây dừng (thường là `SIGSTOP=19` hoặc `SIGTSTP=20`).

```c
if (WIFSTOPPED(status)) {
    int sig = WSTOPSIG(status);
    /* sig = 19 (SIGSTOP) → bị dừng bởi kill -STOP
     * sig = 20 (SIGTSTP) → bị dừng bởi Ctrl+Z */
}
```

### Luồng kiểm tra status chuẩn

```c
int status;
waitpid(pid, &status, 0);

if (WIFEXITED(status)) {
    printf("Child thoát bình thường, exit code = %d\n", WEXITSTATUS(status));
} else if (WIFSIGNALED(status)) {
    printf("Child bị giết bởi signal %d (%s)\n",
           WTERMSIG(status), strsignal(WTERMSIG(status)));
} else if (WIFSTOPPED(status)) {
    printf("Child bị dừng bởi signal %d\n", WSTOPSIG(status));
}
```

### Ví dụ non-blocking (polling)

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    pid_t pid = fork();

    if (pid == 0) {
        sleep(3);  /* Child chạy 3 giây */
        _exit(42);
    }

    /* Parent: polling trạng thái child */
    int status;
    pid_t result;

    do {
        result = waitpid(pid, &status, WNOHANG);
        if (result == 0) {
            printf("Child vẫn đang chạy...\n");
            sleep(1);
        }
    } while (result == 0);

    printf("Child thoát, exit code = %d\n", WEXITSTATUS(status));
    return 0;
}
```

---

## 6. system() — Chạy lệnh shell

### Prototype

```c
#include <stdlib.h>

int system(const char *command);
```

### Cơ chế

`system()` nội bộ thực hiện: `fork()` → `exec("/bin/sh", "-c", command)` → `waitpid()`

### Ví dụ

```c
#include <stdlib.h>
#include <stdio.h>

int main(void)
{
    int ret = system("ls -la /tmp");

    if (ret == -1) {
        perror("system");
    } else {
        printf("Lệnh trả về: %d\n", WEXITSTATUS(ret));
    }

    return 0;
}
```

### ⚠️ Cảnh báo bảo mật

- **KHÔNG BAO GIỜ** dùng `system()` với input từ người dùng → nguy cơ **shell injection**.
- Chỉ dùng cho script/tool đơn giản, **không dùng trong production** hay daemon.

```c
/* ❌ NGUY HIỂM — shell injection */
char cmd[256];
snprintf(cmd, sizeof(cmd), "ls %s", user_input);
system(cmd);  /* Nếu user_input = "; rm -rf /" → thảm họa! */

/* ✅ AN TOÀN — dùng fork() + execvp() */
pid_t pid = fork();
if (pid == 0) {
    execlp("ls", "ls", user_input, NULL);
    _exit(127);
}
waitpid(pid, NULL, 0);
```

---

## 7. Signal trong Linux — Bắn từ đâu ra?

Signal là cơ chế **thông báo bất đồng bộ** (asynchronous notification) giữa kernel và process, hoặc giữa các process với nhau.

### Signal là gì?

- Signal là một **"tín hiệu" phần mềm** được gửi đến process.
- Khi process nhận signal, nó phải **phản ứng ngay lập tức**: hoặc chạy handler tùy chỉnh, hoặc thực hiện hành vi mặc định (thường là thoát).
- Signal được đánh số từ 1 đến 64, mỗi số có tên và ý nghĩa riêng.

### Ai gửi signal? — Nguồn gốc

| Nguồn | Cách gửi | Ví dụ |
|---|---|---|
| **Người dùng (terminal)** | Nhấn phím tắt | `Ctrl+C` → `SIGINT`, `Ctrl+Z` → `SIGTSTP`, `Ctrl+\` → `SIGQUIT` |
| **Process khác** | Gọi hàm `kill()` | `kill(pid, SIGTERM)` — gửi signal đến process có PID cụ thể |
| **Shell** | Lệnh `kill` | `kill -9 1234` → gửi `SIGKILL` đến PID 1234 |
| **Kernel** | Tự động khi có sự kiện | Segfault → `SIGSEGV`, chia cho 0 → `SIGFPE`, pipe vỡ → `SIGPIPE` |
| **Chính process đó** | Gọi `abort()` hoặc `raise()` | `abort()` → gửi `SIGABRT` cho chính mình |
| **Timer / Alarm** | Gọi `alarm()` hoặc `timer_create()` | `alarm(5)` → sau 5 giây kernel gửi `SIGALRM` |
| **Child thoát** | Kernel tự động gửi cho parent | Child `exit()` → kernel gửi `SIGCHLD` cho parent |

### Bảng signal thường gặp

| Số | Tên | Hành vi mặc định | Nguyên nhân phổ biến |
|---|---|---|---|
| 1 | `SIGHUP` | Thoát | Terminal bị đóng, mất kết nối |
| 2 | `SIGINT` | Thoát | Người dùng nhấn **Ctrl+C** |
| 3 | `SIGQUIT` | Thoát + core dump | Người dùng nhấn **Ctrl+\** |
| 6 | `SIGABRT` | Thoát + core dump | Process gọi `abort()` |
| 9 | `SIGKILL` | Thoát (không thể bắt!) | `kill -9`, kernel ép buộc dừng |
| 11 | `SIGSEGV` | Thoát + core dump | **Segmentation fault** — truy cập bộ nhớ không hợp lệ |
| 13 | `SIGPIPE` | Thoát | Ghi vào pipe/socket đã đóng |
| 14 | `SIGALRM` | Thoát | Timer hết hạn (`alarm()`) |
| 15 | `SIGTERM` | Thoát | Yêu cầu thoát lịch sự (mặc định của `kill`) |
| 17 | `SIGCHLD` | Bỏ qua | **Child thoát** hoặc bị stopped |
| 19 | `SIGSTOP` | Dừng tạm (không thể bắt!) | `kill -STOP`, debugger |
| 20 | `SIGTSTP` | Dừng tạm | Người dùng nhấn **Ctrl+Z** |
| 18 | `SIGCONT` | Tiếp tục | `kill -CONT`, `fg` trong shell |

### Gửi signal từ code C

```c
#include <signal.h>

/* Gửi signal đến process khác */
kill(pid, SIGTERM);     /* Gửi SIGTERM (yêu cầu thoát lịch sự) đến PID */
kill(pid, SIGKILL);     /* Gửi SIGKILL (ép buộc thoát) đến PID */
kill(pid, SIGSTOP);     /* Dừng tạm process */
kill(pid, SIGCONT);     /* Tiếp tục process đã bị dừng */

/* Gửi signal cho chính mình */
raise(SIGTERM);         /* Tự gửi SIGTERM */
abort();                /* Tự gửi SIGABRT → core dump */
```

### Bắt (catch) signal

```c
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

void handler(int sig)
{
    /* Hàm này chạy khi nhận signal */
    printf("Nhận signal %d (%s)\n", sig, strsignal(sig));
}

int main(void)
{
    struct sigaction sa;
    sa.sa_handler = handler;       /* Hàm xử lý khi nhận signal       */
    sigemptyset(&sa.sa_mask);      /* Không chặn signal nào thêm       */
    sa.sa_flags = SA_RESTART;      /* Tự động restart syscall bị gián đoạn */

    sigaction(SIGINT, &sa, NULL);  /* Đăng ký handler cho SIGINT (Ctrl+C) */

    printf("Nhấn Ctrl+C để test (Ctrl+\\ để thoát)\n");
    while (1) {
        sleep(1);
    }
    return 0;
}
```

> **Lưu ý**: `SIGKILL` (9) và `SIGSTOP` (19) **KHÔNG THỂ bắt hay bỏ qua** — kernel luôn thực thi hành vi mặc định.

### Mối liên hệ Signal ↔ Process Spawn

```
  Parent                     Child
    │                          │
  fork()──────────────────► exec(program)
    │                          │
    │                     chạy program...
    │                          │
    │                     ┌────┴────────────────────┐
    │                     │ Có thể xảy ra:          │
    │                     │ • exit(0)  → WIFEXITED  │
    │                     │ • SIGSEGV → WIFSIGNALED │
    │                     │ • Ctrl+C  → WIFSIGNALED │
    │                     │ • Ctrl+Z  → WIFSTOPPED  │
    │                     └────┬────────────────────┘
    │ ◄── kernel gửi SIGCHLD ──┘
    │
  waitpid() ← kiểm tra status bằng macro
```

---

## 8. posix_spawn() — Spawn hiện đại

`posix_spawn()` kết hợp `fork() + exec()` trong một API duy nhất, tối ưu hơn trên các hệ thống nhúng (embedded) vì tránh copy bộ nhớ không cần thiết.

### Prototype

```c
#include <spawn.h>

int posix_spawn(pid_t *pid,
                const char *path,
                const posix_spawn_file_actions_t *file_actions,
                const posix_spawnattr_t *attrp,
                char *const argv[],
                char *const envp[]);

int posix_spawnp(pid_t *pid,
                 const char *file,          /* Tìm trong PATH */
                 const posix_spawn_file_actions_t *file_actions,
                 const posix_spawnattr_t *attrp,
                 char *const argv[],
                 char *const envp[]);
```

### Giải thích chi tiết từng đối số

| Đối số | Kiểu | Ý nghĩa |
|---|---|---|
| `pid` | `pid_t *` | Con trỏ **output** — sau khi spawn thành công, kernel ghi PID của child vào đây |
| `path` / `file` | `const char *` | Đường dẫn đến chương trình cần chạy. `posix_spawn` cần path đầy đủ, `posix_spawnp` tìm trong `$PATH` |
| `file_actions` | `posix_spawn_file_actions_t *` | Cấu hình **redirect I/O** cho child (mở/đóng/dup file descriptor). Truyền `NULL` nếu không cần |
| `attrp` | `posix_spawnattr_t *` | Thuộc tính process (signal mask, scheduling, process group). Truyền `NULL` cho mặc định |
| `argv` | `char *const []` | Mảng tham số dòng lệnh, giống `argv` trong `execvp()`. Phần tử cuối **phải là `NULL`** |
| `envp` | `char *const []` | Mảng biến môi trường. Thường truyền `environ` (biến toàn cục) để kế thừa từ parent |

**Giá trị trả về**: Khác với `fork()`/`exec()`, `posix_spawn()` trả về `0` nếu thành công, hoặc **mã lỗi** (errno) nếu thất bại — **không** trả về `-1`.

### Ví dụ

```c
#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <sys/wait.h>

extern char **environ;

int main(void)
{
    pid_t pid;
    char *argv[] = {"ls", "-l", "/tmp", NULL};

    int ret = posix_spawnp(&pid, "ls", NULL, NULL, argv, environ);

    if (ret != 0) {
        fprintf(stderr, "posix_spawnp failed: %d\n", ret);
        return 1;
    }

    printf("Spawned child PID=%d\n", pid);

    int status;
    waitpid(pid, &status, 0);
    printf("Child exit code: %d\n", WEXITSTATUS(status));

    return 0;
}
```

### `posix_spawn_file_actions_t` — Điều khiển File Descriptor của child

#### Nó là gì?

`posix_spawn_file_actions_t` là một **struct chứa danh sách hành động** mà kernel sẽ thực hiện trên các file descriptor (fd) của child process **sau khi fork nhưng TRƯỚC khi exec**. Nó cho phép bạn:

- **Mở file mới** và gán vào fd cụ thể (redirect I/O)
- **Đóng** fd không cần thiết
- **Nhân bản** (duplicate) fd từ cái này sang cái khác

```
  Luồng thực thi bên trong posix_spawn():

  Parent ──► [fork] ──► Child (bản sao)
                           │
                     ① Thực thi file_actions:
                        • addopen  → mở file, gán vào fd
                        • addclose → đóng fd
                        • adddup2  → dup2(oldfd, newfd)
                           │
                     ② Áp dụng spawnattr (signal, scheduler...)
                           │
                     ③ exec(program)
```

#### Vòng đời (lifecycle)

```c
posix_spawn_file_actions_t actions;

/* 1. Khởi tạo — BẮT BUỘC gọi trước khi dùng */
posix_spawn_file_actions_init(&actions);

/* 2. Thêm các hành động (có thể thêm nhiều cái) */
posix_spawn_file_actions_addopen(...);
posix_spawn_file_actions_addclose(...);
posix_spawn_file_actions_adddup2(...);

/* 3. Truyền vào posix_spawn() */
posix_spawnp(&pid, "prog", &actions, NULL, argv, environ);

/* 4. Giải phóng — BẮT BUỘC gọi sau khi xong */
posix_spawn_file_actions_destroy(&actions);
```

#### Các hàm API chi tiết

##### ① `addopen` — Mở file và gán vào fd

```c
int posix_spawn_file_actions_addopen(
    posix_spawn_file_actions_t *file_actions,  /* struct chứa danh sách hành động   */
    int fd,              /* fd muốn gán kết quả vào (vd: 1 = stdout)               */
    const char *path,    /* đường dẫn file cần mở                                  */
    int oflag,           /* cờ mở file (giống open(): O_RDONLY, O_WRONLY, O_CREAT…) */
    mode_t mode          /* quyền file nếu tạo mới (vd: 0644)                      */
);
```

**Tương đương trong fork+exec:**
```c
/* Trong child sau fork(): */
int new_fd = open("output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
dup2(new_fd, STDOUT_FILENO);  /* gán new_fd → stdout */
close(new_fd);
```

##### ② `addclose` — Đóng fd trong child

```c
int posix_spawn_file_actions_addclose(
    posix_spawn_file_actions_t *file_actions,  /* struct chứa danh sách hành động */
    int fd               /* fd cần đóng trong child (vd: đóng đầu đọc của pipe)   */
);
```

**Khi nào dùng?** Khi parent mở pipe hoặc socket, child chỉ cần **một đầu**, đầu kia cần đóng để tránh rò rỉ fd.

##### ③ `adddup2` — Nhân bản fd (giống `dup2`)

```c
int posix_spawn_file_actions_adddup2(
    posix_spawn_file_actions_t *file_actions,  /* struct chứa danh sách hành động */
    int oldfd,           /* fd nguồn (đã tồn tại)                                 */
    int newfd            /* fd đích — sẽ trở thành bản sao của oldfd               */
);
```

**Tương đương:** `dup2(oldfd, newfd)` — sau lệnh này, `newfd` trỏ đến cùng file/pipe với `oldfd`.

#### Ví dụ 1: Redirect stdout → file

```c
#include <spawn.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdio.h>

extern char **environ;

int main(void)
{
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);

    /*
     * Hành động: mở "output.txt" và gán vào fd=1 (stdout).
     * → Mọi thứ child ghi ra stdout sẽ vào file thay vì terminal.
     *
     * Tham số:
     *   &actions    — struct chứa hành động
     *   1           — fd đích = STDOUT_FILENO
     *   "output.txt"— file cần mở
     *   O_WRONLY | O_CREAT | O_TRUNC — ghi, tạo mới, xóa nội dung cũ
     *   0644        — quyền: owner đọc/ghi, group+others chỉ đọc
     */
    posix_spawn_file_actions_addopen(&actions, 1,
        "output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);

    pid_t pid;
    char *argv[] = {"ls", "-la", NULL};

    posix_spawnp(&pid, "ls", &actions, NULL, argv, environ);

    int status;
    waitpid(pid, &status, 0);
    posix_spawn_file_actions_destroy(&actions);

    printf("Output đã được ghi vào output.txt\n");
    return 0;
}
```

#### Ví dụ 2: Tạo pipe giữa 2 child (`ls | wc -l`)

```c
#include <spawn.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>

extern char **environ;

int main(void)
{
    int pipefd[2];
    pipe(pipefd);
    /* pipefd[0] = đầu đọc (read end)
     * pipefd[1] = đầu ghi (write end)  */

    /* ---- Child 1: "ls" — stdout → pipe write end ---- */
    posix_spawn_file_actions_t act1;
    posix_spawn_file_actions_init(&act1);
    posix_spawn_file_actions_adddup2(&act1, pipefd[1], 1);  /* stdout = pipe write */
    posix_spawn_file_actions_addclose(&act1, pipefd[0]);    /* đóng đầu đọc (không cần) */
    posix_spawn_file_actions_addclose(&act1, pipefd[1]);    /* đóng bản gốc (đã dup2) */

    pid_t pid1;
    char *argv1[] = {"ls", NULL};
    posix_spawnp(&pid1, "ls", &act1, NULL, argv1, environ);
    posix_spawn_file_actions_destroy(&act1);

    /* ---- Child 2: "wc -l" — stdin ← pipe read end ---- */
    posix_spawn_file_actions_t act2;
    posix_spawn_file_actions_init(&act2);
    posix_spawn_file_actions_adddup2(&act2, pipefd[0], 0);  /* stdin = pipe read */
    posix_spawn_file_actions_addclose(&act2, pipefd[1]);    /* đóng đầu ghi (không cần) */
    posix_spawn_file_actions_addclose(&act2, pipefd[0]);    /* đóng bản gốc (đã dup2) */

    pid_t pid2;
    char *argv2[] = {"wc", "-l", NULL};
    posix_spawnp(&pid2, "wc", &act2, NULL, argv2, environ);
    posix_spawn_file_actions_destroy(&act2);

    /* Parent: đóng cả 2 đầu pipe (đã truyền cho child) */
    close(pipefd[0]);
    close(pipefd[1]);

    /* Đợi cả 2 child */
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);

    return 0;
}
```

---

### `posix_spawnattr_t` — Thuộc tính process của child

#### Nó là gì?

`posix_spawnattr_t` là một **struct chứa các thuộc tính hệ thống** mà child process sẽ được gán. Trong khi `file_actions` điều khiển **file descriptor**, thì `spawnattr` điều khiển:

- **Signal mask** — child sẽ chặn/mở signal nào
- **Signal default** — reset handler signal nào về mặc định
- **Scheduling** — chính sách lập lịch (SCHED_FIFO, SCHED_RR...)
- **Process group** — gán child vào group nào
- **Flags** — bật/tắt các tính năng trên

#### Vòng đời (lifecycle)

```c
posix_spawnattr_t attr;

/* 1. Khởi tạo */
posix_spawnattr_init(&attr);

/* 2. Cấu hình các thuộc tính */
posix_spawnattr_setflags(&attr, flags);
posix_spawnattr_setsigmask(&attr, &sigmask);
posix_spawnattr_setsigdefault(&attr, &sigdefault);
posix_spawnattr_setpgroup(&attr, pgroup);
posix_spawnattr_setschedpolicy(&attr, policy);
posix_spawnattr_setschedparam(&attr, &param);

/* 3. Truyền vào posix_spawn() */
posix_spawnp(&pid, "prog", NULL, &attr, argv, environ);

/* 4. Giải phóng */
posix_spawnattr_destroy(&attr);
```

#### Flags — Bật tính năng nào?

Bạn **phải** dùng `posix_spawnattr_setflags()` để cho kernel biết **thuộc tính nào cần áp dụng**. Nếu không set flag, thuộc tính tương ứng bị bỏ qua dù bạn đã cấu hình.

```c
int posix_spawnattr_setflags(
    posix_spawnattr_t *attr,   /* struct thuộc tính           */
    short flags                /* tổ hợp OR của các cờ dưới   */
);
```

| Flag | Ý nghĩa |
|---|---|
| `POSIX_SPAWN_SETSIGMASK` | Áp dụng signal mask (chặn/mở signal) cho child |
| `POSIX_SPAWN_SETSIGDEF` | Reset các signal handler được chỉ định về `SIG_DFL` (mặc định) |
| `POSIX_SPAWN_SETPGROUP` | Gán child vào process group cụ thể |
| `POSIX_SPAWN_SETSCHEDULER` | Áp dụng chính sách scheduling (FIFO, RR...) |
| `POSIX_SPAWN_SETSCHEDPARAM` | Áp dụng scheduling priority |
| `POSIX_SPAWN_RESETIDS` | Reset effective UID/GID về real UID/GID |

#### Các hàm API chi tiết

##### ① Signal Mask — Child chặn signal nào?

```c
int posix_spawnattr_setsigmask(
    posix_spawnattr_t *attr,   /* struct thuộc tính                */
    const sigset_t *sigmask    /* tập signal mà child sẽ CHẶN      */
);
```

**Ý nghĩa**: Khi parent đang chặn (block) một số signal (ví dụ để xử lý critical section), nhưng muốn child **không kế thừa** việc chặn đó → dùng hàm này để set signal mask sạch cho child.

##### ② Signal Default — Reset handler về mặc định

```c
int posix_spawnattr_setsigdefault(
    posix_spawnattr_t *attr,      /* struct thuộc tính                    */
    const sigset_t *sigdefault    /* tập signal cần reset về SIG_DFL      */
);
```

**Ý nghĩa**: Nếu parent đã cài handler tùy chỉnh cho một số signal, child sau `exec()` sẽ kế thừa. Hàm này ép các signal chỉ định quay về hành vi mặc định.

##### ③ Process Group

```c
int posix_spawnattr_setpgroup(
    posix_spawnattr_t *attr,   /* struct thuộc tính                      */
    pid_t pgroup               /* 0 = child tạo group mới, >0 = join group đó */
);
```

##### ④ Scheduling

```c
int posix_spawnattr_setschedpolicy(
    posix_spawnattr_t *attr,   /* struct thuộc tính                */
    int policy                 /* SCHED_OTHER, SCHED_FIFO, SCHED_RR */
);

int posix_spawnattr_setschedparam(
    posix_spawnattr_t *attr,           /* struct thuộc tính        */
    const struct sched_param *param    /* chứa sched_priority      */
);
```

#### Ví dụ: Spawn child với signal mask sạch

```c
#include <spawn.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>

extern char **environ;

int main(void)
{
    /*
     * Tình huống: Parent đang chặn SIGINT (Ctrl+C),
     * nhưng muốn child nhận SIGINT bình thường.
     */

    /* Parent chặn SIGINT */
    sigset_t block_set;
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGINT);
    sigprocmask(SIG_BLOCK, &block_set, NULL);

    /* Cấu hình attr: child sẽ có signal mask rỗng (không chặn gì) */
    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);

    /* Bật flag: áp dụng signal mask + reset signal default */
    posix_spawnattr_setflags(&attr,
        POSIX_SPAWN_SETSIGMASK | POSIX_SPAWN_SETSIGDEF);

    /* Signal mask cho child: rỗng → không chặn signal nào */
    sigset_t empty_mask;
    sigemptyset(&empty_mask);
    posix_spawnattr_setsigmask(&attr, &empty_mask);

    /* Reset SIGINT về hành vi mặc định trong child */
    sigset_t default_sigs;
    sigemptyset(&default_sigs);
    sigaddset(&default_sigs, SIGINT);
    posix_spawnattr_setsigdefault(&attr, &default_sigs);

    /* Spawn child */
    pid_t pid;
    char *argv[] = {"sleep", "10", NULL};
    posix_spawnp(&pid, "sleep", NULL, &attr, argv, environ);

    posix_spawnattr_destroy(&attr);

    printf("Child PID=%d — thử nhấn Ctrl+C, child sẽ bị kill nhưng parent thì không\n", pid);

    int status;
    waitpid(pid, &status, 0);

    if (WIFSIGNALED(status)) {
        printf("Child bị giết bởi signal %d\n", WTERMSIG(status));
    }

    return 0;
}
```

**Biên dịch:**

```bash
gcc -o spawn_demo spawn_demo.c
```

---

## 9. Zombie & Orphan Process

### Tại sao Parent cần gọi waitpid()?

Việc Parent gọi `waitpid()` không chỉ để "chặn" luồng chạy, mà nó giải quyết 3 mục đích cốt lõi trong kiến trúc Linux:

1. **Để dọn rác (Tránh thảm họa Zombie Process) — Quan trọng nhất:**
   Khi child thoát (`exit`), kernel giải phóng RAM nhưng vẫn giữ lại một "vỏ rỗng" (Process Control Block) chứa exit code và PID. Vỏ rỗng này chờ Parent tới lấy. `waitpid()` chính là hành động "đến lấy kết quả và ký giấy khai tử". Nếu không gọi `waitpid()`, vỏ rỗng này biến thành Zombie. Quá nhiều Zombie sẽ làm cạn kiệt số lượng PID của hệ thống.
2. **Để lấy kết quả chạy của Child (Exit Status):**
   Parent cần biết child chạy thành công hay thất bại (ví dụ: lệnh `gcc` build code có lỗi không). Dữ liệu này được kernel nhét vào biến `status` của hàm `waitpid`.
3. **Để đồng bộ hóa luồng (Synchronization):**
   Giống như Terminal (`bash`), khi bạn gõ lệnh `ls`, Terminal sinh ra child chạy `ls` và phải gọi `waitpid()` để chờ `ls` in xong kết quả, sau đó mới in lại dấu nhắc lệnh `~$` cho bạn nhập tiếp.

### Zombie Process

Khi child **đã thoát** nhưng parent **chưa gọi `wait()`**, child trở thành **zombie**.

```c
/* Tạo zombie process */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(void)
{
    pid_t pid = fork();

    if (pid == 0) {
        printf("Child PID=%d thoát ngay\n", getpid());
        _exit(0);  /* Child thoát → trở thành zombie */
    }

    /* Parent KHÔNG gọi wait() → zombie tồn tại */
    printf("Parent ngủ 30s, kiểm tra zombie bằng: ps aux | grep Z\n");
    sleep(30);

    return 0;
}
```

### Cách xử lý Zombie

**Cách 1**: Luôn gọi `wait()` hoặc `waitpid()`.

**Cách 2**: Bắt signal `SIGCHLD` để thu hồi tự động.

```c
#include <signal.h>
#include <sys/wait.h>

void sigchld_handler(int sig)
{
    (void)sig;
    /* Thu hồi TẤT CẢ child đã thoát (non-blocking) */
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
}

int main(void)
{
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    /* ... fork() các child process ... */
}
```

**Cách 3**: Ignore `SIGCHLD` (kernel tự thu hồi).

```c
signal(SIGCHLD, SIG_IGN);
```

### Orphan Process

Khi **parent thoát trước child**, child trở thành **orphan** và được `init` (PID=1) "nhận nuôi".

---

## 10. So sánh các phương pháp

| Phương pháp | Ưu điểm | Nhược điểm | Khi nào dùng |
|---|---|---|---|
| `fork() + exec()` | Linh hoạt, kiểm soát hoàn toàn | Code dài hơn | Mọi trường hợp production |
| `system()` | Đơn giản, nhanh | Không an toàn, kém linh hoạt | Script/tool đơn giản |
| `posix_spawn()` | Tối ưu, API gọn | Ít phổ biến, ít tài liệu hơn | Embedded, hiệu năng cao |
| `fork()` đơn lẻ | Chạy code song song | Dùng nhiều bộ nhớ (COW giảm thiểu) | Multiprocessing cùng code |

---

## 11. Ví dụ tổng hợp

Chương trình spawn nhiều child process, mỗi child chạy một lệnh khác nhau:

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

typedef struct {
    const char *name;
    char *const *argv;
} Command;

static void run_commands(Command *cmds, int count)
{
    pid_t pids[count];

    /* Spawn tất cả child */
    for (int i = 0; i < count; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if (pids[i] == 0) {
            /* CHILD */
            execvp(cmds[i].argv[0], cmds[i].argv);
            perror(cmds[i].name);
            _exit(127);
        }
        printf("[Spawned] %-10s → PID %d\n", cmds[i].name, pids[i]);
    }

    /* Đợi tất cả child */
    for (int i = 0; i < count; i++) {
        int status;
        waitpid(pids[i], &status, 0);

        if (WIFEXITED(status)) {
            printf("[Done]    %-10s → exit code %d\n",
                   cmds[i].name, WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("[Killed]  %-10s → signal %d\n",
                   cmds[i].name, WTERMSIG(status));
        }
    }
}

int main(void)
{
    char *argv_uname[]  = {"uname", "-a", NULL};
    char *argv_date[]   = {"date", NULL};
    char *argv_whoami[] = {"whoami", NULL};

    Command cmds[] = {
        {"uname",  argv_uname},
        {"date",   argv_date},
        {"whoami", argv_whoami},
    };

    int count = sizeof(cmds) / sizeof(cmds[0]);
    run_commands(cmds, count);

    return 0;
}
```

**Biên dịch & chạy:**

```bash
gcc -Wall -Wextra -o multi_spawn multi_spawn.c
./multi_spawn
```

**Output mẫu:**

```
[Spawned] uname      → PID 5001
[Spawned] date       → PID 5002
[Spawned] whoami     → PID 5003
Linux myhost 5.15.0 ... x86_64 GNU/Linux
Thu Jun  6 14:00:00 +07 2026
ledat
[Done]    uname      → exit code 0
[Done]    date       → exit code 0
[Done]    whoami     → exit code 0
```

---

## Tham khảo

- `man 2 fork` · `man 3 exec` · `man 2 waitpid` · `man 3 system` · `man 3 posix_spawn`
- [The Linux Programming Interface — Michael Kerrisk](https://man7.org/tlpi/)
- [Advanced Programming in the UNIX Environment — Stevens & Rago](https://www.apuebook.com/)
