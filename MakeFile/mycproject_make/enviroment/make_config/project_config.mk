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
# 3) System Startup layer
# ---------------------------------------------------------------------------
ifeq ($(USE_SERIAL_LOG),ON)
    CPPFLAGS += -DSERIAL_LOG
endif

# ---------------------------------------------------------------------------
# 4) Devices layer
# ---------------------------------------------------------------------------
ifeq ($(USE_DHT22),ON)
    CPPFLAGS += -DUSE_DHT22
endif
ifeq ($(USE_HMC5883L),ON)
    CPPFLAGS += -DUSE_HMC5883L
endif
ifeq ($(USE_MPU6050),ON)
    CPPFLAGS += -DUSE_MPU6050
endif
ifeq ($(USE_QMC5883L),ON)
    CPPFLAGS += -DUSE_QMC5883L
endif
ifeq ($(USE_RC522),ON)
    CPPFLAGS += -DUSE_RC522
endif
ifeq ($(USE_RF24L01),ON)
    CPPFLAGS += -DUSE_RF24L01
endif
ifeq ($(USE_SDCARD_DEV),ON)
    CPPFLAGS += -DUSE_SDCARD_DEV
endif
ifeq ($(USE_USB_HOST_DEV),ON)
    CPPFLAGS += -DUSE_USB_HOST_DEV
endif

# ---------------------------------------------------------------------------
# 5) Middlewares layer
# ---------------------------------------------------------------------------
ifeq ($(USE_MCUBOOT),ON)
    CPPFLAGS += -DUSE_MCUBOOT
endif
ifeq ($(USE_TINYCRYPT),ON)
    CPPFLAGS += -DUSE_TINYCRYPT
endif
ifeq ($(USE_FATFS),ON)
    CPPFLAGS += -DUSE_FATFS
endif
ifeq ($(USE_MICRORL),ON)
    CPPFLAGS += -DUSE_MICRORL
endif

# ---------------------------------------------------------------------------
# 6) Services layer
# ---------------------------------------------------------------------------
ifeq ($(USE_ORIENTATION_SENSOR),ON)
    CPPFLAGS += -DUSE_ORIENTATION_SENSOR
endif
ifeq ($(USE_RADIO),ON)
    CPPFLAGS += -DUSE_RADIO
endif
ifeq ($(USE_RFID),ON)
    CPPFLAGS += -DUSE_RFID
endif
ifeq ($(USE_SD_CARD),ON)
    CPPFLAGS += -DUSE_SD_CARD
endif
ifeq ($(USE_USB_DRIVE),ON)
    CPPFLAGS += -DUSE_USB_DRIVE
endif
ifeq ($(USE_SHA256_ECDSA_P256_AES_128_CCM),ON)
    CPPFLAGS += -DUSE_SHA256_ECDSA_P256_AES_128_CCM
endif

# ---------------------------------------------------------------------------
# 7) Libraries layer
# ---------------------------------------------------------------------------
ifeq ($(USE_KALMAN_FILTER),ON)
    CPPFLAGS += -DUSE_KALMAN_FILTER
endif
ifeq ($(USE_MADGWICK_FILTER),ON)
    CPPFLAGS += -DUSE_MADGWICK_FILTER
endif
ifeq ($(USE_PID),ON)
    CPPFLAGS += -DUSE_PID
endif
ifeq ($(USE_MATH_ULTILITY),ON)
    CPPFLAGS += -DUSE_MATH_ULTILITY
endif
ifeq ($(USE_RING_BUFFER),ON)
    CPPFLAGS += -DUSE_RING_BUFFER
endif
ifeq ($(USE_ELAPSED_TIME),ON)
    CPPFLAGS += -DUSE_ELAPSED_TIME
endif

# ---------------------------------------------------------------------------
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
