/*
 * Lattice Panel: LED display interface
 *
 * Output ports are configured in system_init()
 *
 * Control Lines:
 *
 * DIN	PC6	Display Inhibit - driven by PWM for dimmer
 * TOP	PC7	Data for panel top shift reg
 * BOT	PC8	Data for panel bottom shift reg
 * CLK	PC9	Panel shift register clock
 * LCH	PC10	Panel shift register latch control
 * PCK	PC11	Panel select clock
 * SEL	PC12	Panel select control line
 */
#include "stm32f3xx.h"
#include "display.h"

#define DISPLAY_DIN	GPIO_ODR_6	/* Brown */
#define DISPLAY_TOP	GPIO_ODR_7	/* Orange */
#define DISPLAY_BOT	GPIO_ODR_8	/* Green */
#define DISPLAY_CLK	GPIO_ODR_9	/* White */
#define DISPLAY_LCH	GPIO_ODR_10	/* Yellow */
#define DISPLAY_PCK	GPIO_ODR_11	/* Blue */
#define DISPLAY_SEL	GPIO_ODR_12	/* Purple */
#define DMASK		(DISPLAY_TOP | DISPLAY_BOT)

struct display_buf dbuf;

/* Select the first card in the chain */
void card_first(void)
{
	GPIOC->BSRR = DISPLAY_SEL<<16U;
	GPIOC->BSRR = DISPLAY_PCK;
	GPIOC->BSRR = DISPLAY_PCK<<16U;
}

/* Advance to the next card in the chain */
void card_next(void)
{
	GPIOC->BSRR = DISPLAY_SEL;
	GPIOC->BSRR = DISPLAY_PCK;
	GPIOC->BSRR = DISPLAY_PCK<<16U;
}

/* shift out current top and bottom nibble */
void shift_nibble(uint32_t top, uint32_t bot)
{
	uint32_t cnt = 4;
	uint32_t tmp;
	do {
		tmp = (top & DISPLAY_TOP) | (bot & DISPLAY_BOT);
		tmp |= ((~tmp) & DMASK) << 16U;
		GPIOC->BSRR = tmp;		/* set display data bits */
		top<<=1;
		GPIOC->BSRR = DISPLAY_CLK;	/* toggle clock line */
		bot<<=1;
		GPIOC->BSRR = DISPLAY_CLK<<16U;
		cnt--;
	} while (cnt);
}

/* Shift the relevant line data into display panel */
void card_send(uint32_t oft)
{
	uint32_t t;
	uint32_t b;
	uint32_t l = 0;
	/* Columns 0 - 3 */
	do {
		t = dbuf.line[oft+l] >> 4;
		b = dbuf.line[oft+l+6] >> 3;
		shift_nibble(t, b);
		l++;
	} while (l < 6);	
	/* Columns 4 - 7 */
	l = 0;
	do {
		t = dbuf.line[oft+l];
		b = dbuf.line[oft+l+6] << 1;
		shift_nibble(t, b);
		l++;
	} while (l < 6);	
	/* Columns 8 - 11 */
	l = 0;
	do {
		t = dbuf.line[oft+l] << 4;
		b = dbuf.line[oft+l+6] << 5;
		shift_nibble(t, b);
		l++;
	} while (l < 6);	
}

void display_brightness(uint32_t val)
{
	TIM3->CCR1 = val;
}

/* Latch the current display registers */
void display_latch(void)
{
	GPIOC->BSRR = DISPLAY_LCH;
	GPIOC->BSRR = DISPLAY_LCH<<16U;
}

/* Transfer the display buffer to the display */
void display_transfer(void)
{
	card_first();
	card_send(0);
	card_next();
	card_send(12);
	card_next();
}

void display_init(uint32_t brightness)
{
	GPIOC->ODR = 0x0000UL;
	/* clear display shift registers and latch*/
	display_transfer();
	display_latch();
	/* Setup timer and period for PWM */
	TIM3->ARR = 3999U;
	TIM3->CCR1 = brightness;
	/* configure OC1 as PWM mode 1 */
	TIM3->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2;
	TIM3->CCER |= TIM_CCER_CC1P | TIM_CCER_CC1E;
	__DSB();
	/* start timer */
	TIM3->CR1 |= TIM_CR1_CEN;
}

