/*
 * Lattice Panel: LED Display Interface
 */
#ifndef DISPLAY_H
#define DISPLAY_H
#include <stdint.h>

#define DISPLAY_COLS	12
#define DISPLAY_LINES	24
#define DISPLAY_DIN     GPIO_ODR_6      /* Brown */
#define DISPLAY_TOP     GPIO_ODR_7      /* Orange */
#define DISPLAY_BOT     GPIO_ODR_8      /* Green */
#define DISPLAY_CLK     GPIO_ODR_9      /* White */
#define DISPLAY_LCH     GPIO_ODR_10     /* Yellow */
#define DISPLAY_PCK     GPIO_ODR_11     /* Blue */
#define DISPLAY_SEL     GPIO_ODR_12     /* Purple */
#define DMASK           (DISPLAY_TOP | DISPLAY_BOT)

struct display_buf {
	uint32_t	line[DISPLAY_LINES];
};

extern struct display_buf dbuf;

void display_init(uint32_t brightness);
void display_brightness(uint32_t val);
void display_latch(void);
void display_transfer(void);

#endif /* DISPLAY_H */
