// Immediate generator.
//   i_imm_sel: 3'd0 I, 3'd1 S, 3'd2 B, 3'd3 U, 3'd4 J
module immgen (
  input  logic [31:0] i_instr,
  input  logic [2:0]  i_imm_sel,
  output logic [31:0] o_imm
);

  always_comb begin
    case (i_imm_sel)
      3'd0:    o_imm = {{20{i_instr[31]}}, i_instr[31:20]};
      3'd1:    o_imm = {{20{i_instr[31]}}, i_instr[31:25], i_instr[11:7]};
      3'd2:    o_imm = {{19{i_instr[31]}}, i_instr[31], i_instr[7], i_instr[30:25], i_instr[11:8], 1'b0};
      3'd3:    o_imm = {i_instr[31:12], 12'd0};
      3'd4:    o_imm = {{11{i_instr[31]}}, i_instr[31], i_instr[19:12], i_instr[20], i_instr[30:21], 1'b0};
      default: o_imm = 32'd0;
    endcase
  end

endmodule : immgen
