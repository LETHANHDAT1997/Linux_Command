# Design Patterns (C++)

Tài liệu và ví dụ C++17 cho các mẫu GoF. Trang tổng hợp UML/code: [`cpp_design_patterns.html`](cpp_design_patterns.html) (đủ 23 mẫu). Phần dưới đây là các ví dụ **biên dịch được** kèm ghi chú tiếng Việt.

## Biên dịch

```bash
cd Design_Patterns
cmake -S . -B build
cmake --build build
```

Chạy từng ví dụ từ `build/` (tên target trùng tên mẫu, ví dụ `factory_method`, `observer`).

## Mục lục

| Nhóm | Mẫu | Code | Ghi chú | Trạng thái |
| :--- | :--- | :--- | :--- | :--- |
| Creational | Factory Method | [factory_method.cpp](Creational/factory_method/factory_method.cpp) | [md](Creational/factory_method/factory_method.md) | xong |
| Creational | Abstract Factory | [abstract_factory.cpp](Creational/abstract_factory/abstract_factory.cpp) | [md](Creational/abstract_factory/abstract_factory.md) | xong |
| Creational | Builder | [builder.cpp](Creational/builder/builder.cpp) | [md](Creational/builder/builder.md) | xong |
| Creational | Singleton | — | chỉ trong HTML | thiếu |
| Creational | Prototype | — | chỉ trong HTML | thiếu |
| Structural | Adapter | [adapter.cpp](Structural/adapter/adapter.cpp) | [md](Structural/adapter/adapter.md) | xong |
| Structural | Proxy | [proxy.cpp](Structural/proxy/proxy.cpp) | [md](Structural/proxy/proxy.md) | xong |
| Structural | Bridge | — | chỉ trong HTML | thiếu |
| Structural | Composite | — | chỉ trong HTML | thiếu |
| Structural | Decorator | — | chỉ trong HTML | thiếu |
| Structural | Facade | — | chỉ trong HTML | thiếu |
| Structural | Flyweight | — | chỉ trong HTML | thiếu |
| Behavioral | Observer | [observer.cpp](Behavioral/observer/observer.cpp) | [md](Behavioral/observer/observer.md) | xong |
| Behavioral | State | [state.cpp](Behavioral/state/state.cpp) | [md](Behavioral/state/state.md) | xong |
| Behavioral | Strategy | [strategy.cpp](Behavioral/strategy/strategy.cpp) | [md](Behavioral/strategy/strategy.md) | xong |
| Behavioral | Chain of Responsibility | — | chỉ trong HTML | thiếu |
| Behavioral | Command | — | chỉ trong HTML | thiếu |
| Behavioral | Iterator | — | chỉ trong HTML | thiếu |
| Behavioral | Mediator | — | chỉ trong HTML | thiếu |
| Behavioral | Memento | — | chỉ trong HTML | thiếu |
| Behavioral | Template Method | — | chỉ trong HTML | thiếu |
| Behavioral | Visitor | — | chỉ trong HTML | thiếu |
| Behavioral | Interpreter | — | chỉ trong HTML | thiếu |

## So sánh nhanh các mẫu gần nhau

- **Factory Method** tạo *một* sản phẩm; **Abstract Factory** tạo *cả họ* sản phẩm khớp nhau; **Builder** dựng một đối tượng phức tạp theo từng bước.
- **Adapter** đổi giao diện; **Proxy** kiểm soát truy cập cùng giao diện; **Decorator** thêm hành vi cùng giao diện; **Facade** gom nhiều lớp thành một API đơn giản.
- **Strategy** đổi thuật toán lúc chạy; **State** đổi hành vi theo trạng thái nội bộ; **Template Method** cố định khung thuật toán, chỉ override từng bước.
