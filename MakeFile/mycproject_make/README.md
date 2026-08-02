# Bộ khung Makefile mẫu cho MyCProject

Bộ file này là bản chuyển đổi trực tiếp từ 5 file CMake bạn đã gửi
(`CMakeLists.txt` gốc, `cmake_init.cmake`, `project_config.cmake`,
`build_config.cmake`, `gcc-arm-none-eabi.cmake`, và
`middlewares/MCUBoot/CMakeLists.txt`). Đã được kiểm thử bằng `make -n`
(dry-run) với nhiều tổ hợp cấu hình (mặc định, `USE_APPLICATION=BootLoader_Test`,
`BUILD_TYPE=Debug`, bật/tắt `USE_MCUBOOT`...) và một bộ test cơ chế Make
riêng (dependency theo header, phát hiện file mới, cờ biên dịch riêng theo
module) bằng gcc thật — xem giải thích chi tiết trong tài liệu hướng dẫn
đi kèm (`Huong_Dan_Su_Dung_Makefile.md`).

## Cách dùng

1. Copy toàn bộ các file/thư mục trong bộ này (giữ nguyên đường dẫn
   tương đối) vào **gốc dự án thật** của bạn — nơi đang chứa `CMakeLists.txt`.
   Không có file `.c`/`.h`/`.s` nào trong bộ này — mã nguồn thật của bạn
   đã có sẵn, cứ để nguyên vị trí.
2. Mở `enviroment/make_config/config.mk`, đối chiếu lại với
   `build_config.cmake` thật của bạn (đề phòng bạn đã sửa gì đó sau khi
   gửi file cho mình) và với biến `USE_MBEDTLS` chưa rõ giá trị (xem mục 4).
3. Chạy thử trước khi build thật:
   ```
   make -n
   ```
   Lệnh này chỉ IN RA các lệnh biên dịch/liên kết SẼ chạy, không build gì
   cả — dùng để soát lỗi đường dẫn/thiếu file trước.
4. Build thật:
   ```
   make -j$(nproc)
   ```

## Những chỗ CẦN BẠN ĐIỀU CHỈNH (vì file gốc bạn gửi chưa có đủ thông tin)

- **`middlewares/MCUBoot/mcuboot_config/MCUBoot_Config.mk`** — đây chỉ là
  bản mẫu tối thiểu (`USE_MBEDTLS ?= ON`). Bạn chưa gửi
  `MCUBoot_Config.cmake` gốc, hãy mở file đó và chuyển nốt các
  `set(... ON/OFF)` sang cú pháp `.mk` giống các file khác trong bộ này.
- **Danh sách `@...@` trong rule sinh linker script** (trong `Makefile`,
  đoạn `sed -e 's/@RAM_ORIGIN@/.../g' ...`) — mình suy ra 6 biến này từ
  các `set(RAM_ORIGIN ...)` v.v. trong `build_config.cmake`, nhưng chưa
  thấy nội dung thật của `STM32F446XX_LINKER_SCRIPT.ld.in`. Hãy mở file
  `.ld.in` thật và đối chiếu: nếu nó dùng thêm placeholder nào khác (hoặc
  ít hơn), sửa lại danh sách `-e 's/@.../.../g'` cho khớp.
- **`platform/module.mk`, `devices/module.mk`, `libraries/module.mk`,
  `services/module.mk`, `application/module.mk`, `system_startup/module.mk`,
  `middlewares/module.mk`** — các file này dùng cách gom nguồn tự động
  (`find ... -name '*.c'`) vì mình không có danh sách file thật của các
  module này (khác với `Lib_MCUBoot` — module đó mình có đủ danh sách nên
  đã liệt kê tay 1:1 giống CMakeLists.txt gốc). Nếu cấu trúc thư mục
  `src/`, `inc/` thật của bạn khác (ví dụ nhiều cấp thư mục con, hoặc tên
  khác), sửa lại đường dẫn trong từng `module.mk` cho khớp.
- **`LIB_ALGORITHMS_NAME`** — file `CMakeLists.txt` gốc bạn gửi có link tới
  thư viện này (`target_link_libraries(... ${LIB_ALGORITHMS_NAME} ...)`)
  nhưng mình không thấy nó được `set()` hay thêm vào `ADD_SUBDIRECTORY` ở
  bất kỳ file nào bạn gửi — có thể nó nằm ở một file cấu hình khác, hoặc
  là phần thừa từ một lần refactor trước. Bộ Makefile này **không** tạo
  module `algorithms` vì không có thông tin; nếu dự án bạn thật sự có thư
  mục đó, thêm một `module.mk` tương tự `platform/module.mk` cho nó rồi
  thêm `algorithms` vào biến `MODULES` trong `project_config.mk`.

## Danh sách file trong bộ này và file CMake tương ứng

| File Makefile                                              | Thay thế cho                                             |
| ------------------------------------------------------------ | ------------------------------------------------------- |
| `Makefile`                                                    | `CMakeLists.txt` (gốc)                                   |
| `enviroment/make_config/common.mk`                            | `enviroment/cmake_config/cmake_init.cmake`                |
| `enviroment/make_config/config.mk`                             | `enviroment/cmake_config/build_config.cmake`               |
| `enviroment/make_config/project_config.mk`                     | `enviroment/cmake_config/project_config.cmake`              |
| `enviroment/make_config/toolchain.mk`                          | `enviroment/cmake_config/gcc-arm-none-eabi.cmake`            |
| `platform/module.mk` (và tương tự cho các thư mục khác)         | `<thư mục>/CMakeLists.txt` (`add_library` cho từng module)    |
| `middlewares/MCUBoot/module.mk`                                | `middlewares/MCUBoot/CMakeLists.txt`                        |
| `middlewares/MCUBoot/mcuboot_config/MCUBoot_Config.mk`          | `middlewares/MCUBoot/mcuboot_config/MCUBoot_Config.cmake` (⚠ bản mẫu) |

Giải thích đầy đủ TẠI SAO từng dòng được viết như vậy — xem tài liệu
`Huong_Dan_Su_Dung_Makefile.md` đi kèm.
