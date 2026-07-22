#ifndef KRAW_SYSCALL_H
#define KRAW_SYSCALL_H

/*
 * Linux x86_64 System Call Numbers (Kernel ABI)
 */
#define SYS_READ                0
#define SYS_WRITE               1
#define SYS_OPEN                2
#define SYS_CLOSE               3
#define SYS_STAT                4
#define SYS_FSTAT               5
#define SYS_LSTAT               6
#define SYS_MMAP                9
#define SYS_MPROTECT            10
#define SYS_MUNMAP              11
#define SYS_BRK                 12
#define SYS_IOCTL               16
#define SYS_SOCKET              41
#define SYS_CONNECT             42
#define SYS_ACCEPT              43
#define SYS_SENDTO              44
#define SYS_RECVFROM            45
#define SYS_BIND                49
#define SYS_LISTEN              50
#define SYS_CLONE               56
#define SYS_EXIT                60
#define SYS_MEMFD_CREATE        319
#define SYS_BPF                 321
#define SYS_IO_URING_SETUP      425
#define SYS_IO_URING_ENTER      426
#define SYS_IO_URING_REGISTER   427

/*
 * x86_64 Linux Syscall Calling Convention:
 *   RAX = Syscall Number / Return Value
 *   RDI = Arg 1
 *   RSI = Arg 2
 *   RDX = Arg 3
 *   R10 = Arg 4 (Replaces RCX used by userland ABI)
 *   R8  = Arg 5
 *   R9  = Arg 6
 *
 * Clobbered by hardware 'syscall' instruction: RCX, R11, Memory
 */

#define ALWAYS_INLINE static inline __attribute__((always_inline))

ALWAYS_INLINE long sys_call0(long num) {
    long ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(num)
        : "rcx", "r11", "memory"
    );
    return ret;
}

ALWAYS_INLINE long sys_call1(long num, long a1) {
    long ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(num), "D"(a1)
        : "rcx", "r11", "memory"
    );
    return ret;
}

ALWAYS_INLINE long sys_call2(long num, long a1, long a2) {
    long ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(num), "D"(a1), "S"(a2)
        : "rcx", "r11", "memory"
    );
    return ret;
}

ALWAYS_INLINE long sys_call3(long num, long a1, long a2, long a3) {
    long ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(num), "D"(a1), "S"(a2), "d"(a3)
        : "rcx", "r11", "memory"
    );
    return ret;
}

ALWAYS_INLINE long sys_call4(long num, long a1, long a2, long a3, long a4) {
    long ret;
    register long r10 __asm__("r10") = a4;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(num), "D"(a1), "S"(a2), "d"(a3), "r"(r10)
        : "rcx", "r11", "memory"
    );
    return ret;
}

ALWAYS_INLINE long sys_call5(long num, long a1, long a2, long a3, long a4, long a5) {
    long ret;
    register long r10 __asm__("r10") = a4;
    register long r8  __asm__("r8")  = a5;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(num), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8)
        : "rcx", "r11", "memory"
    );
    return ret;
}

ALWAYS_INLINE long sys_call6(long num, long a1, long a2, long a3, long a4, long a5, long a6) {
    long ret;
    register long r10 __asm__("r10") = a4;
    register long r8  __asm__("r8")  = a5;
    register long r9  __asm__("r9")  = a6;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(num), "D"(a1), "S"(a2), "d"(a3), "r"(r10), "r"(r8), "r"(r9)
        : "rcx", "r11", "memory"
    );
    return ret;
}

#endif /* KRAW_SYSCALL_H */
