Trong Linux, **“I/O object” (đối tượng I/O)** không phải là một kiểu dữ liệu chính thức trong kernel, mà là **khái niệm trừu tượng** chỉ **bất kỳ thực thể nào mà tiến trình có thể đọc (read) hoặc ghi (write)**.

Linux thiết kế theo triết lý:

> **“Everything is a file.”**

Nghĩa là hầu hết các tài nguyên I/O đều được **biểu diễn dưới dạng một file-like interface**.

Vì vậy, khi nói **I/O object**, ta thường đang nói đến:

* file
* socket
* pipe
* device
* terminal
* eventfd / timerfd / epollfd
* v.v.

Tất cả những thứ này đều có thể được truy cập thông qua **file descriptor (FD)**.

---

# 1. Định nghĩa đơn giản

**I/O object = một thực thể trong kernel hỗ trợ thao tác input/output thông qua file descriptor.**

Ví dụ:

```c
int fd = open("data.txt", O_RDONLY);
read(fd, buf, 100);
```

Ở đây:

* `data.txt` → I/O object
* `fd` → handle trỏ tới object đó

---

# 2. Các loại I/O object phổ biến

## 1. Regular File

File bình thường trên disk.

Ví dụ:

```
/home/user/file.txt
```

Syscall:

```c
open()
read()
write()
lseek()
```

---

## 2. Device File

Thiết bị phần cứng được expose thành file trong `/dev`.

Ví dụ:

```
/dev/tty
/dev/sda
/dev/i2c-1
/dev/spidev0.0
```

Ví dụ đọc từ ADC driver:

```c
int fd = open("/dev/adc0", O_RDONLY);
read(fd, buf, 4);
```

Driver kernel xử lý request.

---

## 3. Pipe

Cơ chế IPC giữa process.

```c
pipe(fd);
```

Kernel tạo **pipe object**.

```
process A → write → pipe → read → process B
```

---

## 4. Socket

Network communication.

```c
socket()
bind()
connect()
send()
recv()
```

Socket cũng là **I/O object**.

---

## 5. Terminal / TTY

Ví dụ:

```
/dev/tty
/dev/pts/0
```

STDIN/STDOUT cũng là TTY.

---

## 6. Event-based objects

Linux còn có các object phục vụ event-driven programming:

| Object   | Mục đích           |
| -------- | ------------------ |
| eventfd  | event notification |
| timerfd  | timer              |
| signalfd | nhận signal        |
| epollfd  | multiplexing       |

Tất cả đều trả về **file descriptor**.

---

# 3. Tại sao Linux gom tất cả thành I/O object

Lý do là **unified interface**.

Thay vì API khác nhau:

```
read_file()
read_socket()
read_device()
```

Linux dùng:

```
read()
write()
```

Ví dụ:

```c
read(fd, buf, size);
```

`fd` có thể là:

* file
* socket
* pipe
* device
* tty

Kernel dispatch tới implementation tương ứng.

---

# 4. Bên trong kernel: I/O object được biểu diễn thế nào

Trong kernel, mỗi I/O object thường gắn với:

```
struct file
```

Process table:

```
task_struct
   │
   └── files_struct
           │
           └── fdtable
                   │
                   └── struct file *
```

`struct file` chứa:

```
struct file {
    struct file_operations *f_op;
    void *private_data;
}
```

`file_operations`:

```c
struct file_operations {
    ssize_t (*read)(...);
    ssize_t (*write)(...);
    int (*open)(...);
}
```

Kernel gọi function pointer tùy object.

---

# 5. Flow khi process đọc I/O object

Ví dụ:

```c
read(fd, buf, 100);
```

Flow:

```
user space
   │
   └─ syscall read()
        │
        └─ fd → struct file
                │
                └─ file->f_op->read()
                        │
                        ├─ filesystem read
                        ├─ socket read
                        ├─ pipe read
                        └─ device driver read
```

---

# 6. Ví dụ cụ thể

### File

```
fd = open("data.txt")
read(fd)
```

→ VFS → ext4 driver

---

### Socket

```
fd = socket()
read(fd)
```

→ TCP stack

---

### Serial

```
fd = open("/dev/ttyUSB0")
read(fd)
```

→ UART driver

---

# 7. Quan hệ giữa I/O object và File Descriptor

Rất quan trọng:

```
I/O object (kernel object)
        ↑
     struct file
        ↑
File Descriptor (int)
        ↑
    process
```

FD chỉ là **index** vào table.

---

# 8. Những API dùng I/O object

Các syscall hoạt động với **mọi I/O object**:

```
read
write
close
poll
select
epoll
fcntl
ioctl
```

Ví dụ:

```c
epoll_wait()
```

có thể monitor:

* socket
* pipe
* eventfd
* timerfd
* tty

---

# 9. Ví dụ cực kỳ hay trong Linux

Bạn có thể:

```
cat /dev/random
cat /proc/cpuinfo
cat /sys/class/thermal/thermal_zone0/temp
```

Các file này **không tồn tại trên disk**.

Nhưng vẫn là **I/O object**.

---

# 10. Kết luận

**I/O object trong Linux là:**

> Bất kỳ thực thể kernel nào hỗ trợ thao tác read/write thông qua file descriptor.

Các loại chính:

```
Regular file
Device
Pipe
Socket
TTY
Event objects
```

Tất cả được truy cập bằng **cùng interface syscall**:

```
open
read
write
close
poll
epoll
```

---

Nếu bạn muốn, tôi có thể giải thích tiếp một phần **rất quan trọng nhưng ít người hiểu đúng**:

### 👉 **Sự khác nhau giữa**

```
file
file descriptor
inode
dentry
struct file
```

Đây là **5 khái niệm cốt lõi của Linux I/O** và hiểu được chúng thì **Linux kernel I/O gần như sáng tỏ hoàn toàn**.
