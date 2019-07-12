#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include "Vikbd_tb.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

static Vikbd_tb *tb;
static VerilatedVcdC *trace;
static int tickcount;

// commands to be sent to IKBD
unsigned char cmd_reset[] = { 2, 0x80, 0x01 };
unsigned char cmd_interrogate_mouse_position[] = { 1, 0x0D };

int st = 0;
unsigned char *sp = NULL;
int sc = 0;

// ticks per uart bit. ticks come for both clock edges ->
// 8M ticks @ 4Mhz
#define GAP  1   // extra pause between bytes in bits
#define TPB  (int)(125*4000000*2/7812.5)

void serial_do() {
  static int rxsr = 0;
  static int rxcnt = 0;
  static int rxt = 0;
  
  // check for incoming data on po
  if(!(tb->po2 & 0x10) && !rxcnt) {
    // printf("@%d: RX start\n", tickcount);

    // make sure we sample in the middle of the bit
    rxt = tickcount - TPB/2;
    rxcnt = 10;
    rxsr = 0;
  }

  // rx in progess 
  if(rxcnt) {
    if((tickcount - rxt) == TPB) {
      // printf("Peek @ %d %d: @%d %02x\n", tickcount, TPB, rxcnt, rxsr);

      if(rxcnt > 1) {
	rxsr >>= 1;
	if(!(tb->po2 & 0x10))
	  rxsr |= 0x80;
      } else    
	printf("@%.2fµs RX %02x\n", tickcount/1000.0, rxsr);
      
      rxt = tickcount;
      rxcnt--;
    }    
  }

  if(!sp) return;

  if(tickcount - st < TPB) return;

  int bitn = sc%(10+GAP);
  int byten = sc/(10+GAP);
  int bit = 0, n='G';
  if(bitn == 0) {
    bit = 1;
    n = 'S';
    printf("TX %02x\n", sp[byten+1]);
  } else if(bitn < 9) {
    bit = (sp[byten+1]&(1<<(bitn-1)))?1:0;
    n = '0'+(bitn-1);
  }
    
  printf("@%.2fµs TX %d/%c: %d\n", tickcount/1000.0, byten, n, bit);

  if(bit) tb->pi2 = 0x17;
  else    tb->pi2 = 0x1f;

  sc++;
  st = tickcount;
  if(sc == (10+GAP)*sp[0])  // 10 bits per byte incl start, stop and GAP
    sp = NULL;
}

void serial_start(unsigned char *msg) {
  printf("start tx %d bytes\n", msg[0]);

  sc = 0;
  sp = msg;
  st = 0;
}
  
void tick(int c) {
  tb->clk = c;
  tb->eval();
  trace->dump(tickcount);
  tickcount += 125; // 2*125ns/cycle -> 4MHz

  // start a transmission
  if(tickcount == 40000000)
    serial_start(cmd_reset);
  // serial_start(cmd_interrogate_mouse_position);

  serial_do();
}

void ticks(int c) {
  for(int i=0;i<c;i++) {
    tick(1);
    tick(0);
  }
}

int main(int argc, char **argv) {
  // Initialize Verilators variables
  Verilated::commandArgs(argc, argv);
  //	Verilated::debug(1);
  Verilated::traceEverOn(true);
  trace = new VerilatedVcdC;
  
  // Create an instance of our module under test
  tb = new Vikbd_tb;
  tb->trace(trace, 99);
  trace->open("ikbd.vcd");

  // init all signals
  tb->res = 1;
  tb->nmi = 0;
  tb->irq = 0;
  
  tb->pi1 = 0xff;
  tb->pi2 = 0x1f;
  
  // apply reset
  ticks(5);
  tb->res = 0;
  printf("out of reset\n");
  
  for(int i=0;i<1000000;i++) {
    tick(1);
    tick(0);
  }
    
  trace->close();
}
