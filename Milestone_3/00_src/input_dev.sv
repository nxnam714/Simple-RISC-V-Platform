// Read-only input device (used for SW at 0x4000_2000 and KEY at 0x4000_3000).
//   +0x0: returns i_value (already zero-extended / polarity-corrected by the parent).
//   Other offsets read 0. Writes are ignored (no storage at all).
module input_dev (
    input  logic [11:0] i_addr  ,
    input  logic [31:0] i_value ,
    output logic [31:0] o_rdata
);

  logic sel;
  assign sel     = (i_addr[11:2] == 10'h000);
  assign o_rdata = {32{sel}} & i_value;

endmodule
