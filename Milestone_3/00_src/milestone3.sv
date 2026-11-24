`ifndef IMEM_START_ADDR
    `define IMEM_START_ADDR 32'h8000_0000
`endif

`ifndef IMEM_ADDR_WIDTH
    `define IMEM_ADDR_WIDTH 16
`endif

`ifndef DMEM_START_ADDR
    `define DMEM_START_ADDR 32'h8400_0000
`endif

`ifndef DMEM_ADDR_WIDTH
    `define DMEM_ADDR_WIDTH 14
`endif

`ifndef PROGRAMFILE
    `define PROGRAMFILE ""
`endif

module milestone3 (
    input  logic        i_clk         ,
    input  logic        i_rstn        ,
    // Switch & Button
    input  logic [17:0] i_sw          ,
    input  logic [ 3:0] i_key         ,
    // LED
    output logic [31:0] o_ledr        ,
    output logic [31:0] o_ledg        ,
    output logic [31:0] o_ledb        ,
    // HEX
    output logic [ 6:0] o_hex0        ,
    output logic [ 6:0] o_hex1        ,
    output logic [ 6:0] o_hex2        ,
    output logic [ 6:0] o_hex3        ,
    output logic [ 6:0] o_hex4        ,
    output logic [ 6:0] o_hex5        ,
    output logic [ 6:0] o_hex6        ,
    output logic [ 6:0] o_hex7        ,
    // GPIO
    input  logic [31:0] i_gpio_in     ,
    output logic [31:0] o_gpio_out    ,
    output logic [31:0] o_gpio_dir    ,
    // Debug Interface
    output logic        o_event_valid ,
    output logic [31:0] o_event_data  ,
    output logic        o_print_valid ,
    output logic [ 7:0] o_print_char  ,
    output logic        o_halt        ,
    output logic        o_db_valid
);

  // ---------------------------------------------------------------------------
  // Processor <-> IMEM
  // ---------------------------------------------------------------------------
  logic [31:0] core_imem_addr;
  logic [31:0] imem_core_rdata;

  // Processor load/store port <-> bus
  logic [31:0] core_bus_addr;
  logic [31:0] core_bus_wdata;
  logic [ 3:0] core_bus_bmsk;
  logic        core_bus_wren;
  logic [31:0] bus_core_rdata;

  // Bus <-> devices
  logic        dmem_wren, led_wren, hex_wren, timer_wren, gpio_wren, test_wren;
  logic [31:0] dmem_rdata, led_rdata, hex_rdata, sw_rdata, key_rdata, timer_rdata, gpio_rdata;
  logic [11:0] dev_addr;

  assign dev_addr = core_bus_addr[11:0];

  // ---------------------------------------------------------------------------
  // Milestone 2 single-cycle processor (unchanged)
  // ---------------------------------------------------------------------------
  singlecycle u_core (
    .i_clk        (i_clk          ),
    .i_rstn       (i_rstn         ),
    .o_db_valid   (o_db_valid     ),
    .o_imem_addr  (core_imem_addr ),
    .i_imem_rdata (imem_core_rdata),
    .o_dmem_addr  (core_bus_addr  ),
    .o_dmem_wdata (core_bus_wdata ),
    .o_dmem_bmsk  (core_bus_bmsk  ),
    .o_dmem_wren  (core_bus_wren  ),
    .i_dmem_rdata (bus_core_rdata )
  );

  imem #(
    .START_ADDR  (`IMEM_START_ADDR),
    .ADDR_WIDTH  (`IMEM_ADDR_WIDTH),
    .PROGRAMFILE (`PROGRAMFILE    )
  ) u_imem (
    .i_clk  (i_clk          ),
    .i_addr (core_imem_addr ),
    .o_data (imem_core_rdata)
  );

  // ---------------------------------------------------------------------------
  // Simple bus
  // ---------------------------------------------------------------------------
  mcu_bus u_bus (
    .i_core_addr   (core_bus_addr ),
    .i_core_wren   (core_bus_wren ),
    .o_core_rdata  (bus_core_rdata),
    .o_dmem_wren   (dmem_wren     ),
    .o_led_wren    (led_wren      ),
    .o_hex_wren    (hex_wren      ),
    .o_timer_wren  (timer_wren    ),
    .o_gpio_wren   (gpio_wren     ),
    .o_test_wren   (test_wren     ),
    .i_dmem_rdata  (dmem_rdata    ),
    .i_led_rdata   (led_rdata     ),
    .i_hex_rdata   (hex_rdata     ),
    .i_sw_rdata    (sw_rdata      ),
    .i_key_rdata   (key_rdata     ),
    .i_timer_rdata (timer_rdata   ),
    .i_gpio_rdata  (gpio_rdata    )
  );

  // ---------------------------------------------------------------------------
  // Devices
  // ---------------------------------------------------------------------------
  dmem #(
    .START_ADDR (`DMEM_START_ADDR),
    .ADDR_WIDTH (`DMEM_ADDR_WIDTH)
  ) u_dmem (
    .i_clk   (i_clk         ),
    .i_addr  (core_bus_addr ),
    .i_wdata (core_bus_wdata),
    .i_bmsk  (core_bus_bmsk ),
    .i_wren  (dmem_wren     ),
    .o_rdata (dmem_rdata    )
  );

  led_dev u_led (
    .i_clk   (i_clk         ),
    .i_rstn  (i_rstn        ),
    .i_addr  (dev_addr      ),
    .i_wdata (core_bus_wdata),
    .i_bmsk  (core_bus_bmsk ),
    .i_wren  (led_wren      ),
    .o_rdata (led_rdata     ),
    .o_ledr  (o_ledr        ),
    .o_ledg  (o_ledg        ),
    .o_ledb  (o_ledb        )
  );

  hex_dev u_hex (
    .i_clk   (i_clk         ),
    .i_rstn  (i_rstn        ),
    .i_addr  (dev_addr      ),
    .i_wdata (core_bus_wdata),
    .i_bmsk  (core_bus_bmsk ),
    .i_wren  (hex_wren      ),
    .o_rdata (hex_rdata     ),
    .o_hex0  (o_hex0        ),
    .o_hex1  (o_hex1        ),
    .o_hex2  (o_hex2        ),
    .o_hex3  (o_hex3        ),
    .o_hex4  (o_hex4        ),
    .o_hex5  (o_hex5        ),
    .o_hex6  (o_hex6        ),
    .o_hex7  (o_hex7        )
  );

  // Switches: bit = 1 when the switch is on.
  input_dev u_sw (
    .i_addr  (dev_addr          ),
    .i_value ({14'h0, i_sw}     ),
    .o_rdata (sw_rdata          )
  );

  // Buttons: pins are active low, register reads 1 while pressed.
  input_dev u_key (
    .i_addr  (dev_addr          ),
    .i_value ({28'h0, ~i_key}   ),
    .o_rdata (key_rdata         )
  );

  timer_dev u_timer (
    .i_clk   (i_clk         ),
    .i_rstn  (i_rstn        ),
    .i_addr  (dev_addr      ),
    .i_wdata (core_bus_wdata),
    .i_bmsk  (core_bus_bmsk ),
    .i_wren  (timer_wren    ),
    .o_rdata (timer_rdata   )
  );

  gpio_dev u_gpio (
    .i_clk      (i_clk         ),
    .i_rstn     (i_rstn        ),
    .i_addr     (dev_addr      ),
    .i_wdata    (core_bus_wdata),
    .i_bmsk     (core_bus_bmsk ),
    .i_wren     (gpio_wren     ),
    .o_rdata    (gpio_rdata    ),
    .i_gpio_in  (i_gpio_in     ),
    .o_gpio_out (o_gpio_out    ),
    .o_gpio_dir (o_gpio_dir    )
  );

  // Testbench device: combinational outputs, never registered.
  tb_device u_tb_device (
    .i_addr        (dev_addr      ),
    .i_wdata       (core_bus_wdata),
    .i_wren        (test_wren     ),
    .o_event_valid (o_event_valid ),
    .o_event_data  (o_event_data  ),
    .o_print_valid (o_print_valid ),
    .o_print_char  (o_print_char  ),
    .o_halt        (o_halt        )
  );

endmodule
