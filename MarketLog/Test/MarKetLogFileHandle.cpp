#include "MarKetLogFileHandle.h"

SingleFileHandler::SingleFileHandler()
{

}

SingleFileHandler::~SingleFileHandler()
{

}

void SingleFileHandler::start()
{
    m_running                 = true;
    m_SingleFileHandlerThread = std::thread(&SingleFileHandler::run, this);
}

void SingleFileHandler::stop()
{
    m_running = false;
    if (m_SingleFileHandlerThread.joinable())
    {
        m_SingleFileHandlerThread.join();
    }
}

void SingleFileHandler::run()
{
    while (m_running) 
    {
        ExampleMakeFile(DEFAULT_DIR_PATH);
    }
}

void SingleFileHandler::SetMaxSizeLog(uint32_t maxSize)
{
    m_MaxSizeLog = maxSize;
}

void SingleFileHandler::SetLogContext(LogContext& ctx)
{
    m_ctx = &ctx;
}

void SingleFileHandler::ExampleMakeFile(const std::string& dir)
{
    /* =======================================================
     * 1. GỬI YÊU CẦU ROTATE
     * ======================================================= */
    // m_ctx->rotateRequested.store(true, std::memory_order_release);
    // m_ctx->cvRotate.notify_one();

    // while (true) 
    // {
        /* =======================================================
        * 2. CHỜ ROTATE HOÀN TẤT
        * ======================================================= */
        {
            std::unique_lock<std::mutex> lk(m_ctx->filesMutex);
            m_ctx->cvRotateDone.wait   (lk,    [&] 
                                            {
                                                return  !m_ctx->rotateRequested.load(std::memory_order_acquire) && 
                                                        !m_ctx->rotating.load(std::memory_order_acquire);
                                            }
                                    );
        }

        /* =======================================================
        * 3. TẠO temp.log MỚI
        * ======================================================= */
        std::filesystem::path tempPath = std::filesystem::path(dir) / "temp.log";

        {
            std::ofstream ofs(tempPath, std::ios::out | std::ios::trunc);
            if (!ofs.is_open())
            {
                throw std::runtime_error("CreateFile: cannot create temp.log");
            }
        } // RAII: file đóng ở đây

        /* =======================================================
        * 4. LẤY KÍCH THƯỚC FILE
        * ======================================================= */
        uint64_t fileSize = 0;
        try
        {
            fileSize = std::filesystem::file_size(tempPath);
        }
        catch (...)
        {
            fileSize = 0;
        }

        /* =======================================================
        * 5. THÊM METADATA VÀO m_ctx->files
        * ======================================================= */
        {
            std::lock_guard<std::mutex> lk(m_ctx->filesMutex);

            LogFileInfo info;
            info.path = tempPath.string();
            info.size = fileSize;

            m_ctx->files.push_front(info);
            printf("\nFile CreateFile =%s", info.path.c_str());

            m_ctx->rotateRequested.store(true, std::memory_order_release);
            m_ctx->cvRotate.notify_one();
        }

        /* =======================================================
        * 6. BÁO EVENT
        * ======================================================= */
        // m_ctx->lastEvent.store(LogEvent::FILE_UPDATED, std::memory_order_release);
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    // }
}