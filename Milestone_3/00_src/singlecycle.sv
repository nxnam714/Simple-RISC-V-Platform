module singlecycle (
    input  logic        i_clk        ,
    input  logic        i_rstn       ,
    output logic        o_db_valid   ,
    output logic [31:0] o_imem_addr  ,
    input  logic [31:0] i_imem_rdata ,
    output logic [31:0] o_dmem_addr  ,
    output logic [31:0] o_dmem_wdata ,
    output logic [ 3:0] o_dmem_bmsk  ,
    output logic        o_dmem_wren  ,
    input  logic [31:0] i_dmem_rdata
);

// Insert your code here.
  // Program counter
  logic [31:0] pc, pc_four, pc_next;
  logic        pc_sel;

  // Datapath
  logic [31:0] instr;
  logic [31:0] rs1_data, rs2_data;
  logic [31:0] imm;
  logic [31:0] op_a, op_b;
  logic [31:0] alu_data;
  logic [31:0] ld_data;
  logic [31:0] wb_data;

  // Control
  logic        rd_wren;
  logic [2:0]  imm_sel;
  logic        br_un, br_eq, br_lt;
  logic        opa_sel, opb_sel;
  logic [3:0]  alu_op;
  logic        mem_wren;
  logic [1:0]  wb_sel;

  logic        pc4_cout;

  assign instr       = i_imem_rdata;
  assign o_imem_addr = pc;

  // PC register
  always_ff @(posedge i_clk or negedge i_rstn) begin
    if (!i_rstn) pc <= 32'h8000_0000;
    else         pc <= pc_next;
  end

  adder u_pc_adder (
    .i_a        (pc),
    .i_b        (32'd4),
    .i_sub      (1'b0),
    .i_unsigned (1'b1),
    .o_sum      (pc_four),
    .o_cout     (pc4_cout)
  );

  // Jump/branch targets come from the ALU; bit 0 is cleared (JALR).
  assign pc_next = pc_sel ? {alu_data[31:1], 1'b0} : pc_four;

  controller u_ctrl (
    .i_rstn        (i_rstn),
    .i_instr       (instr),
    .i_br_equal    (br_eq),
    .i_br_less     (br_lt),
    .o_pc_sel      (pc_sel),
    .o_rd_wren     (rd_wren),
    .o_imm_sel     (imm_sel),
    .o_br_unsigned (br_un),
    .o_opa_sel     (opa_sel),
    .o_opb_sel     (opb_sel),
    .o_alu_op      (alu_op),
    .o_mem_wren    (mem_wren),
    .o_wb_sel      (wb_sel),
    .o_instr_valid (o_db_valid)
  );

  regfile u_regfile (
    .i_clk      (i_clk),
    .i_rstn     (i_rstn),
    .i_rs1_addr (instr[19:15]),
    .i_rs2_addr (instr[24:20]),
    .i_rd_addr  (instr[11:7]),
    .i_rd_data  (wb_data),
    .i_rd_wren  (rd_wren),
    .o_rs1_data (rs1_data),
    .o_rs2_data (rs2_data)
  );

  immgen u_immgen (
    .i_instr   (instr),
    .i_imm_sel (imm_sel),
    .o_imm     (imm)
  );

  brc u_brc (
    .i_rs1_data    (rs1_data),
    .i_rs2_data    (rs2_data),
    .i_br_unsigned (br_un),
    .o_br_equal    (br_eq),
    .o_br_less     (br_lt)
  );

  assign op_a = opa_sel ? pc  : rs1_data;
  assign op_b = opb_sel ? imm : rs2_data;

  alu u_alu (
    .i_op_a     (op_a),
    .i_op_b     (op_b),
    .i_alu_op   (alu_op),
    .o_alu_data (alu_data)
  );

  lsu u_lsu (
    .i_addr       (alu_data),
    .i_st_data    (rs2_data),
    .i_funct3     (instr[14:12]),
    .i_wren       (mem_wren),
    .i_dmem_rdata (i_dmem_rdata),
    .o_ld_data    (ld_data),
    .o_dmem_addr  (o_dmem_addr),
    .o_dmem_wdata (o_dmem_wdata),
    .o_dmem_bmsk  (o_dmem_bmsk),
    .o_dmem_wren  (o_dmem_wren)
  );

  always_comb begin
    case (wb_sel)
      2'd0:    wb_data = alu_data;
      2'd1:    wb_data = ld_data;
      2'd2:    wb_data = pc_four;
      default: wb_data = imm;
    endcase
  end

  logic unused_ok;
  assign unused_ok = &{1'b0, pc4_cout};

endmodule


