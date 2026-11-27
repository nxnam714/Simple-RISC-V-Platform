// Simple single-master bus: address decoding, write-enable routing and
// read-data selection. Every access completes in the same cycle.
//
//   0x8400_0000 - 0x8400_3FFF  DMEM
//   0x4000_0000 - 0x4000_0FFF  LED
//   0x4000_1000 - 0x4000_1FFF  HEX
//   0x4000_2000 - 0x4000_2FFF  SW
//   0x4000_3000 - 0x4000_3FFF  KEY
//   0x4000_4000 - 0x4000_4FFF  Timer
//   0x4000_5000 - 0x4000_5FFF  GPIO
//   0x4000_7000 - 0x4000_7FFF  Calculator mailbox
//   0x4000_F000 - 0x4000_FFFF  Testbench device
//   anything else              unmapped (read 0, write ignored)
module mcu_bus (
    // Processor load/store port
    input  logic [31:0] i_core_addr   ,
    input  logic        i_core_rden   ,
    input  logic        i_core_wren   ,
    output logic [31:0] o_core_rdata  ,
    // Per-device read/write enables
    output logic        o_dmem_rden   ,
    output logic        o_dmem_wren   ,
    output logic        o_led_wren    ,
    output logic        o_hex_wren    ,
    output logic        o_timer_wren  ,
    output logic        o_gpio_wren   ,
    output logic        o_mailbox_wren,
    output logic        o_test_wren   ,
    // Per-device read data
    input  logic [31:0] i_dmem_rdata  ,
    input  logic [31:0] i_led_rdata   ,
    input  logic [31:0] i_hex_rdata   ,
    input  logic [31:0] i_sw_rdata    ,
    input  logic [31:0] i_key_rdata   ,
    input  logic [31:0] i_timer_rdata ,
    input  logic [31:0] i_gpio_rdata  ,
    input  logic [31:0] i_mailbox_rdata
);

  logic io_page;
  logic sel_dmem, sel_led, sel_hex, sel_sw, sel_key, sel_timer, sel_gpio, sel_mailbox, sel_test;

  assign io_page   = (i_core_addr[31:16] == 16'h4000);
  assign sel_led   = io_page & (i_core_addr[15:12] == 4'h0);
  assign sel_hex   = io_page & (i_core_addr[15:12] == 4'h1);
  assign sel_sw    = io_page & (i_core_addr[15:12] == 4'h2);
  assign sel_key   = io_page & (i_core_addr[15:12] == 4'h3);
  assign sel_timer = io_page & (i_core_addr[15:12] == 4'h4);
  assign sel_gpio  = io_page & (i_core_addr[15:12] == 4'h5);
  assign sel_mailbox = io_page & (i_core_addr[15:12] == 4'h7);
  assign sel_test  = io_page & (i_core_addr[15:12] == 4'hF);
  assign sel_dmem  = (i_core_addr[31:16] == 16'h8400) & (i_core_addr[15:14] == 2'b00);

  // Read/write enables for DMEM and mapped peripherals.
  assign o_dmem_rden  = i_core_rden & sel_dmem;
  assign o_dmem_wren  = i_core_wren & sel_dmem;
  assign o_led_wren   = i_core_wren & sel_led;
  assign o_hex_wren   = i_core_wren & sel_hex;
  assign o_timer_wren = i_core_wren & sel_timer;
  assign o_gpio_wren  = i_core_wren & sel_gpio;
  assign o_mailbox_wren = i_core_wren & sel_mailbox;
  assign o_test_wren  = i_core_wren & sel_test;

  // A load returns the data of the mapped device; unmapped/test device -> 0.
  assign o_core_rdata = ({32{sel_dmem}}  & i_dmem_rdata)
                      | ({32{sel_led}}   & i_led_rdata)
                      | ({32{sel_hex}}   & i_hex_rdata)
                      | ({32{sel_sw}}    & i_sw_rdata)
                      | ({32{sel_key}}   & i_key_rdata)
                      | ({32{sel_timer}} & i_timer_rdata)
                      | ({32{sel_gpio}}   & i_gpio_rdata)
                      | ({32{sel_mailbox}} & i_mailbox_rdata);

endmodule
