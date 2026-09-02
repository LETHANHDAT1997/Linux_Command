# Sổ Tay Chuyên Sâu: Git Rebase, Cherry-Pick & Xử Lý Conflict

Tài liệu tổng hợp chi tiết các khái niệm cốt lõi, cơ chế hoạt động bên dưới, cách xử lý sự cố thực tế với Git Rebase, Cherry-Pick và kỹ thuật Squash commit.

---

## Mục Lục
1. [Bản Chất Của Một Commit, Rebase & Cherry-Pick](#1-bản-chất-của-một-commit-rebase--cherry-pick)
   - [Một Commit thực sự chứa những gì?](#một-commit-thực-sự-chứa-những-gì)
   - ["Lấy" nội dung khi Cherry-Pick/Rebase là lấy cái gì?](#lấy-nội-dung-khi-cherry-pickrebase-là-lấy-cái-gì)
   - [So sánh trực quan: `git rebase` vs `git cherry-pick`](#so-sánh-trực-quan-git-rebase-vs-git-cherry-pick)
2. [Giá Trị Của "Lịch Sử Thẳng" (Linear History)](#2-giá-trị-của-lịch-sử-thẳng-linear-history)
3. [Xử Lý Sự Cố & "Địa Ngục Conflict" Khi Rebase (Rebase Hell)](#3-xử-lý-sự-cố--địa-ngục-conflict-khi-rebase-rebase-hell)
   - [Nhận diện commit đang bị conflict](#nhận-diện-commit-đang-bị-conflict)
   - [Lỗi commit rỗng / trùng lặp sau khi sửa conflict](#lỗi-commit-rỗng--trùng-lặp-sau-khi-sửa-conflict)
   - [3 Vũ khí xử lý xung đột dây chuyền](#3-vũ-khí-xử-lý-xung-đột-dây-chuyền)
   - [Gộp commit (Squash): Điều gì xảy ra với Code & Metadata?](#gộp-commit-squash-điều-gì-xảy-ra-với-code--metadata)

---

## 1. Bản Chất Của Một Commit, Rebase & Cherry-Pick

### Một Commit thực sự chứa những gì?
Một commit trong Git không đơn thuần là một đoạn text, mà là một **gói dữ liệu hoàn chỉnh** bao gồm:
1. **Tree Object / Snapshot:** Cây thư mục và nội dung toàn bộ file tại thời điểm commit.
2. **Commit Message:** Tiêu đề và mô tả lý do thay đổi.
3. **Author (Tác giả gốc) & AuthorDate:** Tên, email của người viết code và thời điểm viết code.
4. **Committer & CommitDate:** Tên, email của người thực hiện áp commit lên nhánh hiện tại và thời gian thực hiện.
5. **Parent Pointer(s):** Con trỏ trỏ về (các) commit cha ngay trước nó.
6. **Commit Hash (Mã SHA-1/SHA-256):** Khóa định danh duy nhất tính toán từ toàn bộ các thông tin trên.

---

### "Lấy" nội dung khi Cherry-Pick/Rebase là lấy cái gì?
Khi bạn thực hiện `cherry-pick` hoặc `rebase`, Git hoạt động theo nguyên lý **Patch (Miếng vá)**:

$$\text{Patch của Commit D} = \text{Commit D} - \text{Commit Cha }(D - 1)$$

* **Git CHỈ lấy đúng những dòng thay đổi (Diff/Delta) của riêng commit đó** (kèm theo một vài dòng ngữ cảnh xung quanh để xác định vị trí).
* **Git TUYỆT ĐỐI KHÔNG đè toàn bộ file** và **KHÔNG mang theo các thay đổi của các commit khác** trên nhánh nguồn sang.

**Ví dụ:**
* Trên nhánh `feature`, đồng nghiệp sửa từ dòng 1 đến 30 (Commit 1, 2, 3).
* Đến Commit `D`, họ sửa đúng 2 dòng ở dòng 80.
* Khi bạn đứng ở `main` và chạy `git cherry-pick D`:
  * File trên `main` **chỉ thay đổi đúng 2 dòng ở dòng 80**.
  * Các dòng 1 đến 30 trên `main` **giữ nguyên 100% code cũ**, không hề bị ảnh hưởng.

#### Những gì được giữ lại và những gì bị thay đổi khi chuyển commit:
| Thông tin | Khi Cherry-Pick / Rebase | Ghi chú |
| :--- | :--- | :--- |
| **Code Diff (Miếng vá)** | **Giữ nguyên** | Áp vào vị trí tương ứng trên nhánh mới |
| **Commit Message** | **Giữ nguyên** | Giữ nguyên lời nhắn gốc |
| **Author & AuthorDate** | **Giữ nguyên** | Vẫn ghi nhận người viết ban đầu |
| **Committer & CommitDate** | **Thay đổi** | Ghi nhận bạn và thời gian bạn chạy lệnh |
| **Parent Commit** | **Thay đổi** | Nối vào commit đỉnh của nhánh hiện tại |
| **Commit Hash** | **Thay đổi mã mới** | Do Parent và CommitDate đã thay đổi |

---

### So sánh trực quan: `git rebase` vs `git cherry-pick`

Giả sử ban đầu có cấu trúc nhánh như sau:
```text
main:       A --- B --- C
                   \
feature:            D --- E
```

#### 1. `git cherry-pick <commit>` — Nhặt commit riêng lẻ
* **Bản chất:** Bạn đứng ở nhánh này và muốn "hái trộm" 1 hoặc vài commit cụ thể từ nhánh khác mà không cần lấy toàn bộ nhánh.
* **Thao tác:**
  ```bash
  git switch main
  git cherry-pick D
  ```
* **Kết quả:**
  ```text
  main:       A --- B --- C --- D'   (Chỉ lấy riêng D, bỏ lại E)
  feature:    (vẫn giữ nguyên A-B-D-E)
  ```

#### 2. `git rebase <base-branch>` — Đổi gốc toàn bộ nhánh
* **Bản chất:** Nhấc bổng **toàn bộ các commit** của nhánh hiện tại (`D`, `E`) đặt lên trên đỉnh của nhánh đích (`C`).
* **Thao tác:**
  ```bash
  git switch feature
  git rebase main
  ```
* **Kết quả:**
  ```text
  main:       A --- B --- C
                           \
  feature:                  D' --- E'   (Gốc của feature chuyển từ B sang C)
  ```

---

## 2. Giá Trị Của "Lịch Sử Thẳng" (Linear History)

Tại sao các công ty lớn và dự án mã nguồn mở lại ưu tiên `git rebase` hơn `git merge` thông thường?

1. **Tránh "Mạng nhện" trong `git log` (Spaghetti History):**
   * Dùng `merge` liên tục sẽ tạo ra hàng loạt commit rác dạng `Merge branch 'main' into feature` khiến biểu đồ lịch sử đan chéo phức tạp.
   * `rebase` giữ toàn bộ lịch sử thành 1 đường thẳng duy nhất, dễ đọc, dễ theo dõi tiến độ.
2. **Hỗ trợ truy vết lỗi cực nhanh với `git bisect`:**
   * Khi gặp lỗi nghiêm trọng trên Production/Firmware, `git bisect` chạy tìm kiếm nhị phân trên lịch sử thẳng chỉ mất 3–5 lần test để chỉ đích danh commit gây lỗi.
   * Lịch sử có nhiều nhánh đan xen làm `git bisect` dễ rơi vào trạng thái trung gian gây sai lệch kết quả test.
3. **An toàn và dễ dàng khi cần `git revert`:**
   * Revert một commit đơn lẻ trên đường thẳng rất đơn giản: `git revert <hash>`.
   * Revert một Merge Commit rất phức tạp (phải chỉ định `-m 1` hoặc `-m 2` vì có 2 commit cha), dễ dẫn đến mất code ngầm khi merge lại về sau.

---

## 3. Xử Lý Sự Cố & "Địa Ngục Conflict" Khi Rebase (Rebase Hell)

### Nhận diện commit đang bị conflict
Khi rebase dừng lại vì conflict, gõ lệnh:
```bash
git status
```
Git sẽ hiển thị vị trí chính xác:
```text
interactive rebase in progress; onto abc1234
Last command done (5 of 10):
   pick a1b2c3d Sửa hàm uart_init()       <-- Commit số 5/10 đang gây conflict
Next commands to do (6 of 10):
   pick e4f5g6h Thêm module SPI           <-- Các commit tiếp theo
Unmerged paths:
   both modified:   drivers/uart.c        <-- File cần sửa conflict
```

---

### Lỗi commit rỗng / trùng lặp sau khi sửa conflict
Sau khi giải quyết conflict và chạy `git add .`, nếu nội dung sửa đổi đã có sẵn ở nhánh đích khiến commit không còn diff nào mới, lệnh `git rebase --continue` sẽ báo:
```text
The previous cherry-pick is now empty, possibly due to conflict resolution:
    git commit --allow-empty

Otherwise, please use 'git rebase --skip'
```

* **Cách xử lý chuẩn (Bỏ qua commit này và đi tiếp):**
  ```bash
  git rebase --skip
  ```
* **Nếu muốn giữ lại commit dù nó rỗng:**
  ```bash
  git commit --allow-empty
  git rebase --continue
  ```

---

### 3 Vũ khí xử lý xung đột dây chuyền

Khi 1 file bị sửa ở nhiều commit liên tiếp (5, 6, 7, 8), việc rebase có thể gây conflict lặp đi lặp lại. Hãy áp dụng 3 kỹ thuật sau:

#### Vũ khí 1: Gộp commit (Squash) TRƯỚC KHI Rebase *(Khuyên Dùng Nhất)*
Nếu nhánh feature có nhiều commit vụn vặt, hãy gộp chúng thành 1 commit duy nhất trước khi rebase lên `main`:
```bash
git rebase -i HEAD~5   # Gộp 5 commit gần nhất thành 1
```
* **Lợi ích:** Bạn chỉ cần giải quyết conflict **đúng 1 lần duy nhất** thay vì phải sửa 5 lần.

#### Vũ khí 2: Bật tính năng ghi nhớ giải quyết conflict (`git rerere`)
Bật tính năng **Reuse Recorded Resolution** (chỉ cần bật 1 lần duy nhất):
```bash
git config --global rerere.enabled true
```
* **Cơ chế:** Khi bạn sửa conflict ở commit 5, Git sẽ tự ghi nhớ. Khi gặp lại conflict giống hệt ở commit 6, 7, 8, Git sẽ **tự động điền kết quả giải quyết** mà bạn không cần can thiệp thủ công.

#### Vũ khí 3: Đường lui an toàn với `git rebase --abort`
Nếu quá trình rebase quá phức tạp và rối loạn:
```bash
git rebase --abort
```
Toàn bộ trạng thái sẽ quay về nguyên vẹn như trước khi gõ lệnh rebase. Sau đó bạn có thể chuyển sang dùng `git merge` cho an toàn.

---

### Gộp commit (Squash): Điều gì xảy ra với Code & Metadata?

Nhiều người e ngại khi gộp nhiều commit (ví dụ 5 commit thành 1) thì mọi thông tin hay code sẽ biến mất. Dưới đây là cơ chế chính xác:

#### 1. Về mặt CODE (Nội dung thay đổi):
* **KHÔNG MẤT BẤT KỲ DÒNG CODE NÀO!**
* Toàn bộ diff (các dòng code thêm, xóa, sửa) trong suốt 5 commit đó sẽ được gộp chung lại thành một thay đổi tổng thể duy nhất trên 1 commit mới.

#### 2. Về mặt LỊCH SỬ & METADATA (Thông tin commit):
| Thành phần Metadata | Trạng thái sau khi Squash | Chi tiết cơ chế |
| :--- | :--- | :--- |
| **Mã Hash cũ** | **Bị thay thế** | 5 mã hash cũ sẽ không còn xuất hiện trên nhánh, thay bằng **1 mã SHA mới**. |
| **Commit Message** | **KHÔNG BỊ MẤT** | Mặc định Git sẽ gom toàn bộ 5 lời nhắn cũ lại và mở editor cho bạn chỉnh sửa, tổng hợp thành một message hoàn chỉnh. |
| **Author (Tác giả)** | **Giữ tác giả commit đầu** | Git lấy Author và AuthorDate của commit đầu tiên (commit được `pick`). |
| **Committer & Date** | **Cập nhật mới** | Ghi nhận người thực hiện lệnh squash và thời gian thực tế lúc gộp. |
| **Parent Commit** | **Cập nhật** | Trỏ tới commit cha đứng trước commit đầu tiên trong nhóm gộp. |

#### 3. Phân biệt lệnh `squash` và `fixup` trong Interactive Rebase (`git rebase -i`):
* **`squash` (hoặc `s`):** Gộp code vào commit trước **VÀ giữ lại commit message** của nó để biên tập chung.
* **`fixup` (hoặc `f`):** Gộp code vào commit trước **NHƯNG vứt bỏ commit message** của nó (dùng cho các commit sửa lỗi chính tả, fix vặt không cần ghi vào lịch sử).

#### 4. Metadata và commit cũ có bị mất vĩnh viễn không?
* **KHÔNG!** Trong Git, không có gì biến mất ngay lập tức.
* Toàn bộ 5 commit cũ với đầy đủ mã hash, message, tác giả vẫn được lưu trong **`git reflog`** trong khoảng **30 đến 90 ngày**.
* Nếu muốn khôi phục lại trạng thái trước khi squash:
  ```bash
  git reflog                      # Tìm mã commit trước khi rebase/squash
  git reset --hard HEAD@{n}       # Đưa nhánh quay lại đúng trạng thái cũ
  ```

#### 5. Khi nào NÊN và KHÔNG NÊN Squash?
* **NÊN Squash khi:** Nhánh có nhiều commit nháp, commit sửa vặt (`WIP`, `fix typo`, `test lai`, `sua bug nho`...) &rarr; Gộp lại thành 1 commit có ý nghĩa rõ ràng trước khi merge/PR vào `main`.
* **KHÔNG NÊN Squash khi:** 5 commit đó là 5 bước logic / module độc lập và có giá trị riêng biệt cần giữ lại để team review hoặc tra cứu lịch sử sau này.

