// ====================================================================
//
//  This program is free software; you can redistribute it and/or modify it
//  under the terms of the GNU General Public License as published by the Free
//  Software Foundation; either version 2 of the License, or (at your option)
//  any later version.
//
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along
//  with this program; if not, write to the Free Software Foundation, Inc.,
//  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
//============================================================================

module ikbd_tb (
		input clk,
		input res,
		input nmi,
		input irq,
		input [7:0] pi1,
		input [4:0] pi2,
		output [7:0] po1,
		output [4:0] po2,
		output [7:0] po3,
		output [7:0] po4
		);
   
   HD63701V0_M6 HD63701V0_M6 (
			      .CLKx2(clk),
			      .RST(res),
			      .NMI(nmi),
			      .IRQ(irq),
                              
			      .RW(),
			      .AD({po4, po3}),
			      .DO(),
			      .DI(),
			      
                              .PI1(pi1),
			      .PO1(po1),
			      .PI2(pi2),
			      .PO2(po2)
	      );
   
endmodule;
