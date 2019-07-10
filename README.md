# IKBD

This repository contains a verilog implementation of the HD6301
running the rom of the Atari ST ikbd within a verilator environment.

To use it enter ```make``` in the tb directory and watch the
```ikbd.vcd``` using e.g. gtkwave.

## Current state

The IKBD boots and passes RAM and ROM tests. It also seems to scan the
key matrix.

## ToDo

The [https://github.com/freecores/hd63701](HD63701) does not implement
single chip mode (mode 7) which is the mode used by the IKBD. It's
thus lacking some of the ports to scan the matrix and it's lacking the
serial port.
