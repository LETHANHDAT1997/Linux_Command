#ifndef MARKET_LOG_UTILITY_H
#define MARKET_LOG_UTILITY_H
#include <cstdint>
#include <cstring>
#include <string>

class MarKetLogUtility
{
public:
    /**
     * Constructor
     */
    MarKetLogUtility();

    /**
     * Destructor
     */
    ~MarKetLogUtility();

    static uint32_t     ExtractLogIndex(const std::string& fullpath);
    static std::string  GetDirectory(const std::string& path);
    static std::string  GetFilename(const std::string& path);
    static std::string  RotateFileName(const std::string& base);
    static void         SplitExtension(const std::string& filename, std::string& name, std::string& ext);
    static void         SplitNumericSuffix(const std::string& name, std::string& prefix, int& index);
};

#endif
