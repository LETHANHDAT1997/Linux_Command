#pragma once
#include "MIRecord.h"
#include <string_view>

// Stateless recursive-descent parser for GDB Machine Interface (MI) output.
// One raw line in → one MIRecord out.
// Throws std::runtime_error on unrecoverable parse errors.

class GDBMIParser {
public:
    static MIRecord parseLine(std::string_view line);

private:
    static int         consumeToken(std::string_view& s);
    static std::string parseIdent(std::string_view& s);
    static std::string parseString(std::string_view& s);
    static MIValue     parseValue(std::string_view& s);
    static MIValue     parseTuple(std::string_view& s);
    static MIValue     parseList(std::string_view& s);
    static void        skipWs(std::string_view& s);
};
