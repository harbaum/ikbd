# IKBD

This repository contains a verilog implementation of the HD6301
running the rom of the Atari ST ikbd within a verilator environment.
A reset command is being sent to the ikbd and it replies with $f1.

To use it enter ```make``` in the tb directory and watch the
```ikbd.vcd``` using e.g. gtkwave.

## Current state

The IKBD boots and passes RAM and ROM tests. It scans the key
matrix and sends the $f1 reset reply. It receives serial
commands and replies to them.

## Current work

The [HD63701](https://github.com/freecores/hd63701) does not implement
single chip mode (mode 7) which is the mode used by the IKBD. It was
thus lacking some of the ports to scan the matrix and the serial port.

These ports have been added and the IKBD can communicate over P2.3 and
P2.4 at 7812.5 bit/s.
