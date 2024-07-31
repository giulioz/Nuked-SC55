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
  output wire wait_bus,

  // input wire [7:0] wave_data,
  // output wire [19:0] wave_addr,
  // output wire [6:0] wave_cs,
  // output wire wave_cs7_a20,
  
  // input wire [7:0] mem_data,
  // output wire [14:0] mem_addr,
  // output wire mem_we,
  // output wire mem_cs,

  output reg [15:0] audio_out_l,
  output reg [15:0] audio_out_r
  // output wire audio_clk,
  // output wire audio_lr
);

reg [9:0] current_cycle;

reg [31:0] ram1 [0:31][0:7];
reg [15:0] ram2 [0:31][0:15];

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

reg [6:0] interp_lut_addr;
reg [11:0] interp_lut_out_1;
reg [11:0] interp_lut_out_2;
reg [11:0] interp_lut_out_3;
always @(interp_lut_addr) begin
  case (interp_lut_addr)
    7'h00: begin interp_lut_out_1 = 12'hd39; interp_lut_out_2 = 12'h2c6; interp_lut_out_3 = 12'h0; end
    7'h01: begin interp_lut_out_1 = 12'hd49; interp_lut_out_2 = 12'h2d6; interp_lut_out_3 = 12'h0; end
    7'h02: begin interp_lut_out_1 = 12'hd59; interp_lut_out_2 = 12'h2e6; interp_lut_out_3 = 12'h0; end
    7'h03: begin interp_lut_out_1 = 12'hd68; interp_lut_out_2 = 12'h2f6; interp_lut_out_3 = 12'h1; end
    7'h04: begin interp_lut_out_1 = 12'hd78; interp_lut_out_2 = 12'h307; interp_lut_out_3 = 12'h1; end
    7'h05: begin interp_lut_out_1 = 12'hd87; interp_lut_out_2 = 12'h318; interp_lut_out_3 = 12'h1; end
    7'h06: begin interp_lut_out_1 = 12'hd96; interp_lut_out_2 = 12'h329; interp_lut_out_3 = 12'h2; end
    7'h07: begin interp_lut_out_1 = 12'hda4; interp_lut_out_2 = 12'h33a; interp_lut_out_3 = 12'h2; end
    7'h08: begin interp_lut_out_1 = 12'hdb2; interp_lut_out_2 = 12'h34c; interp_lut_out_3 = 12'h3; end
    7'h09: begin interp_lut_out_1 = 12'hdc1; interp_lut_out_2 = 12'h35d; interp_lut_out_3 = 12'h3; end
    7'h0a: begin interp_lut_out_1 = 12'hdce; interp_lut_out_2 = 12'h36f; interp_lut_out_3 = 12'h3; end
    7'h0b: begin interp_lut_out_1 = 12'hddc; interp_lut_out_2 = 12'h381; interp_lut_out_3 = 12'h4; end
    7'h0c: begin interp_lut_out_1 = 12'hdea; interp_lut_out_2 = 12'h393; interp_lut_out_3 = 12'h4; end
    7'h0d: begin interp_lut_out_1 = 12'hdf7; interp_lut_out_2 = 12'h3a5; interp_lut_out_3 = 12'h5; end
    7'h0e: begin interp_lut_out_1 = 12'he04; interp_lut_out_2 = 12'h3b8; interp_lut_out_3 = 12'h5; end
    7'h0f: begin interp_lut_out_1 = 12'he11; interp_lut_out_2 = 12'h3cb; interp_lut_out_3 = 12'h6; end
    7'h10: begin interp_lut_out_1 = 12'he1e; interp_lut_out_2 = 12'h3de; interp_lut_out_3 = 12'h6; end
    7'h11: begin interp_lut_out_1 = 12'he2a; interp_lut_out_2 = 12'h3f1; interp_lut_out_3 = 12'h7; end
    7'h12: begin interp_lut_out_1 = 12'he36; interp_lut_out_2 = 12'h404; interp_lut_out_3 = 12'h8; end
    7'h13: begin interp_lut_out_1 = 12'he42; interp_lut_out_2 = 12'h417; interp_lut_out_3 = 12'h8; end
    7'h14: begin interp_lut_out_1 = 12'he4e; interp_lut_out_2 = 12'h42b; interp_lut_out_3 = 12'h9; end
    7'h15: begin interp_lut_out_1 = 12'he59; interp_lut_out_2 = 12'h43f; interp_lut_out_3 = 12'ha; end
    7'h16: begin interp_lut_out_1 = 12'he65; interp_lut_out_2 = 12'h452; interp_lut_out_3 = 12'ha; end
    7'h17: begin interp_lut_out_1 = 12'he70; interp_lut_out_2 = 12'h466; interp_lut_out_3 = 12'hb; end
    7'h18: begin interp_lut_out_1 = 12'he7b; interp_lut_out_2 = 12'h47b; interp_lut_out_3 = 12'hc; end
    7'h19: begin interp_lut_out_1 = 12'he86; interp_lut_out_2 = 12'h48f; interp_lut_out_3 = 12'hd; end
    7'h1a: begin interp_lut_out_1 = 12'he90; interp_lut_out_2 = 12'h4a4; interp_lut_out_3 = 12'he; end
    7'h1b: begin interp_lut_out_1 = 12'he9b; interp_lut_out_2 = 12'h4b8; interp_lut_out_3 = 12'hf; end
    7'h1c: begin interp_lut_out_1 = 12'hea5; interp_lut_out_2 = 12'h4cd; interp_lut_out_3 = 12'h10; end
    7'h1d: begin interp_lut_out_1 = 12'heaf; interp_lut_out_2 = 12'h4e2; interp_lut_out_3 = 12'h11; end
    7'h1e: begin interp_lut_out_1 = 12'heb8; interp_lut_out_2 = 12'h4f7; interp_lut_out_3 = 12'h12; end
    7'h1f: begin interp_lut_out_1 = 12'hec2; interp_lut_out_2 = 12'h50c; interp_lut_out_3 = 12'h13; end
    7'h20: begin interp_lut_out_1 = 12'hecb; interp_lut_out_2 = 12'h522; interp_lut_out_3 = 12'h14; end
    7'h21: begin interp_lut_out_1 = 12'hed4; interp_lut_out_2 = 12'h537; interp_lut_out_3 = 12'h16; end
    7'h22: begin interp_lut_out_1 = 12'hedd; interp_lut_out_2 = 12'h54d; interp_lut_out_3 = 12'h17; end
    7'h23: begin interp_lut_out_1 = 12'hee6; interp_lut_out_2 = 12'h563; interp_lut_out_3 = 12'h18; end
    7'h24: begin interp_lut_out_1 = 12'heef; interp_lut_out_2 = 12'h578; interp_lut_out_3 = 12'h1a; end
    7'h25: begin interp_lut_out_1 = 12'hef7; interp_lut_out_2 = 12'h58f; interp_lut_out_3 = 12'h1b; end
    7'h26: begin interp_lut_out_1 = 12'heff; interp_lut_out_2 = 12'h5a5; interp_lut_out_3 = 12'h1d; end
    7'h27: begin interp_lut_out_1 = 12'hf07; interp_lut_out_2 = 12'h5bb; interp_lut_out_3 = 12'h1e; end
    7'h28: begin interp_lut_out_1 = 12'hf0f; interp_lut_out_2 = 12'h5d1; interp_lut_out_3 = 12'h20; end
    7'h29: begin interp_lut_out_1 = 12'hf17; interp_lut_out_2 = 12'h5e8; interp_lut_out_3 = 12'h22; end
    7'h2a: begin interp_lut_out_1 = 12'hf1e; interp_lut_out_2 = 12'h5fe; interp_lut_out_3 = 12'h24; end
    7'h2b: begin interp_lut_out_1 = 12'hf26; interp_lut_out_2 = 12'h615; interp_lut_out_3 = 12'h26; end
    7'h2c: begin interp_lut_out_1 = 12'hf2d; interp_lut_out_2 = 12'h62c; interp_lut_out_3 = 12'h28; end
    7'h2d: begin interp_lut_out_1 = 12'hf34; interp_lut_out_2 = 12'h642; interp_lut_out_3 = 12'h2a; end
    7'h2e: begin interp_lut_out_1 = 12'hf3b; interp_lut_out_2 = 12'h659; interp_lut_out_3 = 12'h2c; end
    7'h2f: begin interp_lut_out_1 = 12'hf41; interp_lut_out_2 = 12'h670; interp_lut_out_3 = 12'h2e; end
    7'h30: begin interp_lut_out_1 = 12'hf48; interp_lut_out_2 = 12'h687; interp_lut_out_3 = 12'h31; end
    7'h31: begin interp_lut_out_1 = 12'hf4e; interp_lut_out_2 = 12'h69f; interp_lut_out_3 = 12'h33; end
    7'h32: begin interp_lut_out_1 = 12'hf54; interp_lut_out_2 = 12'h6b6; interp_lut_out_3 = 12'h35; end
    7'h33: begin interp_lut_out_1 = 12'hf5a; interp_lut_out_2 = 12'h6cd; interp_lut_out_3 = 12'h38; end
    7'h34: begin interp_lut_out_1 = 12'hf60; interp_lut_out_2 = 12'h6e4; interp_lut_out_3 = 12'h3b; end
    7'h35: begin interp_lut_out_1 = 12'hf66; interp_lut_out_2 = 12'h6fc; interp_lut_out_3 = 12'h3e; end
    7'h36: begin interp_lut_out_1 = 12'hf6c; interp_lut_out_2 = 12'h713; interp_lut_out_3 = 12'h41; end
    7'h37: begin interp_lut_out_1 = 12'hf71; interp_lut_out_2 = 12'h72b; interp_lut_out_3 = 12'h44; end
    7'h38: begin interp_lut_out_1 = 12'hf76; interp_lut_out_2 = 12'h742; interp_lut_out_3 = 12'h47; end
    7'h39: begin interp_lut_out_1 = 12'hf7b; interp_lut_out_2 = 12'h75a; interp_lut_out_3 = 12'h4a; end
    7'h3a: begin interp_lut_out_1 = 12'hf80; interp_lut_out_2 = 12'h772; interp_lut_out_3 = 12'h4d; end
    7'h3b: begin interp_lut_out_1 = 12'hf85; interp_lut_out_2 = 12'h789; interp_lut_out_3 = 12'h51; end
    7'h3c: begin interp_lut_out_1 = 12'hf8a; interp_lut_out_2 = 12'h7a1; interp_lut_out_3 = 12'h54; end
    7'h3d: begin interp_lut_out_1 = 12'hf8f; interp_lut_out_2 = 12'h7b9; interp_lut_out_3 = 12'h58; end
    7'h3e: begin interp_lut_out_1 = 12'hf93; interp_lut_out_2 = 12'h7d0; interp_lut_out_3 = 12'h5c; end
    7'h3f: begin interp_lut_out_1 = 12'hf97; interp_lut_out_2 = 12'h7e8; interp_lut_out_3 = 12'h60; end
    7'h40: begin interp_lut_out_1 = 12'hf9b; interp_lut_out_2 = 12'h800; interp_lut_out_3 = 12'h64; end
    7'h41: begin interp_lut_out_1 = 12'hfa0; interp_lut_out_2 = 12'h815; interp_lut_out_3 = 12'h68; end
    7'h42: begin interp_lut_out_1 = 12'hfa4; interp_lut_out_2 = 12'h82f; interp_lut_out_3 = 12'h6d; end
    7'h43: begin interp_lut_out_1 = 12'hfa7; interp_lut_out_2 = 12'h847; interp_lut_out_3 = 12'h71; end
    7'h44: begin interp_lut_out_1 = 12'hfab; interp_lut_out_2 = 12'h85f; interp_lut_out_3 = 12'h76; end
    7'h45: begin interp_lut_out_1 = 12'hfaf; interp_lut_out_2 = 12'h876; interp_lut_out_3 = 12'h7a; end
    7'h46: begin interp_lut_out_1 = 12'hfb2; interp_lut_out_2 = 12'h88e; interp_lut_out_3 = 12'h7f; end
    7'h47: begin interp_lut_out_1 = 12'hfb6; interp_lut_out_2 = 12'h8a6; interp_lut_out_3 = 12'h84; end
    7'h48: begin interp_lut_out_1 = 12'hfb9; interp_lut_out_2 = 12'h8bd; interp_lut_out_3 = 12'h89; end
    7'h49: begin interp_lut_out_1 = 12'hfbc; interp_lut_out_2 = 12'h8d5; interp_lut_out_3 = 12'h8f; end
    7'h4a: begin interp_lut_out_1 = 12'hfbf; interp_lut_out_2 = 12'h8ec; interp_lut_out_3 = 12'h94; end
    7'h4b: begin interp_lut_out_1 = 12'hfc2; interp_lut_out_2 = 12'h904; interp_lut_out_3 = 12'h9a; end
    7'h4c: begin interp_lut_out_1 = 12'hfc5; interp_lut_out_2 = 12'h91b; interp_lut_out_3 = 12'ha0; end
    7'h4d: begin interp_lut_out_1 = 12'hfc8; interp_lut_out_2 = 12'h933; interp_lut_out_3 = 12'ha5; end
    7'h4e: begin interp_lut_out_1 = 12'hfca; interp_lut_out_2 = 12'h94a; interp_lut_out_3 = 12'hab; end
    7'h4f: begin interp_lut_out_1 = 12'hfcd; interp_lut_out_2 = 12'h961; interp_lut_out_3 = 12'hb2; end
    7'h50: begin interp_lut_out_1 = 12'hfcf; interp_lut_out_2 = 12'h979; interp_lut_out_3 = 12'hb8; end
    7'h51: begin interp_lut_out_1 = 12'hfd2; interp_lut_out_2 = 12'h990; interp_lut_out_3 = 12'hbf; end
    7'h52: begin interp_lut_out_1 = 12'hfd4; interp_lut_out_2 = 12'h9a7; interp_lut_out_3 = 12'hc5; end
    7'h53: begin interp_lut_out_1 = 12'hfd6; interp_lut_out_2 = 12'h9be; interp_lut_out_3 = 12'hcc; end
    7'h54: begin interp_lut_out_1 = 12'hfd9; interp_lut_out_2 = 12'h9d5; interp_lut_out_3 = 12'hd3; end
    7'h55: begin interp_lut_out_1 = 12'hfdb; interp_lut_out_2 = 12'h9eb; interp_lut_out_3 = 12'hdb; end
    7'h56: begin interp_lut_out_1 = 12'hfdd; interp_lut_out_2 = 12'ha02; interp_lut_out_3 = 12'he2; end
    7'h57: begin interp_lut_out_1 = 12'hfdf; interp_lut_out_2 = 12'ha19; interp_lut_out_3 = 12'hea; end
    7'h58: begin interp_lut_out_1 = 12'hfe0; interp_lut_out_2 = 12'ha2f; interp_lut_out_3 = 12'hf1; end
    7'h59: begin interp_lut_out_1 = 12'hfe2; interp_lut_out_2 = 12'ha46; interp_lut_out_3 = 12'hf9; end
    7'h5a: begin interp_lut_out_1 = 12'hfe4; interp_lut_out_2 = 12'ha5c; interp_lut_out_3 = 12'h101; end
    7'h5b: begin interp_lut_out_1 = 12'hfe6; interp_lut_out_2 = 12'ha72; interp_lut_out_3 = 12'h10a; end
    7'h5c: begin interp_lut_out_1 = 12'hfe7; interp_lut_out_2 = 12'ha88; interp_lut_out_3 = 12'h112; end
    7'h5d: begin interp_lut_out_1 = 12'hfe9; interp_lut_out_2 = 12'ha9e; interp_lut_out_3 = 12'h11b; end
    7'h5e: begin interp_lut_out_1 = 12'hfea; interp_lut_out_2 = 12'hab4; interp_lut_out_3 = 12'h124; end
    7'h5f: begin interp_lut_out_1 = 12'hfec; interp_lut_out_2 = 12'haca; interp_lut_out_3 = 12'h12d; end
    7'h60: begin interp_lut_out_1 = 12'hfed; interp_lut_out_2 = 12'hadf; interp_lut_out_3 = 12'h136; end
    7'h61: begin interp_lut_out_1 = 12'hfee; interp_lut_out_2 = 12'haf5; interp_lut_out_3 = 12'h13f; end
    7'h62: begin interp_lut_out_1 = 12'hfef; interp_lut_out_2 = 12'hb0a; interp_lut_out_3 = 12'h149; end
    7'h63: begin interp_lut_out_1 = 12'hff1; interp_lut_out_2 = 12'hb1f; interp_lut_out_3 = 12'h153; end
    7'h64: begin interp_lut_out_1 = 12'hff2; interp_lut_out_2 = 12'hb34; interp_lut_out_3 = 12'h15d; end
    7'h65: begin interp_lut_out_1 = 12'hff3; interp_lut_out_2 = 12'hb49; interp_lut_out_3 = 12'h167; end
    7'h66: begin interp_lut_out_1 = 12'hff4; interp_lut_out_2 = 12'hb5e; interp_lut_out_3 = 12'h171; end
    7'h67: begin interp_lut_out_1 = 12'hff5; interp_lut_out_2 = 12'hb73; interp_lut_out_3 = 12'h17c; end
    7'h68: begin interp_lut_out_1 = 12'hff6; interp_lut_out_2 = 12'hb87; interp_lut_out_3 = 12'h187; end
    7'h69: begin interp_lut_out_1 = 12'hff6; interp_lut_out_2 = 12'hb9b; interp_lut_out_3 = 12'h192; end
    7'h6a: begin interp_lut_out_1 = 12'hff7; interp_lut_out_2 = 12'hbaf; interp_lut_out_3 = 12'h19d; end
    7'h6b: begin interp_lut_out_1 = 12'hff8; interp_lut_out_2 = 12'hbc3; interp_lut_out_3 = 12'h1a8; end
    7'h6c: begin interp_lut_out_1 = 12'hff9; interp_lut_out_2 = 12'hbd7; interp_lut_out_3 = 12'h1b4; end
    7'h6d: begin interp_lut_out_1 = 12'hff9; interp_lut_out_2 = 12'hbeb; interp_lut_out_3 = 12'h1c0; end
    7'h6e: begin interp_lut_out_1 = 12'hffa; interp_lut_out_2 = 12'hbfe; interp_lut_out_3 = 12'h1cc; end
    7'h6f: begin interp_lut_out_1 = 12'hffb; interp_lut_out_2 = 12'hc11; interp_lut_out_3 = 12'h1d8; end
    7'h70: begin interp_lut_out_1 = 12'hffb; interp_lut_out_2 = 12'hc24; interp_lut_out_3 = 12'h1e4; end
    7'h71: begin interp_lut_out_1 = 12'hffc; interp_lut_out_2 = 12'hc37; interp_lut_out_3 = 12'h1f1; end
    7'h72: begin interp_lut_out_1 = 12'hffc; interp_lut_out_2 = 12'hc4a; interp_lut_out_3 = 12'h1fe; end
    7'h73: begin interp_lut_out_1 = 12'hffd; interp_lut_out_2 = 12'hc5c; interp_lut_out_3 = 12'h20b; end
    7'h74: begin interp_lut_out_1 = 12'hffd; interp_lut_out_2 = 12'hc6e; interp_lut_out_3 = 12'h218; end
    7'h75: begin interp_lut_out_1 = 12'hffe; interp_lut_out_2 = 12'hc80; interp_lut_out_3 = 12'h225; end
    7'h76: begin interp_lut_out_1 = 12'hffe; interp_lut_out_2 = 12'hc92; interp_lut_out_3 = 12'h233; end
    7'h77: begin interp_lut_out_1 = 12'hffe; interp_lut_out_2 = 12'hca4; interp_lut_out_3 = 12'h241; end
    7'h78: begin interp_lut_out_1 = 12'hffe; interp_lut_out_2 = 12'hcb5; interp_lut_out_3 = 12'h24f; end
    7'h79: begin interp_lut_out_1 = 12'hfff; interp_lut_out_2 = 12'hcc7; interp_lut_out_3 = 12'h25d; end
    7'h7a: begin interp_lut_out_1 = 12'hfff; interp_lut_out_2 = 12'hcd8; interp_lut_out_3 = 12'h26b; end
    7'h7b: begin interp_lut_out_1 = 12'hfff; interp_lut_out_2 = 12'hce8; interp_lut_out_3 = 12'h27a; end
    7'h7c: begin interp_lut_out_1 = 12'hfff; interp_lut_out_2 = 12'hcf9; interp_lut_out_3 = 12'h288; end
    7'h7d: begin interp_lut_out_1 = 12'hfff; interp_lut_out_2 = 12'hd0a; interp_lut_out_3 = 12'h297; end
    7'h7e: begin interp_lut_out_1 = 12'hfff; interp_lut_out_2 = 12'hd1a; interp_lut_out_3 = 12'h2a7; end
    7'h7f: begin interp_lut_out_1 = 12'hfff; interp_lut_out_2 = 12'hd2a; interp_lut_out_3 = 12'h2b6; end
  endcase
end

function [31:0] addclip20;
  input [31:0] add1, add2, cin;
  begin
    // TODO
    addclip20 = add1 + add2 + cin;
    if (add1[19] && add2[19] && !(addclip20[19])) addclip20 = 32'h80000;
    else if ((!add1[19]) && (!add2[19]) && addclip20[19]) addclip20 = 32'h7ffff;
  end
endfunction

function [19:0] multi;
  input [19:0] val1;
  input [7:0] val2;
  begin
    // TODO
    multi = val1 * val2;
  end
endfunction

always @(negedge sys_clk) begin : main_sm
  reg [31:0] accum_l;
  reg [31:0] accum_r;
  reg [31:0] rcsum_0;
  reg [31:0] rcsum_1;

  reg [15:0] shifter;
  reg xr;
  reg [7:0] noise_mask;
  reg [7:0] orval;
  reg [7:0] write_mask;
  reg [7:0] dac_mask;

  if (!sys_rst) begin
    current_cycle       <= 10'h0;
    select_channel      <= 5'h0;
    voice_mask          <= 32'h0;
    voice_mask_pending  <= 32'h0;
    voice_mask_updating <= 1'h0;
    write_latch         <= 32'h0;
    wave_read_address   <= 32'h0;
    wave_byte_latch     <= 8'h0;
    read_latch          <= 32'h0;
    config_reg_3c       <= 8'h0;
    config_reg_3d       <= 8'h0;
    irq_channel         <= 32'h0;
    irq_assert          <= 32'h0;
    nfs                 <= 32'h0;
    tv_counter          <= 32'h0;
    accum_l             <= 32'h0;
    accum_r             <= 32'h0;
    rcsum_0             <= 32'h0;
    rcsum_1             <= 32'h0;
  end

  else if (!wr_bus) begin
    case (address_bus)
      6'h70: begin voice_mask_pending[31:24] <= data_bus[7:0]; voice_mask_updating <= 1'b1; end
      6'h71: begin voice_mask_pending[23:16] <= data_bus[7:0]; voice_mask_updating <= 1'b1; end
      6'h72: begin voice_mask_pending[15:8]  <= data_bus[7:0]; voice_mask_updating <= 1'b1; end
      6'h73: begin voice_mask_pending[7:0]   <= data_bus[7:0]; voice_mask_updating <= 1'b1; end
     
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

  else if (!rd_bus) begin
    case (address_bus)
      6'h80: begin voice_mask_pending[31:24] <= data_bus[7:0]; voice_mask_updating <= 1'b1; end
      6'h81: begin voice_mask_pending[23:16] <= data_bus[7:0]; voice_mask_updating <= 1'b1; end
      6'h82: begin voice_mask_pending[15:8]  <= data_bus[7:0]; voice_mask_updating <= 1'b1; end
      6'h83: begin voice_mask_pending[7:0]   <= data_bus[7:0]; voice_mask_updating <= 1'b1; end
      
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

  // final mixing
  else if (current_cycle == 10'h00) begin
    if ((config_reg_3c & 8'h30) != 0) begin
      case ((config_reg_3c >> 2) & 3)
        1: noise_mask = 3;
        2: noise_mask = 7;
        3: noise_mask = 15;
      endcase
      case (config_reg_3c & 3)
        1: orval = orval| (1 << 8);
        2: orval = orval| (1 << 10);
      endcase
      write_mask = 15;
      dac_mask = ~15;
    end
    else begin
      case ((config_reg_3c >> 2) & 3)
        2: noise_mask = 1;
        3: noise_mask = 3;
      endcase
      case (config_reg_3c & 3)
        1: orval = orval| (1 << 6);
        2: orval = orval| (1 << 8);
      endcase
      write_mask = 3;
      dac_mask = ~3;
    end
    if ((config_reg_3c & 8'h80) == 0)
      write_mask = 0;
    if ((config_reg_3c & 8'h30) == 8'h30)
      orval = orval| (1 << 12);

    shifter = ram2[30][10];
    xr = (shifter >> 0) ^ (shifter >> 1) ^ (shifter >> 7) ^ (shifter >> 12);
    shifter = (shifter >> 1) | (xr << 15);
    ram2[30][10] <= shifter;

    accum_l = addclip20(accum_l, ram1[30][0], 0);
    accum_l = addclip20(accum_r, ram1[30][1], 0);

    ram1[30][2] = addclip20(accum_l, orval | (shifter & noise_mask), 0);
    ram1[30][4] = addclip20(accum_r, orval | (shifter & noise_mask), 0);

    ram1[30][0] = accum_l & write_mask;
    ram1[30][1] = accum_r & write_mask;

    audio_out_l <= (ram1[30][2] & ~write_mask) << 12;
    audio_out_r <= (ram1[30][4] & ~write_mask) << 12;

    // TODO: post sample

    xr = ((shifter >> 0) ^ (shifter >> 1) ^ (shifter >> 7) ^ (shifter >> 12)) & 1;
    shifter = (shifter >> 1) | (xr << 15);

    accum_l = addclip20(accum_l, ram1[30][0], 0);
    accum_r = addclip20(accum_r, ram1[30][1], 0);

    ram1[30][3] = addclip20(accum_l, orval | (shifter & noise_mask), 0);
    ram1[30][5] = addclip20(accum_r, orval | (shifter & noise_mask), 0);

    // oversampling
    if (config_reg_3c[6])
      current_cycle <= current_cycle + 10'h1;
    else
      current_cycle <= current_cycle + 10'h2;
  end

  // TODO: is this its own cycle?
  // oversampling
  else if (current_cycle == 10'h01) begin
    ram2[30][10] <= shifter;
    ram2[30][0] <= accum_l & write_mask;
    ram2[30][1] <= accum_r & write_mask;
    audio_out_l <= (ram1[30][3] & ~write_mask) << 12;
    audio_out_r <= (ram1[30][5] & ~write_mask) << 12;

    // TODO: post sample

    current_cycle <= current_cycle + 10'h1;
  end

  // global counter for envelopes
  else if (current_cycle == 10'h02) begin
    // if (!nfs)
    //   pcm.tv_counter = pcm.ram2[31][8];

    // pcm.tv_counter -= 1;
    // pcm.tv_counter &= 0x3fff;

    current_cycle <= current_cycle + 10'h1;
  end
end

endmodule
