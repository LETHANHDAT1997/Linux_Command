#pragma once
#include "MIRecord.h"
#include <string_view>

class GDBMIParser {
public:
    static MIRecord parseLine(std::string_view line);
};