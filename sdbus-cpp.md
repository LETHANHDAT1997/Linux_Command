### Giới thiệu về sdbus-cpp
sdbus-cpp là một thư viện C++ high-level cho D-Bus trên Linux, được thiết kế để cung cấp API dễ sử dụng và mạnh mẽ với phong cách C++ hiện đại. Nó xây dựng trên sd-bus (thư viện C từ systemd), nhưng không bị ràng buộc với systemd và có thể dùng trong môi trường khác (như libelogind hoặc basu). Thư viện tập trung vào việc trừu tượng hóa các chi tiết low-level, hỗ trợ synchronous/asynchronous calls, signals, properties, và code generation từ XML. Đây là lựa chọn tốt nếu bạn muốn tránh complexity của libdbus hoặc libdbus-c++ cũ hơn, với hiệu suất tốt hơn nhờ sd-bus.

Dưới đây là các thông tin cần biết khi sử dụng, dựa trên tài liệu chính thức và các nguồn liên quan. Tôi sẽ tập trung vào dependencies, installation, tính năng, cách sử dụng cơ bản, ví dụ, và lưu ý.

### Dependencies
- **Thư viện chính**: libsystemd, libelogind, hoặc basu (phiên bản >= 238 để đảm bảo compatibility).
- **Công cụ build**: pkgconfig (để tìm dependencies), CMake (cho build từ source).
- **Cho code generation**: expat (nếu enable SDBUSCPP_BUILD_CODEGEN=ON).
- **Cho tests**: googletest (tự động download nếu build tests).
- Không phụ thuộc internet; thư viện lightweight và không cần thêm packages nặng.

Lưu ý: Nếu dùng trong môi trường không systemd (như embedded), dùng libelogind hoặc basu để thay thế.

### Installation
- **Từ packages (khuyến nghị cho production)**:
  - Ubuntu/Debian: `sudo apt install libsdbus-c++-dev libsdbus-c++-bin` (bao gồm codegen tool `sdbus-c++-xml2cpp`). Phiên bản mới nhất trên Ubuntu là 2.1.0 (từ cuối 2024).
  - Alpine Linux: `apk add sdbus-cpp-dev sdbus-cpp-doc`.
  - Conan (package manager): `conan install sdbus-cpp`.
- **Build từ source (cho customize)**:
  ```
  git clone https://github.com/Kistler-Group/sdbus-cpp.git
  cd sdbus-cpp
  mkdir build && cd build
  cmake .. -DCMAKE_BUILD_TYPE=Release -DSDBUSCPP_BUILD_CODEGEN=ON  # Enable codegen nếu cần
  cmake --build .
  sudo cmake --build . --target install
  ```
  Sau đó, link khi compile: `g++ your_code.cpp -lsdbus-c++`.

### Required C++ Standard
- Minimum: C++17 (API public tương thích hoàn toàn).
- Khuyến nghị: C++20 để sử dụng extra features như coroutines cho async operations. Compile với `-std=c++17` hoặc cao hơn.

### Main Features
- **Connections**: Hỗ trợ system/session bus, custom connections.
- **Objects và Proxies**: Đăng ký objects (server-side), tạo proxies (client-side) để gọi methods/signals/properties.
- **Methods và Calls**: Synchronous/asynchronous, hỗ trợ complex types (variants, structs, arrays).
- **Signals**: Emit (server) và subscribe (client).
- **Properties**: Get/set với caching tùy chọn.
- **Code Generation**: Tạo stub code từ XML IDL, giúp tránh boilerplate.
- **Async Support**: Sử dụng callbacks hoặc coroutines (C++20).
- **Error Handling**: Exceptions-based, với chi tiết lỗi từ sd-bus.
- **Khác**: Không ràng buộc systemd, hỗ trợ ObjectManager, introspection.

### Code Generation
Công cụ `sdbus-c++-xml2cpp` tạo glue code từ XML file mô tả interface (theo D-Bus spec).
- **Steps**:
  1. Viết XML: Định nghĩa interfaces, methods, signals với types (e.g., "i" cho int, "s" cho string, "a{y}" cho array of bytes).
  2. Chạy: `sdbus-c++-xml2cpp your_interface.xml --adaptor=server_glue.h --proxy=client_glue.h`.
  3. Kế thừa classes generated và implement pure virtual methods.
- **Benefits**: Giảm code thủ công, dễ maintain cho APIs phức tạp.
- **Tips**: Thêm annotation cho async: `<annotation name="org.freedesktop.DBus.Method.Async" value="clientserver"/>`. Đảm bảo types khớp D-Bus spec để tránh lỗi.

Ví dụ XML đơn giản cho method "sayHello":
```xml
<node>
  <interface name="org.example.HelloWorld">
    <method name="sayHello">
      <arg type="s" name="name" direction="in"/>
      <arg type="s" name="response" direction="out"/>
    </method>
  </interface>
</node>
```
Sau generate, dùng trong code để implement.

### Basic Usage Patterns
- **Tạo Connection**:
  ```cpp
  #include <sdbus-c++/sdbus-c++.h>
  auto conn = sdbus::createDefaultConnection();  // Session bus
  // Hoặc: sdbus::createSystemBusConnection();
  ```
- **Server-side (Objects, Methods, Signals)**:
  - Đăng ký object: `auto obj = sdbus::createObject(*conn, "/org/example/HelloWorld");`
  - Đăng ký method: `obj->registerMethod("sayHello").implementedAs([](const std::string& name){ return "Hello " + name; });`
  - Emit signal: `obj->emitSignal("helloSignal").withArguments("Hello!");`
  - Run loop: `conn->enterEventLoop();`
- **Client-side (Proxies, Calls)**:
  - Tạo proxy: `auto proxy = sdbus::createProxy(*conn, "org.example.HelloWorldService", "/org/example/HelloWorld");`
  - Gọi sync: `std::string response; proxy->callMethod("sayHello").onInterface("org.example.HelloWorld").withArguments("Client").storeResultsTo(response);`
  - Subscribe signal: `proxy->uponSignal("helloSignal").onInterface("org.example.HelloWorld").call([](const std::string& msg){ std::cout << msg; });`
- **Properties**: `obj->registerProperty("myProp").withGetter([]{ return 42; });` (server); `int val; proxy->getProperty("myProp").onInterface("...").storeValueTo(val);` (client).

### Asynchronous Operations
- Method calls: `auto asyncReply = proxy->callMethodAsync("sayHello").uponReplyInvoke([](sdbus::MethodReply& reply){ /* handle */ });`
- Sử dụng coroutines (C++20): Hỗ trợ co_await cho calls.
- Event loop integration: Có thể tích hợp với GLib GMainLoop hoặc custom loops bằng cách xử lý file descriptors từ connection. Pitfall: Đảm bảo event loop chạy để xử lý async; tránh block main thread.

### Error Handling
- Ném exceptions: `sdbus::Error` với name và message từ D-Bus.
- Kiểm tra: Try-catch quanh calls, ví dụ `try { proxy->callMethod(...); } catch (const sdbus::Error& e) { std::cerr << e.getName() << ": " << e.getMessage(); }`
- Best practice: Sử dụng `callMethodAsync` để tránh block, và handle errors trong callbacks.

### Examples
- **Server cơ bản** (tương tự ví dụ trước của bạn):
  ```cpp
  #include <sdbus-c++/sdbus-c++.h>
  int main() {
      auto conn = sdbus::createDefaultConnection();
      conn->requestName("org.example.HelloWorldService");
      auto obj = sdbus::createObject(*conn, "/org/example/HelloWorld");
      obj->registerMethod("sayHello").onInterface("org.example.HelloWorld").implementedAs([](const std::string& name){ return "Hello, " + name + "!"; });
      obj->finishRegistration();
      conn->enterEventLoop();
      return 0;
  }
  ```
- **Client cơ bản**:
  ```cpp
  #include <sdbus-c++/sdbus-c++.h>
  int main() {
      auto conn = sdbus::createDefaultConnection();
      auto proxy = sdbus::createProxy(*conn, "org.example.HelloWorldService", "/org/example/HelloWorld");
      std::string response;
      proxy->callMethod("sayHello").onInterface("org.example.HelloWorld").withArguments("Client").storeResultsTo(response);
      std::cout << response << std::endl;
      return 0;
  }
  ```
  Thêm ví dụ chi tiết: Xem repo examples trên GitHub như obj-manager-server.cpp hoặc sdbus-cpp-examples (sử dụng XML cho proxies).

### Best Practices, Common Pitfalls, và Performance Tips
- **Best Practices**:
  - Sử dụng code generation cho interfaces phức tạp để tránh lỗi types.
  - Prefer async calls cho apps không block (e.g., GUI hoặc high-throughput).
  - Integrate event loop đúng cách; dùng `conn->enterEventLoopAsync()` nếu cần non-blocking.
  - Kiểm tra compatibility với systemd version trước deploy.
- **Common Pitfalls**:
  - Quên request name service hoặc finish registration object dẫn đến lỗi connection.
  - Types mismatch trong XML/calls gây runtime errors.
  - Async: Quên wait response trong client async.
  - Multi-thread: sd-bus không thread-safe mặc định; dùng mutex nếu cần.
- **Performance Tips**:
  - sd-bus optimize hơn libdbus, nhưng tránh excessive calls (batch nếu có thể).
  - Sử dụng properties caching để giảm overhead.
  - Với large data, dùng variants hoặc structs hiệu quả; test với high-load để profile memory. (General C++ tips áp dụng, vì ít info cụ thể cho sdbus-cpp).

### Known Limitations và Recent Updates (tính đến 2025)
- **Limitations**: Không hỗ trợ full D-Bus features cổ (như old protocol variants); tập trung vào modern usage. Có thể cần custom config cho non-systemd env.
- **Updates**: Phiên bản 2.1.0 (2024) thêm improvements cho codegen và async. ChangeLog cho thấy fixes cho concurrency và compatibility với systemd mới. Không có updates lớn năm 2025 dựa trên dữ liệu hiện tại, nhưng repo active với contributions welcome.

Nếu bạn cần ví dụ cụ thể hơn (e.g., tích hợp với BLE như BlueZ) hoặc help debug, hãy cung cấp thêm chi tiết!