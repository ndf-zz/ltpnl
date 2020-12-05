/*
 * Lattice Panel: "Simulation" data structure and definitions
 */
#ifndef _SIM_H
#define _SIM_H
#include <stdint.h>

typedef void (*void_function)(void);
struct sim {
	void_function trigger;
	void_function update;
	volatile uint32_t state;
	uint32_t config;
};

#endif /* _SIM_H */
