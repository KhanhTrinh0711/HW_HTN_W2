#include "stm32f1xx.h"
volatile char buffer[64];
volatile int i = 0;
volatile int ready = 0;
void Uart_Init(void){
    RCC->APB2ENR |= (1 << 2);
    RCC->APB2ENR |= (1 << 14);

    GPIOA->CRH &= ~(0xF << 4);
    GPIOA->CRH |= (0xB << 4);
    
    GPIOA->CRH &= ~(0xF << 8);
    GPIOA->CRH |= (0x4 << 8);

    USART1->BRR = 7500;
    USART1->CR1 &= ~(1 << 12);
    USART1->CR1 |= (1 << 2);
    USART1->CR1 |= (1 << 3);
    USART1->CR1 |= (1 << 5);
    USART1->CR1 |= (1 << 13);
    NVIC_EnableIRQ(USART1_IRQn);
}
void Send_Char(char a){
    while(!(USART1->SR & (1 << 7)));
    USART1->DR = a;
}
void Send_String(const char *string)
{
    while (*string != '\0') {
        Send_Char(*string);
        string++;
    }
}
void Send_Buffer()
{
    int j = 0;
    while (buffer[j] != '\0')
    {
        Send_Char(buffer[j]);
        j++;
    }
}
void USART1_IRQHandler(){
    char c = USART1->DR;
    if(c == '!') {
        buffer[i] = '\0';
        ready = 1;
    }
    else if(c != '\r' && c != '\n'){
        buffer[i] = c;
        i++;
    }
}
int main(void){
    Uart_Init();
    while(1){
        if(ready){
            Send_String("<ELE1415><02>: ");
            Send_Buffer();
            Send_String("\r\n");
            i = 0;
            ready = 0;
        }
        }
}
