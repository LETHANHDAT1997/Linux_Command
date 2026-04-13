
## 📄 **Doc đã chỉnh sửa (chuẩn hóa kiến trúc)**

---

# 📁 ClientPublic

👉 **Giá trị:** *API sạch cho user + định nghĩa dependency cấp cao*

* Che giấu toàn bộ hệ thống bên dưới
* Cung cấp interface dễ dùng (`setBreakpoint`, `run`, `halt`…)
* Nhận event qua callback và forward cho user

### Chứa:

* `DebugClient`
* `IDebugFactory`  ← abstraction để tạo Protocol + Transport

---

# 📁 ClientProtocol

👉 **Giá trị:** *Nơi hiểu và điều phối debug + định nghĩa abstraction cho IO*

* Encode command (GDB MI / RSP…)
* Parse response / event
* Quản lý:

  * state (running, stopped…)
  * command flow (queue, pending nếu có)
* Là cầu nối logic giữa Public ↔ Transport

### Chứa:

* `IProtocol`
* `ITransport`  ← abstraction (⚠️ thuộc về Protocol, không phải Transport)
* `GDBProtocol`

---

# 📁 ClientTransport

👉 **Giá trị:** *IO thuần (implementation)*

* Gửi / nhận dữ liệu raw (pipe, socket…)
* Không biết protocol, không biết logic
* Chỉ đảm bảo dữ liệu đi/đến

### Chứa:

* `TCPTransport` (implements `ITransport`)
* (sau này: `USBTransport`, `SerialTransport`…)

---

# 📁 ClientFactory (Composition Layer)

👉 **Giá trị:** *Wiring / tạo object graph*

* Biết concrete Protocol + Transport
* Kết hợp chúng lại thành hệ thống hoàn chỉnh

### Chứa:

* `GDBDebugFactory` (implements `IDebugFactory`)

---

# 🔗 Dependency (CHUẨN – KHÔNG ĐỔI)

```text
ClientPublic
    ↓
ClientProtocol
    ↓
ClientTransport

ClientFactory → depends on ALL
```

---

# ⚠️ Nguyên tắc quan trọng

### 1. Interface không thuộc về thằng implement

* `ITransport` nằm ở **ClientProtocol**, không phải ClientTransport

---

### 2. Interface thuộc về nơi cần abstraction

* Protocol cần tách khỏi Transport → define `ITransport`

---

### 3. Factory không phải business layer

* Nó chỉ là **composition/wiring**

---

# ✅ Tóm tắt 1 dòng (bản nâng cấp)

```text
Public = API + abstraction cấp cao
Protocol = logic + abstraction IO
Transport = IO implementation
Factory = wiring
```

---

# 📌 Key insight (chốt)

> **Transport implement abstraction của Protocol, không sở hữu nó**

---
