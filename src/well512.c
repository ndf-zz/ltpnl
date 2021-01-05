/*
 * Lattice Panel: WELL512 PRNG
 *
 * Public domain WELL512 by Chris Lomont, with RTC jitter init
 *
 * http://lomont.org/papers/2008/Lomont_PRNG_2008.pdf
 */
#include "stm32f3xx.h"
#include "display.h"

static uint32_t state[16U];
static uint32_t index = 0U;

/* wait for the next sample time */
void wait_sample(void)
{
	static uint32_t lt = 0U;
	do {
		wait_for_interrupt();
	} while (lt == Uptime);
	lt = Uptime;
}

/* fetch the next random bit by sampling the RTC */
static uint32_t next_bit(void)
{
	static uint32_t lr = 0U;
	uint32_t ret = 0U;
	uint32_t tmp; /* volatile ? */
	do {
		wait_sample();
		tmp = RTC->SSR;
		ret = ((lr - tmp)&0x1U);
		dbuf.line[0] = tmp;
		dbuf.line[2] = lr-tmp;
		lr = tmp;
		barrier();
		display_transfer();
		display_latch();
		barrier();
		wait_sample();
		tmp = RTC->SSR;
		ret <<= 1U;
		ret |= ((lr - tmp)&0x1U);
		dbuf.line[1] = tmp;
		dbuf.line[3] = lr-tmp;
		lr = tmp;
		if (ret == 0x2U) {
			ret = 0x1U;
			tmp = 0x0U;
		} else if (ret == 0x1U) {
			ret = 0x0U;
			tmp = 0x0U;
		} else {
			tmp = 0x1U;
		}
		dbuf.line[4] = ret;
		barrier();
		display_transfer();
		display_latch();
	} while(tmp);
	return ret;
}

/* fetch the next word worth of random data */
static uint32_t next_word(void)
{
	uint32_t ret = 0U;
	uint32_t j = 0U;
	while (j < 32) {
		ret <<= 1;
		ret |= next_bit();
		dbuf.line[5] = ret&0xfffU;
		dbuf.line[6] = (ret>>12)&0xfffU;
		dbuf.line[7] = (ret>>24)&0xfffU;
		j++;
	}
	return ret;
}

/* seed the prng with jitter from RTC - note this function
 * displays progress on the panel and sleeps on the systick
 * interrupt.
 */
void wellrng512_init(void)
{
	uint32_t i = 0UL;
	do {
		state[i] = next_word();
		dbuf.line[i+8] = state[i];
		i++;
	} while (i < 16U);
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
