`ifndef IMEM_START_ADDR
    `define IMEM_START_ADDR 32'h8000_0000
`endif

`ifndef IMEM_ADDR_WIDTH
    `define IMEM_ADDR_WIDTH 19
`endif

`ifndef DMEM_START_ADDR
    `define DMEM_START_ADDR 32'h8008_0000
`endif

`ifndef DMEM_ADDR_WIDTH
    `define DMEM_ADDR_WIDTH 16
`endif

module milestone2 (
    input  logic        i_clk         ,
    input  logic        i_rstn        ,
    output logic        o_print_valid ,
    output logic [ 7:0] o_print_char  ,
    output logic        o_isa_valid   ,
    output logic [31:0] o_isa_data    ,
    output logic        o_halt        ,
    output logic        o_db_valid
);

logic [31:0] core_imem_addr  ;
logic [31:0] imem_core_rdata ;
logic [31:0] core_dmem_addr  ;
logic [31:0] core_dmem_wdata ;
logic [ 3:0] core_dmem_bmsk  ;
logic        core_dmem_wren  ;
logic [31:0] dmem_core_rdata ;
logic        core_db_valid   ;

singlecycle u_core (
    .i_clk        (i_clk            ),
    .i_rstn       (i_rstn           ),
    .o_db_valid   (core_db_valid    ),
    .o_imem_addr  (core_imem_addr   ),
    .i_imem_rdata (imem_core_rdata  ),
    .o_dmem_addr  (core_dmem_addr   ),
    .o_dmem_wdata (core_dmem_wdata  ),
    .o_dmem_bmsk  (core_dmem_bmsk   ),
    .o_dmem_wren  (core_dmem_wren   ),
    .i_dmem_rdata (dmem_core_rdata  )
);

imem #(
    .START_ADDR  (`IMEM_START_ADDR),
    .ADDR_WIDTH  (`IMEM_ADDR_WIDTH),
    .PROGRAMFILE (`PROGRAMFILE    )
) u_imem (
    .i_clk  (i_clk              ),
    .i_addr (core_imem_addr     ),
    .o_data (imem_core_rdata    )
);

dmem #(
    .START_ADDR (`DMEM_START_ADDR),
    .ADDR_WIDTH (`DMEM_ADDR_WIDTH)
) u_dmem (
    .i_clk   (i_clk             ),
    .i_addr  (core_dmem_addr    ),
    .i_wdata (core_dmem_wdata   ),
    .i_bmsk  (core_dmem_bmsk    ),
    .i_wren  (core_dmem_wren    ),
    .o_rdata (dmem_core_rdata   )
);

assign o_print_valid  = core_dmem_wren && core_dmem_bmsk[0] && (core_dmem_addr == (`DMEM_START_ADDR + 32'h0000_FFF0));
assign o_print_char   = core_dmem_wdata[7:0];
assign o_isa_valid    = core_dmem_wren && (core_dmem_bmsk == 4'b1111) && (core_dmem_addr == (`DMEM_START_ADDR + 32'h0000_FFE0));
assign o_isa_data     = core_dmem_wdata;
assign o_halt         = core_dmem_wren && core_dmem_bmsk[0] && (core_dmem_addr == (`DMEM_START_ADDR + 32'h0000_FFF4));
assign o_db_valid     = core_db_valid;

endmodule

