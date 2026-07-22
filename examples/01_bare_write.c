#include "../include/LKAPI/types.h"
#include "../include/LKAPI/syscall.h"

long kmain(int argc, char **argv) {
    (void)argc;
    (void)argv;

    const char msg[] = "[KRAW] System online. Bare-metal operational.\n";
    
    // SYS_WRITE to stdout (fd = 1)
    sys_call3(SYS_WRITE, 1, (long)msg, sizeof(msg) - 1);

    return 0;
}
