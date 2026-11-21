#include "my_key.h"
volatile uint8_t key_scan_enabled = 0;   // 上电初始关闭，main 初始化完成后再打开
KeyState_t KEY_1_STATE=STATE_IDLE;
	static uint32_t KEY_1_DELAY=0;
KeyState_t KEY_2_STATE=STATE_IDLE;
	static uint32_t KEY_2_DELAY=0;
KeyState_t KEY_3_STATE=STATE_IDLE;
	static uint32_t KEY_3_DELAY=0;
void KEY_1_SCAN(void){
	uint32_t KEY_NOW = HAL_GetTick();
	
	uint8_t KEY_1_VALUE=HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_0);
	
	switch(KEY_1_STATE){
		case STATE_IDLE:
			if(KEY_1_VALUE==0){
				KEY_1_STATE=STATE_DEBOUNCE;
				KEY_1_DELAY=KEY_NOW;
				System_Reset_TimerCount();
			}
			break;
		case STATE_DEBOUNCE:
			if(KEY_NOW-KEY_1_DELAY>=100){
				if(KEY_1_VALUE==0){
					KEY_1_STATE=STATE_PRESS_HOLD;
				}else{
					KEY_1_STATE=STATE_IDLE;
				}
			}
			break;
		case STATE_PRESS_HOLD:
			if(KEY_1_VALUE==0){
				if(KEY_NOW-KEY_1_DELAY>=1000){
					if(KEY_1_VALUE==0){
						EventBuffer_WriteByte(EV_KEY1_LONG);
						KEY_1_STATE=STATE_WAIT_RELEASE;
					}
				}
			}else{
						EventBuffer_WriteByte(EV_KEY1_SHORT);
						KEY_1_STATE=STATE_WAIT_RELEASE;
				}
			break;
		case STATE_WAIT_RELEASE:
			if(KEY_1_VALUE==1)
					KEY_1_STATE=STATE_IDLE;
			
			break;
	}
}

void KEY_2_SCAN(void){
	uint32_t KEY_NOW = HAL_GetTick();
	
	uint8_t KEY_2_VALUE=HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_12);
	
	switch(KEY_2_STATE){
		case STATE_IDLE:
			if(KEY_2_VALUE==0){
				KEY_2_STATE=STATE_DEBOUNCE;
				KEY_2_DELAY=KEY_NOW;
				System_Reset_TimerCount();
			}
			break;
		case STATE_DEBOUNCE:
			if(KEY_NOW-KEY_2_DELAY>=100){
				if(KEY_2_VALUE==0){
					KEY_2_STATE=STATE_PRESS_HOLD;
				}else{
					KEY_2_STATE=STATE_IDLE;
				}
			}
			break;
		case STATE_PRESS_HOLD:
			if(KEY_2_VALUE==0){
				if(KEY_NOW-KEY_2_DELAY>=1000){
					if(KEY_2_VALUE==0){
						EventBuffer_WriteByte(EV_KEY2_LONG);
						KEY_2_STATE=STATE_WAIT_RELEASE;
					}
				}
			}else{
						EventBuffer_WriteByte(EV_KEY2_SHORT);
						KEY_2_STATE=STATE_WAIT_RELEASE;
				}
			break;
		case STATE_WAIT_RELEASE:
			if(KEY_2_VALUE==1)
					KEY_2_STATE=STATE_IDLE;
			break;
	}
}

void KEY_3_SCAN(void){
	uint32_t KEY_NOW = HAL_GetTick();

	uint8_t KEY_3_VALUE=HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_13);
	
	switch(KEY_3_STATE){
		case STATE_IDLE:
			if(KEY_3_VALUE==0){
				KEY_3_STATE=STATE_DEBOUNCE;
				KEY_3_DELAY=KEY_NOW;
				System_Reset_TimerCount();
			}
			break;
		case STATE_DEBOUNCE:
			if(KEY_NOW-KEY_3_DELAY>=100){
				if(KEY_3_VALUE==0){
					KEY_3_STATE=STATE_PRESS_HOLD;
				}else{
					KEY_3_STATE=STATE_IDLE;
				}
			}
			break;
		case STATE_PRESS_HOLD:
			if(KEY_3_VALUE==0){
				if(KEY_NOW-KEY_3_DELAY>=1000){
					if(KEY_3_VALUE==0){
						EventBuffer_WriteByte(EV_KEY3_LONG);
						KEY_3_STATE=STATE_WAIT_RELEASE;
					}
				}
			}else{
						EventBuffer_WriteByte(EV_KEY3_SHORT);
						KEY_3_STATE=STATE_WAIT_RELEASE;
				}
			break;
		case STATE_WAIT_RELEASE:
			if(KEY_3_VALUE==1)
					KEY_3_STATE=STATE_IDLE;
			break;
	}
}


