# IlliniX - Educational Operating System Kernel

## Overview

IlliniX is a fully-functional educational operating system kernel developed for RISC-V architecture. This project was completed as part of an Operating Systems course, implementing core OS functionality from scratch including process management, memory management, device drivers, and a file system.

## Key Features

### Core Kernel Components
- **Process Management**: Complete implementation of process creation (`fork`), execution (`exec`), and termination (`exit`)
- **Thread Management**: Multi-threading support with context switching and scheduling
- **Memory Management**: Virtual memory with paging, heap allocation, and memory protection
- **Interrupt & Exception Handling**: Full interrupt controller support (PLIC) with timer interrupts
- **System Calls**: Comprehensive system call interface for user-space programs

### I/O and Device Support
- **Device Drivers**: 
  - UART (NS16550a) serial device drivers for console I/O
  - VirtIO block device drivers for storage
- **Device Manager**: Unified device management interface
- **File System**: Custom file system implementation with file operations (open, read, write, close)
- **Pipes**: Inter-process communication through pipes

### Synchronization
- **Locking Mechanisms**: Mutual exclusion primitives for concurrent access protection
- **Reference Counting**: Proper resource management with reference counting

### User Space
- **Shell**: Interactive command-line interface with built-in commands:
  - `ls` - List files
  - `cat` - Display file contents
  - Program execution support
- **Test Programs**: Comprehensive test suite including:
  - `fib` - Fibonacci sequence calculation
  - `lock_test` - Concurrency testing
  - `refcnt` - Reference counting validation
  - `pipe_test` - Pipe functionality testing
  - Various initialization programs for different test scenarios

## Project Structure

```
IlliniX/
├── src/
│   ├── kern/          # Kernel source code
│   │   ├── main.c     # Kernel entry point
│   │   ├── process.c  # Process management
│   │   ├── thread.c   # Thread management
│   │   ├── memory.c   # Memory management
│   │   ├── syscall.c  # System call handlers
│   │   ├── intr.c     # Interrupt handling
│   │   ├── kfs.c      # File system implementation
│   │   ├── pipe.c     # Pipe implementation
│   │   └── ...        # Other kernel components
│   ├── user/          # User-space programs
│   │   ├── shell.c    # Interactive shell
│   │   ├── init*.c    # Various initialization programs
│   │   └── ...        # Test programs
│   └── util/          # Utilities and build tools
├── grades_and_notes/  # Course grades and documentation
└── README.md          # This file
```

## Building and Running

### Prerequisites
- RISC-V toolchain (gcc, binutils)
- QEMU for RISC-V emulation
- Make build system

### Quick Start

1. **Select Test Program**: Edit `src/kern/main.c` to choose the initial user program:
```c
#define INIT_PROC "shell"  // or "fib", "lock_test", etc.
```

2. **Build File System Image**: Create the initial file system with user programs:
```bash
sh get_init_fsimg.sh
```

3. **Run the Kernel**:
```bash
make run-kernel
```

## Testing

The kernel includes comprehensive testing capabilities:

- **Concurrency Testing**: Use `lock_test` to verify proper synchronization
- **Memory Management**: Test reference counting with `refcnt`
- **IPC Testing**: Validate pipe functionality with `pipe_test`
- **System Integration**: Run various `init_*` programs for different test scenarios

## Technical Highlights

- **RISC-V Architecture**: Native support for RISC-V 64-bit instruction set
- **Modular Design**: Clean separation between kernel and user space
- **Hardware Abstraction**: Device-independent I/O interface
- **Robust Error Handling**: Comprehensive error checking and panic recovery
- **Educational Focus**: Well-documented code suitable for learning OS concepts

## Contributors

This project was developed by:
- **Rick Xu** (rickxu2)
- **Ziheng Qi** (zihengq2)
- **Ziyi Wang** (zw67)

## Acknowledgments

Special thanks to the course instructors, TAs, and CAs who provided guidance throughout this challenging but rewarding project. This implementation demonstrates a deep understanding of operating system principles and low-level system programming.

---

*"Hard work will eventually pay off."* - The IlliniX Team