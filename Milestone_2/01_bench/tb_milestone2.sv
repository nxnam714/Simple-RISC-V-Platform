`timescale 1ns / 1ps
`include "bench_defines.svh"

module tb_milestone2;

`include "tlib.svh"

logic        clk          ;
logic        rstn         ;
logic        print_valid  ;
logic [ 7:0] print_char   ;
logic        isa_valid    ;
logic [31:0] isa_data     ;
logic        halt         ;
logic        halt_latched ;
logic        db_valid     ;
logic        summary_seen ;
logic        is_result    ;

string       print_line   ;

int          cycle_count  ;
int          instr_count  ;
int          case_pass    ;
int          case_fail    ;
int          case_total   ;
int          test_pass    ;
int          test_fail    ;
int          group_pass   ;
int          group_fail   ;

localparam logic [6:0] EVENT_GROUP   = 7'd125;
localparam logic [6:0] EVENT_TEST    = 7'd126;
localparam logic [6:0] EVENT_SUMMARY = 7'd127;
localparam logic [7:0] CHAR_NEWLINE  = 8'h0A;
localparam logic [7:0] CHAR_RETURN   = 8'h0D;
localparam int         TEST_COUNT    = 37;
localparam int         GROUP_COUNT   = 86;
localparam real        ISA_POINTS    = 6.0;

initial tsk_clk_gen(clk, `CLK_DUR);
initial tsk_rstn_gen(rstn, `RST_DUR);
initial tsk_timeout(`TIMEOUT);
initial $timeformat(-9, 0, " ns", 0);


milestone2 u_dut (
    .i_clk          (clk         ),
    .i_rstn         (rstn        ),
    .o_print_valid  (print_valid ),
    .o_print_char   (print_char  ),
    .o_isa_valid    (isa_valid   ),
    .o_isa_data     (isa_data    ),
    .o_halt         (halt        ),
    .o_db_valid     (db_valid    )
);


`ifdef WAVE_SHM
initial begin
    if (`WAVE_STOP_TIME < `WAVE_START_TIME) begin
        $fatal(1, "WAVE_STOP_TIME (%0d ns) is earlier than WAVE_START_TIME (%0d ns)", `WAVE_STOP_TIME, `WAVE_START_TIME);
    end

    $shm_open(`SHM_FILE);

    if (`WAVE_START_TIME > 0) #(`WAVE_START_TIME);

    if (`WAVE_STOP_TIME > `WAVE_START_TIME) begin
        $shm_probe(tb_milestone2.u_dut.u_core, "AS");
        #(`WAVE_STOP_TIME - `WAVE_START_TIME);
    end

    $shm_close;
end
`endif

`ifdef WAVE_VCD
initial begin
    $dumpfile(`VCD_FILE);
    $dumpvars(0, tb_milestone2.u_dut.u_core);

    if (`WAVE_START_TIME > 0) #(`WAVE_START_TIME);
    $dumpon;

    if (`WAVE_STOP_TIME > `WAVE_START_TIME) begin
        #(`WAVE_STOP_TIME - `WAVE_START_TIME);
        $dumpoff;
    end
end
`endif



always_ff @(posedge clk) begin
    if (!rstn) begin
        cycle_count <= 0;
        instr_count <= 0;
    end
    else begin
        cycle_count <= cycle_count + 1;
        if (db_valid) instr_count <= instr_count + 1;
    end
end



always_ff @(posedge clk or negedge rstn) begin
    if      (!rstn)     halt_latched <= 1'b0;
    else if (halt )     halt_latched <= 1'b1;
end



always @(posedge clk) begin
    if (!rstn) begin
        print_line  = "";
        is_result   = 1'b0;
    end
    else if (print_valid) begin
        if (print_char == CHAR_NEWLINE) begin
            is_result = 1'b0;

            if (print_line.len() >= 4) begin
                if      (print_line.substr(0, 3) == "PASS")     is_result = 1'b1;
                else if (print_line.substr(0, 3) == "FAIL")     is_result = 1'b1;
            end

            if (is_result) $display("%s%0t", print_line, $time);
            else $display("%s", print_line);

            print_line = "";
        end
        else if (print_char != CHAR_RETURN) begin
            print_line = {print_line, print_char};
        end
    end
end


always_ff @(posedge clk or negedge rstn) begin
    if (!rstn) begin
        case_pass    <= 0;
        case_fail    <= 0;
        case_total   <= 0;
        test_pass    <= 0;
        test_fail    <= 0;
        group_pass   <= 0;
        group_fail   <= 0;
        summary_seen <= 1'b0;
    end
    else if (isa_valid) begin
        if (!isa_data[31] && (isa_data[30:24] == EVENT_SUMMARY)) begin
            case_total   <= {8'b0, isa_data[23:0]};
            summary_seen <= 1'b1;
        end
        else if (isa_data[30:24] == EVENT_GROUP) begin
            if (isa_data[31])       group_fail <= group_fail + 1;
            else                    group_pass <= group_pass + 1;
        end
        else if (isa_data[30:24] == EVENT_TEST) begin
            if (isa_data[31])       test_fail <= test_fail + 1;
            else                    test_pass <= test_pass + 1;
        end
        else if (isa_data[31]) begin
            case_fail <= case_fail + 1;
        end
        else begin
            case_pass <= case_pass + 1;
        end
    end
end



initial begin
    logic timed_out;
    real  ipc;
    real  isa_score;

    timed_out = 1'b0;

    @(posedge rstn);

    forever begin
        @(negedge clk);
        if (halt_latched) break;
        if (cycle_count >= `MAX_CYCLES) begin
            timed_out = 1'b1;
            break;
        end
    end

    if (timed_out) $fatal(1, "Application timed out after %0d cycles", cycle_count);

    ipc = (cycle_count > 0) ? real'(instr_count) / real'(cycle_count) : 0.0;



`ifdef ISA_TEST
    if (!summary_seen) begin
        $fatal(1, "RV32I self-check did not emit its suite trailer");
    end
    if ((case_pass + case_fail) != case_total) begin
        $fatal(1, "RV32I self-check event count mismatch: observed %0d expected %0d", case_pass + case_fail, case_total);
    end
    if ((test_pass + test_fail) != TEST_COUNT) begin
        $fatal(1, "RV32I instruction-test summary mismatch: observed %0d expected %0d", test_pass + test_fail, TEST_COUNT);
    end
    if ((group_pass + group_fail) != GROUP_COUNT) begin
        $fatal(1, "RV32I check-group summary mismatch: observed %0d expected %0d", group_pass + group_fail, GROUP_COUNT);
    end

    isa_score = ISA_POINTS * real'(group_pass) / real'(GROUP_COUNT);

    $display("============================================================");
    $display("SIMULATION SUMMARY");
    $display("============================================================");
    $display("Total tests passed   : %0d/%0d", test_pass, TEST_COUNT);
    $display("Check groups passed  : %0d/%0d", group_pass, GROUP_COUNT);
    $display("Total cases passed   : %0d/%0d", case_pass, case_total);
    $display("Total cycles         : %0d", cycle_count);
    $display("Total instructions   : %0d", instr_count);
    $display("IPC                  : %.4f", ipc);
    $display("ISA score            : %.2f/%.2f", isa_score, ISA_POINTS);
    $display("============================================================");
    $display("");

    if (case_fail != 0) $fatal(1, "RV32I self-check reported failures");
`else
    $display("");
    $display("PERFORMANCE SUMMARY");
    $display("Total cycles         : %0d", cycle_count);
    $display("Total instructions   : %0d", instr_count);
    $display("IPC                  : %.4f", ipc);
    $display("");
`endif

    $finish;
end

endmodule
