#include "utils/micros.h"

#include "stm32f0xx.h"
#include "stm32f0xx_ll_cortex.h"

volatile uint32_t ticks;

uint32_t micros(void)
{
    /* Ensure COUNTFLAG is reset by reading SysTick control and status register */
    LL_SYSTICK_IsActiveCounterFlag();
    uint32_t m = ticks;
    const uint32_t tms = SysTick->LOAD + 1;
    __IO uint32_t u = tms - SysTick->VAL;
    if (LL_SYSTICK_IsActiveCounterFlag())
    {
        m = ticks;
        u = tms - SysTick->VAL;
    }
    return (m * 1000 + (u * 1000) / tms);
}