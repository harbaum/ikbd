/***************************************************************************
       This file is part of "HD63701V0 Compatible Processor Core".
****************************************************************************/
`timescale 1ps / 1ps

`include "HD63701_defs.i"

module HD63701_SEQ
(
	input						CLK,
	input						RST,

	input						NMI,
	input						IRQ,

	input						IRQ0,
	input						IRQ2,
	input  	[3:0]			IRQ2V,

	input 	[7:0]			DI,

	output `mcwidth		mcout,
	input		[7:0]			vect,
	input						inte,
	output					fncu
);

`define MC_SEI {`mcSCB,   `bfI    ,`mcrC,`mcpN,`amPC,`pcN}
`define MC_YLD {`mcNOP,`mcrn,`mcrn,`mcrn,`mcpK,`amPC,`pcN} 

reg [7:0]   opcode;
reg `mcwidth mcode;
reg  mcside;
wire TRG_AIM = opcode == 8'h71;
wire TRG_OIM = opcode == 8'h72;
wire TRG_EIM = opcode == 8'h75;
wire TRG_TIM = opcode == 8'h7C;
   


reg  pNMI, pIRQ, pIR0, pIR2;

reg  [3:0]  fINT;
wire bIRQ  = fINT[2] & inte;
wire bIRQ2 = fINT[1] & inte;
wire bIRQ0 = fINT[0] & inte;

wire  	   bINT = fINT[3]|bIRQ|bIRQ2|bIRQ0;
wire [7:0] vINT = fINT[3] ? `vaNMI :    // NMI     $fc
	   bIRQ    ? `vaIRQ :           // ext IRQ $f8
	   bIRQ2   ? {4'hF,IRQ2V} :     // TIM OCF $f4
	   bIRQ0   ? 8'hf0 :            // SCI     $f0
	   0;

function [3:0] INTUpd;
input [3:0] n;
   INTUpd = n[3]?{1'b0,n[2:0]}:
	    n[2]?{2'b00,n[1:0]}:
	    n[1]?{3'b000,n[0]}:
	    4'b0000;
endfunction


reg [5:0] PHASE;
always @( posedge CLK or posedge RST ) begin
	if (RST) begin
		fINT <= 0;
		pIRQ <= 0;
		pNMI <= 0;
		pIR0 <= 0;
		pIR2 <= 0;

		opcode <= 0;
		mcode  <= 0;
		mcside <= 0;
	end
	else begin


		case (PHASE)

		// Reset
		`phRST :	mcside <= 1;

		// Load Vector
		`phVECT: mcside <= 1;

		// Execute
		`phEXEC: begin
						opcode <= DI;
						if ( bINT & (opcode[7:1]!=7'b0000111) ) begin
							mcside <= 0;
							mcode  <= {`mcINT,vINT,`mcrn,`mcpI,`amPC,`pcN};
							fINT   <= INTUpd(fINT);
						end
						else mcside <= 1;
					end

		// Interrupt (TRAP/IRQ/NMI/SWI/WAI)
		`phINTR:  mcside <= 1; 
		`phINTR8: begin
						mcside <= 0;
						if (vect==`vaWAI) begin
							if (bINT) begin
								mcode  <= `MC_SEI;
								opcode <= vINT;
								fINT   <= INTUpd(fINT);
							end
							else mcode <= `MC_YLD;
						end
						else begin
							opcode <= vect;
							mcode  <= `MC_SEI;
						end
					 end
		`phINTR9: mcode <= {`mcLDV, opcode,`mcrn,`mcpV,`amE0,`pcN};	//(Load Vector)

		// Sleep
		`phSLEP: begin
						mcside <= 0;
						if (bINT) begin
							mcode  <= {`mcINT,vINT,`mcrn,`mcpI,`amPC,`pcN};
							fINT   <= INTUpd(fINT);
						end
						else mcode <= `MC_YLD;
					end

		// HALT (Bug in MicroCode)
		`phHALT: $stop;

		default:;
		endcase // case (PHASE)
	   
		// Capture Interrupt signal edge
		if ((pNMI^NMI)&NMI)   fINT[3] <= 1'b1; pNMI <= NMI;
		if ((pIRQ^IRQ)&IRQ)   fINT[2] <= 1'b1; pIRQ <= IRQ;
		if ((pIR2^IRQ2)&IRQ2) fINT[1] <= 1'b1; pIR2 <= IRQ2;
		if ((pIR0^IRQ0)&IRQ0) fINT[0] <= 1'b1; pIR0 <= IRQ0;

	end
end

// Update Phase
wire [2:0] mcph = mcout[6:4];
always @( negedge CLK or posedge RST ) begin
	if (RST) PHASE <= 0;
	else begin
		case (mcph)
			`mcpN: PHASE <= PHASE+6'h1;
			`mcp0: PHASE <=`phEXEC;
			`mcpI: PHASE <=`phINTR;
			`mcpV: PHASE <=`phVECT;
			`mcpH: PHASE <=`phHALT;
			`mcpS: PHASE <=`phSLEP;
		 default: PHASE <= PHASE;
		endcase
	end
end

// Output MicroCode
wire `mcwidth mcoder;
HD63701_MCROM mcr( CLK, PHASE, (PHASE==`phEXEC) ? DI : opcode, mcoder );
assign mcout = mcside ? mcoder : mcode;

assign fncu = ( opcode[7:4]==4'h2)|
				  ((opcode[7:4]==4'h3)&(opcode[3:0]!=4'hD));

endmodule

