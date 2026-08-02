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
