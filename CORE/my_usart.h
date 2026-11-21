#include "main.h"
#include "stdio.h"
#include "usart.h"
#include "my_even.h"

extern volatile uint8_t rx_buff;

typedef struct{
	uint8_t buffer[4096];
	uint16_t head;
	uint16_t tail;
}RingBuffer_t;

typedef enum {
    UART_STATE_HEAD = 0,    
    UART_STATE_CMD,         
    UART_STATE_DATA,        
    UART_STATE_TAIL        
} UartState_t;
void RingBuffer_WriteByte(uint8_t byte);
uint8_t RingBuffer_ReadByte(uint8_t* byte);


