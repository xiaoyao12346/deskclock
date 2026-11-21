#include "mpu6050.h"

/**
 * @brief  初始化 MPU6050
 * @return 0: 成功, 1: 失败 (未检测到器件)
 */
uint8_t MPU6050_Init(void) {
    uint8_t check = 0;
    uint8_t data = 0;

    // 1. 检查 WHO_AM_I 寄存器 (0x75)，默认值为 0x68
    HAL_I2C_Mem_Read(MPU6050_I2C_HANDLE, MPU6050_ADDR, 0x75, 1, &check, 1, 10);
    if (check != 0x68) {
        return 1; // 设备通信异常或地址错误
    }

    // 2. 解除休眠状态，选择内部 8MHz 时钟 (PWR_MGMT_1 寄存器 0x6B)
    data = 0x00;
    HAL_I2C_Mem_Write(MPU6050_I2C_HANDLE, MPU6050_ADDR, 0x6B, 1, &data, 1, 10);

    // 3. 开启低通滤波器 (CONFIG 寄存器 0x1A)，设置截止频率约 21Hz，抗高频噪点
    data = 0x03;
    HAL_I2C_Mem_Write(MPU6050_I2C_HANDLE, MPU6050_ADDR, 0x1A, 1, &data, 1, 10);

    // 4. 设置陀螺仪量程为 ±500°/s (GYRO_CONFIG 寄存器 0x1B)
    data = 0x08;
    HAL_I2C_Mem_Write(MPU6050_I2C_HANDLE, MPU6050_ADDR, 0x1B, 1, &data, 1, 10);

    // 5. 设置加速度计量程为 ±4g (ACCEL_CONFIG 寄存器 0x1C)
    data = 0x08;
    HAL_I2C_Mem_Write(MPU6050_I2C_HANDLE, MPU6050_ADDR, 0x1C, 1, &data, 1, 10);

    return 0; // 初始化成功
}

/**
 * @brief  单次突发读取 14 字节（加速度+温度+陀螺仪），耗时仅约 0.3ms
 * @param  Data 指向数据接收结构体
 * @return 0: 成功, 1: 读取超时或失败
 */
uint8_t MPU6050_ReadData(MPU6050_Data_t *Data) {
    uint8_t recBuf[14];

    // 从 ACCEL_XOUT_H (0x3B) 开始连续读取 14 字节
    // 异步系统优化：Timeout 设为 5ms，防止总线死锁
    if (HAL_I2C_Mem_Read(MPU6050_I2C_HANDLE, MPU6050_ADDR, 0x3B, 1, recBuf, 14, 5) == HAL_OK) {
        
        int16_t rawAccX  = (int16_t)(recBuf[0]  << 8 | recBuf[1]);
        int16_t rawAccY  = (int16_t)(recBuf[2]  << 8 | recBuf[3]);
        int16_t rawAccZ  = (int16_t)(recBuf[4]  << 8 | recBuf[5]);
        int16_t rawTemp  = (int16_t)(recBuf[6]  << 8 | recBuf[7]);
        int16_t rawGyroX = (int16_t)(recBuf[8]  << 8 | recBuf[9]);
        int16_t rawGyroY = (int16_t)(recBuf[10] << 8 | recBuf[11]);
        int16_t rawGyroZ = (int16_t)(recBuf[12] << 8 | recBuf[13]);

        // 转换为物理单位 (根据 ±4g 与 ±500°/s 的灵敏度系数)
        Data->AccX  = rawAccX  / 8192.0f;
        Data->AccY  = rawAccY  / 8192.0f;
        Data->AccZ  = rawAccZ  / 8192.0f;
        
        Data->GyroX = rawGyroX / 65.5f;
        Data->GyroY = rawGyroY / 65.5f;
        Data->GyroZ = rawGyroZ / 65.5f;

        // 温度转换公式：(RAW / 340.0) + 36.53
        Data->Temp  = (rawTemp / 340.0f) + 36.53f;

        return 0;
    }

    return 1;
}

/**
 * @brief  配置运动/敲击中断（用于硬件触发 INT 引脚唤醒单片机）
 * @param  threshold 触发阈值，范围 1~255，越小越灵敏 (建议敲击设为 4~10)
 */
void MPU6050_Config_Motion_Interrupt(uint8_t threshold) {
    uint8_t data;

    // 1. 设置加速度计高通滤波器，滤除重力分量，仅响应加速度骤变（敲击）
    data = 0x09; // 保留 ±4g 且开启 HPF
    HAL_I2C_Mem_Write(MPU6050_I2C_HANDLE, MPU6050_ADDR, 0x1C, 1, &data, 1, 10);

    // 2. 写入运动检测加速度阈值 (MOT_THR 寄存器 0x1F)
    HAL_I2C_Mem_Write(MPU6050_I2C_HANDLE, MPU6050_ADDR, 0x1F, 1, &threshold, 1, 10);

    // 3. 写入持续时间 1ms (MOT_DUR 寄存器 0x20)
    data = 0x01;
    HAL_I2C_Mem_Write(MPU6050_I2C_HANDLE, MPU6050_ADDR, 0x20, 1, &data, 1, 10);

    // 4. 配置 INT 引脚：高电平有效，产生 50us 脉冲，任意读取自动清除中断
    data = 0x10;
    HAL_I2C_Mem_Write(MPU6050_I2C_HANDLE, MPU6050_ADDR, 0x37, 1, &data, 1, 10);

    // 5. 使能运动检测中断 (INT_ENABLE 寄存器 0x38 Bit6)
    data = 0x40;
    HAL_I2C_Mem_Write(MPU6050_I2C_HANDLE, MPU6050_ADDR, 0x38, 1, &data, 1, 10);
}
//void MPU6050_Clear_Motion_Interrupt(void) {
//    uint8_t data;

//    // 1. 读 INT_STATUS(0x3A) 清中断标志 —— 关键！
//    //    不清的话 MOT_INT 一直挂着，下次敲击不会再发脉冲，等于只有第一次能唤醒
//    HAL_I2C_Mem_Read(MPU6050_I2C_HANDLE, MPU6050_ADDR, 0x3A, 1, &data, 1, 10);

//    // 2. 关运动中断使能 INT_ENABLE(0x38) = 0
//    //    否则唤醒后正常使用时，一动它就在 PA1 上脉冲
//    //data = 0x00;
//    //HAL_I2C_Mem_Write(MPU6050_I2C_HANDLE, MPU6050_ADDR, 0x38, 1, &data, 1, 10);

//    // 3. 恢复加速度计配置 ACCEL_CONFIG(0x1C) = 0x08，关高通滤波
//    //    因为 Config_Motion_Interrupt 把它设成了 0x09（开了 HPF，会滤掉重力），
//    //    不恢复的话唤醒后趋势页 MPU6050_ReadData 读到的加速度是不带重力的值
//    data = 0x08;
//    HAL_I2C_Mem_Write(MPU6050_I2C_HANDLE, MPU6050_ADDR, 0x1C, 1, &data, 1, 10);
//}
void MPU6050_Clear_Motion_Interrupt(void) {
    uint8_t data;

    // 只读 INT_STATUS(0x3A) 清中断标志。
    // 因为 INT_PIN_CFG(0x37) 里 INT_RD_CLEAR=1，读一次 0x3A 标志就会自动清掉，
    // 下一次运动又能再次触发 INT，不需要重新配置。
    // 注意：不要写 INT_ENABLE(0x38)=0，否则就把运动中断整个关了。
    HAL_I2C_Mem_Read(MPU6050_I2C_HANDLE, MPU6050_ADDR, 0x3A, 1, &data, 1, 10);
}
void Motion_Enable_EXTI_Wakeup(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;  // 上升沿
    GPIO_InitStruct.Pull = GPIO_NOPULL;          // MPU6050 推挽输出，不用上拉
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_1);        // 清挂起，防止一进 STOP 就被唤
    HAL_NVIC_SetPriority(EXTI1_IRQn, 0x0F, 0);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);
}

void Motion_Disable_EXTI_Wakeup(void) {
    HAL_NVIC_DisableIRQ(EXTI1_IRQn);
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_1);
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

