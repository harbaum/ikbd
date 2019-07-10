module MCU_BIROM
  (
   input 	CLK,
   input [11:0] AD,
   output [7:0] DO
   );
   
   reg [7:0] 	rom[0:4095];
   initial begin
      $readmemh ("../rom/ikbd.hex", rom, 0);
   end
   
   always @(posedge CLK) begin
      DO <= rom[AD];
   end
   
endmodule; // MCU_BIROM
