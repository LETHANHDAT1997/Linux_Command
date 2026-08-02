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
