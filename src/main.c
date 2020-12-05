/*
 * Lattice Panel: Main loop
 */
#include "stm32f3xx.h"
#include "display.h"
#include "well512.h"
#include "mca.h"

#define BVMAX 32U
#define BVUP 2U
#define BVDOWN 1U

void main(void)
{
	uint32_t bv = BVMAX;
	uint32_t lbv = BVMAX;
	uint32_t lt = 0U;
	display_init(bv);
	wellrng512_init();
	struct sim *g = gol_init();
	do {
		wait_for_interrupt();
		if (Uptime != lt) {
			lt = Uptime;

			/* blit */
			display_latch();

			/* fade */
			if (g->state) 
				bv = bv < BVMAX ? bv + BVUP : BVMAX;
			else
				bv = bv ? bv - BVDOWN : 0U;
			if (bv != lbv) {
				display_brightness(bv);
				lbv = bv;
			}

			/* trigger */
			if ((GPIOC->IDR & 0x1U) == 0)
				g->trigger();

			/* update */
			g->update();

			/* transfer */
			display_transfer();
		}
	} while(1);
}
