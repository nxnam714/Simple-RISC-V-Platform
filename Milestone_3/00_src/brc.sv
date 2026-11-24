// Branch Comparison Unit.
module brc (
  input  logic [31:0] i_rs1_data,
  input  logic [31:0] i_rs2_data,
  input  logic        i_br_unsigned,
  output logic        o_br_equal,
  output logic        o_br_less
);

  logic unused_gt;

  comparator u_cmp (
    .i_a        (i_rs1_data),
    .i_b        (i_rs2_data),
    .i_unsigned (i_br_unsigned),
    .o_eq       (o_br_equal),
    .o_lt       (o_br_less),
    .o_gt       (unused_gt)
  );

endmodule : brc
