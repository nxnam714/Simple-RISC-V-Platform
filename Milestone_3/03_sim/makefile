-include local.mk

TOP              ?= tb_milestone3
WAVE             ?= SHM
WAVE_START_TIME  ?= 0
WAVE_STOP_TIME   ?= TIMEOUT
MAX_CYCLES       ?= $(if $(filter ISA_TEST,$(TEST)),50000000,5000000)
FILELIST         ?= ./flist
IMEM_ADDR_WIDTH  ?= $(if $(filter ISA_TEST,$(TEST)),26,16)
DMEM_ADDR_WIDTH  ?= 14
GROUP_COUNT      := $(if $(filter ISA_TEST,$(TEST)),86,$(if $(filter MCU_TEST,$(TEST)),8,12))
SCORE_ARG        := $(if $(filter ISA_TEST MCU_TEST,$(TEST)),+define+SCORE_POINTS=1.5)


RTL_DIR          ?= ../00_src
BENCH_DIR        ?= ../01_bench
TEST_DIR         ?= ../02_test
override RTL_DIR   := $(abspath $(RTL_DIR))
override BENCH_DIR := $(abspath $(BENCH_DIR))
override TEST_DIR  := $(abspath $(TEST_DIR))
XRUN             ?= xrun
SIMVISION        ?= simvision

TEST              ?= ISA_TEST
IMEM_FORMAT       ?= 1B
ISA_TEST          := $(TEST_DIR)/isa_test/build/isa_test
CHECK_REG         := $(TEST_DIR)/c_program/check_reg/build/check_reg
MCU_TEST          := $(TEST_DIR)/c_program/mcu_test/build/mcu_test
CHECK_TIMER       := $(TEST_DIR)/c_program/check_timer/build/check_timer
CHECK_SW_LED      := $(TEST_DIR)/c_program/check_sw_led/build/check_sw_led
CHECK_HEX         := $(TEST_DIR)/c_program/check_hex/build/check_hex
CHECK_GPIO        := $(TEST_DIR)/c_program/check_gpio/build/check_gpio
PROGRAM_BASE      := $($(TEST))
IMEM_SUFFIX_1B    := 1b
IMEM_SUFFIX_4B    := 4b
IMEM_SUFFIX       := $(IMEM_SUFFIX_$(IMEM_FORMAT))
PROGRAMFILE       := $(PROGRAM_BASE)_$(IMEM_SUFFIX).hex

WAVE_ARG          := $(if $(filter SHM,$(WAVE)), \
                     +define+WAVE_SHM \
                     +define+WAVE_START_TIME=$(WAVE_START_TIME) \
                     $(if $(filter-out TIMEOUT,$(WAVE_STOP_TIME)),+define+WAVE_STOP_TIME=$(WAVE_STOP_TIME)) \
                     +define+SHM_FILE=\"$(CURDIR)/wave.shm\")
IMEM_FORMAT_ARG   := $(if $(filter 4B,$(IMEM_FORMAT)),+define+IMEM_4B)
OUTPUT_ARG        := -quiet -nocopyright
XRUN_ARGS          = -access +rwc -f $(FILELIST) -top $(TOP) \
                     +define+MAX_CYCLES=$(MAX_CYCLES) \
                     +define+IMEM_ADDR_WIDTH=$(IMEM_ADDR_WIDTH) \
                     +define+DMEM_ADDR_WIDTH=$(DMEM_ADDR_WIDTH) \
                     +define+GROUP_COUNT=$(GROUP_COUNT) \
                     +define+PROGRAMFILE=\"$(PROGRAMFILE)\" \
                     $(WAVE_ARG) $(IMEM_FORMAT_ARG) $(SCORE_ARG)

ifneq ($(filter $(IMEM_FORMAT),1B 4B),$(IMEM_FORMAT))
$(error IMEM_FORMAT must be 1B or 4B)
endif

ifneq ($(filter $(WAVE),SHM NONE),$(WAVE))
$(error WAVE must be SHM or NONE)
endif

.DEFAULT_GOAL := sim



.PHONY: filelist
filelist:
	@rm -f flist
	@echo "-sv" >> flist
	@echo "-timescale 1ns/100ps" >> flist
	@find $(RTL_DIR) $(BENCH_DIR) -type f \( -name "*.svh" -o -name "*.vh" \) -printf '%h\n' | sort -u | sed 's|^|+incdir+|' >> flist
	@echo "" >> flist

	@find $(RTL_DIR) $(BENCH_DIR) -type f \
			\( -name "*_pkg.sv" -o -name "*_package.sv" -o -name "package.sv" \) \
		| sort >> flist
	@echo "" >> flist

	@find $(RTL_DIR) -type f \( -name "*.sv" -o -name "*.v" \) \
		! -name "*_pkg.sv" ! -name "*_package.sv" ! -name "package.sv" \
		| sort >> flist
	@echo "" >> flist

	@find $(BENCH_DIR) -type f \( -name "*.sv" -o -name "*.v" \) \
			! -name "*_pkg.sv" ! -name "*_package.sv" ! -name "package.sv" \
		| sort >> flist
	@echo "Created ./flist"




.PHONY: check_test
check_test:
	@if [ -z "$(PROGRAM_BASE)" ]; then \
		echo "[ERROR] Unknown TEST value: $(TEST)"; \
		echo "        Run 'make list_test' to see valid values."; \
		exit 1; \
	fi
	@if [ ! -f "$(PROGRAMFILE)" ]; then \
		echo "[ERROR] Program file is missing: $(PROGRAMFILE)"; \
		exit 1; \
	fi
	@if [ "$(WAVE)" = "SHM" ]; then \
		case "$(WAVE_START_TIME)" in ''|*[!0-9]*) \
			echo "[ERROR] WAVE_START_TIME must be a non-negative integer in ns"; exit 1;; \
		esac; \
		if [ "$(WAVE_STOP_TIME)" != "TIMEOUT" ]; then \
			case "$(WAVE_STOP_TIME)" in ''|*[!0-9]*) \
				echo "[ERROR] WAVE_STOP_TIME must be TIMEOUT or a non-negative integer in ns"; exit 1;; \
			esac; \
			if [ "$(WAVE_STOP_TIME)" -lt "$(WAVE_START_TIME)" ]; then \
				echo "[ERROR] WAVE_STOP_TIME must not be earlier than WAVE_START_TIME"; exit 1; \
			fi; \
		fi; \
	fi
.PHONY: list_test


list_test:
	@printf '  %-18s %s\n' 'TEST value' 'Description'
	@printf '  %-18s %s\n' '------------------' '----------------------------------------------'
	@printf '  %-18s %s\n' 'ISA_TEST'         'Tests the RV32I instructions'
	@printf '  %-18s %s\n' 'MCU_TEST'         'C program: tests the processor, bus and all devices working together'
	@printf '  %-18s %s\n' 'CHECK_REG'        'C program: tests address decoding and each device register'
	@printf '  %-18s %s\n' 'CHECK_SW_LED'     'C program: tests switches and LEDs'
	@printf '  %-18s %s\n' 'CHECK_TIMER'      'C program: tests timer accuracy'
	@printf '  %-18s %s\n' 'CHECK_HEX'        'C program: tests the timer and seven-segment displays'
	@printf '  %-18s %s\n' 'CHECK_GPIO'       'C program: tests GPIO inputs and outputs'



.PHONY: sim
sim: check_test
	@$(XRUN) $(OUTPUT_ARG) $(XRUN_ARGS)


.PHONY: wave
wave:
	@$(SIMVISION) wave.shm &



.PHONY: clean
clean:
	@echo "-> CLEAN"
	@rm -rf xcelium.d INCA_libs *.shm *.log
	@rm -f xrun.history xrun.key history test_report.txt



.PHONY: help
help:
	@printf '%s\n' \
		'Milestone 3 simulation' \
		'' \
		'Run these commands from the 03_sim directory.' \
		'Cadence Xcelium (xrun) must be loaded.' \
		'' \
		'Quick start:' \
		'  make filelist' \
		'  make sim TEST=ISA_TEST WAVE=NONE' \
		'' \
		'Targets:' \
		'  make sim              Compile and run the selected test (default target).' \
		'  make wave             Open wave.shm in SimVision after an SHM run.' \
		'  make filelist         Rebuild flist after adding or removing an RTL file.' \
		'  make list_test        List available TEST values and their coverage.' \
		'  make clean            Remove generated simulator files and waveforms; preserve flist.' \
		'  make help             Show this help.' \
		'' \
		'Options (override with NAME=value):' \
		'  TEST=ISA_TEST         Select a program shown by make list_test.' \
		'  IMEM_FORMAT=1B        Select byte-packed (1B) or word-packed (4B) program files.' \
		'  WAVE=SHM|NONE         Enable SHM recording or disable waveform generation.' \
		'  WAVE_START_TIME=0     Begin waveform recording at this simulation time in ns.' \
		'  WAVE_STOP_TIME=TIMEOUT Stop recording at this time in ns; defaults to timeout.' \
		'  MAX_CYCLES=5000000    Stop after this many cycles; self-check uses 50000000 by default.' \
		'  TOP=tb_milestone3     Select the simulation top module.' \
		'  FILELIST=./flist      Select the simulator file list.' \
		'  XRUN=xrun             Xcelium executable name or full path.' \
		'' \
		'Available tests:'
	@$(MAKE) --no-print-directory list_test
	@printf '%s\n' \
		'' \
		'Useful examples:' \
		'  make sim TEST=ISA_TEST WAVE=NONE' \
		'  make sim TEST=ISA_TEST WAVE=NONE IMEM_FORMAT=4B' \
		'  make sim TEST=ISA_TEST WAVE_START_TIME=1000000 WAVE_STOP_TIME=1100000'
