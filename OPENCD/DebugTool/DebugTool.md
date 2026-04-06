Let me first load the design module, then build the architecture diagram.Here's the full redesign — architecture diagram first, then the complete class specification layer by layer.Click any node for a focused explanation of that class. Here is the complete design, layer by layer.

---

## Supporting structs (used everywhere)

```cpp
// debugger/Types.h

enum class DebugState { Idle, Connecting, Connected, Running, Stopped, Error };

struct BreakpointInfo {
    int         id;
    std::string file;
    int         line;
    bool        enabled = true;
};

struct FrameInfo {
    int         level;
    uint32_t    addr;
    std::string function;
    std::string file;
    int         line;
};

enum class DebugEventType { Stopped, Running, BreakpointHit, Disconnected, ConsoleOutput, Error };

struct DebugEvent {
    DebugEventType type;
    std::string    reason;   // "breakpoint-hit", "end-stepping-range", "signal-received"…
    FrameInfo      frame;    // where execution stopped (valid on Stopped/BreakpointHit)
    int            bkptId = -1;
    std::string    message;  // for Error and ConsoleOutput
};
```

---

## Layer C — MI protocol (`GDBMIParser`, `GDBMIWriter`, `GDBMIReader`, `GDBMIChannel`)

This is the lowest layer that talks directly to the GDB process. Everything here is GDB-specific and knows nothing about STM32 or OpenOCD.

### `MIRecord` and `MIValue`

GDB MI output has three distinct record families. Model them exactly:

```cpp
// protocol/MIRecord.h

struct MIValue {
    using Tuple = std::map<std::string, MIValue>;
    using List  = std::vector<MIValue>;

    std::variant<std::monostate, std::string, Tuple, List> data;

    bool        isString() const;
    bool        isTuple()  const;
    bool        isList()   const;

    std::string_view        str()   const;
    const Tuple&            tuple() const;
    const List&             list()  const;

    // Convenience: rec["frame"]["file"].str()
    const MIValue& operator[](std::string_view key) const;
    const MIValue& operator[](size_t index)          const;
};

struct MIRecord {
    enum class Type {
        Result,         // ^done / ^error / ^running
        ExecAsync,      // *stopped / *running
        NotifyAsync,    // =thread-created / =breakpoint-modified
        StatusAsync,    // +download
        ConsoleStream,  // ~"text"
        TargetStream,   // @"text"
        LogStream,      // &"text"
    };

    Type        type;
    int         token  = -1;    // -1 means absent
    std::string klass;          // "done", "error", "stopped", …
    MIValue     payload;        // parsed result/async data
};
```

### `GDBMIParser`

Stateless parser. Takes one raw line from GDB stdout, returns one `MIRecord`.

```cpp
// protocol/GDBMIParser.h

class GDBMIParser {
public:
    // Throws std::runtime_error on malformed input.
    static MIRecord parseLine(std::string_view line);

private:
    static MIValue  parseValue(std::string_view& s);
    static MIValue  parseTuple(std::string_view& s);
    static MIValue  parseList(std::string_view& s);
    static int      consumeToken(std::string_view& s);
};
```

### `GDBMIWriter`

Serialises a command with a token and writes it atomically to GDB's stdin fd.

```cpp
// protocol/GDBMIWriter.h

class GDBMIWriter {
public:
    void attach(int stdinFd);

    // Sends: "42-exec-continue\n"
    // Returns the token prepended.
    void write(int token, std::string_view command);

private:
    int        fd_{-1};
    std::mutex mtx_;   // serialize concurrent writes from different threads
};
```

### `GDBMIReader`

Owns the dedicated reader thread. Runs a `read()`-blocking loop on GDB's stdout, calls `GDBMIParser::parseLine()` for each line, then dispatches to `GDBMIChannel`.

```cpp
// protocol/GDBMIReader.h

class GDBMIReader {
public:
    using DispatchFn = std::function<void(MIRecord)>;

    void start(int stdoutFd, DispatchFn dispatch);
    void stop();    // signals thread to exit, joins

    ~GDBMIReader() { stop(); }

private:
    void           readerLoop();

    int            fd_{-1};
    DispatchFn     dispatch_;
    std::thread    thread_;
    std::atomic<bool> running_{false};
};
```

### `GDBMIChannel`

The central coordination point. Owns `GDBMIWriter` and `GDBMIReader`. Manages the pending-command map for token correlation. Called from both the main thread (to send commands) and the reader thread (to dispatch results).

```cpp
// protocol/GDBMIChannel.h

class GDBMIChannel {
public:
    using ResponseCb  = std::function<void(const MIRecord&)>;
    using AsyncCb     = std::function<void(const MIRecord&)>;
    using StreamCb    = std::function<void(std::string_view text)>;

    void attach(int stdinFd, int stdoutFd);
    void detach();

    // Thread-safe. Returns the token used.
    int sendCommand(std::string_view command, ResponseCb onResult = nullptr);

    // Called when an async record arrives (*stopped, =thread-created…)
    void setAsyncHandler(AsyncCb cb);

    // Called for console/target/log stream output
    void setStreamHandler(StreamCb cb);

    // Called by the reader thread for every parsed record.
    // Routes: result records → pending map; async → asyncHandler_; stream → streamHandler_.
    void dispatch(MIRecord rec);

private:
    std::atomic<int>                           nextToken_{1};
    std::mutex                                 pendingMtx_;
    std::unordered_map<int, ResponseCb>        pending_;

    AsyncCb     asyncHandler_;
    StreamCb    streamHandler_;

    GDBMIWriter writer_;
    GDBMIReader reader_;
};
```

---

## Layer B — Process management (`ProcessHandle`, `GDBProcess`, `OpenOCDProcess`)

These are RAII wrappers around subprocesses. They never parse MI — they just own the file descriptors.

```cpp
// process/ProcessHandle.h

class ProcessHandle {
public:
    virtual ~ProcessHandle() { terminate(); }

    bool  isRunning()  const;
    void  terminate();         // SIGTERM then SIGKILL after timeout

    int   stdinFd()  const { return stdinPipe_; }
    int   stdoutFd() const { return stdoutPipe_; }
    int   stderrFd() const { return stderrPipe_; }

protected:
    // Forks, sets up pipes, execs argv[0] with remaining args.
    bool spawnProcess(const std::vector<std::string>& argv);

    pid_t pid_{-1};
    int   stdinPipe_{-1};
    int   stdoutPipe_{-1};
    int   stderrPipe_{-1};
};
```

```cpp
// process/GDBProcess.h

class GDBProcess : public ProcessHandle {
public:
    // gdbPath: full path or just "arm-none-eabi-gdb" if on PATH.
    // The "--interpreter=mi" flag is added automatically.
    bool start(const std::string& gdbPath = "arm-none-eabi-gdb");
};
```

```cpp
// process/OpenOCDProcess.h

class OpenOCDProcess : public ProcessHandle {
public:
    bool start(const std::string& configFile,
               const std::string& openocdPath = "openocd",
               int gdbPort = 3333);

    // Polls the TCP port until OpenOCD is accepting connections.
    // Returns false on timeout (default 5 s).
    bool waitReady(std::chrono::milliseconds timeout = std::chrono::seconds{5});

private:
    int port_{3333};
};
```

---

## Layer B — Adapter components

These live inside `GDBDebugger`. Each wraps a group of related MI commands.

### `BreakpointManager`

```cpp
// adapter/BreakpointManager.h

class BreakpointManager {
public:
    using DoneCb = std::function<void(int id)>;  // id = -1 on failure

    void attach(GDBMIChannel& ch);

    // Async — fires cb when GDB confirms insertion.
    void add(const std::string& file, int line, DoneCb cb);
    void addAtAddress(uint32_t addr, DoneCb cb);
    void remove(int id, std::function<void()> cb);
    void enable(int id, bool on);

    // Synchronous query of local cache (always up to date after callbacks fire).
    const BreakpointInfo* find(int id) const;
    std::vector<BreakpointInfo> all() const;

private:
    GDBMIChannel*                             channel_{nullptr};
    mutable std::mutex                        mtx_;
    std::unordered_map<int, BreakpointInfo>   breakpoints_;

    int  parseId(const MIRecord& rec) const;
};
```

### `VariableManager`

The `GDBMIChannel` API is always async (callback-based). `VariableManager` wraps those callbacks with `std::promise`/`std::future` to give `IDebugger` callers a blocking API. **The caller must not invoke these from the UI thread** — the `DebugCore` facade handles dispatching to a worker thread.

```cpp
// adapter/VariableManager.h

class VariableManager {
public:
    void attach(GDBMIChannel& ch);

    std::string              evaluate(const std::string& expr);
    std::vector<uint8_t>     readMemory(uint32_t addr, size_t len);
    std::string              readRegisters();
    std::vector<FrameInfo>   backtrace();

private:
    GDBMIChannel* channel_{nullptr};

    // Generic helper: sends `cmd`, waits for result record, returns it.
    MIRecord sendSync(std::string_view cmd);
};
```

### `DebugStateMachine`

```cpp
// adapter/DebugStateMachine.h

class DebugStateMachine {
public:
    DebugState state() const;

    // Validates transition against the allowed table.
    // Throws std::logic_error on illegal transition.
    void transition(DebugState next);

private:
    std::atomic<DebugState> state_{DebugState::Idle};

    // Allowed transitions:
    // Idle        → Connecting
    // Connecting  → Connected | Error
    // Connected   → Running | Stopped | Idle
    // Running     → Stopped | Error | Idle
    // Stopped     → Running | Idle
    // Error       → Idle
    static bool isAllowed(DebugState from, DebugState to);
};
```

### `EventDispatcher`

```cpp
// adapter/EventDispatcher.h

class EventDispatcher {
public:
    void add(IDebugEventListener* l);
    void remove(IDebugEventListener* l);

    // Called from the reader thread. Acquires the lock, copies
    // the listener list, releases lock, then calls each listener.
    // (Lock not held during callbacks to avoid re-entrancy deadlock.)
    void dispatch(const DebugEvent& evt);

private:
    std::mutex                      mtx_;
    std::vector<IDebugEventListener*> listeners_;
};
```

---

## Layer A — Interface and facade

### `IDebugger`

```cpp
// IDebugger.h

class IDebugger {
public:
    virtual ~IDebugger() = default;

    // Lifecycle — must be called in order:
    // start() → connectToTarget() → loadElf() → run()/halt()
    virtual bool start() = 0;
    virtual void stop()  = 0;

    virtual bool connectToTarget(const std::string& host = "localhost",
                                  int port = 3333) = 0;
    virtual bool loadElf(const std::string& elfPath) = 0;
    virtual void reset(bool halt = true) = 0;

    // Execution control
    virtual void run()      = 0;
    virtual void halt()     = 0;
    virtual void stepInto() = 0;
    virtual void stepOver() = 0;
    virtual void stepOut()  = 0;

    // Breakpoints
    virtual int  addBreakpoint(const std::string& file, int line) = 0;
    virtual int  addBreakpointAtAddress(uint32_t addr)            = 0;
    virtual void removeBreakpoint(int id)                          = 0;
    virtual void enableBreakpoint(int id, bool enable)             = 0;

    // Inspection — block the calling thread until GDB replies.
    // Do NOT call these from the UI thread.
    virtual std::string            evaluate(const std::string& expr) = 0;
    virtual std::vector<uint8_t>   readMemory(uint32_t addr, size_t len) = 0;
    virtual std::string            readRegisters()                   = 0;
    virtual std::vector<FrameInfo> backtrace()                       = 0;

    // State
    virtual DebugState state() const = 0;

    // Event listeners (thread-safe)
    virtual void addListener(IDebugEventListener* l)    = 0;
    virtual void removeListener(IDebugEventListener* l) = 0;
};
```

### `GDBDebugger` — the adapter

```cpp
// GDBDebugger.h

class GDBDebugger : public IDebugger {
public:
    // IDebugger
    bool start() override;   // starts openocd → waits → starts gdb → attaches channel
    void stop()  override;

    bool connectToTarget(const std::string& host, int port) override;
    bool loadElf(const std::string& elfPath) override;
    void reset(bool halt) override;

    void run()      override;
    void halt()     override;
    void stepInto() override;
    void stepOver() override;
    void stepOut()  override;

    int  addBreakpoint(const std::string& file, int line) override;
    int  addBreakpointAtAddress(uint32_t addr)            override;
    void removeBreakpoint(int id)                          override;
    void enableBreakpoint(int id, bool enable)             override;

    std::string            evaluate(const std::string& expr)    override;
    std::vector<uint8_t>   readMemory(uint32_t addr, size_t len) override;
    std::string            readRegisters()                        override;
    std::vector<FrameInfo> backtrace()                            override;

    DebugState state() const override { return stateMachine_.state(); }

    void addListener(IDebugEventListener* l)    override { dispatcher_.add(l); }
    void removeListener(IDebugEventListener* l) override { dispatcher_.remove(l); }

private:
    // Called by GDBMIChannel when an async record arrives (*stopped, etc.)
    void onAsyncRecord(const MIRecord& rec);

    // Translates a *stopped MI record into a DebugEvent and dispatches it.
    void handleStopped(const MIRecord& rec);

    OpenOCDProcess    openocd_;
    GDBProcess        gdb_;
    GDBMIChannel      channel_;

    BreakpointManager breakpoints_;
    VariableManager   variables_;
    DebugStateMachine stateMachine_;
    EventDispatcher   dispatcher_;
};
```

### `DebugCore` — facade for the UI

```cpp
// DebugCore.h

class DebugCore {
public:
    void setBackend(std::unique_ptr<IDebugger> backend);

    // Convenience: starts the session end-to-end.
    // Internally: backend_->start() → connectToTarget() → loadElf()
    bool startSession(const std::string& elfPath,
                      const std::string& openocdConfig);
    void endSession();

    // Direct access to the IDebugger for everything else.
    // Returns nullptr if no backend is set.
    IDebugger* debugger();

    // Listener forwarding helpers.
    void addListener(IDebugEventListener* l);
    void removeListener(IDebugEventListener* l);

private:
    std::unique_ptr<IDebugger> backend_;
};
```

---

## Threading model

```
Main thread (UI worker)         Reader thread (inside GDBMIReader)
────────────────────────        ──────────────────────────────────
DebugCore::startSession()       blocks on read(stdoutFd)
  GDBMIChannel::sendCommand()      parses one MI line
    GDBMIWriter::write()           GDBMIChannel::dispatch()
    registers callback in map        if result record:
                                       pendingMtx_.lock()
                                       find callback by token
                                       pendingMtx_.unlock()
                                       call callback()  ← on reader thread
                                     if async record:
                                       asyncHandler_()  ← on reader thread
                                         GDBDebugger::onAsyncRecord()
                                           EventDispatcher::dispatch()
                                             IDebugEventListener::onDebugEvent()
                                               [UI schedules a UI-thread update]
```

Key rules that follow from this:
- `IDebugEventListener::onDebugEvent()` is **always called from the reader thread**. The UI must post a message/signal to its own thread — never touch UI widgets directly in the callback.
- `VariableManager::sendSync()` blocks the calling thread with `future.get()`. The reader thread resolves the promise via the response callback. Never call `evaluate()` from the reader thread itself — that would deadlock.
- The `pendingMtx_` protects the map. Callbacks are invoked **after releasing the lock** to avoid re-entrancy.

---

## Startup sequence

```
DebugCore::startSession(elfPath, openocdConfig)
  ├── OpenOCDProcess::start(openocdConfig)
  ├── OpenOCDProcess::waitReady()          ← polls TCP :3333 up to 5 s
  ├── GDBProcess::start()
  ├── GDBMIChannel::attach(gdb.stdinFd, gdb.stdoutFd)
  │     └── GDBMIReader::start()           ← reader thread begins
  ├── sendCommand("-gdb-set target-async on")
  ├── IDebugger::connectToTarget()
  │     └── sendCommand("-target-select remote localhost:3333")
  ├── IDebugger::loadElf(elfPath)
  │     └── sendCommand("-file-exec-and-symbols " + elfPath)
  └── stateMachine_.transition(Connected)
```

Shutdown reverses the order: stop GDB first (so it closes the TCP connection cleanly), then stop OpenOCD.

---

## Flow: breakpoint hit

```
hardware hits breakpoint
    → OpenOCD sends stop packet over TCP :3333
        → GDB emits: *stopped,reason="breakpoint-hit",bkptno="2",frame={...}
            → GDBMIReader reads the line
                → GDBMIParser::parseLine() → MIRecord{ExecAsync, klass="stopped"}
                    → GDBMIChannel::dispatch() → asyncHandler_()
                        → GDBDebugger::onAsyncRecord()
                            → handleStopped()
                                → stateMachine_.transition(Stopped)
                                → build DebugEvent{BreakpointHit, frame, bkptId=2}
                                → EventDispatcher::dispatch(evt)
                                    → UI listener posts update to UI thread
```

---

## Class count by layer

| Layer | Classes / structs |
|---|---|
| Supporting types | `BreakpointInfo`, `FrameInfo`, `DebugEvent`, `DebugState`, `DebugEventType` |
| Process management | `ProcessHandle`, `GDBProcess`, `OpenOCDProcess` |
| MI protocol | `MIValue`, `MIRecord`, `GDBMIParser`, `GDBMIWriter`, `GDBMIReader`, `GDBMIChannel` |
| Adapter components | `BreakpointManager`, `VariableManager`, `DebugStateMachine`, `EventDispatcher` |
| Abstraction | `IDebugger`, `IDebugEventListener`, `GDBDebugger`, `DebugCore` |

That is 22 types total, but the separation is non-negotiable — each class has exactly one reason to change. If you swap OpenOCD for a J-Link server, only `OpenOCDProcess` changes. If GDB's MI format changes, only `GDBMIParser` changes. If you later add an LLDB backend, you write a new `IDebugger` implementation and `DebugCore` doesn't notice.
