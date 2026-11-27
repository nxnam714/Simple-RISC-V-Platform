---
name: de10-riscv-calculator
description: Guide for building the PC -> UART -> HPS Linux -> HPS-to-FPGA bridge -> mailbox -> RISC-V MCU full-calculator demo on DE10-Standard, where the MCU displays only its result on FPGA seven-segment HEX digits and never returns it to the PC.
---

# DE10-Standard RISC-V full calculator (HPS + mailbox) skill

This is a system-integration extension of Milestone 3. It must not break the
standalone calculator (SW/KEY input, HEX/LED output) or the existing Milestone 3
test programs.

## Goal

```text
PC/web backend --UART--> HPS Linux --LW HPS-to-FPGA bridge--> FPGA mailbox <--MMIO-- RISC-V MCU
                             |                                     ^                    |
                             v
                  UART completion/error only       HEX0-HEX5
                                                   (MCU result)
```

- The PC sends calculator requests and computes its own reference result for the browser.
- The RISC-V MCU performs the calculation.
- The MCU displays only its signed decimal result on the FPGA's HEX0-HEX5 seven-segment
  displays. HEX5 is a minus sign when the result is negative; leading zeroes are blank.
- HPS Linux waits for `RES_SEQ` completion but does not read the result/status registers or
  write to any display. It may send transport/input errors to the PC, but never the result.
- The user compares the browser reference and the seven-segment result visually.

## Hard requirements (from the course and the current design)

- **All application behavior is in software** (Section 7.7, `Milestone_3/90_doc/7.7_Applications.md`).
  The submitted MCU application is RISC-V assembly. The mailbox is only storage plus a
  handshake; it must contain no arithmetic or calculator logic.
- **The core is RV32I (no M extension).** `mul`/`div`/`rem` are illegal on this core.
  Multiplication and signed division must be implemented in software
  (shift-and-add, restoring division with sign fix-up). The C reference uses `*` and `/`,
  so it only works when the compiler links software helpers; the assembly port must
  provide its own routines.
- **Keep the existing memory system.** DMEM stays on the external SDRAM controller
  (`sdram_controller.sv`, CPU stalls via `i_stall`). Do not put the mailbox in SDRAM.
  Mailbox accesses must complete in one cycle and must not assert the core stall.
- **Do not break existing tests.** `check_reg` requires these addresses to read 0 and
  ignore writes: `0x4000_6000`, `0x4000_E000`, `0x4001_0000`, `0x9000_0000`,
  `DMEM_BASE + DMEM_BYTES`. Therefore **do not map the mailbox at `0x4000_6000`**.
  Use a free page such as `0x4000_7000` and confirm it against `mcu_bus.sv` and all
  test programs before committing.

## Current repository state (verify before editing)

- Active Quartus project: `Milestone_3/04_fpga/altera/simple_riscv/` (top `simple_riscv.v`,
  System Builder style with an `altpll`). `Milestone_3/04_fpga/quartus/` is an older project.
- The top level has **no HPS instance and no `HPS_*` ports**. Adding the bridge requires a
  Platform Designer (Qsys) system containing the Cyclone V HPS, its mandatory DDR3/HPS
  I/O exports, and the Lightweight HPS-to-FPGA master exported to fabric, then
  instantiating that system in the top level.
- MCU address map (`Milestone_3/00_src/mcu_bus.sv`): DMEM `0x8400_0000`, LED `0x4000_0000`,
  HEX `0x4000_1000`, SW `0x4000_2000`, KEY `0x4000_3000`, Timer `0x4000_4000`,
  GPIO `0x4000_5000`, testbench device `0x4000_F000`.
- Calculator reference: `Milestone_3/02_test/full_calculator/full_calculator.c` and `README.md`.

## Calculator contract

Keep the existing semantics unless the task explicitly changes them:

| Op code | Operation | Result range for signed 8-bit operands |
|---|---|---|
| 0 | a + b | -256 .. 254 |
| 1 | a - b | -255 .. 255 |
| 2 | a * b | -16256 .. 16384 |
| 3 | a / b, truncate toward zero | -127 .. 128 (`-128 / -1 = 128`) |

- Operands: signed 8-bit, -128 .. 127. Every result fits in a signed 32-bit word, so there
  is no overflow case; publish the result sign-extended to 32 bits.
- Division by zero: publish a status code (e.g. `STATUS_DIV0`) and display `E` on HEX0.
  Keep the standalone build's `E`/LED behavior unchanged.
- Out-of-range operands or unknown op codes: rejected by the HPS service; the MCU also
  reports `STATUS_BAD_REQ` if they reach it.

## Implementation workflow

### 1. Confirm board and software environment

- Identify the Linux image on the board, how `/dev/mem` or a UIO/driver exposes the
  Lightweight bridge, and the bridge base address from the Platform Designer/device tree
  (on Cyclone V it is normally `0xFF20_0000`; confirm, do not assume offsets).
- **UART conflict:** the DE10-Standard USB-UART is HPS UART0, which is usually the Linux
  serial console (`/dev/ttyS0`). Either disable the console/getty on it or use another
  UART; otherwise kernel/login output corrupts the calculator stream.
- No HPS LCD driver is needed for this demo. The RISC-V MCU uses the existing FPGA HEX
  peripheral at `0x4000_1000`; verify the active top-level connects HEX0-HEX5 to board pins.

### 2. Define the mailbox register map and protocol

Specify widths, byte order (little-endian), reset values and ownership before coding.
Suggested map (same offsets seen by HPS and MCU, 32-bit registers):

| Offset | Name | Writer | Meaning |
|---|---|---|---|
| 0x00 | `REQ_A` | HPS | operand A, sign-extended |
| 0x04 | `REQ_B` | HPS | operand B, sign-extended |
| 0x08 | `REQ_OP` | HPS | op code 0..3 |
| 0x0C | `REQ_SEQ` | HPS | written **last**; a new value means a new request |
| 0x10 | `RES_VALUE` | MCU | result |
| 0x14 | `RES_STATUS` | MCU | 0 OK, 1 DIV0, 2 BAD_REQ |
| 0x18 | `RES_SEQ` | MCU | written **last**; equals the `REQ_SEQ` it answers |

Rules:
- HPS writes a new request only when `RES_SEQ == REQ_SEQ` (previous one finished).
- MCU processes a request when `REQ_SEQ != last_seen_seq`, then writes value, status,
  and finally `RES_SEQ`.
- HPS accepts a result only when `RES_SEQ` equals the sequence it sent (no stale display).
- Reset clears all registers; define what happens when only the MCU (KEY0) is reset.
- Write enable from either side must be decoded per register so each side can only write
  its own fields.

### 3. Implement and verify the FPGA mailbox

- One module with two ports: an Avalon-MM slave (or AXI-lite via the exported LW bridge)
  for HPS, and an MCU MMIO port with the same interface style as `led_dev.sv`
  (`i_addr[11:0]`, `i_wdata`, `i_bmsk`, `i_wren`, `o_rdata`).
- Prefer clocking the LW bridge interface (`h2f_lw_axi_clk`) and the mailbox from the same
  50 MHz clock as the MCU to avoid clock-domain crossing. If the clocks differ, synchronize
  the sequence registers and only sample data fields after the sequence change is seen.
- Add the new select line, write enable and read-data mux entry in `mcu_bus.sv`, and the
  new ports in `milestone3.sv`. For simulation builds, tie the HPS port inactive and keep
  `tb_milestone3.sv` compiling.
- Simulate: register reset values, HPS write/read, MCU write/read, byte-mask writes,
  sequence handshake, and a new request arriving while the MCU is still busy.
- Re-run all existing tests (`05_verilator`: `make all_tests`) to prove nothing regressed.

### 4. Implement the HPS Linux service

- Read newline-delimited requests from the configured UART (e.g. `A B OP\n`).
- Validate framing, op code and operand ranges; report malformed input to the PC as an
  error, but never send the MCU result.
- Write `REQ_A`, `REQ_B`, `REQ_OP`, then `REQ_SEQ` last (use volatile accesses / memory
  barriers so the order is preserved on the bridge).
- Poll `RES_SEQ` with a timeout. Do not read `RES_VALUE`/`RES_STATUS` to send results to
  the PC or to drive a display; the MCU handles the seven-segment output.

### 5. Adapt the RISC-V calculator (assembly)

- Write a C variant first if useful for behavior checks, then the submitted assembly version.
- Replace SW/KEY input with polling of `REQ_SEQ`; read the request fields after seeing a
  new sequence.
- Implement `mul` and signed `div` in software (RV32I only). Test boundaries:
  `-128*-128`, `127*-128`, `-128/-1`, `-127/2`, `7/-2`, `x/0`.
- Write `RES_VALUE`, `RES_STATUS`, then `RES_SEQ` last.
- Display only the signed decimal result on HEX0-HEX4 and a minus sign on HEX5; blank
  leading digits. Display `E` on HEX0 for divide-by-zero and invalid requests. Do not
  display operands or write to the HPS LCD.
- Rebuild the IMEM image (`*_4b.hex`) and point the Quartus `imem.sv` `$readmemh` path at it.

### 6. Integrate and test end to end

1. Software arithmetic routines, including all boundary and division-by-zero cases.
2. Mailbox RTL in simulation, then existing Milestone 3 regression tests.
3. HPS bridge reads/writes to the mailbox on the board (e.g. with a small `/dev/mem` tool).
4. MCU request detection, mailbox completion, and seven-segment result display.
5. HPS UART parsing and request submission.
6. Confirm the HPS service only observes completion sequence and never reads result data.
7. Full flow: PC request -> browser shows its reference -> MCU computes -> HEX displays
   only the MCU result.

For each step verify: each request is processed exactly once, stale results are never
displayed, and a new request cannot corrupt an in-flight one.

## Completion criteria

- A valid PC request reaches the MCU through UART, HPS Linux, the LW bridge and the mailbox.
- The MCU computes the result in software (RV32I assembly) and publishes it to the mailbox.
- The MCU displays only its result on FPGA HEX0-HEX5; HPS does not access the display.
- The PC shows only its own input and reference result; the MCU result is not sent back.
- Errors (malformed input, division by zero, timeout) and repeated requests behave deterministically.
- The SDRAM-backed DMEM path is unchanged and independent of the mailbox.
- All existing Milestone 3 tests still pass, and the design and tests are documented.
