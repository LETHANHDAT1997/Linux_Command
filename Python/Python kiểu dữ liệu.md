# 🐍 Tài Liệu Học Python

---

## 1. Kiểu Dữ Liệu Trong Python (Data Types)

Python là ngôn ngữ **kiểu động** (dynamically typed) — bạn không cần khai báo kiểu biến, Python tự suy ra khi chạy.

Dùng hàm `type()` để kiểm tra kiểu dữ liệu, `isinstance()` để kiểm tra kiểu cụ thể.

---

### 1.1 Kiểu Số (Numeric Types)

#### `int` — Số nguyên
```python
x = 10
y = -5
z = 0
big = 999_999_999   # dùng _ để dễ đọc số lớn
hex_num = 0xFF      # 255 (hệ 16)
bin_num = 0b1010    # 10  (hệ 2)
oct_num = 0o17      # 15  (hệ 8)

print(type(x))      # <class 'int'>
```

#### `float` — Số thực (dấu phẩy động)
```python
pi = 3.14
e  = 2.718
science = 1.5e3     # 1500.0 (ký hiệu khoa học)
tiny    = 1.5e-4    # 0.00015

print(type(pi))     # <class 'float'>
```

#### `complex` — Số phức
```python
c = 3 + 4j

print(type(c))      # <class 'complex'>
print(c.real)       # 3.0   (phần thực)
print(c.imag)       # 4.0   (phần ảo)
print(abs(c))       # 5.0   (module = √(3²+4²))
```

#### Toán tử số học
```python
a = 15
b = 4

print(a + b)    # 19      Cộng
print(a - b)    # 11      Trừ
print(a * b)    # 60      Nhân
print(a / b)    # 3.75    Chia (luôn trả về float)
print(a // b)   # 3       Chia lấy phần nguyên
print(a % b)    # 3       Chia lấy phần dư
print(a ** b)   # 50625   Lũy thừa (15⁴)
```

#### Các hàm tính toán hữu ích
```python
# Hàm có sẵn (built-in)
print(abs(-10))         # 10     (giá trị tuyệt đối)
print(round(3.7))       # 4      (làm tròn)
print(round(3.14159, 2))# 3.14   (làm tròn 2 chữ số)
print(max(1, 5, 3))     # 5      (giá trị lớn nhất)
print(min(1, 5, 3))     # 1      (giá trị nhỏ nhất)
print(pow(2, 10))       # 1024   (lũy thừa)
print(divmod(17, 5))    # (3, 2) (thương và dư)

# Thư viện math — cần import
import math
print(math.sqrt(16))    # 4.0    (căn bậc 2)
print(math.ceil(3.2))   # 4      (làm tròn lên)
print(math.floor(3.9))  # 3      (làm tròn xuống)
print(math.pi)          # 3.141592653589793
print(math.log(100, 10))# 2.0    (log cơ số 10)
print(math.factorial(5))# 120    (giai thừa 5!)
```

---

### 1.2 Kiểu Chuỗi (String — `str`)

Chuỗi ký tự, **không thể thay đổi** (immutable), dùng `'...'` hoặc `"..."` hoặc `"""..."""`.

```python
name    = "Python"
message = 'Xin chào!'
multi   = """Đây là
chuỗi nhiều dòng"""
raw     = r"C:\Users\name"   # raw string — không xử lý \n, \t
```

#### Truy cập & Slicing (cắt chuỗi)
```python
s = "Hello Python"

# Chỉ số (index) — bắt đầu từ 0
print(s[0])         # H     (ký tự đầu)
print(s[-1])        # n     (ký tự cuối)

# Slicing: s[start:end:step]
print(s[0:5])       # Hello  (từ 0 đến 4, không bao gồm 5)
print(s[6:])        # Python (từ 6 đến hết)
print(s[:5])        # Hello  (từ đầu đến 4)
print(s[::2])       # HloPto (lấy cách 1 ký tự)
print(s[::-1])      # nohtyP olleH (đảo ngược chuỗi)
```

#### Các method thường dùng
```python
text = "  Hello, World!  "

# --- Tìm kiếm ---
print(text.find("World"))        # 9   (vị trí tìm thấy, -1 nếu không có)
print(text.index("World"))       # 9   (giống find, nhưng lỗi nếu không có)
print(text.count("l"))           # 3   (đếm số lần xuất hiện)
print("Hello" in text)           # True (kiểm tra có chứa không)
print(text.startswith("  Hello"))# True
print(text.endswith("!  "))      # True

# --- Biến đổi ---
print(text.upper())              # "  HELLO, WORLD!  "
print(text.lower())              # "  hello, world!  "
print(text.title())              # "  Hello, World!  "
print(text.capitalize())         # "  hello, world!  "
print(text.swapcase())           # "  hELLO, wORLD!  "

# --- Xóa khoảng trắng ---
print(text.strip())              # "Hello, World!"   (hai bên)
print(text.lstrip())             # "Hello, World!  " (bên trái)
print(text.rstrip())             # "  Hello, World!" (bên phải)

# --- Thay thế & Tách/Nối ---
print(text.replace("World", "Python"))  # "  Hello, Python!  "

csv = "a,b,c,d"
print(csv.split(","))            # ['a', 'b', 'c', 'd']

words = ["Xin", "chào", "bạn"]
print(" ".join(words))           # "Xin chào bạn"
print("-".join(words))           # "Xin-chào-bạn"

# --- Kiểm tra ---
print("123".isdigit())           # True  (toàn số)
print("abc".isalpha())           # True  (toàn chữ)
print("abc123".isalnum())        # True  (chữ hoặc số)
print("   ".isspace())           # True  (toàn khoảng trắng)

# --- Căn chỉnh ---
print("Hi".center(10, "-"))      # "----Hi----"
print("Hi".ljust(10, "."))       # "Hi........"
print("Hi".rjust(10, "."))       # "........Hi"
print("42".zfill(5))             # "00042"
```

#### Định dạng chuỗi (String Formatting)
```python
name = "An"
age  = 25
pi   = 3.14159

# Cách 1: f-string (Python 3.6+) ⭐ khuyên dùng
print(f"Tên: {name}, Tuổi: {age}")
print(f"Pi = {pi:.2f}")          # "Pi = 3.14" (2 chữ thập phân)
print(f"{'Hi':>10}")             # "        Hi" (căn phải 10 ký tự)
print(f"{1000000:,}")            # "1,000,000" (thêm dấu phẩy)

# Cách 2: .format()
print("Tên: {}, Tuổi: {}".format(name, age))
print("Tên: {0}, Tuổi: {1}".format(name, age))

# Cách 3: % (kiểu cũ)
print("Tên: %s, Tuổi: %d" % (name, age))
```

---

### 1.3 Kiểu Boolean (`bool`)

Chỉ có 2 giá trị: `True` hoặc `False` (viết hoa chữ cái đầu).

```python
a = True
b = False

print(type(a))      # <class 'bool'>
print(int(True))    # 1
print(int(False))   # 0
```

#### Toán tử logic
```python
print(True and False)    # False  (VÀ — cả hai True mới True)
print(True or False)     # True   (HOẶC — một cái True là đủ)
print(not True)          # False  (PHỦ ĐỊNH)

# Kết hợp toán tử so sánh
x = 15
print(x > 10 and x < 20)    # True
print(10 < x < 20)          # True  (viết gọn hơn)
```

#### Toán tử so sánh (trả về bool)
```python
print(5 == 5)     # True   (bằng)
print(5 != 3)     # True   (khác)
print(5 > 3)      # True   (lớn hơn)
print(5 < 3)      # False  (nhỏ hơn)
print(5 >= 5)     # True   (lớn hơn hoặc bằng)
print(5 <= 3)     # False  (nhỏ hơn hoặc bằng)
```

#### Giá trị Truthy / Falsy
```python
# Các giá trị coi như False (falsy):
# False, 0, 0.0, "", [], {}, (), set(), None

# Mọi giá trị khác đều coi như True (truthy)

print(bool(0))       # False
print(bool(""))      # False
print(bool([]))      # False
print(bool(None))    # False

print(bool(1))       # True
print(bool("hi"))    # True
print(bool([1,2]))   # True

# Ứng dụng thực tế:
name = ""
if name:
    print(f"Xin chào {name}")
else:
    print("Tên trống!")    # ← In ra dòng này
```

---

### 1.4 Kiểu Danh Sách (`list`)

Dãy có thứ tự, **có thể thay đổi** (mutable), cho phép phần tử trùng lặp.

```python
fruits = ["táo", "cam", "chuối"]
mixed  = [1, "hello", 3.14, True]
empty  = []                  # list rỗng
nums   = list(range(1, 6))   # [1, 2, 3, 4, 5]
```

#### Truy cập & Slicing
```python
colors = ["đỏ", "cam", "vàng", "xanh", "tím"]

print(colors[0])        # đỏ      (phần tử đầu)
print(colors[-1])       # tím     (phần tử cuối)
print(colors[1:3])      # ['cam', 'vàng']
print(colors[:3])       # ['đỏ', 'cam', 'vàng']
print(colors[::2])      # ['đỏ', 'vàng', 'tím']  (cách 1)
print(colors[::-1])     # ['tím', 'xanh', 'vàng', 'cam', 'đỏ']  (đảo ngược)
```

#### Thêm phần tử
```python
fruits = ["táo", "cam"]

fruits.append("chuối")           # thêm 1 phần tử vào cuối → ['táo', 'cam', 'chuối']
fruits.insert(1, "xoài")         # chèn vào vị trí 1  → ['táo', 'xoài', 'cam', 'chuối']
fruits.extend(["nho", "ổi"])     # nối thêm list khác  → [..., 'nho', 'ổi']
fruits += ["dưa"]                # tương tự extend     → [..., 'dưa']
```

#### Xóa phần tử
```python
fruits = ["táo", "cam", "chuối", "cam"]

fruits.remove("cam")     # xóa phần tử "cam" ĐẦU TIÊN tìm thấy
item = fruits.pop()      # xóa phần tử CUỐI, trả về nó   → item = "cam"
item = fruits.pop(0)     # xóa phần tử ở vị trí 0        → item = "táo"
del fruits[0]            # xóa phần tử ở vị trí 0 (không trả về)
fruits.clear()           # xóa TẤT CẢ phần tử            → []
```

#### Tìm kiếm & Đếm
```python
nums = [3, 1, 4, 1, 5, 9, 2, 6]

print(1 in nums)             # True  (kiểm tra có phần tử không)
print(nums.index(4))         # 2     (vị trí đầu tiên tìm thấy)
print(nums.count(1))         # 2     (đếm số lần xuất hiện)
```

#### Sắp xếp & Đảo
```python
nums = [3, 1, 4, 1, 5]

nums.sort()                  # sắp xếp tăng dần tại chỗ  → [1, 1, 3, 4, 5]
nums.sort(reverse=True)      # sắp xếp giảm dần          → [5, 4, 3, 1, 1]
nums.reverse()               # đảo ngược thứ tự           → [1, 1, 3, 4, 5]

# sorted() tạo list mới, KHÔNG thay đổi list gốc
original = [3, 1, 4]
new_list = sorted(original)  # new_list = [1, 3, 4], original vẫn = [3, 1, 4]
```

#### Sao chép list — Cơ chế tham chiếu (reference)

Trong Python, **biến không chứa dữ liệu trực tiếp**, mà chứa **địa chỉ** (tham chiếu)
trỏ đến dữ liệu trong bộ nhớ. Dùng hàm `id()` để xem địa chỉ.

```python
# === b = a → SAO CHÉP ĐỊA CHỈ, không sao chép dữ liệu ===

a = [1, 2, 3]
b = a                # b nhận cùng địa chỉ với a

#  Sơ đồ bộ nhớ:
#  ┌───┐        ┌───────────┐
#  │ a │───────→│ [1, 2, 3] │  ← chỉ có MỘT list trong bộ nhớ
#  └───┘   ┌──→│           │
#  ┌───┐   │   └───────────┘
#  │ b │───┘
#  └───┘
#  → a và b cùng trỏ đến MỘT list

print(id(a))         # 2305678  (ví dụ)
print(id(b))         # 2305678  ← CÙNG địa chỉ!
print(a is b)        # True     ← cùng một đối tượng

b.append(4)
print(a)             # [1, 2, 3, 4] ← a CŨNG thay đổi vì là cùng list!
print(b)             # [1, 2, 3, 4]
```

```python
# === .copy() → TẠO LIST MỚI, sao chép giá trị sang ===

a = [1, 2, 3]
c = a.copy()         # tạo list mới, copy dữ liệu

#  Sơ đồ bộ nhớ:
#  ┌───┐        ┌───────────┐
#  │ a │───────→│ [1, 2, 3] │  ← list gốc (ô nhớ #1234)
#  └───┘        └───────────┘
#  ┌───┐        ┌───────────┐
#  │ c │───────→│ [1, 2, 3] │  ← list MỚI (ô nhớ #5678)
#  └───┘        └───────────┘
#  → a và c trỏ đến 2 list KHÁC nhau

print(id(a))         # 2305678
print(id(c))         # 9876543  ← KHÁC địa chỉ!
print(a is c)        # False    ← khác đối tượng
print(a == c)        # True     ← nhưng giá trị giống nhau

c.append(4)
print(c)             # [1, 2, 3, 4]
print(a)             # [1, 2, 3]   ← a KHÔNG bị ảnh hưởng!
```

```python
# === Các cách sao chép list ===

a = [1, 2, 3]

c1 = a.copy()        # cách 1: method .copy()
c2 = a[:]            # cách 2: slicing toàn bộ
c3 = list(a)         # cách 3: hàm list()

# Cả 3 cách đều tạo bản sao MỚI, kết quả giống nhau
```

```python
# === ⚠️ CHÚ Ý: .copy() chỉ sao chép "nông" (shallow copy) ===
# Nếu list chứa list con, list con vẫn là THAM CHIẾU chung!

a = [[1, 2], [3, 4]]
b = a.copy()         # shallow copy

b[0].append(99)      # sửa list con
print(a)             # [[1, 2, 99], [3, 4]]  ← a CŨNG bị ảnh hưởng!

# Dùng deepcopy để sao chép SÂU — copy hoàn toàn tất cả các tầng
import copy
a = [[1, 2], [3, 4]]
c = copy.deepcopy(a) # deep copy

c[0].append(99)
print(a)             # [[1, 2], [3, 4]]      ← a KHÔNG bị ảnh hưởng
print(c)             # [[1, 2, 99], [3, 4]]
```

> 📌 **Tóm tắt:**
> - `b = a` → **cùng trỏ** đến 1 đối tượng → sửa b thì a cũng đổi
> - `b = a.copy()` → **sao chép nông** → list con vẫn dùng chung
> - `b = copy.deepcopy(a)` → **sao chép sâu** → hoàn toàn độc lập
> - Quy tắc này áp dụng cho **tất cả** kiểu có thể thay đổi: `list`, `dict`, `set`

#### List Comprehension (tạo list nhanh)
```python
# Cú pháp: [biểu_thức for biến in dãy if điều_kiện]

squares = [x**2 for x in range(1, 6)]        # [1, 4, 9, 16, 25]
evens   = [x for x in range(10) if x % 2 == 0]  # [0, 2, 4, 6, 8]
upper   = [s.upper() for s in ["hi", "bye"]]  # ['HI', 'BYE']
```

#### List lồng nhau (Nested List)
```python
matrix = [
    [1, 2, 3],
    [4, 5, 6],
    [7, 8, 9]
]

print(matrix[0])        # [1, 2, 3]  (hàng đầu)
print(matrix[1][2])     # 6          (hàng 1, cột 2)

# Duyệt ma trận
for row in matrix:
    for val in row:
        print(val, end=" ")
    print()   # xuống dòng
```

#### Các hàm hữu ích với list
```python
nums = [3, 1, 4, 1, 5]

print(len(nums))     # 5     (số phần tử)
print(sum(nums))     # 14    (tổng)
print(min(nums))     # 1     (nhỏ nhất)
print(max(nums))     # 5     (lớn nhất)

# Duyệt kèm chỉ số bằng enumerate()
fruits = ["táo", "cam", "chuối"]
for i, fruit in enumerate(fruits):
    print(f"{i}: {fruit}")
# 0: táo
# 1: cam
# 2: chuối

# Ghép 2 list bằng zip()
names  = ["An", "Bình"]
scores = [90, 85]
for name, score in zip(names, scores):
    print(f"{name}: {score} điểm")
```

---

### 1.5 Kiểu Tuple (`tuple`)

Dãy có thứ tự, **không thể thay đổi** (immutable), cho phép phần tử trùng lặp.

Dùng khi dữ liệu không nên bị sửa đổi (tọa độ, RGB, cấu hình cố định, v.v.).

```python
point  = (10, 20)
colors = ("đỏ", "xanh", "vàng")
single = (42,)           # ⚠️ tuple 1 phần tử — BẮT BUỘC có dấu phẩy
not_tuple = (42)         # ← đây chỉ là số 42, KHÔNG phải tuple!
empty  = ()              # tuple rỗng
from_list = tuple([1, 2, 3])  # tạo tuple từ list
```

#### Truy cập & Slicing (giống list)
```python
t = ("a", "b", "c", "d", "e")

print(t[0])         # a
print(t[-1])        # e
print(t[1:4])       # ('b', 'c', 'd')
print(t[::-1])      # ('e', 'd', 'c', 'b', 'a')
```

#### Unpacking (giải nén)
```python
point = (10, 20, 30)
x, y, z = point
print(x, y, z)      # 10 20 30

# Dùng * để gom phần còn lại
first, *rest = (1, 2, 3, 4, 5)
print(first)    # 1
print(rest)     # [2, 3, 4, 5]  ← chú ý: rest là list

# Hoán đổi 2 biến (swap) — Python đặc biệt!
a, b = 1, 2
a, b = b, a
print(a, b)     # 2 1
```

#### Các method & thao tác
```python
t = (1, 2, 3, 2, 1)

print(t.count(2))    # 2   (đếm số lần xuất hiện)
print(t.index(3))    # 2   (vị trí đầu tiên tìm thấy)
print(len(t))        # 5
print(2 in t)        # True
print(min(t))        # 1
print(max(t))        # 3
print(sum(t))        # 9

# Nối 2 tuple
t1 = (1, 2)
t2 = (3, 4)
print(t1 + t2)       # (1, 2, 3, 4)
print(t1 * 3)        # (1, 2, 1, 2, 1, 2)
```

#### Tuple vs List — khi nào dùng cái nào?
```python
# Tuple nhanh hơn list, tiết kiệm bộ nhớ hơn
# Tuple có thể dùng làm key của dict (vì không thể thay đổi)

locations = {
    (10.5, 106.7): "TP.HCM",
    (21.0, 105.8): "Hà Nội"
}
print(locations[(10.5, 106.7)])  # TP.HCM

# Hàm trả về nhiều giá trị → Python tự đóng gói thành tuple
def get_name_age():
    return "An", 25

result = get_name_age()  # result = ("An", 25)
name, age = get_name_age()  # unpacking luôn
```

---

### 1.6 Kiểu Tập Hợp (`set`)

Tập hợp **không có thứ tự**, **không trùng lặp**, **có thể thay đổi**.

```python
nums  = {1, 2, 3, 3, 2}
print(nums)          # {1, 2, 3} — tự loại trùng

empty = set()        # ⚠️ set rỗng phải dùng set(), KHÔNG phải {} (đó là dict!)
from_list = set([1, 1, 2, 3])  # {1, 2, 3}
```

#### Thêm & Xóa phần tử
```python
s = {1, 2, 3}

s.add(4)             # thêm 1 phần tử         → {1, 2, 3, 4}
s.update([5, 6])     # thêm nhiều phần tử      → {1, 2, 3, 4, 5, 6}

s.remove(6)          # xóa — LỖI nếu không tìm thấy
s.discard(99)        # xóa — KHÔNG lỗi nếu không tìm thấy
item = s.pop()       # xóa và trả về 1 phần tử BẤT KỲ
s.clear()            # xóa tất cả
```

#### Phép toán tập hợp
```python
a = {1, 2, 3, 4}
b = {3, 4, 5, 6}

# Hợp (Union) — tất cả phần tử của cả hai
print(a | b)             # {1, 2, 3, 4, 5, 6}
print(a.union(b))        # giống nhau

# Giao (Intersection) — phần tử chung
print(a & b)             # {3, 4}
print(a.intersection(b)) # giống nhau

# Hiệu (Difference) — có ở a mà không có ở b
print(a - b)             # {1, 2}
print(a.difference(b))   # giống nhau

# Hiệu đối xứng — có ở a HOẶC b, nhưng KHÔNG ở cả hai
print(a ^ b)                       # {1, 2, 5, 6}
print(a.symmetric_difference(b))   # giống nhau
```

#### Kiểm tra quan hệ tập hợp
```python
a = {1, 2}
b = {1, 2, 3, 4}

print(a.issubset(b))       # True  — a là tập con của b
print(b.issuperset(a))     # True  — b chứa tất cả phần tử của a
print(a.isdisjoint({5,6})) # True  — a và {5,6} không có phần tử chung
```

#### Ứng dụng thực tế: loại bỏ trùng lặp
```python
names = ["An", "Bình", "An", "Cường", "Bình"]
unique = list(set(names))   # ['An', 'Bình', 'Cường']

# Set comprehension
evens = {x for x in range(10) if x % 2 == 0}  # {0, 2, 4, 6, 8}
```

#### `frozenset` — Set không thể thay đổi
```python
fs = frozenset([1, 2, 3])
# fs.add(4)  → Lỗi! frozenset không thể thay đổi

# frozenset có thể dùng làm key của dict hoặc phần tử của set khác
groups = {frozenset([1,2]): "Nhóm A", frozenset([3,4]): "Nhóm B"}
```

---

### 1.7 Kiểu Từ Điển (`dict`)

Lưu trữ cặp **key–value**, có thứ tự (Python 3.7+), **có thể thay đổi**, key không được trùng.

```python
person = {
    "name": "An",
    "age" : 25,
    "city": "Hà Nội"
}
empty = {}                       # dict rỗng
from_pairs = dict(name="An", age=25)  # cách khác để tạo dict
```

#### Truy cập giá trị
```python
d = {"name": "An", "age": 25}

print(d["name"])                 # An   — LỖI nếu key không tồn tại
print(d.get("age"))              # 25
print(d.get("phone", "N/A"))    # N/A  — trả về giá trị mặc định, không lỗi
```

#### Thêm & Sửa
```python
d = {"name": "An"}

d["age"] = 25                    # thêm key mới
d["name"] = "Bình"               # sửa value (key đã tồn tại)

d.update({"city": "HN", "age": 26})  # cập nhật / thêm nhiều cặp
d |= {"email": "a@b.com"}             # toán tử merge (Python 3.9+)

# setdefault — chỉ thêm nếu key CHƯA tồn tại
d.setdefault("phone", "N/A")     # thêm phone = "N/A"
d.setdefault("name", "Cường")    # KHÔNG đổi — name đã có = "Bình"
```

#### Xóa
```python
d = {"a": 1, "b": 2, "c": 3, "d": 4}

val = d.pop("a")         # xóa key "a", trả về value → val = 1
val = d.pop("z", None)   # key không tồn tại → trả về None (không lỗi)
del d["b"]               # xóa key "b" — LỖI nếu không tìm thấy
last = d.popitem()       # xóa cặp CUỐI CÙNG, trả về (key, value)
d.clear()                # xóa tất cả
```

#### Duyệt dict
```python
person = {"name": "An", "age": 25, "city": "HCM"}

# Duyệt key
for key in person:
    print(key)               # name, age, city

# Duyệt value
for val in person.values():
    print(val)               # An, 25, HCM

# Duyệt cả key và value
for key, val in person.items():
    print(f"{key}: {val}")   # name: An, age: 25, city: HCM
```

#### Kiểm tra & Method khác
```python
d = {"name": "An", "age": 25}

print("name" in d)           # True  (kiểm tra key có tồn tại)
print("An" in d.values())    # True  (kiểm tra value)
print(len(d))                # 2     (số cặp key-value)

print(d.keys())              # dict_keys(['name', 'age'])
print(d.values())            # dict_values(['An', 25])
print(d.items())             # dict_items([('name', 'An'), ('age', 25)])
print(list(d.keys()))        # ['name', 'age']  (chuyển sang list)
```

#### Dict Comprehension (tạo dict nhanh)
```python
# Cú pháp: {key_expr: val_expr for biến in dãy if điều_kiện}

squares = {x: x**2 for x in range(1, 6)}
print(squares)   # {1: 1, 2: 4, 3: 9, 4: 16, 5: 25}

# Lọc dict
scores = {"An": 90, "Bình": 65, "Cường": 80}
passed = {k: v for k, v in scores.items() if v >= 70}
print(passed)    # {'An': 90, 'Cường': 80}

# Đảo key-value
flipped = {v: k for k, v in scores.items()}
print(flipped)   # {90: 'An', 65: 'Bình', 80: 'Cường'}
```

#### Dict lồng nhau (Nested Dict)
```python
students = {
    "An": {
        "age": 20,
        "scores": {"math": 90, "english": 85}
    },
    "Bình": {
        "age": 22,
        "scores": {"math": 75, "english": 95}
    }
}

print(students["An"]["scores"]["math"])   # 90

# Duyệt dict lồng
for name, info in students.items():
    print(f"{name} - Tuổi: {info['age']}")
    for subject, score in info["scores"].items():
        print(f"  {subject}: {score}")
```

---

### 1.8 Kiểu `None`

Đại diện cho **không có giá trị** (tương tự `null` ở các ngôn ngữ khác).

```python
result = None

print(type(result))         # <class 'NoneType'>
print(result is None)       # True  ← luôn dùng 'is' thay vì '=='
print(result is not None)   # False
```

#### Khi nào gặp `None`?
```python
# 1. Giá trị mặc định cho biến chưa có dữ liệu
user_input = None

# 2. Hàm không có return → tự trả về None
def say_hello():
    print("Hello")

result = say_hello()     # In "Hello"
print(result)            # None

# 3. Giá trị mặc định cho tham số hàm
def greet(name=None):
    if name is None:
        print("Xin chào!")
    else:
        print(f"Xin chào {name}!")

greet()          # Xin chào!
greet("An")      # Xin chào An!

# 4. dict.get() khi key không tồn tại
d = {"a": 1}
print(d.get("b"))    # None
```

---

## 2. Bảng Tổng Hợp

| Kiểu        | Ký hiệu      | Thứ tự | Thay đổi được | Trùng lặp |
|-------------|--------------|:------:|:-------------:|:---------:|
| `int`       | `42`         | —      | ❌             | —         |
| `float`     | `3.14`       | —      | ❌             | —         |
| `complex`   | `3+4j`       | —      | `—`             | —         |
| `str`       | `"hello"`    | ✅     | ❌            | ✅        |
| `bool`      | `True/False` | —      | ❌             | —         |
| `list`      | `[...]`      | ✅     | ✅            | ✅        |
| `tuple`     | `(...)`      | ✅     | ❌            | ✅        |
| `set`       | `{...}`      | ❌     | ✅            | ❌        |
| `frozenset` | `frozenset()`| ❌     | ❌            | ❌        |
| `dict`      | `{k: v}`     | ✅     | ✅            | ❌ (key)  |
| `NoneType`  | `None`       | —      | —             | —         |

> 📌 **Giải thích cột "Thứ tự" (ordered):**
>
> **"Có thứ tự" ≠ "Tự sắp xếp"**. Ý nghĩa thật sự:
> - ✅ **Có thứ tự (ordered)** = phần tử **giữ nguyên vị trí** bạn đặt vào, truy cập được bằng chỉ số `[0]`, `[1]`...
> - ❌ **Không có thứ tự (unordered)** = vị trí phần tử **không đảm bảo**, KHÔNG truy cập được bằng chỉ số

```python
# ✅ list — CÓ thứ tự: phần tử ở đâu thì ở đó, không tự đổi chỗ
a = [3, 1, 2]
print(a[0])   # 3 ← luôn là 3, KHÔNG tự sắp xếp thành 1
print(a)      # [3, 1, 2]  ← giữ nguyên thứ tự bạn tạo

# ✅ tuple, str — tương tự, giữ nguyên thứ tự
t = ("chuối", "táo", "cam")
print(t[0])   # "chuối" ← không tự đổi

# ✅ dict — CÓ thứ tự (từ Python 3.7+): key giữ nguyên thứ tự thêm vào
d = {"c": 3, "a": 1, "b": 2}
print(list(d.keys()))   # ['c', 'a', 'b'] ← giữ nguyên, KHÔNG tự sắp theo ABC

# ❌ set — KHÔNG có thứ tự: vị trí phần tử không biết trước
s = {3, 1, 2}
print(s)        # có thể ra {1, 2, 3} hoặc {2, 3, 1} — KHÔNG đảm bảo!
# print(s[0])   # ❌ LỖI! set không hỗ trợ [index]

# ⚠️ Muốn SẮP XẾP thì phải GỌI HÀM, Python KHÔNG tự sắp xếp:
a = [3, 1, 2]
a.sort()        # bây giờ a = [1, 2, 3] ← phải gọi .sort() mới sắp xếp
```

---

## 3. Ép Kiểu (Type Casting)

```python
# Sang số nguyên
int("42")       # 42
int(3.9)        # 3  (không làm tròn, cắt phần thập phân)
int(True)       # 1

# Sang số thực
float("3.14")   # 3.14
float(5)        # 5.0

# Sang chuỗi
str(100)        # "100"
str(True)       # "True"

# Sang list / tuple / set
list((1,2,3))   # [1, 2, 3]
tuple([1,2,3])  # (1, 2, 3)
set([1,1,2,3])  # {1, 2, 3}
```

---

> 💡 **Ghi nhớ nhanh:**
> - Cần thứ tự + sửa được → **`list`**
> - Cần thứ tự + không sửa → **`tuple`**
> - Cần không trùng lặp → **`set`**
> - Cần key–value → **`dict`**
