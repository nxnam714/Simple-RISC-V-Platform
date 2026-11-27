`timescale 1ns / 1ps

module tb_full_calculator;

  logic        mcu_clk = 1'b0;
  logic        hps_clk = 1'b0;
  logic        reset_n = 1'b0;
  logic        hps_reset_n = 1'b0;
  logic [ 4:0] hps_address = 5'b0;
  logic        hps_read = 1'b0;
  logic        hps_write = 1'b0;
  logic [31:0] hps_writedata = 32'b0;
  logic [ 3:0] hps_byteenable = 4'hf;
  logic [31:0] hps_readdata;
  logic        hps_waitrequest;
  logic [6:0]  hex [0:5];
  int          test_count = 0;

  always #7 mcu_clk = ~mcu_clk;
  always #5 hps_clk = ~hps_clk;

  milestone3 dut (
    .i_clk         (mcu_clk),
    .i_rstn        (reset_n),
    .i_sw          (18'b0),
    .i_key         (4'b0),
    .o_ledr        (),
    .o_ledg        (),
    .o_ledb        (),
    .o_hex0        (hex[0]),
    .o_hex1        (hex[1]),
    .o_hex2        (hex[2]),
    .o_hex3        (hex[3]),
    .o_hex4        (hex[4]),
    .o_hex5        (hex[5]),
    .o_hex6        (),
    .o_hex7        (),
    .i_gpio_in     (32'b0),
    .o_gpio_out    (),
    .o_gpio_dir    (),
    .i_hps_clk         (hps_clk),
    .i_hps_reset_n     (hps_reset_n),
    .i_hps_address     (hps_address),
    .i_hps_read        (hps_read),
    .i_hps_write       (hps_write),
    .i_hps_writedata   (hps_writedata),
    .i_hps_byteenable  (hps_byteenable),
    .o_hps_readdata    (hps_readdata),
    .o_hps_waitrequest (hps_waitrequest),
    .o_dram_addr   (),
    .o_dram_ba     (),
    .o_dram_ldqm   (),
    .o_dram_udqm   (),
    .o_dram_ras_n  (),
    .o_dram_cas_n  (),
    .o_dram_cke    (),
    .o_dram_clk    (),
    .o_dram_we_n   (),
    .o_dram_cs_n   (),
    .io_dram_dq    (),
    .o_event_valid (),
    .o_event_data  (),
    .o_print_valid (),
    .o_print_char  (),
    .o_halt        (),
    .o_db_valid    ()
  );

  task automatic write_reg(input logic [4:0] address, input logic [31:0] value);
    @(negedge hps_clk);
    hps_address = address;
    hps_writedata = value;
    hps_byteenable = 4'hf;
    hps_write = 1'b1;
    @(negedge hps_clk);
    hps_write = 1'b0;
  endtask

  task automatic read_reg(input logic [4:0] address, output logic [31:0] value);
    hps_address = address;
    hps_read = 1'b1;
    #1;
    value = hps_readdata;
    hps_read = 1'b0;
  endtask

  task automatic calculate(
      input logic [31:0] operand_a,
      input logic [31:0] operand_b,
      input logic [31:0] operation,
      input logic [31:0] req_sequence,
      input logic [31:0] expected_value,
      input logic [31:0] expected_status
  );
    logic [31:0] observed;
    int timeout;
    begin
      write_reg(5'h00, operand_a);
      write_reg(5'h04, operand_b);
      write_reg(5'h08, operation);
      write_reg(5'h0c, req_sequence);
      observed = 32'b0;
      timeout = 0;
      while ((observed != req_sequence) && (timeout < 20000)) begin
        @(negedge hps_clk);
        read_reg(5'h18, observed);
        timeout++;
      end
      if (observed != req_sequence) $fatal(1, "timeout waiting for sequence %0d", req_sequence);
      read_reg(5'h10, observed);
      if (observed !== expected_value) begin
        $fatal(1, "seq %0d result: got %08x expected %08x", req_sequence, observed, expected_value);
      end
      read_reg(5'h14, observed);
      if (observed !== expected_status) begin
        $fatal(1, "seq %0d status: got %08x expected %08x", req_sequence, observed, expected_status);
      end
      test_count++;
    end
  endtask

  logic [31:0] observed_seq;
  task automatic expect_hex(
      input logic [6:0] hex0,
      input logic [6:0] hex1,
      input logic [6:0] hex2,
      input logic [6:0] hex3,
      input logic [6:0] hex4,
      input logic [6:0] hex5
  );
    begin
      #1;
      if ({hex[5], hex[4], hex[3], hex[2], hex[1], hex[0]} !==
          {hex5, hex4, hex3, hex2, hex1, hex0}) begin
        $fatal(1, "HEX output got %02x %02x %02x %02x %02x %02x",
               hex[5], hex[4], hex[3], hex[2], hex[1], hex[0]);
      end
    end
  endtask

  initial begin
    repeat (5) @(negedge mcu_clk);
    reset_n = 1'b1;
    hps_reset_n = 1'b1;
    repeat (8) @(negedge mcu_clk);
    calculate(32'hffff_ff80, 32'h0000_007f, 0, 1, 32'hffff_ffff, 0);
    expect_hex(7'h79, 7'h7f, 7'h7f, 7'h7f, 7'h7f, 7'h3f);
    calculate(32'h0000_007f, 32'hffff_ff80, 1, 2, 32'h0000_00ff, 0);
    calculate(32'hffff_ff80, 32'hffff_ff80, 2, 3, 32'h0000_4000, 0);
    expect_hex(7'h19, 7'h00, 7'h30, 7'h02, 7'h79, 7'h7f);
    calculate(32'h0000_007f, 32'hffff_ff80, 2, 4, 32'hffff_c080, 0);
    expect_hex(7'h02, 7'h12, 7'h24, 7'h02, 7'h79, 7'h3f);
    calculate(32'hffff_ff80, 32'hffff_ffff, 3, 5, 32'h0000_0080, 0);
    calculate(32'hffff_ff81, 32'h0000_0002, 3, 6, 32'hffff_ffc1, 0);
    calculate(32'h0000_0007, 32'hffff_fffe, 3, 7, 32'hffff_fffd, 0);
    calculate(32'h0000_0007, 32'h0000_0000, 3, 8, 32'h0000_0000, 1);
    expect_hex(7'h06, 7'h7f, 7'h7f, 7'h7f, 7'h7f, 7'h7f);
    calculate(32'h0000_0007, 32'h0000_0001, 4, 9, 32'h0000_0000, 2);
    calculate(32'h0000_0100, 32'h0000_0001, 0, 10, 32'h0000_0000, 2);

    reset_n = 1'b0;
    repeat (3) @(negedge mcu_clk);
    write_reg(5'h00, 32'h0000_0011);
    write_reg(5'h04, 32'h0000_0003);
    write_reg(5'h08, 32'h0000_0002);
    write_reg(5'h0c, 32'h0000_000b);
    read_reg(5'h18, observed_seq);
    if (observed_seq !== 32'd10) $fatal(1, "completed sequence was not retained across MCU reset");
    reset_n = 1'b1;
    calculate(32'h0000_0011, 32'h0000_0003, 2, 11, 32'h0000_0033, 0);

    $display("PASS full calculator RV32I tests: %0d requests", test_count);
    $finish;
  end

endmodule
