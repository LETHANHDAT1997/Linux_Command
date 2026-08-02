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
MIN_STACK_SIZE             ?= 0x800
FLASH_ORIGIN               ?= 0x08000000
FLASH_LENGTH                ?= 128K - 0x200

# ──────────────────── Devices ────────────────────────────────────────────
LIB_DEVICES_NAME ?= Lib_Devices
LIB_DEVICES_DIR  ?= devices
USE_DHT22         ?= OFF
USE_HMC5883L      ?= OFF
USE_MPU6050       ?= OFF
USE_QMC5883L      ?= OFF
USE_RC522         ?= OFF
USE_RF24L01       ?= OFF
USE_SDCARD_DEV    ?= ON
USE_USB_HOST_DEV  ?= OFF

# ──────────────────── Middlewares ────────────────────────────────────────
LIB_MIDDLEWARES_NAME ?= Lib_Middleware
LIB_MIDDLEWARES_DIR  ?= middlewares
USE_MCUBOOT           ?= OFF
USE_TINYCRYPT          ?= ON
USE_FATFS               ?= ON
USE_MICRORL              ?= ON

# ──────────────────── Services ───────────────────────────────────────────
LIB_SERVICES_NAME ?= Lib_Services
LIB_SERVICES_DIR  ?= services
USE_ORIENTATION_SENSOR ?= OFF
USE_RADIO                ?= OFF
USE_RFID                  ?= OFF
USE_SD_CARD                ?= ON
USE_USB_DRIVE                ?= OFF
USE_SHA256_ECDSA_P256_AES_128_CCM ?= ON

# ──────────────────── Libraries ──────────────────────────────────────────
LIB_LIBRARIES_NAME ?= Lib_Libraries
LIB_LIBRARIES_DIR  ?= libraries
USE_KALMAN_FILTER   ?= OFF
USE_MADGWICK_FILTER  ?= OFF
USE_PID                ?= OFF
USE_MATH_ULTILITY        ?= ON
USE_RING_BUFFER            ?= ON
USE_ELAPSED_TIME              ?= OFF
