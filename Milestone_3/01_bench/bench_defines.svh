`ifndef BENCH_DEFINES_SVH
    `define BENCH_DEFINES_SVH

    `define CLK_DUR 20
    `define RST_DUR 100

    `ifndef MAX_CYCLES
        `define MAX_CYCLES 2_000_000
    `endif

    `define TIMEOUT (`RST_DUR + `CLK_DUR * (`MAX_CYCLES + 1000))

    `ifndef WAVE_START_TIME
        `define WAVE_START_TIME 0
    `endif

    `ifndef WAVE_STOP_TIME
        `define WAVE_STOP_TIME `TIMEOUT
    `endif

    `ifndef REPORT_FILE
        `define REPORT_FILE "test_report.txt"
    `endif

    `ifndef GROUP_COUNT
        `define GROUP_COUNT 12
    `endif

`endif
