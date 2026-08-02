# Tài liệu hướng dẫn sử dụng Makefile
### Thay thế cho hệ thống build CMake của dự án `MyCProject` (STM32F446xx + MCUBoot)

> Tài liệu này được viết dựa trên đúng 6 file bạn đã gửi: `CMakeLists.txt` (gốc),
> `middlewares/MCUBoot/CMakeLists.txt`, `cmake_init.cmake`, `project_config.cmake`,
> `build_config.cmake`, `gcc-arm-none-eabi.cmake`. Mọi ví dụ trong tài liệu đều
> lấy từ các giá trị/logic thật trong các file đó, không phải ví dụ chung chung.
> Bộ file Makefile mẫu đi kèm đã được build-thử (`make -n`) với nhiều tổ hợp
> cấu hình và một bộ test cơ chế bằng gcc thật — chi tiết ở Phần 4.4.

---

## Mục lục

1. [Vì sao chuyển từ CMake sang Make không phải là "dịch 1-1"](#1)
2. [Nền tảng cú pháp Makefile](#2)
3. [Bảng đối chiếu CMake ⇄ Makefile](#3)
4. [Áp dụng vào project của bạn](#4)
5. [Các lệnh build thường dùng](#5)
6. [Lỗi thường gặp và cách debug](#6)
7. [Bảng tra nhanh (cheat sheet)](#7)

---

<a name="1"></a>
## 1. Vì sao chuyển từ CMake sang Make không phải là "dịch 1-1"

CMake **không phải** là công cụ build — nó là công cụ **sinh ra** file cho một
công cụ build khác (Makefile, Ninja, Visual Studio project...). Khi bạn chạy
`cmake -B build`, CMake đọc `CMakeLists.txt` và **tự sinh ra** một bộ Makefile
rất phức tạp trong `build/`. Khi bạn chạy `make` trong project hiện tại, thực
ra bạn đang chạy Makefile **do CMake viết hộ**, không phải Makefile do bạn viết.

Điều này giải thích vì sao CMake "có vẻ" làm được nhiều thứ "tự động" mà khi
chuyển sang Make thuần, bạn phải tự tay làm:

| Việc CMake tự làm hộ | Bạn phải tự làm khi dùng Make thuần |
|---|---|
| Theo dõi file `.h` nào ảnh hưởng tới file `.c` nào, để build lại đúng phần cần thiết | Tự bật `-MMD -MP` và `include` các file `.d` sinh ra (Phần 2.12) |
| Tự tạo thư mục `build/` con tương ứng cấu trúc source | Tự viết `mkdir -p $(dir $@)` trong rule biên dịch |
| Scope include-dir / macro theo từng target (PUBLIC/PRIVATE) | Phải tự kiểm soát bằng target-specific variable nếu muốn (Phần 2.11) |
| Kiểm tra version, tìm compiler, sinh lỗi rõ ràng khi thiếu | Tự viết `$(error ...)` / kiểm tra `command -v` |

Nói cách khác: Makefile không "kém" CMake, nhưng CMake đứng ở một tầng trừu
tượng cao hơn, còn Make thao tác trực tiếp ở mức "chạy lệnh gì, khi nào chạy
lại". Học Make thực chất là học lại đúng những gì CMake vẫn luôn làm phía sau,
chỉ là giờ bạn nhìn thấy và viết ra tường minh.

---

<a name="2"></a>
## 2. Nền tảng cú pháp Makefile

Phần này là phần **quan trọng nhất** của tài liệu — nắm chắc phần này thì đọc
phần 3, 4 sẽ rất dễ, vì suy cho cùng cả bộ Makefile ở Phần 4 chỉ là ráp các
khối cú pháp dưới đây lại với nhau.

### 2.1. Rule: target — prerequisites — recipe

Đơn vị nhỏ nhất của Makefile là một **rule** (quy tắc), có dạng:

```makefile
target: prerequisite1 prerequisite2
	lệnh_shell_1
	lệnh_shell_2
```

- **target**: thứ cần tạo ra — thường là tên file (`main.o`), đôi khi chỉ là
  một cái tên hành động không sinh file thật (`clean`, `all` — gọi là *phony
  target*, xem 2.9).
- **prerequisites** (hay *dependencies*): danh sách file/target mà target này
  cần có trước, cách nhau bởi khoảng trắng.
- **recipe**: các dòng lệnh shell sẽ chạy để tạo ra target. **Bắt buộc mỗi
  dòng lệnh phải bắt đầu bằng ký tự TAB**, không phải dấu cách — xem 2.2.

Ví dụ tối giản, tương đương việc CMake biên dịch 1 file `.c` thành `.o`:

```makefile
main.o: main.c common.h
	arm-none-eabi-gcc -c main.c -o main.o
```

Đọc là: "để tạo `main.o`, cần có `main.c` và `common.h`; nếu 1 trong 2 file đó
mới hơn `main.o` (hoặc `main.o` chưa tồn tại), chạy lệnh gcc bên dưới."
**Make chỉ chạy lại recipe khi target CŨ hơn ít nhất 1 prerequisite** (so sánh
bằng thời gian sửa đổi file — mtime). Đây là cơ chế cốt lõi giúp Make chỉ
build lại phần thay đổi, y hệt tinh thần incremental build của CMake.

### 2.2. QUAN TRỌNG NHẤT: Tab, không phải dấu cách

Đây là lỗi **99% người mới học Make gặp phải**. Mọi dòng recipe (dòng lệnh
shell bên dưới target) phải bắt đầu bằng đúng 1 ký tự **Tab** (`\t`), tuyệt
đối không phải 4 hay 8 dấu cách nhìn-giống-tab. Nếu bạn gõ dấu cách, Make báo
lỗi:

```
Makefile:20: *** missing separator.  Stop.
```

(Lỗi này mình thực sự gặp phải khi soạn bộ Makefile mẫu ở Phần 4 — xem hộp
ghi chú ở 4.4). Hầu hết editor hiện đại (VS Code với extension "Makefile
Tools", Vim, v.v.) tự nhận diện file `Makefile`/`*.mk` và tự chèn tab đúng
chỗ — nhưng nếu bạn copy code từ đâu đó (kể cả từ tài liệu này!) qua một nơi
tự động đổi tab thành space (một số trình duyệt, một số trình chat), hãy kiểm
tra lại bằng lệnh:

```bash
cat -A Makefile | grep '\^I' | head -5   # ^I nghĩa là có tab — dòng đúng
```

### 2.3. Biến: `=`, `:=`, `?=`, `+=`

Make có 4 kiểu gán biến hay dùng, khác nhau về **thời điểm được tính giá trị**
— đây là khái niệm hay gây nhầm lẫn nhất cho người quen ngôn ngữ khác:

| Cú pháp | Tên gọi | Khi nào giá trị được tính | Tương tự bên CMake |
|---|---|---|---|
| `X = $(Y)` | recursive (đệ quy) | **Mỗi lần** `$(X)` được dùng, Make mới tra lại `$(Y)` lúc đó | — |
| `X := $(Y)` | simply expanded (tức thời) | **Ngay khi** dòng này được đọc, `$(Y)` được thay giá trị luôn | gần giống `set(X ${Y})` |
| `X ?= giá_trị` | conditional | Chỉ gán **nếu `X` chưa có giá trị** (chưa từng `set` trước đó, kể cả từ dòng lệnh) | `option(X ... )` / cache variable |
| `X += giá_trị` | append | Nối thêm vào cuối, phân cách bởi 1 dấu cách | `list(APPEND X ...)` |

Hệ quả thực tế:

```makefile
A := $(B)     # A nhận giá trị của B NGAY LÚC NÀY (nếu B chưa định nghĩa, A = rỗng)
B = hello     # B định nghĩa SAU cũng không cứu được A vì A đã "chốt" giá trị rồi

C = $(D)      # C là công thức, chưa tính vội
D = hello
$(info $(C))  # In ra "hello" — vì C chỉ tính khi được DÙNG, lúc này D đã có giá trị
```

**Quy tắc mình dùng xuyên suốt tài liệu này**: dùng `:=` mặc định (dễ đoán,
tính ngay), chỉ dùng `=` khi thực sự cần biến "tính muộn" (ví dụ `LDFLAGS`
cần tham chiếu `$(LDSCRIPT)` được định nghĩa ở dòng sau). Dùng `?=` cho MỌI
biến bạn muốn người dùng ghi đè được từ dòng lệnh, ví dụ:

```bash
make BUILD_TYPE=Debug USE_APPLICATION=BootLoader_Test
```

— đây chính là thứ thay thế cho `cmake -DCMAKE_BUILD_TYPE=Debug -D...` bên CMake.

### 2.4. Automatic variables (biến tự động trong recipe)

Bên trong 1 recipe, Make cung cấp sẵn vài biến đặc biệt để bạn không phải gõ
lại tên file nhiều lần:

| Biến | Ý nghĩa |
|---|---|
| `$@` | Tên của **target** hiện tại |
| `$<` | Tên của **prerequisite đầu tiên** |
| `$^` | Tên **tất cả** prerequisites, cách nhau dấu cách (đã loại trùng) |
| `$?` | Các prerequisites **mới hơn** target (phần thực sự gây ra việc build lại) |
| `$(@D)` / `$(@F)` | Thư mục / tên-file-không-kèm-thư-mục của `$@` |
| `$*` | Phần khớp với `%` trong pattern rule (xem 2.5) |

Viết lại ví dụ ở 2.1 bằng automatic variables (đây chính là kiểu viết bạn sẽ
thấy suốt Phần 4):

```makefile
main.o: main.c common.h
	arm-none-eabi-gcc -c $< -o $@
```

### 2.5. Pattern rule — trái tim của một Makefile "không lặp code"

Thay vì viết 1 rule cho từng file `.c` (tưởng tượng project bạn có 100 file
`.c`!), Make cho phép viết **1 rule dùng chung** bằng ký tự `%` (khớp với bất
kỳ chuỗi nào):

```makefile
build/%.o: %.c
	arm-none-eabi-gcc -c $< -o $@
```

Rule này áp dụng cho MỌI cặp `build/xxx.o` ↔ `xxx.c`. Đây chính là kỹ thuật
mà toàn bộ hệ thống build ở Phần 4 dựa vào — thay cho việc CMake tự sinh rule
cho từng file nguồn trong `add_library(... nguon1.c nguon2.c ...)`.

### 2.6. Các hàm dựng sẵn hay dùng

Make gọi hàm bằng cú pháp `$(tên_hàm arg1,arg2,...)`. Vài hàm bạn sẽ thấy liên
tục trong Phần 4:

```makefile
$(wildcard src/*.c)              # liệt kê file khớp mẫu glob (KHÔNG đệ quy vào thư mục con)
$(shell find src -name '*.c')    # chạy lệnh shell, lấy kết quả làm giá trị — dùng để glob ĐỆ QUY
$(patsubst %.c,%.o,$(SRCS))      # thay hậu tố: đổi mọi *.c trong SRCS thành *.o
$(subst PROJECT_ROOT/,,$(X))     # thay chuỗi con (không dùng pattern %)
$(filter %.c,$(SRCS))            # chỉ giữ lại phần tử khớp mẫu
$(notdir path/to/file.c)         # -> file.c  (bỏ phần thư mục)
$(dir path/to/file.c)            # -> path/to/  (chỉ lấy phần thư mục)
$(addprefix -I,$(DIRS))          # thêm tiền tố "-I" vào từng phần tử — hay dùng dựng include flags
$(foreach d,$(DIRS),-I$(d))      # lặp qua danh sách, y hệt addprefix nhưng linh hoạt hơn
$(abspath ./a/../b)              # chuẩn hoá thành đường dẫn tuyệt đối
$(call ten_macro,arg1,arg2)      # gọi 1 "hàm tự định nghĩa" bằng define/endef — xem 2.10
$(error thông báo lỗi)           # in lỗi và DỪNG NGAY LÚC ĐỌC FILE — giống message(FATAL_ERROR ...)
$(warning thông báo)             # in cảnh báo, không dừng — giống message(WARNING ...)
$(info thông báo)                # in thông tin, không dừng — giống message(STATUS ...)
```

> **Lưu ý quan trọng về `wildcard` vs `shell find`**: `$(wildcard dir/*.c)`
> của Make **không** tự động vào thư mục con (không có khái niệm `**` như một
> số ngôn ngữ khác). Muốn gom file `.c` ở nhiều cấp thư mục con (như
> `file(GLOB_RECURSE ...)` bên CMake), phải nhờ shell: `$(shell find dir -name '*.c')`.
> Bộ Makefile ở Phần 4 dùng đúng kỹ thuật này.

> **Makefile có "thấy" file mới thêm vào không?** Có — vì Make **đọc lại toàn
> bộ Makefile từ đầu mỗi lần bạn gõ `make`** (không có khái niệm cache
> Makefile giữa 2 lần chạy), nên `$(shell find ...)` cũng chạy lại mỗi lần,
> tự thấy file `.c` mới ngay từ lần build kế tiếp — không cần sửa gì cả.

### 2.7. `include` và `-include`

```makefile
include mak/config.mk     # nạp nội dung file khác vào ngay vị trí này — LỖI nếu file không tồn tại
-include mak/config.mk    # giống trên nhưng KHÔNG báo lỗi nếu file chưa tồn tại
```

`include` tương đương `include(...)` bên CMake — merge biến/rule của file
được nạp vào cùng một không gian tên với file gọi nó (không tạo scope riêng).
Đây là cách cả bộ Makefile ở Phần 4 được ráp lại từ nhiều file `.mk` nhỏ.
`-include` (có dấu `-` phía trước) dùng khi file có thể **chưa tồn tại ở lần
chạy đầu tiên** — trường hợp điển hình: nạp file `.d` sinh ra từ lần biên dịch
trước (Phần 2.12), vì ở lần build đầu tiên, các file `.d` chưa hề tồn tại.

### 2.8. Điều kiện: `ifeq` / `ifneq` / `ifdef` / `ifndef`

```makefile
ifeq ($(PLATFORM_DRIVER),USE_HAL_DRIVER)
    CPPFLAGS += -DUSE_HAL_DRIVER
else ifeq ($(PLATFORM_DRIVER),USE_LL_DRIVER)
    CPPFLAGS += -DUSE_LL_DRIVER
else
    $(error Invalid PLATFORM_DRIVER value: $(PLATFORM_DRIVER))
endif
```

Đây là kiểu bạn sẽ thấy lặp lại rất nhiều trong `project_config.mk` ở Phần 4
— tương đương từng cặp `if(X STREQUAL "ON") add_definitions(-DX) endif()` bên
CMake. Khác biệt cần nhớ: điều kiện này được xét **lúc đọc file** (giống
CMake configure-time), không phải lúc build.

### 2.9. `.PHONY` — khai báo target "không phải là file"

```makefile
.PHONY: all clean help

clean:
	rm -rf build/
```

Nếu không khai báo `.PHONY`, và lỡ trong thư mục hiện tại có 1 file/thư mục
tên `clean`, Make sẽ hiểu nhầm target `clean` "đã tồn tại và không cũ hơn gì
cả" rồi **bỏ qua không chạy recipe**. Khai báo `.PHONY` nói với Make: "target
này luôn được coi là cần chạy lại, đừng so sánh mtime file". Quy tắc thực
dụng: **target nào không sinh ra 1 file đúng tên nó thì phải khai `.PHONY`.**

### 2.10. `define ... endef` — "hàm" nhiều dòng, gọi bằng `$(call ...)`

CMake có `macro()`/`function()`. Make có `define`/`endef` + `$(call)`:

```makefile
define print_info
	@printf "\033[1;34m%s\033[0m\n" "$(1)"
endef
```

`$(1)`, `$(2)`... là tham số vị trí (giống `${ARGV0}` bên CMake). Gọi macro:

```makefile
some_target:
	$(call print_info,Đang biên dịch...)
```

**Lưu ý cực kỳ quan trọng (mình đã tự vấp phải lỗi này khi soạn Phần 4, xem
4.4):** nếu nội dung `define` chứa các dòng bắt đầu bằng recipe (tab + lệnh
shell, như ví dụ trên), thì `$(call ...)` **chỉ được dùng bên trong 1 recipe**
(dưới 1 target, ở vị trí lẽ ra là lệnh shell). Gọi nó ở **ngoài** mọi recipe
(ví dụ ngay đầu Makefile để in banner "bắt đầu chạy") sẽ gây đúng lỗi
`missing separator` ở mục 2.2 — vì Make cố hiểu đoạn text chứa tab đó như một
recipe "mồ côi", không có target đứng trước. Muốn in thông báo **ngay lúc đọc
file** (không phải lúc chạy recipe), dùng `$(info ...)` (2.6) chứ không phải
`$(call macro_dạng_recipe,...)`.

### 2.11. Target-specific variable / Pattern-specific variable

Đây là cách Make tái hiện `target_compile_options(TARGET PRIVATE ...)` của
CMake — tức là "chỉ áp dụng cờ này cho MỘT target/nhóm file cụ thể, không ảnh
hưởng phần còn lại của project":

```makefile
# Chỉ áp dụng cho ĐÚNG 1 file:
main.o: CFLAGS += -O3

# Áp dụng cho MỌI file khớp pattern (dùng %, khớp cả qua nhiều cấp thư mục):
build/middlewares/MCUBoot/%.o: CFLAGS += -Wno-unused-variable -include assert.h
```

Cơ chế: khi Make build 1 target khớp với dòng khai báo này, nó tạm thời cộng
thêm giá trị vào biến `CFLAGS` **chỉ trong phạm vi build target đó** (và các
prerequisite của nó), sau đó biến trở lại giá trị cũ cho các target khác. Đây
chính là kỹ thuật mình dùng để tái hiện các cờ `-Wno-...` riêng cho
`Lib_MCUBoot` trong CMakeLists.txt gốc của bạn — xem module.mk của MCUBoot ở
Phần 4, và bài test xác nhận cờ **không bị rò rỉ** sang module khác ở Phần 4.4.

### 2.12. Sinh dependency tự động theo header (`-MMD -MP`)

Đây là phần CMake **âm thầm làm hộ bạn** (thông qua depfile khi dùng Makefiles
hoặc Ninja generator) mà nhiều người dùng CMake còn không biết nó tồn tại —
cho tới khi chuyển sang Make thuần và phát hiện: sửa 1 file `.h` xong build lại,
Make **không hề biết** cần biên dịch lại các `.c` include file `.h` đó, vì mặc
định Make chỉ biết prerequisite là đúng những gì bạn gõ tay trong rule.

Giải pháp chuẩn: bảo compiler **tự sinh** ra 1 file `.d` liệt kê toàn bộ header
mà file `.c` đó thực sự include (kể cả include gián tiếp), rồi `include`
ngược file `.d` đó vào Makefile:

```makefile
build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -MF $(@:.o=.d) -c $< -o $@

-include $(OBJS:.o=.d)
```

- `-MMD`: sinh file `.d` (định dạng: `target.o: file.c header1.h header2.h ...`)
- `-MP`: với mỗi header trong danh sách đó, sinh thêm 1 "phony-ish target"
  rỗng cho chính header đó — để tránh lỗi `No rule to make target 'header.h'`
  nếu sau này bạn **xoá** header đó đi (nếu không có `-MP`, file `.d` cũ vẫn
  còn nhắc tới header đã xoá, và Make sẽ báo lỗi thay vì tự hiểu "thôi bỏ qua").
- `-include` (không phải `include` thường — xem 2.7): vì ở lần build đầu tiên,
  chưa có file `.d` nào tồn tại cả.

### 2.13. Order-only prerequisite (`|`) và tự tạo thư mục output

```makefile
build/%.o: %.c | build
	@mkdir -p $(dir $@)
	$(CC) -c $< -o $@

build:
	@mkdir -p $@
```

Dấu `|` tách phần **order-only prerequisite**: Make đảm bảo `build` tồn tại
*trước khi* chạy recipe, nhưng **không** coi việc "thư mục `build` mới hơn
file `.o`" là lý do phải build lại `.o` (khác với prerequisite thường, vốn so
sánh mtime). Trong bộ Makefile ở Phần 4, mình chọn cách đơn giản hơn — gọi
thẳng `mkdir -p $(dir $@)` làm dòng đầu tiên của mọi recipe biên dịch — tốn
thêm 1 lệnh `mkdir` mỗi lần build (không đáng kể) nhưng chắc chắn đúng ngay cả
khi object file nằm ở thư mục con nhiều cấp (mirror lại cấu trúc thư mục
nguồn, xem Phần 4.2), mà không cần khai báo target thư mục riêng cho từng cấp.

---

<a name="3"></a>
## 3. Bảng đối chiếu CMake ⇄ Makefile

Bảng dưới lấy trực tiếp từng dòng trong 6 file bạn đã gửi làm ví dụ cột trái,
để bạn tra cứu nhanh khi đọc Phần 4.

| CMake (trong file bạn gửi) | Makefile tương đương | Ghi chú |
|---|---|---|
| `add_library(Lib_MCUBoot STATIC src1.c src2.c ...)` | Gán 1 biến liệt kê các `.c` rồi cộng vào `SRCS` toàn cục (xem `module.mk`) | Make không có khái niệm "thư viện" riêng trừ khi bạn tự đóng gói `.a` bằng `ar` — xem Phần 4.3 |
| `target_include_directories(Lib_MCUBoot PUBLIC dir1 dir2)` | `INCLUDES += -Idir1 -Idir2` | CMake phân biệt PUBLIC/PRIVATE/INTERFACE (scope theo target); Make không tự phân biệt — xem lưu ý cuối Phần 4.2 |
| `target_compile_definitions(... -DXXX)` / `add_definitions(-DXXX)` | `CPPFLAGS += -DXXX` | |
| `target_compile_options(Lib_MCUBoot PRIVATE -Wno-...)` | Target/pattern-specific variable (2.11): `build/.../MCUBoot/%.o: CFLAGS += -Wno-...` | Chỉ ảnh hưởng đúng target đó, không lan sang module khác |
| `target_link_libraries(A PUBLIC B)` | Trong mô hình "link thẳng .o" ở Phần 4: **không cần** — mọi module đều vào chung 1 lần link | Nếu tách `.a` riêng từng module thì cần khai `A.a: B.a` hoặc đơn giản là liệt kê đúng thứ tự trên dòng lệnh link |
| `foreach(dir IN LISTS ADD_SUBDIRECTORY) add_subdirectory(dir) endforeach()` | `include $(patsubst %,%/module.mk,$(MODULES))` | `add_subdirectory` của CMake **không** sinh tiến trình con — nó merge biến vào cùng scope cha, y hệt `include` của Make (khác hẳn "recursive make" kiểu gọi `$(MAKE) -C dir`) |
| `option(USE_X "..." ON)` / `if(USE_X STREQUAL "ON")` | `USE_X ?= ON` rồi `ifeq ($(USE_X),ON)` | `?=` cho phép ghi đè từ dòng lệnh, giống cache variable của CMake |
| `set(CMAKE_BUILD_TYPE Release)` | `BUILD_TYPE ?= Release` rồi `ifeq`/`else ifeq` chọn cờ tối ưu hoá | |
| `message(FATAL_ERROR "...")` (dừng lúc configure) | `$(error ...)` | Dừng ngay lúc Make **đọc** Makefile |
| `message(WARNING "...")` | `$(warning ...)` | |
| `message(STATUS "...")` | `$(info ...)` | |
| Macro `print_error_message()` tự viết bằng `execute_process` + màu ANSI | `define print_error ... endef` + `$(call print_error,...)` | Chỉ dùng được **trong 1 recipe** — dùng lúc đọc file thì phải `$(error ...)` (xem 2.10) |
| `configure_file(x.ld.in x.ld @ONLY)` | Rule Make chạy `sed -e 's/@VAR@/.../g' x.ld.in > x.ld` | `@ONLY` nghĩa là chỉ thay `@VAR@`, không đụng tới `${VAR}` — `sed` làm đúng việc tương tự |
| `add_custom_command(TARGET x POST_BUILD COMMAND objcopy ...)` | Thêm 1 rule mới có target là file `.bin`, phụ thuộc vào file `.elf` | Make không có khái niệm "POST_BUILD hook" gắn vào 1 target có sẵn — thay vào đó tạo target mới rồi đưa vào danh sách phụ thuộc của `all` |
| `CMAKE_HOST_SYSTEM_NAME` / kiểm tra OS để chọn toolchain path | `ifeq ($(OS),Windows_NT)` / `$(shell uname -s)` | Xem `toolchain.mk` Phần 4.2 |
| `enable_language(C CXX ASM)` + set riêng `CMAKE_C_STANDARD`/`CMAKE_CXX_STANDARD` | Khai riêng `CFLAGS`, `CXXFLAGS`, `ASFLAGS` dùng chung 1 `COMMON_FLAGS`, cộng thêm `-std=...` riêng từng ngôn ngữ | Xem lưu ý trong `toolchain.mk` — không được gộp `-std=gnu11` vào phần dùng chung vì C++ cần `-std=gnu++17` |
| CMake tự chọn "linker language" (dùng `g++` để link nếu có `.cpp`) | `ifneq ($(filter %.cpp,$(SRCS)),) ... else ... endif` chọn `$(CXX)` hay `$(CC)` làm driver link | Xem `Makefile` gốc Phần 4.2 |
| `-MMD -MP` do CMake tự thêm ngầm (generator Makefiles/Ninja) | Bạn tự thêm `-MMD -MP -MF ...` + `-include *.d` | Xem 2.12 — đây là khác biệt hay bị bỏ sót nhất |
| `list(APPEND X a b c)` | `X += a b c` | |
| `cmake -DCMAKE_BUILD_TYPE=Debug -DUSE_X=ON` | `make BUILD_TYPE=Debug USE_X=ON` | |
| `cmake --build build --target clean` | `make clean` (tự viết target `clean: rm -rf build/`) | |

---

<a name="4"></a>
## 4. Áp dụng vào project của bạn

Toàn bộ file trong phần này đã được đóng gói sẵn, kiểm thử, và gửi kèm theo
tài liệu (xem file đính kèm `mycproject_make_example.zip`). Phần dưới đây
giải thích **từng file, từng dòng quan trọng** — bạn có thể đọc lướt và chỉ
dừng lại ở phần nào cần hiểu sâu.

### 4.1. Cấu trúc thư mục

```
MyCProject/                                  (thư mục gốc dự án của bạn)
├── Makefile                                 ← thay CMakeLists.txt (gốc)
├── enviroment/
│   ├── make_config/
│   │   ├── common.mk                        ← thay cmake_init.cmake
│   │   ├── config.mk                        ← thay build_config.cmake
│   │   ├── project_config.mk                ← thay project_config.cmake
│   │   └── toolchain.mk                     ← thay gcc-arm-none-eabi.cmake
│   └── Arm_GNU_Toolchain/...                ← giữ nguyên, không đổi gì
├── platform/{src,inc}/...  + module.mk      ← thay platform/CMakeLists.txt
├── devices/{src,inc}/...   + module.mk
├── libraries/{src,inc}/... + module.mk
├── services/{src,inc}/...  + module.mk
├── system_startup/{src,inc}/... + module.mk
├── application/{src,inc}/... + module.mk
└── middlewares/
    ├── {src,inc}/...      + module.mk       ← thay middlewares/CMakeLists.txt
    └── MCUBoot/            + module.mk       ← thay middlewares/MCUBoot/CMakeLists.txt
        └── mcuboot_config/MCUBoot_Config.mk  ← thay MCUBoot_Config.cmake (bản mẫu, xem 4.5)
```

Giữ nguyên chính tả `enviroment` (thiếu chữ "n") giống hệt project gốc của
bạn, để không phải sửa lại đường dẫn tới thư mục toolchain đã cài sẵn.

Bạn chỉ cần **copy các file `.mk`/`Makefile` này vào đúng vị trí tương ứng**
trong project thật — không đụng gì tới mã nguồn `.c`/`.h`/`.s` đang có.

### 4.2. Đi qua từng file

#### a) `Makefile` (gốc) — thay `CMakeLists.txt`

Đây là file trung tâm, ráp tất cả các mảnh lại với nhau theo đúng thứ tự mà
CMakeLists.txt gốc của bạn `include(...)` — thứ tự include ở đây **quan
trọng** vì file sau có thể dùng biến từ file trước (`toolchain.mk` cần
`BUILD_DIR` đã có giá trị, nên `BUILD_DIR` phải định nghĩa trước dòng
`include toolchain.mk`).

```makefile
# ============================================================================
#  Makefile (gốc)
#  Thay thế cho: CMakeLists.txt (ở thư mục gốc dự án)
# ============================================================================

# Không dùng shell mặc định /bin/sh (thiếu tính năng) mà dùng bash cho chắc
SHELL := /bin/bash

# Thư mục gốc của dự án = thư mục chứa chính Makefile này.
# Dùng cách này thay vì $(CURDIR) để "make" chạy đúng dù bạn gọi lệnh
# "make" từ một thư mục con khác (CMake luôn tự biết CMAKE_SOURCE_DIR,
# Make thì không, nên phải tự tính).
PROJECT_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

MAKE_CONFIG_DIR := $(PROJECT_ROOT)/enviroment/make_config

# ---- 1) Nạp macro dùng chung (thay cmake_init.cmake) ----
include $(MAKE_CONFIG_DIR)/common.mk

# LƯU Ý QUAN TRỌNG: không được viết "$(call print_complete,...)" ở ngoài
# một recipe (như dòng này) — print_complete là macro DẠNG RECIPE (có tab
# + @printf bên trong define), chỉ hợp lệ khi nằm dưới một target. Gọi nó
# ở đây (lúc Make mới ĐỌC file, chưa chạy recipe nào) sẽ gây lỗi
# "missing separator". Muốn in ngay lúc đọc Makefile (giống banner CMake in
# lúc configure), phải dùng $(info ...) — hàm in tức thời của Make.
$(info $(COLOR_GREEN)--------------------------------------------- MAKE STARTED RUN ---------------------------------------------$(COLOR_RESET))

# ---- Chế độ verbose: "make V=1" sẽ in đầy đủ lệnh compiler/linker thay vì
#      chỉ in [CC]/[LD] gọn gàng. Đây là thay thế đơn giản cho việc CMake
#      luôn cho phép xem lệnh đầy đủ qua "make VERBOSE=1" hoặc compile_commands.json ----
ifeq ($(V),1)
    Q :=
else
    Q := @
endif

# ---- 2) Nạp giá trị cấu hình cụ thể (thay build_config.cmake) ----
include $(MAKE_CONFIG_DIR)/config.mk

# ---- 3) Suy ra macro -D... + danh sách module (thay project_config.cmake) ----
include $(MAKE_CONFIG_DIR)/project_config.mk

# ---- Các thư mục output, phải khai báo TRƯỚC khi include toolchain.mk vì
#      toolchain.mk cần BUILD_DIR để dựng -Wl,-Map=...           ----
BUILD_DIR := $(PROJECT_ROOT)/build
LDSCRIPT  := $(BUILD_DIR)/STM32F446XX_LINKER_SCRIPT.ld

# ---- 4) Nạp cấu hình trình biên dịch (thay gcc-arm-none-eabi.cmake) ----
include $(MAKE_CONFIG_DIR)/toolchain.mk

# Đặt tên project cho target chính (tương đương project(${PROJECT_NAME}))
TARGET_ELF := $(BUILD_DIR)/$(PROJECT_NAME).elf
TARGET_BIN := $(BUILD_DIR)/$(PROJECT_NAME).bin

# ============================================================================
#  Thêm các "thư mục con" — thay cho:
#     foreach(dir IN LISTS ADD_SUBDIRECTORY)
#         add_subdirectory("${dir}")
#     endforeach()
#
#  Mỗi module.mk có nhiệm vụ append đường dẫn nguồn của riêng nó vào biến
#  toàn cục SRCS (và, nếu cần, INCLUDES riêng). Vì đây là "include" (không
#  phải gọi $(MAKE) -C con), toàn bộ module.mk chia sẻ chung một không gian
#  biến — giống add_subdirectory() của CMake vốn cũng không sinh tiến trình
#  con mà chỉ "nhập" biến vào cùng scope.
# ============================================================================
SRCS :=
INCLUDES :=

include $(patsubst %,$(PROJECT_ROOT)/%/module.mk,$(MODULES))

INCLUDES += -I$(PROJECT_ROOT)

# ---- Quy đổi danh sách nguồn -> danh sách object, giữ nguyên cấu trúc
#      thư mục con bên trong build/ để tránh trùng tên file giữa 2 module ----
OBJS := $(patsubst $(PROJECT_ROOT)/%.c,$(BUILD_DIR)/%.o,$(filter %.c,$(SRCS)))
OBJS += $(patsubst $(PROJECT_ROOT)/%.cpp,$(BUILD_DIR)/%.o,$(filter %.cpp,$(SRCS)))
OBJS += $(patsubst $(PROJECT_ROOT)/%.s,$(BUILD_DIR)/%.o,$(filter %.s,$(SRCS)))
OBJS += $(patsubst $(PROJECT_ROOT)/%.S,$(BUILD_DIR)/%.o,$(filter %.S,$(SRCS)))
DEPS := $(OBJS:.o=.d)

# Tương đương việc CMake tự chọn "linker language": nếu có ít nhất 1 file
# .cpp trong SRCS thì dùng g++ để link (và nối thêm -lstdc++ -lsupc++,
# giống CMAKE_CXX_LINK_FLAGS), ngược lại dùng gcc như bình thường.
ifneq ($(filter %.cpp,$(SRCS)),)
    LINK_DRIVER := $(CXX)
    LDLIBS      += -Wl,--start-group -lstdc++ -lsupc++ -Wl,--end-group
else
    LINK_DRIVER := $(CC)
endif

# ============================================================================
#  Target mặc định
# ============================================================================
.PHONY: all
all: $(TARGET_BIN)
	$(call print_info,--------------------------------------------- SIZE INFO -----------------------------------------------------)
	@$(SIZE) $(TARGET_ELF)

# ---- Sinh linker script từ template .ld.in — thay cho configure_file(... @ONLY) ----
$(LDSCRIPT): $(PROJECT_ROOT)/system_startup/Startup_Config/WeAct_STM32446/src/STM32F446XX_LINKER_SCRIPT.ld.in | $(BUILD_DIR)
	$(call print_info,[GEN] $(notdir $@))
	$(Q)sed -e 's/@RAM_ORIGIN@/$(RAM_ORIGIN)/g' \
	     -e 's/@RAM_LENGTH@/$(RAM_LENGTH)/g' \
	     -e 's/@FLASH_ORIGIN@/$(FLASH_ORIGIN)/g' \
	     -e 's/@FLASH_LENGTH@/$(FLASH_LENGTH)/g' \
	     -e 's/@MIN_HEAP_SIZE@/$(MIN_HEAP_SIZE)/g' \
	     -e 's/@MIN_STACK_SIZE@/$(MIN_STACK_SIZE)/g' \
	     $< > $@

# ---- Liên kết ra file .elf — thay add_executable + target_link_libraries ----
# Không cần "-Wl,--whole-archive": vì ta liên kết thẳng các file .o (không
# đóng gói .a trung gian), trình liên kết BẮT BUỘC phải đưa toàn bộ .o vào,
# không có chuyện "chọn lọc" như khi liên kết archive .a — nên không còn
# lý do phải ép --whole-archive nữa. (Xem thêm phần "Vì sao bỏ --whole-archive"
# trong tài liệu hướng dẫn đi kèm.)
$(TARGET_ELF): $(OBJS) $(LDSCRIPT)
	$(call print_info,[LD] $(notdir $@))
	$(Q)$(LINK_DRIVER) $(LDFLAGS) $(OBJS) -o $@ $(LDLIBS)

# ---- Sinh file .bin từ .elf — thay add_bin_output() ----
$(TARGET_BIN): $(TARGET_ELF)
	$(call print_info,[BIN] $(notdir $@))
	$(Q)$(OBJCOPY) -O binary $< $@

# ---- Quy tắc biên dịch (pattern rule) dùng chung cho MỌI module ----
$(BUILD_DIR)/%.o: $(PROJECT_ROOT)/%.c
	@mkdir -p $(dir $@)
	$(call print_info,[CC] $(subst $(PROJECT_ROOT)/,,$<))
	$(Q)$(CC) $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(PROJECT_ROOT)/%.cpp
	@mkdir -p $(dir $@)
	$(call print_info,[CXX] $(subst $(PROJECT_ROOT)/,,$<))
	$(Q)$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(PROJECT_ROOT)/%.s
	@mkdir -p $(dir $@)
	$(call print_info,[AS] $(subst $(PROJECT_ROOT)/,,$<))
	$(Q)$(CC) $(ASFLAGS) $(CPPFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(PROJECT_ROOT)/%.S
	@mkdir -p $(dir $@)
	$(call print_info,[AS] $(subst $(PROJECT_ROOT)/,,$<))
	$(Q)$(CC) $(ASFLAGS) $(CPPFLAGS) $(INCLUDES) $(DEPFLAGS) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $@

# ============================================================================
#  Nạp các file .d đã sinh ra, để Make biết build lại .o nào khi 1 header
#  thay đổi. Dấu "-" phía trước "include" giúp Make KHÔNG báo lỗi ở lần
#  build đầu tiên, khi các file .d chưa tồn tại.
# ============================================================================
-include $(DEPS)

# ============================================================================
#  Các target tiện ích
# ============================================================================
.PHONY: clean
clean:
	$(call print_warning,Removing $(BUILD_DIR))
	@rm -rf $(BUILD_DIR)

.PHONY: help
help:
	@echo "Cac lenh thuong dung:"
	@echo "  make                          Build o che do $(BUILD_TYPE) (mac dinh)"
	@echo "  make BUILD_TYPE=Debug         Build ban Debug"
	@echo "  make USE_APPLICATION=BootLoader_Test   Build voi cau hinh ung dung khac"
	@echo "  make clean                    Xoa thu muc build/"
	@echo "  make -j\$$(nproc)               Build song song nhieu luong"
	@echo "  make V=1                      In day du lenh compiler dang chay (xem toolchain.mk)"
```

Vài điểm đáng chú ý so với bản CMake gốc:

- **`PROJECT_ROOT`** (dòng 13): CMake tự có sẵn `CMAKE_SOURCE_DIR`; Make không
  có khái niệm tương đương nên phải tự tính bằng
  `$(abspath $(dir $(lastword $(MAKEFILE_LIST))))` — nghĩa đen là "lấy đường
  dẫn tuyệt đối của thư mục chứa Makefile đang được đọc". Nhờ vậy `make` chạy
  đúng dù bạn gọi từ thư mục con nào.
- **Banner "MAKE STARTED RUN"** dùng `$(info ...)` chứ không phải
  `$(call print_complete,...)` — xem hộp cảnh báo ở mục 2.10, đây là lỗi thật
  mình gặp phải khi soạn file này (chi tiết ở 4.4).
- **`SRCS :=` / `INCLUDES :=`** rồi mới `include` các `module.mk`: đây là mô
  hình "1 hồ chứa chung" — mọi module đổ nguồn của mình vào cùng 2 biến toàn
  cục này, khác với CMake vốn coi mỗi `add_library` là 1 target tách biệt.
  Đánh đổi và lý do chọn cách này: xem mục 4.3.
- **`LINK_DRIVER`**: tự động chọn `g++` làm driver link nếu project có bất kỳ
  file `.cpp` nào (mô phỏng CMake tự chọn "linker language"), còn không thì
  dùng `gcc` bình thường — đúng với bản gốc dùng `g++` làm `CMAKE_LINKER`.
- **Rule sinh linker script** (`$(LDSCRIPT): .../STM32F446XX_LINKER_SCRIPT.ld.in`)
  dùng `sed` để thay các placeholder `@RAM_ORIGIN@`, `@RAM_LENGTH@`, v.v. —
  đúng cơ chế `configure_file(... @ONLY)` của CMake. **Bạn cần đối chiếu với
  file `.ld.in` thật của mình** — xem mục 4.5.
- **Không còn `-Wl,--whole-archive`** — xem giải thích đầy đủ ở mục 4.3.

#### b) `common.mk` — thay `cmake_init.cmake`

```makefile
# ============================================================================
#  common.mk
#  Thay thế cho: enviroment/cmake_config/cmake_init.cmake
#
#  Chứa các macro dùng chung: in thông báo có màu, và các hàm tiện ích.
#  Trong CMake đây là các "macro()"/"function()". Trong Make, ta dùng
#  "define ... endef" để định nghĩa và "$(call ten_macro,tham_so)" để gọi.
# ============================================================================

# ---- Sinh ký tự ESC thật (0x1B) để dùng cho mã màu ANSI ----
# Không thể gõ thẳng "\033" vào biến Make và mong nó thành ký tự điều khiển
# (Make không tự diễn dịch escape như C). Cách chuẩn là nhờ shell "printf"
# sinh ra byte ESC thật, rồi lưu byte đó vào một biến Make.
ESC          := $(shell printf '\033')
COLOR_GREEN  := $(ESC)[1;32m
COLOR_BLUE   := $(ESC)[1;34m
COLOR_RED    := $(ESC)[1;31m
COLOR_YELLOW := $(ESC)[1;33m
COLOR_RESET  := $(ESC)[0m

# ---- Tương đương print_complete_message() ----
define print_complete
	@printf "$(COLOR_GREEN)%s$(COLOR_RESET)\n" "$(1)"
endef

# ---- Tương đương print_infor_message() ----
define print_info
	@printf "$(COLOR_BLUE)%s$(COLOR_RESET)\n" "$(1)"
endef

# ---- Tương đương print_warning_message() ----
define print_warning
	@printf "$(COLOR_YELLOW)%s$(COLOR_RESET)\n" "$(1)"
endef

# ---- Tương đương print_error_message() ----
# CMake: in màu đỏ rồi message(FATAL_ERROR ...) để dừng hẳn quá trình.
# Make: in màu đỏ rồi "exit 1" để dừng recipe hiện tại với mã lỗi != 0.
define print_error
	@printf "$(COLOR_RED)%s$(COLOR_RESET)\n" "$(1)"
	@exit 1
endef

# Ghi chú: nếu bạn cần dừng build ngay khi ĐỌC Makefile (giống hệt việc
# CMake dừng ngay khi configure, ví dụ khi kiểm tra một biến không hợp lệ),
# hãy dùng thẳng hàm $(error ...) sẵn có của Make thay vì $(call print_error,...):
#
#   $(error Invalid PLATFORM_DRIVER value: $(PLATFORM_DRIVER))
#
# $(error ...) tương đương message(FATAL_ERROR ...): in ra và dừng NGAY,
# không cần đợi đến bước build. $(call print_error,...) ở trên chỉ dừng được
# một recipe (nghĩa là phải nằm trong 1 target), nên dùng cho lỗi phát hiện
# lúc build, còn $(error ...) dùng cho lỗi phát hiện lúc đọc cấu hình.
```

Điểm kỹ thuật đáng chú ý: sinh ký tự ESC thật bằng `$(shell printf '\033')`
rồi lưu vào biến — vì Make **không** tự diễn dịch escape `\033` kiểu C trong
text thường; cách duy nhất để có byte ESC thật là nhờ shell tạo ra hộ, đúng 1
lần, rồi tái sử dụng biến đó ở mọi nơi cần in màu (kể cả trong `$(info ...)`
lẫn trong `@printf` bên trong recipe).

#### c) `config.mk` — thay `build_config.cmake`

File này **không có logic if/else**, chỉ có giá trị cụ thể — y hệt tinh thần
`build_config.cmake` gốc. Toàn bộ dùng `?=` để bạn override được từ dòng lệnh.
Trích đoạn đại diện (đầy đủ 78 dòng xem file đính kèm):

```makefile
# ============================================================================
#  config.mk
#  Thay thế cho: enviroment/cmake_config/build_config.cmake
#
#  File này chỉ chứa GIÁ TRỊ cụ thể (giống build_config.cmake), không chứa
#  logic if/else. Logic suy ra macro -D... nằm ở project_config.mk.
#
#  Dùng "?=" (chỉ gán NẾU biến chưa có giá trị) cho những biến bạn muốn
#  người dùng có thể ghi đè từ dòng lệnh, ví dụ:
#     make BUILD_TYPE=Debug USE_APPLICATION=BootLoader_Test
#  giống hệt "cmake -DCMAKE_BUILD_TYPE=Debug -DUSE_APPLICATION=..." bên CMake.
# ============================================================================

PROJECT_NAME ?= MyCProject
BUILD_TYPE   ?= Release

# ──────────────────── Application ────────────────────────────────────────
LIB_APPLICATION_NAME ?= Lib_Application
LIB_APPLICATION_DIR  ?= application
USE_APPLICATION       ?= BootLoader_TinyCrypt_Test

# ──────────────────── Platform ───────────────────────────────────────────
LIB_PLATFORM_NAME ?= Lib_Platform
LIB_PLATFORM_DIR  ?= platform
USE_FREERTOS       ?= OFF
PLATFORM_DRIVER    ?= USE_HAL_DRIVER
MCU_FAMILY          ?= STM32F446xx

# ──────────────────── System Startup ─────────────────────────────────────
LIB_SYSTEM_STARTUP_NAME ?= Lib_System_Startup
LIB_SYSTEM_STARTUP_DIR  ?= system_startup
USE_SERIAL_LOG           ?= ON
RAM_ORIGIN                ?= 0x20000000
RAM_LENGTH                ?= 128K
MIN_HEAP_SIZE              ?= 0x800
# ... (các nhóm Devices / Middlewares / Services / Libraries theo đúng
#      khuôn mẫu này — xem file .mk đầy đủ trong bản đính kèm)
```

#### d) `project_config.mk` — thay `project_config.cmake`

Nhiệm vụ: (1) biến từng cờ `ON/OFF` thành `-D...` tương ứng, (2) dựng danh
sách module cần build. Trích đoạn đại diện:

```makefile
# ============================================================================
#  project_config.mk
#  Thay thế cho: enviroment/cmake_config/project_config.cmake
#
#  Nhiệm vụ:
#   1) Biến mỗi cờ ON/OFF trong config.mk thành một -D tương ứng
#      (giống add_definitions(-DXXX) trong CMake)
#   2) Dựng danh sách "module con" cần build, tương đương biến
#      ADD_SUBDIRECTORY trong project_config.cmake
# ============================================================================

# ---------------------------------------------------------------------------
# 1) Application layer
# ---------------------------------------------------------------------------
ifeq ($(filter $(USE_APPLICATION),BootLoader_Test BootLoader_TinyCrypt_Test),$(USE_APPLICATION))
    CPPFLAGS += -DUSE_BOOTLOADER
endif

# ---------------------------------------------------------------------------
# 2) Platform layer
# ---------------------------------------------------------------------------
ifeq ($(USE_FREERTOS),ON)
    CPPFLAGS += -DUSE_FREERTOS
endif

ifeq ($(PLATFORM_DRIVER),USE_HAL_DRIVER)
    CPPFLAGS += -DUSE_HAL_DRIVER
else ifeq ($(PLATFORM_DRIVER),USE_LL_DRIVER)
    CPPFLAGS += -DUSE_LL_DRIVER
else
    $(error Invalid PLATFORM_DRIVER value: $(PLATFORM_DRIVER). Valid values are USE_HAL_DRIVER or USE_LL_DRIVER.)
endif

ifeq ($(MCU_FAMILY),STM32F446xx)
    CPPFLAGS += -DSTM32F446xx
else
    $(error Invalid MCU_FAMILY value: $(MCU_FAMILY). Valid values are STM32F446xx or STM32L476xx.)
endif

# ---------------------------------------------------------------------------
# ... (các khối Devices / Middlewares / Services / Libraries lặp lại đúng
#      khuôn mẫu "ifeq ($(USE_X),ON) CPPFLAGS += -DUSE_X endif" cho từng cờ)
```

Và đoạn dựng danh sách module (tương đương biến `ADD_SUBDIRECTORY` bên CMake):

```makefile
# 8) Danh sách module cần add — tương đương biến ADD_SUBDIRECTORY
# ---------------------------------------------------------------------------
# LƯU Ý: giống hệt project_config.cmake gốc, nhánh BootLoader_Test đang KHÔNG
# thêm devices/ và services/. Nếu đây là chủ ý của bạn thì giữ nguyên, còn
# nếu là thiếu sót trong file CMake gốc thì đây cũng là dịp để sửa luôn.
ifeq ($(USE_APPLICATION),BootLoader_Test)
    MODULES := $(LIB_PLATFORM_DIR) \
               $(LIB_SYSTEM_STARTUP_DIR) \
               $(LIB_LIBRARIES_DIR) \
               $(LIB_MIDDLEWARES_DIR) \
               $(LIB_APPLICATION_DIR)
else
    MODULES := $(LIB_PLATFORM_DIR) \
               $(LIB_DEVICES_DIR) \
               $(LIB_LIBRARIES_DIR) \
               $(LIB_MIDDLEWARES_DIR) \
               $(LIB_SERVICES_DIR) \
               $(LIB_SYSTEM_STARTUP_DIR) \
               $(LIB_APPLICATION_DIR)
endif
```

> **Lưu ý y hệt file gốc của bạn**: nhánh `BootLoader_Test` hiện KHÔNG thêm
> `devices/` và `services/` vào build — mình giữ đúng logic này từ
> `project_config.cmake` bạn gửi. Nếu đây là chủ ý (bootloader tối giản không
> cần các driver thiết bị) thì không cần sửa gì; nếu là thiếu sót từ trước thì
> đây cũng là dịp tiện sửa luôn.

#### e) `toolchain.mk` — thay `gcc-arm-none-eabi.cmake`

Đây là file dài nhất vì gánh nhiều việc nhất: chọn đường dẫn compiler theo OS,
định nghĩa cờ theo MCU, theo `BUILD_TYPE`, và cờ liên kết.

```makefile
# ============================================================================
#  toolchain.mk
#  Thay thế cho: enviroment/cmake_config/gcc-arm-none-eabi.cmake
#
#  Yêu cầu: các biến PROJECT_ROOT, BUILD_DIR, PROJECT_NAME, LDSCRIPT, BUILD_TYPE,
#  MCU_FAMILY phải được định nghĩa TRƯỚC khi include file này (xem Makefile gốc).
# ============================================================================

# ---- Xác định hệ điều hành đang chạy Make (thay CMAKE_HOST_SYSTEM_NAME) ----
ifeq ($(OS),Windows_NT)
    HOST_OS     := Windows
    EXE_SUFFIX  := .exe
else
    HOST_OS     := $(shell uname -s)
    EXE_SUFFIX  :=
endif

# ---- Chọn đường dẫn toolchain theo hệ điều hành ----
# (dùng $(info ...) chứ không phải $(call print_info,...) — lý do xem
#  ghi chú "LƯU Ý QUAN TRỌNG" trong Makefile gốc: đây là lúc ĐỌC cấu hình,
#  chưa có recipe nào đang chạy)
ifeq ($(HOST_OS),Linux)
    $(info $(COLOR_BLUE)--------------------------------------------- Run on Linux -------------------------------------------------$(COLOR_RESET))
    TOOLCHAIN_PREFIX := $(PROJECT_ROOT)/enviroment/Arm_GNU_Toolchain/arm-gnu-toolchain-15.2.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi
else ifeq ($(HOST_OS),Windows)
    $(info $(COLOR_BLUE)--------------------------------------------- Run on Windows ---------------------------------------------$(COLOR_RESET))
    TOOLCHAIN_PREFIX := $(PROJECT_ROOT)/enviroment/Arm_GNU_Toolchain/1331/bin/arm-none-eabi
else ifeq ($(HOST_OS),Darwin)
    $(info $(COLOR_BLUE)--------------------------------------------- Run on macOS ---------------------------------------------$(COLOR_RESET))
    TOOLCHAIN_PREFIX := arm-none-eabi
else
    $(info $(COLOR_BLUE)Other OS: $(HOST_OS)$(COLOR_RESET))
    TOOLCHAIN_PREFIX := arm-none-eabi
endif

CC      := $(TOOLCHAIN_PREFIX)-gcc$(EXE_SUFFIX)
CXX     := $(TOOLCHAIN_PREFIX)-g++$(EXE_SUFFIX)
AS      := $(TOOLCHAIN_PREFIX)-gcc$(EXE_SUFFIX)
OBJCOPY := $(TOOLCHAIN_PREFIX)-objcopy$(EXE_SUFFIX)
SIZE    := $(TOOLCHAIN_PREFIX)-size$(EXE_SUFFIX)

# Kiểm tra công cụ tồn tại trước khi build thật sự (tương đương các
# message(FATAL_ERROR "arm-none-eabi-objcopy not found...") trong cmake_init.cmake).
# Chỉ kiểm tra khi thực sự build (không kiểm tra khi chạy "make clean", "make help"...).
ifeq ($(filter clean help,$(MAKECMDGOALS)),)
    ifeq ($(shell command -v $(CC) 2>/dev/null),)
        $(error $(CC) not found. Kiem tra lai TOOLCHAIN_PREFIX trong toolchain.mk)
    endif
endif

# ---- Chuẩn ngôn ngữ (tương đương CMAKE_C_STANDARD=11 + CMAKE_C_EXTENSIONS ON) ----
C_STD   := -std=gnu11
CXX_STD := -std=gnu++17

# ---- Bảng cấu hình MCU ----
ifeq ($(MCU_FAMILY),STM32F446xx)
    MCU_CORE       := cortex-m4
    MCU_FPU        := fpv4-sp-d16
    MCU_FLOAT_ABI  := hard
    MCU_ARCH_FLAGS :=
else ifeq ($(MCU_FAMILY),STM32H563xx)
    MCU_CORE       := cortex-m33
    MCU_FPU        := fpv5-sp-d16
    MCU_FLOAT_ABI  := hard
    MCU_ARCH_FLAGS := -march=armv8-m.main+dsp+fp
else
    $(error Unsupported MCU_FAMILY: $(MCU_FAMILY))
endif

MCU_FLAGS := -mcpu=$(MCU_CORE) -mfpu=$(MCU_FPU) -mfloat-abi=$(MCU_FLOAT_ABI) $(MCU_ARCH_FLAGS)

# ---- Cờ tối ưu hoá theo BUILD_TYPE (Debug / Release) ----
ifeq ($(BUILD_TYPE),Debug)
    OPT_FLAGS := -O0 -g3
else ifeq ($(BUILD_TYPE),Release)
    OPT_FLAGS := -Os -g0
else
    $(error BUILD_TYPE khong hop le: '$(BUILD_TYPE)'. Chi chap nhan Debug hoac Release)
endif

# ---- Cờ CHUNG cho cả C / C++ / ASM ----
# QUAN TRỌNG: trong gcc-arm-none-eabi.cmake gốc, CMAKE_CXX_FLAGS và
# CMAKE_ASM_FLAGS được định nghĩa bằng cách kế thừa TOÀN BỘ CMAKE_C_FLAGS
# rồi cộng thêm cờ riêng — chứ KHÔNG viết lại từ đầu. Ta mô phỏng đúng cấu
# trúc đó bằng một biến COMMON_FLAGS dùng chung, để tránh lặp code và tránh
# quên đồng bộ khi sau này cần sửa 1 cờ cảnh báo cho cả 3 ngôn ngữ.
#
# Cờ "-std=..." KHÔNG nằm trong COMMON_FLAGS vì C và C++ dùng chuẩn khác
# nhau (gnu11 vs gnu++17) — CMake tách riêng việc này qua CMAKE_C_STANDARD/
# CMAKE_CXX_STANDARD, nên ta cũng tách riêng C_STD/CXX_STD tương tự.
COMMON_FLAGS := $(MCU_FLAGS) $(OPT_FLAGS)
COMMON_FLAGS += -Wall -Wextra -Wpedantic -fdata-sections -ffunction-sections
COMMON_FLAGS += -Wconversion -Wsign-conversion -Wcast-align -Wnull-dereference -Wnonnull
COMMON_FLAGS += -Wreturn-type -Wimplicit-function-declaration
COMMON_FLAGS += -Werror=return-type -Werror=implicit-function-declaration -Werror=implicit-int

# ---- Cờ biên dịch C ----
CFLAGS := $(COMMON_FLAGS) $(C_STD)

# ---- Cờ biên dịch ASM (assembler-with-cpp: file .s được tiền xử lý qua cpp) ----
ASFLAGS := $(COMMON_FLAGS) -x assembler-with-cpp

# ---- Cờ biên dịch C++ ----
CXXFLAGS := $(COMMON_FLAGS) $(CXX_STD)
CXXFLAGS += -fno-rtti -fno-exceptions -fno-threadsafe-statics

# ---- Sinh dependency tự động theo header (.d) ----
# Đây là phần CMake ÂM THẦM làm hộ bạn (qua depfile của Makefiles/Ninja generator).
# Sang Make thuần thì phải tự khai báo: -MMD sinh file .d liệt kê các .h mà
# từng .o phụ thuộc, -MP thêm target rỗng cho từng header đó (tránh lỗi
# "No rule to make target" khi một header bị XOÁ đi giữa hai lần build).
DEPFLAGS = -MMD -MP -MF $(@:.o=.d)

# ---- Cờ liên kết ----
# LDSCRIPT/BUILD_DIR/PROJECT_NAME được định nghĩa ở Makefile gốc trước khi include file này.
LDFLAGS := $(MCU_FLAGS)
LDFLAGS += -T $(LDSCRIPT)
LDFLAGS += --specs=nano.specs
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(PROJECT_NAME).map -Wl,--gc-sections
LDFLAGS += -Wl,--print-memory-usage

# Không dùng "BootLoader_Test" thuần (giống target_link_options -u _printf_float
# trong CMakeLists.txt) — bootloader tối giản không cần in số thực qua printf.
ifneq ($(USE_APPLICATION),BootLoader_Test)
    LDFLAGS += -u _printf_float
endif

LDLIBS := -Wl,--start-group -lc -lm -Wl,--end-group

# Nếu project có dùng C++ ở bất kỳ module nào, mở comment dòng dưới
# (tương đương CMAKE_CXX_LINK_FLAGS thêm -lstdc++ -lsupc++):
# LDLIBS += -Wl,--start-group -lstdc++ -lsupc++ -Wl,--end-group
```

Điểm khác biệt quan trọng nhất so với việc "dịch thẳng" là khối
**`COMMON_FLAGS`**: bản CMake gốc định nghĩa `CMAKE_CXX_FLAGS` và
`CMAKE_ASM_FLAGS` bằng cách **kế thừa toàn bộ `CMAKE_C_FLAGS`** rồi cộng thêm
cờ riêng (`${CMAKE_C_FLAGS} -fno-rtti ...`) — chứ không viết lại cờ cảnh báo
từ đầu. Nếu dịch "ngây thơ" từng dòng `set()` riêng lẻ, rất dễ vô tình viết
`CXXFLAGS`/`ASFLAGS` thiếu mất một nửa số cờ cảnh báo mà bản C có. Mình tách
một biến `COMMON_FLAGS` dùng chung cho cả 3 (đúng cấu trúc kế thừa của bản
gốc), rồi mỗi ngôn ngữ chỉ cộng thêm phần riêng của nó (`-std=...`,
`-fno-rtti`, `-x assembler-with-cpp`...). Đây cũng là lý do `-std=gnu11`
**không** nằm trong `COMMON_FLAGS` — vì C++ cần `-std=gnu++17`, gộp chung sẽ
sai ngay lập tức khi biên dịch file `.cpp`.

M��t điểm nữa: khối kiểm tra `command -v $(CC)` (dòng có `$(error $(CC) not found...)`)
là thứ **CMake cũng ngầm làm** (nó tự báo lỗi rõ ràng nếu không tìm thấy
compiler lúc configure) — mình thêm tương đương cho Make, nhưng có loại trừ
`clean`/`help` ra khỏi việc kiểm tra này (qua `$(filter clean help,$(MAKECMDGOALS))`),
vì bạn chạy `make clean` hay `make help` thì không cần có sẵn compiler.

#### f) `module.mk` — thay từng `CMakeLists.txt` con

Có 2 kiểu, tuỳ mức độ bạn muốn kiểm soát:

**Kiểu 1 — tự động gom nguồn (dùng cho hầu hết module):**

```makefile
# ============================================================================
#  module.mk cho thư mục "platform"
#  Thay thế cho: platform/CMakeLists.txt  (add_library(Lib_Platform STATIC ...))
#
#  Đây là MẪU DÙNG CHUNG cho các module "phẳng", không cần liệt kê tay từng
#  file: mọi *.c trong src/ (kể cả thư mục con) được tự động gom vào SRCS,
#  giống tinh thần file(GLOB_RECURSE ...) bên CMake. Copy file này sang
#  devices/, libraries/, services/, system_startup/, application/ và chỉ
#  cần đổi MODULE_DIR.
#
#  Muốn kiểm soát chặt như CMakeLists.txt gốc (liệt kê tay từng file)? Xem
#  middlewares/MCUBoot/module.mk để có ví dụ đầy đủ theo phong cách đó.
# ============================================================================
MODULE_DIR := $(PROJECT_ROOT)/platform

# "find" chạy lại mỗi khi bạn gõ "make" (Makefile được đọc lại từ đầu mỗi
# lần chạy), nên thêm file .c mới vào src/ sẽ tự được thấy ở lần build kế
# tiếp — không cần sửa module.mk, tương tự cảm giác dùng GLOB bên CMake.
SRCS     += $(shell find $(MODULE_DIR)/src -name '*.c' 2>/dev/null)
SRCS     += $(shell find $(MODULE_DIR)/src -name '*.cpp' 2>/dev/null)
# Gom luôn file hợp ngữ khởi động (startup_*.s) nếu module này có
SRCS     += $(shell find $(MODULE_DIR)/src -name '*.s' -o -name '*.S' 2>/dev/null)
INCLUDES += -I$(MODULE_DIR)/inc
```

Copy nguyên file này sang `devices/`, `libraries/`, `services/`,
`system_startup/`, `application/` — chỉ cần đổi `MODULE_DIR`.

**Kiểu 2 — liệt kê tay từng file (dùng khi cần khớp chính xác, ví dụ MCUBoot):**

`middlewares/module.mk` trước hết gom nguồn cục bộ của chính `middlewares/`,
sau đó `include` tiếp vào `MCUBoot/module.mk` một cách **có điều kiện** — mô
phỏng đúng việc `middlewares/CMakeLists.txt` gốc của bạn gọi
`add_subdirectory(MCUBoot)` có điều kiện theo `USE_MCUBOOT`:

```makefile
# ============================================================================
#  module.mk cho thư mục "middlewares"
#  Thay thế cho: middlewares/CMakeLists.txt
#
#  middlewares/ vừa có thể chứa mã nguồn riêng của nó (ví dụ lớp bọc cho
#  FatFs, MicroRL...), vừa gọi tiếp add_subdirectory(MCUBoot) một cách có
#  điều kiện. Ở đây ta mô phỏng lại: gom nguồn cục bộ trước, sau đó "include"
#  tiếp module.mk con — đúng tinh thần add_subdirectory lồng nhau của CMake.
# ============================================================================
MODULE_DIR := $(PROJECT_ROOT)/middlewares

SRCS     += $(shell find $(MODULE_DIR)/src -name '*.c' 2>/dev/null)
INCLUDES += -I$(MODULE_DIR)/inc

# Thay cho: if(USE_MCUBOOT) add_subdirectory(MCUBoot) endif() (giả định
# cấu trúc middlewares/CMakeLists.txt gốc của bạn có logic tương tự)
ifeq ($(USE_MCUBOOT),ON)
    include $(MODULE_DIR)/MCUBoot/module.mk
endif
```

Và đây là bản dịch **1:1, đầy đủ** từ `middlewares/MCUBoot/CMakeLists.txt` —
file duy nhất trong 6 file bạn gửi mà mình có đủ danh sách nguồn thật, nên
liệt kê tay giống hệt bản gốc để bạn đối chiếu từng dòng:

```makefile
# ============================================================================
#  module.mk cho middlewares/MCUBoot
#  Thay thế 1:1 cho: middlewares/MCUBoot/CMakeLists.txt
#  (add_library(Lib_MCUBoot STATIC ...) trong file bạn đã gửi)
#
#  Đây là ví dụ chuyển đổi TRỰC TIẾP, liệt kê tay từng file — giống hệt
#  phong cách CMakeLists.txt gốc của bạn — để bạn dễ đối chiếu từng dòng.
#  Với các module khác không cần kiểm soát chặt như vậy, bạn có thể dùng
#  cách "wildcard/find" gọn hơn (xem platform/module.mk).
# ============================================================================
MCUBOOT_DIR := $(PROJECT_ROOT)/middlewares/MCUBoot

# ---- STM32 Porting Sources ----
MCUBOOT_SRCS := \
    $(MCUBOOT_DIR)/STM32_Porting/src/app_dfu.c \
    $(MCUBOOT_DIR)/STM32_Porting/src/crypto_backend.c \
    $(MCUBOOT_DIR)/STM32_Porting/src/flash_map_backend.c \
    $(MCUBOOT_DIR)/STM32_Porting/src/os_abstraction.c \
    \
    $(MCUBOOT_DIR)/boot/bootutil/src/bootutil_area.c \
    $(MCUBOOT_DIR)/boot/bootutil/src/bootutil_find_key.c \
    $(MCUBOOT_DIR)/boot/bootutil/src/bootutil_img_hash.c \
    $(MCUBOOT_DIR)/boot/bootutil/src/bootutil_img_security_cnt.c \
    $(MCUBOOT_DIR)/boot/bootutil/src/bootutil_loader.c \
    $(MCUBOOT_DIR)/boot/bootutil/src/bootutil_misc.c \
    $(MCUBOOT_DIR)/boot/bootutil/src/bootutil_public.c \
    $(MCUBOOT_DIR)/boot/bootutil/src/image_validate.c \
    $(MCUBOOT_DIR)/boot/bootutil/src/image_ecdsa.c \
    $(MCUBOOT_DIR)/boot/bootutil/src/loader.c \
    $(MCUBOOT_DIR)/boot/bootutil/src/caps.c \
    $(MCUBOOT_DIR)/boot/bootutil/src/tlv.c \
    $(MCUBOOT_DIR)/boot/bootutil/src/fault_injection_hardening.c \
    $(MCUBOOT_DIR)/boot/bootutil/src/fault_injection_hardening_delay_rng_mbedtls.c \
    $(MCUBOOT_DIR)/boot/bootutil/src/swap_misc.c \
    $(MCUBOOT_DIR)/boot/bootutil/src/swap_scratch.c \
    $(MCUBOOT_DIR)/boot/bootutil/src/encrypted.c

MCUBOOT_INCLUDES := \
    -I$(MCUBOOT_DIR)/STM32_Porting/inc \
    -I$(MCUBOOT_DIR)/boot/bootutil/include \
    -I$(MCUBOOT_DIR)/boot/bootutil/src

# ---- Nạp cấu hình riêng của MCUBoot ----
# Thay cho: include("${CMAKE_CURRENT_SOURCE_DIR}/mcuboot_config/MCUBoot_Config.cmake")
include $(MCUBOOT_DIR)/mcuboot_config/MCUBoot_Config.mk

# ---- Nếu sử dụng MBEDTLS thì import các thư viện mbedtls vào ----
ifeq ($(USE_MBEDTLS),ON)
    MCUBOOT_SRCS += \
        $(MCUBOOT_DIR)/ext/mbedtls/library/aes.c \
        $(MCUBOOT_DIR)/ext/mbedtls/library/asn1parse.c \
        $(MCUBOOT_DIR)/ext/mbedtls/library/asn1write.c \
        $(MCUBOOT_DIR)/ext/mbedtls/library/bignum.c \
        $(MCUBOOT_DIR)/ext/mbedtls/library/bignum_core.c \
        $(MCUBOOT_DIR)/ext/mbedtls/library/constant_time.c \
        $(MCUBOOT_DIR)/ext/mbedtls/library/ecdsa.c \
        $(MCUBOOT_DIR)/ext/mbedtls/library/ecp.c \
        $(MCUBOOT_DIR)/ext/mbedtls/library/ecp_curves.c \
        $(MCUBOOT_DIR)/ext/mbedtls/library/error.c \
        $(MCUBOOT_DIR)/ext/mbedtls/library/md.c \
        $(MCUBOOT_DIR)/ext/mbedtls/library/oid.c \
        $(MCUBOOT_DIR)/ext/mbedtls/library/pk.c \
        $(MCUBOOT_DIR)/ext/mbedtls/library/pkparse.c \
        $(MCUBOOT_DIR)/ext/mbedtls/library/platform.c \
        $(MCUBOOT_DIR)/ext/mbedtls/library/platform_util.c \
        $(MCUBOOT_DIR)/ext/mbedtls/library/sha256.c

    MCUBOOT_INCLUDES += -I$(MCUBOOT_DIR)/ext/mbedtls/include
endif

# ---- Nếu sử dụng tinycrypt thì import vào ----
# CMake gốc: print_error_message("TINYCRYPT not support") -> FATAL_ERROR.
# Đây là lỗi phát hiện lúc ĐỌC cấu hình (không phải lúc build), nên dùng
# thẳng $(error ...) của Make (xem giải thích trong common.mk) thay vì
# $(call print_error,...) — macro đó chỉ dùng được BÊN TRONG một recipe.
ifeq ($(USE_TINYCRYPT),ON)
    $(error TINYCRYPT not support)
endif

# ---- Liên kết tất cả các thư viện thuật toán con vào "Lib_MCUBoot" ----
# CMake gốc: target_link_libraries(Lib_MCUBoot PUBLIC Lib_Platform Lib_System_Startup)
# Trong mô hình Make đơn giản hoá ở tài liệu này (không đóng gói .a trung
# gian cho từng module — xem "Vì sao bỏ --whole-archive" trong Makefile
# gốc), MỌI module cuối cùng đều được liên kết chung vào 1 file .elf duy
# nhất, nên không cần khai báo "module nào phụ thuộc module nào" nữa.
# Nếu bạn triển khai theo hướng đóng gói .a riêng cho từng module (mục
# "nâng cao" trong tài liệu), đây là chỗ bạn sẽ thêm phụ thuộc liên kết.

# ---- target_compile_options(Lib_MCUBoot PRIVATE ...) ----
# CMake giới hạn các cờ "nới lỏng cảnh báo" này CHỈ cho các file thuộc
# target Lib_MCUBoot. Trong Make, kỹ thuật tương ứng là "pattern-specific
# variable": gắn thêm giá trị vào CFLAGS chỉ cho các .o nằm dưới đúng
# thư mục build tương ứng với module này — module khác không bị ảnh hưởng.
$(BUILD_DIR)/middlewares/MCUBoot/%.o: CFLAGS += \
    -Wno-error=implicit-function-declaration \
    -Wno-unused-variable \
    -Wno-sign-conversion \
    -Wno-sign-compare \
    -Wno-conversion \
    -include assert.h

SRCS     += $(MCUBOOT_SRCS)
INCLUDES += $(MCUBOOT_INCLUDES)
```

So sánh nhanh với CMakeLists.txt gốc để thấy sự tương ứng:

| CMakeLists.txt gốc | module.mk |
|---|---|
| `add_library(Lib_MCUBoot STATIC ${CMAKE_CURRENT_SOURCE_DIR}/STM32_Porting/src/app_dfu.c ...)` | `MCUBOOT_SRCS := $(MCUBOOT_DIR)/STM32_Porting/src/app_dfu.c \` |
| `include("${CMAKE_CURRENT_SOURCE_DIR}/mcuboot_config/MCUBoot_Config.cmake")` | `include $(MCUBOOT_DIR)/mcuboot_config/MCUBoot_Config.mk` |
| `if(USE_MBEDTLS STREQUAL "ON") target_sources(... PRIVATE ...) target_include_directories(... PUBLIC ...) endif()` | `ifeq ($(USE_MBEDTLS),ON) MCUBOOT_SRCS += ... MCUBOOT_INCLUDES += ... endif` |
| `if(USE_TINYCRYPT STREQUAL "ON") print_error_message("TINYCRYPT not support") endif()` | `ifeq ($(USE_TINYCRYPT),ON) $(error TINYCRYPT not support) endif` |
| `target_compile_options(Lib_MCUBoot PRIVATE -Wno-... -include assert.h)` | `$(BUILD_DIR)/middlewares/MCUBoot/%.o: CFLAGS += -Wno-... -include assert.h` |


### 4.3. Vì sao bỏ được `-Wl,--whole-archive` / `-Wl,--no-whole-archive`

Trong `CMakeLists.txt` gốc, mỗi module được `add_library(... STATIC ...)`
đóng gói thành 1 file `.a`, rồi lúc link, mỗi `.a` được bọc bởi
`-Wl,--whole-archive ... -Wl,--no-whole-archive`. Lý do CMake (hay đúng hơn,
lý do bạn) cần làm vậy: **trình liên kết (`ld`) chỉ kéo vào những file `.o`
bên trong 1 archive `.a` nếu file đó thực sự "giải quyết" một symbol đang bị
thiếu tại thời điểm nó quét tới archive đó** — nếu file `.o` không có symbol
nào được gọi trực tiếp (ví dụ: file chỉ chứa 1 hàm tự đăng ký ngắt, một
constructor có `__attribute__((constructor))`, một biến static được gom vào
section riêng bởi linker script...), `ld` sẽ **âm thầm bỏ qua** file `.o` đó —
đây là hành vi rất thường gặp gây lỗi khó hiểu trong firmware nhúng (ví dụ:
driver "biến mất" dù đã build, ISR không được cài đặt...). `--whole-archive`
ép `ld` phải lấy **toàn bộ** file `.o` trong archive, bất kể có symbol nào
được gọi hay không.

Trong bộ Makefile ở Phần 4, mình **không đóng gói `.a` trung gian cho từng
module** — mọi file `.c`/`.s` của mọi module được biên dịch thẳng ra `.o` rồi
đưa **toàn bộ** danh sách `.o` đó vào 1 lệnh link duy nhất
(`$(LINK_DRIVER) $(LDFLAGS) $(OBJS) -o $(TARGET_ELF)`). Khi bạn đưa thẳng file
`.o` (không phải nằm trong archive `.a`) vào dòng lệnh `ld`, **`ld` luôn lấy
toàn bộ**, không có khái niệm "chọn lọc" như với `.a` — nên vấn đề mà
`--whole-archive` giải quyết **không còn tồn tại**, và cờ đó trở nên thừa.

**Đánh đổi:** cách này đơn giản hơn (ít file trung gian, ít lệnh `ar`, không
cần nhớ thứ tự `--whole-archive`/`--no-whole-archive`), nhưng bạn sẽ không có
sẵn từng file `.a` riêng lẻ nếu sau này cần **phân phối** một module như một
thư viện độc lập (ví dụ đóng gói `Lib_MCUBoot.a` để dùng ở project khác). Nếu
cần điều đó, đây là cách thêm 1 bước đóng gói `.a` cho 1 module (ví dụ
`platform`), vẫn tương thích với toàn bộ phần còn lại:

```makefile
PLATFORM_OBJS := $(patsubst $(PROJECT_ROOT)/%.c,$(BUILD_DIR)/%.o,$(filter platform/%.c,$(SRCS)))

build/libPlatform.a: $(PLATFORM_OBJS)
	@mkdir -p $(dir $@)
	$(AR) rcs $@ $^

# Lúc link, bọc lại whole-archive giống bản CMake gốc nếu bạn quay về mô hình .a:
# $(LINK_DRIVER) $(LDFLAGS) -Wl,--whole-archive build/libPlatform.a -Wl,--no-whole-archive ... -o $(TARGET_ELF)
```

(`$(AR)` cần được định nghĩa thêm trong `toolchain.mk`, ví dụ
`AR := $(TOOLCHAIN_PREFIX)-ar`.)

### 4.4. Những gì đã được kiểm thử thật (để bạn yên tâm)

Trước khi gửi cho bạn, mình đã dựng một bản sao cấu trúc thư mục giống hệt
project của bạn (dùng đúng tên biến, đúng giá trị trong `build_config.cmake`,
đúng danh sách file của `Lib_MCUBoot`) và chạy thật bằng GNU Make 4.3 trên
Ubuntu 24.04. Vì không có sẵn toolchain `arm-none-eabi-gcc` trong môi trường
kiểm thử, mình kiểm chứng theo 2 lớp:

**Lớp 1 — `make -n` (dry-run) trên bản sao cấu trúc đầy đủ**, để xác nhận
Makefile đọc được, biến được tính đúng, và đúng lệnh sẽ chạy:

- Cấu hình mặc định (`USE_MCUBOOT=OFF`, `USE_APPLICATION=BootLoader_TinyCrypt_Test`
  giống hệt `build_config.cmake` bạn gửi) → build đúng 6 module, đúng macro
  `-D...` (đối chiếu tay từng cái với `build_config.cmake`, khớp 100%).
- `USE_MCUBOOT=ON` với `USE_TINYCRYPT=ON` (giữ nguyên) → **dừng đúng** với
  thông báo lỗi `TINYCRYPT not support`, giống hệt `print_error_message` gốc.
- `USE_MCUBOOT=ON USE_TINYCRYPT=OFF` → biên dịch đúng toàn bộ nguồn MCUBoot +
  mbedTLS, đúng include path, và **đúng cờ `-Wno-...` riêng** chỉ áp dụng cho
  các file trong `middlewares/MCUBoot/` — xác nhận file `platform/` khác
  **không** bị dính cờ này (không rò rỉ scope).
- `USE_APPLICATION=BootLoader_Test` → danh sách module giảm đúng (bỏ
  `devices/`, `services/`), và cờ `-u _printf_float` **bị loại đúng như
  logic gốc**.
- `BUILD_TYPE=Debug` → đúng `-O0 -g3` thay vì `-Os -g0`.
- `BUILD_TYPE=Turbo` (giá trị sai) → dừng đúng với thông báo lỗi rõ ràng.

**Lớp 2 — build thật bằng `gcc` (không phải bản ARM) trên 1 bộ 2-module thu
nhỏ**, để kiểm chứng đúng các *cơ chế* Make khó nhất mà `-n` không kiểm tra
được (vì `-n` không thực sự chạy lệnh):

- Sửa 1 file header dùng chung → **chỉ đúng** file `.c` include header đó
  được biên dịch lại ở lần `make` kế tiếp; file `.c` khác không đụng tới
  (xác nhận cơ chế `-MMD -MP` + `-include *.d` ở mục 2.12 hoạt động đúng).
- Thêm 1 file `.c` mới vào thư mục `src/` (không sửa `module.mk`) → file mới
  **tự động được phát hiện và biên dịch** ở lần `make` kế tiếp (xác nhận
  `$(shell find ...)` ở mục 2.6 hoạt động đúng).
- 1 module cố tình có biến không dùng tới (`-Wunused-variable` sẽ biến thành
  lỗi cứng vì bật `-Werror`) → biên dịch **thành công** nhờ target-specific
  variable thêm `-Wno-unused-variable`, trong khi module khác vẫn giữ nguyên
  `-Werror` đầy đủ (xác nhận cơ chế ở mục 2.11 hoạt động đúng và có phạm vi
  đúng).
- `make` (mặc định) chỉ in `[CC] file.c`; `make V=1` in đầy đủ dòng lệnh gcc.

**2 lỗi thật mình mắc phải và đã sửa trong lúc soạn** (kể ra để bạn hiểu rõ
hơn cái bẫy ở mục 2.10, vì đây là lỗi rất dễ lặp lại khi bạn tự viết thêm):
gọi `$(call print_complete,...)`/`$(call print_info,...)` (các macro **dạng
recipe**) ở ngay ngoài mọi target — để in banner "bắt đầu chạy" và banner chọn
hệ điều hành — gây lỗi `missing separator`. Sửa bằng cách đổi sang
`$(info ...)` như giải thích ở 2.10. Toàn bộ code trong Phần 4 đã là bản ĐÃ
SỬA, không còn lỗi này.

### 4.5. Những gì bạn cần tự bổ sung

Vì chỉ có 6 file bạn gửi (không có toàn bộ project), có vài chỗ mình phải suy
luận hợp lý hoặc để lại dạng khung — đã đánh dấu rõ bằng comment `⚠` trong
từng file, tóm tắt lại ở đây:

1. **`middlewares/MCUBoot/mcuboot_config/MCUBoot_Config.mk`** chỉ là bản mẫu
   tối thiểu (đoán `USE_MBEDTLS ?= ON` dựa trên cấu trúc code còn lại) — bạn
   chưa gửi `MCUBoot_Config.cmake` gốc, cần tự chuyển nốt.
2. **Danh sách `@RAM_ORIGIN@`, `@RAM_LENGTH@`...** trong rule sinh linker
   script — suy ra từ các biến `set(RAM_ORIGIN ...)` trong `build_config.cmake`,
   nhưng chưa đối chiếu được với nội dung thật của
   `STM32F446XX_LINKER_SCRIPT.ld.in` (bạn chưa gửi file này).
3. **`platform/`, `devices/`, `libraries/`, `services/`, `application/`,
   `system_startup/module.mk`** dùng gom nguồn tự động (`find`) vì không có
   danh sách file thật của các module này (khác `Lib_MCUBoot` — module duy
   nhất mình có đủ thông tin nên liệt kê tay 1:1).
4. **`LIB_ALGORITHMS_NAME`** xuất hiện trong `target_link_libraries(...)` ở
   `CMakeLists.txt` gốc bạn gửi, nhưng không được `set()` hay thêm vào
   `ADD_SUBDIRECTORY` ở bất kỳ file nào trong 6 file — có thể nằm ở 1 file cấu
   hình khác chưa gửi, hoặc là phần thừa từ lần refactor trước. Bộ Makefile
   này chưa tạo module `algorithms` vì thiếu thông tin.


---

<a name="5"></a>
## 5. Các lệnh build thường dùng

```bash
make                                  # build ở chế độ mặc định (Release)
make -j$(nproc)                       # build song song, dùng hết số lượng CPU core
make BUILD_TYPE=Debug                 # build bản Debug (-O0 -g3)
make USE_APPLICATION=BootLoader_Test  # build với cấu hình ứng dụng khác
make USE_MCUBOOT=ON USE_TINYCRYPT=OFF # bật MCUBoot + mbedTLS
make V=1                              # in đầy đủ dòng lệnh compiler/linker
make clean                            # xoá thư mục build/
make help                             # xem danh sách lệnh
make -n                               # "dry-run": chỉ in ra lệnh SẼ chạy, không build gì cả
```

Tương quan với CMake, để dễ hình dung:

| CMake | Make |
|---|---|
| `cmake -B build -DCMAKE_BUILD_TYPE=Debug` rồi `cmake --build build` | `make BUILD_TYPE=Debug` (gộp làm 1 bước — Make không có bước "configure" riêng) |
| `cmake --build build -j8` | `make -j8` |
| `cmake --build build --target clean` | `make clean` |
| `cmake --build build --verbose` | `make V=1` |
| `ccmake` / xem lại các option đã set | `make help` hoặc mở `config.mk` |

---

<a name="6"></a>
## 6. Lỗi thường gặp và cách debug

| Thông báo lỗi | Nguyên nhân thường gặp | Cách sửa |
|---|---|---|
| `Makefile:N: *** missing separator.  Stop.` | Dòng N là recipe nhưng thụt đầu dòng bằng dấu cách thay vì Tab — **hoặc** bạn gọi 1 macro `define` dạng-recipe (như `print_info`) ở ngoài mọi target (xem 2.10) | Kiểm tra `cat -A Makefile \| sed -n 'Np'` xem có `^I` (tab) ở đầu dòng không; nếu là lỗi macro, đổi sang `$(info ...)` |
| `No rule to make target 'x.h', needed by 'x.o'` | File `x.h` từng tồn tại (đã có trong 1 file `.d` cũ) nhưng vừa bị xoá/đổi tên | Thường tự hết sau khi `make clean` rồi build lại; nếu không thì kiểm tra đã có `-MP` trong `DEPFLAGS` chưa (xem 2.12) |
| Sửa `.h` xong nhưng build không thấy gì thay đổi | Thiếu `-MMD -MP` hoặc thiếu dòng `-include $(DEPS)` | Xem lại 2.12 — đây là phần CMake tự làm hộ mà Make thì không |
| `arm-none-eabi-gcc: not found` / lỗi tương tự | Đường dẫn `TOOLCHAIN_PREFIX` trong `toolchain.mk` không khớp nơi bạn cài toolchain thật | Sửa lại 2 nhánh `TOOLCHAIN_PREFIX` trong `toolchain.mk` cho khớp đường dẫn cài đặt thật |
| Cờ cảnh báo bạn thêm cho 1 module lại ảnh hưởng module khác | Sửa nhầm vào biến `CFLAGS` toàn cục thay vì dùng target-specific variable | Dùng đúng cú pháp `$(BUILD_DIR)/duong-dan/module/%.o: CFLAGS += ...` (xem 2.11) |
| Build lại toàn bộ project dù chỉ sửa 1 file | `SRCS`/`INCLUDES` bị gán lại bằng `:=` ở đâu đó (ghi đè thay vì `+=`), hoặc file `.d` bị `clean` nhưng `.o` thì không (thư mục build lẫn lộn) | Kiểm tra không có `module.mk` nào dùng `:=` cho `SRCS`/`INCLUDES`; `make clean` xoá sạch `build/` trước khi build lại nếu nghi ngờ |
| Muốn xem chính xác Make đang hiểu giá trị 1 biến là gì | — | Thêm dòng tạm `$(info DEBUG SRCS = $(SRCS))` ngay dưới chỗ nghi ngờ, hoặc chạy `make -p \| less` để Make in toàn bộ database biến + rule nó đang có |
| Muốn biết Make quyết định build lại 1 target vì lý do gì | — | Chạy `make --trace` hoặc `make -d` (rất dài dòng, nên lọc bằng `grep`) |

---

<a name="7"></a>
## 7. Bảng tra nhanh (cheat sheet)

```makefile
# ---- Biến ----
X := gia_tri            # tính ngay
X = $(Y)                 # tính mỗi lần dùng (Y có thể định nghĩa sau)
X ?= gia_tri             # chỉ gán nếu X chưa có (ghi đè được bằng: make X=...)
X += them_gia_tri        # nối vào cuối

# ---- Rule ----
target: prereq1 prereq2
	lenh_shell   # PHẢI là 1 ký tự TAB, không phải dấu cách

# ---- Pattern rule ----
build/%.o: %.c
	$(CC) -c $< -o $@

# ---- Automatic variables (dùng trong recipe) ----
$@   # tên target
$<   # prerequisite đầu tiên
$^   # tất cả prerequisites
$(@D) $(@F)   # thư mục / tên file của $@

# ---- Hàm hay dùng ----
$(wildcard *.c)                  # glob 1 cấp, không đệ quy
$(shell find . -name '*.c')      # glob đệ quy (nhờ shell)
$(patsubst %.c,%.o,$(X))         # đổi hậu tố
$(filter %.c,$(X))               # lọc theo mẫu
$(call macro,a,b)                # gọi define/endef (CHỈ trong recipe)
$(error ...) $(warning ...) $(info ...)   # dừng/cảnh báo/in — dùng ở MỌI NƠI (không cần trong recipe)

# ---- Điều kiện ----
ifeq ($(X),gia_tri)
    ...
else
    ...
endif

# ---- define/endef ----
define ten_macro
	@echo "$(1)"
endef
# gọi: $(call ten_macro,xin_chao)   -- CHỈ hợp lệ trong 1 recipe

# ---- Target/pattern-specific variable ----
build/mod/%.o: CFLAGS += -Wno-unused-variable

# ---- Dependency tự động theo header ----
CC_RECIPE = $(CC) $(CFLAGS) -MMD -MP -MF $(@:.o=.d) -c $< -o $@
-include $(OBJS:.o=.d)

# ---- Phony target ----
.PHONY: all clean help

# ---- include ----
include file.mk     # lỗi nếu file không tồn tại
-include file.mk    # không lỗi nếu file không tồn tại (dùng cho file .d)
```

---

## Tệp đính kèm

- **`mycproject_make_example.zip`** — toàn bộ 14 file `.mk`/`Makefile` đã
  soạn và kiểm thử ở Phần 4, sẵn sàng copy vào project thật (kèm
  `README.md` tóm tắt lại đúng những gì cần chỉnh sửa ở mục 4.5).
