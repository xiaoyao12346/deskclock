#include "env_sensor.h"
#include <math.h>
extern uint32_t tick_now;   /* main.c 主循环每轮开头采样（阻塞等待处仍用HAL_GetTick） */

#define FILTER_ALPHA 0.15f 

// BMP280 官方校准参数
static uint16_t dig_T1;
static int16_t  dig_T2, dig_T3;
static uint16_t dig_P1;
static int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static int32_t  t_fine;
 float _3hours_humi[60],_3hours_temp[60],_3hours_press[60];
static Env_Data_t clean_value;        // 第一级限幅滤波后的数据
static Env_Data_t second_clean_value; // 第二级低通滤波处理后的数据
static	uint8_t is_first_run = 1;
Env_Data_t Complete_cleaning;
Env_Data_t g_current_env = {0};   /* filtered result exported to the UI */

// 异步状态机枚举
typedef enum {
    ENV_STATE_TRIGGER = 0,
    ENV_STATE_COLLECT
} Env_State_t;

/**
 * @brief  初始化环境传感器模块 (上电运行一次，不阻塞主循环)
 * @return 0: 成功, 其他: 失败
 */
uint8_t Env_Sensors_Init(void) {
    uint8_t err = 0;
    uint8_t readBuf, id = 0, calib[24];
    uint8_t ctrl_meas = 0x27; // Temp oversampling x1, Pressure oversampling x1, Normal mode
    uint8_t config = 0xA0;    // Standby 1000ms, Filter off

    // ================= 1. 初始化 AHT20 =================
    HAL_Delay(40); // 上电需要等待40ms
    HAL_I2C_Master_Receive(ENV_I2C_HANDLE, AHT20_ADDR, &readBuf, 1, 100);
    
    // 若未标定 (Bit3 != 1)，则发送初始化命令
    if ((readBuf & 0x08) == 0x00) {
        uint8_t initCmd[3] = {0xBE, 0x08, 0x00};
        HAL_I2C_Master_Transmit(ENV_I2C_HANDLE, AHT20_ADDR, initCmd, 3, 100);
        HAL_Delay(10);
    }

    // ================= 2. 初始化 BMP280 =================
    // 读取芯片 ID，应当是 0x58
    HAL_I2C_Mem_Read(ENV_I2C_HANDLE, BMP280_ADDR, 0xD0, 1, &id, 1, 100);
    if (id != 0x58) err++;

    // 写入配置参数
    HAL_I2C_Mem_Write(ENV_I2C_HANDLE, BMP280_ADDR, 0xF4, 1, &ctrl_meas, 1, 100);
    HAL_I2C_Mem_Write(ENV_I2C_HANDLE, BMP280_ADDR, 0xF5, 1, &config, 1, 100);

    // 读取 24 字节工厂校准数据
    HAL_I2C_Mem_Read(ENV_I2C_HANDLE, BMP280_ADDR, 0x88, 1, calib, 24, 100);

    // 解析温度与气压校准数据
    dig_T1 = (calib[1] << 8) | calib[0];   dig_T2 = (calib[3] << 8) | calib[2];
    dig_T3 = (calib[5] << 8) | calib[4];   dig_P1 = (calib[7] << 8) | calib[6];
    dig_P2 = (calib[9] << 8) | calib[8];   dig_P3 = (calib[11] << 8) | calib[10];
    dig_P4 = (calib[13] << 8) | calib[12]; dig_P5 = (calib[15] << 8) | calib[14];
    dig_P6 = (calib[17] << 8) | calib[16]; dig_P7 = (calib[19] << 8) | calib[18];
    dig_P8 = (calib[21] << 8) | calib[20]; dig_P9 = (calib[23] << 8) | calib[22];

    return err;
}

/**
 * @brief  异步状态机读取传感器数据 (非阻塞)
 * @param  Data 指向数据结构体
 * @return 1: 本次数据更新完成; 0: 仍在后台转换或未完成
 */
uint8_t Env_Sensors_Read_Async(Env_Data_t *Data) {
    static Env_State_t current_state = ENV_STATE_TRIGGER;
    static uint32_t aht20_start_tick = 0;
    
    uint8_t aht_cmd[3] = {0xAC, 0x33, 0x00};
    uint8_t recData[7];

    switch (current_state) {
        
        case ENV_STATE_TRIGGER:
            // 1. 触发 AHT20 开始温湿度转换 (下发指令后立即返回)
            HAL_I2C_Master_Transmit(ENV_I2C_HANDLE, AHT20_ADDR, aht_cmd, 3, 10);
            
            // 2. 顺便读取 BMP280 气压与温度 (BMP280硬件在后台一直自动转换)
            if(HAL_I2C_Mem_Read(ENV_I2C_HANDLE, BMP280_ADDR, 0xF7, 1, recData, 6, 10) == HAL_OK) {
                int32_t adc_P = (recData[0] << 12) | (recData[1] << 4) | (recData[2] >> 4);
                int32_t adc_T = (recData[3] << 12) | (recData[4] << 4) | (recData[5] >> 4);
                int32_t var1, var2;
                uint32_t p;
                
                // Bosch 官方补偿算法
                var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
                var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
                t_fine = var1 + var2;
                
                var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
                var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)dig_P6);
                var2 = var2 + ((var1 * ((int32_t)dig_P5)) << 1);
                var2 = (var2 >> 2) + (((int32_t)dig_P4) << 16);
                var1 = ((((int32_t)dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((int32_t)dig_P2) * var1) >> 1)) >> 18;
                var1 = ((((32768 + var1)) * ((int32_t)dig_P1)) >> 15);
                
                if (var1 != 0) {
                    p = (((uint32_t)(((int32_t)1048576) - adc_P) - (var2 >> 12))) * 3125;
                    if (p < 0x80000000) p = (p << 1) / ((uint32_t)var1);
                    else                p = (p / (uint32_t)var1) * 2;
                    var1 = (((int32_t)dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
                    var2 = (((int32_t)(p >> 2)) * ((int32_t)dig_P8)) >> 13;
                    p = (uint32_t)((int32_t)p + ((var1 + var2 + dig_P7) >> 4));
                    
                    Data->Pressure = p/100.0f; // 气压 (hPa)
                }
            }
            
            // 3. 记录时间，切换到等待状态
            aht20_start_tick = HAL_GetTick();
            current_state = ENV_STATE_COLLECT;
            return 0; // 转换未完成

        case ENV_STATE_COLLECT:
            // 检查 80ms 是否已经过去
            if ((HAL_GetTick() - aht20_start_tick) >= 80) {
                // 4. 读取 AHT20 温湿度数据
                if (HAL_I2C_Master_Receive(ENV_I2C_HANDLE, AHT20_ADDR, recData, 7, 10) == HAL_OK) {
                    // 检查状态位：Bit7为0表示空闲/数据有效
                    if ((recData[0] & 0x80) == 0) { 
                        uint32_t humiReg = ((uint32_t)recData[1] << 12) | ((uint32_t)recData[2] << 4) | (recData[3] >> 4);
                        uint32_t tempReg = (((uint32_t)(recData[3] & 0x0F)) << 16) | ((uint32_t)recData[4] << 8) | recData[5];
                        
                        Data->Humidity = (float)humiReg * 100.0f / 1048576.0f;
                        Data->Temperature = (float)tempReg * 200.0f / 1048576.0f - 50.0f;
                    }
                }
                
                // 5. 准备下一次触发
                current_state = ENV_STATE_TRIGGER;
                return 1; // 返回 1 表示数据获取完成
            }
            return 0; // 时间还没到，直接退出避免占用 CPU
    }
    
    return 0;
}


void values_filtering(Env_Data_t *value){//正常速度检测
	
	//限幅滤波----
	if (is_first_run) {//只执行一次
        clean_value.Humidity = value->Humidity;
				clean_value.Pressure = value->Pressure;
        clean_value.Temperature = value->Temperature;
				second_clean_value.Humidity = value->Humidity;
				second_clean_value.Pressure=value->Pressure;
				second_clean_value.Temperature=value->Temperature;
        is_first_run = 0;
        return;
    }
	float Hum_delta = fabsf(value->Humidity - clean_value.Humidity);
	float Temper_delta = fabsf(value->Temperature - clean_value.Temperature);
	float Pres_delta = fabsf(value->Pressure - clean_value.Pressure);
	
	if(Hum_delta<2 && value->Humidity >= 0.0f && value->Humidity <= 100.0f){
			clean_value.Humidity=value->Humidity;
	}
	if(Temper_delta < 0.5f && value->Temperature >= -40.0f && value->Temperature <= 85.0f){
			clean_value.Temperature=value->Temperature;
	}
	if(Pres_delta < 0.3f && value->Pressure >= 800.0f && value->Pressure <= 1100.0f){
			clean_value.Pressure=value->Pressure;
	}
		//低通滤波----// 一阶低通滤波公式：Yn = alpha * Xn + (1 - alpha) * Yn-1
	second_clean_value.Humidity = FILTER_ALPHA * clean_value.Humidity + (1.0f - FILTER_ALPHA) * second_clean_value.Humidity;
	second_clean_value.Temperature = FILTER_ALPHA * clean_value.Temperature + (1.0f - FILTER_ALPHA) * second_clean_value.Temperature;
	second_clean_value.Pressure = FILTER_ALPHA * clean_value.Pressure + (1.0f - FILTER_ALPHA) * second_clean_value.Pressure;
	
		value->Humidity = second_clean_value.Humidity;
    value->Temperature = second_clean_value.Temperature;
    value->Pressure = second_clean_value.Pressure;
}
void values_filtering_slow(Env_Data_t *value) {
    static Env_Data_t prev_slow = {0};
		uint32_t timeout_start = HAL_GetTick();
		while (Env_Sensors_Read_Async(value) == 0) {
        if ((HAL_GetTick() - timeout_start) > 200) {
            return; 
        }
    }

    if (value->Temperature < -40.0f || value->Temperature > 85.0f)  value->Temperature = prev_slow.Temperature;
    if (value->Humidity    < 0.0f   || value->Humidity    > 100.0f) value->Humidity    = prev_slow.Humidity;
    if (value->Pressure    < 800.0f || value->Pressure    > 1100.0f) value->Pressure    = prev_slow.Pressure;
}
void sensor_run(void){
	static uint32_t sensor_time=0;
	static uint32_t display_queue_time=0; 
	static Env_Data_t  EnvData={0};
	static uint8_t  env_update_flag = 0; // 标记是否开始新一轮环境采集
	static float sum_press = 0.0f;	
	static float sum_humi  = 0.0f;
	static float sum_temp  = 0.0f;
	static uint16_t sample_count = 0,is_array_inited=0;
		if(tick_now-sensor_time>=1000){
		 sensor_time = tick_now;
         env_update_flag = 1; 
		}
		if (env_update_flag == 1) {
       if (Env_Sensors_Read_Async(&EnvData) == 1) //读取值
         {
                env_update_flag = 0; 
                values_filtering(&EnvData);//过滤值
                g_current_env = EnvData;
								sample_count++;
								if (is_array_inited == 0) {//第一次读取到数据填充。只执行一次
									for (int i = 0; i < 60; i++) {
											_3hours_humi[i]  = EnvData.Humidity;
											_3hours_temp[i]  = EnvData.Temperature;
											_3hours_press[i] = EnvData.Pressure;
									}
									is_array_inited = 1; 
								}
								sum_humi+=EnvData.Humidity;
								sum_temp+=EnvData.Temperature;
								sum_press+=EnvData.Pressure;
          }
     }
		if(tick_now-display_queue_time>=180000){//3分钟执行一次
			display_queue_time=tick_now;
				if (sample_count > 0) {
				for(uint8_t i=0;i<59;i++){//右边往左边挪数据
					_3hours_humi[i]=_3hours_humi[i+1];
					_3hours_temp[i]=_3hours_temp[i+1];
					_3hours_press[i]=_3hours_press[i+1];
				}
				_3hours_humi[59]=sum_humi/sample_count;
				_3hours_temp[59]=sum_temp/sample_count;
				_3hours_press[59]=	sum_press/sample_count;
				sample_count=0;
				sum_press=0;
				sum_temp=0;
				sum_humi=0;
			}
		}
}
/*
				//59为最新数据，0为3小时前数据
				//天气预测
				//当前气压-3小时前气压   气压差<-2          ------气压骤降，强降水/暴雨（快速下降）。
				
				//当前气压-3小时前气压   -0.5>气压差>-2     ------气压下降，天气转阴或小雨。
				
				//当前气压-3小时前气压   -0.5>气压差>+0.5   ------气压平稳，保持现状（多云/阴）。
		
				//当前气压-3小时前气压   气压差>+0.5        ------气压上升，天气转晴。
*/
/**
 * @brief 基于气压及其变化率的高精度天气预测
 * @param press_array 60个元素的气压数组 (单位: hPa，3分钟采样一次，共3小时)
 */
Weather_State_t Predict_Weather(float* press_array) {
    // 1. 获取不同时间节点的数据
    float p_start   = press_array[0];    // 3小时前气压
    float p_mid     = press_array[30];   // 1.5小时前气压
    float p_recent  = press_array[39];   // 1小时前气压 (第39个元素)
    float p_curr    = press_array[59];   // 当前最新气压

    // 2. 计算动态指标
    float delta_3h  = p_curr - p_start;  // 3小时内气压变化量
    float delta_1h  = p_curr - p_recent; // 近1小时气压变化量 (捕捉突变)

    // 计算凹凸度 (实际中间气压 - 线性插值中间气压)
    // curvature < 0 : 加速下降(凹曲线)，恶劣天气信号
    // curvature > 0 : 减速止跌(凸曲线)，气压回升/趋于平稳
    float curvature = p_mid - ((p_start + p_curr) / 2.0f);

    // 3. 基础气压补偿 (根据基础气压高低微调动态阈值)
    // 低压环境下更容易降水，高压环境下更容易转晴
    float base_offset = 0.0f;
    if (p_curr < 1005.0f) {
        base_offset = 0.4f;  // 低压环境下相当于将降水判断阈值降低 0.4 hPa
    } else if (p_curr > 1020.0f) {
        base_offset = -0.4f; // 高压环境下提高阈值加速判定转晴
    }

    float adj_delta_3h = delta_3h + base_offset; // 修正后的 3小时 气压差

    // 4. 多维度气压决策树

    // 分支 1：气压骤降 / 快速下跌
    if (adj_delta_3h <= -2.0f || delta_1h <= -1.2f) {
        // 下跌仍在加速(凹曲线)或近 1小时内剧烈下跌 1.5 hPa 以上 -> 暴雨/风暴
        if (curvature < -0.15f || delta_1h <= -1.5f) {
            return WEATHER_STORM;
        } else {
            return WEATHER_RAIN;
        }
    }
    
    // 分支 2：中度降压
    else if (adj_delta_3h < -0.6f) {
    // 如果气压虽然在降低但放慢速度出现凸曲线(减速)，且近1小时平稳，说明气压触底回稳 -> 多云
        if (curvature > 0.3f && delta_1h > -0.15f) {
            return WEATHER_CLOUDY; 
        }
        return WEATHER_RAIN;
    }
    
    // 分支 3：气压平稳
    else if (adj_delta_3h <= 0.5f) {
		// 虽然 3小时 整体平稳，但如果近 1小时 有缓慢微降 -> 阴天预警
        if (delta_1h < -0.3f) {
            return WEATHER_OVERCAST;
        }
        return WEATHER_CLOUDY;
    }
    
    // 分支 4：气压上升
    else {
        return WEATHER_SUNNY; // 气压上升，天气转晴
    }
}
/**
 * @brief 基于气压数据计算降水概率 (0% - 100%)
 * @param press_array 60个元素的气压数组 (单位: hPa，3分钟采样一次，共3小时)
 * @return float 降水概率百分比 (0.0 - 100.0)
 */
float Calculate_Rain_Probability(float* press_array) {
    // 1. 获取基本数据 (与您的预测函数逻辑一致)
    float p_start   = press_array[0];    // 3小时前
    float p_curr    = press_array[59];   // 当前
    float p_recent  = press_array[39];   // 1小时前
    
    float delta_3h  = p_curr - p_start;
    float delta_1h  = p_curr - p_recent;

    float probability = 0.0f;

    // --- 步骤 1: 根据当前绝对气压计算基础概率 ---
    // 设定 1025 hPa 为绝对晴天(0%)，1000 hPa 为标准低压(50%基础降水率)
    // 每降低 1 hPa，基础降水率增加 2%
    float base_prob = (1025.0f - p_curr) * 2.0f;
    
    // 限制基础概率在 0% ~ 60% 之间
    if (base_prob < 0.0f) base_prob = 0.0f;
    if (base_prob > 60.0f) base_prob = 60.0f;

    // --- 步骤 2: 根据 3小时气压变化计算趋势加成（核心权重） ---
    // 经验模型：气压每下降 1 hPa，降水概率增加 25%；上升则减少
    // 例如：下降 2 hPa，直接加 50% 概率
    float trend_prob = -delta_3h * 25.0f;

    // --- 步骤 3: 短期突降/风暴加成 ---
    // 如果近1小时气压出现急降（低于 -1.0 hPa），说明对流天气逼近，额外增加概率
    float burst_prob = 0.0f;
    if (delta_1h <= -1.0f) {
        burst_prob = 20.0f; 
    } else if (delta_1h <= -0.5f) {
        burst_prob = 10.0f;
    }

    // --- 步骤 4: 综合计算与限幅 ---
    probability = base_prob + trend_prob + burst_prob;

    // 确保最终输出在 0 到 100 之间
    if (probability > 100.0f) probability = 100.0f;
    if (probability < 0.0f)   probability = 0.0f;

    return probability;
}


/**
 * @brief 通用露点温度计算 (Magnus-Tetens 公式)
 * @param temp 温度 (℃)
 * @param rh   相对湿度 (%)
 * @return float 露点温度 (℃)
 */
float Calculate_Dew_Point(float temp, float rh) {
    if (rh <= 0.0f) rh = 0.1f;
    if (rh > 100.0f) rh = 100.0f;

    const float a = 17.27f;
    const float b = 237.7f;

    float alpha = ((a * temp) / (b + temp)) + logf(rh / 100.0f);
    return (b * alpha) / (a - alpha);
}
/**
 * @brief 环境综合评估与词条生成
 * @param temp_in  室内温度 (℃)
 * @param rh_in    室内湿度 (%)
 * @param temp_out 室外温度 (℃)
 * @param rh_out   室外湿度 (%)
 * @return Env_Result_t 包含所有状态词条的结构体
 */
Env_Brief_Result_t Evaluate_Brief_Environment(float temp_in, float rh_in, float temp_out, float rh_out) {
    Env_Brief_Result_t res;

    // 1. 计算露点及差值
    float dew_in = Calculate_Dew_Point(temp_in, rh_in);
    float dew_out = Calculate_Dew_Point(temp_out, rh_out);
    float temp_diff = temp_out - temp_in; 
    float dew_diff = dew_out - dew_in;    // 绝对水汽差：>0代表室外水汽比室内多

    // 2. 核心判定树（按严重程度从高到低拦截）

    // [优先级 1]: 极端天气
    if (temp_out >= 38.0f) {
        res.condition = "室外偏热"; res.advice = "避免外出"; return res;
    } 
    if (temp_out <= -10.0f) {
        res.condition = "室外偏冷"; res.advice = "避免外出"; return res;
    }

    // [优先级 2]: 物理结露现象 (回南天水汽倒灌，或冬季玻璃起雾滴水)
    if (dew_out > temp_in || temp_out < dew_in) {
        res.condition = "易结露"; res.advice = "建议关窗"; return res;
    }

    // [优先级 3]: 湿度异常
    if (rh_out > 85.0f || dew_diff > 3.0f) {
        // 室外绝对水汽极高或大雨，防止湿气入侵
        res.condition = "室外偏潮"; res.advice = "建议关窗"; return res;
    } 
    if (rh_in > 70.0f) {
        res.condition = "室内偏潮";
        // 如果室内潮，且室外比室内干，就开窗用自然风除湿；否则关窗开机器除湿
        res.advice = (dew_diff < -2.0f) ? "建议开窗" : "开启除湿"; 
        return res;
    }
    if (rh_in < 35.0f) {
        res.condition = "室内偏干"; res.advice = "开启加湿"; return res;
    }

    // [优先级 4]: 温度异常
    if (temp_in > 28.0f) {
        res.condition = "室内偏热"; res.advice = "开启空调"; return res;
    }
    if (temp_in < 15.0f) {
        res.condition = "室内偏冷"; res.advice = "开启暖气"; return res;
    }
    if (fabs(temp_diff) >= 8.0f) {
        res.condition = "温差过大"; res.advice = "建议关窗"; return res;
    }
    if (rh_out < 30.0f) {
        res.condition = "室外偏干"; res.advice = "建议关窗"; return res;
    }

    res.condition = "湿度适宜";
    res.advice    = "适宜通风";
    
    return res;
}
#define mode 4
#if(mode==0)
//�޷��˲��㷨
#define max 0.2
Env_Data_t last_value;
uint8_t is_first_run = 1;
void values_filtering(Env_Data_t *value){
	if (is_first_run) {
        last_value.Humidity = value->Humidity;
        is_first_run = 0;
        return;
    }	
	float delta = fabsf(value->Humidity - last_value.Humidity);
	if(delta>max){
		//���ݲ�����
	}
	else{
			//������������ȡ��ֵ,��������һ��ȫ�ֱ������и�ֵ
		last_value.Humidity=value->Humidity;
	}
}
#elif(mode==1)
//��ֵ�˲��㷨
void swap(float *a,float *b){
	float temp;
	temp=*a;
	*a=*b;
	*b=temp;
}
#define N 5
uint8_t values_buff_count=0;
float temp_values_buff[N];
float new_temp;
void values_filtering(Env_Data_t *value) {
    temp_values_buff[values_buff_count++] = value->Temperature;
    if (values_buff_count >= N) {
        values_buff_count = 0; 
        float sort_buff[N];
        for (uint8_t k = 0; k < N; k++) sort_buff[k] = temp_values_buff[k];

        for (uint8_t i = 0; i < N - 1; i++) {
            for (uint8_t j = 0; j < N - 1 - i; j++) {
                if (sort_buff[j] > sort_buff[j + 1]) {
                    swap(&sort_buff[j], &sort_buff[j + 1]);
                }
            }
        }
        new_temp = sort_buff[N / 2];
    }
}
#elif(mode==2)
//����ƽ���˲��㷨///����FIFO
typedef struct {
    uint8_t head;       // д��ָ��
    float Ring_Buff[5]; // ����Ϊ 5 �Ļ��λ�����
    float sum;          // �����ڲ������ܺ�
} Ring_Filtering_t;

Ring_Filtering_t ringfiltering = {0};

float values_filtering(Env_Data_t *value) {
	ringfiltering.sum=0;
	for(uint8_t i=4;i>=1;i--){
		ringfiltering.Ring_Buff[i]=ringfiltering.Ring_Buff[i-1];
	}
	ringfiltering.Ring_Buff[0]=value->Temperature;
	for(uint8_t i=0;i<5;i++)
		ringfiltering.sum+=ringfiltering.Ring_Buff[i];
	return ringfiltering.sum/5;
}
#elif(mode==3)
//һ�׵�ͨ�˲���
// alpha ȡֵ 0.0f ~ 1.0f ֮�䡣�¶ȴ������Ƽ� 0.1f ~ 0.2f
#define FILTER_ALPHA 0.15f 
float lpf_values_filtering(Env_Data_t *value) {
    static float prev_filter_temp = 0.0f; // ��¼��һ�ε��˲�ֵ Yn-1
    static uint8_t is_first_run = 1;
    // �ϵ��״�����ֱ�Ӹ�ֵ����ֹ�� 0 ��������
    if (is_first_run) {
        prev_filter_temp = value->Temperature;
        is_first_run = 0;
        return prev_filter_temp;
    }
    // һ�׵�ͨ�˲���ʽ��Yn = alpha * Xn + (1 - alpha) * Yn-1
    prev_filter_temp = FILTER_ALPHA * value->Temperature + (1.0f - FILTER_ALPHA) * prev_filter_temp;
    return prev_filter_temp;
}
#elif(mode==4)
#endif


