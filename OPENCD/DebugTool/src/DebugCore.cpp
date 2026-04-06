// --- DebugCore.cpp ---
#include "DebugCore.h"

void DebugCore::setBackend(std::unique_ptr<IDebugger> backend) {
    backend_ = std::move(backend);
}

bool DebugCore::startSession(const std::string& elfPath, const std::string& openocdConfig) 
{
    if (!backend_) return false;
    printf("Starting debug session with ELF: %s and OpenOCD config: %s\n", elfPath.c_str(), openocdConfig.c_str());
    if (!backend_->start()) return false;
    printf("Debugger started. Connecting to target...\n");
    if (!backend_->connectToTarget()) return false;
    printf("Connected to target. Loading ELF...\n");
    if (!backend_->loadElf(elfPath)) return false;
    printf("Debug session started successfully!\n");
    return true;
}

void DebugCore::endSession() {
    if (backend_) backend_->stop();
}

IDebugger* DebugCore::debugger() { return backend_.get(); }

void DebugCore::addListener(IDebugEventListener* l) {
    if (backend_) backend_->addListener(l);
}

void DebugCore::removeListener(IDebugEventListener* l) {
    if (backend_) backend_->removeListener(l);
}