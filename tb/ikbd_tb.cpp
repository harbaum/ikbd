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

void tick(int c) {
  tb->clk = c;
  tb->eval();
  trace->dump(tickcount++);
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
  tb->pi2 = 0xff;
  
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
