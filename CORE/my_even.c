#include "my_even.h"
EventType_t eventype;
volatile Event_t eventbuff;
EventQueue_t ui_eventq;   /* UI data event queue instance */


uint8_t EventBuffer_ISEmpty(void){
	if(eventbuff.head==eventbuff.tail)
		return 1;
	return 0;
}

uint8_t EventBuffer_ISFull(void){
	if((eventbuff.head+1)%100==eventbuff.tail)
		return 1;
	return 0;
}

void EventBuffer_WriteByte(EventType_t even){
	if(EventBuffer_ISFull())return;
	eventbuff.buff[eventbuff.head++]=even;
	if(eventbuff.head>=100)eventbuff.head=0;
}

uint8_t EventBuffer_ReadByte(EventType_t* even){
	if(EventBuffer_ISEmpty())return 0;
	*even=eventbuff.buff[eventbuff.tail++];
	if(eventbuff.tail>=100)eventbuff.tail=0;
	return 1;
}




/* ---------- UI data event queue ---------- */
void EventQueue_Init(EventQueue_t *q){
	q->head=0;
	q->tail=0;
}

uint8_t EventQueue_Write(EventQueue_t *q, EventType_t ev){
	if((q->head+1)%UI_QUEUE_DEPTH==q->tail)return 0;   /* full: drop */
	q->buff[q->head++]=ev;
	if(q->head>=UI_QUEUE_DEPTH)q->head=0;
	return 1;
}

uint8_t EventQueue_Read(EventQueue_t *q, EventType_t *out){
	if(q->head==q->tail)return 0;                      /* empty */
	*out=q->buff[q->tail++];
	if(q->tail>=UI_QUEUE_DEPTH)q->tail=0;
	return 1;
}





