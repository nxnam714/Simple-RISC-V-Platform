# Local Verilator flow for Milestone 3 (Xcelium is not available on this machine).
# Mirrors the settings of ../03_sim/makefile; grading still uses 03_sim + Xcelium.
# Usage: make sim TEST=CHECK_REG   |  make lint  |  make all_tests

TOP              ?= tb_milestone3
VERILATOR        ?= verilator
OBJ_DIR          ?= obj_dir_$(TEST)
TEST             ?= ISA_TEST
MAX_CYCLES       ?= $(if $(filter ISA_TEST,$(TEST)),50000000,5000000)
IMEM_ADDR_WIDTH  ?= $(if $(filter ISA_TEST,$(TEST)),26,16)
DMEM_ADDR_WIDTH  ?= 14
GROUP_COUNT      := $(if $(filter ISA_TEST,$(TEST)),86,$(if $(filter MCU_TEST,$(TEST)),8,12))
SCORE_ARG        := $(if $(filter ISA_TEST MCU_TEST,$(TEST)),-DSCORE_POINTS=1.5)

RTL_DIR          := $(abspath ../00_src)
BENCH_DIR        := $(abspath ../01_bench)
TEST_DIR         := $(abspath ../02_test)

ISA_TEST         := $(TEST_DIR)/isa_test/build/isa_test
CHECK_REG        := $(TEST_DIR)/c_program/check_reg/build/check_reg
MCU_TEST         := $(TEST_DIR)/c_program/mcu_test/build/mcu_test
CHECK_TIMER      := $(TEST_DIR)/c_program/check_timer/build/check_timer
CHECK_SW_LED     := $(TEST_DIR)/c_program/check_sw_led/build/check_sw_led
CHECK_HEX        := $(TEST_DIR)/c_program/check_hex/build/check_hex
CHECK_GPIO       := $(TEST_DIR)/c_program/check_gpio/build/check_gpio
PROGRAMFILE      := $($(TEST))_1b.hex

RTL_FILES        := $(sort $(wildcard $(RTL_DIR)/*.sv))
BENCH_FILES      := $(BENCH_DIR)/tb_milestone3.sv

DEFS := -DMAX_CYCLES=$(MAX_CYCLES) -DIMEM_ADDR_WIDTH=$(IMEM_ADDR_WIDTH) \
        -DDMEM_ADDR_WIDTH=$(DMEM_ADDR_WIDTH) -DGROUP_COUNT=$(GROUP_COUNT) \
        -DPROGRAMFILE='"$(PROGRAMFILE)"' -DREPORT_FILE='"report_$(TEST).txt"' $(SCORE_ARG)

TESTS := ISA_TEST CHECK_REG CHECK_SW_LED CHECK_TIMER CHECK_HEX CHECK_GPIO MCU_TEST

.PHONY: sim lint all_tests clean
sim:
	$(VERILATOR) --binary --sv --timing -Wno-fatal -Wno-lint -Wno-style -O2 \
		--top-module $(TOP) -I$(BENCH_DIR) $(DEFS) --Mdir $(OBJ_DIR) \
		$(RTL_FILES) $(BENCH_FILES) > build_$(TEST).log 2>&1 || { tail -40 build_$(TEST).log; exit 1; }
	./$(OBJ_DIR)/V$(TOP) > sim_$(TEST).log 2>&1; tail -14 sim_$(TEST).log

lint:
	$(VERILATOR) --lint-only -Wall --top-module milestone3 -DPROGRAMFILE='""' $(RTL_FILES)

all_tests:
	@for t in $(TESTS); do echo "=== $$t ==="; $(MAKE) --no-print-directory sim TEST=$$t; done

clean:
	rm -rf obj_dir_* *.log report_*.txt
