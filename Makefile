# kraw zero-overhead build system

CC      := gcc
AS      := gcc
CFLAGS  := -ffreestanding \
           -nostdlib \
           -fno-stack-protector \
           -fno-asynchronous-unwind-tables \
           -fno-builtin \
           -fomit-frame-pointer \
           -O3 \
           -Wall -Wextra -pedantic \
           -Iinclude \
           -std=c23

LDFLAGS := -nostdlib -static -e _start

BUILD_DIR := build
OBJ_DIR   := $(BUILD_DIR)/obj

SRC_ASM  := src/arch/x86_64/entry.S
CORE_SRC := src/sys/alloc.c \
            src/sys/slab.c \
            src/sys/timer.c \
            src/io/io_uring.c \
            src/io/fs.c \
            src/net/net.c

OBJ_ASM  := $(BUILD_DIR)/entry.o
CORE_OBJ := $(CORE_SRC:src/%.c=$(OBJ_DIR)/%.o)

EXAMPLES := 01_bare_write 02_uring_echo 03_async_copy
TARGETS  := $(addprefix $(BUILD_DIR)/, $(EXAMPLES))

.PHONY: all clean size bench

all: $(TARGETS)

# Compile Assembly entry point
$(OBJ_ASM): $(SRC_ASM)
	@mkdir -p $(dir $@)
	$(AS) $(CFLAGS) -c $< -o $@

# Compile Core C Subsystems
$(OBJ_DIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Link Example 01: Bare Write (Minimal dependency)
$(BUILD_DIR)/01_bare_write: examples/01_bare_write.c $(OBJ_ASM)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $< $(OBJ_ASM) -o $@
	@strip -s -R .comment -R .gnu.version -R .gnu.hash $@

# Link Example 02: Async TCP Echo Server
$(BUILD_DIR)/02_uring_echo: examples/02_uring_echo.c $(OBJ_ASM) $(CORE_OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@
	@strip -s -R .comment -R .gnu.version -R .gnu.hash $@

# Link Example 03: Async File Copy
$(BUILD_DIR)/03_async_copy: examples/03_async_copy.c $(OBJ_ASM) $(CORE_OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@
	@strip -s -R .comment -R .gnu.version -R .gnu.hash $@

# Binary Size & ELF Headers Inspection
size: all
	@echo "=== KRAW BINARY SIZE ANALYSIS ==="
	@size $(TARGETS)
	@echo ""
	@ls -lh $(TARGETS)

# Automated Pipeline Benchmark
bench: all
	@echo "=== RUNNING KRAW BENCHMARKS ==="
	@echo "[1/2] Executing 01_bare_write..."
	@./$(BUILD_DIR)/01_bare_write
	@echo "[2/2] Testing 03_async_copy with 10MB pseudo-random file..."
	@dd if=/dev/urandom of=$(BUILD_DIR)/test_src.bin bs=1M count=10 status=none
	@./$(BUILD_DIR)/03_async_copy $(BUILD_DIR)/test_src.bin $(BUILD_DIR)/test_dst.bin
	@cmp -s $(BUILD_DIR)/test_src.bin $(BUILD_DIR)/test_dst.bin && echo "[SUCCESS] Data Integrity 100% (MD5/CMP match)" || echo "[FAIL] Data Mismatch"
	@rm -f $(BUILD_DIR)/test_src.bin $(BUILD_DIR)/test_dst.bin

clean:
	rm -rf $(BUILD_DIR)
