/*
  IKBD/HD6301 verilator testbench

  (c) 2019 by Till Harbaum <till@harbaum.org>
*/

#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include "Vikbd.h"
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
unsigned char cmd_set_relative_mouse_positioning[] = { 1, 0x08 };
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
#define TJOY  1
#define TTEXT 2
#define TPS2K 3
#define TPS2M 4

#define IO_DONE           0
#define IO_JOY(a)         1, a
#define IO_WAIT(a)        2, a

unsigned char io_joy_up_n_fire[] = {
  IO_JOY(UP),        IO_WAIT(200),
  IO_JOY(DOWN),      IO_WAIT(200),
  IO_JOY(DOWN|FIRE), IO_WAIT(200),
  IO_JOY(NONE),      IO_DONE };

unsigned char io_joy_fire_on_off[] = {
  IO_JOY(FIRE),      IO_WAIT(10),
  IO_JOY(NONE),      IO_DONE };

#define BL 0x01
#define BR 0x02
#define BM 0x04
#define PS2_PAUSE(n)   0xff, n
#define PS2_DONE       0x00
#define PS2_MOUSE(b,x,y)  ((y&0x100)?0x20:0x00)|((x&0x100)?0x10:0x00)|0x08|b,x&0xff,y&0xff

unsigned char io_ps2_press_and_release_shift_e[] = {
  0x12,       PS2_PAUSE(50),    // press shift
  0x24,       PS2_PAUSE(50),    // press e
  0xf0, 0x24, PS2_PAUSE(50),    // release e
  0xf0, 0x12, PS2_DONE          // release shift
};
  
unsigned char io_ps2_caps_lock_n_up[] = {
  0x58, 0xe0, 0x75, PS2_PAUSE(100), 0xf0, 0x58, 0xe0, 0xf0, 0x75, PS2_DONE };

unsigned char io_ps2_mouse_up_right[] = { PS2_MOUSE(BL,10,20), PS2_DONE };
unsigned char io_ps2_mouse_down_left[] = { PS2_MOUSE(0,-20,-10), PS2_DONE };
unsigned char io_ps2_mouse_button_right[] = { PS2_MOUSE(BR,0,0), PS2_DONE };

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
#define RUNTIME_MS 300
  {    1, TTEXT, (unsigned char*)"Key scan, pressing and releasing shift-e" },  
  {  100, TPS2K, io_ps2_press_and_release_shift_e },
#endif
#if 0 // caps lock and ps2 extended codes test
#define RUNTIME_MS 500
  {    1, TTEXT, (unsigned char*)"Key scan, pressing and releasing capslock and cursor up" },  
  {  200, TPS2K, io_ps2_caps_lock_n_up },
#endif
#if 0 // relative mouse
#define RUNTIME_MS 400
  {   80, TTEXT, (unsigned char*)"Test relative mouse movement" },
  {   80, TTEXT, (unsigned char*)"Should end at X:-10, Y:10" },
  {  100, TPS2M, io_ps2_mouse_up_right },
  {  250, TPS2M, io_ps2_mouse_down_left },
  // should end at x:-10,y:10
#endif
#if 1 // relative mouse
#define RUNTIME_MS 350
  {   80, TTEXT, (unsigned char*)"Test relative mouse movement" },
  {   80, TTEXT, (unsigned char*)"Should end at X:-10, Y:10" },
  {   90, TSER, cmd_set_relative_mouse_positioning },
  {  100, TPS2M, io_ps2_mouse_up_right },
  {  200, TPS2M, io_ps2_mouse_button_right },
  {  250, TPS2M, io_ps2_mouse_down_left },
  {  300, TPS2M, io_ps2_mouse_button_right },
  // should end at x:-10,y:10
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
#endif
#if 0 // request joystick information
#define RUNTIME_MS 1000
  {   80, TTEXT, (unsigned char*)"Interrogate joystick" },
  {   80,  TSER, cmd_set_joystick_interrogation_mode },
  {   90,  TSER, cmd_interrogate_joystick },
  // reply: $fd,$00,$00
  {  100, TTEXT, (unsigned char*)"Joystick 1 up and fire" },
  {  100,  TJOY, io_joy_up_n_fire },
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
  {   80,  TSER, NULL },  // disable ikbd output parsing
  {  200,  TJOY, io_joy_up_n_fire },
#endif
#if 0 // fire button monitoring
#define RUNTIME_MS 110
  {   80, TTEXT, (unsigned char*)"Test fire button monitoring" },
  {   80,  TSER, NULL },  // disable ikbd output parsing
  {   80,  TSER, cmd_set_fire_button_monitoring },
  {   90,  TJOY, io_joy_fire_on_off },
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

// ps2 event generation
int pd = 0;
int pt = 0;
unsigned char *pp = NULL;
int pc = 0;

static Vikbd *tb;
static VerilatedVcdC *trace;
static int tickcount;

// ticks per uart bit. ticks come for both clock edges ->
// 4M ticks @ 2Mhz
#define GAP  1   // extra pause between bytes in bits
#define TPB  (int)(250*2000000*2/7812.5)

// by default the testbench parses ikbd replies
static int ikbd_parse = 1;

void serial_do() {
  static int rxsr = 0;
  static int rxcnt = 0;
  static int rxt = 0;
  static int rx_mouse_x = 0;
  static int rx_mouse_y = 0;

  // check for incoming data on po
  if(!tb->tx && !rxcnt) {
    // make sure we sample in the middle of the bit
    rxt = tickcount - TPB/2;
    rxcnt = 10;
    rxsr = 0;
  }

  // rx in progess 
  if(rxcnt) {
    if((tickcount - rxt) == TPB) {
      static int rx_state = 0;
      static int rx_msg_cnt = 0;

      if(rxcnt > 1) {
	rxsr >>= 1;
	if(tb->tx)
	  rxsr |= 0x80;
      } else if(tb->tx) {
	char desc[100] = "";
	if(ikbd_parse) {	
	  if(rx_msg_cnt == 0) {
	    switch(rxsr) {
	    case 0xf0:
	      sprintf(desc, " => BOOT OK V0 or KEY RELEASE KEYPAD-0");
	      break;
	    case 0xf1:
	      sprintf(desc, " => BOOT OK V1 or KEY RELEASE KEYPAD-.");
	      break;
	    case 0xf6:
	      sprintf(desc, " => STATUS REPLY");
	      rx_state = 0xf6; rx_msg_cnt = 7;
	      break;
	    case 0xf7:
	      sprintf(desc, " => MOUSE ABS");
	      rx_state = 0xf7; rx_msg_cnt = 5;
	      break;
	    case 0xf8:
	    case 0xf9:
	    case 0xfa:
	    case 0xfb:
	      rx_state = 0xf8; rx_msg_cnt = 2;
	      sprintf(desc, " => MOUSE REL L:%s R:%s", rxsr&1?"on":"off",
		      rxsr&2?"on":"off");
	      break;
	    case 0xfc:
	      sprintf(desc, " => TIME OF DAY");
	      rx_state = 0xfc; rx_msg_cnt = 6;
	      break;
	    case 0xfd:
	      sprintf(desc, " => JOYSTICK");
	      rx_state = 0xfd; rx_msg_cnt = 2;
	      break;
	    case 0xfe:
	      sprintf(desc, " => JOYSTICK 0");
	      rx_state = 0xfe; rx_msg_cnt = 1;
	      break;
	    case 0xff:
	      sprintf(desc, " => JOYSTICK 1");
	      rx_state = 0xff; rx_msg_cnt = 1;
	      break;
	    default:
	      if(rxsr & 0x80) sprintf(desc, " => KEY RELEASE(%02x)", rxsr & 0x7f);
	      else            sprintf(desc, " => KEY PRESS(%02x)", rxsr & 0x7f);
	    }
	  } else {
	    switch(rx_state) {
	    case 0xf6:  // status report
	      sprintf(desc, " => ST(%d)=%02x", 7-rx_msg_cnt, rxsr);
	      break;	    
	    case 0xf7:  // absolute mouse report
	      switch(rx_msg_cnt) {
	      case 5:
		sprintf(desc, " => MOUSE BTN %1x", rxsr % 0x0f);
		break;
	      case 4:
		rx_mouse_x = rxsr;
		break;
	      case 3:
		rx_mouse_x = rx_mouse_x * 256 + rxsr;
		sprintf(desc, " => MOUSE X=%d", rx_mouse_x);
		break;
	      case 2:
		rx_mouse_y = rxsr;
		break;
	      case 1:
		rx_mouse_y = rx_mouse_y * 256 + rxsr;
		sprintf(desc, " => MOUSE Y=%d", rx_mouse_y);
		break;
	      }
	      break;
	    case 0xf8:  // relative mouse report
	      if(rx_msg_cnt == 2) {
		rx_mouse_x += (char)rxsr;
		sprintf(desc, " => MOUSE X %d=%d", (char)rxsr, rx_mouse_x);
	      }
	      if(rx_msg_cnt == 1) {
		rx_mouse_y += (char)rxsr;
		sprintf(desc, " => MOUSE Y %d=%d", (char)rxsr, rx_mouse_y);
	      }
	      break;
	    case 0xfc: {  // time of day
	      char name[][6] = { "SEC", "MIN", "HOUR", "DAY", "MONTH", "YEAR" };
	      sprintf(desc, " => %s %02x", name[rx_msg_cnt-1], rxsr);
	    } break;	    
	    case 0xfd: // joystick
	      sprintf(desc, " => JOY(%d), F:%s D:%1x", 2-rx_msg_cnt, (rxsr&0x80)?"on ":"off",rxsr&0xf);
	      break;
	      
	    default:
	      break;
	    }
	    
	    rx_msg_cnt--;
	  }
	}
	printf("@%.2fµs IKBD RX %02x%s\n", tickcount/1000.0, rxsr, desc);
      }
	
      rxt = tickcount;
      rxcnt--;
    }    
  }

  if(!sp) return;

  if(tickcount - st < TPB) return;

  int bitn = sc%(10+GAP);
  int byten = sc/(10+GAP);
  int bit = 1, n='G';
  if(bitn == 0) {
    bit = 0;
    n = 'S';
    printf("@%.2fµs IKBD TX %02x\n", tickcount/1000.0, sp[byten+1]);
  } else if(bitn < 9) {
    bit = (sp[byten+1]&(1<<(bitn-1)))?1:0;
    n = '0'+(bitn-1);
  }

  // drive ikbds rx line
  tb->rx = bit;

  sc++;
  st = tickcount;
  if(sc == (10+GAP)*sp[0])  // 10 bits per byte incl start, stop and GAP
    sp = NULL;
}

void serial_start(unsigned char *msg) {
  // this test sets the ikbd in a special report mode where the
  // reply should not be parsed as usual
  if(msg == 0) {
    ikbd_parse = 0;
    return;
  }

  sc = 0;
  sp = msg;
  st = 0;
  serial_do();
}

void joystick_do() {
  static uint16_t col = 0xffff;
    
  while(wp && tickcount >= wt) {
    switch(wp[wc]) {
    case 1: {  // set jpystick
      printf("@%.2fµs JOY(%02x)\n", tickcount/1000.0, wp[wc+1]);
      tb->joystick = wp[wc+1];
      wc+=2;
    } break;
    case 2:  // wait
      // printf("@%.2fµs WAIT(%dms)\n", tickcount/1000.0, wp[wc+1]);
      wt = tickcount + 1000000*wp[wc+1];   // X us
      wc+=2;
      break;
    default:
      printf("HUH???\n");
      break;
    }
    
    // DONE ?  
    if(!wp[wc])
      wp = NULL;
  }
}

#define PS2_CLK   12000    // 12khz
#define PS2_NS    (1000000000/PS2_CLK/2)

void ps2_do() {
  static int caps = 1;

  if(tb->caps_lock != caps) {
    caps = tb->caps_lock;
    printf("@%.2fµs CAPS LOCK changed to %d!\n", tickcount/1000.0, caps);
  }
  
  if(pp && tickcount >= pt) {
    // set data on rising edge    
    if(pd == 0) tb->ps2_kbd_clk = !tb->ps2_kbd_clk;
    else        tb->ps2_mouse_clk = !tb->ps2_mouse_clk;

    if((pd == 0 && tb->ps2_kbd_clk) || (pd == 1 && tb->ps2_mouse_clk)) {
      pc++;
      
      int bit = pc%11;    // start, 8 data, parity, stop
      int byte = pc/11;
      int b = pp[byte];
      int par = 1;
      for(char i=0;i<8;i++)
	if(b&(1<<i)) par = !par;

      if(!b) {
	pp = NULL;
	return;
      }

      if(bit == 0) {	
	if(b == PS2_DONE) {
	  pp = NULL;
	  return;
	}
	printf("@%.2fµs PS2 TX %02x\n", tickcount/1000.0, b);
      }

      if(pd == 0) {      
	if(bit == 0)             tb->ps2_kbd_data = 0;    // start
	if(bit >= 1 && bit <= 8) tb->ps2_kbd_data = (b&(1<<(bit-1)))?1:0;
	if(bit == 9)             tb->ps2_kbd_data = par;  // parity
	if(bit == 10)            tb->ps2_kbd_data = 1;    // stop
      } else {
	if(bit == 0)             tb->ps2_mouse_data = 0;    // start
	if(bit >= 1 && bit <= 8) tb->ps2_mouse_data = (b&(1<<(bit-1)))?1:0;
	if(bit == 9)             tb->ps2_mouse_data = par;  // parity
	if(bit == 10)            tb->ps2_mouse_data = 1;    // stop
      }
    }
    
    pt = tickcount + PS2_NS;

    // check if next is a "pause" and extend pause accordingly
    // TODO: Pause is not 100% correct. Clock stays low until begin of next byte
    if(((pd == 0 && !tb->ps2_kbd_clk)||(pd == 1 && !tb->ps2_mouse_clk)) && (pc%11 == 10)) {      
      if(pp[pc/11+1] == 0xff) {
	// PS2_PAUSE
	pt = tickcount + PS2_NS + 1000000 * pp[pc/11+2];
	// skip next two byes
	pc += 22;
      }
    }	    
  }
}

void joystick_start(unsigned char *msg) {
  wc = 0;
  wp = msg;
  wt = 0;
  joystick_do();
}

void ps2_start(char dev, unsigned char *msg) {
  pd = dev;
  pc = 0;
  pp = msg;
  
  printf("@%.2fµs PS2 TX %02x\n", tickcount/1000.0, pp[0]);
  if(pd == 0) tb->ps2_kbd_data = 0;     // start bit
  else        tb->ps2_mouse_data = 0;     // start bit
  pt = tickcount + PS2_NS;
  ps2_do();
}

void tick(int c) {
  tb->clk = c;
  tb->eval();
  trace->dump(tickcount);
  tickcount += 250; // 2*250ns/cycle -> 2MHz, matching a real 6301@4MHz

  joystick_do();
  serial_do();
  ps2_do();
  
  // check if an event is supposed to start
  for(char i=0;events[i].time;i++) {
    if(events[i].time * 1000000 == tickcount) {
      if(events[i].type == TSER)
	serial_start(events[i].cmd);
      if(events[i].type == TJOY)
	joystick_start(events[i].cmd);
      if(events[i].type == TTEXT)
	printf("@%.2fµs %s\n", tickcount/1000.0, events[i].cmd);
      if(events[i].type == TPS2K)
	ps2_start(0, events[i].cmd);
      if(events[i].type == TPS2M)
	ps2_start(1, events[i].cmd);
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
  tb = new Vikbd;
  tb->trace(trace, 99);
  trace->open("ikbd.vcd");

  // init all signals
  tb->res = 1;

  // init ps2 keyboard
  tb->ps2_kbd_clk = 1;
  tb->ps2_kbd_data = 1;
  
  // init ps2 mouse
  tb->ps2_mouse_clk = 1;
  tb->ps2_mouse_data = 1;

  // init ikbd uart
  tb->rx = 1;
  
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
