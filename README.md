# Linux Kernel API
Operating directly against the Linux Kernel ABI without standard headers or libc (<unistd.h>, <stdio.h>, <sys/socket.h>) means executing raw system calls directly at the hardware-software boundary.
 * Zero-Libc / No CRT: Completely bypasses standard C runtime overhead (glibc, musl). Execution starts directly at _start via pure assembly.
 * Direct Register ABI (x86_64): Passes parameters directly through CPU registers (rax for syscall number, rdi, rsi, rdx, r10, r8, r9) via the syscall instruction.
 * Inlined Kernel Data Structures: Wire formats, flag bitmasks, and ABI structures (e.g., io_uring_sqe, sockaddr_in) are mapped directly to match the kernel's internal uapi/ headers.
 * Freestanding Execution: Compiled with -ffreestanding -nostdlib to ensure zero hidden compiler dependencies, minimal binary size (less than 10 KB), and maximum execution speed.
