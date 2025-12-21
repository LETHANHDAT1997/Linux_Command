#include "MarKetLog.h"

MarKetLog::MarKetLog()
{
    ScanExistingLogs(DEFAULT_DIR_PATH, DEFAULT_EXTENSION);
}

MarKetLog::~MarKetLog()
{
    // StopThreadRotator();
    StopThreadFileHandle();
}

uint32_t MarKetLog::ExtractLogIndex(const std::string& fullpath)
{
    const std::filesystem::path p(fullpath);
    const std::string stem = p.stem().string();

    std::size_t pos = stem.size();

    // Lùi từ cuối stem để tìm dãy số liên tiếp
    while (pos > 0 && std::isdigit(static_cast<unsigned char>(stem[pos - 1])))
    {
        --pos;
    }

    // Không có chữ số nào ở cuối
    if (pos >= stem.size())
        return 0;
    
    // Lấy chuỗi số từ vị trí pos
    std::string numStr = stem.substr(pos);
    if (numStr.empty())
        return 0;
    
    // Ép kiểu thành số với xử lý ngoại lệ
    try 
    {
        return static_cast<std::uint32_t>(std::stoul(numStr));
    }
    catch (...) 
    {
        return 0;
    }
}

void MarKetLog::ScanExistingLogs(const std::string& logDir, const std::string& extension)
{
    // std::lock_guard<std::mutex> lock(m_Context.filesMutex);
    m_Context.files.clear();

    if (!std::filesystem::exists(logDir) || !std::filesystem::is_directory(logDir))
    {
        std::filesystem::create_directories(logDir);
        if (!std::filesystem::exists(logDir) || !std::filesystem::is_directory(logDir))
        {
            printf("\nLỗi tạo thư mục!");
            return;
        }
    }

    for (const auto& entry : std::filesystem::directory_iterator(logDir)) 
    {
        if (!entry.is_regular_file())
            continue;

        const auto& path = entry.path();
        if (path.extension() != extension)
            continue;

        LogFileInfo info;
        info.path = path.string();
        info.size = std::filesystem::file_size(path);
        
        m_Context.files.push_back(info);
        printf("\nTìm thấy tệp %s!",info.path.c_str());
    }

    if(m_Context.files.size() > 0 )
    {
        // Sắp xếp để deterministic (quan trọng cho rotate)
        std::sort(m_Context.files.begin(), m_Context.files.end(),
                                                        [this](const LogFileInfo& a, const LogFileInfo& b) 
                                                        {
                                                            const uint32_t ia = ExtractLogIndex(a.path);
                                                            const uint32_t ib = ExtractLogIndex(b.path);
                                                            return ia < ib;
                                                        }
                );
    }
    for (const auto& f : m_Context.files)
    {
        printf("\nFile After Sort=%s", f.path.c_str());
    }
}

void MarKetLog::StartThreadRotator(void)
{
    m_Rotator.SetMaxFiles(MAX_FILES_LOG);
    m_Rotator.SetLogContext(m_Context);
    m_Rotator.start();
}

void MarKetLog::StopThreadRotator(void)
{
    m_Rotator.stop();
}

void MarKetLog::StartThreadFileHandle(void)
{
    m_FileHandler.SetMaxSizeLog(MAX_SIZE_LOG);
    m_FileHandler.SetLogContext(m_Context);
    m_FileHandler.start();
}

void MarKetLog::StopThreadFileHandle(void)
{
    m_FileHandler.stop();
}
