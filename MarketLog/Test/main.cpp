#include "main.h"

int main(void)
{
    MarKetLog m_MarKetLog;
    m_MarKetLog.StartThreadFileHandle();
    m_MarKetLog.StartThreadRotator();
    std::this_thread::sleep_for(std::chrono::seconds(30));
    m_MarKetLog.StopThreadRotator();
    m_MarKetLog.StopThreadFileHandle();
    return 0;
}