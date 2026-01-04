#include "main.h"

/* Get Input from terminal */
void GetInputFromTerminal(std::string& messages, std::string& path)
{
    size_t lnposnewline = messages.find_last_not_of("\n");

    if (lnposnewline != std::string::npos) 
    {
        messages.erase(lnposnewline + 1);
    } else 
    {
        return;
    }

    while(1)
    {
        std::cout << messages;
        if(std::getline(std::cin,path))
        {
            return;
        }
        else
        {
            path.clear();
            std::cout << "Invalid input. Please try again";
        }
        std::cin.ignore();
    }
}


int main(void)
{
    
    MarKetLog m_MarKetLog;
    m_MarKetLog.start();
    static int count = 10000;
    while(1)
    {

        std::string messages = "Enter choice is: ";
        std::string lsvalue;
        GetInputFromTerminal(messages, lsvalue);  
        uint8_t  lvalue = static_cast<uint8_t>(std::stoull(lsvalue));

        switch (lvalue)
        {
            case 0:
            {
                JK_LOGI("MarKetLogSendNotify ACC ON!");
                MARKETLOG_STATE_e npMs = MARKETLOG_STATE_ACC_ON;
                m_MarKetLog.MarKetLogSendNotify(npMs);
                break;
            }

            case 1:
            {
                JK_LOGI("MarKetLogSendNotify ACC OFF!");
                MARKETLOG_STATE_e npMs = MARKETLOG_STATE_ACC_OFF;
                m_MarKetLog.MarKetLogSendNotify(npMs);
                break;
            }

            default:
            {
                JK_LOGW("Value invalid! %d",lvalue);
                break;
            }
        }

    }

    return 0;
}