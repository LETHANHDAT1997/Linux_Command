### 1. Ubuntu 25.04 (bản mới nhất)

#### **Cài đặt gói ngôn ngữ tiếng Việt (giao diện đồ họa)**
1. Mở ứng dụng **Settings** (Cài đặt) từ menu hệ thống hoặc tìm kiếm "Settings".
2. Chuyển đến mục **Region & Language** (Khu vực & Ngôn ngữ).
3. Nhấn **Manage Installed Languages** (Quản lý ngôn ngữ đã cài đặt).
4. Trong cửa sổ **Language Support**, nhấn **Install/Remove Languages**.
5. Tìm **Vietnamese (Tiếng Việt)** trong danh sách, đánh dấu chọn và nhấn **Apply**.
6. Hệ thống sẽ yêu cầu mật khẩu và tự động tải các gói ngôn ngữ cần thiết.
7. Sau khi cài đặt, kéo **Vietnamese** lên đầu danh sách trong **Language Support** để đặt làm ngôn ngữ mặc định.
8. Đăng xuất và đăng nhập lại để giao diện chuyển sang tiếng Việt.

#### **Cài đặt bộ gõ bàn phím tiếng Việt (giao diện đồ họa)**
1. Trong **Settings > Region & Language**, tìm mục **Input Sources** (Nguồn đầu vào).
2. Nhấn dấu "+" để thêm nguồn đầu vào.
3. Tìm và chọn **Vietnamese (Unikey)**. Nếu không thấy, chọn **Vietnamese** và chọn kiểu gõ (Telex hoặc VNI).
4. Nhấn **Add** để thêm.
5. Sử dụng phím tắt `Super + Space` để chuyển đổi giữa các bàn phím (có thể tùy chỉnh trong Settings).
6. Để cấu hình Unikey (ví dụ, chọn Telex/VNI):
   - Nhấn biểu tượng bàn phím trên thanh trạng thái (góc trên bên phải).
   - Chọn **Setup** hoặc **Configure** trong menu ibus, sau đó điều chỉnh kiểu gõ.

#### **Cài đặt font chữ hỗ trợ tiếng Việt (giao diện đồ họa)**
- Font mặc định của Ubuntu (như DejaVu, Ubuntu Font) đã hỗ trợ tiếng Việt. Nếu cần thêm font:
  1. Tải font hỗ trợ tiếng Việt (như VnTime, Arial Unicode MS) từ nguồn đáng tin cậy.
  2. Mở file font (.ttf) bằng ứng dụng **Font Viewer** (có sẵn trong Ubuntu).
  3. Nhấn **Install** để cài đặt font cho người dùng hiện tại.

#### **Bổ sung qua dòng lệnh (nếu cần)**
- Cài gói ngôn ngữ:
  ```
  sudo apt update
  sudo apt install language-pack-vi language-pack-gnome-vi
  sudo locale-gen vi_VN vi_VN.UTF-8
  sudo update-locale LC_ALL=vi_VN.UTF-8
  ```
- Cài bộ gõ:
  ```
  sudo apt install ibus ibus-unikey
  ibus restart
  ```
- Cài font:
  ```
  sudo apt install fonts-dejavu fonts-liberation
  fc-cache -fv
  ```

---

### 2. Manjaro 25.0 "Zetar" (bản mới nhất)

Manjaro sử dụng các môi trường như KDE, GNOME, hoặc XFCE. Dưới đây là hướng dẫn cho giao diện đồ họa, tập trung vào KDE (phổ biến nhất trên Manjaro).

#### **Cài đặt gói ngôn ngữ tiếng Việt (giao diện đồ họa)**
1. Mở **System Settings** (Cài đặt hệ thống) từ menu hoặc tìm kiếm.
2. Chuyển đến **Regional Settings > Translations**.
3. Nhấn **Add Languages** (Thêm ngôn ngữ).
4. Tìm **Vietnamese** (Tiếng Việt), chọn và nhấn **Apply**.
5. Hệ thống sẽ tải gói ngôn ngữ qua trình quản lý gói Pamac (nếu không tự động, xem bước dòng lệnh).
6. Trong **Regional Settings > Formats**, chọn **Vietnamese** làm định dạng mặc định.
7. Đăng xuất và đăng nhập lại để áp dụng giao diện tiếng Việt.

#### **Cài đặt bộ gõ bàn phím tiếng Việt (giao diện đồ họa)**
1. Mở **System Settings > Input Devices > Keyboard**.
2. Trong tab **Layouts**, bật tùy chọn **Configure layouts**.
3. Nhấn **Add** và tìm **Vietnamese (Unikey)** hoặc **Vietnamese**.
4. Nếu không thấy Unikey, bạn cần cài ibus-unikey (xem bước dòng lệnh).
5. Chuyển đổi bàn phím bằng phím tắt `Alt + Shift` hoặc tùy chỉnh trong **Layouts**.
6. Để cấu hình Unikey, nhấp vào biểu tượng ibus trên thanh trạng thái (góc trên bên phải), chọn **Input Method > Vietnamese (Unikey)** và điều chỉnh kiểu gõ (Telex/VNI).

#### **Cài đặt font chữ hỗ trợ tiếng Việt (giao diện đồ họa)**
1. Mở **System Settings > Appearance > Fonts**.
2. Nhấn **Add Fonts** và chọn file font (.ttf) từ nguồn đáng tin cậy (như Noto Sans, DejaVu).
3. Hoặc sử dụng **Pamac Software Manager**:
   - Tìm kiếm `ttf-dejavu` hoặc `noto-fonts`.
   - Cài đặt bằng cách nhấn **Install**.

#### **Bổ sung qua dòng lệnh (nếu cần)**
- Cài gói ngôn ngữ:
  ```
  sudo pacman -Syu
  sudo nano /etc/locale.gen  # Bỏ # trước vi_VN.UTF-8
  sudo locale-gen
  sudo localectl set-locale LANG=vi_VN.UTF-8
  ```
- Cài bộ gõ:
  ```
  sudo pacman -S ibus ibus-unikey
  ibus-daemon -drx
  ```
- Cài font:
  ```
  sudo pacman -S ttf-dejavu noto-fonts
  fc-cache -fv
  ```

---

### 3. Raspberry Pi OS (dựa trên Debian 12 Bookworm, bản 5/2025)

Raspberry Pi OS sử dụng môi trường LXDE, tối ưu cho tài nguyên thấp.

#### **Cài đặt gói ngôn ngữ tiếng Việt (giao diện đồ họa)**
1. Mở **Raspberry Pi Configuration** từ menu **Preferences**.
2. Chuyển đến tab **Localisation**.
3. Nhấn **Set Locale**.
4. Chọn:
   - **Language**: Vietnamese
   - **Country**: Vietnam
   - **Character Set**: UTF-8
5. Nhấn **OK** và đợi hệ thống cập nhật.
6. Khởi động lại để áp dụng giao diện tiếng Việt.

#### **Cài đặt bộ gõ bàn phím tiếng Việt (giao diện đồ họa)**
1. Mở **Preferences > Keyboard and Mouse > Keyboard**.
2. Trong tab **Keyboard Layout**, nhấn **Add**.
3. Tìm **Vietnamese (Unikey)**. Nếu không thấy, cần cài ibus-unikey qua dòng lệnh.
4. Chuyển đổi bàn phím bằng biểu tượng ibus trên thanh trạng thái (góc trên bên phải).
5. Cấu hình Unikey: Nhấn chuột phải vào biểu tượng ibus, chọn **Setup**, thêm **Vietnamese (Unikey)** và chọn kiểu gõ.

#### **Cài đặt font chữ hỗ trợ tiếng Việt (giao diện đồ họa)**
1. Tải file font (.ttf) như DejaVu hoặc Noto từ nguồn đáng tin cậy.
2. Mở file bằng **File Manager**, nhấp đúp và chọn **Install** trong **Font Viewer**.
3. Hoặc dùng **Software Manager** (trong menu Preferences) để tìm và cài đặt `fonts-dejavu`.

#### **Bổ sung qua dòng lệnh (nếu cần)**
- Cài gói ngôn ngữ:
  ```
  sudo apt update
  sudo apt install language-pack-vi
  sudo locale-gen vi_VN vi_VN.UTF-8
  sudo update-locale LC_ALL=vi_VN.UTF-8
  ```
- Cài bộ gõ:
  ```
  sudo apt install ibus ibus-unikey
  ibus restart
  ```
- Cài font:
  ```
  sudo apt install fonts-dejavu
  fc-cache -fv
  ```

---

### 4. Linux Mint 22.2 (bản mới nhất)

Linux Mint 22.2 sử dụng môi trường Cinnamon, rất thân thiện với người dùng.

#### **Cài đặt gói ngôn ngữ tiếng Việt (giao diện đồ họa)**
1. Mở **Menu > Preferences > Languages**.
2. Nhấn **Install/Remove Languages**.
3. Tìm **Vietnamese (Tiếng Việt)**, đánh dấu chọn và nhấn **Apply**.
4. Sau khi cài đặt, kéo **Vietnamese** lên đầu danh sách trong **Languages** để đặt làm mặc định.
5. Đăng xuất và đăng nhập lại để áp dụng giao diện tiếng Việt.

#### **Cài đặt bộ gõ bàn phím tiếng Việt (giao diện đồ họa)**
1. Mở **Menu > Preferences > Input Method** (hoặc **Keyboard > Input Sources**).
2. Nhấn dấu "+" để thêm nguồn đầu vào.
3. Tìm **Vietnamese (Unikey)** và thêm vào.
4. Nếu không thấy Unikey, cài ibus-unikey qua dòng lệnh.
5. Chuyển đổi bàn phím bằng phím tắt `Super + Space`.
6. Cấu hình Unikey: Nhấn biểu tượng bàn phím trên thanh trạng thái, chọn **Setup**, điều chỉnh kiểu gõ (Telex/VNI).

#### **Cài đặt font chữ hỗ trợ tiếng Việt (giao diện đồ họa)**
1. Mở **Menu > Preferences > Fonts**.
2. Nhấn **Add Fonts** và chọn file font (.ttf) từ nguồn đáng tin cậy.
3. Hoặc dùng **Software Manager**:
   - Tìm `fonts-dejavu` hoặc `fonts-liberation`.
   - Nhấn **Install**.

#### **Bổ sung qua dòng lệnh (nếu cần)**
- Cài gói ngôn ngữ:
  ```
  sudo apt update
  sudo apt install language-pack-vi language-pack-gnome-vi
  sudo locale-gen vi_VN vi_VN.UTF-8
  sudo update-locale LC_ALL=vi_VN.UTF-8
  ```
- Cài bộ gõ:
  ```
  sudo apt install ibus ibus-unikey
  ibus restart
  ```
- Cài font:
  ```
  sudo apt install fonts-dejavu fonts-liberation
  fc-cache -fv
  ```

---

### Lưu ý chung
- **Kiểm tra cài đặt**: Nếu giao diện không chuyển sang tiếng Việt, kiểm tra xem gói `language-pack-gnome-vi` (Ubuntu/Mint) hoặc tương đương đã cài chưa.
- **Phím tắt**: Đảm bảo phím tắt chuyển đổi bàn phím (thường là `Super + Space`) không xung đột với ứng dụng khác.
- **Font chữ**: Các font như DejaVu, Noto Sans, hoặc Liberation đều hỗ trợ tốt tiếng Việt. Chỉ cần thêm font nếu bạn muốn kiểu chữ đặc biệt.
- **Khởi động lại**: Một số thay đổi (như locale) yêu cầu đăng xuất hoặc khởi động lại hệ thống.
- **Nếu gặp lỗi**: Kiểm tra log bằng lệnh `journalctl -xe` hoặc đảm bảo hệ thống đã được cập nhật đầy đủ.

Nếu bạn cần thêm chi tiết hoặc gặp vấn đề cụ thể trên một bản phân phối, hãy cho tôi biết!