// ALU: one adder, one comparator, one shifter shared by all operations.
// No package dependency (keeps compile order independent).
//   4'd0 ADD, 1 SUB, 2 SLT, 3 SLTU, 4 XOR, 5 OR, 6 AND, 7 SLL, 8 SRL, 9 SRA
module alu (
  input  logic [31:0] i_op_a,
  input  logic [31:0] i_op_b,
  input  logic [3:0]  i_alu_op,
  output logic [31:0] o_alu_data
);

  localparam logic [3:0] ALU_ADD  = 4'd0;
  localparam logic [3:0] ALU_SUB  = 4'd1;
  localparam logic [3:0] ALU_SLT  = 4'd2;
  localparam logic [3:0] ALU_SLTU = 4'd3;
  localparam logic [3:0] ALU_XOR  = 4'd4;
  localparam logic [3:0] ALU_OR   = 4'd5;
  localparam logic [3:0] ALU_AND  = 4'd6;
  localparam logic [3:0] ALU_SLL  = 4'd7;
  localparam logic [3:0] ALU_SRL  = 4'd8;
  localparam logic [3:0] ALU_SRA  = 4'd9;

  logic        add_sub;
  logic        cmp_unsigned;
  logic [1:0]  shift_mode;

  logic [31:0] add_sum;
  logic        add_cout;
  logic        cmp_eq, cmp_lt, cmp_gt;
  logic [31:0] shift_data;

  assign add_sub      = (i_alu_op == ALU_SUB);
  assign cmp_unsigned = (i_alu_op == ALU_SLTU);

  always_comb begin
    case (i_alu_op)
      ALU_SLL: shift_mode = 2'b01;
      ALU_SRL: shift_mode = 2'b00;
      ALU_SRA: shift_mode = 2'b10;
      default: shift_mode = 2'b11;
    endcase
  end

  adder u_adder (
    .i_a        (i_op_a),
    .i_b        (i_op_b),
    .i_sub      (add_sub),
    .i_unsigned (1'b0),
    .o_sum      (add_sum),
    .o_cout     (add_cout)
  );

  comparator u_comparator (
    .i_a        (i_op_a),
    .i_b        (i_op_b),
    .i_unsigned (cmp_unsigned),
    .o_eq       (cmp_eq),
    .o_lt       (cmp_lt),
    .o_gt       (cmp_gt)
  );

  shifter u_shifter (
    .i_data  (i_op_a),
    .i_shamt (i_op_b[4:0]),
    .i_mode  (shift_mode),
    .o_data  (shift_data)
  );

  always_comb begin
    case (i_alu_op)
      ALU_ADD,
      ALU_SUB:  o_alu_data = add_sum;
      ALU_SLT,
      ALU_SLTU: o_alu_data = {31'd0, cmp_lt};
      ALU_XOR:  o_alu_data = i_op_a ^ i_op_b;
      ALU_OR:   o_alu_data = i_op_a | i_op_b;
      ALU_AND:  o_alu_data = i_op_a & i_op_b;
      ALU_SLL,
      ALU_SRL,
      ALU_SRA:  o_alu_data = shift_data;
      default:  o_alu_data = 32'd0;
    endcase
  end

  logic unused_ok;
  assign unused_ok = &{1'b0, add_cout, cmp_eq, cmp_gt};

endmodule : alu
