#include "../../include/process/OpenOCDProcess.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <chrono>

bool OpenOCDProcess::start(const std::string& configFile,
                            const std::string& openocdPath,
                            int gdbPort) {
    port_ = gdbPort;
    // -f <config>  : board/interface config file
    // -c "..."     : inline command — set the GDB server port explicitly
    std::string portCmd = "gdb_port " + std::to_string(gdbPort);
    return spawnProcess({openocdPath, "-f", configFile, "-c", portCmd});
}

bool OpenOCDProcess::waitReady(std::chrono::milliseconds timeout) {
    auto deadline = std::chrono::steady_clock::now() + timeout;

    while (std::chrono::steady_clock::now() < deadline) {
        int sock = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(static_cast<uint16_t>(port_));
        ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

        bool connected = (::connect(sock,
                                    reinterpret_cast<sockaddr*>(&addr),
                                    sizeof(addr)) == 0);
        ::close(sock);

        if (connected) return true;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}
