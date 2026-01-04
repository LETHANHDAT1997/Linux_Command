#include "MarKetLogUtility.h"
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>

/**
 * Constructor
 */
MarKetLogUtility::MarKetLogUtility(void)
{
    
}

/**
 * Destructor
 */
MarKetLogUtility::~MarKetLogUtility()
{
    
}

uint32_t MarKetLogUtility::ExtractLogIndex(const std::string& fullpath)
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
    {
        return 0;
    }
    
    // Lấy chuỗi số từ vị trí pos
    std::string numStr = stem.substr(pos);
    if (numStr.empty())
    {
        return 0;
    }
    
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

std::string MarKetLogUtility::GetDirectory(const std::string& path)
{
    auto pos = path.find_last_of("/\\");
    if (pos == std::string::npos)
    {
        return "";
    }

    return path.substr(0, pos + 1); // giữ dấu '/'
}

std::string MarKetLogUtility::GetFilename(const std::string& path)
{
    auto pos = path.find_last_of("/\\");
    if (pos == std::string::npos)
    {
        return path;
    }   

    return path.substr(pos + 1);
}

void MarKetLogUtility::SplitExtension(const std::string& filename, std::string& name, std::string& ext)
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

void MarKetLogUtility::SplitNumericSuffix(const std::string& name, std::string& prefix, int& index)
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

std::string MarKetLogUtility::RotateFileName(const std::string& filename)
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
