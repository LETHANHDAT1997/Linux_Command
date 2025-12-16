# **Mục lục**

* Giới thiệu Bash Script
* Biến và dữ liệu vào
* Cấu trúc điều khiển (if, for, while, case)
* Thao tác với tệp và thư mục
* Xử lý văn bản
* Toán tử trong Bash (số học, chuỗi, logic, file test, nâng cao)
* Quyền và thực thi script
* Ví dụ thực tế (script kiểm tra, script backup)
* Các lưu ý và best practices

---

# **Hướng dẫn Bash Script cơ bản và nâng cao**

## **`1. Bash Script là gì?`**

* **Bash Script** là tập hợp các lệnh terminal được lưu vào một tệp tin để thực thi tuần tự, tự động.
* Giống như viết một "to-do list" cho shell: thay vì gõ từng lệnh, bạn để script làm thay bạn.

Ví dụ đơn giản:

```bash
#!/bin/bash
echo "Bắt đầu kiểm tra..."
pwd
ls -l
echo "Đã kiểm tra xong!"
```

Chạy bằng:

```bash
chmod +x kiemtra.sh
./kiemtra.sh
```

---

## **`2. Biến và Dữ liệu vào`**

* **Khai báo biến:**

  ```bash
  ten_bien="Gia_tri"
  ```
* **Sử dụng biến:**

  ```bash
  echo $ten_bien
  ```
* **Đọc dữ liệu người dùng nhập:**

  ```bash
  read -p "Nhập tên của bạn: " ten
  echo "Chào bạn, $ten"
  ```

---

## **`3. Cấu trúc điều khiển`**

### If-Else

```bash
if [ dieu_kien ]; then
    # ...
elif [ dieu_kien_khac ]; then
    # ...
else
    # ...
fi
```

Ví dụ:

```bash
read -p "Nhập số: " so
if [ $so -gt 10 ]; then
  echo "Lớn hơn 10"
else
  echo "≤ 10"
fi
```

### Vòng lặp For

```bash
for i in {1..5}; do
   echo "Số: $i"
done
```

### Vòng lặp While

```bash
dem=1
while [ $dem -le 5 ]; do
    echo "Đếm: $dem"
    ((dem++))
done
```

### Câu lệnh Case

```bash
read -p "Chọn (a/b): " lua_chon
case $lua_chon in
  a) echo "Bạn chọn A";;
  b) echo "Bạn chọn B";;
  *) echo "Không hợp lệ";;
esac
```

---

## **`4. Thao tác với Tệp và Thư mục`**

* ls, cd, pwd
* mkdir ten_thu_muc
* touch ten_file.txt
* cp file_nguon file_dich
* mv ten_cu ten_moi
* rm file | rm -r thu_muc

---

## **`5. Xử lý Văn bản`**

* echo "Xin chào!"
* cat file.txt
* grep "chuoi" file.txt
* head -n 5 file.txt
* tail -n 5 file.txt
* wc -l file.txt

---

## **`6. Toán tử trong Bash`**

### **So sánh số học**

| Toán tử | Ý nghĩa           |
| ------- | ----------------- |
| -eq     | Bằng              |
| -ne     | Không bằng        |
| -gt     | Lớn hơn           |
| -ge     | Lớn hơn hoặc bằng |
| -lt     | Nhỏ hơn           |
| -le     | Nhỏ hơn hoặc bằng |

### **So sánh chuỗi**

* `=` : bằng
* `!=` : không bằng
* `-z str` : chuỗi rỗng
* `-n str` : chuỗi không rỗng

### **Toán tử logic**

* `&&` : và
* `||` : hoặc
* `!` : phủ định

### **Kiểm tra tệp**

* `-d` : thư mục tồn tại
* `-f` : tệp tin thường
* `-e` : tồn tại
* `-r/-w/-x` : quyền đọc/ghi/thực thi

### **Nâng cao**

#### 1. Toán tử so khớp biểu thức chính quy (`=~`)

* **Mục đích:** kiểm tra chuỗi có khớp với regex hay không.
* **Chỉ dùng được trong `[[ ... ]]`.**
* **Ví dụ:**

  ```bash
  read -p "Nhập email: " email
  if [[ $email =~ ^[a-z0-9._%+-]+@[a-z0-9.-]+\.[a-z]{2,}$ ]]; then
    echo "Email hợp lệ"
  else
    echo "Email không hợp lệ"
  fi
  ```

#### 2. So khớp mẫu (wildcards) với `==`

* Khi dùng trong `[[ ... ]]`, ký tự `*` và `?` sẽ được hiểu như **wildcard**.
* **Ví dụ:**

  ```bash
  file="report.txt"
  if [[ $file == *.txt ]]; then
    echo "Đây là file văn bản"
  fi
  ```

#### 3. Toán tử thao tác bit (bitwise)

* Dùng trong biểu thức số học `(( ... ))`.
* **Ví dụ:**

  ```bash
  a=5   # 0101
  b=3   # 0011
  (( c = a & b ))   # AND → 0001
  echo "a & b = $c"   # 1

  (( d = a | b ))   # OR → 0111
  echo "a | b = $d"   # 7
  ```

#### 4. Toán tử trên biến (Parameter Expansion)

#### a. Cắt bỏ tiền tố/hậu tố

* `${var#pattern}` : bỏ tiền tố ngắn nhất khớp pattern.
* `${var##pattern}` : bỏ tiền tố dài nhất.
* `${var%pattern}` : bỏ hậu tố ngắn nhất.
* `${var%%pattern}` : bỏ hậu tố dài nhất.
* **Ví dụ:**

  ```bash
  path="/home/user/file.txt"
  echo ${path#*/}   # home/user/file.txt
  echo ${path##*/}  # file.txt
  echo ${path%/*}   # /home/user
  echo ${path%%/*}  # (rỗng, vì bỏ hết sau dấu / đầu tiên)
  ```

#### b. Gán giá trị mặc định

* `${var:-default}` : dùng giá trị mặc định nếu biến rỗng/chưa đặt.
* `${var:=default}` : giống trên nhưng **gán luôn** cho biến.
* `${var:+alt}` : trả về `alt` nếu biến **đã được đặt**.
* `${var:?error}` : báo lỗi và dừng script nếu biến rỗng.
* **Ví dụ:**

  ```bash
  echo ${username:-"guest"}   # nếu $username rỗng → in "guest"

  unset name
  echo ${name:="Unknown"}     # gán "Unknown" vào name
  echo $name                  # Unknown

  user="admin"
  echo ${user:+logged_in}     # in "logged_in"
  ```
---

## **`7. Quyền và Thực thi`**

* `chmod +x script.sh` : cấp quyền chạy
* `./script.sh` : chạy script
* `#` : viết chú thích
* `sleep 5` : dừng 5s
* `exit 0` : kết thúc thành công

---

## **`8. Ví dụ thực tế`**

### Tạo thư mục backup theo ngày

```bash
#!/bin/bash
TODAY=$(date +"%Y-%m-%d")
BACKUP_DIR="backup-$TODAY"

if [[ ! -d "$BACKUP_DIR" ]]; then
  mkdir "$BACKUP_DIR"
fi

cp *.log "$BACKUP_DIR/"
echo "Backup hoàn tất: $BACKUP_DIR"
```

---


## **`9. Lưu ý & Best Practices (giải thích chi tiết)`**

### 1. Luôn dùng dấu ngoặc kép quanh biến: `"$var"`

* **Tại sao?**
  Nếu biến chứa khoảng trắng hoặc ký tự đặc biệt, việc không đặt trong dấu ngoặc kép sẽ khiến Bash coi đó là nhiều đối số khác nhau.
* **Ví dụ sai:**

  ```bash
  filename="my file.txt"
  rm $filename   # lỗi! rm sẽ hiểu là "my" và "file.txt"
  ```
* **Ví dụ đúng:**

  ```bash
  rm "$filename" # rm xử lý đúng tệp "my file.txt"
  ```

---

### 2. Dùng `[[ ... ]]` thay vì `[ ... ]` khi có thể

* **Tại sao?**

  * `[[ ... ]]` là cú pháp mở rộng của Bash, an toàn hơn khi xử lý chuỗi có ký tự đặc biệt (như `*`, `>`, `<`).
  * Hỗ trợ regex (`=~`) và so sánh chuỗi linh hoạt hơn.
* **Ví dụ sai (dùng `[ ]` dễ gây lỗi):**

  ```bash
  str="a*b"
  if [ $str = "a*b" ]; then   # shell có thể hiểu * là wildcard
    echo "Match"
  fi
  ```
* **Ví dụ đúng:**

  ```bash
  if [[ $str == "a*b" ]]; then
    echo "Match"
  fi
  ```

---

### 3. Thêm `set -e` vào đầu script

* **Tại sao?**

  * Mặc định, Bash vẫn chạy tiếp khi gặp lỗi (exit code ≠ 0), điều này nguy hiểm nếu script đang sao lưu/xóa dữ liệu.
  * `set -e` giúp script dừng ngay khi có lệnh lỗi → an toàn hơn.
* **Ví dụ không dùng `set -e`:**

  ```bash
  cp important.txt /backup/
  rm important.txt   # nếu cp thất bại, rm vẫn chạy → mất dữ liệu
  ```
* **Ví dụ dùng `set -e`:**

  ```bash
  set -e
  cp important.txt /backup/
  rm important.txt   # nếu cp thất bại, script dừng, rm không chạy
  ```

---

### 4. Ghi chú rõ ràng bằng `#`

* **Tại sao?**

  * Script càng dài càng khó nhớ mục đích từng dòng.
  * Chú thích giúp người khác (hoặc chính bạn sau này) hiểu nhanh hơn.
* **Ví dụ:**

  ```bash
  # Sao lưu các file log vào thư mục backup hằng ngày
  TODAY=$(date +"%Y-%m-%d")
  BACKUP_DIR="backup-$TODAY"
  mkdir -p "$BACKUP_DIR"
  cp *.log "$BACKUP_DIR/"
  ```

---

### 5. Kiểm tra điều kiện tồn tại trước khi thao tác (idempotency)

* **Tại sao?**

  * Tránh lỗi khi thư mục/ tập tin đã tồn tại hoặc không có.
* **Ví dụ:**

  ```bash
  if [[ ! -d "$BACKUP_DIR" ]]; then
    mkdir "$BACKUP_DIR"
  fi
  ```

---

### 6. Trả về mã thoát rõ ràng (`exit 0`, `exit 1`)

* **Tại sao?**

  * Giúp các script khác (hoặc cronjob, systemd) biết script thành công hay thất bại.
* **Ví dụ:**

  ```bash
  if cp *.log "$BACKUP_DIR/"; then
    echo "Backup thành công"
    exit 0
  else
    echo "Backup thất bại"
    exit 1
  fi
  ```

---




