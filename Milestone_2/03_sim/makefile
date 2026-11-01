-include local.mk

TOP              ?= tb_milestone2
WAVE             ?= SHM
WAVE_START_TIME  ?= 0
WAVE_STOP_TIME   ?= TIMEOUT
MAX_CYCLES       ?= $(if $(filter ISA_TEST,$(TEST)),50000000,5000000)
FILELIST         ?= ./flist
IMEM_ADDR_WIDTH  ?= $(if $(filter ISA_TEST,$(TEST)),26,19)
DMEM_START_ADDR  ?= $(if $(filter ISA_TEST,$(TEST)),32\'h8400_0000,32\'h8008_0000)


RTL_DIR          ?= ../00_src
BENCH_DIR        ?= ../01_bench
XRUN             ?= xrun
SIMVISION        ?= simvision

TEST              ?= ISA_TEST
IMEM_FORMAT       ?= 1B
ISA_TEST          := ../02_test/isa_test/build/isa_test
PROG_HANOI        := ../02_test/c_program/00_hanoi/build/hanoi
PROG_SORT         := ../02_test/c_program/01_bubble_sort/build/bubble_sort
PROG_MATRIX       := ../02_test/c_program/02_matrix_mul/build/matrix_mul
PROG_SOFTFLOAT    := ../02_test/c_program/03_softfloat/build/softfloat
PROG_SIGMOID      := ../02_test/c_program/04_sigmoid/build/sigmoid
PROG_CONVOLUTION  := ../02_test/c_program/05_convolution/build/convolution
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
ISA_ARG           := $(if $(filter ISA_TEST,$(TEST)),+define+ISA_TEST)
IMEM_FORMAT_ARG   := $(if $(filter 4B,$(IMEM_FORMAT)),+define+IMEM_4B)
OUTPUT_ARG        := -quiet -nocopyright
XRUN_ARGS          = -access +rwc -f $(FILELIST) -top $(TOP) \
                     +define+MAX_CYCLES=$(MAX_CYCLES) \
                     +define+IMEM_ADDR_WIDTH=$(IMEM_ADDR_WIDTH) \
                     +define+DMEM_START_ADDR=$(DMEM_START_ADDR) \
                     +define+PROGRAMFILE=\"$(PROGRAMFILE)\" \
                     $(WAVE_ARG) $(ISA_ARG) $(IMEM_FORMAT_ARG)

ifneq ($(filter $(WAVE),SHM NONE),$(WAVE))
$(error WAVE must be SHM or NONE)
endif

ifneq ($(filter $(IMEM_FORMAT),1B 4B),$(IMEM_FORMAT))
$(error IMEM_FORMAT must be 1B or 4B)
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
	@printf '  %-18s %s\n' 'PROG_HANOI'       'Run C program: Solve and validate 12-disk Tower of Hanoi'
	@printf '  %-18s %s\n' 'PROG_SORT'        'Run C program: Compare six signed-integer sorting algorithms'
	@printf '  %-18s %s\n' 'PROG_MATRIX'      'Run C program: Multiply and cross-check signed 8x8 matrices'
	@printf '  %-18s %s\n' 'PROG_SIGMOID'     'Run C program: Validate sigmoid behavior and three-class inference'
	@printf '  %-18s %s\n' 'PROG_SOFTFLOAT'   'Run C program: Check arithmetic across four floating-point formats'
	@printf '  %-18s %s\n' 'PROG_CONVOLUTION' 'Run C program: Apply Gaussian and Sobel kernels to a signed 7x7 matrix'
	@printf '  %-18s %s\n' 'ISA_TEST'         'Run the complete graded RV32I ISA test'



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
	@rm -f xrun.history xrun.key history



.PHONY: help
help:
	@printf '%s\n' \
		'Milestone 2 simulation' \
		'' \
		'Run these commands from the 03_sim directory.' \
		'Cadence Xcelium (xrun) must be loaded.' \
		'' \
		'Quick start:' \
		'  make filelist' \
		'  make sim TEST=ISA_TEST' \
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
		'  TOP=tb_milestone2     Select the simulation top module.' \
		'  FILELIST=./flist      Select the simulator file list.' \
		'  XRUN=xrun             Xcelium executable name or full path.' \
		'' \
		'Available tests:'
	@$(MAKE) --no-print-directory list_test
	@printf '%s\n' \
		'' \
		'Useful examples:' \
		'  make sim TEST=ISA_TEST' \
		'  make sim TEST=ISA_TEST IMEM_FORMAT=4B' \
		'  make sim TEST=ISA_TEST WAVE=NONE' \
		'  make sim TEST=ISA_TEST WAVE_START_TIME=1000000 WAVE_STOP_TIME=1100000' \
		'  make sim TEST=PROG_MATRIX WAVE=NONE'
