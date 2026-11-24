// Timer device (base 0x4000_4000)
//   +0x0 CTRL     R/W  bit0 EN (stored), bit1 CLR (self-clearing, reads 0)
//   +0x4 COUNT_LO R
//   +0x8 COUNT_HI R
// Each cycle: CLR write -> count = 0; else EN -> count + 1; else hold.
// The increment is built from the Milestone 2 adder (no '+' operator).
module timer_dev (
    input  logic        i_clk   ,
    input  logic        i_rstn  ,
    input  logic [11:0] i_addr  ,
    input  logic [31:0] i_wdata ,
    input  logic [ 3:0] i_bmsk  ,
    input  logic        i_wren  ,
    output logic [31:0] o_rdata
);

  logic        sel_ctrl, sel_lo, sel_hi;
  logic        ctrl_we, clr;
  logic        en;
  logic [31:0] cnt_lo, cnt_hi;
  logic [31:0] lo_inc, hi_inc;
  logic        lo_cout, hi_cout;
  logic        lo_full;

  assign sel_ctrl = (i_addr[11:2] == 10'h000);
  assign sel_lo   = (i_addr[11:2] == 10'h001);
  assign sel_hi   = (i_addr[11:2] == 10'h002);

  // EN and CLR live in byte 0: only a write that includes byte 0 affects them.
  assign ctrl_we  = i_wren & sel_ctrl & i_bmsk[0];
  assign clr      = ctrl_we & i_wdata[1];

  always_ff @(posedge i_clk or negedge i_rstn) begin
    if (!i_rstn)      en <= 1'b0;
    else if (ctrl_we) en <= i_wdata[0];
  end

  adder u_lo_inc (
    .i_a        (cnt_lo),
    .i_b        (32'd1),
    .i_sub      (1'b0),
    .i_unsigned (1'b1),
    .o_sum      (lo_inc),
    .o_cout     (lo_cout)
  );

  adder u_hi_inc (
    .i_a        (cnt_hi),
    .i_b        (32'd1),
    .i_sub      (1'b0),
    .i_unsigned (1'b1),
    .o_sum      (hi_inc),
    .o_cout     (hi_cout)
  );

  // Carry into the upper word when the lower word is all ones.
  assign lo_full = &cnt_lo;

  always_ff @(posedge i_clk or negedge i_rstn) begin
    if (!i_rstn) begin
      cnt_lo <= 32'h0;
      cnt_hi <= 32'h0;
    end
    else if (clr) begin
      cnt_lo <= 32'h0;
      cnt_hi <= 32'h0;
    end
    else if (en) begin
      cnt_lo <= lo_inc;
      if (lo_full) cnt_hi <= hi_inc;
    end
  end

  assign o_rdata = ({32{sel_ctrl}} & {31'h0, en})
                 | ({32{sel_lo}}   & cnt_lo)
                 | ({32{sel_hi}}   & cnt_hi);

  logic unused_ok;
  assign unused_ok = &{1'b0, lo_cout, hi_cout};

endmodule
