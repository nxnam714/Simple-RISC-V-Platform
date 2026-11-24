// Load/Store Unit. DMEM returns the aligned 32-bit word containing i_addr;
// the LSU selects/extends the addressed byte or halfword for loads and
// aligns data + generates the byte mask for stores.
//   i_funct3: 000 B, 001 H, 010 W, 100 BU, 101 HU
module lsu (
  input  logic [31:0] i_addr,
  input  logic [31:0] i_st_data,
  input  logic [2:0]  i_funct3,
  input  logic        i_wren,
  input  logic [31:0] i_dmem_rdata,
  output logic [31:0] o_ld_data,
  output logic [31:0] o_dmem_addr,
  output logic [31:0] o_dmem_wdata,
  output logic [3:0]  o_dmem_bmsk,
  output logic        o_dmem_wren
);

  logic [7:0]  ld_byte;
  logic [15:0] ld_half;

  assign o_dmem_addr = i_addr;
  assign o_dmem_wren = i_wren;

  // Store path
  always_comb begin
    case (i_funct3[1:0])
      2'b00: begin
        o_dmem_wdata = {4{i_st_data[7:0]}};
        case (i_addr[1:0])
          2'b00:   o_dmem_bmsk = 4'b0001;
          2'b01:   o_dmem_bmsk = 4'b0010;
          2'b10:   o_dmem_bmsk = 4'b0100;
          default: o_dmem_bmsk = 4'b1000;
        endcase
      end
      2'b01: begin
        o_dmem_wdata = {2{i_st_data[15:0]}};
        o_dmem_bmsk  = i_addr[1] ? 4'b1100 : 4'b0011;
      end
      default: begin
        o_dmem_wdata = i_st_data;
        o_dmem_bmsk  = 4'b1111;
      end
    endcase
    if (!i_wren) o_dmem_bmsk = 4'b0000;
  end

  // Load path
  always_comb begin
    case (i_addr[1:0])
      2'b00:   ld_byte = i_dmem_rdata[7:0];
      2'b01:   ld_byte = i_dmem_rdata[15:8];
      2'b10:   ld_byte = i_dmem_rdata[23:16];
      default: ld_byte = i_dmem_rdata[31:24];
    endcase
    ld_half = i_addr[1] ? i_dmem_rdata[31:16] : i_dmem_rdata[15:0];

    case (i_funct3)
      3'b000:  o_ld_data = {{24{ld_byte[7]}}, ld_byte};
      3'b001:  o_ld_data = {{16{ld_half[15]}}, ld_half};
      3'b100:  o_ld_data = {24'd0, ld_byte};
      3'b101:  o_ld_data = {16'd0, ld_half};
      default: o_ld_data = i_dmem_rdata;
    endcase
  end

endmodule : lsu
