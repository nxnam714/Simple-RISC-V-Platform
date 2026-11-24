module dmem #(
    parameter logic [31:0] START_ADDR = 32'h8008_0000,
    parameter int unsigned ADDR_WIDTH = 16
) (
    input  logic        i_clk   ,
    input  logic [31:0] i_addr  ,
    input  logic [31:0] i_wdata ,
    input  logic [ 3:0] i_bmsk  ,
    input  logic        i_wren  ,
    output logic [31:0] o_rdata
);

// Insert your code here.
  logic [7:0] mem [2**ADDR_WIDTH];

  logic [ADDR_WIDTH-1:2] word_index;
  assign word_index = i_addr[ADDR_WIDTH-1:2];

  always_ff @(posedge i_clk) begin
    if (i_wren) begin
      if (i_bmsk[0]) mem[{word_index, 2'b00}] <= i_wdata[7:0];
      if (i_bmsk[1]) mem[{word_index, 2'b01}] <= i_wdata[15:8];
      if (i_bmsk[2]) mem[{word_index, 2'b10}] <= i_wdata[23:16];
      if (i_bmsk[3]) mem[{word_index, 2'b11}] <= i_wdata[31:24];
    end
  end

  assign o_rdata = {mem[{word_index, 2'b11}],
                    mem[{word_index, 2'b10}],
                    mem[{word_index, 2'b01}],
                    mem[{word_index, 2'b00}]};

  logic unused_ok;
  assign unused_ok = &{1'b0, START_ADDR};

endmodule

