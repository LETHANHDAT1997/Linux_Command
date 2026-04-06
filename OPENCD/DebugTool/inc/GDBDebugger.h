#pragma once
#include "IDebugger.h"
#include "ProcessHandle.h"
#include "GDBMIChannel.h"
#include "VariableManager.h"
#include "DebugStateMachine.h"
#include "EventDispatcher.h"

class GDBDebugger : public IDebugger {
public:
    bool start() override;
    void stop() override;
    bool connectToTarget(const std::string& host, int port) override;
    bool loadElf(const std::string& elfPath) override;
    
    void run() override;
    void halt() override;
    
    DebugState state() const override;
    void addListener(IDebugEventListener* l) override;
    void removeListener(IDebugEventListener* l) override;

private:
    void onAsyncRecord(const MIRecord& rec);
    MIRecord executeSync(std::string_view cmd, int timeoutMs = 2000);

    OpenOCDProcess    openocd_;
    GDBProcess        gdb_;
    GDBMIChannel      channel_;
    VariableManager   variables_;
    DebugStateMachine stateMachine_;
    EventDispatcher   dispatcher_;
};