#ifndef __MY_KEY_H
#define __MY_KEY_H

#include "main.h"
#include "my_even.h"
#include	"low_power.h"

typedef enum{
		STATE_IDLE,        
    STATE_DEBOUNCE,    
    STATE_PRESS_HOLD,  
    STATE_WAIT_RELEASE 
}KeyState_t;
extern KeyState_t KEY_1_STATE;
extern KeyState_t KEY_2_STATE;
extern KeyState_t KEY_3_STATE;
extern volatile uint8_t key_scan_enabled;

void KEY_1_SCAN(void);
void KEY_2_SCAN(void);
void KEY_3_SCAN(void);

#endif /* __MY_KEY_H */


