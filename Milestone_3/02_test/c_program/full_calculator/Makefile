RISCV_CC      ?= riscv64-unknown-elf-gcc
RISCV_OBJCOPY ?= riscv64-unknown-elf-objcopy
BUILD_DIR     := build
C_ELF         := $(BUILD_DIR)/full_calculator.elf
C_BIN         := $(BUILD_DIR)/full_calculator.bin
C_HEX         := $(BUILD_DIR)/full_calculator_4b.hex
CFLAGS        := -march=rv32i -mabi=ilp32 -O2 -ffreestanding -fno-builtin \
                 -fno-stack-protector -fno-pic -fno-jump-tables \
                 -fno-tree-switch-conversion -msmall-data-limit=0

.PHONY: all clean
all: $(C_HEX)

$(BUILD_DIR):
	mkdir -p $@

$(C_ELF): full_calculator.c startup.S Makefile | $(BUILD_DIR)
	$(RISCV_CC) $(CFLAGS) -nostdlib -nostartfiles \
		-Wl,-Ttext=0x80000000 -Wl,-e,_start -Wl,--no-relax \
		-o $@ startup.S full_calculator.c

$(C_BIN): $(C_ELF)
	$(RISCV_OBJCOPY) -O binary --only-section=.text $< $@

$(C_HEX): $(C_BIN)
	od -An -v -tx4 -w4 $< | tr -d ' ' > $@

clean:
	rm -f $(C_ELF) $(C_BIN) $(C_HEX)
