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
