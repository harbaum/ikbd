#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include "Vikbd_tb.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

// ========= commands to be sent to IKBD ============
unsigned char cmd_reset[] = { 2, 0x80, 0x01 };
unsigned char cmd_set_time[] = { 7, 0x1B, 0x19, 0x07, 0x15, 0x11, 0x57, 0x30 };
unsigned char cmd_get_time[] = { 1, 0x1C };

unsigned char cmd_request_mouse_mode[] = { 1, 0x88 };
unsigned char cmd_set_absolute_mouse_positioning[] = {
  5, 0x09, 640/256, 640%256, 400/256, 400%256 };
unsigned char cmd_interrogate_mouse_position[] = { 1, 0x0D };
unsigned char cmd_request_mouse_enable[] = { 1, 0x92 };

unsigned char cmd_interrogate_joystick[] = { 1, 0x16 };
unsigned char cmd_set_joystick_monitoring[] = { 2, 0x17, 10 };
unsigned char cmd_set_fire_button_monitoring[] = { 1, 0x18 };

#define TSER  0
#define TWIRE 1
#define TTEXT 2

#define IO_DONE           0
#define IO_PORT(a,b)      ((a<<4)|b)
#define IO_SET_BIT(a,b)   1, IO_PORT(a,b)
#define IO_CLR_BIT(a,b)   2, IO_PORT(a,b)
#define IO_WAIT(a)        3, a

unsigned char io_joy_fire_on[] = { IO_CLR_BIT(2, 2), IO_DONE };
unsigned char io_joy_fire_off[] = { IO_SET_BIT(2, 2), IO_DONE };
unsigned char io_joy_fire_on_off[] = {
  IO_CLR_BIT(2, 2), IO_WAIT(10), IO_SET_BIT(2, 2), IO_DONE };

// ========= events to be generated during simulation =========
// Consists of time in ms when request is to be sent to ikbd
// and the command to be sent.
struct {
  int time;
  int type;
  unsigned char *cmd;
} events[] = {
#if 0 // test reset only
#define RUNTIME_MS 200
  { 80, TSER, cmd_reset },
  // TODO: Reply after reset ~65ms, docs indicate 300ms
#endif
#if 0 // set/get time
#define RUNTIME_MS 2500
  { 80,   TSER, cmd_set_time },
  { 100,  TSER, cmd_get_time },  // should return same time as set
  { 1100, TSER, cmd_get_time },  // should return 1 sec advanced
  { 2100, TSER, cmd_get_time },  // should return 2 secs advanced
#endif
#if 0 // interrogate mouse position
#define RUNTIME_MS 200
  { 80,  TSER, cmd_request_mouse_mode },     // reply: mouse is in relative mode
  { 100, TSER, cmd_set_absolute_mouse_positioning },
  { 120, TSER, cmd_request_mouse_enable },   // reply: mouse is in absolute mode
  { 140, TSER, cmd_interrogate_mouse_position },
  // TODO: No reply?
#endif
#if 0 // request joystick information
#define RUNTIME_MS 200
  { 80,  TTEXT, (unsigned char*)"Request joystick information" },
  { 80, TSER, cmd_interrogate_joystick },
  // reply: $fd,$00,$00
  { 90, TWIRE, io_joy_fire_on },
  { 100, TSER, cmd_interrogate_joystick },
  // TODO: reply also: $fd,$00,$00 ???
  { 110, TWIRE, io_joy_fire_off },
#endif
#if 0 // joystick monitoring
#define RUNTIME_MS 120
  { 80, TSER, cmd_set_joystick_monitoring },
  { 90, TWIRE, io_joy_fire_on_off },
#endif
#if 1 // fire button monitoring
#define RUNTIME_MS 110
  { 80,  TTEXT, (unsigned char*)"Test fire button monitoring" },
  { 80, TSER, cmd_set_fire_button_monitoring },
  { 90, TWIRE, io_joy_fire_on_off },
#endif
  { 0, 0, NULL }
};

// serial signal generation
int st = 0;
unsigned char *sp = NULL;
int sc = 0;

// wire event generation
int wt = 0;
unsigned char *wp = NULL;
int wc = 0;

unsigned char P[4] = { 0xff, 0x1f, 0xff, 0xff };

static Vikbd_tb *tb;
static VerilatedVcdC *trace;
static int tickcount;

// ticks per uart bit. ticks come for both clock edges ->
// 4M ticks @ 2Mhz
#define GAP  1   // extra pause between bytes in bits
#define TPB  (int)(250*2000000*2/7812.5)

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
    printf("@%.2fµs TX %02x\n", tickcount/1000.0, sp[byten+1]);
  } else if(bitn < 9) {
    bit = (sp[byten+1]&(1<<(bitn-1)))?1:0;
    n = '0'+(bitn-1);
  }
    
  //  printf("@%.2fµs TX %d/%c: %d\n", tickcount/1000.0, byten, n, bit);

  // mix serial signals into port 2
  if(bit) { P[1] &= ~0x08; tb->pi2 = P[1]; }
  else    { P[1] |=  0x08; tb->pi2 = P[1]; };

  sc++;
  st = tickcount;
  if(sc == (10+GAP)*sp[0])  // 10 bits per byte incl start, stop and GAP
    sp = NULL;
}

void serial_start(unsigned char *msg) {
  sc = 0;
  sp = msg;
  st = 0;
}

void wire_do() {
  if(!wp) return;
  if(tickcount < wt) return;

  wt = tickcount + 1000;   // 1us
  
  switch(wp[wc]) {
  case 1: {  // set
    int port = wp[wc+1]>>4;
    int bit  = wp[wc+1]&15;
    printf("@%.2fµs SET(%d,%d)\n", tickcount/1000.0, port, bit);
    P[port-1] |= (1<<bit);    
    if(port == 1) tb->pi1 = P[0];
    if(port == 2) tb->pi2 = P[1];
    wc+=2;
  } break;
  case 2: {  // clear
    int port = wp[wc+1]>>4;
    int bit  = wp[wc+1]&15;
    printf("@%.2fµs CLR(%d,%d)\n", tickcount/1000.0, port, bit);
    P[port-1] &= ~(1<<bit);    
    if(port == 1) tb->pi1 = P[0];
    if(port == 2) tb->pi2 = P[1];
    wc+=2;
  } break;
  case 3:  // wait
    // printf("@%.2fµs WAIT(%dms)\n", tickcount/1000.0, wp[wc+1]);
    wt = tickcount + 1000000*wp[wc+1];   // X us
    wc+=2;
    break;

  default:
    break;
  }
  
  // DONE ?  
  if(!wp[wc]) {
    // printf("@%.2fµs DONE\n", tickcount/1000.0);
    wp = NULL;
  }
}
  
void wire_start(unsigned char *msg) {
  wc = 0;
  wp = msg;
  wt = 0;
}

void tick(int c) {
  tb->clk = c;
  tb->eval();
  trace->dump(tickcount);
  tickcount += 250; // 2*250ns/cycle -> 2MHz, matching a real 6301@4MHz

  // check if an event is supposed to start
  for(char i=0;events[i].time;i++) {
    if(events[i].time * 1000000 == tickcount) {
      if(events[i].type == TSER)
	serial_start(events[i].cmd);
      if(events[i].type == TWIRE)
	wire_start(events[i].cmd);
      if(events[i].type == TTEXT)
	printf("@%.2fµs %s\n", tickcount/1000.0, events[i].cmd);
    }
  }

  serial_do();
  wire_do();
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
  
  tb->pi1 = P[0];
  tb->pi2 = P[1];
  
  // apply reset
  ticks(5);
  tb->res = 0;
  printf("@%.2fµs out of reset\n", tickcount/1000.0);
  
  for(int i=0;i<2000*RUNTIME_MS;i++) {
    tick(1);
    tick(0);
  }
    
  trace->close();
}
