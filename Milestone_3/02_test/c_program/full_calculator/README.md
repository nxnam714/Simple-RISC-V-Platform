# Full calculator

This directory contains the C implementation for the HPS/mailbox calculator
integration:

- `full_calculator.c`: freestanding C compiled for RV32I.
- `startup.S`: minimal reset bootstrap that initializes `sp` before calling C.

All calculator behavior is implemented in C and uses only RV32I. Multiplication
is implemented with shift-and-add, and signed division uses repeated unsigned
subtraction after sign normalization. The C build is freestanding and does not
link a runtime, so it cannot silently depend on hosted C support or a hardware
`M` extension. Since the MCU has separate instruction and data memories,
compiler-generated constant lookup tables are disabled; the C image keeps all
executable content in `.text`.

The bootstrap remains assembly because the core resets its PC but does not
initialize a stack pointer. C code needs a valid stack before its first
function prologue; `startup.S` only sets `sp` and transfers control to C.

The existing standalone calculator in `../full_calculator/` remains unchanged
and continues to use SW/KEY input with HEX/LED output.

## Contract

- Two sign-extended signed 8-bit operands in `REQ_A` and `REQ_B`.
- `REQ_OP`: 0 add, 1 subtract, 2 multiply, 3 signed divide truncated toward
  zero.
- `REQ_SEQ` is published by HPS after the request fields are stable.
- `RES_VALUE` and `RES_STATUS` are written by the MCU before `RES_SEQ`.
- Status values are 0 success, 1 divide by zero, and 2 invalid request.
- Division by zero and invalid input publish a zero value with the corresponding
  status. Every valid result fits in signed 32-bit.
- On success, the MCU displays only the signed decimal result on HEX0-HEX4;
  HEX5 shows a minus sign for negative results. Leading zeroes are blanked.
  Division by zero and invalid requests show `E` on HEX0.
- HPS Linux waits for `RES_SEQ` but does not read the result or write any display.
  The result remains on the FPGA's seven-segment display and is never returned
  to the PC.
- The mailbox is at MCU address `0x40007000`, not in SDRAM.
- MCU reset preserves mailbox registers. On restart, the application compares
  request and completion sequences: a completed request is not repeated, while
  a request that was pending during reset is processed.

## Build and simulation

```sh
make
make -C ../../../05_verilator full_calculator_test
```

The simulation runs the C-generated image on the Milestone 3 core and exercises
signed boundary arithmetic, invalid operands/op codes, division by zero,
seven-segment output, and a request pending across an MCU-only reset. Run the
full regression with:

```sh
make -C ../../../05_verilator all_tests
```

The generated image is `build/full_calculator_4b.hex`.

The image is ignored build output. To select it in the active Quartus project,
build it first and define `CALCULATOR_APP`; the default project image remains
the standalone SW/KEY-to-HEX/LED application.

The current Quartus top-level still ties the HPS mailbox port inactive. A
Platform Designer HPS system and the board-specific Linux bridge setup are
required before this application can receive requests on the physical board.
