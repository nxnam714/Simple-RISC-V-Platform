// Seven-segment device (base 0x4000_1000)
//   +0x00..+0x1C HEX0..HEX7 R/W, only bits [6:0] used (1 = segment on).
//   Pins are active low: o_hexN = ~HEXN[6:0].
module hex_dev (
    input  logic        i_clk   ,
    input  logic        i_rstn  ,
    input  logic [11:0] i_addr  ,
    input  logic [31:0] i_wdata ,
    input  logic [ 3:0] i_bmsk  ,
    input  logic        i_wren  ,
    output logic [31:0] o_rdata ,
    output logic [ 6:0] o_hex0  ,
    output logic [ 6:0] o_hex1  ,
    output logic [ 6:0] o_hex2  ,
    output logic [ 6:0] o_hex3  ,
    output logic [ 6:0] o_hex4  ,
    output logic [ 6:0] o_hex5  ,
    output logic [ 6:0] o_hex6  ,
    output logic [ 6:0] o_hex7
);

  localparam logic [31:0] SEG_MASK = 32'h0000_007F;

  logic        in_range;
  logic [ 7:0] sel;
  logic [31:0] q0, q1, q2, q3, q4, q5, q6, q7;

  // Offsets 0x00..0x1C: i_addr[11:5] = 0, i_addr[4:2] selects the digit.
  assign in_range = (i_addr[11:5] == 7'h00);
  assign sel[0]   = in_range & (i_addr[4:2] == 3'd0);
  assign sel[1]   = in_range & (i_addr[4:2] == 3'd1);
  assign sel[2]   = in_range & (i_addr[4:2] == 3'd2);
  assign sel[3]   = in_range & (i_addr[4:2] == 3'd3);
  assign sel[4]   = in_range & (i_addr[4:2] == 3'd4);
  assign sel[5]   = in_range & (i_addr[4:2] == 3'd5);
  assign sel[6]   = in_range & (i_addr[4:2] == 3'd6);
  assign sel[7]   = in_range & (i_addr[4:2] == 3'd7);

  reg32_bm #(.USED_MASK(SEG_MASK)) u_hex0 (.i_clk(i_clk), .i_rstn(i_rstn), .i_we(i_wren & sel[0]), .i_bmsk(i_bmsk), .i_wdata(i_wdata), .o_q(q0));
  reg32_bm #(.USED_MASK(SEG_MASK)) u_hex1 (.i_clk(i_clk), .i_rstn(i_rstn), .i_we(i_wren & sel[1]), .i_bmsk(i_bmsk), .i_wdata(i_wdata), .o_q(q1));
  reg32_bm #(.USED_MASK(SEG_MASK)) u_hex2 (.i_clk(i_clk), .i_rstn(i_rstn), .i_we(i_wren & sel[2]), .i_bmsk(i_bmsk), .i_wdata(i_wdata), .o_q(q2));
  reg32_bm #(.USED_MASK(SEG_MASK)) u_hex3 (.i_clk(i_clk), .i_rstn(i_rstn), .i_we(i_wren & sel[3]), .i_bmsk(i_bmsk), .i_wdata(i_wdata), .o_q(q3));
  reg32_bm #(.USED_MASK(SEG_MASK)) u_hex4 (.i_clk(i_clk), .i_rstn(i_rstn), .i_we(i_wren & sel[4]), .i_bmsk(i_bmsk), .i_wdata(i_wdata), .o_q(q4));
  reg32_bm #(.USED_MASK(SEG_MASK)) u_hex5 (.i_clk(i_clk), .i_rstn(i_rstn), .i_we(i_wren & sel[5]), .i_bmsk(i_bmsk), .i_wdata(i_wdata), .o_q(q5));
  reg32_bm #(.USED_MASK(SEG_MASK)) u_hex6 (.i_clk(i_clk), .i_rstn(i_rstn), .i_we(i_wren & sel[6]), .i_bmsk(i_bmsk), .i_wdata(i_wdata), .o_q(q6));
  reg32_bm #(.USED_MASK(SEG_MASK)) u_hex7 (.i_clk(i_clk), .i_rstn(i_rstn), .i_we(i_wren & sel[7]), .i_bmsk(i_bmsk), .i_wdata(i_wdata), .o_q(q7));

  assign o_rdata = ({32{sel[0]}} & q0) | ({32{sel[1]}} & q1)
                 | ({32{sel[2]}} & q2) | ({32{sel[3]}} & q3)
                 | ({32{sel[4]}} & q4) | ({32{sel[5]}} & q5)
                 | ({32{sel[6]}} & q6) | ({32{sel[7]}} & q7);

  assign o_hex0 = ~q0[6:0];
  assign o_hex1 = ~q1[6:0];
  assign o_hex2 = ~q2[6:0];
  assign o_hex3 = ~q3[6:0];
  assign o_hex4 = ~q4[6:0];
  assign o_hex5 = ~q5[6:0];
  assign o_hex6 = ~q6[6:0];
  assign o_hex7 = ~q7[6:0];

endmodule
