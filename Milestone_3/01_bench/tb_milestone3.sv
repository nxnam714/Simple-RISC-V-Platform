`timescale 1ns / 1ps
`include "bench_defines.svh"

module tb_milestone3;

`include "tlib.svh"

localparam logic [6:0] EVENT_CASE        = 7'h01 ;
localparam logic [6:0] EVENT_SET_SW      = 7'h60 ;
localparam logic [6:0] EVENT_SET_KEY     = 7'h61 ;
localparam logic [6:0] EVENT_CHECK_LED   = 7'h62 ;
localparam logic [6:0] EVENT_CHECK_HEX   = 7'h63 ;
localparam logic [6:0] EVENT_SET_GPIO    = 7'h65 ;
localparam logic [6:0] EVENT_CHECK_GPIO  = 7'h66 ;
localparam logic [6:0] EVENT_GROUP_BEGIN = 7'h7C ;
localparam logic [6:0] EVENT_GROUP       = 7'h7D ;
localparam logic [6:0] EVENT_SUMMARY     = 7'h7F ;
localparam int         GROUP_COUNT       = `GROUP_COUNT ;

logic        clk                ;
logic        rstn               ;
logic [ 9:0] tb_dut_sw          ;
logic [ 3:0] tb_dut_key         ;
logic [31:0] dut_tb_led [3]     ;
logic [ 6:0] dut_tb_hex [8]     ;
logic [31:0] tb_dut_gpio_in     ;
logic [31:0] dut_tb_gpio_out    ;
logic [31:0] dut_tb_gpio_dir    ;
logic [31:0] tb_gpio_drive      ;
wire  [15:0] dut_tb_dram_dq     ;
logic        dut_tb_event_valid ;
logic [31:0] dut_tb_event_data  ;
logic        dut_tb_print_valid ;
logic [ 7:0] dut_tb_print_char  ;
logic        dut_tb_halt        ;
logic        dut_tb_db_valid    ;

logic        event_fail         ;
logic [ 6:0] event_code         ;
logic [23:0] event_payload      ;
logic [ 7:0] current_group      ;
logic [15:0] observed_half      ;
logic [31:0] observed_word      ;
logic [31:0] observed_mask      ;
logic        halt_latched       ;

int          report_fd          ;
string       print_line         ;
longint      cycle_count        ;
longint      instr_count        ;
int          case_pass          ;
int          case_fail          ;
int          case_total         ;
int          pin_pass           ;
int          pin_fail           ;
int          groups_reported    ;
int          console_pass_lines ;
int          console_fail_lines ;
logic        summary_seen       ;
logic        group_case_fail    ;
logic        row_pin_fail       ;
logic        row_verdict_fail   ;
logic        group_program_pass [256] ;
logic        group_seen         [256] ;
logic        group_pin_fail     [256] ;

initial tsk_clk_gen(clk, `CLK_DUR)   ;
initial tsk_rstn_gen(rstn, `RST_DUR) ;
initial tsk_timeout(`TIMEOUT)        ;
initial $timeformat(-9, 0, " ns", 0) ;

milestone3 u_dut (
    .i_clk         (clk                ),
    .i_rstn        (rstn               ),
    .i_sw          ({8'd0, tb_dut_sw}  ),
    .i_key         (tb_dut_key         ),
    .o_ledr        (dut_tb_led[0]      ),
    .o_ledg        (dut_tb_led[1]      ),
    .o_ledb        (dut_tb_led[2]      ),
    .o_hex0        (dut_tb_hex[0]      ),
    .o_hex1        (dut_tb_hex[1]      ),
    .o_hex2        (dut_tb_hex[2]      ),
    .o_hex3        (dut_tb_hex[3]      ),
    .o_hex4        (dut_tb_hex[4]      ),
    .o_hex5        (dut_tb_hex[5]      ),
    .o_hex6        (dut_tb_hex[6]      ),
    .o_hex7        (dut_tb_hex[7]      ),
    .i_gpio_in     (tb_dut_gpio_in     ),
    .o_gpio_out    (dut_tb_gpio_out    ),
    .o_gpio_dir    (dut_tb_gpio_dir    ),
    .i_hps_clk         (clk             ),
    .i_hps_reset_n     (rstn            ),
    .i_hps_address     (5'b0            ),
    .i_hps_read        (1'b0            ),
    .i_hps_write       (1'b0            ),
    .i_hps_writedata   (32'b0           ),
    .i_hps_byteenable  (4'b0            ),
    .o_hps_readdata    (                ),
    .o_hps_waitrequest (                ),
    .o_dram_addr   (                   ),
    .o_dram_ba     (                   ),
    .o_dram_ldqm   (                   ),
    .o_dram_udqm   (                   ),
    .o_dram_ras_n  (                   ),
    .o_dram_cas_n  (                   ),
    .o_dram_cke    (                   ),
    .o_dram_clk    (                   ),
    .o_dram_we_n   (                   ),
    .o_dram_cs_n   (                   ),
    .io_dram_dq    (dut_tb_dram_dq     ),
    .o_event_valid (dut_tb_event_valid ),
    .o_event_data  (dut_tb_event_data  ),
    .o_print_valid (dut_tb_print_valid ),
    .o_print_char  (dut_tb_print_char  ),
    .o_halt        (dut_tb_halt        ),
    .o_db_valid    (dut_tb_db_valid    )
);

task automatic tsk_report(input string text);
    $display("%s", text) ;
    if (report_fd != 0) $fdisplay(report_fd, "%s", text) ;
endtask

initial begin
    report_fd = $fopen(`REPORT_FILE, "w") ;
    if (report_fd == 0) $display("Cannot open report file = %s", `REPORT_FILE) ;
end

`ifdef WAVE_SHM
initial begin
    if (`WAVE_STOP_TIME < `WAVE_START_TIME) $fatal(1, "WAVE_STOP_TIME = %0d is earlier than WAVE_START_TIME = %0d", `WAVE_STOP_TIME, `WAVE_START_TIME) ;
    $shm_open(`SHM_FILE) ;
    if (`WAVE_START_TIME > 0) #(`WAVE_START_TIME) ;
    $shm_probe(tb_milestone3, "AS") ;
    #(`WAVE_STOP_TIME - `WAVE_START_TIME) ;
    $shm_close ;
end
`endif

assign event_fail    = dut_tb_event_data[31]    ;
assign event_code    = dut_tb_event_data[30:24] ;
assign event_payload = dut_tb_event_data[23:0]  ;

assign tb_dut_gpio_in = (dut_tb_gpio_dir& dut_tb_gpio_out) | (~dut_tb_gpio_dir& tb_gpio_drive) ;

always_ff @(posedge clk) begin
    if (!rstn) begin
        cycle_count  <= 0 ;
        instr_count  <= 0 ;
    end
    else begin
        cycle_count <= cycle_count + 1 ;
        if (dut_tb_db_valid) instr_count <= instr_count + 1 ;
    end
end

always_ff @(posedge clk or negedge rstn) begin
    if      (!rstn) halt_latched <= 1'b0 ;
    else if (dut_tb_halt ) halt_latched <= 1'b1 ;
end

initial begin
    tb_dut_sw      = 10'd0 ;
    tb_dut_key     = 4'hF  ;
    tb_gpio_drive  = 32'd0 ;
end

always @(posedge clk) begin
    if (rstn && dut_tb_event_valid) begin
        if (event_code == EVENT_SET_SW )        tb_dut_sw  <= event_payload[9:0]  ;
        if (event_code == EVENT_SET_KEY)        tb_dut_key <= ~event_payload[3:0] ;

        if (event_code == EVENT_SET_GPIO) begin
            if (event_payload[16])              tb_gpio_drive[31:16] <= event_payload[15:0] ;
            else                                tb_gpio_drive[15: 0] <= event_payload[15:0] ;
        end
    end
end

initial begin
    case_pass       = 0    ;
    case_fail       = 0    ;
    case_total      = 0    ;
    pin_pass        = 0    ;
    pin_fail        = 0    ;
    groups_reported = 0    ;
    summary_seen    = 1'b0 ;
    group_case_fail = 1'b0 ;
    row_pin_fail    = 1'b0 ;
    current_group   = 8'd0 ;
    for (int index = 0; index < 256; index++) begin
        group_program_pass[index] = 1'b0 ;
        group_seen[index]         = 1'b0 ;
        group_pin_fail[index]     = 1'b0 ;
    end
end

always @(posedge clk) begin
    if (rstn && dut_tb_event_valid) begin
        case (event_code)
            EVENT_CASE: begin
                if (event_fail) begin
                    case_fail++ ;
                    group_case_fail = 1'b1 ;
                end
                else            case_pass++ ;
            end

            EVENT_GROUP_BEGIN: current_group = event_payload[7:0] ;

            EVENT_CHECK_LED: begin
                observed_word = (event_payload[18:17] <  3) ? dut_tb_led[event_payload[18:17]] : 32'hx ;
                observed_mask = (event_payload[18:17] == 0) ? 32'h0003_FFFF : (event_payload[18:17] == 1) ? 32'h0000_00FF : 32'h0000_0001 ;
                observed_mask =  event_payload[16] ? (observed_mask >> 16)  : observed_mask ;
                observed_half = (event_payload[16] ? (observed_word[31:16]) : observed_word[15:0]) & observed_mask[15:0] ;
                if (observed_half === (event_payload[15:0] & observed_mask[15:0])) pin_pass++ ;
                else begin
                    pin_fail++ ;
                    group_pin_fail[current_group] = 1'b1 ;
                    row_pin_fail = 1'b1 ;
                    tsk_report($sformatf("ERROR  LED%s pins [%0d:%0d] at %0t: expected = 0x%04h, observed = 0x%04h",
                        (event_payload[18:17] == 0) ? "R" : (event_payload[18:17] == 1) ? "G" : "B",
                        event_payload[16] ? 31 : 15, event_payload[16] ? 16 : 0, $time, event_payload[15:0], observed_half)) ;
                end
            end

            EVENT_CHECK_HEX: begin
                if (dut_tb_hex[event_payload[10:8]] === ~event_payload[6:0]) pin_pass++ ;
                else begin
                    pin_fail++ ;
                    group_pin_fail[current_group] = 1'b1 ;
                    row_pin_fail = 1'b1 ;
                    tsk_report($sformatf("ERROR  HEX%0d pins at %0t: expected = 0x%02h, observed = 0x%02h", event_payload[10:8], $time, ~event_payload[6:0], dut_tb_hex[event_payload[10:8]])) ;
                end
            end

            EVENT_CHECK_GPIO: begin
                observed_word = (event_payload[17]) ? dut_tb_gpio_dir: dut_tb_gpio_out ;
                observed_half = (event_payload[16]) ? observed_word[31:16] : observed_word[15:0] ;
                if (observed_half === event_payload[15:0]) pin_pass++ ;
                else begin
                    pin_fail++ ;
                    group_pin_fail[current_group] = 1'b1 ;
                    row_pin_fail = 1'b1 ;
                    tsk_report($sformatf("ERROR  %s [%0d:%0d] at %0t: expected = 0x%04h, observed = 0x%04h",
                        event_payload[17] ? "o_gpio_dir" : "o_gpio_out", event_payload[16] ? 31 : 15, event_payload[16] ? 16 : 0,
                        $time, event_payload[15:0], observed_half)) ;
                end
            end

            EVENT_GROUP: begin
                groups_reported++ ;
                group_seen[event_payload[7:0]]         = 1'b1        ;
                group_program_pass[event_payload[7:0]] = !event_fail && !group_case_fail ;
                group_case_fail = 1'b0 ;
            end

            EVENT_SUMMARY: begin
                case_total   = event_payload ;
                summary_seen = 1'b1          ;
            end

            default: ;
        endcase
    end
end

function automatic string fn_row(input string text);
    string stamp ;
    string line  ;
    if (text.len() >= 72) return $sformatf("%s    %0t", text, $time) ;
    stamp = $sformatf("%0t", $time) ;
    line  = text ;
    while ((line.len() + stamp.len()) < 84) line = {line, " "} ;
    return {line, stamp} ;
endfunction

function automatic bit fn_contains(input string text, input string word);
    for (int index = 0; index + word.len() <= text.len(); index++) begin
        if (text.substr(index, index + word.len() - 1) == word) return 1'b1 ;
    end
    return 1'b0 ;
endfunction

initial begin
    print_line         = "" ;
    console_pass_lines = 0  ;
    console_fail_lines = 0  ;
end

always @(posedge clk) begin
    if (rstn && dut_tb_print_valid) begin
        if (dut_tb_print_char == 8'h0A) begin
            if ((print_line.len() >= 3) && (print_line[0] == "@")) begin
                row_verdict_fail = (print_line[1] == "F") || ((print_line[1] == "G") ? group_pin_fail[current_group] : row_pin_fail) ;
                print_line       = {row_verdict_fail ? "FAIL    " : "PASS    ", print_line.substr(3, print_line.len() - 1)} ;
                row_pin_fail     = 1'b0 ;
            end

            if      ((print_line.len() >= 4) && (print_line.substr(0, 3) == "PASS"))    tsk_report(fn_row(print_line)) ;
            else if ((print_line.len() >= 4) && (print_line.substr(0, 3) == "FAIL"))    tsk_report(fn_row(print_line)) ;
            else                                                                        tsk_report(print_line) ;

            if (fn_contains(print_line, "PASS")) console_pass_lines++ ;
            if (fn_contains(print_line, "FAIL")) console_fail_lines++ ;
            print_line = "" ;
        end
        else if (dut_tb_print_char != 8'h0D) begin
            print_line = {print_line, string'(dut_tb_print_char)} ;
        end
    end
end

initial begin
    automatic logic timed_out     = 1'b0 ;
    automatic int   groups_passed = 0    ;
    automatic int   failures      = 0    ;
    automatic real  ipc                  ;

    @(posedge rstn) ;
    forever begin
        @(negedge clk) ;
        if (halt_latched)  break ;
        if (cycle_count >= `MAX_CYCLES) begin
            timed_out = 1'b1 ;
            break ;
        end
    end

    ipc = (cycle_count > 0) ? real'(instr_count) / real'(cycle_count) : 0.0 ;

    tsk_report("")                                                             ;
    tsk_report("============================================================") ;
    tsk_report("SIMULATION SUMMARY")                                           ;
    tsk_report("============================================================") ;
    if (timed_out) begin
        tsk_report($sformatf("TIMEOUT after cycles = %0d", cycle_count)) ;
        tsk_report("The program never finished. Check the core, the bus, and that DMEM starts at 0x8400_0000.") ;
        if (current_group != 0) begin
            for (int index = 1; index <= GROUP_COUNT; index++) if (group_seen[index] && group_program_pass[index] && !group_pin_fail[index]) groups_passed++ ;
            tsk_report($sformatf("The program stopped inside group = %0d", current_group)) ;
            tsk_report($sformatf("Check groups passed  = %0d/%0d", groups_passed, GROUP_COUNT)) ;
        end
        failures++ ;
    end

    if (summary_seen) begin
        for (int index = 1; index <= GROUP_COUNT; index++) begin
            if      (group_seen[index] && group_program_pass[index] && !group_pin_fail[index])    groups_passed++ ;
            else if (!group_seen[index])        tsk_report($sformatf("Group %0d never reported a result", index)) ;
            else if (group_pin_fail[index])     tsk_report($sformatf("Group %0d failed a pin check", index)) ;
            else tsk_report($sformatf("Group %0d failed", index)) ;
        end
        if ((case_pass + case_fail) != case_total) begin
            tsk_report($sformatf("Case count mismatch: observed = %0d, expected = %0d", case_pass + case_fail, case_total)) ;
            failures++ ;
        end
        if (groups_reported != GROUP_COUNT) begin
            tsk_report($sformatf("Group count mismatch: observed = %0d, expected = %0d", groups_reported, GROUP_COUNT)) ;
            failures++ ;
        end
        tsk_report($sformatf("Check groups passed  = %0d/%0d", groups_passed, GROUP_COUNT)) ;
        tsk_report($sformatf("Cases passed         = %0d/%0d", case_pass, case_total))      ;
        tsk_report($sformatf("Pin checks passed    = %0d/%0d", pin_pass, pin_pass + pin_fail)) ;
        if (groups_passed != GROUP_COUNT) failures++ ;
        if (case_fail != 0) failures++ ;
    end
    else if (!timed_out) begin
        tsk_report($sformatf("Console lines with PASS = %0d, with FAIL = %0d", console_pass_lines, console_fail_lines)) ;
        if ((console_pass_lines == 0) || (console_fail_lines != 0))         failures++ ;
        if ((pin_pass + pin_fail) > 0)  tsk_report($sformatf("Pin checks passed    = %0d/%0d", pin_pass, pin_pass + pin_fail)) ;
        if (pin_fail != 0)      failures++ ;
    end

    tsk_report($sformatf("Total cycles         = %0d", cycle_count))  ;
    tsk_report($sformatf("Total instructions   = %0d", instr_count))  ;
    tsk_report($sformatf("IPC                  = %.4f", ipc))         ;
`ifdef SCORE_POINTS
    begin
        automatic int groups_scored = 0 ;
        for (int index = 1; index <= GROUP_COUNT; index++) if (group_seen[index] && group_program_pass[index] && !group_pin_fail[index]) groups_scored++ ;
        tsk_report($sformatf("SCORE                = %.2f / %.2f", (`SCORE_POINTS * groups_scored) / GROUP_COUNT, real'(`SCORE_POINTS))) ;
    end
`endif
    if (failures == 0)      tsk_report("RESULT = PASS") ;
    else                    tsk_report("RESULT = FAIL") ;

    tsk_report("============================================================") ;
    if (report_fd != 0)     $fclose(report_fd) ;
    if (failures  != 0)     $fatal(1, "Simulation failed") ;
    $finish ;
end

endmodule
