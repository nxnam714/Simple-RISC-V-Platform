module imem #(
    parameter logic [31:0] START_ADDR  = 32'h8000_0000,
    parameter int unsigned ADDR_WIDTH  = 19,
    parameter string       PROGRAMFILE = ""
) (
    input  logic        i_clk  ,
    input  logic [31:0] i_addr ,
    output logic [31:0] o_data
);

// Insert your code here.
`ifdef IMEM_4B
  localparam int unsigned N_WORDS = (2 ** ADDR_WIDTH) / 4;
  logic [31:0] mem [0:N_WORDS-1];

  assign o_data = mem[i_addr[ADDR_WIDTH-1:2]];
`else
  logic [7:0] mem [0:2**ADDR_WIDTH-1];
  logic [ADDR_WIDTH-1:2] word_index;
  assign word_index = i_addr[ADDR_WIDTH-1:2];
`endif

  initial begin
    $readmemh(PROGRAMFILE, mem);
  end

`ifndef IMEM_4B
  assign o_data = {mem[{word_index, 2'b11}],
                   mem[{word_index, 2'b10}],
                   mem[{word_index, 2'b01}],
                   mem[{word_index, 2'b00}]};
`endif

  logic unused_ok;
  assign unused_ok = &{1'b0, i_clk, START_ADDR};

endmodule


