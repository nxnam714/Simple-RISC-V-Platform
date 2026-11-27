# HPS calculator service

This Linux service is a userspace program for HPS Linux. It implements the HPS
side of the calculator protocol:

1. Read a newline-delimited request from a configured 115200-8N1 UART.
2. Validate signed 8-bit operands and the operation code (`0` add, `1` subtract,
   `2` multiply, `3` signed divide).
3. Write the request fields to the FPGA mailbox through `/dev/mem`.
4. Wait for the matching `RES_SEQ` completion sequence.

Valid MCU results are **not** written to the UART. The PC may display its own
reference calculation; the MCU displays only its signed decimal result on the
FPGA's HEX0-HEX5 seven-segment displays. The MCU application drives those
existing FPGA peripherals directly. HPS Linux does not write to the LCD or
seven-segment displays and does not read the result/status registers. Invalid
requests and timeouts are reported to the UART as errors.

## Build

```sh
make
```

The host build only checks that the service compiles. It does not configure or
access HPS hardware.

## Board configuration

All hardware-specific values are required at launch; the service deliberately
does not assume physical addresses or device names:

```sh
sudo ./build/calculator_service \
  --uart /dev/ttyS0 \
  --bridge-base 0x<verified-bridge-base> \
  --bridge-span 0x<verified-bridge-span> \
  --mailbox-offset 0x<Platform-Designer-mailbox-offset> \
  --timeout-ms 5000
```

Obtain the bridge mapping, span, and mailbox offset from the generated
Platform Designer system and the running Linux device tree. Do not copy example
addresses from another DE10-Standard image. `/dev/mem` access usually requires
root or an equivalent device/driver policy.

The DE10-Standard USB-UART is commonly the HPS UART used by the Linux console.
Before using it for calculator input, confirm the board image configuration and
disable the serial console/getty on that port or select another UART. Otherwise
login/kernel output can corrupt the request stream.

## Mailbox register map

All registers are little-endian 32-bit words. The offset is relative to the
mailbox's HPS bridge address.

| Offset | Register | Writer | Meaning |
|---:|---|---|---|
| `0x00` | `REQ_A` | HPS | Sign-extended operand A |
| `0x04` | `REQ_B` | HPS | Sign-extended operand B |
| `0x08` | `REQ_OP` | HPS | Operation code 0 through 3 |
| `0x0c` | `REQ_SEQ` | HPS | Request sequence, written last |
| `0x10` | `RES_VALUE` | MCU | Sign-extended signed result; available for debug, not read by HPS |
| `0x14` | `RES_STATUS` | MCU | 0 OK, 1 divide by zero, 2 invalid request; available for debug |
| `0x18` | `RES_SEQ` | MCU | Completed request sequence, written last |

The mailbox is mapped into the RISC-V bus at `0x40007000`; it is not in SDRAM.
On successful requests, the MCU displays only the result in signed decimal on
HEX0-HEX4, with a minus sign on HEX5 when needed. Division by zero and invalid
requests display `E`. The existing standalone SW/KEY-to-HEX/LED calculator
remains separate and unchanged.
