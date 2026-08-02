# ============================================================================
#  module.mk cho thư mục "system_startup"
#  Thay thế cho: system_startup/CMakeLists.txt
#  (add_library(Lib_System_Startup STATIC ...) tương ứng)
#
#  Lưu ý: file linker script STM32F446XX_LINKER_SCRIPT.ld.in KHÔNG được
#  liệt vào đây — nó được xử lý riêng bằng rule sinh $(LDSCRIPT) ở Makefile
#  gốc (tương đương configure_file(...) trong gcc-arm-none-eabi.cmake),
#  vì đó là một bước "sinh file" (code generation), không phải "biên dịch".
# ============================================================================
MODULE_DIR := $(PROJECT_ROOT)/system_startup

SRCS     += $(shell find $(MODULE_DIR)/src -name '*.c' 2>/dev/null)
SRCS     += $(shell find $(MODULE_DIR)/src -name '*.cpp' 2>/dev/null)
SRCS     += $(shell find $(MODULE_DIR)/src -name '*.s' -o -name '*.S' 2>/dev/null)
INCLUDES += -I$(MODULE_DIR)/inc
