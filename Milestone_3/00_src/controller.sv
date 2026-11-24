// Controller: decodes the instruction, validates the encoding and drives the datapath.
// Illegal instructions (and reset) produce no register/memory write and o_instr_valid = 0;
// the PC simply advances to pc + 4 for them.
//
//   o_imm_sel : 0 I, 1 S, 2 B, 3 U, 4 J
//   o_opa_sel : 0 rs1, 1 pc
//   o_opb_sel : 0 rs2, 1 imm
//   o_wb_sel  : 0 alu, 1 load data, 2 pc+4, 3 imm
//   o_alu_op  : 0 ADD, 1 SUB, 2 SLT, 3 SLTU, 4 XOR, 5 OR, 6 AND, 7 SLL, 8 SRL, 9 SRA
module controller (
  input  logic        i_rstn,
  input  logic [31:0] i_instr,
  input  logic        i_br_equal,
  input  logic        i_br_less,
  output logic        o_pc_sel,
  output logic        o_rd_wren,
  output logic [2:0]  o_imm_sel,
  output logic        o_br_unsigned,
  output logic        o_opa_sel,
  output logic        o_opb_sel,
  output logic [3:0]  o_alu_op,
  output logic        o_mem_wren,
  output logic [1:0]  o_wb_sel,
  output logic        o_instr_valid
);

  localparam logic [6:0] OP_LUI    = 7'b0110111;
  localparam logic [6:0] OP_AUIPC  = 7'b0010111;
  localparam logic [6:0] OP_JAL    = 7'b1101111;
  localparam logic [6:0] OP_JALR   = 7'b1100111;
  localparam logic [6:0] OP_BRANCH = 7'b1100011;
  localparam logic [6:0] OP_LOAD   = 7'b0000011;
  localparam logic [6:0] OP_STORE  = 7'b0100011;
  localparam logic [6:0] OP_IMM    = 7'b0010011;
  localparam logic [6:0] OP_REG    = 7'b0110011;

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

  logic [6:0] opcode;
  logic [2:0] f3;
  logic [6:0] f7;

  logic       valid;
  logic       take;
  logic       rd_wren_raw;
  logic       mem_wren_raw;
  logic       br_cond;
  logic [3:0] alu_f3;

  assign opcode = i_instr[6:0];
  assign f3     = i_instr[14:12];
  assign f7     = i_instr[31:25];

  // ALU operation derived from funct3 (OP / OP-IMM)
  always_comb begin
    case (f3)
      3'b000:  alu_f3 = ((opcode == OP_REG) && i_instr[30]) ? ALU_SUB : ALU_ADD;
      3'b001:  alu_f3 = ALU_SLL;
      3'b010:  alu_f3 = ALU_SLT;
      3'b011:  alu_f3 = ALU_SLTU;
      3'b100:  alu_f3 = ALU_XOR;
      3'b101:  alu_f3 = i_instr[30] ? ALU_SRA : ALU_SRL;
      3'b110:  alu_f3 = ALU_OR;
      default: alu_f3 = ALU_AND;
    endcase
  end

  // Branch condition
  always_comb begin
    case (f3)
      3'b000:  br_cond = i_br_equal;
      3'b001:  br_cond = ~i_br_equal;
      3'b100,
      3'b110:  br_cond = i_br_less;
      3'b101,
      3'b111:  br_cond = ~i_br_less;
      default: br_cond = 1'b0;
    endcase
  end

  assign o_br_unsigned = f3[1];

  always_comb begin
    valid        = 1'b0;
    take         = 1'b0;
    rd_wren_raw  = 1'b0;
    mem_wren_raw = 1'b0;
    o_imm_sel    = 3'd0;
    o_opa_sel    = 1'b0;
    o_opb_sel    = 1'b1;
    o_alu_op     = ALU_ADD;
    o_wb_sel     = 2'd0;

    if (i_instr[1:0] == 2'b11) begin
      case (opcode)
        OP_LUI: begin
          valid       = 1'b1;
          o_imm_sel   = 3'd3;
          rd_wren_raw = 1'b1;
          o_wb_sel    = 2'd3;
        end
        OP_AUIPC: begin
          valid       = 1'b1;
          o_imm_sel   = 3'd3;
          o_opa_sel   = 1'b1;
          rd_wren_raw = 1'b1;
        end
        OP_JAL: begin
          valid       = 1'b1;
          o_imm_sel   = 3'd4;
          o_opa_sel   = 1'b1;
          take        = 1'b1;
          rd_wren_raw = 1'b1;
          o_wb_sel    = 2'd2;
        end
        OP_JALR: begin
          valid       = (f3 == 3'b000);
          o_imm_sel   = 3'd0;
          take        = 1'b1;
          rd_wren_raw = 1'b1;
          o_wb_sel    = 2'd2;
        end
        OP_BRANCH: begin
          valid       = (f3 != 3'b010) && (f3 != 3'b011);
          o_imm_sel   = 3'd2;
          o_opa_sel   = 1'b1;
          take        = br_cond;
        end
        OP_LOAD: begin
          valid       = (f3 == 3'b000) || (f3 == 3'b001) || (f3 == 3'b010) ||
                        (f3 == 3'b100) || (f3 == 3'b101);
          o_imm_sel   = 3'd0;
          rd_wren_raw = 1'b1;
          o_wb_sel    = 2'd1;
        end
        OP_STORE: begin
          valid        = (f3 == 3'b000) || (f3 == 3'b001) || (f3 == 3'b010);
          o_imm_sel    = 3'd1;
          mem_wren_raw = 1'b1;
        end
        OP_IMM: begin
          o_imm_sel   = 3'd0;
          rd_wren_raw = 1'b1;
          o_alu_op    = alu_f3;
          case (f3)
            3'b001:  valid = (f7 == 7'b0000000);
            3'b101:  valid = (f7 == 7'b0000000) || (f7 == 7'b0100000);
            default: valid = 1'b1;
          endcase
        end
        OP_REG: begin
          o_opb_sel   = 1'b0;
          rd_wren_raw = 1'b1;
          o_alu_op    = alu_f3;
          case (f3)
            3'b000,
            3'b101:  valid = (f7 == 7'b0000000) || (f7 == 7'b0100000);
            default: valid = (f7 == 7'b0000000);
          endcase
        end
        default: valid = 1'b0;
      endcase
    end
  end

  assign o_instr_valid = valid & i_rstn;
  assign o_rd_wren     = rd_wren_raw  & o_instr_valid;
  assign o_mem_wren    = mem_wren_raw & o_instr_valid;
  assign o_pc_sel      = take         & o_instr_valid;

endmodule : controller
