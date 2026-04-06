#pragma once
#include "Types.h"

struct MIValue 
{
    using Tuple = std::map<std::string, MIValue>;
    using List  = std::vector<MIValue>;

    std::variant<std::monostate, std::string, Tuple, List> data;

    bool isString() const { return std::holds_alternative<std::string>(data); }
    bool isTuple()  const { return std::holds_alternative<Tuple>(data); }
    bool isList()   const { return std::holds_alternative<List>(data); }

    std::string str() const { 
        if (isString()) return std::get<std::string>(data); 
        return ""; 
    }
};

struct MIRecord 
{
    enum class Type { Result, ExecAsync, NotifyAsync, StatusAsync, ConsoleStream, TargetStream, LogStream, Unknown };
    Type        type = Type::Unknown;
    int         token = -1;
    std::string klass;
    MIValue     payload;
};