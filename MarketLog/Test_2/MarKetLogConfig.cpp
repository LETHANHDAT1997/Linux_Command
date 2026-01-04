#include "MarKetLogConfig.h"
#include "JKDebug.h"

/*
 * Constructor
*/
MarKetLogConfig::MarKetLogConfig(LogContext& nLogContext, std::string nFilePath)
{
    m_ctx             = &nLogContext;
    m_strYAMLFilePath = nFilePath;
    if(IsLoadFileYAMLSuccess() != false)
    {
        JK_LOGD("Đọc tệp YAML thành công");
    }
    else
    {
        JK_LOGE("Đọc tệp YAML thất bại");
    }
}

/*
 * Destructor
*/
MarKetLogConfig::~MarKetLogConfig()
{

}

/*
 * Parse Config From YAML
*/
bool MarKetLogConfig::ParseConfigFromYAML(void)
{
    m_ctx->m_VerSion                = m_CYAMLConfig["VERSION"].as<double>();
    m_ctx->m_MaxLogFiles            = m_CYAMLConfig["MAX_LOG_FILES"].as<uint32_t>();
    m_ctx->m_MaxSizeLogFile         = m_CYAMLConfig["MAX_SIZE_LOG_FILE"].as<uint32_t>();
    // m_ctx->m_WriteLogBuffer         = m_CYAMLConfig["CAPACITY_DEFAULT_WRITE_LOG_BUFFER"].as<uint32_t>();
    // m_ctx->m_LogBuffer              = m_CYAMLConfig["CAPACITY_DEFAULT_LOG_BUFFER"].as<uint32_t>();
    m_ctx->m_DirPath                = m_CYAMLConfig["DEFAULT_DIR_PATH"].as<std::string>();
    m_ctx->m_FileNameWithoutIndex   = m_CYAMLConfig["DEFAULT_FILE_NAME_WITHOUT_INDEX"].as<std::string>();
    m_ctx->m_Extension              = m_CYAMLConfig["DEFAULT_EXTENSION"].as<std::string>();
    m_ctx->m_TempFileName           = m_CYAMLConfig["DEFAULT_TEMP_NAME_FILE"].as<std::string>();

    // In các giá trị ra màn hình
    JK_LOGD("VERSION: %lu" ,                          m_ctx->m_VerSion);
    JK_LOGD("MAX_LOG_FILES: %d" ,                     m_ctx->m_MaxLogFiles);
    JK_LOGD("MAX_SIZE_LOG_FILE: %d" ,                 m_ctx->m_MaxSizeLogFile);
    // JK_LOGD("CAPACITY_DEFAULT_WRITE_LOG_BUFFER: %d" , m_ctx->m_WriteLogBuffer);
    // JK_LOGD("CAPACITY_DEFAULT_LOG_BUFFER: %d" ,       m_ctx->m_LogBuffer);
    JK_LOGD("DEFAULT_DIR_PATH: %s" ,                  m_ctx->m_DirPath.c_str());
    JK_LOGD("DEFAULT_FILE_NAME_WITHOUT_INDEX: %s" ,   m_ctx->m_FileNameWithoutIndex.c_str());
    JK_LOGD("DEFAULT_EXTENSION: %s" ,                 m_ctx->m_Extension.c_str());
    JK_LOGD("DEFAULT_TEMP_NAME_FILE: %s" ,            m_ctx->m_TempFileName.c_str());
}

/*
 * Check YAML file is valid or not
*/
bool MarKetLogConfig::IsLoadFileYAMLSuccess(void)
{
    try
    {
        /* Load the YAML configuration file from the path m_strYAMLFilePath */
        m_CYAMLConfig = YAML::LoadFile(m_strYAMLFilePath);

        /* Check YAML content is valid or not */
        if (m_CYAMLConfig.IsNull()) 
        {
            return false;
        }
        else
        {
            /* If the YAML content is valid, proceed to parse the supporting data from the YAML */
            if( ParseConfigFromYAML() == false )
            {
                return false;
            }
            else
            {
                // Do nothing
            }
        }
    }
    /* If the YAML file cannot be opened (file does not exist or error), return false */
    catch (const YAML::BadFile& e)
    {
        JK_LOGE("YAML file cannot be opened (file does not exist or error)!");
        return false;
    }
    /* If there is an error parsing the YAML file, return false */
    catch (YAML::ParserException& e)
    {
        JK_LOGE("Parsing YAML file has an error!");
        return false;
    }
    /* If during the processing (when accessing nodes in ParseSupportFromYAML), an invalid node is encountered, this exception will occur */
    catch (const YAML::InvalidNode& e)
    {
        JK_LOGE("The YAML file has an invalid node!");
        return false;
    }
    catch(...)
    {
        /* Catch all other exceptions */
        JK_LOGE("The YAML file has unknown exception!");
        return false;
    }

    return true;

}

