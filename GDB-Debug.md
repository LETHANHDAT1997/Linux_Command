I'll provide you with a comprehensive guide to debugging STM32 using arm-none-eabi-gdb.

## Hardware & Software Requirements

### Hardware
1. **STM32 Development Board** - Any STM32 microcontroller board
2. **Debug Probe** - One of the following:
   - ST-Link V2/V3 (official ST debugger)
   - J-Link (Segger)
   - Black Magic Probe
   - CMSIS-DAP compatible debugger
3. **USB Cable** - To connect debug probe to your computer
4. **Connection** - Debug probe connected to STM32 via SWD (SWDIO, SWCLK, GND, VCC) or JTAG pins

### Software
1. **ARM GCC Toolchain** - Contains `arm-none-eabi-gdb`
   - Linux: `sudo apt install gcc-arm-none-eabi gdb-multiarch`
   - macOS: `brew install --cask gcc-arm-embedded`
   - Windows: Download from ARM developer website

2. **GDB Server** - Depends on your debug probe:
   - **ST-Link**: OpenOCD or ST-Link GDB server
   - **J-Link**: JLinkGDBServer
   - **Black Magic Probe**: Built-in GDB server

3. **OpenOCD** (most common/versatile):
   - Linux: `sudo apt install openocd`
   - macOS: `brew install openocd`
   - Windows: Download from OpenOCD website

## Complete Debugging Steps

### Step 1: Start the GDB Server

#### Using OpenOCD (for ST-Link):
```bash
# Find your board configuration
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg

# Common board configurations:
# STM32F1: target/stm32f1x.cfg
# STM32F4: target/stm32f4x.cfg
# STM32L4: target/stm32l4x.cfg
# STM32H7: target/stm32h7x.cfg
```

OpenOCD will start and listen on port 3333 by default. You should see:
```
Info : Listening on port 3333 for gdb connections
```

#### Using J-Link:
```bash
JLinkGDBServer -device STM32F407VG -if SWD -speed 4000
```

### Step 2: Launch GDB

Open a new terminal and start GDB with your ELF file:
```bash
arm-none-eabi-gdb firmware.elf
```

### Step 3: Connect to GDB Server

In the GDB prompt:
```gdb
# Connect to OpenOCD (default port 3333)
(gdb) target extended-remote localhost:3333

# Or for J-Link (default port 2331)
(gdb) target extended-remote localhost:2331
```

### Step 4: Load the Firmware

```gdb
# Load the program into flash memory
(gdb) load

# Verify the program was loaded correctly
(gdb) compare-sections
```

### Step 5: Set Up Debugging

```gdb
# Reset and halt the processor
(gdb) monitor reset halt

# Set a breakpoint at main
(gdb) break main

# Or break at specific function
(gdb) break myFunction

# Continue execution (will stop at main)
(gdb) continue
```

### Step 6: Debug Your Code

Now you can use various GDB commands to debug (see commands section below).

## Essential GDB Commands Reference

### Program Control
```gdb
# Execution
continue (c)              # Continue execution
step (s)                  # Step into (enters functions)
next (n)                  # Step over (skips over functions)
finish                    # Run until current function returns
until <line>              # Run until specified line
stepi (si)                # Step one machine instruction (with entering)
nexti (ni)                # Step one machine instruction (without entering)

# Running
run (r)                   # Start program execution
kill                      # Kill the running program
quit (q)                  # Exit GDB
```

### Breakpoints
```gdb
# Setting breakpoints
break main                # Break at function
break file.c:42           # Break at file line
break *0x08001234         # Break at memory address
break main if x == 5      # Conditional breakpoint

# Managing breakpoints
info breakpoints (i b)    # List all breakpoints
delete <num>              # Delete breakpoint number
delete                    # Delete all breakpoints
disable <num>             # Disable breakpoint
enable <num>              # Enable breakpoint
clear <location>          # Clear breakpoint at location
```

### Watchpoints & Catchpoints
```gdb
watch variable            # Break when variable is written
rwatch variable           # Break when variable is read
awatch variable           # Break when variable is accessed
info watchpoints          # List watchpoints
```

### Examining Data
```gdb
# Print variables
print variable (p)        # Print variable value
print /x variable         # Print in hexadecimal
print /t variable         # Print in binary
print /d variable         # Print in decimal
print *pointer            # Dereference pointer

# Display (auto-print each stop)
display variable          # Auto-display variable
undisplay <num>           # Remove auto-display
info display              # List auto-displays

# Examine memory
x/nfu <address>           # Examine memory
  # n = count, f = format, u = unit
x/10x 0x20000000         # Show 10 hex words
x/4i $pc                  # Show 4 instructions at PC
x/s 0x08001000           # Show string at address

# Format: x(hex), d(decimal), u(unsigned), o(octal), t(binary), i(instruction), s(string), c(char)
# Unit: b(byte), h(halfword), w(word), g(giant 8-byte)
```

### Stack & Frames
```gdb
backtrace (bt)            # Show call stack
frame <num> (f)           # Select frame number
up                        # Move up one frame
down                      # Move down one frame
info frame                # Show current frame info
info args                 # Show function arguments
info locals               # Show local variables
```

### Registers
```gdb
info registers (i r)      # Show all registers
info registers r0 r1      # Show specific registers
print $r0                 # Print register value
set $r0 = 0x1234         # Modify register
```

### Memory & Variables
```gdb
set variable x = 10       # Set variable value
set {int}0x20000000 = 42 # Write to memory address
```

### Code Navigation
```gdb
list (l)                  # Show source code
list main                 # List source around function
list 50                   # List around line 50
disassemble main          # Disassemble function
disassemble /m main       # Disassemble with source
info functions            # List all functions
info variables            # List all variables
```

### OpenOCD Monitor Commands
```gdb
monitor reset halt        # Reset and halt target
monitor reset init        # Reset and initialize
monitor reset run         # Reset and run
monitor halt              # Halt the target
monitor flash write_image erase firmware.bin 0x08000000
```

## GDB Initialization Script (.gdbinit)

Create a file named `.gdbinit` in your project directory:

```gdb
# .gdbinit - GDB initialization script for STM32

# Don't ask for confirmation
set confirm off

# Enable history
set history save on
set history filename ~/.gdb_history
set history size 10000

# Print settings
set print pretty on
set print array on
set print array-indexes on

# Target configuration
target extended-remote localhost:3333

# Load symbols from ELF
file firmware.elf

# Monitor commands
monitor reset halt
monitor flash write_image erase firmware.elf

# Load program
load

# Reset
monitor reset halt

# Enable semihosting (optional, for printf debugging)
monitor arm semihosting enable

# Set breakpoint at main
break main

# Auto-display useful registers
display/x $sp
display/x $pc
display/x $lr

# Continue to main
continue

# Print message
echo \n=== Debugging session started ===\n
echo Type 'continue' to run the program\n\n
```

To use this script:
```bash
arm-none-eabi-gdb -x .gdbinit firmware.elf
```

## GDB Command Script for Automation

Create a file `debug_script.gdb`:

```gdb
# debug_script.gdb - Automated debugging script

# Connect to target
target extended-remote localhost:3333

# Reset and halt
monitor reset halt

# Flash the firmware
monitor flash write_image erase firmware.elf

# Load symbols
load

# Reset again
monitor reset halt

# Set breakpoints
break main
break HardFault_Handler
break error_handler

# Set up watchpoints (example)
# watch global_counter

# Display configuration
display/x $pc
display/x $sp

# Define custom commands
define reset_target
    monitor reset halt
    load
    monitor reset halt
end

define show_context
    info registers
    backtrace
    list
end

# Run to main
continue

# Automated test sequence example
# continue
# next 5
# print result
# continue
```

Run with:
```bash
arm-none-eabi-gdb firmware.elf -x debug_script.gdb
```

## Advanced GDB Scripting

### Python Scripting in GDB

Create `debug_helper.py`:

```python
# debug_helper.py - Python GDB script

import gdb

class ResetCommand(gdb.Command):
    """Custom reset command"""
    
    def __init__(self):
        super(ResetCommand, self).__init__("reset", gdb.COMMAND_USER)
    
    def invoke(self, arg, from_tty):
        gdb.execute("monitor reset halt")
        gdb.execute("load")
        gdb.execute("monitor reset halt")
        print("Target reset and firmware reloaded")

class ShowPeripheralCommand(gdb.Command):
    """Show peripheral register contents"""
    
    def __init__(self):
        super(ShowPeripheralCommand, self).__init__("show_peripheral", gdb.COMMAND_USER)
    
    def invoke(self, arg, from_tty):
        # Example: Read GPIOA registers
        base = 0x40020000
        gdb.execute(f"x/8x {base}")

# Register commands
ResetCommand()
ShowPeripheralCommand()

print("Custom GDB commands loaded")
```

Load in `.gdbinit`:
```gdb
source debug_helper.py
```

## Common Debugging Workflow Example

```gdb
# 1. Start GDB with script
arm-none-eabi-gdb firmware.elf -x .gdbinit

# 2. Check if stopped at main
(gdb) where

# 3. Set additional breakpoints
(gdb) break uart_send_data

# 4. Continue execution
(gdb) continue

# 5. Examine variables when stopped
(gdb) print buffer
(gdb) print /x status_register

# 6. Step through code
(gdb) next
(gdb) next
(gdb) step  # step into function

# 7. Check memory
(gdb) x/16x 0x20000000

# 8. Modify variable to test
(gdb) set variable error_count = 0

# 9. Continue and hit next breakpoint
(gdb) continue

# 10. When done
(gdb) quit
```

## Troubleshooting Tips

1. **Can't connect**: Check that OpenOCD/GDB server is running and listening
2. **Loading fails**: Verify correct OpenOCD target configuration
3. **Breakpoints not hit**: Ensure code is compiled with `-g` flag (debug symbols)
4. **Optimizations hide variables**: Compile with `-O0` or `-Og`
5. **SWD connection issues**: Check wiring, check board power, try lower speed

## Compilation Flags for Debugging

Ensure your firmware is compiled with proper debug flags:
```makefile
CFLAGS += -g3          # Maximum debug information
CFLAGS += -O0          # No optimization (or -Og for debug optimization)
CFLAGS += -gdwarf-4    # DWARF 4 debug format
```

This complete guide should get you debugging STM32 efficiently with GDB! Let me know if you need clarification on any specific part.