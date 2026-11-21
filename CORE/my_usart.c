#include "my_usart.h"

extern uint8_t wifi_rx;  /* single-byte UART RX buffer, defined in my_wifi.c */

volatile uint8_t rx_buff;
volatile RingBuffer_t ringbuffer;
UartState_t uartstate;
int fputc(int ch,FILE*f){
	HAL_UART_Transmit(&huart1,(uint8_t *)&ch,1,100);
	return ch;
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if(huart==&huart1){
		RingBuffer_WriteByte(wifi_rx);
		HAL_UART_Receive_IT(&huart1,(uint8_t *)&wifi_rx,1);
	}
}
uint8_t RingBuffer_ISEmpty(void){
	if(ringbuffer.head==ringbuffer.tail)
		return 1;
	return 0;
}
uint8_t RingBuffer_ISFull(void){
	if((ringbuffer.head+1)%4096==ringbuffer.tail)
		return 1;
	return 0;
}
void RingBuffer_WriteByte(uint8_t byte){
	if(RingBuffer_ISFull())return ;
	ringbuffer.buffer[ringbuffer.head++]=byte;
	if(ringbuffer.head>=4096)ringbuffer.head=0;
}
uint8_t RingBuffer_ReadByte(uint8_t* byte){
	if(RingBuffer_ISEmpty())return 0;
	*byte=ringbuffer.buffer[ringbuffer.tail++];
	if(ringbuffer.tail>=4096)ringbuffer.tail=0;
	return 1;
}
//void Uart_Parse_Task(){
//	static uint8_t buf,cmd_mode;
//	if(!RingBuffer_ReadByte(&buf)) {
//        return; 
//    }
//	switch(uartstate){
//		case UART_STATE_HEAD:
//			if(buf==0xAA)
//					uartstate=UART_STATE_CMD;
//			break;
//		case UART_STATE_CMD:
//			if(buf==0x10){
//					cmd_mode=0;
//					//远程切模式
//					uartstate=UART_STATE_DATA;
//			}else if(buf==0x20){
//					//手动模式下调速
//					cmd_mode=1;
//					uartstate=UART_STATE_DATA;
//			}else{
//					uartstate=UART_STATE_HEAD;
//			}
//			break;
//		case UART_STATE_DATA:
//			if(!cmd_mode){
//				//DATA 0x00 切换到 AUTO，0x01 切换到 MANUAL
//				if(buf==0x00 || buf==0x01){
//					//EventBuffer_WriteByte(EVENT_CMD_MODE_MANUAL);
//					uartstate=UART_STATE_TAIL;
//				}else
//					uartstate=UART_STATE_HEAD;
//			}
//			else{
//			//DATA 为档位 0~3
//				if(buf==0x00){
//				//DATA 为档位0
//					EventBuffer_WriteByte(EVENT_CMD_SET_SPEED_0);
//					uartstate=UART_STATE_TAIL;
//				}else if(buf==0x01){
//				//DATA 为档位1
//					EventBuffer_WriteByte(EVENT_CMD_SET_SPEED_1);
//					uartstate=UART_STATE_TAIL;
//				}else if(buf==0x02){
//				//DATA 为档位2
//					EventBuffer_WriteByte(EVENT_CMD_SET_SPEED_2);
//					uartstate=UART_STATE_TAIL;
//				}else if(buf==0x03){
//				//DATA 为档位3
//					EventBuffer_WriteByte(EVENT_CMD_SET_SPEED_3);
//					uartstate=UART_STATE_TAIL;
//				}
//				else
//					uartstate=UART_STATE_HEAD;
//			}
//			break;
//		case UART_STATE_TAIL:
//			if(buf==0x55){
//				//接收完成
//			}
//			uartstate=UART_STATE_HEAD;
//			break;
//	}
//}



