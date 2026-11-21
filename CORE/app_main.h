#ifndef __APP_MAIN_H
#define __APP_MAIN_H

#include "main.h"
#include "my_even.h"
#include "ssd1322.h"
#include "my_key.h"
#include "my_wifi.h"
#include "mpu6050.h"
#include "env_sensor.h"
#include	"low_power.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "rtc.h"
#include <stdlib.h>
//主的枚举
typedef enum {
    PAGE_HOME = 0,     
    PAGE_ENV,           
    PAGE_TREND,         
    PAGE_POMODORO,    
    PAGE_SYS,           
} PageId_t;
//室外单独一页
typedef enum {
		ENV_SUB_OVERVIEW = 0,       	// 0: 3日天气概览 (主页/缩略预览)
    ENV_SUB_TODAY_DETAIL,       // 1: 今天天气详情 (1/4)
    ENV_SUB_TOMORROW_DETAIL,    // 2: 明天天气详情 (2/4)
    ENV_SUB_AFTER_TOM_DETAIL,   // 3: 后天天气详情 (3/4)
    ENV_SUB_WIND_HUMIDITY,      // 4: 3日风向与湿度对比 (4/4)
} EnvSubPage_t;
//室内单独一页
typedef enum {
    INDOOR_SUB_OVERVIEW = 0,    // 0. 主概览
    INDOOR_SUB_TEMP,            // 1. 温度 (1/5)
    INDOOR_SUB_HUMI,            // 2. 湿度 (2/5)
    INDOOR_SUB_PRESS,           // 3. 气压 (3/5)
    INDOOR_SUB_FORECAST,        // 4. 预测 (4/5)
    INDOOR_SUB_EVAL,            // 5. 对比评估 (5/5)     
} TrendSubPage_t;
//时钟
typedef enum {
    POMO_BROWSE = 0,    // 浏览态：左右切顶层页，中键进选择
    POMO_SELECT,        // 选择态：左右切卡片高亮，中键进编辑
    POMO_EDIT           // 编辑态：调数字（后续实现）
} PomoMode_t;

typedef enum {
    SYS_SUB_MAIN = 0,     // 设置列表
    SYS_SUB_WIFI          // WiFi 配网提示
} SysSubPage_t;
extern PageId_t current_page;
void ui_create(void);
void main_run(void);
void UI_Show(void);
void RTC_Set_From_GTime(void);
void RTC_Restore_Date(void);
void 		Ringing_handling(void);
uint8_t App_Is_Timing(void);

#endif /* __APP_MAIN_H */


