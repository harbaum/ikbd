OBJ_DIR=obj_dir
VERILATOR_DIR=/usr/share/verilator/include
HDL_FILES = ../hd63701/HD63701.v ../hd63701/HD63701_CORE.v 

all: ikbd.vcd

# hexdump -C ../rom/IKBD.ROM | cut -c 11-58 > ikbd.hex

ikbd.vcd: ikbd_tb
	./ikbd_tb

Vikbd_tb.cpp: ${OBJ_DIR}/Vikbd_tb.cpp


${OBJ_DIR}/Vikbd_tb.cpp: ikbd_tb.v ${HDL_FILES}
	verilator -Wno-fatal --trace --top-module ikbd_tb -I../hd63701 -cc ikbd_tb.v ${HDL_FILES}

ikbd_tb: ${OBJ_DIR}/Vikbd_tb.cpp ikbd_tb.cpp
	g++ -I $(OBJ_DIR) -I$(VERILATOR_DIR) $(VERILATOR_DIR)/verilated.cpp $(VERILATOR_DIR)/verilated_vcd_c.cpp ikbd_tb.cpp  $(OBJ_DIR)/Vikbd_tb__Trace.cpp $(OBJ_DIR)/Vikbd_tb__Trace__Slow.cpp $(OBJ_DIR)/Vikbd_tb.cpp $(OBJ_DIR)/Vikbd_tb__Syms.cpp -DOPT=-DVL_DEBUG -o ikbd_tb
