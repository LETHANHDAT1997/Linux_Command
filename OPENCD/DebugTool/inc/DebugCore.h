#pragma once
#include "IDebugger.h"
#include <memory>
#include <string>

class DebugCore {
public:
    void setBackend(std::unique_ptr<IDebugger> backend);
    bool startSession(const std::string& elfPath, const std::string& openocdConfig);
    void endSession();
    IDebugger* debugger();
    
    void addListener(IDebugEventListener* l);
    void removeListener(IDebugEventListener* l);

private:
    std::unique_ptr<IDebugger> backend_;
};