#include "MarKetLog.h"
#include "MarKetLogUtility.h"

MarKetLog::MarKetLog()
: m_Rotator(this),
  m_Watcher(this),
  m_CMarKetLogYAML(m_Context,"/home/datlt49/MarKetLog/Test/MarKetLogConfig.yaml")
{
    ScanExistingLogs(m_Context.m_DirPath, m_Context.m_Extension);

    m_Rotator.SetMaxFiles(m_Context.m_MaxLogFiles);
    m_Rotator.SetLogContext(m_Context);

    m_Watcher.SetLogContext(m_Context);
}

MarKetLog::~MarKetLog()
{
    stop();
}

void MarKetLog::ScanExistingLogs(const std::string& logDir, const std::string& extension)
{
    m_Context.files.clear();

    if (!std::filesystem::exists(logDir) || !std::filesystem::is_directory(logDir))
    {
        std::filesystem::create_directories(logDir);
        if (!std::filesystem::exists(logDir) || !std::filesystem::is_directory(logDir))
        {
            JK_LOGE("\nLỗi tạo thư mục!");
            return;
        }
    }

    for (const auto& entry : std::filesystem::directory_iterator(logDir)) 
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        const auto& path = entry.path();
        if (path.extension() != extension)
        {
            continue;
        }

        LogFileInfo info;
        info.path = path.string();
        info.size = std::filesystem::file_size(path);
        
        m_Context.files.push_back(info);
        JK_LOGD("Tìm thấy tệp %s!",info.path.c_str());
    }

    if(m_Context.files.size() > 0 )
    {
        // Sắp xếp để deterministic (quan trọng cho rotate)
        std::sort(m_Context.files.begin(), m_Context.files.end(),
                                                        [this](const LogFileInfo& a, const LogFileInfo& b) 
                                                        {
                                                            const uint32_t ia = MarKetLogUtility::ExtractLogIndex(a.path);
                                                            const uint32_t ib = MarKetLogUtility::ExtractLogIndex(b.path);
                                                            return ia < ib;
                                                        }
                );
    }

    for (const auto& f : m_Context.files)
    {
        JK_LOGD("File After Sort=%s", f.path.c_str());
    }
}

void MarKetLog::StartThreadRotator(void)
{
    m_Rotator.start();
}

void MarKetLog::StopThreadRotator(void)
{
    m_Rotator.stop();
}

void MarKetLog::StartThreadLogWatcher(void)
{
    m_Watcher.start();
}

void MarKetLog::StopThreadLogWatcher(void)
{
    m_Watcher.stop();
}

void MarKetLog::start(void)
{
    m_MarKetLogThread = std::thread(&MarKetLog::MainThreadJKC, this);
    StartThreadRotator();
    StartThreadLogWatcher();
}

void MarKetLog::stop(void)
{
    if (m_MarKetLogThread.joinable())
    {
        m_MarKetLogThread.join();
    }
}

JKQueue<MARKETLOG_STATE_e>::EN_STS MarKetLog::MarKetLogWaitReceiveQueueSignal(MARKETLOG_STATE_e &npMs)
{
    JKQueue<MARKETLOG_STATE_e>::EN_STS luRval = JKQueue<MARKETLOG_STATE_e>::EN_STS::en_STS_SUCCESS;
	luRval = m_eQueue.ReceiveQueue(&npMs, -1);
	return luRval;
}

JKQueue<MARKETLOG_STATE_e>::EN_STS MarKetLog::MarKetLogSendQueueSignal(MARKETLOG_STATE_e &npMs)
{
    JKQueue<MARKETLOG_STATE_e>::EN_STS luRval = JKQueue<MARKETLOG_STATE_e>::EN_STS::en_STS_SUCCESS;
	luRval = m_eQueue.SendQueue(npMs);
	return luRval;
}

void MarKetLog::MarKetLogClockMutexLogContext(void)
{
    JK_LOGD("ClockMutexLogContext");
    JKSyncAuto cLck(&m_Context.m_cLogContextMutex);
}

void MarKetLog::MarKetLogNotifyToRotator(ROTATOR_FILES_e &npMs)
{
    // JK_LOGD("ClockMutexLogContext");
    // JKSyncAuto cLck(&m_Context.m_cLogContextMutex);
    m_Rotator.NotifyToRotator(npMs);
}

void MarKetLog::MarKetLogSendNotify(MARKETLOG_STATE_e &npMs)
{
    // JK_LOGD("ClockMutexLogContext");
    // JKSyncAuto cLck(&m_Context.m_cLogContextMutex);
    MarKetLogSendQueueSignal(npMs);    
}

void MarKetLog::MainThreadJKC(void)
{
    JKQueue<MARKETLOG_STATE_e>::EN_STS luRval = JKQueue<MARKETLOG_STATE_e>::EN_STS::en_STS_SUCCESS;
    MARKETLOG_STATE_e leMsgBuf;
    
    while (1)
    {
        luRval = MarKetLogWaitReceiveQueueSignal(leMsgBuf);
        if(luRval == JKQueue<MARKETLOG_STATE_e>::EN_STS::en_STS_SUCCESS)
        {
            switch(leMsgBuf)
            {
                
                case MARKETLOG_STATE_ACC_ON:
                {
                    JK_LOGI("ACC ON");
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    break;
                }

                case MARKETLOG_STATE_ACC_OFF:
                {
                    JK_LOGD("ACC OFF");
                    break;
                }      

                default:
                {
                    JK_LOGD("default");
                    break;
                }

            }
        }
    }
}
