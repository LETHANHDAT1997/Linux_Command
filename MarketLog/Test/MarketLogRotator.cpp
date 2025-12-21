#include "MarketLogRotator.h"

LogRotator::LogRotator()
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

void LogRotator::start()
{
    m_running           = true;
    m_LogRotatorThread  = std::thread(&LogRotator::run, this);
}

void LogRotator::stop()
{
    m_running = false;

    if (m_LogRotatorThread.joinable())
    {
        m_LogRotatorThread.join();
    }
}

void LogRotator::run()
{
    while (m_running)
    {
        /* 1. Chờ yêu cầu rotate */
        {
            std::unique_lock<std::mutex> lk(m_ctx->filesMutex);
            m_ctx->cvRotate.wait (lk,    [&] 
                                        {
                                            return  m_ctx->rotateRequested.load(std::memory_order_acquire) || 
                                                    !m_running;
                                        }
                                );
        }

        if (!m_running)
            break;

        /* 2. Đánh dấu đang rotate */
        m_ctx->rotating.store(true, std::memory_order_release);
        m_ctx->rotateRequested.store(false, std::memory_order_release);

        /* 3. Rotate thật sự (KHÔNG GIỮ MUTEX) */
        rotate();

        /* 4. Kết thúc rotate */
        m_ctx->rotating.store(false, std::memory_order_release);
        m_ctx->cvRotateDone.notify_all();
    }
}

std::string LogRotator::GetDirectory(const std::string& path)
{
    auto pos = path.find_last_of("/\\");
    if (pos == std::string::npos)
        return "";

    return path.substr(0, pos + 1); // giữ dấu '/'
}

std::string LogRotator::GetFilename(const std::string& path)
{
    auto pos = path.find_last_of("/\\");
    if (pos == std::string::npos)
        return path;

    return path.substr(pos + 1);
}

void LogRotator::SplitExtension(const std::string& filename, std::string& name, std::string& ext)
{
    auto dotPos = filename.rfind('.');
    if (dotPos == std::string::npos)
    {
        name = filename;
        ext.clear();
        return;
    }

    name = filename.substr(0, dotPos);
    ext  = filename.substr(dotPos); // ".log"
}

void LogRotator::SplitNumericSuffix(const std::string& name, std::string& prefix, int& index)
{
    int i = static_cast<int>(name.size()) - 1;
    while (i >= 0 && std::isdigit(static_cast<unsigned char>(name[i])))
    {
        --i;
    }

    prefix = name.substr(0, i + 1);

    if (i + 1 < static_cast<int>(name.size()))
    {
        index = std::stoi(name.substr(i + 1));
    }
    else
    {
        index = 0;
    }   
}

std::string LogRotator::RotateFileName(const std::string& filename)
{
    std::string dir  = GetDirectory(filename);
    std::string file = GetFilename(filename);

    std::string base;
    std::string ext;
    SplitExtension(file, base, ext);

    std::string prefix;
    int index = 0;
    SplitNumericSuffix(base, prefix, index);

    index += 1;

    return dir + prefix + std::to_string(index) + ext;
}

void LogRotator::DeleteFilesWhenExceedLimit(void)
{
    // std::lock_guard<std::mutex> lock(m_ctx->filesMutex);
    // 1. XÓA FILE CŨ NẾU VƯỢT QUÁ SỐ LƯỢNG GIỚI HẠN
    const size_t lsizedeque = m_ctx->files.size();
    printf("\nsize deque = %d",lsizedeque);
    if ( lsizedeque > m_maxFiles )
    {
        const size_t excess = lsizedeque - m_maxFiles;
        printf("\nChênh lệch là %d",excess);
        for (size_t i = 0; i < excess; ++i)
        {
            // File cũ nhất luôn nằm ở cuối
            auto& oldFile = m_ctx->files.back();

            try
            {
                if (std::filesystem::exists(oldFile.path))
                {
                    std::filesystem::remove(oldFile.path);
                }
            }
            catch (const std::filesystem::filesystem_error& e)
            {

            }

            // Xóa luôn metadata khỏi context
            m_ctx->files.pop_back();
        }
    }
}

void LogRotator::RotateFiles(void)
{
    // std::lock_guard<std::mutex> lock(m_ctx->filesMutex);
    // 2. Rotate files
    const size_t lscanfile = m_ctx->files.size();
    for (size_t i = lscanfile; i-- > 0;) 
    {
        const std::string base = m_ctx->files[i].path;
        printf("\nbase=%s", base.c_str());
        std::string file = GetFilename(base);
        if(file == "temp.log")
        {
            std::string dir         = GetDirectory(base);
            std::string oldpathfile = dir + file;
            std::string newpathfile = dir + "log.log";
            printf("\nGet oldpathfile is %s and newpathfile is %s",oldpathfile.c_str(),newpathfile.c_str());
            try
            {
                if (std::filesystem::exists(oldpathfile)) 
                {
                    std::filesystem::rename(oldpathfile, newpathfile);
                    printf("\nRename from %s to %s",oldpathfile.c_str(),newpathfile.c_str());
                    // >>> CẬP NHẬT METADATA <<<
                    m_ctx->files[i].path = newpathfile;
                }
                else
                {
                    printf("\nRename temp.log không được");
                }
            }
            catch(...)
            {
                printf("\noldpathfile bị lỗi!");
            }
        }
        else
        {
            std::string from = base;
            std::string to   = RotateFileName(from);
            printf("\nGet from is %s and to is %s",from.c_str(),to.c_str());
            try 
            {
                if (std::filesystem::exists(from)) 
                {
                    std::filesystem::rename(from, to);
                    printf("\nRename from %s to %s",from.c_str(),to.c_str());
                    // >>> CẬP NHẬT METADATA <<<
                    m_ctx->files[i].path = to;
                }
                else
                {
                    printf("\nRename log cũ không được");
                }
            } 
            catch (...) 
            {
                printf("\nFile lỗi");
            }
        }

    }
}

void LogRotator::rotate()
{
    /* Delete files when the file limit is exceeded. */
    DeleteFilesWhenExceedLimit();

    /* Debug */
    printdebug();

    /* Rotate files */
    RotateFiles();
}

void LogRotator::printdebug(void)
{
    // Debug
    // std::lock_guard<std::mutex> lock(m_ctx->filesMutex);
    for (const auto& f : m_ctx->files)
    {
        printf("\nFile=%s", f.path.c_str());
    }
}
