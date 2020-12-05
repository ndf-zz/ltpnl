/*
 * Lattice Panel: Moore Neighbourhood 2D Cellular Automata
 */
#include "stm32f3xx.h"
#include "display.h"
#include "sim.h"
#include "well512.h"

#define MAXCOUNT (1<<13)

struct sim gol_sim;
struct display_buf tmp;
uint32_t cnt_lut[] = {0, 1, 1, 2, 1, 2, 2, 3};
uint32_t pv[3];

uint32_t alive_dead(uint32_t count, uint32_t self)
{
	uint32_t alive;
	switch (count) {
	case 3:
		alive = 1;
		break;
	case 4:
		alive = self;
		break;
	default:
		alive = 0;
		break;
	}
	return alive;
}

uint32_t calc_line(uint32_t *p, uint32_t *q, uint32_t *r)
{
	uint32_t res = 0;
	uint32_t chk;
	uint32_t sum;

	/* msb */
	chk =  (((*p)<<2)&0x4)|(((*p)>>30)&0x3);
	sum = cnt_lut[chk];
	chk =  (((*q)<<2)&0x4)|(((*q)>>30)&0x3);
	sum += cnt_lut[chk];
	chk =  (((*r)<<2)&0x4)|(((*r)>>30)&0x3);
	sum += cnt_lut[chk];
	res |= alive_dead(sum, ((*q)>>31)&1);

	/* intermediates */
	uint32_t i = 1;
	do {
		res <<= 1;
		chk = ((*p)>>(30-i))&0x7;
		sum = cnt_lut[chk];
		chk = ((*q)>>(30-i))&0x7;
		sum += cnt_lut[chk];
		chk = ((*r)>>(30-i))&0x7;
		sum += cnt_lut[chk];
		res |= alive_dead(sum, ((*q)>>(31-i))&1);
		i++;
	} while (i < 31);

	/* lsb */
	res <<= 1;
	chk = (((*p)<<1)|((*p)>>31))&0x7;
	sum = cnt_lut[chk];
	chk = (((*q)<<1)|((*q)>>31))&0x7;
	sum += cnt_lut[chk];
	chk = (((*r)<<1)|((*r)>>31))&0x7;
	sum += cnt_lut[chk];
	res |= alive_dead(sum, (*q)&1);

	return res;
}

void gol_update(void)
{
	uint32_t *p;
	uint32_t *q;
	uint32_t *r;

	CRC->CR |= CRC_CR_RESET;

	/* first row */
	tmp.line[23] = dbuf.line[23];
	p = &tmp.line[23];
	tmp.line[0] = dbuf.line[0];
	q = &tmp.line[0];
	tmp.line[1] = dbuf.line[1];
	r = &tmp.line[1];
	dbuf.line[0] = calc_line(p, q, r);
	CRC->DR = dbuf.line[0];

	/* intermediates */
	uint32_t i = 1;
	do {
		tmp.line[i+1] = dbuf.line[i + 1];
		p = &tmp.line[i-1];
		q = &tmp.line[i];
		r = &tmp.line[i+1];
		dbuf.line[i] = calc_line(p, q, r);
		CRC->DR = dbuf.line[i];
		i++;
	} while (i < 23);

	/* last row */
	p = &tmp.line[22];
	q = &tmp.line[23];
	r = &tmp.line[0];
	dbuf.line[23] = calc_line(p, q, r);
	CRC->DR = dbuf.line[23];
	/* Check for end of evolution */
	if (pv[0] == CRC->DR ||
		(CRC->DR == pv[1] && pv[0] == pv[2]))
		gol_sim.state = 0;
	else
		gol_sim.state = gol_sim.state ? gol_sim.state - 1U : 0U;
	pv[2] = pv[1];
	pv[1] = pv[0];
	pv[0] = CRC->DR;
}

void gol_trigger(void)
{
	gol_sim.state = MAXCOUNT;
	uint32_t tv = wellrng512();
	dbuf.line[8] |= (tv<<2) & 0x3fcU;
        dbuf.line[9] |= (tv>>6) & 0x3fcU;
	dbuf.line[10] |= (tv>>14) & 0x3fcU;
	dbuf.line[11] |= (tv>>22) & 0x3fcU;
	tv = wellrng512();
	dbuf.line[12] |= (tv<<2) & 0x3fcU;
	dbuf.line[13] |= (tv>>6) & 0x3fcU;
	dbuf.line[14] |= (tv>>14) & 0x3fcU;
	dbuf.line[15] |= (tv>>22) & 0x3fcU;
}

struct sim *gol_init(void)
{
	gol_sim.trigger = gol_trigger;
	gol_sim.update = gol_update;
	gol_sim.state = MAXCOUNT;
	return &gol_sim;
}

