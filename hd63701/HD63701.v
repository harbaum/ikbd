/******************************************************
    HD63701V0(Mode6) Compatible Processor Core
              Written by Tsuyoshi HASEGAWA 2013-14
*******************************************************/
module HD63701V0_M6
(
 input 	       CLKx2, // XTAL/EXTAL (200K~2.0MHz)

 input 	       RST, // RES
 input 	       NMI, // NMI
 input 	       IRQ, // IRQ1

 output        RW, // CS2
 output [15:0] AD, //  AS ? {PO4,PO3}
 output [7:0]  DO, // ~AS ? {PO3}
 input [7:0]   DI, //       {PI3}

 input [7:0]   PI1, // Port1 IN
 output [7:0]  PO1, //       OUT

 input [4:0]   PI2, // Port2 IN
 output [4:0]  PO2  //       OUT
);

// map sci tx onto PO3 if transmitter is enabled
assign PO2 = te?{txd,PO2I[3:0]}:PO2I;      
wire [4:0] 	  PO2I;
      
// trigger on certain rom addresses
wire TRG = ADI == 16'hf914;
wire TRG_IRQ0 = ADI == 16'hfee2;
wire TRG_IRQ2 = ADI == 16'hfd9d;

wire [15:0] ADI;
wire [2:0] PC;   
wire [7:0] PO3;   
wire [7:0] PO4;   

// Multiplex PO3 and PO4 onto external AD port in mode 7
assign AD = (PC == 3'b111)?{ PO4, PO3 }:ADI;  
   
// Built-In Instruction ROM TODO: include mode (PC) here
wire en_birom = (ADI[15:12]==4'b1111);			// $F000-$FFFF
wire [7:0] biromd;
MCU_BIROM irom( CLKx2, ADI[11:0], biromd );


// Built-In WorkRAM
wire		  en_biram;
wire [7:0] biramd;
HD63701_BIRAM biram( CLKx2, ADI, RW, DO, en_biram, biramd );


// Built-In I/O Ports
wire		  en_biio;
wire [7:0] biiod;
HD63701_IOPort iopt( RST, CLKx2, ADI, RW, DO, en_biio, biiod, PI1, PI2, PC, PO1, PO2I, PO3, PO4 );


// Built-In Serial Communication Hardware
wire		  irq0;
wire		  txd;
wire		  te;
wire		  en_bisci;
wire [7:0] biscid;
HD63701_SCI sci( RST, CLKx2, ADI, RW, DO, PI2[3], txd, te, irq0, en_bisci, biscid );


// Built-In Timer
wire		  irq2;
wire [3:0] irq2v;
wire		  en_bitim;
wire [7:0] bitimd;
HD63701_Timer timer( RST, CLKx2, ADI, RW, DO, irq2, irq2v, en_bitim, bitimd );


// Built-In Devices Data Selector
wire [7:0] biddi;
HD63701_BIDSEL bidsel
(
 biddi,
 en_birom, biromd,
 en_biram, biramd,
 en_biio , biiod, 
 en_bisci, biscid,
 en_bitim, bitimd,
 DI
 );

// Processor Core
HD63701_Core core
  (
   .CLKx2(CLKx2),.RST(RST),
   .NMI(NMI),.IRQ(IRQ),.IRQ2(irq2),.IRQ2V(irq2v),.IRQ0(irq0),
   .RW(RW),.AD(ADI),.DO(DO),.DI(biddi)
   );
  
endmodule

module HD63701_BIDSEL 
(
 output [7:0] o,

 input 	      e0, input [7:0] d0,
 input 	      e1, input [7:0] d1,
 input 	      e2, input [7:0] d2,
 input 	      e3, input [7:0] d3,
 input 	      e4, input [7:0] d4,

 input [7:0]  dx
);

assign o = e0 ? d0 :
	   e1 ? d1 :
	   e2 ? d2 :
	   e3 ? d3 :
	   e4 ? d4 :
	   dx;
   
endmodule


module HD63701_BIRAM
(
 input 		  mcu_clx2,
 input [15:0] 	  mcu_ad,
 input 		  mcu_wr,
 input [7:0] 	  mcu_do,
 output 	  en_biram,
 output reg [7:0] biramd
);

assign en_biram = (mcu_ad[15: 7]==9'b000000001);	// $0080-$00FF
wire [6:0] biad = mcu_ad[6:0];

reg [7:0] bimem[0:127];
always @( posedge mcu_clx2 ) begin
	if (en_biram & mcu_wr) bimem[biad] <= mcu_do;
	else biramd <= bimem[biad];
end

endmodule


module HD63701_IOPort
(
 input 		  mcu_rst,
 input 		  mcu_clx2,
 input [15:0] 	  mcu_ad,
 input 		  mcu_wr,
 input [7:0] 	  mcu_do,

 output 	  en_io,
 output [7:0] 	  iod,
	
 input [7:0] 	  PI1,
 input [4:0] 	  PI2,

 output reg [2:0] PC,
 output [7:0] 	  PO1,
 output [4:0] 	  PO2,
 output [7:0] 	  PO3,
 output [7:0] 	  PO4
);

   assign PO1 = (~DDR1) | PO1R;
   assign PO2 = (~DDR2) | PO2R;
   assign PO3 = (~DDR3) | PO3R;
   assign PO4 = (~DDR4) | PO4R;
   
   reg [7:0] 	  DDR1;
   reg [4:0] 	  DDR2;
   reg [7:0] 	  DDR3;
   reg [7:0] 	  DDR4;
   
   reg [7:0] 	  PO1R;   
   reg [4:0] 	  PO2R;
   reg [7:0] 	  PO3R;   
   reg [7:0] 	  PO4R;
  
always @( posedge mcu_clx2 or posedge mcu_rst ) begin
   if (mcu_rst) begin
      PC  <= PI2[2:0];
      DDR1 <= 8'h00;
      DDR2 <= 5'h00;      
      DDR3 <= 8'h00;
      DDR4 <= 8'h00;      
      PO1R <= 8'hFF;
      PO2R <= 5'h1F;
      PO3R <= 8'hFF;
      PO4R <= 8'hFF;
   end
   else begin
      if (mcu_wr) begin
	 if (mcu_ad==16'h0) DDR1 <= mcu_do;
	 if (mcu_ad==16'h1) DDR2 <= mcu_do;
	 if (mcu_ad==16'h2) PO1R <= mcu_do;
	 if (mcu_ad==16'h3) PO2R <= mcu_do[4:0];
	 if (mcu_ad==16'h4) DDR3 <= mcu_do;
	 if (mcu_ad==16'h5) DDR4 <= mcu_do;
	 if (mcu_ad==16'h6) PO3R <= mcu_do;
	 if (mcu_ad==16'h7) PO4R <= mcu_do;
      end
   end
end
   
// IO from 0x0000 to 0x0007
assign en_io = (mcu_ad[15:3] == 13'h0);
// only addresses 2 and 3 return data
assign iod = 
	     (mcu_ad==16'h0) ? DDR1 : 
	     (mcu_ad==16'h1) ? DDR2 : 
	     (mcu_ad==16'h2) ? PI1 : 
	     (mcu_ad==16'h3) ? {3'hF,PI2}:
	     (mcu_ad==16'h4) ? DDR3 : 
	     (mcu_ad==16'h5) ? DDR4 : 
	     (mcu_ad==16'h6) ? 8'hff :  // no PI3 yet
	     8'hff;                     // no PI4 yet

endmodule


module HD63701_SCI
(
 input 	      mcu_rst,
 input 	      mcu_clx2,
 input [15:0] mcu_ad,
 input 	      mcu_wr,
 input [7:0]  mcu_do,

 input 	      rx,
 output       tx,
 output       te,
 output       mcu_irq0,
 output       en_sci,
 output [7:0] iod
);

   reg [7:0]  RMCR;   // Rate and Mode Control Register
   reg [7:0]  TRCSR;  // Transmit/Receive Control and Status Register   
   reg [7:0]  RDR;    // Receive Data Register
   reg [7:0]  TDR;    // Transmit Data Register 	     

   reg 	      RDRF = 1'b0;  // receive data register full  
   reg 	      TDRE = 1'b1;  // transmit data register empty
   
   reg 	      last_rx = 1'b0;

   reg [8:0]  rxsr;   // receive shift register    
   reg [8:0]  rxcnt;  // 9 bit receive counter
   
   reg [8:0]  txcnt;  // 9 bit transmit counter
   reg [8:0]  txsr;
      
   always @( posedge mcu_clx2 or posedge mcu_rst ) begin
      if (mcu_rst) begin
	 RMCR  <= 8'h00;
	 TRCSR <= 8'h00;
	 RDR   <= 8'h00;
	 TDR   <= 8'h00;
	 last_rx <= 1'b1;
	 rxcnt <= 9'h000;
	 txcnt <= 9'h000;
	 rxsr <= 9'h000;
	 txsr <= 9'h1ff;
	 tx <= 1'b1;
      end
      else begin
	 // sync rx clock on any data edge
	 last_rx <= rx;
	 rxcnt <= rxcnt + 1;	 
	 if(last_rx != rx) begin
	    rxcnt <= 9'h000;	    
	 end

	 // sample serial bit in the middle of the 512 clock
	 // cycle @ 7812.5 bit/s and shift it into rx buffer
	 if(rxcnt == 9'd256) begin
	    rxsr <= { !rx, rxsr[8:1] };
	    // a full byte has been received whenever the
	    // lowest bit would become '1' due to the start bit
	    // and ~rx must be 0 due to stop bit
	    if(rxsr[0] == 1'b1 && ~rx == 1'b0) begin
	       rxsr <= 9'h000;
	       RDR <= rxsr[8:1];
	       RDRF <= 1'b1;
	    end
	 end
	 
	 if(en_sci && !mcu_wr && (mcu_ad==16'h12)) begin
	    RDRF <= 1'b0;	    
	 end
	 
	 if (mcu_wr) begin
	    if (mcu_ad==16'h10) RMCR <= mcu_do;
	    if (mcu_ad==16'h11) TRCSR <= mcu_do;
	    if (mcu_ad==16'h13) begin
	       TDRE <= 1'b0;	       
	       TDR <= mcu_do;
	    end
	 end

	 txcnt <= txcnt + 1;
	 if(txcnt == 9'h1ff) begin	 
	    // if txsr == 0x1ff then no transmission is in progress
	    if((txsr == 9'h1ff) && !TDRE) begin
	       TDRE <= 1'b1;
	       txcnt <= 9'h000;       // start tx bit timer
	       txsr <= { 1'b0, TDR }; // data incl stop bit
	       tx <= 1'b0;	      // send start bit
	    end

	    // transmit byte if txsr is not empty
	    if(txsr != 9'h1ff) begin
	       tx <= ~txsr[0];
	       txsr <= { 1'b1, txsr[8:1] };
	    end
	 end
      end
   end
   
   // bit 0 (wakeup) is cleared by the hardware after seeing 10 1's on RX,
   // we always return 0
   wire [7:0] TRCSR_O = { RDRF, TRCSR[6], TDRE, TRCSR[4:1], 1'b0 };

   assign te = TRCSR[1];   // transmitter enable

   // interrupt on receive or transmit
   assign mcu_irq0 = (TRCSR[4] && RDRF) ||
		     (TRCSR[2] && TDRE); 
   
   assign en_sci = (mcu_ad[15:2] == 14'h004);
   assign iod = (mcu_ad==16'h10) ? RMCR :
		(mcu_ad==16'h11) ? TRCSR_O :
		(mcu_ad==16'h12) ? RDR :
		TDR;
   
endmodule


module HD63701_Timer
(
	input			 mcu_rst,
	input			 mcu_clx2,
	input [15:0] mcu_ad,
	input 		 mcu_wr,
	input  [7:0] mcu_do,

	output		 mcu_irq2,
	output [3:0] mcu_irq2v,

	output		 en_timer,
	output [7:0] timerd
);

reg		  oci, oce;
reg [15:0] ocr, icr;
reg [16:0] frc;
reg  [7:0] frt;
reg  [7:0] rmc;

always @( posedge mcu_clx2 or posedge mcu_rst ) begin
	if (mcu_rst) begin
		oce <= 0;
		ocr <= 16'hFFFF;
		icr <= 16'hFFFF;
		frc <= 0;
		frt <= 0;
		rmc <= 8'h40;
	end
	else begin
		frc <= frc+1;
		if (mcu_wr) begin
			case (mcu_ad)
				16'h08: oce <= mcu_do[3];
				16'h09: frt <= mcu_do;
				16'h0A: frc <= {frt,mcu_do,1'h0};
				16'h0B: ocr[15:8] <= mcu_do;
				16'h0C: ocr[ 7:0] <= mcu_do;
				16'h0D: icr[15:8] <= mcu_do;
				16'h0E: icr[ 7:0] <= mcu_do;
				16'h14: rmc <= {mcu_do[7:6],6'h0};
				default:;
			endcase
		end
	end
end

always @( negedge mcu_clx2 or posedge mcu_rst ) begin
	if (mcu_rst) begin
		oci <= 1'b0;
	end
	else begin
		case (mcu_ad)
			16'h0B: oci <= 1'b0;
			16'h0C: oci <= 1'b0;
			default: if (frc[16:1]==ocr) oci <= 1'b1;
		endcase
	end
end

assign mcu_irq2  = oci & oce;
assign mcu_irq2v = 4'h4;

assign en_timer = ((mcu_ad>=16'h8)&(mcu_ad<=16'hE))|(mcu_ad==16'h14);

assign   timerd = (mcu_ad==16'h08) ? {1'b0,oci,2'b10,oce,3'b000}:
		  (mcu_ad==16'h09) ? frc[16:9] :
		  (mcu_ad==16'h0A) ? frc[ 8:1] :
		  (mcu_ad==16'h0B) ? ocr[15:8] :
		  (mcu_ad==16'h0C) ? ocr[ 7:0] :
		  (mcu_ad==16'h0D) ? icr[15:8] :
		  (mcu_ad==16'h0E) ? icr[ 7:0] :
		  (mcu_ad==16'h14) ? rmc :
		  8'h0;

endmodule

