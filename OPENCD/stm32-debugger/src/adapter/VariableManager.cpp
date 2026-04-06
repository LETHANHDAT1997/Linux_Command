#include "../../include/adapter/VariableManager.h"
#include <future>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <memory>

void VariableManager::attach(GDBMIChannel& ch) {
    channel_ = &ch;
}

// ─── Core synchronous helper ──────────────────────────────────────────────────

MIRecord VariableManager::sendSync(std::string_view cmd) {
    // Use a shared_ptr so the lambda can outlive this stack frame if the timeout fires.
    auto promise = std::make_shared<std::promise<MIRecord>>();
    auto future  = promise->get_future();
    bool fulfilled = false;

    channel_->sendCommand(cmd,  [promise, &fulfilled](const MIRecord& rec) 
                                {
                                    if (!fulfilled) 
                                    {
                                        fulfilled = true;
                                        promise->set_value(rec);
                                    }
                                }
                         );

    if (future.wait_for(std::chrono::seconds{5}) != std::future_status::ready)
    {
        throw std::runtime_error("VariableManager: GDB command timed out: " + std::string(cmd));
    }
    
    return future.get();
}

// ─── evaluate ────────────────────────────────────────────────────────────────

std::string VariableManager::evaluate(const std::string& expr) 
{
    // -data-evaluate-expression "expr"
    MIRecord rec = sendSync("-data-evaluate-expression \"" + expr + "\"");

    if (rec.klass == "error") 
    {
        const MIValue& msg = rec.payload["msg"];
        return msg.isString() ? "error: " + msg.str() : "error";
    }

    const MIValue& val = rec.payload["value"];
    return val.isString() ? val.str() : "";
}

// ─── readMemory ───────────────────────────────────────────────────────────────

std::vector<uint8_t> VariableManager::readMemory(uint32_t addr, size_t len) 
{
    std::ostringstream oss;
    oss << "-data-read-memory-bytes 0x"
        << std::hex << std::uppercase << addr
        << " " << std::dec << len;

    MIRecord rec = sendSync(oss.str());
    std::vector<uint8_t> result;

    if (rec.klass != "done") return result;

    // ^done,memory=[{begin="0x...",end="0x...",offset="0x...",contents="AABBCC..."}]
    const MIValue& mem = rec.payload["memory"];
    if (!mem.isList() || mem.list().empty()) return result;

    const MIValue& block    = mem[0];
    const MIValue& contents = block["contents"];
    if (!contents.isString()) return result;

    const std::string& hex = contents.str();
    result.reserve(hex.size() / 2);

    for (size_t i = 0; i + 1 < hex.size(); i += 2) 
    {
        try 
        {
            result.push_back(   static_cast<uint8_t>(   std::stoul(hex.substr(i, 2), nullptr, 16)   ) );
        } 
        catch (...) 
        {
            break;
        }
    }
    return result;
}

// ─── readRegisters ───────────────────────────────────────────────────────────

std::string VariableManager::readRegisters() 
{
    // Get register names first so we can label each value.
    MIRecord namesRec = sendSync("-data-list-register-names");
    MIRecord valsRec  = sendSync("-data-list-register-values x");

    // Build a number → name map.
    std::vector<std::string> names;
    if (namesRec.klass == "done") 
    {
        const MIValue& nl = namesRec.payload["register-names"];
        if (nl.isList()) 
        {
            for (const auto& n : nl.list())
            {
                names.push_back(n.isString() ? n.str() : "?");
            }
        }
    }

    if (valsRec.klass != "done") return "";

    const MIValue& regs = valsRec.payload["register-values"];
    if (!regs.isList()) return "";

    std::ostringstream oss;
    for (const auto& rv : regs.list()) 
    {
        if (!rv.isTuple()) continue;

        const MIValue& numV = rv["number"];
        const MIValue& valV = rv["value"];
        if (!numV.isString() || !valV.isString()) continue;

        int num = -1;
        try { num = std::stoi(numV.str()); } catch (...) {}

        std::string name = (num >= 0 && static_cast<size_t>(num) < names.size())
                           ? names[static_cast<size_t>(num)]
                           : ("r" + numV.str());

        if (!name.empty())
            oss << name << "=" << valV.str() << "\n";
    }
    return oss.str();
}

// ─── backtrace ────────────────────────────────────────────────────────────────

std::vector<FrameInfo> VariableManager::backtrace() 
{
    MIRecord rec = sendSync("-stack-list-frames");
    std::vector<FrameInfo> frames;

    if (rec.klass != "done") return frames;

    // ^done,stack=[frame={level="0",addr="0x...",func="main",file="main.c",line="42"},...]
    const MIValue& stack = rec.payload["stack"];
    if (!stack.isList()) return frames;

    for (const auto& entry : stack.list()) 
    {
        // entry is the frame tuple (the "frame" key was consumed by the list parser)
        const MIValue& frame = entry.isTuple() ? entry : entry["frame"];
        if (!frame.isTuple()) continue;

        FrameInfo fi;
        try 
        {
            if (frame["level"].isString()) fi.level    = std::stoi(frame["level"].str());
            if (frame["addr"].isString())  fi.addr     = static_cast<uint32_t>(
                                               std::stoul(frame["addr"].str(), nullptr, 16));
            if (frame["func"].isString())  fi.function = frame["func"].str();
            if (frame["file"].isString())  fi.file     = frame["file"].str();
            if (frame["line"].isString())  fi.line     = std::stoi(frame["line"].str());
        } 
        catch (...) 
        {
            // Partial frame is still useful — include it.
        }
        frames.push_back(fi);
    }
    return frames;
}
