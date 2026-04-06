#include "../../include/process/GDBProcess.h"

bool GDBProcess::start(const std::string& gdbPath) {
    // --interpreter=mi  : enable the Machine Interface protocol
    // --quiet           : suppress the GDB banner (reduces noise on stdout)
    // --nx              : skip .gdbinit (reproducible behaviour)
    return spawnProcess({gdbPath, "--interpreter=mi", "--quiet", "--nx"});
}
