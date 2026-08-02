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
