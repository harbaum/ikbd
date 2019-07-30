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
@65218.25µs IKBD RX f1 => BOOT OK V1 or KEY RELEASE KEYPAD-.
@80000.00µs Test relative mouse movement
@80000.00µs Should end at X:-10, Y:10
@100000.00µs PS2 TX 09
@100918.50µs PS2 TX 0a
@101837.00µs PS2 TX 14
@105666.25µs IKBD RX f8 => MOUSE REL L:off R:off
@106946.25µs IKBD RX 01 => MOUSE X 1=1
@108226.25µs IKBD RX 00 => MOUSE Y 0=0
@109506.25µs IKBD RX f8 => MOUSE REL L:off R:off
@110786.25µs IKBD RX 01 => MOUSE X 1=2
@112066.25µs IKBD RX 01 => MOUSE Y 1=1
@113346.25µs IKBD RX f8 => MOUSE REL L:off R:off
@114626.25µs IKBD RX 04 => MOUSE X 4=6
@115906.25µs IKBD RX 04 => MOUSE Y 4=5
@117186.25µs IKBD RX f9 => MOUSE REL L:on R:off
@118466.25µs IKBD RX 01 => MOUSE X 1=7
@119746.25µs IKBD RX 01 => MOUSE Y 1=6
@121026.25µs IKBD RX f9 => MOUSE REL L:on R:off
@122306.25µs IKBD RX 02 => MOUSE X 2=9
@123586.25µs IKBD RX 07 => MOUSE Y 7=13
@124866.25µs IKBD RX f9 => MOUSE REL L:on R:off
@126146.25µs IKBD RX 00 => MOUSE X 0=9
@127426.25µs IKBD RX 04 => MOUSE Y 4=17
@128706.25µs IKBD RX f9 => MOUSE REL L:on R:off
@129986.25µs IKBD RX 00 => MOUSE X 0=9
@131266.25µs IKBD RX 02 => MOUSE Y 2=19

```

In this case the IKBD sends its default boot message $f1 after
~65ms. Then the testbench sends a PS2 mouse movement event into
tke IKBD and replies with a set of relative mouse movement events.

More test patterns can be selected in ```tb/ikbd_tb.cpp```.

## Current state

The IKBD seems to be working completely. A ps2 keyboard and mouse
can be parsed into input states for the hd6301. A single joystick
is also supported on port 1.

## ToDo

Add the ability to switch between the mouse and a second joystick
on ikbd port 0.
