#include <iostream>
#include <thread>
#include <chrono>

// Giả sử các file header từ kiến trúc trước đã được include ở đây
#include "DebugCore.h"
#include "GDBDebugger.h"
#include "IDebugEventListener.h"

// 1. Tạo một Listener để nhận sự kiện từ GDB Reader Thread
class ConsoleLogger : public IDebugEventListener 
{
public:
    void onDebugEvent(const DebugEvent& evt) override 
    {
        // LƯU Ý QUAN TRỌNG: Hàm này được gọi từ Reader Thread của GDBMIReader!
        // Trong ứng dụng GUI thực tế (Qt/ImGui), bạn không được update UI trực tiếp ở đây,
        // mà phải đẩy một tín hiệu (signal/event) về UI Thread.
        
        std::cout << "\n[ASYNC EVENT] ";
        switch (evt.type) {
            case DebugEventType::Stopped:
                std::cout << "Target stopped. Reason: " << evt.reason << "\n";
                break;
            case DebugEventType::Running:
                std::cout << "Target is now running...\n";
                break;
            case DebugEventType::BreakpointHit:
                std::cout << "Breakpoint #" << evt.bkptId << " hit at " 
                          << evt.frame.file << ":" << evt.frame.line << "\n";
                break;
            case DebugEventType::Error:
                std::cout << "Error: " << evt.message << "\n";
                break;
            default:
                std::cout << "Unknown event\n";
                break;
        }
        std::cout << "gdb> " << std::flush; // Giữ lại prompt cho người dùng
    }
};

int main() {
    std::cout << "=== Khởi tạo Debug Session ===\n";

    // 2. Khởi tạo Core và thiết lập Backend (GDB)
    DebugCore debugCore;
    debugCore.setBackend(std::make_unique<GDBDebugger>());

    // 3. Đăng ký Listener để nghe sự kiện
    ConsoleLogger logger;
    debugCore.addListener(&logger);

    // 4. Bắt đầu session (Khởi chạy OpenOCD, GDB, load ELF)
    std::string elfFile = "build/firmware.elf";
    std::string ocdConfig = "board/stm32f4discovery.cfg";
    
    std::cout << "Đang kết nối OpenOCD và GDB...\n";
    if (!debugCore.startSession(elfFile, ocdConfig)) 
    {
        std::cerr << "Lỗi: Không thể khởi chạy debug session!\n";
        return 1;
    }

    // Lấy con trỏ debugger để thao tác chi tiết
    IDebugger* debugger = debugCore.debugger();

    // 5. Chạy chương trình
    std::cout << "Phát lệnh RUN tới target...\n";
    debugger->run(); // Lệnh này không block, trả về ngay lập tức

    // 6. Vòng lặp chính của ứng dụng (UI Thread)
    // Mô phỏng việc UI thread vẫn đang chạy, nhận input từ người dùng
    // trong khi Reader Thread đang chờ GDB output ở background.
    for (int i = 0; i < 5; ++i) 
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));

        // Kiểm tra trạng thái hiện tại (Thread-safe thông qua std::atomic)
        if (debugger->state() == DebugState::Stopped) 
        {
            std::cout << "\n[UI THREAD] Chương trình đang dừng.\n";
        } 
        else if (debugger->state() == DebugState::Running) 
        {
            std::cout << "\n[UI THREAD] Chương trình vẫn đang chạy...\n";
        }
        else if (debugger->state() == DebugState::Error) 
        {
            std::cout << "\n[UI THREAD] Đã xảy ra lỗi trong debug session.\n";
            break;
        }
        else if (debugger->state() == DebugState::Connected) 
        {
            std::cout << "\n[UI THREAD] Đã kết nối với target, nhưng chưa chạy.\n";
        }
        else if (debugger->state() == DebugState::Idle) 
        {
            std::cout << "\n[UI THREAD] Debugger đang ở trạng thái Idle.\n";
        }
        else 
        {
            std::cout << "\n[UI THREAD] Trạng thái không xác định.\n";
        }
    }

    // 7. Kết thúc dọn dẹp
    std::cout << "\n=== Dừng Debug Session ===\n";
    debugger->halt(); // Dừng CPU trước khi thoát
    debugCore.removeListener(&logger);
    debugCore.endSession(); // Dọn dẹp process (kill GDB, OpenOCD)

    return 0;
}