#include "stm32f1xx.h"

#define SYSTEM_CLOCK_HZ 72000000UL
#define SYSTICK_TICK_HZ 1000UL
#define LED_PIN 13U

static volatile uint32_t systick_ms;

static void clock_init(void)
{
    FLASH->ACR = 0x12U;

    RCC->CR |= (1UL << 16);
    while ((RCC->CR & (1UL << 17)) == 0UL) {
    }

    RCC->CFGR |= (7UL << 18);
    RCC->CFGR |= (1UL << 16);
    RCC->CR |= (1UL << 24);
    while ((RCC->CR & (1UL << 25)) == 0UL) {
    }

    RCC->CFGR |= (4UL << 8);
    RCC->CFGR |= (2UL << 0);
    while ((RCC->CFGR & (3UL << 2)) != (2UL << 2)) {
    }
}

static void led_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;

    GPIOC->CRH &= ~(0xFUL << 20);
    GPIOC->CRH |= (0x2UL << 20);
    GPIOC->BSRR = (1UL << (LED_PIN + 16U));
}

static void systick_init(void)
{
    SysTick->LOAD = (SYSTEM_CLOCK_HZ / SYSTICK_TICK_HZ) - 1UL;
    SysTick->VAL = 0UL;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Handler(void)
{
    ++systick_ms;

    if (systick_ms >= 5000UL) {
        systick_ms = 0UL;
        GPIOC->ODR ^= (1UL << LED_PIN);
    }
}

int main(void)
{
    clock_init();
    led_init();
    systick_init();

    while (1) {
    }
}
