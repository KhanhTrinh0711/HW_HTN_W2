#include "stm32f1xx.h"

#define SYSTEM_CLOCK_HZ 72000000UL
#define UART_BAUDRATE 9600UL
#define PWM_FREQUENCY_HZ 1000UL
#define PWM_PRESCALER 71UL
#define PWM_PERIOD ((SYSTEM_CLOCK_HZ / (PWM_PRESCALER + 1UL) / PWM_FREQUENCY_HZ) - 1UL)
#define COMMAND_SIZE 32U

static volatile char command[COMMAND_SIZE];
static volatile uint32_t command_index;
static volatile uint32_t command_ready;
static uint32_t led_on;
static uint32_t pwm_percent = 50U;

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

static void uart_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;

    /* PA9: USART1_TX alternate-function push-pull, 50 MHz. */
    GPIOA->CRH &= ~(0xFUL << 4);
    GPIOA->CRH |= (0xBUL << 4);

    /* PA10: USART1_RX input floating. */
    GPIOA->CRH &= ~(0xFUL << 8);
    GPIOA->CRH |= (0x4UL << 8);

    USART1->BRR = SYSTEM_CLOCK_HZ / UART_BAUDRATE;
    USART1->CR1 = USART_CR1_RE | USART_CR1_TE |
                  USART_CR1_RXNEIE | USART_CR1_UE;
    NVIC_EnableIRQ(USART1_IRQn);
}

static void pwm_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    /* TIM3_CH2 on PA7: alternate-function push-pull, 50 MHz. */
    GPIOA->CRL &= ~(0xFUL << 28);
    GPIOA->CRL |= (0xBUL << 28);

    TIM3->PSC = PWM_PRESCALER;
    TIM3->ARR = PWM_PERIOD;
    TIM3->CCMR1 = (6UL << 12) | (1UL << 11); /* CH2 PWM mode 1 + preload. */
    TIM3->CCER = TIM_CCER_CC2E;
    TIM3->CR1 = TIM_CR1_ARPE;
    TIM3->EGR = TIM_EGR_UG;
    TIM3->CR1 |= TIM_CR1_CEN;
}

static void pwm_set_percent(uint32_t percent)
{
    TIM3->CCR2 = ((PWM_PERIOD + 1UL) * percent) / 100UL;
}

static uint32_t text_equals(const volatile char *text, const char *value)
{
    uint32_t index = 0U;

    while (text[index] != '\0' && value[index] != '\0') {
        if (text[index] != value[index]) {
            return 0U;
        }
        ++index;
    }

    return text[index] == '\0' && value[index] == '\0';
}

static uint32_t parse_percent(const volatile char *text, uint32_t *percent)
{
    uint32_t index = 0U;
    uint32_t value = 0U;
    uint32_t digits = 0U;

    while (text[index] >= '0' && text[index] <= '9') {
        value = (value * 10U) + (uint32_t)(text[index] - '0');
        ++index;
        ++digits;
    }

    if (digits == 0U || text[index] != '\0' || value > 100U) {
        return 0U;
    }

    *percent = value;
    return 1U;
}

static void process_command(void)
{
    uint32_t requested_percent;

    if (text_equals(command, "ON")) {
        led_on = 1U;
        pwm_set_percent(pwm_percent);
        uart_send_string("ON!\r\n");
    } else if (text_equals(command, "OFF")) {
        led_on = 0U;
        TIM3->CCR2 = 0U;
        uart_send_string("OFF!\r\n");
    } else if (parse_percent(command, &requested_percent)) {
        pwm_percent = requested_percent;
        if (led_on != 0U) {
            pwm_set_percent(pwm_percent);
        }
        uart_send_uint(pwm_percent);
        uart_send_string("%\r\n");
    } else if (text_equals(command, "stat")) {
        uart_send_string("Status:");
        uart_send_string(led_on != 0U ? "ON PWM:" : "OFF PWM:");
        uart_send_uint(pwm_percent);
        uart_send_string("%!\r\n");
    } else {
        uart_send_string("ERROR!\r\n");
    }
}

void USART1_IRQHandler(void)
{
    char received = (char)USART1->DR;

    if (command_ready != 0U) {
        return;
    }

    if (received == '!') {
        command[command_index] = '\0';
        command_ready = 1U;
    } else if (received != '\r' && received != '\n' &&
               command_index < (COMMAND_SIZE - 1U)) {
        command[command_index++] = received;
    }
}

int main(void)
{
    pwm_init();
    pwm_set_percent(0U);
    uart_init();

    while (1) {
        if (command_ready != 0U) {
            process_command();
            command_index = 0U;
            command_ready = 0U;
        }
    }
}
