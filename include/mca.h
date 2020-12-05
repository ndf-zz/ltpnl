/*
 * Lattice Panel: Moore Neighbourhood 2D Cellular Automata
 */
#ifndef MCA_H
#define MCA_H
#include "sim.h"

/* Conway's Game of Life (B3/S23) */
struct sim *gol_init(void);

/* Highlife (B36/S23) */
struct sim *hil_init(void);

#endif /* MCA_H */
