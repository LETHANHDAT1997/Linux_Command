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
