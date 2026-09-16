#include "stm32f1xx.h"

#define SYSTEM_CLOCK_HZ 72000000UL
#define UART_BAUDRATE 9600UL

static volatile uint32_t systick_ms;

static void uart_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    /* PA9: USART1_TX, alternate-function push-pull, 50 MHz. */
    GPIOA->CRH &= ~(0xFUL << 4);
    GPIOA->CRH |= (0xBUL << 4);

    USART1->BRR = SYSTEM_CLOCK_HZ / UART_BAUDRATE;
    USART1->CR1 = USART_CR1_TE | USART_CR1_UE;
}

static void uart_send_char(char value)
{
    while ((USART1->SR & USART_SR_TXE) == 0U) {
    }
    USART1->DR = (uint16_t)value;
}

static void uart_send_string(const char *string)
{
    while (*string != '\0') {
        uart_send_char(*string++);
    }
}

static void uart_send_uint(uint32_t value)
{
    char digits[10];
    uint32_t length = 0U;

    if (value == 0U) {
        uart_send_char('0');
        return;
    }

    while (value != 0U) {
        digits[length++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    while (length != 0U) {
        uart_send_char(digits[--length]);
    }
}

static void adc_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_ADC1EN;

    /* PA2: analog input, channel ADC1_IN2. */
    GPIOA->CRL &= ~(0xFUL << 8);

    /* ADC clock = PCLK2 / 6 = 12 MHz. */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_ADCPRE) | RCC_CFGR_ADCPRE_DIV6;

    ADC1->SMPR2 &= ~(0x7UL << 6);
    ADC1->SMPR2 |= (0x7UL << 6); /* Channel 2: 239.5 ADC cycles. */
    ADC1->SQR1 = 0U;              /* One conversion in the regular sequence. */
    ADC1->SQR3 = 2U;              /* Regular conversion: channel 2. */
    ADC1->CR2 |= (7UL << ADC_CR2_EXTSEL_Pos) | ADC_CR2_EXTTRIG;

    ADC1->CR2 |= ADC_CR2_ADON;

    ADC1->CR2 |= ADC_CR2_RSTCAL;
    while ((ADC1->CR2 & ADC_CR2_RSTCAL) != 0U) {
    }

    ADC1->CR2 |= ADC_CR2_CAL;
    while ((ADC1->CR2 & ADC_CR2_CAL) != 0U) {
    }
}

static uint16_t adc_read(void)
{
    ADC1->CR2 |= ADC_CR2_ADON;
    while ((ADC1->SR & ADC_SR_EOC) != 0U) {
        (void)ADC1->DR;
    }

    ADC1->CR2 |= ADC_CR2_SWSTART;
    while ((ADC1->SR & ADC_SR_EOC) == 0U) {
    }

    return (uint16_t)ADC1->DR;
}

static void systick_init(void)
{
    SysTick->LOAD = (SYSTEM_CLOCK_HZ / 1000UL) - 1UL;
    SysTick->VAL = 0UL;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_ENABLE_Msk;
}

void SysTick_Handler(void)
{
    ++systick_ms;
}

int main(void)
{
    uint32_t last_sample = 0U;

    uart_init();
    systick_init();

    adc_init();

    while (1) {
        if ((systick_ms - last_sample) >= 1000UL) {
            uint32_t raw;

            last_sample = systick_ms;
            raw = adc_read();

            uart_send_uint(raw);
            uart_send_string("\r\n");
        }
    }
}
