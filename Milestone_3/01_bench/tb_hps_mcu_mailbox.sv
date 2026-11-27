`timescale 1ns / 1ps

module tb_hps_mcu_mailbox;

  logic        hps_clk = 1'b0;
  logic        mcu_clk = 1'b0;
  logic        hps_reset_n = 1'b0;
  logic        mcu_reset_n = 1'b0;
  logic [ 4:0] hps_address = 5'b0;
  logic        hps_read = 1'b0;
  logic        hps_write = 1'b0;
  logic [31:0] hps_writedata = 32'b0;
  logic [ 3:0] hps_byteenable = 4'hf;
  logic [31:0] hps_readdata;
  logic        hps_waitrequest;
  logic [11:0] mcu_addr = 12'b0;
  logic [31:0] mcu_wdata = 32'b0;
  logic [ 3:0] mcu_bmsk = 4'hf;
  logic        mcu_wren = 1'b0;
  logic [31:0] mcu_rdata;

  always #5 hps_clk = ~hps_clk;
  always #7 mcu_clk = ~mcu_clk;

  hps_mcu_mailbox dut (
    .i_hps_clk         (hps_clk),
    .i_hps_reset_n     (hps_reset_n),
    .i_hps_address     (hps_address),
    .i_hps_read        (hps_read),
    .i_hps_write       (hps_write),
    .i_hps_writedata   (hps_writedata),
    .i_hps_byteenable  (hps_byteenable),
    .o_hps_readdata    (hps_readdata),
    .o_hps_waitrequest (hps_waitrequest),
    .i_mcu_clk         (mcu_clk),
    .i_mcu_reset_n     (mcu_reset_n),
    .i_mcu_addr        (mcu_addr),
    .i_mcu_wdata       (mcu_wdata),
    .i_mcu_bmsk        (mcu_bmsk),
    .i_mcu_wren        (mcu_wren),
    .o_mcu_rdata       (mcu_rdata)
  );

  task automatic hps_write_reg(input logic [4:0] address, input logic [31:0] value, input logic [3:0] byteenable);
    @(negedge hps_clk);
    hps_address = address;
    hps_writedata = value;
    hps_byteenable = byteenable;
    hps_write = 1'b1;
    @(negedge hps_clk);
    hps_write = 1'b0;
  endtask

  task automatic mcu_write_reg(input logic [11:0] address, input logic [31:0] value, input logic [3:0] mask);
    @(negedge mcu_clk);
    mcu_addr = address;
    mcu_wdata = value;
    mcu_bmsk = mask;
    mcu_wren = 1'b1;
    @(negedge mcu_clk);
    mcu_wren = 1'b0;
  endtask

  task automatic expect_hps(input logic [4:0] address, input logic [31:0] expected);
    hps_address = address;
    hps_read = 1'b1;
    #1;
    if (hps_readdata !== expected) $fatal(1, "HPS read %02x: got %08x expected %08x", address, hps_readdata, expected);
    hps_read = 1'b0;
  endtask

  task automatic expect_mcu(input logic [11:0] address, input logic [31:0] expected);
    mcu_addr = address;
    #1;
    if (mcu_rdata !== expected) $fatal(1, "MCU read %03x: got %08x expected %08x", address, mcu_rdata, expected);
  endtask

  initial begin
    repeat (3) @(negedge hps_clk);
    hps_reset_n = 1'b1;
    mcu_reset_n = 1'b1;
    repeat (3) @(negedge mcu_clk);

    if (hps_waitrequest !== 1'b0) $fatal(1, "mailbox unexpectedly stalls Avalon");
    expect_hps(5'h00, 32'b0);
    expect_hps(5'h18, 32'b0);
    expect_mcu(12'h000, 32'b0);

    hps_write_reg(5'h00, 32'hffff_ff85, 4'hf);
    hps_write_reg(5'h04, 32'h0000_0007, 4'hf);
    hps_write_reg(5'h08, 32'h0000_0002, 4'hf);
    hps_write_reg(5'h0c, 32'h0000_0001, 4'hf);
    repeat (4) @(negedge mcu_clk);
    expect_mcu(12'h000, 32'hffff_ff85);
    expect_mcu(12'h004, 32'h0000_0007);
    expect_mcu(12'h008, 32'h0000_0002);
    expect_mcu(12'h00c, 32'h0000_0001);

    hps_write_reg(5'h00, 32'h0000_0011, 4'hf);
    hps_write_reg(5'h0c, 32'h0000_0002, 4'hf);
    repeat (2) @(negedge mcu_clk);
    expect_mcu(12'h000, 32'hffff_ff85);
    expect_mcu(12'h00c, 32'h0000_0001);

    mcu_write_reg(12'h010, 32'hffff_ffcb, 4'hf);
    mcu_write_reg(12'h014, 32'h0000_0000, 4'hf);
    mcu_write_reg(12'h018, 32'h0000_0002, 4'hf);
    expect_mcu(12'h018, 32'h0000_0000);
    mcu_write_reg(12'h018, 32'h0000_0001, 4'hf);
    repeat (4) @(negedge hps_clk);
    expect_hps(5'h10, 32'hffff_ffcb);
    expect_hps(5'h14, 32'h0000_0000);
    expect_hps(5'h18, 32'h0000_0001);

    hps_write_reg(5'h00, 32'h1234_5678, 4'b0010);
    expect_hps(5'h00, 32'hffff_5685);

    mcu_reset_n = 1'b0;
    repeat (2) @(negedge mcu_clk);
    mcu_reset_n = 1'b1;
    repeat (2) @(negedge hps_clk);
    expect_hps(5'h18, 32'h0000_0001);
    expect_mcu(12'h018, 32'h0000_0001);

    hps_write_reg(5'h00, 32'hffff_ff80, 4'hf);
    hps_write_reg(5'h04, 32'hffff_ffff, 4'hf);
    hps_write_reg(5'h08, 32'h0000_0003, 4'hf);
    hps_write_reg(5'h0c, 32'h0000_0002, 4'hf);
    repeat (3) @(negedge mcu_clk);
    mcu_reset_n = 1'b0;
    repeat (2) @(negedge mcu_clk);
    mcu_reset_n = 1'b1;
    repeat (4) @(negedge mcu_clk);
    expect_mcu(12'h00c, 32'h0000_0002);
    mcu_write_reg(12'h010, 32'h0000_0080, 4'hf);
    mcu_write_reg(12'h014, 32'h0000_0000, 4'hf);
    mcu_write_reg(12'h018, 32'h0000_0002, 4'hf);
    repeat (4) @(negedge hps_clk);
    expect_hps(5'h18, 32'h0000_0002);
    expect_hps(5'h10, 32'h0000_0080);

    $display("PASS HPS-MCU mailbox CDC, ownership, byte masks, and MCU reset retention");
    $finish;
  end

endmodule
