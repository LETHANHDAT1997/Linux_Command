# Manjaro Utilities

Manjaro sử sụng kiến trúc ArchLinux nên sử dụng trình quản lí gói 'pacman'.
## Cài đặt thư viện thiết yếu như sau

- `sudo pacman -Syu`: Cập nhật toàn hệ thống (update + upgrade + full-upgrade)
- `sudo pacman -S base-devel`: build-essential tương đương (gồm gcc, g++, make, ...)
- `sudo pacman -S pkgconf`: Thay cho pkg-config
- `sudo pacman -S ninja`: Trình build song song
- `sudo pacman -S meson`: Hệ thống build hiện đại
- `sudo pacman -S cmake`: Trình tạo build files CMake
- `sudo pacman -S python`: Cài đặt python
- `sudo pacman -S python-pip python-setuptools python-virtualenv`: Cài đặt công cụ bổ sung python

## Lưu ý về Python
Một sồ thư viện trên python không thể sử dụng 'pip' để cài đặt, do Manjaro (Arch Linux) đã bật cơ chế bảo vệ môi trường hệ thống theo PEP 668. Điều này ngăn cài đặt gói bằng pip vào hệ thống để tránh xung đột với pacman.

### Cách 1: Tạo môi trường ảo (an toàn, được khuyến nghị nhất)
- `python -m venv myenv`: Tạo môi trường
- `source myenv/bin/activate`: source
- `pip install openpyxl`: install lib openpyxl

### Cách 2: Cài qua pacman (dành cho hệ thống)
- `sudo pacman -S python-openpyxl`: do Arch duy trì. Nếu bạn không cần phiên bản mới nhất thì cài cách này là nhanh nhất.


