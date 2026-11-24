// GPIO device (base 0x4000_5000)
//   +0x0 GPIO_OUT R/W  value driven on output pins
//   +0x4 GPIO_DIR R/W  1 = output, 0 = input (reset: all inputs)
//   +0x8 GPIO_IN  R    current pin level
module gpio_dev (
    input  logic        i_clk      ,
    input  logic        i_rstn     ,
    input  logic [11:0] i_addr     ,
    input  logic [31:0] i_wdata    ,
    input  logic [ 3:0] i_bmsk     ,
    input  logic        i_wren     ,
    output logic [31:0] o_rdata    ,
    input  logic [31:0] i_gpio_in  ,
    output logic [31:0] o_gpio_out ,
    output logic [31:0] o_gpio_dir
);

  logic        sel_out, sel_dir, sel_in;
  logic [31:0] gpio_out, gpio_dir, gpio_level;

  assign sel_out = (i_addr[11:2] == 10'h000);
  assign sel_dir = (i_addr[11:2] == 10'h001);
  assign sel_in  = (i_addr[11:2] == 10'h002);

  reg32_bm u_out (.i_clk(i_clk), .i_rstn(i_rstn), .i_we(i_wren & sel_out), .i_bmsk(i_bmsk), .i_wdata(i_wdata), .o_q(gpio_out));
  reg32_bm u_dir (.i_clk(i_clk), .i_rstn(i_rstn), .i_we(i_wren & sel_dir), .i_bmsk(i_bmsk), .i_wdata(i_wdata), .o_q(gpio_dir));

  assign gpio_level = (gpio_dir & gpio_out) | (~gpio_dir & i_gpio_in);

  assign o_rdata = ({32{sel_out}} & gpio_out)
                 | ({32{sel_dir}} & gpio_dir)
                 | ({32{sel_in}}  & gpio_level);

  assign o_gpio_out = gpio_out;
  assign o_gpio_dir = gpio_dir;

endmodule
