module pcm (
  input wire sys_clk,
  input wire sys_rst,
  output wire uart_tx,
  input wire uart_rx,

  inout wire [7:0] data_bus,
  input wire [5:0] address_bus,
  input wire rd_bus,
  input wire wr_bus,
  input wire cs_bus,
  output wire gpint,
  output wire wait_bus

  // input wire [7:0] wave_data,
  // output wire [19:0] wave_addr,
  // output wire [6:0] wave_cs,
  // output wire wave_cs7_a20,
  
  // input wire [7:0] mem_data,
  // output wire [14:0] mem_addr,
  // output wire mem_we,
  // output wire mem_cs,

  // output wire [15:0] audio_out,
  // output wire audio_clk,
  // output wire audio_lr
);

reg [31:0] ram1;
reg [15:0] ram2;

reg [4:0] select_channel;
reg [31:0] voice_mask;
reg [31:0] voice_mask_pending;
reg voice_mask_updating;
reg [31:0] write_latch;
reg [31:0] wave_read_address;
reg [7:0] wave_byte_latch;
reg [31:0] read_latch;
reg [7:0] config_reg_3c;
reg [7:0] config_reg_3d;
reg [31:0] irq_channel;
reg [31:0] irq_assert;
reg [31:0] nfs;
reg [31:0] tv_counter;

reg [31:0] accum_l;
reg [31:0] accum_r;
reg [31:0] rcsum_0;
reg [31:0] rcsum_1;

always @(negedge wr_bus) begin
  case (address_bus)
    6'h00: begin voice_mask_pending[31:24] <= data_bus[7:0]; voice_mask_updating <= 1'b1; end
    6'h01: begin voice_mask_pending[23:16] <= data_bus[7:0]; voice_mask_updating <= 1'b1; end
    6'h02: begin voice_mask_pending[15:8]  <= data_bus[7:0]; voice_mask_updating <= 1'b1; end
    6'h03: begin voice_mask_pending[7:0]   <= data_bus[7:0]; voice_mask_updating <= 1'b1; end
    
    6'h05: write_latch[23:16] <= data_bus[7:0];
    6'h06: write_latch[15:8]  <= data_bus[7:0];
    6'h07: write_latch[7:0]   <= data_bus[7:0]; // TODO: write ram1
    6'h09: write_latch[23:16] <= data_bus[7:0];
    6'h0a: write_latch[15:8]  <= data_bus[7:0];
    6'h0b: write_latch[7:0]   <= data_bus[7:0]; // TODO: write ram1
    6'h0d: write_latch[23:16] <= data_bus[7:0];
    6'h0e: write_latch[15:8]  <= data_bus[7:0];
    6'h0f: write_latch[7:0]   <= data_bus[7:0]; // TODO: write ram1

    6'h21: wave_read_address[23:16] <= data_bus[7:0];
    6'h22: wave_read_address[15:8]  <= data_bus[7:0];
    6'h23: wave_read_address[7:0]   <= data_bus[7:0]; // TODO: read rom
    
    6'h25: write_latch[23:16] <= data_bus[7:0];
    6'h26: write_latch[15:8]  <= data_bus[7:0];
    6'h27: write_latch[7:0]   <= data_bus[7:0]; // TODO: write ram1

    6'h3c: config_reg_3c  <= data_bus;
    6'h3d: config_reg_3d  <= data_bus;
    6'h3e: select_channel <= data_bus[4:0];
  endcase
end

endmodule
