RISCV_CC      ?= riscv64-unknown-elf-gcc
RISCV_OBJCOPY ?= riscv64-unknown-elf-objcopy
BUILD_DIR     := build
ELF           := $(BUILD_DIR)/factorial_fibonacci.elf
BIN           := $(BUILD_DIR)/factorial_fibonacci.bin
HEX           := $(BUILD_DIR)/factorial_fibonacci_4b.hex
CFLAGS        := -march=rv32i -mabi=ilp32 -O2 -ffreestanding -fno-builtin \
                 -fno-stack-protector -fno-pic -fno-jump-tables \
                 -fno-tree-switch-conversion -msmall-data-limit=0

.PHONY: all clean
all: $(HEX)

$(BUILD_DIR):
	mkdir -p $@

$(ELF): factorial_fibonacci.c startup.S ../mcu.h Makefile | $(BUILD_DIR)
	$(RISCV_CC) $(CFLAGS) -nostdlib -nostartfiles \
		-Wl,-Ttext=0x80000000 -Wl,-e,_start -Wl,--no-relax \
		-o $@ startup.S factorial_fibonacci.c

$(BIN): $(ELF)
	$(RISCV_OBJCOPY) -O binary --only-section=.text $< $@

$(HEX): $(BIN)
	od -An -v -tx4 -w4 $< | tr -d ' ' > $@

clean:
	rm -f $(ELF) $(BIN) $(HEX)
