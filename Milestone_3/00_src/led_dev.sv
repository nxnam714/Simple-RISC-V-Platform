// LED device (base 0x4000_0000)
//   +0x0 LEDR R/W, +0x4 LEDG R/W, +0x8 LEDB R/W, other offsets read 0.
module led_dev (
    input  logic        i_clk   ,
    input  logic        i_rstn  ,
    input  logic [11:0] i_addr  ,
    input  logic [31:0] i_wdata ,
    input  logic [ 3:0] i_bmsk  ,
    input  logic        i_wren  ,
    output logic [31:0] o_rdata ,
    output logic [31:0] o_ledr  ,
    output logic [31:0] o_ledg  ,
    output logic [31:0] o_ledb
);

  logic sel_r, sel_g, sel_b;

  assign sel_r = (i_addr[11:2] == 10'h000);
  assign sel_g = (i_addr[11:2] == 10'h001);
  assign sel_b = (i_addr[11:2] == 10'h002);

  reg32_bm u_ledr (.i_clk(i_clk), .i_rstn(i_rstn), .i_we(i_wren & sel_r), .i_bmsk(i_bmsk), .i_wdata(i_wdata), .o_q(o_ledr));
  reg32_bm u_ledg (.i_clk(i_clk), .i_rstn(i_rstn), .i_we(i_wren & sel_g), .i_bmsk(i_bmsk), .i_wdata(i_wdata), .o_q(o_ledg));
  reg32_bm u_ledb (.i_clk(i_clk), .i_rstn(i_rstn), .i_we(i_wren & sel_b), .i_bmsk(i_bmsk), .i_wdata(i_wdata), .o_q(o_ledb));

  assign o_rdata = ({32{sel_r}} & o_ledr)
                 | ({32{sel_g}} & o_ledg)
                 | ({32{sel_b}} & o_ledb);

endmodule
