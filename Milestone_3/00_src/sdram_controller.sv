// SDRAM controller for DE10-Standard (IS42S16320D / 64MB, 16-bit data bus, 50 MHz).
// Bridges 32-bit word-aligned DMEM requests (2 x 16-bit SDRAM accesses per 32-bit word)
// with an active-high stall signal (o_stall) while the external SDRAM is initializing,
// refreshing, or executing a read/write transaction.
module sdram_controller #(
    parameter int unsigned INIT_WAIT_CYCLES = 10000, // 200 us at 50 MHz
    parameter int unsigned REFRESH_INTERVAL = 380,   // ~7.6 us at 50 MHz
    parameter int unsigned CAS_LATENCY      = 2      // CL = 2 at 50 MHz
) (
    input  logic        i_clk       ,
    input  logic        i_rstn      ,
    // Processor / Bus DMEM interface
    input  logic [31:0] i_addr      ,
    input  logic [31:0] i_wdata     ,
    input  logic [ 3:0] i_bmsk      ,
    input  logic        i_rden      ,
    input  logic        i_wren      ,
    output logic [31:0] o_rdata     ,
    output logic        o_stall     ,
    // External SDRAM pins (DE10-Standard)
    output logic [12:0] o_dram_addr ,
    output logic [ 1:0] o_dram_ba   ,
    output logic        o_dram_ldqm ,
    output logic        o_dram_udqm ,
    output logic        o_dram_ras_n,
    output logic        o_dram_cas_n,
    output logic        o_dram_cke  ,
    output logic        o_dram_clk  ,
    output logic        o_dram_we_n ,
    output logic        o_dram_cs_n ,
    inout  wire  [15:0] io_dram_dq
);

  // SDRAM commands: {CS_N, RAS_N, CAS_N, WE_N}
  localparam logic [3:0] CMD_NOP    = 4'b0111;
  localparam logic [3:0] CMD_ACTIVE = 4'b0011;
  localparam logic [3:0] CMD_READ   = 4'b0101;
  localparam logic [3:0] CMD_WRITE  = 4'b0100;
  localparam logic [3:0] CMD_PRECHG = 4'b0010;
  localparam logic [3:0] CMD_AUTORF = 4'b0001;
  localparam logic [3:0] CMD_LMR    = 4'b0000;

  typedef enum logic [4:0] {
    ST_INIT_WAIT  = 5'd0,
    ST_INIT_PRE   = 5'd1,
    ST_INIT_TRP   = 5'd2,
    ST_INIT_REF1  = 5'd3,
    ST_INIT_TRFC1 = 5'd4,
    ST_INIT_REF2  = 5'd5,
    ST_INIT_TRFC2 = 5'd6,
    ST_INIT_LMR   = 5'd7,
    ST_INIT_TMRD  = 5'd8,
    ST_IDLE       = 5'd9,
    ST_REF_CMD    = 5'd10,
    ST_REF_WAIT   = 5'd11,
    ST_ACT        = 5'd12,
    ST_TRCD       = 5'd13,
    ST_RD_LO      = 5'd14,
    ST_RD_HI      = 5'd15,
    ST_RD_WAIT1   = 5'd16,
    ST_RD_WAIT2   = 5'd17,
    ST_WR_LO      = 5'd18,
    ST_WR_HI      = 5'd19,
    ST_WR_TWR     = 5'd20,
    ST_PRE_WAIT   = 5'd21,
    ST_DONE       = 5'd22
  } state_e;

  state_e state;

  logic [15:0] wait_cnt;
  logic [15:0] ref_cnt;
  logic        ref_pending;

  // Latched request
  logic [31:0] req_addr;
  logic [31:0] req_wdata;
  logic [ 3:0] req_bmsk;
  logic        req_wren;

  // Address mapping (word address = req_addr[25:2]):
  //   Bank : req_addr[25:24] (2 bits)
  //   Row  : req_addr[23:11] (13 bits)
  //   Col  : {req_addr[10:2], half_sel} (10 bits)
  logic [ 1:0] addr_ba;
  logic [12:0] addr_row;
  logic [ 8:0] addr_col_word;

  assign addr_ba       = req_addr[25:24];
  assign addr_row      = req_addr[23:11];
  assign addr_col_word = req_addr[10:2];

  // SDRAM command & outputs registered for clean I/O timing
  logic [ 3:0] cmd_reg;
  logic [12:0] dram_addr_reg;
  logic [ 1:0] dram_ba_reg;
  logic        dram_ldqm_reg;
  logic        dram_udqm_reg;
  logic [15:0] dram_dq_out;
  logic        dram_dq_oe;

  // Drive SDRAM clock 180-deg out of phase with i_clk so commands/data are center-aligned
  assign o_dram_clk   = ~i_clk;
  assign o_dram_cke   = 1'b1;
  assign o_dram_cs_n  = cmd_reg[3];
  assign o_dram_ras_n = cmd_reg[2];
  assign o_dram_cas_n = cmd_reg[1];
  assign o_dram_we_n  = cmd_reg[0];
  assign o_dram_addr  = dram_addr_reg;
  assign o_dram_ba    = dram_ba_reg;
  assign o_dram_ldqm  = dram_ldqm_reg;
  assign o_dram_udqm  = dram_udqm_reg;
  assign io_dram_dq   = dram_dq_oe ? dram_dq_out : 16'hzzzz;

  logic req_valid;
  assign req_valid = i_rden | i_wren;

  // Stall CPU whenever a request is active and we have not reached ST_DONE
  assign o_stall = req_valid & (state != ST_DONE);

  // Refresh timer
  always_ff @(posedge i_clk or negedge i_rstn) begin
    if (!i_rstn) begin
      ref_cnt     <= 16'd0;
      ref_pending <= 1'b0;
    end else begin
      if (state == ST_REF_CMD) begin
        ref_pending <= 1'b0;
      end else if (ref_cnt >= REFRESH_INTERVAL[15:0]) begin
        ref_cnt     <= 16'd0;
        ref_pending <= 1'b1;
      end else begin
        ref_cnt <= ref_cnt + 16'd1;
      end
    end
  end

  // Main FSM
  always_ff @(posedge i_clk or negedge i_rstn) begin
    if (!i_rstn) begin
      state         <= ST_INIT_WAIT;
      wait_cnt      <= 16'd0;
      cmd_reg       <= CMD_NOP;
      dram_addr_reg <= 13'd0;
      dram_ba_reg   <= 2'd0;
      dram_ldqm_reg <= 1'b1;
      dram_udqm_reg <= 1'b1;
      dram_dq_out   <= 16'd0;
      dram_dq_oe    <= 1'b0;
      req_addr      <= 32'd0;
      req_wdata     <= 32'd0;
      req_bmsk      <= 4'd0;
      req_wren      <= 1'b0;
      o_rdata       <= 32'd0;
    end else begin
      // Default command each cycle is NOP
      cmd_reg    <= CMD_NOP;
      dram_dq_oe <= 1'b0;

      case (state)
        // 1. Wait 200 us power-up delay with NOP and DQM high
        ST_INIT_WAIT: begin
          dram_ldqm_reg <= 1'b1;
          dram_udqm_reg <= 1'b1;
          if (wait_cnt >= INIT_WAIT_CYCLES[15:0]) begin
            wait_cnt <= 16'd0;
            state    <= ST_INIT_PRE;
          end else begin
            wait_cnt <= wait_cnt + 16'd1;
          end
        end

        // 2. Precharge all banks (A10 = 1)
        ST_INIT_PRE: begin
          cmd_reg       <= CMD_PRECHG;
          dram_addr_reg <= 13'b0_0100_0000_0000;
          state         <= ST_INIT_TRP;
        end

        ST_INIT_TRP: begin
          state <= ST_INIT_REF1;
        end

        // 3. Auto-refresh #1
        ST_INIT_REF1: begin
          cmd_reg  <= CMD_AUTORF;
          wait_cnt <= 16'd0;
          state    <= ST_INIT_TRFC1;
        end

        ST_INIT_TRFC1: begin
          if (wait_cnt >= 16'd3) begin
            state <= ST_INIT_REF2;
          end else begin
            wait_cnt <= wait_cnt + 16'd1;
          end
        end

        // 4. Auto-refresh #2
        ST_INIT_REF2: begin
          cmd_reg  <= CMD_AUTORF;
          wait_cnt <= 16'd0;
          state    <= ST_INIT_TRFC2;
        end

        ST_INIT_TRFC2: begin
          if (wait_cnt >= 16'd3) begin
            state <= ST_INIT_LMR;
          end else begin
            wait_cnt <= wait_cnt + 16'd1;
          end
        end

        // 5. Load Mode Register: Write Burst = Single (A9=1), OpMode=00, CAS Latency=2 (A6:4=010), Burst=Seq, Len=1
        ST_INIT_LMR: begin
          cmd_reg       <= CMD_LMR;
          dram_ba_reg   <= 2'b00;
          dram_addr_reg <= {3'b000, 1'b1, 2'b00, CAS_LATENCY[2:0], 1'b0, 3'b000};
          wait_cnt      <= 16'd0;
          state         <= ST_INIT_TMRD;
        end

        ST_INIT_TMRD: begin
          if (wait_cnt >= 16'd1) begin
            state <= ST_IDLE;
          end else begin
            wait_cnt <= wait_cnt + 16'd1;
          end
        end

        // 6. Idle: service pending refresh or start new read/write transaction
        ST_IDLE: begin
          dram_ldqm_reg <= 1'b1;
          dram_udqm_reg <= 1'b1;
          if (ref_pending) begin
            state <= ST_REF_CMD;
          end else if (req_valid) begin
            req_addr  <= i_addr;
            req_wdata <= i_wdata;
            req_bmsk  <= i_bmsk;
            req_wren  <= i_wren;
            state     <= ST_ACT;
          end
        end

        ST_REF_CMD: begin
          cmd_reg  <= CMD_AUTORF;
          wait_cnt <= 16'd0;
          state    <= ST_REF_WAIT;
        end

        ST_REF_WAIT: begin
          if (wait_cnt >= 16'd3) begin
            state <= ST_IDLE;
          end else begin
            wait_cnt <= wait_cnt + 16'd1;
          end
        end

        // 7. Activate Row
        ST_ACT: begin
          cmd_reg       <= CMD_ACTIVE;
          dram_ba_reg   <= addr_ba;
          dram_addr_reg <= addr_row;
          state         <= ST_TRCD;
        end

        // tRCD = 20ns (1 NOP cycle between ACT and RD/WR at 50 MHz)
        ST_TRCD: begin
          if (req_wren) begin
            state <= ST_WR_LO;
          end else begin
            state <= ST_RD_LO;
          end
        end

        // 8a. Read lower 16-bit halfword (col = {word_col, 1'b0}, A10 = 0 no auto-precharge)
        ST_RD_LO: begin
          cmd_reg       <= CMD_READ;
          dram_ba_reg   <= addr_ba;
          dram_addr_reg <= {3'b000, addr_col_word, 1'b0};
          dram_ldqm_reg <= 1'b0;
          dram_udqm_reg <= 1'b0;
          state         <= ST_RD_HI;
        end

        // Read upper 16-bit halfword (col = {word_col, 1'b1}, A10 = 1 auto-precharge)
        ST_RD_HI: begin
          cmd_reg       <= CMD_READ;
          dram_ba_reg   <= addr_ba;
          dram_addr_reg <= {3'b001, addr_col_word, 1'b1};
          dram_ldqm_reg <= 1'b0;
          dram_udqm_reg <= 1'b0;
          state         <= ST_RD_WAIT1;
        end

        // With registered outputs and CL=2:
        //   ST_RD_LO sets cmd_reg=READ_LO -> appears on pins during ST_RD_HI
        //   ST_RD_HI sets cmd_reg=READ_HI -> appears on pins during ST_RD_WAIT1
        //   Lower 16-bit data valid on io_dram_dq at end of ST_RD_WAIT1
        ST_RD_WAIT1: begin
          o_rdata[15:0] <= io_dram_dq;
          state         <= ST_RD_WAIT2;
        end

        //   Upper 16-bit data valid on io_dram_dq at end of ST_RD_WAIT2
        ST_RD_WAIT2: begin
          o_rdata[31:16] <= io_dram_dq;
          dram_ldqm_reg  <= 1'b1;
          dram_udqm_reg  <= 1'b1;
          state          <= ST_DONE;
        end

        // 8b. Write lower 16-bit halfword (col = {word_col, 1'b0}, A10 = 0)
        ST_WR_LO: begin
          cmd_reg       <= CMD_WRITE;
          dram_ba_reg   <= addr_ba;
          dram_addr_reg <= {3'b000, addr_col_word, 1'b0};
          dram_ldqm_reg <= ~req_bmsk[0];
          dram_udqm_reg <= ~req_bmsk[1];
          dram_dq_out   <= req_wdata[15:0];
          dram_dq_oe    <= 1'b1;
          state         <= ST_WR_HI;
        end

        // Write upper 16-bit halfword (col = {word_col, 1'b1}, A10 = 1 auto-precharge)
        ST_WR_HI: begin
          cmd_reg       <= CMD_WRITE;
          dram_ba_reg   <= addr_ba;
          dram_addr_reg <= {3'b001, addr_col_word, 1'b1};
          dram_ldqm_reg <= ~req_bmsk[2];
          dram_udqm_reg <= ~req_bmsk[3];
          dram_dq_out   <= req_wdata[31:16];
          dram_dq_oe    <= 1'b1;
          state         <= ST_WR_TWR;
        end

        // tWR + tRP recovery before next activate
        ST_WR_TWR: begin
          dram_ldqm_reg <= 1'b1;
          dram_udqm_reg <= 1'b1;
          state         <= ST_PRE_WAIT;
        end

        ST_PRE_WAIT: begin
          state <= ST_DONE;
        end

        // 9. Release o_stall for 1 cycle so CPU commits load/store and advances PC
        ST_DONE: begin
          state <= ST_IDLE;
        end

        default: state <= ST_IDLE;
      endcase
    end
  end

  logic unused_ok;
  assign unused_ok = &{1'b0, req_addr[31:26], req_addr[1:0]};

endmodule : sdram_controller
