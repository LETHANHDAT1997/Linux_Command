#pragma once
#include <string>
#include <variant>
#include <map>
#include <vector>
#include <stdexcept>
#include <string_view>

// ─── MIValue ─────────────────────────────────────────────────────────────────
// Represents a parsed GDB MI value: a C-string, a tuple {key=value,...},
// or a list [value,...].

struct MIValue 
{
    using Tuple = std::map<std::string, MIValue>;
    using List  = std::vector<MIValue>;

    std::variant<std::monostate, std::string, Tuple, List> data;

    bool isNone()   const { return std::holds_alternative<std::monostate>(data); }
    bool isString() const { return std::holds_alternative<std::string>(data); }
    bool isTuple()  const { return std::holds_alternative<Tuple>(data); }
    bool isList()   const { return std::holds_alternative<List>(data); }

    const std::string& str() const 
    {
        if (!isString()) throw std::runtime_error("MIValue: expected string");
        return std::get<std::string>(data);
    }
    const Tuple& tuple() const 
    {
        if (!isTuple()) throw std::runtime_error("MIValue: expected tuple");
        return std::get<Tuple>(data);
    }
    const List& list() const 
    {
        if (!isList()) throw std::runtime_error("MIValue: expected list");
        return std::get<List>(data);
    }

    // Safe key lookup — returns a static empty MIValue if not found.
    const MIValue& operator[](std::string_view key) const 
    {
        static const MIValue empty;
        if (!isTuple()) return empty;
        const auto& t = std::get<Tuple>(data);
        auto it = t.find(std::string(key));
        return it != t.end() ? it->second : empty;
    }

    // Safe index — returns a static empty MIValue if out of range.
    const MIValue& operator[](size_t idx) const 
    {
        static const MIValue empty;
        if (!isList()) return empty;
        const auto& l = std::get<List>(data);
        return idx < l.size() ? l[idx] : empty;
    }
};

// ─── MIRecord ─────────────────────────────────────────────────────────────────
// One line of GDB MI output after parsing.
//
// GDB MI output families:
//   ^done,...        Result record     (response to a command)
//   *stopped,...     Exec async record (execution state change)
//   =thread-created  Notify async record
//   +download        Status async record
//   ~"console text"  Console stream
//   @"target text"   Target stream
//   &"log text"      Log stream

struct MIRecord 
{
    enum class Type 
    {
        Result,         // ^
        ExecAsync,      // *
        NotifyAsync,    // =
        StatusAsync,    // +
        ConsoleStream,  // ~
        TargetStream,   // @
        LogStream,      // &
        Unknown
    };

    Type        type  = Type::Unknown;
    int         token = -1;      // command token; -1 = absent
    std::string klass;           // "done" | "error" | "running" | "stopped" | ...
    MIValue     payload;         // parsed result / async data
};
