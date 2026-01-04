#include "MarketLogRotator.h"
#include "JKDebug.h"
#include "MarKetLog.h"
#include "MarKetLogUtility.h"

LogRotator::LogRotator(MarKetLog* npMarKetLog)
: m_pMarKetLogShareToRotator(npMarKetLog)
{

}

LogRotator::~LogRotator()
{
    // stop();
}

void LogRotator::SetMaxFiles(int maxFiles)
{
    m_maxFiles = maxFiles;
}

void LogRotator::SetLogContext(LogContext& ctx)
{
    m_ctx = &ctx;
}

void LogRotator::start(void)
{
    m_LogRotatorThread  = std::thread(&LogRotator::RunQueue, this);
}

void LogRotator::stop(void)
{
    if (m_LogRotatorThread.joinable())
    {
        m_LogRotatorThread.join();
    }
}

std::string LogRotator::RotateFileName(const std::string& filepath)
{
    std::string dir  = MarKetLogUtility::GetDirectory(filepath);
    std::string file = MarKetLogUtility::GetFilename(filepath);

    std::string base;
    std::string ext;
    MarKetLogUtility::SplitExtension(file, base, ext);

    std::string prefix;
    int index = 0;
    MarKetLogUtility::SplitNumericSuffix(base, prefix, index);

    index += 1;

    return dir + prefix + std::to_string(index) + ext;
}

void LogRotator::DeleteFilesByFileCountLimit(void)
{
    /* Get the total number of files */
    const size_t lsizedeque = m_ctx->files.size();
    JK_LOGD("Size deque = %d",lsizedeque);

    /* Check if the number of folders has exceeded the limit */
    if ( lsizedeque > m_maxFiles )
    {
        /* Calculate the number of files to delete */
        const size_t excess = lsizedeque - m_maxFiles;
        JK_LOGD("Chênh lệch là %d",excess);

        /* Check the file in order from bottom to top */
        for (size_t i = 0; i < excess; ++i)
        {
            /* The oldest file is always at the bottom */
            auto& oldFile = m_ctx->files.back();

            /* Try remove log file */
            try
            {
                /* Check log file is exists */
                if (std::filesystem::exists(oldFile.path))
                {
                    /* Delete log file */
                    std::filesystem::remove(oldFile.path);
                }
            }
            catch (const std::filesystem::filesystem_error& e)
            {

            }

            /* Delete the file from the context */
            m_ctx->files.pop_back();
        }
    }
}

void LogRotator::RotateFiles(void)
{
    /* Get the total number of files */
    const size_t lscanfile = m_ctx->files.size();

    /* Check the file in order from bottom to top */
    for (size_t i = lscanfile; i-- > 0;) 
    {
        /* Get file path */
        const std::string base = m_ctx->files[i].path;
        JK_LOGD("String base=%s", base.c_str());

        /* Split file name */
        std::string file = MarKetLogUtility::GetFilename(base);
        std::string tempfile = m_ctx->m_TempFileName + m_ctx->m_Extension;

        /* Check if the file name matches the temporary file name */
        if(file == tempfile)
        {
            /* Get the old file path and the new file path */
            std::string dir         = MarKetLogUtility::GetDirectory(base);
            std::string oldpathfile = base;
            std::string newpathfile = dir + m_ctx->m_FileNameWithoutIndex + m_ctx->m_Extension;
            JK_LOGD("oldpathfile is %s and newpathfile is %s",oldpathfile.c_str(),newpathfile.c_str());

            /* Try rename */
            try
            {
                /* Check log file is exists or not */
                if (std::filesystem::exists(oldpathfile)) 
                {
                    /* Rename */
                    std::filesystem::rename(oldpathfile, newpathfile);
                    JK_LOGD("Rename from %s to %s",oldpathfile.c_str(), newpathfile.c_str());

                    /* Reassign the new file path to the LogContext list */
                    m_ctx->files[i].path = newpathfile;
                }
                else
                {
                    JK_LOGE("Rename temp.log không được");
                }
            }
            catch(...)
            {
                JK_LOGE("Oldpathfile bị lỗi!");
            }
        }
        else
        {
            /* Get the old file path and the new file path */
            std::string from = base;
            std::string to   = RotateFileName(from);
            JK_LOGD("from is %s and to is %s",from.c_str(),to.c_str());

            /* Try rename */
            try 
            {
                /* Check log file is exists or not */
                if (std::filesystem::exists(from)) 
                {
                    /* Rename */
                    std::filesystem::rename(from, to);
                    JK_LOGD("Rename from %s to %s",from.c_str(),to.c_str());

                    /* Reassign the new file path to the LogContext list */
                    m_ctx->files[i].path = to;
                }
                else
                {
                    JK_LOGE("Rename log cũ không được");
                }
            }
            catch (...) 
            {
                JK_LOGE("File lỗi");
            }
        }

    }
}

void LogRotator::rotate(void)
{
    /* Delete files when the file limit is exceeded. */
    DeleteFilesByFileCountLimit();

    /* Debug */
    printdebug();

    /* Rotate files */
    RotateFiles();
}

void LogRotator::printdebug(void)
{
    for (const auto& f : m_ctx->files)
    {
        JK_LOGD("File=%s", f.path.c_str());
    }
}

JKQueue<ROTATOR_FILES_e>::EN_STS LogRotator::WaitReceiveQueueSignal(ROTATOR_FILES_e &npMs)
{
    JKQueue<ROTATOR_FILES_e>::EN_STS luRval = JKQueue<ROTATOR_FILES_e>::EN_STS::en_STS_SUCCESS;
	luRval = m_eQueue.ReceiveQueue(&npMs, -1);
	return luRval;
}

JKQueue<ROTATOR_FILES_e>::EN_STS LogRotator::SendQueueSignal(ROTATOR_FILES_e &npMs)
{
    JKQueue<ROTATOR_FILES_e>::EN_STS luRval = JKQueue<ROTATOR_FILES_e>::EN_STS::en_STS_SUCCESS;
	luRval = m_eQueue.SendQueue(npMs);
	return luRval;
}

void LogRotator::ClockMutexLogContext(void)
{
    JK_LOGD("ClockMutexLogContext");
    JKSyncAuto cLck(&m_ctx->m_cLogContextMutex);
}

void LogRotator::NotifyToRotator(ROTATOR_FILES_e &npMs)
{
    SendQueueSignal(npMs);
}

void LogRotator::RunQueue(void)
{
    JKQueue<ROTATOR_FILES_e>::EN_STS luRval = JKQueue<ROTATOR_FILES_e>::EN_STS::en_STS_SUCCESS;
    ROTATOR_FILES_e leMsgBuf;
    while (1)
    {
        luRval = WaitReceiveQueueSignal(leMsgBuf);
        if(luRval == JKQueue<ROTATOR_FILES_e>::EN_STS::en_STS_SUCCESS)
        {
            switch(leMsgBuf)
            {

                case ROTATOR_FILES_STATE_ROTATOR_ACC_ON:
                {
                    /* Khóa Siganl của LogContext, đảm bảo trong quá trình rotate không luồng nào được phép lấy thông tin của các tệp log */
                    ClockMutexLogContext();

                    /* Thực hiện rotate */
                    JK_LOGI("Thực hiện Rotate khi đang hoạt động");
                    rotate();
                    break;
                }

                case ROTATOR_FILES_STATE_ROTATOR_ACC_OFF:
                {
                    /* Khóa Siganl của LogContext, đảm bảo trong quá trình rotate không luồng nào được phép lấy thông tin của các tệp log */
                    ClockMutexLogContext();

                    /* Thực hiện rotate */
                    JK_LOGI("Thực hiện Rotate khi ACC OFF");
                    rotate();
                    return;
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