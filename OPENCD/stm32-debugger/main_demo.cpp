#include "DebugCore.h"
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>

// ─── Listener example ─────────────────────────────────────────────────────────

class PrintListener : public IDebugEventListener {
public:
    void onDebugEvent(const DebugEvent& evt) override {
        // This runs on the GDB reader thread — post to a real UI thread in production.
        switch (evt.type) {
            case DebugEventType::BreakpointHit:
                std::cout << "[BREAKPOINT] id=" << evt.bkptId
                          << " at " << evt.frame.file << ":" << evt.frame.line
                          << " in " << evt.frame.function << "\n";
                break;
            case DebugEventType::Stopped:
                std::cout << "[STOPPED] reason=" << evt.reason
                          << " at " << evt.frame.file << ":" << evt.frame.line << "\n";
                break;
            case DebugEventType::Running:
                std::cout << "[RUNNING]\n";
                break;
            case DebugEventType::ConsoleOutput:
                std::cout << "[GDB] " << evt.message;
                break;
            case DebugEventType::Disconnected:
                std::cout << "[DISCONNECTED] " << evt.message << "\n";
                break;
            case DebugEventType::Error:
                std::cerr << "[ERROR] " << evt.message << "\n";
                break;
        }
    }
};

// ─── Entry point ──────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) 
{
    if (argc < 3) 
    {
        std::cerr << "Usage: debugger_demo <elf-file> <openocd-config>\n"
                  << "  e.g. debugger_demo firmware.elf stm32f4.cfg\n";
        return 1;
    }

    const std::string elfPath      = argv[1];
    const std::string openocdCfg   = argv[2];

    DebugCore core;
    PrintListener listener;
    core.addListener(&listener);

    std::cout << "Starting session...\n";
    if (!core.startSession(elfPath, openocdCfg)) {
        std::cerr << "Failed to start debug session.\n";
        return 2;
    }
    std::cout << "Session started. ELF loaded.\n";

    IDebugger* dbg = core.debugger();

    // Set a breakpoint at main().
    int bpId = dbg->addBreakpoint("main.c", 1);
    std::cout << "Breakpoint set: id=" << bpId << "\n";

    // Reset the MCU and halt at reset vector, then run to main().
    dbg->reset(true);
    dbg->run();

    // Wait for the breakpoint hit (the PrintListener will print it).
    std::this_thread::sleep_for(std::chrono::seconds{5});

    // Inspect state.
    std::cout << "\n--- Registers ---\n" << dbg->readRegisters();

    auto frames = dbg->backtrace();
    std::cout << "\n--- Backtrace ---\n";
    for (const auto& f : frames)
        std::cout << "  #" << f.level << " " << f.function
                  << " at " << f.file << ":" << f.line << "\n";

    // Read 16 bytes from address 0x20000000 (SRAM start on STM32).
    auto mem = dbg->readMemory(0x20000000, 16);
    std::cout << "\n--- Memory @ 0x20000000 ---\n  ";
    for (uint8_t b : mem)
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(b) << " ";
    std::cout << "\n";

    core.endSession();
    std::cout << "\nSession ended.\n";
    return 0;
}
