#include "../include/DebugCore.h"
#include "../include/adapter/GDBDebugger.h"

void DebugCore::setBackend(std::unique_ptr<IDebugger> backend) {
    backend_ = std::move(backend);
}

bool DebugCore::startSession(const std::string& elfPath,
                              const std::string& openocdConfig,
                              const std::string& gdbPath,
                              const std::string& openocdPath) {
    // If no backend has been injected, create the default GDB one.
    if (!backend_) 
    {
        auto gdb = std::make_unique<GDBDebugger>();
        gdb->configure(openocdConfig, gdbPath, openocdPath);
        backend_ = std::move(gdb);
    }

    if (!backend_->start())            return false;
    if (!backend_->connectToTarget())  return false;
    if (!backend_->loadElf(elfPath))   return false;
    return true;
}

void DebugCore::endSession() {
    if (backend_) backend_->stop();
}

IDebugger* DebugCore::debugger() {
    return backend_.get();
}

void DebugCore::addListener(IDebugEventListener* l) {
    if (backend_) backend_->addListener(l);
}

void DebugCore::removeListener(IDebugEventListener* l) {
    if (backend_) backend_->removeListener(l);
}
