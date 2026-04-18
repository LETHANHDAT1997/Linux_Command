#include "DebugServer.h"
#include "DebugClient.h"
#include "GDBProtocol.h"
#include "TCPTransport.h"
#include "OpenOCD.h"
#include <memory>
#include <iostream>

/**************************************** Simulator parameters, which can be replaced after the interface is available ****************************************/
constexpr uint16_t      OpenOCDPort = 3333;
std::vector<std::string> g_argv_OpenOCD = { "openocd",
                                            "-f", "interface/stlink.cfg",
                                            "-f", "target/stm32f4x.cfg",
                                            "-c", "bindto 127.0.0.1",
                                            "-c", "gdb_port " + std::to_string(OpenOCDPort),
                                            "-c", "init; reset halt"
                                          }; // Example usage of DebugServer with OpenOCD as the debug server implementation.

constexpr const char*   LocalHost           = "localhost";
constexpr const char*   ElfPath             = "/path/to/your/elf";
std::vector<std::string> g_argv_ArmGDB_Tool =   {   "/ArmToolChain/arm-none-eabi-gdb", 
                                                    "--interpreter=mi", "--quiet", 
                                                    "--nx"
                                                }; // Example usage of DebugClient with GDB MI protocol over TCP transport.
/**************************************** Simulator parameters, which can be replaced after the interface is available ****************************************/

int main(int argc, char* argv[]) 
{
    auto serverProtocol = std::make_unique<OpenOCD>();
    DebugServer server(std::move(serverProtocol));

    // auto transport = std::make_unique<TCPTransport>();
    auto protocol  = std::make_unique<GDBProtocol>();
    DebugClient client(std::move(protocol));

    /** 
     * Note: Start OpenOCD aka DebugServer first — it must be listening before GDB connects. 
     * */ 
    if (server.Start(g_argv_OpenOCD) == false)
    {
        std::cerr << "Failed to start debug server\n";
        server.terminate();
        return 1;
    }
    else
    {
        if (!server.WaitReady()) 
        {
            std::cerr << "Debug server did not become ready in time\n";
            server.StopProcess();
            return 1;
        }
        std::cout << "Debug server started successfully\n";
    }

    /** 
     * Note: Start the debug client process (GDB MI)
     * */ 
    if (client.start(g_argv_ArmGDB_Tool) == false) 
    {
        std::cerr << "Failed to start debug client\n";
        client.terminate();
        return 1;
    }
    else
    {
        int outFd = client.stdoutFd();
        int inFd  = client.stdinFd();
    }
    
    // Now we can use the client API to control the debug session.
    client.connect(LocalHost, OpenOCDPort);
    client.importElf(ElfPath);
    client.run();
}