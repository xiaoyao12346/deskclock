#ifndef __MPU6050_H
#define __MPU6050_H

#include "main.h"

/* 绑定硬件 I2C2 句柄 */
extern I2C_HandleTypeDef hi2c2;
#define MPU6050_I2C_HANDLE &hi2c2

/* 设备 I2C 地址 (AD0 引脚接地时为 0xD0，接 3.3V 时修改为 0xD2) */
#define MPU6050_ADDR 0xD0

/* MPU6050 数据采集结构体 */
typedef struct {
    float AccX;     // g (重力加速度)
    float AccY;
    float AccZ;
    float GyroX;    // °/s (角速度)
    float GyroY;
    float GyroZ;
    float Temp;     // 芯片内部温度 ℃
} MPU6050_Data_t;

/* API 声明 */
uint8_t MPU6050_Init(void);
uint8_t MPU6050_ReadData(MPU6050_Data_t *Data);
void MPU6050_Config_Motion_Interrupt(uint8_t threshold);
void MPU6050_Clear_Motion_Interrupt(void);
void Motion_Enable_EXTI_Wakeup(void);
void Motion_Disable_EXTI_Wakeup(void);
#endif


