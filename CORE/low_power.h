#ifndef __LOW_POWER_H
#define __LOW_POWER_H

#include	"main.h"
#include "tim.h"
#include "rtc.h"
#include "env_sensor.h"
#include "ssd1322.h"

extern volatile uint16_t idle_seconds_count;
extern uint16_t sleep_threshold_seconds;
#define WAKE_SRC_NONE  0
#define WAKE_SRC_KEY   1
#define WAKE_SRC_RTC   2
#define WAKE_SRC_MOTION 3
extern volatile uint8_t wake_source;
extern Env_Data_t stop_data;
void System_Reset_TimerCount(void) ;
void System_Enter_Stop_With_RTC_F4(void) ;
void Key_Enable_EXTI_Wakeup(void);
void Key_Disable_EXTI_Wakeup(void);
void LowPower_Try_Enter_Stop(void);

#endif /* __LOW_POWER_H */


