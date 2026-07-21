# Quy Tắc Format Code C/C++

Áp dụng các quy tắc trình bày mã nguồn sau cho tất cả file C và C++ trong dự án.

## 1. Dấu ngoặc nhọn `{}` phải thẳng hàng, mỗi cái một dòng riêng

**Đúng:**
```cpp
if (condition) 
{
    doSomething();
}
else 
{
    doOther();
}
```

**Sai:**
```cpp
if (condition) {
    doSomething();
} else {
    doOther();
}
```

Quy tắc này áp dụng cho tất cả: `if`, `else`, `for`, `while`, `switch`, `struct`, `class`, hàm, lambda,...

## 2. Không ghi gọn trên cùng một hàng khi body có logic

Dù body chỉ có 1 dòng, vẫn phải tách `{}` ra dòng riêng.

**Đúng:**
```cpp
GStreamer_Glue::~GStreamer_Glue() 
{
    g_print("Đã hủy.\n");
}
```

**Sai:**
```cpp
GStreamer_Glue::~GStreamer_Glue() { g_print("Đã hủy.\n"); }
```

## 3. Lời gọi hàm dưới 5 tham số phải viết trên 1 dòng

Không được xuống hàng nếu lời gọi hàm có ít hơn 5 tham số đầu vào.

**Đúng:**
```cpp
gst_device_monitor_add_filter(monitor.get(), filter.c_str(), NULL);
gst_structure_get_string(properties, "device.path");
```

**Sai:**
```cpp
gst_device_monitor_add_filter(monitor.get(),
                              filter.c_str(), NULL);
gst_structure_get_string(properties,
                         "device.path");
```

Nếu có từ 5 tham số trở lên, có thể xuống hàng và căn chỉnh theo tham số đầu tiên.

## 4. Khai báo hàm (function signature) phải viết trên 1 dòng nếu đủ ngắn

Không tách tên hàm và danh sách tham số ra nhiều dòng khi không cần thiết.

**Đúng:**
```cpp
void GStreamer_Glue::Private_Device_Monitor_Get_Devices(const std::string arg_filter) 
{
    // ...
}
```

**Sai:**
```cpp
void GStreamer_Glue::Private_Device_Monitor_Get_Devices(
    const std::string arg_filter) {
    // ...
}
```

## 5. Logic bên trong phải thụt lề hơn logic bên ngoài

Sử dụng 4 spaces cho mỗi cấp thụt lề. Code bên trong block phải thụt lề thêm 1 cấp so với `{}`.

**Đúng:**
```cpp
for (int i = 0; i < n; i++) 
{
    if (condition) 
    {
        doSomething();
    }
}
```

**Sai:**
```cpp
for (int i = 0; i < n; i++) 
{
if (condition) 
{
doSomething();
}
}
```

## 6. Phép gán đơn giản không cần xuống hàng

Nếu vế phải của phép gán ngắn gọn, viết trên cùng 1 dòng.

**Đúng:**
```cpp
local_device_info.Devices_Type = (device_class != NULL) ? device_class : "Unknown";
```

**Sai:**
```cpp
local_device_info.Devices_Type =
    (device_class != NULL) ? device_class : "Unknown";
```
