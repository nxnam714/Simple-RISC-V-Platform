module tb_device (
    // Bus Interface
    input  logic [11:0] i_addr        , // Offset Address
    input  logic [31:0] i_wdata       ,
    input  logic        i_wren        ,
    // Debug Interface
    output logic        o_event_valid ,
    output logic [31:0] o_event_data  ,
    output logic        o_print_valid ,
    output logic [ 7:0] o_print_char  ,
    output logic        o_halt
);

assign o_event_valid = i_wren & (i_addr == 12'h000) ;
assign o_event_data  = i_wdata                      ;
assign o_print_valid = i_wren & (i_addr == 12'h004) ;
assign o_print_char  = i_wdata[7:0]                 ;
assign o_halt        = i_wren & (i_addr == 12'h008) ;

endmodule
