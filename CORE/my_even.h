#ifndef __MY_EVEN_H
#define __MY_EVEN_H
#include "main.h"
typedef enum{
		EV_NONE,
		EV_KEY1_SHORT,       		 // K1 �̰�
		EV_KEY1_LONG,            // K1 ����
		EV_KEY2_SHORT,           // K2 �̰�
		EV_KEY2_LONG ,           // K2 ����
		EV_KEY3_SHORT,
		EV_TIME_SYNCED,          /* UI queue: NTP time synced */
		EV_WEATHER_UPDATED,      /* UI queue: weather data updated */
		EV_KEY3_LONG             // K3 ����
}EventType_t;	
typedef struct{
	EventType_t buff[100];
	volatile	uint8_t head;
	volatile uint8_t tail;
}Event_t;
void EventBuffer_WriteByte(EventType_t even);
uint8_t EventBuffer_ReadByte(EventType_t* even);

/* UI data event queue: producer (my_wifi.c) and consumer (main_run) both
   run in the main loop, single-producer/single-consumer, no critical section */
#define UI_QUEUE_DEPTH 8
typedef struct {
	EventType_t buff[UI_QUEUE_DEPTH];
	uint8_t head;
	uint8_t tail;
}EventQueue_t;
extern EventQueue_t ui_eventq;          /* instance defined in my_even.c */
void EventQueue_Init(EventQueue_t *q);
uint8_t EventQueue_Write(EventQueue_t *q, EventType_t ev);
uint8_t EventQueue_Read(EventQueue_t *q, EventType_t *out);

extern uint32_t tick_now;   /* frame timestamp sampled at main loop start (main.c) */
#endif


