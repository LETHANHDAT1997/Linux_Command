#include "OpenOCD.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <chrono>

constexpr const char* g_RequiredArgs[] = {"ConfigFile", "ServerPath", "gdbPort", "Host"};

OpenOCD::OpenOCD() :
m_port(-1),
m_host("")
{
}

OpenOCD::~OpenOCD()
{

}

bool OpenOCD::ParseArgs(const std::vector<std::string>& argv) 
{
    // -f <config>  : board/interface config file
    // -c "..."     : inline command — set the GDB server port explicitly
    // std::string portCmd = "gdb_port " + m_Config.port;
    for(size_t i = 0; i < argv.size(); ++i)
    {
        if (argv[i] == "-c" && i + 1 < argv.size()) 
        {
            if(argv[i + 1].find("gdb_port") != std::string::npos) 
            {
                m_port = std::stoi(argv[i + 1].substr(argv[i + 1].find("gdb_port") + 9));
            }
            else if(argv[i + 1].find("telnet_port") != std::string::npos)
            {
                m_port = std::stoi(argv[i + 1].substr(argv[i + 1].find("telnet_port") + 11));
            }
            else if(argv[i + 1].find("bindto") != std::string::npos)
            {
                m_host = argv[i + 1].substr(argv[i + 1].find("bindto") + 7);
            }
        } 
        else if (argv[i] == "-f" && i + 1 < argv.size()) 
        {

        }
        else if (argv[i] == "-s" && i + 1 < argv.size()) 
        {
        }
        else if (argv[i] == "-l" && i + 1 < argv.size()) 
        {
        }
        else if (argv[i] == "-d" && i + 1 < argv.size()) 
        {
        }
    }

    if (m_port == -1 && m_host.empty()) 
    {
        std::cerr << "Error: GDB server port not specified in OpenOCD command line\n";
        return false;
    }
    return true;
}

bool OpenOCD::WaitReady(std::chrono::milliseconds timeout) 
{
    auto deadline = std::chrono::steady_clock::now() + timeout;

    while (std::chrono::steady_clock::now() < deadline) 
    {
        int sock = ::socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) 
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        struct sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(m_port);
        ::inet_pton(AF_INET, m_host.c_str(), &addr.sin_addr);

        bool connected = (::connect(sock,
                                    reinterpret_cast<sockaddr*>(&addr),
                                    sizeof(addr)) == 0);
        ::close(sock);

        if (connected) return true;

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}
