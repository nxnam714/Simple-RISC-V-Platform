    .section .text.startup
    .globl _start
    .type _start, @function
_start:
    li      sp, 0x84004000
    call    main
1:
    j       1b
