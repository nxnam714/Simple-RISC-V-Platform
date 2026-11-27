module simple_riscv (
    input  logic        CLOCK_50,
    input  logic [9:0]  SW,
    input  logic [3:0]  KEY,
    output logic [9:0]  LEDR,
    output logic [6:0]  HEX0,
    output logic [6:0]  HEX1,
    output logic [6:0]  HEX2,
    output logic [6:0]  HEX3,
    output logic [6:0]  HEX4,
    output logic [6:0]  HEX5,
    inout  wire  [35:0] GPIO,
    // External SDRAM (DE10-Standard 64MB SDRAM)
    output logic [12:0] DRAM_ADDR,
    output logic [1:0]  DRAM_BA,
    output logic        DRAM_LDQM,
    output logic        DRAM_UDQM,
    output logic        DRAM_RAS_N,
    output logic        DRAM_CAS_N,
    output logic        DRAM_CKE,
    output logic        DRAM_CLK,
    output logic        DRAM_WE_N,
    output logic        DRAM_CS_N,
    inout  wire  [15:0] DRAM_DQ
);

  logic [31:0] gpio_in;
  logic [31:0] gpio_out;
  logic [31:0] gpio_dir;
  logic [31:0] ledr_internal;
  logic [31:0] ledg_unused;
  logic [31:0] ledb_unused;
  logic [6:0]  hex6_unused;
  logic [6:0]  hex7_unused;

  genvar pin;
  generate
    for (pin = 0; pin < 32; pin = pin + 1) begin : gpio_pins
      assign GPIO[pin]    = gpio_dir[pin] ? gpio_out[pin] : 1'bz;
      assign gpio_in[pin] = GPIO[pin];
    end
  endgenerate

  // Tie unused upper GPIO pins to high-Z to eliminate Quartus warning (13040)
  assign GPIO[35:32] = 4'bz;
  assign LEDR        = ledr_internal[9:0];

  milestone3 u_mcu (
    .i_clk         (CLOCK_50),
    .i_rstn        (KEY[0]),
    .i_sw          ({8'b0, SW}),
    .i_key         (KEY),
    .o_ledr        (ledr_internal),
    .o_ledg        (ledg_unused),
    .o_ledb        (ledb_unused),
    .o_hex0        (HEX0),
    .o_hex1        (HEX1),
    .o_hex2        (HEX2),
    .o_hex3        (HEX3),
    .o_hex4        (HEX4),
    .o_hex5        (HEX5),
    .o_hex6        (hex6_unused),
    .o_hex7        (hex7_unused),
    .i_gpio_in     (gpio_in),
    .o_gpio_out    (gpio_out),
    .o_gpio_dir    (gpio_dir),
    .i_hps_clk         (CLOCK_50),
    .i_hps_reset_n     (KEY[0]),
    .i_hps_address     (5'b0),
    .i_hps_read        (1'b0),
    .i_hps_write       (1'b0),
    .i_hps_writedata   (32'b0),
    .i_hps_byteenable  (4'b0),
    .o_hps_readdata    (),
    .o_hps_waitrequest (),
    .o_dram_addr   (DRAM_ADDR),
    .o_dram_ba     (DRAM_BA),
    .o_dram_ldqm   (DRAM_LDQM),
    .o_dram_udqm   (DRAM_UDQM),
    .o_dram_ras_n  (DRAM_RAS_N),
    .o_dram_cas_n  (DRAM_CAS_N),
    .o_dram_cke    (DRAM_CKE),
    .o_dram_clk    (DRAM_CLK),
    .o_dram_we_n   (DRAM_WE_N),
    .o_dram_cs_n   (DRAM_CS_N),
    .io_dram_dq    (DRAM_DQ),
    .o_event_valid (),
    .o_event_data  (),
    .o_print_valid (),
    .o_print_char  (),
    .o_halt        (),
    .o_db_valid    ()
  );

endmodule
