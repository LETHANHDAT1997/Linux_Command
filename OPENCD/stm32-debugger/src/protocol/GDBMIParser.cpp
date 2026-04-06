#include "../../include/protocol/GDBMIParser.h"
#include <cctype>
#include <stdexcept>
#include <sstream>

// ─── Internal helpers ────────────────────────────────────────────────────────

void GDBMIParser::skipWs(std::string_view& s) {
    while (!s.empty() && (s[0] == ' ' || s[0] == '\t'))
        s.remove_prefix(1);
}

int GDBMIParser::consumeToken(std::string_view& s) {
    if (s.empty() || !std::isdigit(static_cast<unsigned char>(s[0])))
        return -1;
    int tok = 0;
    while (!s.empty() && std::isdigit(static_cast<unsigned char>(s[0]))) {
        tok = tok * 10 + (s[0] - '0');
        s.remove_prefix(1);
    }
    return tok;
}

std::string GDBMIParser::parseIdent(std::string_view& s) 
{
    size_t i = 0;
    while ( i < s.size() &&
            (   std::isalnum(static_cast<unsigned char>(s[i]))  || 
                s[i] == '-'                                     || 
                s[i] == '_'
            )
          )
    {
        ++i;
    }
    std::string ident(s.substr(0, i));
    s.remove_prefix(i);
    return ident;
}

std::string GDBMIParser::parseString(std::string_view& s) {
    if (s.empty() || s[0] != '"')
        throw std::runtime_error("GDBMIParser: expected '\"' at: " + std::string(s.substr(0, 20)));
    s.remove_prefix(1);
    std::string result;
    result.reserve(64);
    while (!s.empty() && s[0] != '"') {
        if (s[0] == '\\' && s.size() > 1) {
            switch (s[1]) {
                case 'n':  result += '\n'; break;
                case 'r':  result += '\r'; break;
                case 't':  result += '\t'; break;
                case '\\': result += '\\'; break;
                case '"':  result += '"';  break;
                default:   result += '\\'; result += s[1]; break;
            }
            s.remove_prefix(2);
        } else {
            result += s[0];
            s.remove_prefix(1);
        }
    }
    if (s.empty())
        throw std::runtime_error("GDBMIParser: unterminated string");
    s.remove_prefix(1); // consume closing "
    return result;
}

// Forward declaration
static MIValue doParseValue(std::string_view& s);

static MIValue doParseValue(std::string_view& s) {
    // skipWs handled by caller where needed
    if (s.empty())
        throw std::runtime_error("GDBMIParser: unexpected end of input");

    if (s[0] == '"') {
        // C-string
        MIValue v;
        // We can't call parseString from here because it's private.
        // Use the same inline logic:
        s.remove_prefix(1);
        std::string result;
        result.reserve(64);
        while (!s.empty() && s[0] != '"') {
            if (s[0] == '\\' && s.size() > 1) {
                switch (s[1]) {
                    case 'n':  result += '\n'; break;
                    case 'r':  result += '\r'; break;
                    case 't':  result += '\t'; break;
                    case '\\': result += '\\'; break;
                    case '"':  result += '"';  break;
                    default:   result += '\\'; result += s[1]; break;
                }
                s.remove_prefix(2);
            } else {
                result += s[0];
                s.remove_prefix(1);
            }
        }
        if (!s.empty()) s.remove_prefix(1); // closing "
        v.data = std::move(result);
        return v;
    }

    if (s[0] == '{') {
        // Tuple: { [result (, result)*] }
        s.remove_prefix(1); // consume '{'
        MIValue::Tuple t;
        while (!s.empty() && s[0] != '}') {
            // Parse key
            size_t ki = 0;
            while (ki < s.size() &&
                   (std::isalnum(static_cast<unsigned char>(s[ki])) || s[ki] == '-' || s[ki] == '_'))
                ++ki;
            std::string key(s.substr(0, ki));
            s.remove_prefix(ki);

            if (s.empty() || s[0] != '=')
                throw std::runtime_error("GDBMIParser: expected '=' in tuple");
            s.remove_prefix(1);

            t[key] = doParseValue(s);

            if (!s.empty() && s[0] == ',')
                s.remove_prefix(1);
        }
        if (!s.empty()) s.remove_prefix(1); // consume '}'
        MIValue v;
        v.data = std::move(t);
        return v;
    }

    if (s[0] == '[') {
        // List: [ (value | result) (, ...)* ]
        s.remove_prefix(1); // consume '['
        MIValue::List lst;
        while (!s.empty() && s[0] != ']') {
            // Lookahead: if next token is ident + '=', it's a result — consume key, push value.
            {
                std::string_view peek = s;
                size_t ki = 0;
                while (ki < peek.size() &&
                       (std::isalnum(static_cast<unsigned char>(peek[ki])) || peek[ki] == '-' || peek[ki] == '_'))
                    ++ki;
                if (ki > 0 && ki < peek.size() && peek[ki] == '=') {
                    // It's a result: key = value — consume key+= and parse value
                    s.remove_prefix(ki + 1);
                }
            }
            lst.push_back(doParseValue(s));
            if (!s.empty() && s[0] == ',')
                s.remove_prefix(1);
        }
        if (!s.empty()) s.remove_prefix(1); // consume ']'
        MIValue v;
        v.data = std::move(lst);
        return v;
    }

    // Unquoted value (rare — some GDB versions emit these for certain fields).
    size_t i = 0;
    while (i < s.size() && s[i] != ',' && s[i] != '}' && s[i] != ']' && s[i] != '\n' && s[i] != '\r')
        ++i;
    MIValue v;
    v.data = std::string(s.substr(0, i));
    s.remove_prefix(i);
    return v;
}

MIValue GDBMIParser::parseValue(std::string_view& s) {
    return doParseValue(s);
}

// ─── Public API ──────────────────────────────────────────────────────────────

MIRecord GDBMIParser::parseLine(std::string_view line) 
{
    MIRecord rec;

    // Strip trailing whitespace and line endings.
    while (!line.empty() && (line.back() == '\n' || line.back() == '\r' || line.back() == ' '))
        line.remove_suffix(1);

    if (line.empty() || line == "(gdb)")
        return rec; // Unknown

    // Optional numeric token.
    rec.token = consumeToken(line);

    if (line.empty()) return rec;

    char prefix = line[0];
    line.remove_prefix(1);

    switch (prefix) 
    {
        case '^': rec.type = MIRecord::Type::Result;       break;
        case '*': rec.type = MIRecord::Type::ExecAsync;    break;
        case '=': rec.type = MIRecord::Type::NotifyAsync;  break;
        case '+': rec.type = MIRecord::Type::StatusAsync;  break;
        case '~':
            rec.type = MIRecord::Type::ConsoleStream;
            if (!line.empty()) rec.payload.data = parseString(line);
            return rec;
        case '@':
            rec.type = MIRecord::Type::TargetStream;
            if (!line.empty()) rec.payload.data = parseString(line);
            return rec;
        case '&':
            rec.type = MIRecord::Type::LogStream;
            if (!line.empty()) rec.payload.data = parseString(line);
            return rec;
        default:
            rec.type = MIRecord::Type::Unknown;
            return rec;
    }

    // Class identifier: "done", "error", "running", "stopped", …
    rec.klass = parseIdent(line);

    // Result list: (,key=value)*
    MIValue::Tuple results;
    while (!line.empty() && line[0] == ',') {
        line.remove_prefix(1); // consume ','
        skipWs(line);
        std::string key = parseIdent(line);
        if (line.empty() || line[0] != '=') break;
        line.remove_prefix(1); // consume '='
        try {
            results[key] = doParseValue(line);
        } catch (...) {
            break; // best-effort: skip malformed trailing fields
        }
        skipWs(line);
    }

    if (!results.empty())
        rec.payload.data = std::move(results);

    return rec;
}
