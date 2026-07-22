/* Ultra-optimized bare-metal entry point for Linux x86_64 */
.global _start
.text

_start:
    xorl    %ebp, %ebp       /* Clear RBP (32-bit XOR zeros full 64-bit reg, saves 1 byte) */
    popq    %rdi             /* RDI = argc (popped directly from initial stack frame) */
    movq    %rsp, %rsi       /* RSI = argv (points to array of argument strings) */
    andq    $-16, %rsp       /* Enforce 16-byte stack alignment required by System V ABI */
    call    kmain            /* Transfer control to C entry point: kmain(argc, argv) */

    movq    %rax, %rdi       /* RDI = return status code from kmain */
    pushq   $60              /* SYS_exit number (60) */
    popq    %rax             /* RAX = 60 (3-byte push/pop sequence vs 7-byte mov) */
    syscall                  /* Invoke kernel termination */
