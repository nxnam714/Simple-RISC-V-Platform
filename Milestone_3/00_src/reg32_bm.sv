// 32-bit register with byte-mask write.
//   - Asynchronous active-low reset to 0.
//   - On i_we, each byte lane with i_bmsk[k] = 1 takes i_wdata.
//   - Bits with USED_MASK = 0 are never stored and always read 0.
module reg32_bm #(
    parameter logic [31:0] USED_MASK = 32'hFFFF_FFFF
) (
    input  logic        i_clk   ,
    input  logic        i_rstn  ,
    input  logic        i_we    ,
    input  logic [ 3:0] i_bmsk  ,
    input  logic [31:0] i_wdata ,
    output logic [31:0] o_q
);

  logic [31:0] q;
  logic [31:0] lane_mask;
  logic [31:0] wr_mask;

  assign lane_mask = {{8{i_bmsk[3]}}, {8{i_bmsk[2]}}, {8{i_bmsk[1]}}, {8{i_bmsk[0]}}};
  assign wr_mask   = lane_mask & USED_MASK;

  always_ff @(posedge i_clk or negedge i_rstn) begin
    if (!i_rstn)   q <= 32'h0;
    else if (i_we) q <= (q & ~wr_mask) | (i_wdata & wr_mask);
  end

  assign o_q = q & USED_MASK;

endmodule
