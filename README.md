# Lattice Panel

A small Game of Life display for M20 LED panels.

## Files

	ltpnl.zip	Kicad sch/pcb
	mic.zip		Example sch for microphone trigger
	Makefile	GNU Makefile for firmware

## Pre-requisites

- GNU Make: build-essential

- GCC cross compiler for ARM Cortex-R/M processors: gcc-arm-none-eabi

- GNU assembler, linker and binary utilities for ARM Cortex-R/M processors:
  binutils-arm-none-eabi

- GNU Debugger (with support for multiple architectures): gdb-multiarch

- Open on-chip JTAG debug solution for ARM and MIPS systems: openocd

- Optional: gpg2 (to sign binary)

Install using apt-get:

	# apt-get install build-essential gcc-arm-none-eabi binutils-arm-none-eabi
	# apt-get install gdb-multiarch openocd gnupg2

## Usage

Edit Makefile, connect stlink/v2 programmer to SWD header, build,
then upload image to flash:

	$ make upload

Alternate make targets can be listed with make help:

	$ make help
	
	Targets:
	 elf [default]	build all objects, link and write ltpnl.elf
	 bin		create flash loader binary ltpnl_20201210.bin
	 sign		sign ltpnl_20201210.bin using gpg
	 size		list ltpnl.elf section sizes
	 nm		list all defined symbols in ltpnl.elf
	 list		create text listing for ltpnl.elf
	 ocd		launch openocd on target in foreground
	 debug		debug ltpnl.elf on target
	 erase		bulk erase flash on target
	 upload		write ltpnl_20201210.bin to flash and verify
	 clean		remove all intermediate files and logs

## M20 Connector (J1)

	Pin		Fuction
	2		TOP	Data line for top register
	4		BOT	Data line for bottom register
	6		CLK	Register clock
	8		LCK	Latch display LEDs
	10		DIN	Display inhibit (dimmer)
	12		N/C
	14		PCK	Panel select clock
	16		N/C
	18		N/C
	20		SEL	Panel select data
	22,23,25	VCC	7.5V - 9V
	1,3,5,7,9,11	GND
	13,15,24,26	GND

## SWD Connector (J2)

	Pin	Function	ST-LINK/V2 Pin
	1	MCU VDD		1
	2	SWCLK		9
	3	GND		8
	4	SWDIO		7
	5	NRST		15
	6	TRACESWO	13 (unused)

## Trigger Connector (J3)

	Pin	Function
	1	TRG (Active low input)
	2	N/C
	3	3V3
	4	GND
