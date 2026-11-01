// 32-bit barrel shifter (5-stage logarithmic, single right-shifter + bit reversal for left shift).
// No shift operator and no function used.
//   i_mode: 2'b00 logical right, 2'b01 logical left, 2'b10 arithmetic right, 2'b11 zero
module shifter (
  input  logic [31:0] i_data,
  input  logic [4:0]  i_shamt,
  input  logic [1:0]  i_mode,
  output logic [31:0] o_data
);

  logic        is_left;
  logic        fill;
  logic [31:0] data_rev;
  logic [31:0] s0, s1, s2, s3, s4, s5;
  logic [31:0] s5_rev;
  logic [31:0] res;

  assign is_left = (i_mode == 2'b01);
  assign fill    = (i_mode == 2'b10) & i_data[31];

  assign data_rev = {i_data[0],  i_data[1],  i_data[2],  i_data[3],  i_data[4],  i_data[5],  i_data[6],  i_data[7],
                     i_data[8],  i_data[9],  i_data[10], i_data[11], i_data[12], i_data[13], i_data[14], i_data[15],
                     i_data[16], i_data[17], i_data[18], i_data[19], i_data[20], i_data[21], i_data[22], i_data[23],
                     i_data[24], i_data[25], i_data[26], i_data[27], i_data[28], i_data[29], i_data[30], i_data[31]};

  assign s0 = is_left ? data_rev : i_data;

  assign s1 = i_shamt[0] ? {{1{fill}},  s0[31:1]}  : s0;
  assign s2 = i_shamt[1] ? {{2{fill}},  s1[31:2]}  : s1;
  assign s3 = i_shamt[2] ? {{4{fill}},  s2[31:4]}  : s2;
  assign s4 = i_shamt[3] ? {{8{fill}},  s3[31:8]}  : s3;
  assign s5 = i_shamt[4] ? {{16{fill}}, s4[31:16]} : s4;

  assign s5_rev = {s5[0],  s5[1],  s5[2],  s5[3],  s5[4],  s5[5],  s5[6],  s5[7],
                   s5[8],  s5[9],  s5[10], s5[11], s5[12], s5[13], s5[14], s5[15],
                   s5[16], s5[17], s5[18], s5[19], s5[20], s5[21], s5[22], s5[23],
                   s5[24], s5[25], s5[26], s5[27], s5[28], s5[29], s5[30], s5[31]};

  assign res = is_left ? s5_rev : s5;

  assign o_data = (i_mode == 2'b11) ? 32'd0 : res;

endmodule : shifter
