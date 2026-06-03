# 🔄 Vòng Lặp Trong Python (Loops)

Vòng lặp giúp **lặp lại** một khối lệnh nhiều lần mà không cần viết lại code.

Python có **2 loại** vòng lặp chính:
- `for` — lặp qua một dãy (list, tuple, string, range, ...)
- `while` — lặp khi điều kiện còn đúng

---

## 1. Vòng Lặp `for`

Dùng để **duyệt qua từng phần tử** trong một dãy (iterable).

### 1.1 Cú pháp cơ bản

```python
for biến in dãy:
    # code thực thi cho mỗi phần tử
```

#### Lặp qua list
```python
fruits = ["táo", "cam", "chuối"]

for fruit in fruits:
    print(fruit)
# táo
# cam
# chuối
```

#### Lặp qua string (từng ký tự)
```python
for char in "Python":
    print(char, end=" ")
# P y t h o n
```

#### Lặp qua tuple
```python
colors = ("đỏ", "xanh", "vàng")

for color in colors:
    print(color)
```

#### Lặp qua dict
```python
person = {"name": "An", "age": 25, "city": "HCM"}

# Duyệt key (mặc định)
for key in person:
    print(key)               # name, age, city

# Duyệt value
for val in person.values():
    print(val)               # An, 25, HCM

# Duyệt cả key và value
for key, val in person.items():
    print(f"{key}: {val}")   # name: An, age: 25, city: HCM
```

#### Lặp qua set
```python
nums = {3, 1, 4, 1, 5}      # set tự loại trùng → {1, 3, 4, 5}

for n in nums:
    print(n)                 # thứ tự không đảm bảo
```

---

### 1.2 Hàm `range()` — tạo dãy số

`range()` là hàm **rất hay dùng** với `for` để lặp theo số lần cụ thể.

```python
# range(stop) — từ 0 đến stop-1
for i in range(5):
    print(i, end=" ")
# 0 1 2 3 4

# range(start, stop) — từ start đến stop-1
for i in range(2, 6):
    print(i, end=" ")
# 2 3 4 5

# range(start, stop, step) — bước nhảy
for i in range(0, 10, 2):
    print(i, end=" ")
# 0 2 4 6 8

# Đếm ngược
for i in range(5, 0, -1):
    print(i, end=" ")
# 5 4 3 2 1
```

#### Ứng dụng: lặp N lần
```python
# In "Xin chào" 3 lần
for _ in range(3):           # _ là biến tạm, không cần dùng
    print("Xin chào!")
```

#### Ứng dụng: truy cập list theo chỉ số
```python
fruits = ["táo", "cam", "chuối"]

for i in range(len(fruits)):
    print(f"[{i}] {fruits[i]}")
# [0] táo
# [1] cam
# [2] chuối

# ⭐ Cách tốt hơn: dùng enumerate() (xem mục 1.4)
```

---

### 1.3 `for...else` — else sau vòng lặp

Khối `else` chạy khi vòng lặp kết thúc **bình thường** (không bị `break`).

```python
# Tìm số chẵn trong list
nums = [1, 3, 5, 7, 9]

for n in nums:
    if n % 2 == 0:
        print(f"Tìm thấy số chẵn: {n}")
        break
else:
    print("Không tìm thấy số chẵn nào!")
# → "Không tìm thấy số chẵn nào!" (vì không break)
```

```python
# Kiểm tra số nguyên tố
num = 17

for i in range(2, num):
    if num % i == 0:
        print(f"{num} không phải số nguyên tố (chia hết cho {i})")
        break
else:
    print(f"{num} là số nguyên tố!")
# → "17 là số nguyên tố!"
```

---

### 1.4 `enumerate()` — lặp kèm chỉ số (chi tiết)

Khi lặp qua một dãy mà cần **cả chỉ số (index) lẫn giá trị**, dùng `enumerate()`.

#### Cú pháp

```python
enumerate(iterable, start=0)
```

- `iterable` — dãy cần duyệt (list, tuple, string, ...)
- `start` — chỉ số bắt đầu (mặc định là `0`)
- Trả về các cặp `(chỉ_số, giá_trị)` dưới dạng **tuple**

---

#### Ví dụ cơ bản

```python
fruits = ["táo", "cam", "chuối"]

for i, fruit in enumerate(fruits):
    print(f"{i}: {fruit}")
# 0: táo
# 1: cam
# 2: chuối
```

---

#### Tại sao dùng `enumerate()` thay vì `range(len(...))`?

```python
fruits = ["táo", "cam", "chuối"]

# ❌ Cách cũ — dùng range(len()) — dài dòng, dễ sai
for i in range(len(fruits)):
    print(f"{i}: {fruits[i]}")

# ✅ Cách tốt hơn — dùng enumerate() — gọn, rõ ràng, Pythonic
for i, fruit in enumerate(fruits):
    print(f"{i}: {fruit}")

# Cả 2 cho kết quả giống nhau, nhưng enumerate():
# - Không cần gọi fruits[i] (tránh lỗi IndexError)
# - Code ngắn hơn, dễ đọc hơn
# - Hoạt động với MỌI iterable (range(len()) chỉ dùng với dãy có index)
```

---

#### `start=` — thay đổi chỉ số bắt đầu

Mặc định chỉ số bắt đầu từ `0`. Dùng `start` để đổi.

```python
fruits = ["táo", "cam", "chuối"]

# Đếm từ 1 (phổ biến khi hiển thị danh sách cho người dùng)
for i, fruit in enumerate(fruits, start=1):
    print(f"{i}. {fruit}")
# 1. táo
# 2. cam
# 3. chuối

# Đếm từ 100
for i, fruit in enumerate(fruits, start=100):
    print(f"ID-{i}: {fruit}")
# ID-100: táo
# ID-101: cam
# ID-102: chuối
```

---

#### Cơ chế hoạt động bên trong

`enumerate()` trả về một **enumerate object** — mỗi phần tử là một **tuple (index, value)**.

```python
fruits = ["táo", "cam", "chuối"]

# Xem enumerate trả về gì
result = list(enumerate(fruits))
print(result)
# [(0, 'táo'), (1, 'cam'), (2, 'chuối')]

# Từng phần tử là tuple
for pair in enumerate(fruits):
    print(pair)
# (0, 'táo')
# (1, 'cam')
# (2, 'chuối')

# Dùng unpacking để tách tuple ra 2 biến
for i, fruit in enumerate(fruits):   # i = index, fruit = value
    print(i, fruit)
```

---

#### Dùng với string (từng ký tự)

```python
word = "Python"

for i, char in enumerate(word):
    print(f"Vị trí {i}: '{char}'")
# Vị trí 0: 'P'
# Vị trí 1: 'y'
# Vị trí 2: 't'
# Vị trí 3: 'h'
# Vị trí 4: 'o'
# Vị trí 5: 'n'

# Ứng dụng: tìm tất cả vị trí của một ký tự
text = "banana"
positions = [i for i, ch in enumerate(text) if ch == "a"]
print(positions)   # [1, 3, 5]
```

---

#### Dùng với tuple

```python
colors = ("đỏ", "xanh", "vàng")

for i, color in enumerate(colors, 1):
    print(f"Màu {i}: {color}")
# Màu 1: đỏ
# Màu 2: xanh
# Màu 3: vàng
```

---

#### Dùng với dict

```python
person = {"name": "An", "age": 25, "city": "HCM"}

# Đánh số thứ tự các key
for i, key in enumerate(person, 1):
    print(f"{i}. {key} = {person[key]}")
# 1. name = An
# 2. age = 25
# 3. city = HCM

# Kết hợp với .items()
for i, (key, val) in enumerate(person.items(), 1):
    print(f"{i}. {key}: {val}")
# 1. name: An
# 2. age: 25
# 3. city: HCM
```

---

#### Kết hợp `enumerate()` + `zip()`

```python
names  = ["An", "Bình", "Cường"]
scores = [90, 85, 78]

for i, (name, score) in enumerate(zip(names, scores), 1):
    print(f"{i}. {name}: {score} điểm")
# 1. An: 90 điểm
# 2. Bình: 85 điểm
# 3. Cường: 78 điểm
```

---

#### Dùng trong List Comprehension

```python
fruits = ["táo", "cam", "chuối"]

# Tạo list chuỗi có đánh số
numbered = [f"{i}. {f}" for i, f in enumerate(fruits, 1)]
print(numbered)   # ['1. táo', '2. cam', '3. chuối']

# Tìm vị trí phần tử thỏa điều kiện
nums = [10, 25, 30, 45, 50]
big_positions = [i for i, n in enumerate(nums) if n >= 30]
print(big_positions)   # [2, 3, 4]
```

---

#### Dùng trong Dict Comprehension

```python
fruits = ["táo", "cam", "chuối"]

# Tạo dict {index: value}
fruit_dict = {i: f for i, f in enumerate(fruits)}
print(fruit_dict)   # {0: 'táo', 1: 'cam', 2: 'chuối'}

# Tạo dict {value: index} — tra ngược
index_map = {f: i for i, f in enumerate(fruits)}
print(index_map)    # {'táo': 0, 'cam': 1, 'chuối': 2}
print(index_map["cam"])  # 1
```

---

#### Ứng dụng thực tế

```python
# 1. Đánh số dòng khi đọc file
with open("data.txt") as f:
    for line_num, line in enumerate(f, 1):
        print(f"Dòng {line_num}: {line.strip()}")

# 2. Tìm phần tử lớn nhất và vị trí của nó
nums = [10, 45, 30, 80, 25]
max_val = max(nums)
max_idx = nums.index(max_val)
print(f"Lớn nhất: {max_val} ở vị trí {max_idx}")

# Hoặc dùng enumerate:
max_idx, max_val = max(enumerate(nums), key=lambda x: x[1])
print(f"Lớn nhất: {max_val} ở vị trí {max_idx}")

# 3. Tạo bảng xếp hạng
scores = [("An", 90), ("Bình", 95), ("Cường", 85)]
scores.sort(key=lambda x: x[1], reverse=True)   # sắp giảm dần

for rank, (name, score) in enumerate(scores, 1):
    medal = "🥇🥈🥉"[rank-1] if rank <= 3 else " "
    print(f"{medal} Hạng {rank}: {name} — {score} điểm")
# 🥇 Hạng 1: Bình — 95 điểm
# 🥈 Hạng 2: An — 90 điểm
# 🥉 Hạng 3: Cường — 85 điểm
```

---

#### ⚠️ Lỗi thường gặp

```python
fruits = ["táo", "cam", "chuối"]

# ❌ Quên unpacking — biến nhận tuple, không phải 2 giá trị riêng
for x in enumerate(fruits):
    print(x)           # (0, 'táo') ← x là tuple, không phải chỉ số!

# ✅ Dùng unpacking
for i, fruit in enumerate(fruits):
    print(i, fruit)    # 0 táo

# ❌ Nhầm thứ tự biến — chỉ số luôn đứng TRƯỚC
for fruit, i in enumerate(fruits):  # fruit=0, i='táo' ← NGƯỢC!
    print(i, fruit)

# ✅ Thứ tự đúng: (index, value)
for i, fruit in enumerate(fruits):
    print(i, fruit)
```

---

### 1.5 `zip()` — lặp song song nhiều dãy

Duyệt **nhiều dãy cùng lúc**, dừng ở dãy ngắn nhất.

```python
names  = ["An", "Bình", "Cường"]
scores = [90, 85, 78]
grades = ["A", "B+", "B"]

for name, score, grade in zip(names, scores, grades):
    print(f"{name}: {score} điểm ({grade})")
# An: 90 điểm (A)
# Bình: 85 điểm (B+)
# Cường: 78 điểm (B)

# Kết hợp enumerate + zip
for i, (name, score) in enumerate(zip(names, scores), 1):
    print(f"{i}. {name} - {score}")
```

#### Tạo dict từ 2 list bằng zip
```python
keys   = ["name", "age", "city"]
values = ["An", 25, "HCM"]

result = dict(zip(keys, values))
print(result)   # {'name': 'An', 'age': 25, 'city': 'HCM'}
```

---

## 2. Vòng Lặp `while`

Lặp **khi điều kiện còn đúng** (`True`). Dùng khi không biết trước số lần lặp.

### 2.1 Cú pháp cơ bản

```python
while điều_kiện:
    # code thực thi khi điều kiện = True
```

```python
# Đếm từ 1 đến 5
count = 1

while count <= 5:
    print(count, end=" ")
    count += 1              # ⚠️ PHẢI thay đổi biến, nếu không → lặp vô hạn!
# 1 2 3 4 5
```

#### Ví dụ: tính tổng 1 + 2 + ... + 100
```python
total = 0
n = 1

while n <= 100:
    total += n
    n += 1

print(f"Tổng = {total}")   # Tổng = 5050
```

---

### 2.2 `while True` — lặp vô hạn (có kiểm soát)

Rất phổ biến trong menu, game, hoặc chờ input.

```python
while True:
    answer = input("Nhập 'quit' để thoát: ")
    if answer == "quit":
        print("Tạm biệt!")
        break               # ← thoát vòng lặp
    print(f"Bạn đã nhập: {answer}")
```

#### Ví dụ: menu đơn giản
```python
while True:
    print("\n=== MENU ===")
    print("1. Xin chào")
    print("2. Tính tuổi")
    print("0. Thoát")

    choice = input("Chọn: ")

    if choice == "1":
        name = input("Tên bạn: ")
        print(f"Xin chào {name}!")
    elif choice == "2":
        year = int(input("Năm sinh: "))
        print(f"Bạn khoảng {2025 - year} tuổi")
    elif choice == "0":
        print("Bye!")
        break
    else:
        print("Lựa chọn không hợp lệ!")
```

---

### 2.3 `while...else`

Giống `for...else`, khối `else` chạy khi vòng lặp kết thúc **bình thường** (không bị `break`).

```python
# Tìm số chia hết cho 7 trong khoảng
n = 10

while n < 20:
    if n % 7 == 0:
        print(f"Tìm thấy: {n}")
        break
    n += 1
else:
    print("Không tìm thấy!")
# → "Tìm thấy: 14"
```

---

## 3. Điều Khiển Vòng Lặp

### 3.1 `break` — thoát vòng lặp ngay lập tức

Dừng hoàn toàn vòng lặp, nhảy ra ngoài.

```python
# Tìm phần tử đầu tiên > 50
nums = [10, 25, 60, 35, 80]

for n in nums:
    if n > 50:
        print(f"Tìm thấy: {n}")
        break
    print(f"Kiểm tra: {n}")
# Kiểm tra: 10
# Kiểm tra: 25
# Tìm thấy: 60           ← dừng luôn, không kiểm tra 35, 80
```

---

### 3.2 `continue` — bỏ qua lần lặp hiện tại

Nhảy đến lần lặp tiếp theo, bỏ qua code phía dưới.

```python
# In số lẻ, bỏ qua số chẵn
for i in range(1, 11):
    if i % 2 == 0:
        continue             # bỏ qua, nhảy đến i tiếp theo
    print(i, end=" ")
# 1 3 5 7 9
```

```python
# Bỏ qua chuỗi rỗng
names = ["An", "", "Bình", "", "Cường"]

for name in names:
    if not name:             # chuỗi rỗng "" = falsy
        continue
    print(f"Xin chào {name}!")
# Xin chào An!
# Xin chào Bình!
# Xin chào Cường!
```

---

### 3.3 `pass` — không làm gì cả

Dùng làm **chỗ giữ chỗ** (placeholder) khi chưa viết code.

```python
# Chưa biết xử lý gì, để tạm
for i in range(10):
    pass                     # không làm gì, tránh lỗi cú pháp

# Thường dùng khi lên khung code
for item in data:
    if item > 100:
        pass                 # TODO: xử lý sau
    else:
        print(item)
```

---

### 3.4 So sánh `break` vs `continue` vs `pass`

```python
for i in range(1, 6):
    if i == 3:
        break           # DỪNG hoàn toàn
    print(i)
# 1 2

for i in range(1, 6):
    if i == 3:
        continue        # BỎ QUA i=3, lặp tiếp
    print(i)
# 1 2 4 5

for i in range(1, 6):
    if i == 3:
        pass            # KHÔNG LÀM GÌ, chạy tiếp print bên dưới
    print(i)
# 1 2 3 4 5
```

---

## 4. Vòng Lặp Lồng Nhau (Nested Loops)

Vòng lặp bên trong sẽ chạy **hết** mỗi khi vòng lặp bên ngoài lặp 1 lần.

### 4.1 Ví dụ cơ bản
```python
for i in range(1, 4):          # vòng ngoài: 1, 2, 3
    for j in range(1, 4):      # vòng trong: 1, 2, 3
        print(f"({i},{j})", end=" ")
    print()                    # xuống dòng

# (1,1) (1,2) (1,3)
# (2,1) (2,2) (2,3)
# (3,1) (3,2) (3,3)
```

### 4.2 Bảng cửu chương
```python
for i in range(2, 10):
    print(f"\n--- Bảng {i} ---")
    for j in range(1, 11):
        print(f"{i} x {j} = {i*j}")
```

### 4.3 In hình tam giác sao
```python
# Tam giác vuông
n = 5
for i in range(1, n + 1):
    print("*" * i)
# *
# **
# ***
# ****
# *****

# Tam giác cân
for i in range(1, n + 1):
    spaces = " " * (n - i)
    stars  = "*" * (2 * i - 1)
    print(spaces + stars)
#     *
#    ***
#   *****
#  *******
# *********
```

### 4.4 Duyệt ma trận (list lồng)
```python
matrix = [
    [1, 2, 3],
    [4, 5, 6],
    [7, 8, 9]
]

for i in range(len(matrix)):
    for j in range(len(matrix[i])):
        print(f"{matrix[i][j]:3}", end="")
    print()
#   1  2  3
#   4  5  6
#   7  8  9
```

### 4.5 `break` trong vòng lặp lồng — chỉ thoát vòng trong
```python
for i in range(1, 4):
    for j in range(1, 4):
        if j == 2:
            break            # chỉ thoát vòng trong (j)
        print(f"({i},{j})", end=" ")
    print()

# (1,1)
# (2,1)
# (3,1)
# → mỗi lần j=2 thì break vòng trong, nhưng vòng ngoài vẫn tiếp tục
```

---

## 5. Comprehension — Vòng Lặp Viết Gọn

Cách viết **1 dòng** thay cho vòng lặp tạo list/dict/set.

### 5.1 List Comprehension

```python
# Cú pháp: [biểu_thức for biến in dãy if điều_kiện]

# Cách thường
squares = []
for x in range(1, 6):
    squares.append(x ** 2)

# ⭐ List comprehension — gọn hơn
squares = [x ** 2 for x in range(1, 6)]       # [1, 4, 9, 16, 25]

# Có điều kiện lọc
evens = [x for x in range(20) if x % 2 == 0]  # [0, 2, 4, 6, ..., 18]

# Biến đổi + lọc
words = ["Hello", "", "World", "", "Python"]
upper = [w.upper() for w in words if w]        # ['HELLO', 'WORLD', 'PYTHON']
```

#### if...else trong comprehension
```python
# if...else phải đặt TRƯỚC for
labels = ["chẵn" if x % 2 == 0 else "lẻ" for x in range(1, 6)]
print(labels)   # ['lẻ', 'chẵn', 'lẻ', 'chẵn', 'lẻ']
```

#### Comprehension lồng (Nested)
```python
# Tạo ma trận phẳng
matrix = [[1,2,3], [4,5,6], [7,8,9]]
flat = [num for row in matrix for num in row]
print(flat)     # [1, 2, 3, 4, 5, 6, 7, 8, 9]

# Tương đương:
# for row in matrix:
#     for num in row:
#         flat.append(num)
```

---

### 5.2 Dict Comprehension

```python
# {key: value for biến in dãy if điều_kiện}

squares = {x: x**2 for x in range(1, 6)}
print(squares)   # {1: 1, 2: 4, 3: 9, 4: 16, 5: 25}

# Lọc dict
scores = {"An": 90, "Bình": 55, "Cường": 80}
passed = {k: v for k, v in scores.items() if v >= 60}
print(passed)    # {'An': 90, 'Cường': 80}
```

---

### 5.3 Set Comprehension

```python
# {biểu_thức for biến in dãy if điều_kiện}

nums = [1, 2, 2, 3, 3, 4]
unique_squares = {x**2 for x in nums}
print(unique_squares)   # {1, 4, 9, 16}
```

---

### 5.4 Generator Expression (tiết kiệm bộ nhớ)

Giống list comprehension nhưng dùng `()` — **không tạo list trong bộ nhớ**, duyệt từng phần tử.

```python
# List comprehension — tạo list đầy đủ trong bộ nhớ
sum_list = sum([x**2 for x in range(1000000)])

# ⭐ Generator — tiết kiệm bộ nhớ hơn nhiều
sum_gen = sum(x**2 for x in range(1000000))

# Generator chỉ dùng 1 lần
gen = (x**2 for x in range(5))
print(next(gen))    # 0
print(next(gen))    # 1
print(next(gen))    # 4
# ... hoặc dùng trong for:
for val in gen:
    print(val)      # 9, 16 (tiếp tục từ chỗ dừng)
```

---

## 6. Các Kỹ Thuật Nâng Cao

### 6.1 `reversed()` — lặp ngược
```python
fruits = ["táo", "cam", "chuối"]

for fruit in reversed(fruits):
    print(fruit)
# chuối
# cam
# táo

# Lặp ngược range
for i in reversed(range(5)):
    print(i, end=" ")
# 4 3 2 1 0

# Hoặc dùng range bước âm
for i in range(4, -1, -1):
    print(i, end=" ")
# 4 3 2 1 0
```

---

### 6.2 `sorted()` — lặp theo thứ tự sắp xếp
```python
nums = [3, 1, 4, 1, 5, 9]

# Tăng dần
for n in sorted(nums):
    print(n, end=" ")
# 1 1 3 4 5 9

# Giảm dần
for n in sorted(nums, reverse=True):
    print(n, end=" ")
# 9 5 4 3 1 1

# Sắp xếp theo tiêu chí tùy chỉnh (key)
words = ["chuối", "táo", "cam"]
for w in sorted(words, key=len):    # theo độ dài
    print(w, end=" ")
# táo cam chuối
```

---

### 6.3 `map()` và `filter()` — xử lý dãy
```python
nums = [1, 2, 3, 4, 5]

# map() — áp dụng hàm cho mỗi phần tử
squares = list(map(lambda x: x**2, nums))
print(squares)    # [1, 4, 9, 16, 25]

# filter() — lọc phần tử thỏa điều kiện
evens = list(filter(lambda x: x % 2 == 0, nums))
print(evens)      # [2, 4]

# Kết hợp: lấy bình phương của số chẵn
result = list(map(lambda x: x**2, filter(lambda x: x % 2 == 0, nums)))
print(result)     # [4, 16]

# ⭐ Comprehension thường dễ đọc hơn:
result = [x**2 for x in nums if x % 2 == 0]   # [4, 16]
```

---

### 6.4 `itertools` — thư viện lặp mạnh mẽ
```python
import itertools

# count() — đếm vô hạn
for i in itertools.count(10, 2):   # 10, 12, 14, 16, ...
    if i > 16:
        break
    print(i, end=" ")
# 10 12 14 16

# cycle() — lặp vòng tròn
colors = itertools.cycle(["đỏ", "xanh", "vàng"])
for i in range(6):
    print(next(colors), end=" ")
# đỏ xanh vàng đỏ xanh vàng

# repeat() — lặp lại giá trị
for x in itertools.repeat("Hello", 3):
    print(x)
# Hello Hello Hello

# chain() — nối nhiều dãy
a = [1, 2]
b = [3, 4]
c = [5, 6]
for x in itertools.chain(a, b, c):
    print(x, end=" ")
# 1 2 3 4 5 6

# product() — tích Descartes (mọi tổ hợp)
for x, y in itertools.product("AB", "12"):
    print(f"{x}{y}", end=" ")
# A1 A2 B1 B2

# combinations() — tổ hợp (không lặp)
for combo in itertools.combinations([1,2,3,4], 2):
    print(combo, end=" ")
# (1,2) (1,3) (1,4) (2,3) (2,4) (3,4)

# permutations() — hoán vị
for perm in itertools.permutations("ABC", 2):
    print("".join(perm), end=" ")
# AB AC BA BC CA CB
```

---

## 7. Bài Tập Thực Hành

### Bài 1: Tính giai thừa
```python
n = 5
factorial = 1

for i in range(1, n + 1):
    factorial *= i

print(f"{n}! = {factorial}")   # 5! = 120
```

### Bài 2: Dãy Fibonacci
```python
n = 10
a, b = 0, 1

for _ in range(n):
    print(a, end=" ")
    a, b = b, a + b
# 0 1 1 2 3 5 8 13 21 34
```

### Bài 3: Đoán số
```python
import random

secret = random.randint(1, 100)
attempts = 0

while True:
    guess = int(input("Đoán số (1-100): "))
    attempts += 1

    if guess < secret:
        print("Lớn hơn!")
    elif guess > secret:
        print("Nhỏ hơn!")
    else:
        print(f"🎉 Đúng rồi! Bạn đoán {attempts} lần.")
        break
```

### Bài 4: Kiểm tra mật khẩu
```python
MAX_ATTEMPTS = 3
password = "python123"

for attempt in range(1, MAX_ATTEMPTS + 1):
    user_input = input(f"Lần {attempt}/{MAX_ATTEMPTS} - Nhập mật khẩu: ")

    if user_input == password:
        print("✅ Đăng nhập thành công!")
        break
    else:
        remaining = MAX_ATTEMPTS - attempt
        if remaining > 0:
            print(f"❌ Sai! Còn {remaining} lần thử.")
else:
    print("🔒 Tài khoản bị khóa!")
```

### Bài 5: Tìm số nguyên tố trong khoảng
```python
start, end = 1, 50
primes = []

for num in range(2, end + 1):
    is_prime = True
    for i in range(2, int(num**0.5) + 1):
        if num % i == 0:
            is_prime = False
            break
    if is_prime and num >= start:
        primes.append(num)

print(f"Số nguyên tố từ {start} đến {end}:")
print(primes)
# [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47]
```

---

## 8. Bảng Tổng Hợp

| Cú pháp | Mô tả | Ví dụ |
|---------|--------|-------|
| `for x in list` | Duyệt từng phần tử | `for x in [1,2,3]` |
| `for i in range(n)` | Lặp n lần (0→n-1) | `for i in range(5)` |
| `for i, x in enumerate(list)` | Duyệt kèm chỉ số | `for i, x in enumerate(fruits)` |
| `for a, b in zip(X, Y)` | Duyệt song song | `for a, b in zip(names, scores)` |
| `while điều_kiện` | Lặp khi đúng | `while count < 10` |
| `while True` | Lặp vô hạn | Kết hợp `break` để thoát |
| `break` | Thoát vòng lặp | Dừng hoàn toàn |
| `continue` | Bỏ qua lần lặp | Nhảy đến lần tiếp |
| `pass` | Không làm gì | Giữ chỗ (placeholder) |
| `for...else` | else khi không break | Kiểm tra tìm kiếm |

---

> 💡 **Ghi nhớ nhanh:**
> - Biết trước số lần lặp → **`for` + `range()`**
> - Không biết số lần, lặp theo điều kiện → **`while`**
> - Cần chỉ số kèm giá trị → **`enumerate()`**
> - Cần duyệt nhiều dãy cùng lúc → **`zip()`**
> - Tạo list/dict/set nhanh → **comprehension**
> - ⚠️ Cẩn thận `while` lặp vô hạn — luôn đảm bảo điều kiện sẽ thành `False`!
