#include "stm32f1xx.h"

#define TIMER_CLOCK_HZ 72000000UL
#define PWM_FREQUENCY_HZ 1000UL
#define TIMER_PRESCALER 71UL
#define TIMER_PERIOD ((TIMER_CLOCK_HZ / (TIMER_PRESCALER + 1UL) / PWM_FREQUENCY_HZ) - 1UL)

static void gpio_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;

    /* TIM3_CH1/CH2 on PA6/PA7, alternate-function push-pull, 50 MHz. */
    GPIOA->CRL &= ~(0xFFUL << 24);
    GPIOA->CRL |= (0xBBUL << 24);

    /* TIM3_CH3/CH4 on PB0/PB1, alternate-function push-pull, 50 MHz. */
    GPIOB->CRL &= ~0xFFUL;
    GPIOB->CRL |= 0xBBUL;
}

static void pwm_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    TIM3->PSC = TIMER_PRESCALER;
    TIM3->ARR = TIMER_PERIOD;

    /* PWM mode 1 with preload enabled for channels 1 and 2. */
    TIM3->CCMR1 = (6UL << 4) | (1UL << 3) |
                  (6UL << 12) | (1UL << 11);

    /* PWM mode 1 with preload enabled for channels 3 and 4. */
    TIM3->CCMR2 = (6UL << 4) | (1UL << 3) |
                  (6UL << 12) | (1UL << 11);

    TIM3->CCR1 = ((TIMER_PERIOD + 1UL) * 10UL) / 100UL;
    TIM3->CCR2 = ((TIMER_PERIOD + 1UL) * 30UL) / 100UL;
    TIM3->CCR3 = ((TIMER_PERIOD + 1UL) * 50UL) / 100UL;
    TIM3->CCR4 = ((TIMER_PERIOD + 1UL) * 70UL) / 100UL;

    TIM3->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E |
                 TIM_CCER_CC3E | TIM_CCER_CC4E;
    TIM3->CR1 = TIM_CR1_ARPE;
    TIM3->EGR = TIM_EGR_UG;
    TIM3->CR1 |= TIM_CR1_CEN;
}

int main(void)
{
    gpio_init();
    pwm_init();

    while (1) {
    }
}
