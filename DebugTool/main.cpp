#include "DebugClient.h"
#include "GDBProtocol.h"
#include "TCPTransport.h"
#include <memory>
#include <iostream>

// Example usage of DebugClient with GDB MI protocol over TCP transport.
std::vector<std::string> argv = {"/ArmToolChain/arm-none-eabi-gdb", "--interpreter=mi", "--quiet", "--nx"};

int main(int argc, char* argv[]) 
{
    // auto transport = std::make_unique<TCPTransport>();
    auto protocol  = std::make_unique<GDBProtocol>();
    DebugClient client(std::move(protocol));

    /** 
     * Note: Start OpenOCD aka DebugServer first — it must be listening before GDB connects. 
     * */ 

    /** 
     * Note: Start the debug client process (GDB MI)
     * */ 
    if (client.start(argv) == false) 
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
    client.connect("localhost", 3333);
    client.loadElf("/path/to/your/elf");
    client.run();
}