# Milestone 2 simulation flow (Verilator >= 5).
# Run from this directory.

TOP              ?= tb_milestone2
VERILATOR        ?= verilator
WAVE             ?= FST
WAVE_START_TIME  ?= 0
WAVE_STOP_TIME   ?= TIMEOUT
MAX_CYCLES       ?= $(if $(filter ISA_TEST,$(TEST)),50000000,5000000)
OBJ_DIR          ?= obj_dir
WAVE_FILE        ?= $(CURDIR)/wave.$(shell echo $(WAVE) | tr 'A-Z' 'a-z')

RTL_DIR          ?= ../00_src
BENCH_DIR        ?= ../01_bench
FILELIST         ?= ./flist

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

ifeq ($(WAVE_STOP_TIME),TIMEOUT)
  WAVE_STOP_DEFINE :=
else
  WAVE_STOP_DEFINE := -DWAVE_STOP_TIME=$(WAVE_STOP_TIME)
endif

DEFS := -DMAX_CYCLES=$(MAX_CYCLES) \
        -DIMEM_ADDR_WIDTH=$(if $(filter ISA_TEST,$(TEST)),26,19) \
        -DDMEM_START_ADDR=$(if $(filter ISA_TEST,$(TEST)),32\'h8400_0000,32\'h8008_0000) \
        -DPROGRAMFILE='"$(PROGRAMFILE)"' \
        -DWAVE_START_TIME=$(WAVE_START_TIME) \
        $(WAVE_STOP_DEFINE)

ifeq ($(TEST),ISA_TEST)
  DEFS += -DISA_TEST
endif
ifeq ($(IMEM_FORMAT),4B)
  DEFS += -DIMEM_4B
endif
ifeq ($(WAVE),VCD)
  DEFS += -DWAVE_VCD -DVCD_FILE='"$(WAVE_FILE)"'
  TRACE_ARG := --trace
else ifeq ($(WAVE),FST)
  DEFS += -DWAVE_VCD -DVCD_FILE='"$(WAVE_FILE)"'
  TRACE_ARG := --trace-fst
endif

ifneq ($(filter $(WAVE),VCD FST NONE),$(WAVE))
$(error WAVE must be VCD, FST, or NONE)
endif

ifneq ($(filter $(IMEM_FORMAT),1B 4B),$(IMEM_FORMAT))
$(error IMEM_FORMAT must be 1B or 4B)
endif

.DEFAULT_GOAL := sim
.PHONY: help filelist check_test list_test sim wave clean

help:
	@printf '%s\n' \
		'Milestone 2 Verilator simulation' \
		'' \
		'Run these commands from the 04_verilator directory.' \
		'Verilator >= 5 must be installed.' \
		'' \
		'Targets:' \
		'  make sim              Compile and run the selected test.' \
		'  make wave             Open the waveform with GTKWave.' \
		'  make filelist         Rebuild the Verilator source list.' \
		'  make list_test        List available TEST values.' \
		'  make clean            Remove generated simulator files and waveforms.' \
		'' \
		'Options:' \
		'  TEST=ISA_TEST         Select a program shown by make list_test.' \
		'  IMEM_FORMAT=1B        Select byte-packed (1B) or word-packed (4B) images.' \
		'  WAVE=FST|VCD|NONE     Enable FST/VCD recording or disable waveform generation.' \
		'  WAVE_START_TIME=0     Begin recording at this simulation time in ns.' \
		'  WAVE_STOP_TIME=TIMEOUT Stop recording at this time in ns.' \
		'  MAX_CYCLES=5000000    Stop after this many cycles.' \
		'' \
		'Examples:' \
		'  make sim TEST=ISA_TEST IMEM_FORMAT=1B WAVE=FST' \
		'  make sim TEST=PROG_MATRIX WAVE=NONE'

filelist:
	@find $(RTL_DIR) $(BENCH_DIR) -type f \
		\( -name '*.sv' -o -name '*.v' \) \
		! -name 'tb_singlecycle_custom.sv' | sort > $(FILELIST)
	@echo "Created $(FILELIST)"

check_test:
	@test -n "$(PROGRAM_BASE)" || { echo "[ERROR] Unknown TEST value: $(TEST)"; exit 1; }
	@test -f "$(PROGRAMFILE)" || { echo "[ERROR] Program file is missing: $(PROGRAMFILE)"; exit 1; }
	@if [ "$(WAVE)" != "NONE" ]; then \
		case "$(WAVE_START_TIME)" in ''|*[!0-9]*) \
			echo "[ERROR] WAVE_START_TIME must be a non-negative integer in ns"; exit 1;; esac; \
		if [ "$(WAVE_STOP_TIME)" != "TIMEOUT" ]; then \
			case "$(WAVE_STOP_TIME)" in ''|*[!0-9]*) \
				echo "[ERROR] WAVE_STOP_TIME must be TIMEOUT or a non-negative integer in ns"; exit 1;; esac; \
			if [ "$(WAVE_STOP_TIME)" -lt "$(WAVE_START_TIME)" ]; then \
				echo "[ERROR] WAVE_STOP_TIME must not be earlier than WAVE_START_TIME"; exit 1; \
			fi; \
		fi; \
	fi

list_test:
	@printf '%s\n' \
		'PROG_HANOI PROG_SORT PROG_MATRIX PROG_SOFTFLOAT PROG_SIGMOID PROG_CONVOLUTION ISA_TEST'

sim: check_test
	@test -f "$(FILELIST)" || $(MAKE) --no-print-directory filelist
	$(VERILATOR) --binary --sv --timing --assert -Wno-fatal \
		--top-module $(TOP) -f $(FILELIST) -I$(BENCH_DIR) \
		$(DEFS) $(TRACE_ARG) --Mdir $(OBJ_DIR)
	./$(OBJ_DIR)/V$(TOP) | tee sim.log

wave:
	@gtkwave "$(WAVE_FILE)" &

clean:
	@rm -rf $(OBJ_DIR) sim.log wave.vcd wave.fst
