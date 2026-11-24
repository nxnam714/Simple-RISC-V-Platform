// 32-bit adder/subtractor (Kogge-Stone parallel-prefix adder). No '+' / '-' used.
// Subtraction is a + ~b + 1; the carry-in is injected as generate term at index 0.
module adder (
  input  logic [31:0] i_a,
  input  logic [31:0] i_b,
  input  logic        i_sub,
  input  logic        i_unsigned,
  output logic [31:0] o_sum,
  output logic        o_cout
);

  logic [31:0] b_op;

  logic [32:0] g0, p0;
  logic [32:0] g1, p1;
  logic [32:0] g2, p2;
  logic [32:0] g3, p3;
  logic [32:0] g4, p4;
  logic [32:0] g5, p5;
  logic [32:0] g6;

  assign b_op = i_b ^ {32{i_sub}};

  assign g0 = {i_a & b_op, i_sub};
  assign p0 = {i_a ^ b_op, 1'b0};

  assign g1 = g0 | (p0 & {g0[31:0], 1'b0});
  assign p1 = p0 &       {p0[31:0], 1'b0};

  assign g2 = g1 | (p1 & {g1[30:0], 2'b0});
  assign p2 = p1 &       {p1[30:0], 2'b0};

  assign g3 = g2 | (p2 & {g2[28:0], 4'b0});
  assign p3 = p2 &       {p2[28:0], 4'b0};

  assign g4 = g3 | (p3 & {g3[24:0], 8'b0});
  assign p4 = p3 &       {p3[24:0], 8'b0};

  assign g5 = g4 | (p4 & {g4[16:0], 16'b0});
  assign p5 = p4 &       {p4[16:0], 16'b0};

  assign g6 = g5 | (p5 & {g5[0], 32'b0});

  assign o_sum = p0[32:1] ^ g6[31:0];

  always_comb begin
    if (i_unsigned) begin
      o_cout = i_sub ? ~g6[32] : g6[32];
    end else begin
      o_cout = g6[32] ^ g6[31];
    end
  end

endmodule : adder
