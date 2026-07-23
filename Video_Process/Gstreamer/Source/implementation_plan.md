# Kế hoạch Refactor `Private_Build_Video_Pipeline`

## Câu trả lời cho câu hỏi

### 1. Tại sao lại có "cấu hình đặc biệt" cho AppSink?

Lý do kỹ thuật: **`appsink` là sink duy nhất yêu cầu C API đặc thù sau khi tạo element**.

| Sink | Sau `gst_element_factory_make()` | API bổ sung |
|---|---|---|
| `glimagesink` | Không cần gì thêm | Không |
| `autovideosink` | Không cần gì thêm | Không |
| `fakesink` | Không cần gì thêm | Không |
| `appsink` | Phải set `max-buffers`, `drop`, `emit-signals` **và** đăng ký `GstAppSinkCallbacks` | `gst_app_sink_set_callbacks()` |

Vì vậy cấu hình đặc biệt là hợp lý — nhưng **vị trí đặt code hiện tại** (nằm lẫn trong bước 5 "link elements") thì không hợp lý, nên tách ra step riêng.

### 2. Dùng switch-case cho việc cấu hình sink có vấn đề gì không?

**Không có vấn đề kỹ thuật**, và thực ra đây là cách tốt hơn vì:
- Dễ thêm sink type mới về sau (chỉ thêm `case`).
- Trình biên dịch cảnh báo khi có sink type chưa xử lý (`-Wswitch`).
- Đọc rõ ràng hơn: cấu hình của **mỗi loại sink** nằm ở một chỗ riêng biệt.

Đối với các sink không cần cấu hình (`glimagesink`, `fakesink`, `autovideosink`), dùng `default: break;` bình thường.

---

## Thiết kế Refactor

### Vấn đề hiện tại

Hàm dài ~185 dòng, thực hiện 7 bước độc lập, với nhiều cấp lồng nhau (block trong block).

### Cây tổ chức sau refactor

```
Private_Build_Video_Pipeline()          ← Coordinator (~35 dòng)
    │
    ├─ Private_Step_Create_Elements()   ← Step 1+2: tạo elements, gán camera
    │
    ├─ Private_Step_Build_Caps()        ← Step 3: tạo GstCaps, gán vào filter
    │
    ├─ Private_Step_Add_And_Link()      ← Step 4+5: bin_add + element_link
    │
    ├─ Private_Step_Configure_Sink()    ← Step 5b: switch-case theo sink type
    │
    └─ Private_Step_Setup_Bus_And_Loop()← Step 6+7: bus watch + main loop
```

---

## Proposed Changes

### [MODIFY] [Camera_Devices.cpp](file:///home/ledat/Documents/Linux_Command/Video_Process/Gstreamer/Source/Camera_Devices.cpp)

#### Thêm vào class (phần `private:`) — struct + khai báo hàm

```cpp
/* Struct trung gian truyền elements giữa các bước build.
 * Sau gst_bin_add_many(), pipeline nắm ownership.
 * Struct này chỉ sống trong Private_Build_Video_Pipeline(). */
struct Pipeline_Elements
{
    GstElement *filter  = nullptr;
    GstElement *queue1  = nullptr;
    GstElement *conv    = nullptr;
    GstElement *queue2  = nullptr;
};

bool Private_Step_Create_Elements(const Pipeline_Config &cfg,
                                   Pipeline_Elements &out_elems);

void Private_Step_Build_Caps(const Pipeline_Config &cfg, GstElement *filter);

bool Private_Step_Add_And_Link(const Pipeline_Elements &elems);

void Private_Step_Configure_Sink(const Pipeline_Config &cfg);

void Private_Step_Setup_Bus_And_Loop();
```

#### `Private_Build_Video_Pipeline` sau refactor (~35 dòng)

```cpp
bool GStreamer_Glue::Private_Build_Video_Pipeline(const Pipeline_Config &arg_config)
{
    // Guard: validate
    // Lưu config, tạo pipeline cha

    Pipeline_Elements elems;
    if (!Private_Step_Create_Elements(arg_config, elems)) return false;
    Private_Step_Build_Caps(arg_config, elems.filter);
    if (!Private_Step_Add_And_Link(elems))                return false;
    Private_Step_Configure_Sink(arg_config);
    Private_Step_Setup_Bus_And_Loop();
    return true;
}
```

#### `Private_Step_Configure_Sink` dùng switch-case

```cpp
void GStreamer_Glue::Private_Step_Configure_Sink(const Pipeline_Config &arg_config)
{
    switch (arg_config.sink_type)
    {
        case VideoSinkType::AppSink:
        {
            GstAppSink *appsink = GST_APP_SINK(GStreamer_Pipeline_Structure.video_sink);
            g_object_set(G_OBJECT(appsink), ...);

            if (arg_config.appsink_frame_callback)
            {
                static GstAppSinkCallbacks appsink_cbs;
                // ... đăng ký callback
                gst_app_sink_set_callbacks(appsink, &appsink_cbs, cfg_ptr, nullptr);
            }
            break;
        }
        case VideoSinkType::GlImageSink:
        case VideoSinkType::AutoVideoSink:
        case VideoSinkType::FakeSink:
        default:
            break;
    }
}
```

---

## Lưu ý kỹ thuật

> [!IMPORTANT]
> `static GstAppSinkCallbacks appsink_cbs` **phải giữ nguyên là `static`** khi tách sang `Private_Step_Configure_Sink`. GStreamer lưu trữ con trỏ đến struct này trong vòng đời pipeline — nếu là local variable sẽ bị dangling pointer dẫn đến segfault (đã gặp trước đó).

> [!NOTE]
> `Pipeline_Elements` là struct nội bộ tạm thời — chỉ sống trong quá trình build. Sau `gst_bin_add_many()`, pipeline nắm ownership toàn bộ raw pointers bên trong.

> [!WARNING]
> Chưa commit lên git cho đến khi test đầy đủ sau refactor.

---

## Verification Plan

### Automated (sau refactor)
```bash
# Build lại
g++ -std=c++17 ... Camera_Devices.cpp -o build/camera_devices_test

# Test từng mode
./build/camera_devices_test 2    # fakesink: không crash
./build/camera_devices_test 3    # appsink: nhận đủ 150 frame
```

### Manual
- `./build/camera_devices_test 1` để xác nhận glimagesink còn hoạt động
