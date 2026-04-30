Nếu bạn đang nghĩ “liệt kê *tất cả*” theo nghĩa tuyệt đối thì cần nói thẳng: Bash có hàng trăm built-ins, hàng ngàn lệnh từ hệ sinh thái GNU, và vô số pattern/ký hiệu. Không có danh sách “đầy đủ tuyệt đối” theo kiểu học thuộc. Cách hiệu quả là nắm **taxonomy (phân loại)** + những thứ cốt lõi.

Dưới đây là bản “full map” có hệ thống để bạn học Bash script bài bản.

---

# 1. Nền tảng: Bash là gì?

* Shell phổ biến nhất: Bash
* Tuân theo tiêu chuẩn POSIX (một phần)

---

# 2. Built-in commands (lệnh nội bộ Bash)

Đây là các lệnh *không cần gọi binary ngoài*:

### Điều khiển luồng

* `if`, `then`, `else`, `elif`, `fi`
* `case`, `esac`
* `for`, `while`, `until`, `do`, `done`
* `break`, `continue`

### Hàm & scope

* `function`, `return`
* `declare`, `local`, `readonly`

### Biến & môi trường

* `export`, `unset`
* `set`, `unset`
* `env`

### I/O & shell control

* `echo`, `printf`, `read`
* `exec`, `exit`
* `source` (`.`)

### Job control

* `jobs`, `fg`, `bg`, `wait`, `kill`

---

# 3. External commands (lệnh hệ thống thường dùng)

Những lệnh này đến từ GNU/Linux:

### File & directory

* `ls`, `cp`, `mv`, `rm`, `mkdir`, `touch`

### Text processing (rất quan trọng)

* `cat`, `grep`, `awk`, `sed`, `cut`, `sort`, `uniq`, `tr`

### System

* `ps`, `top`, `kill`, `df`, `du`, `free`

### Network

* `curl`, `wget`, `ping`, `ssh`

---

# 4. Operators & ký hiệu quan trọng

## 4.1 Redirection

* `>` ghi đè
* `>>` append
* `<` input
* `2>` stderr
* `&>` stdout + stderr
* `<<` heredoc

## 4.2 Pipe

* `|` truyền output → input
* `|&` cả stderr

## 4.3 Logical

* `&&` AND
* `||` OR
* `!` NOT

## 4.4 Arithmetic

* `$(( ))`
* `+ - * / %`

## 4.5 Comparison

### String

* `=`, `!=`, `<`, `>`

### Number

* `-eq`, `-ne`, `-lt`, `-le`, `-gt`, `-ge`

### File test

* `-f`, `-d`, `-e`, `-r`, `-w`, `-x`

---

# 5. Biến trong Bash

## 5.1 Khai báo

```bash
x=10
name="Dat"
```

## 5.2 Truy cập

```bash
echo $x
echo ${name}
```

## 5.3 Biến môi trường

```bash
export PATH=/usr/bin
```

---

# 6. Special variables (rất quan trọng khi scripting)

* `$0` → tên script
* `$1..$9` → tham số
* `$#` → số lượng args
* `$@` → tất cả args
* `$*` → args (khác subtle với `$@`)
* `$?` → exit code
* `$$` → PID
* `$!` → PID process background
* `$RANDOM` → số random
* `$SECONDS` → thời gian chạy

---

# 7. Expansion (điểm cực kỳ quan trọng)

## 7.1 Parameter expansion

```bash
${var}
${var:-default}
${var:=default}
${var:+value}
${var:?error}
```

## 7.2 Command substitution

```bash
$(command)
`command`
```

## 7.3 Arithmetic expansion

```bash
$((a + b))
```

## 7.4 Brace expansion

```bash
{1..10}
file{A,B}.txt
```

## 7.5 Globbing

* `*` tất cả
* `?` 1 ký tự
* `[abc]`

---

# 8. Quoting (dễ sai nhất)

* `" "` → giữ biến, giữ space
* `' '` → literal
* `\` → escape

---

# 9. Function

```bash
my_func() {
    echo "Hello"
}
```

---

# 10. Control flow (thực chiến)

## if

```bash
if [ $x -gt 10 ]; then
    echo "big"
fi
```

## for

```bash
for i in {1..5}; do
    echo $i
done
```

## while

```bash
while read line; do
    echo $line
done < file.txt
```

---

# 11. Arrays

```bash
arr=(a b c)
echo ${arr[0]}
echo ${arr[@]}
```

---

# 12. Advanced (phỏng vấn hay hỏi)

* Subshell: `( ... )`
* Process substitution:

```bash
diff <(ls dir1) <(ls dir2)
```

* Trap signal:

```bash
trap "echo exit" SIGINT
```

* set flags:

```bash
set -e   # exit on error
set -u   # undefined var error
set -x   # debug
```

---

# 13. Best practices (quan trọng hơn học thuộc)

* Luôn quote biến: `"${var}"`
* Dùng `set -euo pipefail`
* Tránh dùng backticks, dùng `$(...)`
* Dùng `[[ ... ]]` thay vì `[ ... ]` khi có thể

---

# 14. Nếu bạn học để đi phỏng vấn (embedded/Linux)

Tập trung mạnh vào:

* process & pipe
* file descriptor (`0,1,2`)
* `fork/exec` liên quan shell
* signal + trap
* text processing (`grep/awk/sed`)

---

# Kết luận

Bạn không cần (và không nên) học thuộc toàn bộ. Thứ bạn cần là:

1. Hiểu cơ chế shell (expansion → execution)
2. Thành thạo 20% lệnh dùng 80% thời gian
3. Đọc script người khác và debug được

---

