#ifndef __ENV_SENSOR_H
#define __ENV_SENSOR_H

#include "main.h"

/* ��Ӳ�� I2C1 ��� */
extern I2C_HandleTypeDef hi2c1;
extern  float _3hours_humi[60],_3hours_temp[60],_3hours_press[60];

#define ENV_I2C_HANDLE &hi2c1

/* �豸 I2C ��ַ */
#define AHT20_ADDR     0x70 
#define BMP280_ADDR    0xEE // �����ʼ��ʧ�ܣ��볢�Ը�Ϊ 0xEE
typedef enum {
    WEATHER_SUNNY = 0,   // ����
    WEATHER_CLOUDY,      // ����
    WEATHER_OVERCAST,    // ����/���
    WEATHER_RAIN,        // ����
    WEATHER_STORM        // ǿ�籩/����
} Weather_State_t;
/* �������ݽṹ�� */
typedef struct {
    float Temperature;  // �ں��¶� (��)��Ĭ�ϲ��� AHT20 ������
    float Humidity;     // ���ʪ�� (%)
    float Pressure;     // ��ѹ (Pa)
} Env_Data_t;
typedef struct {
    const char* condition;  // 环境状态 (如: "室外偏潮", "易结露" 等)
    const char* advice;     // 操作建议 (如: "建议关窗", "开启除湿" 等)
} Env_Brief_Result_t;
/* Latest filtered indoor temp/humidity/pressure, refreshed by sensor_run() about every 1s. */
extern Env_Data_t g_current_env;

/* API ���� */
uint8_t Env_Sensors_Init(void);
uint8_t Env_Sensors_Read_Async(Env_Data_t *Data);
void values_filtering(Env_Data_t *value);
void sensor_run(void);
Weather_State_t Predict_Weather(float* press_array);
void values_filtering_slow(Env_Data_t *value);
float Calculate_Rain_Probability(float* press_array);
float Calculate_Dew_Point(float temp, float rh);
Env_Brief_Result_t Evaluate_Brief_Environment(float temp_in, float rh_in, float temp_out, float rh_out);
#endif

