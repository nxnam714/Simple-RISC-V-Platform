module hps_mcu_mailbox (
    input  logic        i_hps_clk          ,
    input  logic        i_hps_reset_n      ,
    input  logic [ 4:0] i_hps_address      ,
    input  logic        i_hps_read         ,
    input  logic        i_hps_write        ,
    input  logic [31:0] i_hps_writedata    ,
    input  logic [ 3:0] i_hps_byteenable   ,
    output logic [31:0] o_hps_readdata     ,
    output logic        o_hps_waitrequest  ,
    input  logic        i_mcu_clk          ,
    input  logic        i_mcu_reset_n      ,
    input  logic [11:0] i_mcu_addr         ,
    input  logic [31:0] i_mcu_wdata        ,
    input  logic [ 3:0] i_mcu_bmsk         ,
    input  logic        i_mcu_wren         ,
    output logic [31:0] o_mcu_rdata
);

  localparam logic [4:0] REG_REQ_A      = 5'h00;
  localparam logic [4:0] REG_REQ_B      = 5'h04;
  localparam logic [4:0] REG_REQ_OP     = 5'h08;
  localparam logic [4:0] REG_REQ_SEQ    = 5'h0c;
  localparam logic [4:0] REG_RES_VALUE  = 5'h10;
  localparam logic [4:0] REG_RES_STATUS = 5'h14;
  localparam logic [4:0] REG_RES_SEQ    = 5'h18;

  logic [31:0] req_a_hps, req_b_hps, req_op_hps, req_seq_hps;
  logic [31:0] res_value_mcu, res_status_mcu, res_seq_mcu;

  (* ASYNC_REG = "TRUE" *) logic [31:0] req_a_meta, req_a_sync;
  (* ASYNC_REG = "TRUE" *) logic [31:0] req_b_meta, req_b_sync;
  (* ASYNC_REG = "TRUE" *) logic [31:0] req_op_meta, req_op_sync;
  (* ASYNC_REG = "TRUE" *) logic [31:0] req_seq_meta, req_seq_sync;
  (* ASYNC_REG = "TRUE" *) logic [31:0] res_value_meta, res_value_sync;
  (* ASYNC_REG = "TRUE" *) logic [31:0] res_status_meta, res_status_sync;
  (* ASYNC_REG = "TRUE" *) logic [31:0] res_seq_meta, res_seq_sync;

  logic hps_idle, mcu_busy;
  logic unused_ok;

  function automatic logic [31:0] merge_bytes(
      input logic [31:0] old_value,
      input logic [31:0] new_value,
      input logic [ 3:0] byte_mask
  );
    logic [31:0] mask;
    begin
      mask = {{8{byte_mask[3]}}, {8{byte_mask[2]}},
              {8{byte_mask[1]}}, {8{byte_mask[0]}}};
      merge_bytes = (old_value & ~mask) | (new_value & mask);
    end
  endfunction

  assign hps_idle          = (req_seq_hps == res_seq_sync);
  assign mcu_busy          = (req_seq_sync != res_seq_mcu);
  assign o_hps_waitrequest = 1'b0;
  assign unused_ok         = &{1'b0, i_hps_address[1:0], i_mcu_addr[11:5], i_mcu_addr[1:0]};

  always_ff @(posedge i_hps_clk or negedge i_hps_reset_n) begin
    if (!i_hps_reset_n) begin
      req_a_hps      <= 32'b0;
      req_b_hps      <= 32'b0;
      req_op_hps     <= 32'b0;
      req_seq_hps    <= 32'b0;
      res_value_meta <= 32'b0;
      res_value_sync <= 32'b0;
      res_status_meta <= 32'b0;
      res_status_sync <= 32'b0;
      res_seq_meta   <= 32'b0;
      res_seq_sync   <= 32'b0;
    end
    else begin
      res_value_meta  <= res_value_mcu;
      res_value_sync  <= res_value_meta;
      res_status_meta <= res_status_mcu;
      res_status_sync <= res_status_meta;
      res_seq_meta    <= res_seq_mcu;
      res_seq_sync    <= res_seq_meta;

      if (i_hps_write && hps_idle) begin
        case ({i_hps_address[4:2], 2'b00})
          REG_REQ_A:  req_a_hps  <= merge_bytes(req_a_hps, i_hps_writedata, i_hps_byteenable);
          REG_REQ_B:  req_b_hps  <= merge_bytes(req_b_hps, i_hps_writedata, i_hps_byteenable);
          REG_REQ_OP: req_op_hps <= merge_bytes(req_op_hps, i_hps_writedata, i_hps_byteenable);
          REG_REQ_SEQ: begin
            if (i_hps_byteenable == 4'hf) req_seq_hps <= i_hps_writedata;
          end
          default: ;
        endcase
      end
    end
  end

  always_comb begin
    o_hps_readdata = 32'b0;
    if (i_hps_read) begin
      case ({i_hps_address[4:2], 2'b00})
        REG_REQ_A:      o_hps_readdata = req_a_hps;
        REG_REQ_B:      o_hps_readdata = req_b_hps;
        REG_REQ_OP:     o_hps_readdata = req_op_hps;
        REG_REQ_SEQ:    o_hps_readdata = req_seq_hps;
        REG_RES_VALUE:  o_hps_readdata = res_value_sync;
        REG_RES_STATUS: o_hps_readdata = res_status_sync;
        REG_RES_SEQ:    o_hps_readdata = res_seq_sync;
        default: ;
      endcase
    end
  end

  always_ff @(posedge i_mcu_clk or negedge i_hps_reset_n or negedge i_mcu_reset_n) begin
    if (!i_hps_reset_n) begin
      req_a_meta  <= 32'b0;
      req_a_sync  <= 32'b0;
      req_b_meta  <= 32'b0;
      req_b_sync  <= 32'b0;
      req_op_meta <= 32'b0;
      req_op_sync <= 32'b0;
      req_seq_meta <= 32'b0;
      req_seq_sync <= 32'b0;
    end
    else if (!i_mcu_reset_n) begin
      req_a_meta   <= 32'b0;
      req_a_sync   <= 32'b0;
      req_b_meta   <= 32'b0;
      req_b_sync   <= 32'b0;
      req_op_meta  <= 32'b0;
      req_op_sync  <= 32'b0;
      req_seq_meta <= 32'b0;
      req_seq_sync <= 32'b0;
    end
    else begin
      req_a_meta   <= req_a_hps;
      req_a_sync   <= req_a_meta;
      req_b_meta   <= req_b_hps;
      req_b_sync   <= req_b_meta;
      req_op_meta  <= req_op_hps;
      req_op_sync  <= req_op_meta;
      req_seq_meta <= req_seq_hps;
      req_seq_sync <= req_seq_meta;
    end
  end

  always_ff @(posedge i_mcu_clk or negedge i_hps_reset_n) begin
    if (!i_hps_reset_n) begin
      res_value_mcu <= 32'b0;
      res_status_mcu <= 32'b0;
      res_seq_mcu <= 32'b0;
    end
    else if (i_mcu_wren && mcu_busy) begin
      case (i_mcu_addr[4:2])
        REG_RES_VALUE[4:2]:
          res_value_mcu <= merge_bytes(res_value_mcu, i_mcu_wdata, i_mcu_bmsk);
        REG_RES_STATUS[4:2]:
          res_status_mcu <= merge_bytes(res_status_mcu, i_mcu_wdata, i_mcu_bmsk);
        REG_RES_SEQ[4:2]: begin
          if ((i_mcu_bmsk == 4'hf) && (i_mcu_wdata == req_seq_sync)) begin
            res_seq_mcu <= i_mcu_wdata;
          end
        end
        default: ;
      endcase
    end
  end

  always_comb begin
    o_mcu_rdata = 32'b0;
    case (i_mcu_addr[4:2])
      REG_REQ_A[4:2]:      o_mcu_rdata = req_a_sync;
      REG_REQ_B[4:2]:      o_mcu_rdata = req_b_sync;
      REG_REQ_OP[4:2]:     o_mcu_rdata = req_op_sync;
      REG_REQ_SEQ[4:2]:    o_mcu_rdata = req_seq_sync;
      REG_RES_VALUE[4:2]:  o_mcu_rdata = res_value_mcu;
      REG_RES_STATUS[4:2]: o_mcu_rdata = res_status_mcu;
      REG_RES_SEQ[4:2]:    o_mcu_rdata = res_seq_mcu;
      default: ;
    endcase
  end

endmodule
