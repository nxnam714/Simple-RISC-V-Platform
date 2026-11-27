# Integer square root demo

The app edits an unsigned four-digit decimal input (`0000` to `9999`) with the
switches and push buttons, then displays its square root rounded to two decimal
places.

- `SW[1:0]` selects the decimal input digit: `0` = HEX0/ones, `1` = HEX1/tens,
  `2` = HEX2/hundreds, `3` = HEX3/thousands.
- Press `KEY1` to increment the selected digit. The digit wraps from 9 to 0;
  other digits do not change.
- Press `KEY2` to calculate and display the rounded square root.
- The result is displayed on HEX0-HEX3 as a fixed-point value with two
  fractional digits. The DE10-Standard HEX display has no decimal-point
  segment, so the decimal point is implicit: `3162` means `31.62`.
- After displaying a result, pressing `KEY1` edits the input again and restores
  its four-digit display. `LEDR[9:0]` mirror the current input while editing.

The MCU exposes pressed keys as logic 1 to software. The app detects rising
edges, so release a button before pressing it again. `KEY0` remains the board
reset button and is not used as an application button.

The square root is calculated as an integer square root of `input * 10000`,
then rounded to the nearest hundredth without floating-point operations.

## Build

Run `make` in this directory to create `build/int_sqrt_4b.hex`. The startup
routine initializes the stack pointer before entering the C `main`.
