/*
 * Lattice Panel: WELL512 PRNG
 *
 * Public domain WELL512 by Chris Lomont
 *
 * http://lomont.org/papers/2008/Lomont_PRNG_2008.pdf
 */
#include "stm32f3xx.h"
#include "display.h"

static uint32_t state[16U];
static uint32_t index = 0U;

void wellrng512_init(void)
{
	/* seed prng with jitter from RTC and display progress */
	uint32_t prev = RTC->SSR;
	uint32_t i = 0UL;
	while(i < 16U) {
		volatile uint32_t tmp;
		uint32_t j = 0U;
		uint32_t v = 0U;
		while (j < 32U) {
			wait_for_interrupt();
			tmp = RTC->SSR;
			v = (v<<1U) | ((tmp - prev)&0x1U);
			prev = tmp;
			dbuf.line[j&0x7] = tmp;
			dbuf.line[i+8U] = v;
			display_transfer();
			display_latch();
			j++;
		}
		state[i] = v;
		i++;
	}
}

uint32_t wellrng512(void)
{
	uint32_t a, b, c, d;
	a = state[index];
	c = state[(index + 13) & 15];
	b = a ^ c ^ (a << 16) ^ (c << 15);
	c = state[(index + 9) & 15];
	c ^= (c >> 11);
	a = state[index] = b ^ c;
	d = a ^ ((a << 5) & 0xDA442D24UL);
	index = (index + 15) & 15;
	a = state[index];
	state[index] = a ^ b ^ d ^ (a << 2) ^ (b << 18) ^ (c << 28);
	return state[index];
}
