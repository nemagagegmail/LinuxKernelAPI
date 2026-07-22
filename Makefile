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

SRC_ASM := src/arch/x86_64/entry.S
SRC_C   := examples/01_bare_write.c
OBJ     := $(SRC_ASM:.S=.o) $(SRC_C:.c=.o)
TARGET  := build/bare_write

.PHONY: all clean run size

all: $(TARGET)

%.o: %.S
	$(AS) $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJ)
	@mkdir -p build
	$(CC) $(LDFLAGS) $(OBJ) -o $@
	@strip -s -R .comment -R .gnu.version $@

run: $(TARGET)
	@./$(TARGET)

size: $(TARGET)
	@size $(TARGET)
	@ls -lh $(TARGET)

clean:
	rm -rf build $(OBJ)
