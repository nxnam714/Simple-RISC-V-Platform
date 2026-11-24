task automatic tsk_clk_gen(ref logic clk, input int clock_duration);
    begin
        #0 clk = 1'b0;
        forever #(real'(clock_duration) / 2.0) clk = ~clk;
    end
endtask

task automatic tsk_rstn_gen(ref logic rstn, input int reset_duration);
    begin
        #0 rstn = 1'b0;
        #reset_duration rstn = 1'b1;
    end
endtask

task automatic tsk_timeout(input longint unsigned timeout);
    begin
        #timeout;
        $display("\n[TIMEOUT] simulation stopped after %0t\n", $time);
        $finish();
    end
endtask
