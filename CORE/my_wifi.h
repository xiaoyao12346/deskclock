#ifndef __MY_WIFI_H
#define __MY_WIFI_H

#include "main.h"
#include "usart.h"
#include "string.h"
#include "stdio.h"
#include "my_usart.h"
#include "ssd1322.h"
#include "env_sensor.h"

#define MY_FLASH_PAGE_SIZE     1024 
/* 警告：代码镜像已超过 0x08040000（约 271KB），Sector 6 属于程序区，禁止擦写！
 * 天气与 WiFi 配置同放最后的 Sector 7：擦除按整扇区进行，任一份数据写入都需两份一起重写 */
#define FLASH_WEATHER_ADDR     0x08060000   /* Sector 7 起始，天气数据 (~492B) */
#define FLASH_WIFI_ADDR 			 0x08070000   /* Sector 7 内 +64KB 处，WiFi 配置 (164B) */
extern uint8_t wifi_rx;
typedef struct {
    uint32_t magic;                     // 魔数标记

    char city[32];                      // 城市名称
    char date[3][16];                   // 日期
    char text_day[3][32];               // 白天天气
    char text_night[3][32];             // 夜间天气
    char high[3][8];                    // 最高气温
    char low[3][8];                     // 最低气温
    char rainfall[3][8];                // 降水量
    char precip[3][8];                  // 降水概率
    char wind_direction[3][16];         // 风向
    char wind_speed[3][8];              // 风速
    char wind_scale[3][8];              // 风级
    char humidity[3][8];                // 相对湿度
} Weather_Info_t;
typedef struct {
    uint32_t magic;     
    char ssid[32];
    char pass[64];
    char city[64];
} Wifi_Config_t;
typedef enum {
    WIFI_STATE_SEND_RST = 0,
    WIFI_STATE_WAIT_RST,
    WIFI_STATE_SEND_AT,
    WIFI_STATE_WAIT_AT,
    WIFI_STATE_SEND_CWMODE,
    WIFI_STATE_WAIT_CWMODE,
    WIFI_STATE_SEND_CIPMUX,
    WIFI_STATE_WAIT_CIPMUX,
    WIFI_STATE_SEND_CWJAP,
    WIFI_STATE_WAIT_CWJAP,
    WIFI_STATE_SEND_CIPCLOSE,
    WIFI_STATE_WAIT_CIPCLOSE,
	
    WIFI_STATE_SEND_CIPSTART,
    WIFI_STATE_WAIT_CIPSTART,
    WIFI_STATE_SEND_CIPSEND,
    WIFI_STATE_WAIT_PROMPT,
    WIFI_STATE_SEND_REQUEST,
    WIFI_STATE_WAIT_RESPONSE,
    WIFI_STATE_DONE,
	
		WIFI_STATE_SEND_SNTP_CFG,      
    WIFI_STATE_WAIT_SNTP_CFG,       
    WIFI_STATE_SEND_SNTP_TIME,     
    WIFI_STATE_WAIT_SNTP_TIME,     
    WIFI_STATE_PARSE_SNTP_TIME,     
	
	
    WIFI_STATE_ERROR,
		WIFI_STATE_IDLE
} WiFi_State_t;

typedef struct {
    uint16_t year;   
    uint8_t  month;  
    uint8_t  date;   
    uint8_t  hour; 
    uint8_t  min;    
    uint8_t  sec;    
    uint8_t  week;  
} Time_Info_t;

typedef enum {
    WIFI_SET_SEND_AT,
    WIFI_SET_WAIT_AT,
    WIFI_SET_SEND_CWMODE,
    WIFI_SET_WAIT_CWMODE,
    WIFI_SET_SEND_CWSAP,
    WIFI_SET_WAIT_CWSAP,
    WIFI_SET_SEND_CIPMUX,
    WIFI_SET_WAIT_CIPMUX,
    WIFI_SET_SEND_CIPSERVER,
    WIFI_SET_WAIT_CIPSERVER,
    
    WIFI_SET_IDLE,              // 空闲，等待手机连接 (+IPD)
    WIFI_SET_CAPTURE_REQ,       // 捕获 HTTP GET 请求内容
    WIFI_SET_SEND_CIPSEND,      // 发送 AT+CIPSEND
    WIFI_SET_WAIT_PROMPT,       // 等待 > 提示符
    WIFI_SET_SEND_HTML,         // 发送 HTML 数据
    WIFI_SET_WAIT_SEND_OK,      // 等待 SEND OK
    WIFI_SET_CONFIG_DONE,       // 配网成功结束
    WIFI_SET_ERROR
} WIFI_Setting_t;
typedef enum {
    SYS_WIFI_MODE_STA = 0,   // 正常工作模式（连WiFi，获取天气/时间）
    SYS_WIFI_MODE_AP_CFG,    // 网页配网模式（开热点等待手机连接）
    SYS_WIFI_MODE_IDLE       // 闲置模式（可选，用于暂停WiFi功能）
} Sys_Wifi_Mode_t;
extern Time_Info_t g_time;
extern Weather_Info_t g_weather;
extern Sys_Wifi_Mode_t current_wifi_mode;
extern Wifi_Config_t wifi_flash_config;
void Send_AT_Cmd(const char *cmd);
void wifi_handle(void);
void Flash_Read_Weather(Weather_Info_t *weather);
uint8_t Flash_Write_Weather(Weather_Info_t *weather);
void wifiset_handle(void);
uint8_t Flash_Write_WifiConfig(Wifi_Config_t *config);
void Flash_Read_WifiConfig(Wifi_Config_t *config);
void Wifi_Task_Manager(void);
const char* weather_en_to_zh(const char *en_text);
const char* get_wind_direction_cn(const char *code);
const char* weather_state_to_zh(Weather_State_t state);

#endif /* __MY_WIFI_H */

