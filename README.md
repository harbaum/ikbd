# IKBD

This repository contains a verilog implementation of the HD6301
running the rom of the Atari ST ikbd within a verilator environment.
This is based on tje [HD63701](https://github.com/thasega/HD63701)
project.

The rom being used is from later keyboard revisions which
use a different approach to select mouse and joystick signals.

To use it enter ```make``` in the tb directory and watch the
```ikbd.vcd``` using e.g. gtkwave. Furthermore some events will
be reported on the console:

```
./ikbd_tb
@2.50µs out of reset
@65218.25µs RX f1
@80000.00µs Test fire button monitoring
@80000.00µs TX 18
@84546.25µs RX 00
@86338.25µs RX 00
@88130.25µs RX 00
@89794.25µs RX 00
@90000.00µs CLR(2,2)
@91586.25µs RX 03
@93378.25µs RX ff
@95170.25µs RX ff
@96706.25µs RX ff
@98498.25µs RX ff
@100001.00µs SET(2,2)
@100290.25µs RX ff
@102082.25µs RX f0
@103746.25µs RX 00
@105538.25µs RX 00
@107330.25µs RX 00
@109250.25µs RX 00

```

In this case the IKBD sends its default boot message $f1 after
~65ms. Then the testbench sends the $18 command (set fire button
monitoring) to the IKBD which in turn starts to send button
reports. At 90ms the testbench "presses fire" which results in
the appropriate bit changes in the IKBDs report. At 100ms the
fire button is released again.

More test patterns can be selected in ```tb/ikbd_tb.cpp```.

## Current state

The IKBD boots and passes RAM and ROM tests. It scans the key
matrix and sends the $f1 reset reply.

Most commands have been tested and keyboard, mouse and joysticks
work. The internal clock also runs.

The TST and EIM instructions were broken and have been
fixed in the HD6301 core and some of the internal peripherals
like the SCI serial interface have been implemented.

## Current work

A PS/2 interface to connect to real ps2 keyboards or the
ps2 emulation internally used by the MIST.
