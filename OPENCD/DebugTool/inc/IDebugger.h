#pragma once
#include "Types.h"
#include "IDebugEventListener.h"

class IDebugger {
public:
    virtual ~IDebugger() = default;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual bool connectToTarget(const std::string& host = "localhost", int port = 3333) = 0;
    virtual bool loadElf(const std::string& elfPath) = 0;
    
    virtual void run() = 0;
    virtual void halt() = 0;
    
    virtual DebugState state() const = 0;
    virtual void addListener(IDebugEventListener* l) = 0;
    virtual void removeListener(IDebugEventListener* l) = 0;
};