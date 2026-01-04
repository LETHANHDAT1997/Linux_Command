#include "LogWatcher.h"
#include "MarKetLog.h" 
#include <sys/inotify.h>
#include <unistd.h>
#include <iostream>

LogWatcher::LogWatcher(MarKetLog* npMarKetLog)
: m_pMarKetLogShareToLogWatcher(npMarKetLog)
{
    if (!initInotify())
    {
        JK_LOGE("Failed to initialize inotify");
    }

    if (!addWatch())
    {
        cleanupInotify();
        JK_LOGE("Failed to add inotify watch");
    }
}

LogWatcher::~LogWatcher()
{
    stop();
}

void LogWatcher::start()
{
    if (m_running.exchange(true))
    {
        return;
    }

    m_thread = std::thread(&LogWatcher::threadLoop, this);
}

void LogWatcher::stop()
{
    if (!m_running.exchange(false))
    {
        return;
    }

    if (m_thread.joinable())
        m_thread.join();
}

void LogWatcher::threadLoop()
{
    while (m_running.load())
    {
        handleEvents();

        /* Tránh busy loop
         * Có thể thay bằng poll/epoll sau */
        usleep(10 * 1000);
    }

    removeWatch();
    cleanupInotify();
}

bool LogWatcher::initInotify()
{
    m_inotifyFd = inotify_init1(IN_NONBLOCK);
    if (m_inotifyFd < 0)
    {
        JK_LOGE("Failed to initialize inotify");
        return false;
    }
    return true;
}

bool LogWatcher::addWatch()
{
    m_watchFd = inotify_add_watch(
        m_inotifyFd,
        m_ctx->m_DirPath.c_str(),
        IN_CREATE | IN_MOVED_TO | IN_CLOSE_WRITE
    );

    if (m_watchFd < 0)
    {
        JK_LOGE("Failed to add inotify watch");
        return false;
    }
    return true;
}

void LogWatcher::removeWatch()
{
    if (m_watchFd >= 0)
    {
        inotify_rm_watch(m_inotifyFd, m_watchFd);
        m_watchFd = -1;
    }
}

void LogWatcher::cleanupInotify()
{
    if (m_inotifyFd >= 0)
    {
        close(m_inotifyFd);
        m_inotifyFd = -1;
    }
}

void LogWatcher::SetLogContext(LogContext& ctx)
{
    m_ctx = &ctx;
}

void LogWatcher::ClockMutexLogContext(void)
{
    JK_LOGD("ClockMutexLogContext");
    JKSyncAuto cLck(&m_ctx->m_cLogContextMutex);
}

void LogWatcher::handleEvents()
{
    char buffer[4096];

    int len = read(m_inotifyFd, buffer, sizeof(buffer));
    if (len <= 0)
    {
        // JK_LOGE("Failed to read inotify events");
        return;
    }

    for (char* p = buffer; p < buffer + len; )
    {
        auto* ev = reinterpret_cast<struct inotify_event*>(p);

        if (ev->len > 0)
        {
            std::string fullPath = m_ctx->m_DirPath + "/" + ev->name;

            /* =====================================================
             * TẠI ĐÂY:
             *  - Bạn lấy inode bằng stat()
             *  - So với bảng inode của bạn
             *  - Phân biệt:
             *      + File mới thật
             *      + Rename / reopen
             * ===================================================== */

            if (ev->mask & IN_CREATE)
            {
                // File vừa được tạo (có thể là file tạm)
            }

            if (ev->mask & IN_MOVED_TO)
            {
                // File được rename/move vào thư mục
            }

            if (ev->mask & IN_CLOSE_WRITE)
            {
                // File đã ghi xong
                // -> check size
                // -> trigger rotate nếu vượt ngưỡng

                ClockMutexLogContext();
                /* Check size tệp và add tệp vào LogContext */
                uint64_t fileSize = 0;
                try
                {
                    fileSize = std::filesystem::file_size(fullPath);
                }
                catch (...)
                {
                    fileSize = 0;
                }
                LogFileInfo info;
                info.path = fullPath;
                info.size = fileSize;
                m_ctx->files.push_front(info);
                ROTATOR_FILES_e npMs = ROTATOR_FILES_STATE_ROTATOR_ACC_ON;
                m_pMarKetLogShareToLogWatcher->MarKetLogNotifyToRotator(npMs);
            }
        }

        p += sizeof(struct inotify_event) + ev->len;
    }
}
