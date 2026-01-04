#ifndef MARKET_LOG_CONFIG_H
#define MARKET_LOG_CONFIG_H
#include <iostream>
#include <yaml-cpp/yaml.h>
#include "MarketLogContext.h"

class MarKetLogConfig
{
public:
    MarKetLogConfig(LogContext& nLogContext, std::string nFilePath);
    ~MarKetLogConfig();
    bool IsLoadFileYAMLSuccess(void);

private:
    bool ParseConfigFromYAML(void);
    YAML::Node m_CYAMLConfig;
    std::string m_strYAMLFilePath;

    /* Con trỏ đến LogContext của MarKetLog */
    LogContext* m_ctx   = nullptr;
};

#endif