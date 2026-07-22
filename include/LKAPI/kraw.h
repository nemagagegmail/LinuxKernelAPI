#ifndef KRAW_H
#define KRAW_H

/*
 * kraw - Zero-Libc High-Performance Async Framework for x86_64
 * Version 1.0.0 Beta
 */

#define KRAW_VERSION_MAJOR 1
#define KRAW_VERSION_MINOR 0
#define KRAW_VERSION_PATCH 0

/* 1. Core Fundamental Types & Syscall Layer */
#include "types.h"
#include "syscall.h"

/* 2. Virtual Memory & Object Allocators */
#include "alloc.h"
#include "slab.h"

/* 3. Core Asynchronous I/O Engine (io_uring) */
#include "io_uring.h"

/* 4. Subsystems (Networking, Timers, Filesystem) */
#include "net.h"
#include "timer.h"
#include "fs.h"

/*
 * Framework-wide Runtime Initializer
 */
static ALWAYS_INLINE void kraw_init(void) {
    kraw_mem_init();
}

#endif /* KRAW_H */
