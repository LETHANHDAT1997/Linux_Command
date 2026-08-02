# ============================================================================
#  MCUBoot_Config.mk
#  Thay thế cho: middlewares/MCUBoot/mcuboot_config/MCUBoot_Config.cmake
#
#  ⚠ Bạn chưa gửi cho mình nội dung file MCUBoot_Config.cmake gốc, nên đây
#  chỉ là bản MẪU TỐI THIỂU để toàn bộ hệ thống build/test chạy được.
#  Hãy mở file .cmake gốc và chuyển từng "set(... ON/OFF)" hoặc "if(...)"
#  sang đúng cú pháp Make như các file .mk khác trong tài liệu này
#  (xem Phần 3 – Bảng đối chiếu CMake ⇄ Makefile).
# ============================================================================

# Suy ra từ việc CMakeLists.txt gốc có nhánh xử lý mbedTLS đầy đủ và nhánh
# TinyCrypt bị đánh dấu "not support" (comment hết source) — nên mặc định
# đoán USE_MBEDTLS=ON. Hãy sửa lại theo đúng giá trị thật trong project của bạn.
USE_MBEDTLS ?= ON
