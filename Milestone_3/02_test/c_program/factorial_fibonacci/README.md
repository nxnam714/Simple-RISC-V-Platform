# Factorial and Fibonacci demo

The application calculates unsigned 64-bit factorials (`n!`) and Fibonacci
numbers. It accepts a four-digit decimal input edited with switches and KEY1,
then shows the result in six-digit pages on HEX0-HEX5.

- `SW[1:0]` selects the decimal input digit: `0` = ones/HEX0, `1` = tens/HEX1,
  `2` = hundreds/HEX2, `3` = thousands/HEX3.
- Press `KEY1` to increment that input digit, wrapping 9 to 0. The input is
  shown on HEX0-HEX3 and mirrored on `LEDR[9:0]`.
- Press `KEY2` to calculate factorial. Valid input is 0-20.
- Press `KEY3` to calculate Fibonacci `F_n`. Valid input is 0-93.
- An out-of-range input displays `E` on HEX0.
- After a valid calculation, `SW[4:2]` selects a result page. Each page shows
  six decimal digits, least-significant digit on HEX0. Page 0 is the least
  significant group; pages 1-3 show progressively more significant groups.
  Pages 4-7 are blank. Leading positions on the highest page are blank.

KEY inputs are active-high in software. Release a button before pressing it
again. `KEY0` remains the board reset button.

The result conversion and factorial arithmetic avoid RV32M instructions and
hosted runtime helpers. Build the image by running `make` here; this creates
`build/factorial_fibonacci_4b.hex`. `startup.S` only initializes the stack
before entering the C `main`.
