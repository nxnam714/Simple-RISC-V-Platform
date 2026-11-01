`ifndef BENCH_DEFINES_SVH
    `define BENCH_DEFINES_SVH

    `ifndef IMEM_START_ADDR
        `define IMEM_START_ADDR 32'h8000_0000
    `endif

    `ifndef IMEM_ADDR_WIDTH
        `define IMEM_ADDR_WIDTH 19
    `endif

    `ifndef DMEM_START_ADDR
        `define DMEM_START_ADDR 32'h8008_0000
    `endif

    `ifndef DMEM_ADDR_WIDTH
        `define DMEM_ADDR_WIDTH 16
    `endif

    `define CLK_DUR 2
    `define RST_DUR 50


    `ifndef MAX_CYCLES
        `define MAX_CYCLES 200_000
    `endif

    `define TIMEOUT (`RST_DUR + `CLK_DUR * (`MAX_CYCLES + 1000))

    `ifndef WAVE_START_TIME
        `define WAVE_START_TIME 0
    `endif

    `ifndef WAVE_STOP_TIME
        `define WAVE_STOP_TIME `TIMEOUT
    `endif
`endif

