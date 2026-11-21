#include	"low_power.h"
#include "app_main.h"   /* App_Is_Timing */
#include "mpu6050.h"    /* MPU6050_Clear_Motion_Interrupt / Motion_xxx_EXTI_Wakeup */
#include "my_key.h"     /* KEY_x_STATE / STATE_WAIT_RELEASE */
extern void SystemClock_Config(void);   /* defined in main.c */
volatile uint16_t idle_seconds_count = 0;   
// Sleep thresholds (seconds): 30s=30 / 1min=60 / 5min=300 / never=0 (0 disables idle counting and sleep)
Env_Data_t stop_data;
uint16_t sleep_threshold_seconds = 60;   /* matches menu default set_sleep=1 ("1min") */
volatile uint8_t wake_source = WAKE_SRC_NONE;     
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM1) {
        if (sleep_threshold_seconds > 0) {
            idle_seconds_count++;
        }
    }
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc){
	 wake_source = WAKE_SRC_RTC;
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0 || GPIO_Pin == GPIO_PIN_12 || GPIO_Pin == GPIO_PIN_13) {
        wake_source = WAKE_SRC_KEY;
    }
		if (GPIO_Pin == GPIO_PIN_1) {          
        wake_source = WAKE_SRC_MOTION;
    }
}
//�������¾͵��ã����¼�������
void System_Reset_TimerCount(void) {
    idle_seconds_count = 0;            
    __HAL_TIM_SET_COUNTER(&htim1, 0);   
}


void System_Enter_Stop_With_RTC_F4(void) {
		HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);
    HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, 180 - 1, RTC_WAKEUPCLOCK_CK_SPRE_16BITS);
}

/**
 * @brief ��׼������ Stop ģʽǰ���ã��� PA0��PB12��PB13 ����Ϊ EXTI �½��ػ���Դ
 */
void Key_Enable_EXTI_Wakeup(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // 1. ���� PA0 Ϊ�½����ⲿ�ж� (EXTI Line 0)
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING; // �½��ش���
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // 2. ���� PB12, PB13 Ϊ�½����ⲿ�ж� (EXTI Line 12, Line 13)
    GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING; // �½��ش���
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // 3. �����ӹؼ������֮ǰ����������?EXTI �жϹ����־λ����ֹһ˯��ȥ�ͱ��뻽��?
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_12);
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13);

    // 4. �� NVIC ��ʹ�ܶ�Ӧ���ж�ͨ�������ýϵ����ȼ����ɣ������ڻ��ѣ�
    HAL_NVIC_SetPriority(EXTI0_IRQn, 0x0F, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0x0F, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}
/**
 * @brief �� Stop ģʽ���Ѻ���ã��ر�?EXTI �жϲ��ָ� GPIO Ϊ��ͨ����ģʽ
 */
void Key_Disable_EXTI_Wakeup(void) {
    // 1. �ر� NVIC �е� EXTI �ж�ͨ��
    HAL_NVIC_DisableIRQ(EXTI0_IRQn);
    HAL_NVIC_DisableIRQ(EXTI15_10_IRQn);

    // 2. �������˲��������жϹ����־�?
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_12);
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_13);

    // 3. �� GPIO ��������Ϊ��ͨ����ģʽ (GPIO_MODE_INPUT)
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

}

/* ------------------------------------------------------------------
 * Idle sleep entry: called from the idle point of the main loop (main.c).
 * Condition: sleep enabled && idle time reached threshold && no timing running.
 * Wake handling: KEY/MOTION -> exit sleep; RTC -> refresh env history, sleep again.
 * ------------------------------------------------------------------ */
void LowPower_Try_Enter_Stop(void)
{
    if ((sleep_threshold_seconds > 0) && (idle_seconds_count >= sleep_threshold_seconds) && !App_Is_Timing()) {
			//休眠时间sleep_threshold_seconds大于0
			//idle_seconds_count现在经过的时间
			//!App_Is_Timing()现在没有运行番茄汁闹钟倒计时
        uint8_t sleeping = 1;
        uint8_t i;
        System_Reset_TimerCount();//清除现在经过的时间和TIM计数
        Oled_Write_cmd(0xAE);//休眠
        Key_Enable_EXTI_Wakeup();//开启按键外部中断
        Motion_Enable_EXTI_Wakeup();//开启陀螺仪外部中断
        HAL_TIM_Base_Stop_IT(&htim1);//停止时钟
        System_Enter_Stop_With_RTC_F4();//设置每隔三分钟唤醒rtc中断
			
        while (sleeping) {
					wake_source = WAKE_SRC_NONE;//唤醒来源设置为空
            HAL_SuspendTick();//挂起滴答定时器
            HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);//进入stop
            //----------------分界线--------------------
						//恢复了后
					SystemClock_Config();//恢复时钟
					HAL_ResumeTick();//恢复系统滴答定时器
            if (wake_source == WAKE_SRC_KEY || wake_source == WAKE_SRC_MOTION) {
							//如果是按键唤醒或者敲击唤醒
                MPU6050_Clear_Motion_Interrupt();//清除陀螺仪的标志
                Motion_Disable_EXTI_Wakeup();
                Key_Disable_EXTI_Wakeup();
								//去除按键和陀螺仪的外部中断
                HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
								//停用（关闭）STM32 的 RTC 唤醒定时器
                sleeping = 0;//停止继续执行
            } else if (wake_source == WAKE_SRC_RTC) {
							//每三分钟采集数据放进去，然后继续在while里面执行
                values_filtering_slow(&stop_data);
                for (i = 0; i < 59; i++) {
                    _3hours_humi[i]  = _3hours_humi[i+1];
                    _3hours_temp[i]  = _3hours_temp[i+1];
                    _3hours_press[i] = _3hours_press[i+1];
                }
                _3hours_humi[59]  = stop_data.Humidity;
                _3hours_temp[59]  = stop_data.Temperature;
                _3hours_press[59] = stop_data.Pressure;
            }
        }
				
				//循环外

        HAL_TIM_Base_Start_IT(&htim1);//开启定时器
        Oled_Write_cmd(0xAF);//屏幕解除休眠
        System_Reset_TimerCount();//清除计数值，和运行了多久的时间
        KEY_1_STATE = STATE_WAIT_RELEASE;//都进入到按键按下后准备释放的状态
        KEY_2_STATE = STATE_WAIT_RELEASE;
        KEY_3_STATE = STATE_WAIT_RELEASE;
        HAL_Delay(30);
    }
}



