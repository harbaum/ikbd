/*
  IKBD/HD6301 verilator testbench

  (c) 2019 by Till Harbaum <till@harbaum.org>
*/

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include "Vikbd_tb.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

// == Port usage ==
// P20: Output: 0 when mouse/joy direction is to be read, 0 for keyboard scan
// P21: Input: Left mouse button and joystick 0 fire button
// P22: Input: Right mouse button and joystick 1 fire button
// P23: SCI RX
// P24: SCI TX

// P10-P17: Input: Row data
// P31-P37: Output: Column select
// P40-P47: Output: Column select

// P30: Unused, in some schematics used as a caps lock led output

// P1 usage for mouse/joystick directions:
// P10-0U, P11-0D, P12-0L, P13-0R, P14-1U, P15-1D, P16-1L, P17-1R
// P3/P4 must be 0xff to avoid conflicts with pressed keys

// ========= commands to be sent to IKBD ============
unsigned char cmd_reset[] = { 2, 0x80, 0x01 };
unsigned char cmd_set_time[] = { 7, 0x1B, 0x19, 0x07, 0x15, 0x11, 0x57, 0x30 };
unsigned char cmd_get_time[] = { 1, 0x1C };

#define U16(a)  (a/256),(a%256)
unsigned char cmd_request_mouse_mode[] = { 1, 0x88 };
unsigned char cmd_set_absolute_mouse_positioning[] = { 5, 0x09, U16(640), U16(400) };
unsigned char cmd_interrogate_mouse_position[] = { 1, 0x0D };
unsigned char cmd_request_mouse_enable[] = { 1, 0x92 };
unsigned char cmd_load_mouse[] = { 6, 0x0e, 0x00, U16(320), U16(200) };

unsigned char cmd_set_joystick_interrogation_mode[] = { 1, 0x15 };
unsigned char cmd_interrogate_joystick[] = { 1, 0x16 };
unsigned char cmd_set_joystick_monitoring[] = { 2, 0x17, 5 };  // 50ms
unsigned char cmd_set_fire_button_monitoring[] = { 1, 0x18 };

#define NONE  (0)
#define UP    (1<<0)
#define DOWN  (1<<1)
#define LEFT  (1<<2)
#define RIGHT (1<<3)
#define FIRE  (1<<4)

#define TSER  0
#define TWIRE 1
#define TTEXT 2

#define IO_DONE           0
#define IO_PORT(a,b)      ((a<<4)|b)
#define IO_SET_BIT(a,b)   1, IO_PORT(a,b)
#define IO_CLR_BIT(a,b)   2, IO_PORT(a,b)
#define IO_WAIT(a)        3, a
#define IO_SET_MTX(a,b)   4, a,b
#define IO_CLR_MTX(a,b)   5, a,b
#define IO_JOY(a,b)       6, ((a<<6)|b)

unsigned char io_joy1_up_n_fire[] = {
  IO_JOY(1,UP),        IO_WAIT(200),
  IO_JOY(1,DOWN),      IO_WAIT(200),
  IO_JOY(1,DOWN|FIRE), IO_WAIT(200),
  IO_JOY(1,NONE),      IO_DONE };

unsigned char io_mouse[] = {
  IO_JOY(0,LEFT),       IO_WAIT(1), IO_JOY(0,LEFT|RIGHT), IO_WAIT(1),
  IO_JOY(0,RIGHT),      IO_WAIT(1), IO_JOY(0,NONE),       IO_WAIT(1),
  IO_JOY(0,LEFT),       IO_WAIT(1), IO_JOY(0,LEFT|RIGHT), IO_WAIT(1),
  IO_JOY(0,RIGHT),      IO_WAIT(1), IO_JOY(0,NONE),       IO_WAIT(1),
  IO_JOY(0,LEFT),       IO_WAIT(1), IO_JOY(0,LEFT|RIGHT), IO_WAIT(1),
  IO_JOY(0,RIGHT),      IO_WAIT(1), IO_JOY(0,NONE),       IO_WAIT(1),
  IO_JOY(0,LEFT),       IO_WAIT(1), IO_JOY(0,LEFT|RIGHT), IO_WAIT(1),
  IO_JOY(0,RIGHT),      IO_WAIT(1), IO_JOY(0,NONE),       IO_WAIT(1),
  IO_DONE };

unsigned char io_press_key_shift_e[] = {
  IO_SET_MTX(5,1), IO_WAIT(100), IO_SET_MTX(4,5),
  IO_WAIT(100),
  IO_CLR_MTX(4,5), IO_WAIT(100), IO_CLR_MTX(5,1),
  IO_DONE };

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
  {   80, TTEXT, (unsigned char*)"Reset" },  
  {   80, TSER, cmd_reset },
  // Reply after reset ~65ms, docs indicate 300ms
#endif
#if 0 // regular operation
#define RUNTIME_MS 500
  {    1, TTEXT, (unsigned char*)"Key scan, pressing and releasing shift-e" },  
  {  100, TWIRE, io_press_key_shift_e },
  // reply: $2a, $12, $92, $aa
#endif
#if 0 // relative mouse
#define RUNTIME_MS 300
  {   80, TTEXT, (unsigned char*)"Test relative mouse movement" },
  {  100, TWIRE, io_mouse },
#endif
#if 0 // set/get time
#define RUNTIME_MS 2500
  {   80, TTEXT, (unsigned char*)"Set/get time" },  
  {   80,  TSER, cmd_set_time },
  {  100,  TSER, cmd_get_time },  // should return same time as set
  { 1100,  TSER, cmd_get_time },  // should return 1 sec advanced
  { 2100,  TSER, cmd_get_time },  // should return 2 secs advanced
#endif
#if 0 // interrogate mouse position
#define RUNTIME_MS 200
  {   80, TTEXT, (unsigned char*)"Interrogate mouse" },  
  {   80,  TSER, cmd_request_mouse_mode },
  // reply: mouse is in relative mode
  {  100,  TSER, cmd_set_absolute_mouse_positioning },
  {  120,  TSER, cmd_load_mouse },
  {  140,  TSER, cmd_request_mouse_mode },
  // reply: mouse is in absolute mode
  {  160,  TSER, cmd_interrogate_mouse_position },
  // reply: $f7,$00,$01,$40,$00,$c8, (320,200)
#endif
#if 1 // request joystick information
#define RUNTIME_MS 1000
  {   80, TTEXT, (unsigned char*)"Interrogate joystick" },
  {   80,  TSER, cmd_set_joystick_interrogation_mode },
  {   90,  TSER, cmd_interrogate_joystick },
  // reply: $fd,$00,$00
  {  100, TTEXT, (unsigned char*)"Joystick 1 up and fire" },
  {  100, TWIRE, io_joy1_up_n_fire },
  {  200,  TSER, cmd_interrogate_joystick },
  // reply: $fd,$00,$01
  {  400,  TSER, cmd_interrogate_joystick },
  // reply: $fd,$00,$02
  {  600,  TSER, cmd_interrogate_joystick },
  // reply: $fd,$80,$02
  {  800,  TSER, cmd_interrogate_joystick },
  // reply: $fd,$00,$00
#endif
#if 0 // joystick monitoring
#define RUNTIME_MS 1000
  {   80, TTEXT, (unsigned char*)"Test joystick monitoring" },
  {   80,  TSER, cmd_set_joystick_monitoring },
  {  200, TWIRE, io_joy1_up_n_fire },
#endif
#if 0 // fire button monitoring
#define RUNTIME_MS 110
  {   80, TTEXT, (unsigned char*)"Test fire button monitoring" },
  {   80,  TSER, cmd_set_fire_button_monitoring },
  {   90, TWIRE, io_joy1_fire_on_off },
#endif
  {    0,     0, NULL }
};

// serial signal generation
int st = 0;
unsigned char *sp = NULL;
int sc = 0;

// wire event generation
int wt = 0;
unsigned char *wp = NULL;
int wc = 0;
unsigned char matrix[] = {
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

// joystick and mouse inputs
unsigned char matrix_joy = 0xff;

// mirror of port states
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
  serial_do();
}

void wire_do() {
  static uint16_t col = 0xffff;

  // permanently monitor IO lines for column selection
  uint16_t tcol = 0x7fff ^ ((tb->po4 << 7) | (tb->po3 >> 1));
  if(tcol != col) {
    col = tcol;

    // do some sanity check. Only one column should be
    // selected at most. So only 16 values are valid    
    int b=0, column = -1;
    for(char i=0;i<15;i++) if(col & (1<<i)) { b++; column = i; }
    // printf("@%.2fµs col %04x %d\n", tickcount/1000.0, col, column);

    if(b > 1)
      printf("@%.2fµs ERROR, too many columns selected. %d!\n", tickcount/1000.0, b);

    if(column >= 0) P[0] = matrix[column];
    else            P[0] = 0xff;
    tb->pi1 = P[0];    
  }

#if 0
  // This code is for a version of the kbd using a 74LS03.
  
  // P20 goes to 74LS03 for mouse directions. Outputs of LS03 are only
  // low if both inputs (P20 and mouse signal) are 1
  unsigned char pi1 = P[0];
  if(tb->po2 & 1)
    pi1 &= ((~matrix_joy) | 0xf0);
  
  // joy2 directions always map (meaning that using the joystick
  // interferes with keyboard input
  // printf("JM %02x %02x\n", P[0], P[0] & (matrix_joy | 0x0f));
  P[0] &= (matrix_joy | 0x0f);
  tb->pi1 = pi1;
#else
  // This code is for a 74LS244 equipped keyboard
  P[3] = matrix_joy;
  tb->pi4 = P[3];
#endif
  
  while(wp && tickcount >= wt) {
    switch(wp[wc]) {
    case 1: {  // set port
      int port = wp[wc+1]>>4;
      int bit  = wp[wc+1]&15;
      printf("@%.2fµs SET(%d,%d)\n", tickcount/1000.0, port, bit);
      P[port-1] |= (1<<bit);    
      if(port == 1) tb->pi1 = P[0];
      if(port == 2) tb->pi2 = P[1];
      wc+=2;
    } break;
    case 2: {  // clear port
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
    case 4: {  // set matrix
      int row = wp[wc+1];
      int col = wp[wc+2];
      printf("@%.2fµs KEYDOWN(%d,%d)\n", tickcount/1000.0, row, col);
      matrix[col] &= ~(1<<row);
      wc+=3;
    } break;
    case 5: {  // clear matrix
      int row = wp[wc+1];
      int col = wp[wc+2];
      printf("@%.2fµs KEYUP(%d,%d)\n", tickcount/1000.0, row, col);
      matrix[col] |=  (1<<row);
      wc+=3;
    } break;
    case 6: {  // set jpystick
      int stick = wp[wc+1]>>6;
      int mask  = wp[wc+1]&0x3f;
      printf("@%.2fµs JOY(%d,0x%02x)\n", tickcount/1000.0, stick, mask);
      if(stick == 0) {
	// joystick 0 fire button is on P21
	if(mask & FIRE) P[1] &= ~(1<<1);
	else            P[1] |=  (1<<1);
	tb->pi2 = P[1];

	// map direction bits onto bottom four bits of matrix input
	matrix_joy = (matrix_joy & 0xf0) | (~mask & 0x0f);
      } else {
	// joystick 1 fire button is on P22
	if(mask & FIRE) P[1] &= ~(1<<2);
	else            P[1] |=  (1<<2);
	tb->pi2 = P[1];
	
	// map direction bits onto top four bits of matrix input
	matrix_joy = (matrix_joy & 0x0f) | ((~mask & 0x0f)<<4);	
      }
      wc+=2;
    } break;
      
    default:
      break;
    }
    
    // DONE ?  
    if(!wp[wc]) {
      // printf("@%.2fµs DONE\n", tickcount/1000.0);
      wp = NULL;
    }
  }
}
  
void wire_start(unsigned char *msg) {
  wc = 0;
  wp = msg;
  wt = 0;
  wire_do();
}

void tick(int c) {
  tb->clk = c;
  tb->eval();
  trace->dump(tickcount);
  tickcount += 250; // 2*250ns/cycle -> 2MHz, matching a real 6301@4MHz

  wire_do();
  serial_do();
  
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
  tb->pi3 = P[2];
  tb->pi4 = P[3];
  
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
