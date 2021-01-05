/*
 * Lattice Panel: System Initialisation
 *
 * Includes default, reset, systick and fault handlers.
 */
#include "stm32f3xx.h"
#include "flash.h"

/* Exported globals */
uint32_t Version = SYSTEMVERSION;
uint32_t SystemID;
volatile uint32_t Uptime;

/* Values provided by the linker */
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

/* Function declarations */
void main(void);

/* Initialise hardware system */
void Reset_Handler(void)
{
	/* Relocate vector table at start of CCMRAM */
	SCB->VTOR = CCMDATARAM_BASE;
	barrier();

	/* Copy .data segment into RAM */
	uint32_t *dsrc = &_sidata;
	uint32_t *ddst = &_sdata;
	uint32_t *dend = &_edata;
	while(ddst < dend)
		*ddst++ = *dsrc++;

	/* Zero .bss */
	uint32_t *bdst = &_sbss;
	uint32_t *bend = &_ebss;
	while(bdst < bend)
		*bdst++ = 0UL;
	__DSB();

	/* Prepare MPU */
	/* Region 2: CCMRAM r/o */
	MPU->RBAR = CCMDATARAM_BASE | MPU_RBAR_VALID_Msk | 2U;
	barrier();
	MPU->RASR = RASR_AP_RO | RASR_TSCB_SRAM | RASR_SZ_16K | RASR_ENABLE;
	barrier();

	/* Region 3: SRAM r/w+NX */
	MPU->RBAR = SRAM_BASE | MPU_RBAR_VALID_Msk | 3U;
	barrier();
	MPU->RASR = RASR_AP_RWNX | RASR_TSCB_SRAM | RASR_SZ_64K | RASR_ENABLE;
	barrier();

	/* Region 4: Flash mem r/w+NX */
	MPU->RBAR = FLASH_BASE | MPU_RBAR_VALID_Msk | 4U;
	barrier();
	MPU->RASR = RASR_AP_RWNX | RASR_TSCB_FLASH | RASR_SZ_512K | RASR_ENABLE;
	barrier();

	/* Region 5: Loader & program text r/o+NX */
	MPU->RBAR = FLASH_BASE | MPU_RBAR_VALID_Msk | 5U;
	barrier();
	MPU->RASR = (0xe0U << 8) | RASR_AP_RONX | RASR_TSCB_FLASH | RASR_SZ_32K | RASR_ENABLE;
	barrier();

	/* Region 6: Flash ram alias no access */
	MPU->RBAR = FLASH_ALIAS | MPU_RBAR_VALID_Msk | 6U;
	barrier();
	MPU->RASR = RASR_AP_NONE | RASR_TSCB_FLASH | RASR_SZ_512K | RASR_ENABLE;
	barrier();

	/* Region 7: Stack guard no-access */
	MPU->RBAR = STACK_BOTTOM | MPU_RBAR_VALID_Msk | 7U;
	barrier();
	MPU->RASR = RASR_AP_NONE | RASR_TSCB_SRAM | RASR_SZ_32B | RASR_ENABLE;
	barrier();

	/* Enable MPU & MemManage fault handler */
	ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);
	barrier();

	/* Enable the LSI clock */
	RCC->CSR |= RCC_CSR_LSION;

	/* Enable peripheral clocks */
	RCC->AHBENR |= RCC_AHBENR_CRCEN | RCC_AHBENR_GPIOCEN;
	RCC->APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_TIM3EN;
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
	barrier();

	/* Compute a hardware-unique identifier */
	CRC->DR = UID->XY;
	CRC->DR = UID->LOTWAF;
	CRC->DR = UID->LOT;
	SystemID = CRC->DR;
	CRC->INIT = SystemID;

	/*
	 * Port configuration notation:
	 *
	 *      Mode:   -       Reset default
	 *              I       Input mode
	 *              O       General purpose output mode
	 *              Xn      Alternate function mode AFn
	 *              A       Analog mode
	 *
	 *      Pull:   ^       pull-up
	 *              v       pull-down
	 *
	 *      Otype:  PP      Output push-pull
	 *              OD      Output open-drain
	 */

	/*
	 * Port C: PWM and OD outputs
	 *  0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15
	 *  I   A   A   A   A   A   X2  O   O   O   O   O   O   A   A   A
	 *                          PP  PP  PP  PP  PP  PP  PP
	 */
	GPIOC->AFR[0] = 0x02000000UL;
	GPIOC->MODER = 0xfd556ffcUL;
	GPIOC->OSPEEDR = 0x01555000UL;
	barrier();

	if (IS_ENABLED(LOCK_GPIO)) {
		/* Lock all GPIO configurations */
		GPIOC->LCKR = 0x0001ffff;
		GPIOC->LCKR = 0x0000ffff;
		GPIOC->LCKR = 0x0001ffff;
	}

	/* ensure option bytes are correctly set */
	flash_set_options();
	barrier();

	/* wait for the LSI to stabilise, then connect the RTC */
	wait_for_bit_set(RCC->CSR, RCC_CSR_LSIRDY);
	barrier();
	PWR->CR |= PWR_CR_DBP;
	__DSB();
	RCC->BDCR |= RCC_BDCR_RTCSEL_LSI;
 	RCC->BDCR |= RCC_BDCR_RTCEN;
	__DSB();

	/* configure the RTC for max SSR usage */
	RTC->WPR = 0xca;
	RTC->WPR = 0x53;
	__DSB();
	
	/* check for ok to continue */
	RTC->ISR |= RTC_ISR_INIT;
	wait_for_bit_set(RTC->ISR, RTC_ISR_INITF);
	barrier();
	RTC->CR |= RTC_CR_BYPSHAD;
	RTC->PRER = 0x00007fffUL;
	RTC->TR = 0x0U;
	RTC->DR = 0x00002101UL;
	barrier();
	RTC->ISR &= (~RTC_ISR_INIT);
	barrier();
	PWR->CR &= (~PWR_CR_DBP);

	/* Start the uptime clock and call main */
	Uptime = 0UL;
	SysTick_Config(400000UL);
	__DSB();
	main();
	while(1);
}

/* Wait for next interupt - in place */
void wait_for_interrupt(void)
{
	barrier();
	__DSB();
	barrier();
	__WFI();
	barrier();
}

/* ARM SysTick Handler */
void SysTick_Handler(void)
{
	Uptime++;
}

/*
 * Default handler
 */
void Default_Handler(void)
{
	BREAKPOINT(254);
	while (1);
}

/* 
 * Handlers for System-critical faults
 *
 * When software watchdog is enabled (USE_IWDG),
 * these handlers result in a reset.
 */
void NMI_Handler(void)
{
	BREAKPOINT(0);
	while (1);
}

void HardFault_Handler(void)
{
	BREAKPOINT(1);
	while (1);
}

void MemManage_Handler(void)
{
	BREAKPOINT(2);
	while (1);
}

void BusFault_Handler(void)
{
	BREAKPOINT(3);
	while (1);
}

void UsageFault_Handler(void)
{
	BREAKPOINT(4);
	while (1);
}
