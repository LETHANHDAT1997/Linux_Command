#pragma once
#include "GDBMIChannel.h"
#include <string>

class VariableManager 
{
public:
    void attach(GDBMIChannel& ch);
    std::string evaluate(const std::string& expr);

private:
    MIRecord sendSync(std::string_view cmd);
    GDBMIChannel* channel_{nullptr};
};