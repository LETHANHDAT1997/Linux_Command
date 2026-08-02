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
