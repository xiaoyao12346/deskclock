#include "my_wifi.h"
#include "my_even.h"
#include "ssd1322.h"
#include "env_sensor.h"

char AT_CMD[300];
char rx_json_buf[4096];
uint8_t wifi_rx;
uint32_t now = 0;
uint32_t wifi_delay = 0;
static uint8_t match_index = 0;
static uint16_t rx_json_len = 0;
Wifi_Config_t wifi_flash_config;
Weather_Info_t g_weather;//解析出来的天气信息
static uint8_t wifi_save_failed = 0;   /* 网页保存配置时 Flash 写入是否失败 */
static uint8_t  flash_diag_stage = 0;  /* Flash 写失败阶段：1=擦除 2=写天气 3=写配置 4=校验天气 5=校验配置 */
static uint32_t flash_diag_sr = 0;     /* 失败时 FLASH->SR 错误标志 */
static uint32_t flash_diag_optcr = 0;  /* 选项字节：检查 nWRP 写保护位 */
Time_Info_t g_time;//解析出来的时间
 WiFi_State_t 	g_wifi_state = WIFI_STATE_SEND_RST;
static WIFI_Setting_t g_wifi_set = WIFI_SET_SEND_AT;
static uint32_t wifi_setdelay = 0;
Sys_Wifi_Mode_t current_wifi_mode = SYS_WIFI_MODE_STA; // 默认上电进入正常获取天气模式
#if 1
void Send_AT_Cmd(const char *cmd) {
    if (huart1.gState == HAL_UART_STATE_READY) {
        HAL_UART_Transmit_IT(&huart1, (uint8_t *)cmd, strlen(cmd));
    }
}
#else
void Send_AT_Cmd(const char *cmd) {
    // 改用阻塞发送，超时时间设为 1000ms。确保长达一百多字节的 HTTP 报文能完整塞进串口发出去
    HAL_UART_Transmit(&huart1, (uint8_t *)cmd, strlen(cmd), 1000);
}
#endif

/**
 * @brief 环形缓冲区流式匹配目标字符串（非阻塞，只匹配不录制数据）
 * @param target 期望匹配的目标字符串（如 "OK"、">"）
 * @return uint8_t 1: 匹配成功；0: 未匹配成功或数据未完全到达
 */
uint8_t Match_Target_Stream(const char *target) {
    uint8_t ch;
    // 从串口环形缓冲区逐字节读取
    while (RingBuffer_ReadByte(&ch)) {
        if (ch == target[match_index]) {
            match_index++;
            if (target[match_index] == '\0') { 
                match_index = 0; // 匹配成功后重置索引
                return 1;        // 返回匹配成功
            }
        } else {
            // 匹配中途失败，判断当前字符是否符合目标首字母
            if (ch == target[0]) {
                match_index = 1;
            } else {
                match_index = 0;
            }
        }
    }
    return 0; // 缓冲区为空或尚未完整匹配
}
#include <ctype.h>

/**
 * @brief 将 JSON 返回的英文风向标识（如 "NW", "NNE"）转换为中文风向
 * @param code 传入的英文字符串（如 "NW", "nw", "N"）
 * @return 对应的中文风向字符串指针
 */
const char* get_wind_direction_cn(const char *code) {
    if (code == NULL || *code == '\0') {
        return "--";  // 空数据直接返回破折号或 "未知"
    }

    // 转大写处理，防止 API 返回小写字母（如 "nw" -> "NW"）
    char buf[8] = {0};
    for (int i = 0; code[i] != '\0' && i < 7; i++) {
        buf[i] = (char)toupper((unsigned char)code[i]);
    }

    // 8 基础方位 (最常用)
    if (strcmp(buf, "N") == 0)   return "北风";
    if (strcmp(buf, "NE") == 0)  return "东北";
    if (strcmp(buf, "E") == 0)   return "东风";
    if (strcmp(buf, "SE") == 0)  return "东南";
    if (strcmp(buf, "S") == 0)   return "南风";
    if (strcmp(buf, "SW") == 0)  return "西南";
    if (strcmp(buf, "W") == 0)   return "西风";
    if (strcmp(buf, "NW") == 0)  return "西北";

    // 16 细分方位
    if (strcmp(buf, "NNE") == 0) return "东北";
    if (strcmp(buf, "ENE") == 0) return "东北";
    if (strcmp(buf, "ESE") == 0) return "东南";
    if (strcmp(buf, "SSE") == 0) return "东南";
    if (strcmp(buf, "SSW") == 0) return "西南";
    if (strcmp(buf, "WSW") == 0) return "西南";
    if (strcmp(buf, "WNW") == 0) return "西北";
    if (strcmp(buf, "NNW") == 0) return "西北";

    // 特殊情况处理（细化以更符合气象常识）
    if (strcmp(buf, "VRB") == 0) {
        return "微风"; // VRB (Variable) 风向不定，通常风力较小，用"微风"最自然
    }
    if (strcmp(buf, "CALM") == 0 || strcmp(buf, "OFF") == 0) {
        return "无风"; // 静风状态
    }

    return "未知"; // 默认兜底，替代原来的“无持续风向”
}

/**
 * @brief 心知天气 (Seniverse) 英文天气描述转中文
 * @param en_text 心知天气返回的英文字符串 (如 "Sunny", "Partly Cloudy")
 * @return 对应的中文文本，仅包含常用单字/双字
 */
const char* weather_state_to_zh(Weather_State_t state) {
    switch (state) {
        case WEATHER_SUNNY:    return "晴";
        case WEATHER_CLOUDY:   return "多云";
        case WEATHER_OVERCAST: return "阴";
        case WEATHER_RAIN:     return "雨";
        case WEATHER_STORM:    return "暴雨";
        default:               return "晴";
    }
}
const char* weather_en_to_zh(const char *en_text)
{
	
    if (en_text == NULL || strlen(en_text) == 0) {
        return "晴"; // 默认回退显示
    }

    typedef struct {
        const char *en;
        const char *zh;
    } WeatherMap;

    // 心知天气 v3 API 所有英文 weather text 精准映射表
    static const WeatherMap map[] = {
        {"Sunny",                   "晴"},
        {"Clear",                   "晴"},
        {"Fair",                    "晴"},
        {"Cloudy",                  "多云"},
        {"Partly Cloudy",           "多云"},
        {"Mostly Cloudy",           "多云"},
        {"Overcast",                "阴"},
        {"Shower",                  "阵雨"},
        {"Showers",                 "阵雨"},
        {"Thundershower",           "雷阵雨"},
        {"Thundershower with Hail", "雷阵雨"},
        {"Light Rain",              "雨"},
        {"Moderate Rain",           "雨"},
        {"Heavy Rain",              "大雨"},
        {"Storm",                   "暴雨"},
        {"Heavy Storm",             "暴雨"},
        {"Severe Storm",            "暴雨"},
        {"Ice Rain",                "雨雪"},
        {"Sleet",                   "雨雪"},
        {"Snow Flurry",             "阵雪"},
        {"Light Snow",              "雪"},
        {"Moderate Snow",           "雪"},
        {"Heavy Snow",              "大雪"},
        {"Snowstorm",               "暴雪"},
        {"Dust",                    "雾霾"},
        {"Sand",                    "雾霾"},
        {"Duststorm",               "雾霾"},
        {"Sandstorm",               "雾霾"},
        {"Foggy",                   "雾"},
        {"Fog",                     "雾"},
        {"Haze",                    "霾"},
        {"Windy",                   "风"},
        {"Blustery",                "风"},
        {"Hurricane",               "风"},
        {"Tropical Storm",          "风"},
        {"Tornado",                 "风"},
        {"Cold",                    "阴"},
        {"Hot",                     "晴"}
    };

    uint16_t count = sizeof(map) / sizeof(map[0]);
    for (uint16_t i = 0; i < count; i++) {
        if (strcmp(en_text, map[i].en) == 0) {
            return map[i].zh;
        }
    }

    // --- 兜底容错机制：如果收到未预期的英文，按关键字提取中文 ---
    if (strstr(en_text, "Rain") || strstr(en_text, "rain")) return "雨";
    if (strstr(en_text, "Snow") || strstr(en_text, "snow")) return "雪";
    if (strstr(en_text, "Sun")  || strstr(en_text, "Clear")) return "晴";
    if (strstr(en_text, "Cloud")|| strstr(en_text, "cloud")) return "多云";
    if (strstr(en_text, "Drizzle") || strstr(en_text, "drizzle")) return "雨";
    if (strstr(en_text, "Hail") || strstr(en_text, "hail")) return "雷阵雨";
    if (strstr(en_text, "Thunder") || strstr(en_text, "thunder")) return "雷阵雨";

    return "晴"; // 无法识别时兜底显示"晴"，避免英文溢出左侧区域
}
/**
 * @brief 从 JSON 文本中提取 "token":"value" 形式的字符串 value
 * @param p       待搜索的 JSON 文本
 * @param token   完整前缀，形如 "\"name\":\""（含引号和冒号）
 * @param dst     输出缓冲区
 * @param maxlen  输出缓冲区大小（字节），实际最多写入 maxlen-1 个字符
 */
static void json_extract_str(const char *p, const char *token, char *dst, int maxlen) {
    const char *f;
    int i;

    if (p == NULL || dst == NULL || maxlen <= 1) return;

    f = strstr(p, token);
    if (f == NULL) return;

    f += strlen(token); /* 跳过前缀，指向 value 起始 */

    for (i = 0; i < maxlen - 1 && f[i] != '\0' && f[i] != '"'; i++) {
        dst[i] = f[i];
    }
    dst[i] = '\0';
}

/**
 * @brief 解析心知天气 JSON 字符串数据
 * @param buf 接收到的完整 HTTP 响应报文
 * @return uint8_t 1: 解析成功；0: 数据缺失或解析失败
 */
uint8_t Parse_Weather_Data(const char *buf) {
    const char *p;
    const char *h;

    p = strstr(buf, "\"results\"");
    if (p == NULL) return 0;

    /* 城市名称 */
    json_extract_str(p, "\"name\":\"", g_weather.city, sizeof(g_weather.city));

    /* daily 数组 */
    p = strstr(p, "\"daily\":[");
    if (p == NULL) return 0;

    for (int i = 0; i < 3; i++) {
        p = strstr(p, "{\"date\""); /* 定位每一天对象 */
        if (p == NULL) break;

        json_extract_str(p, "\"date\":\"",           g_weather.date[i],           sizeof(g_weather.date[i]));
        json_extract_str(p, "\"text_day\":\"",       g_weather.text_day[i],       sizeof(g_weather.text_day[i]));
        json_extract_str(p, "\"text_night\":\"",     g_weather.text_night[i],     sizeof(g_weather.text_night[i]));
        json_extract_str(p, "\"high\":\"",           g_weather.high[i],           sizeof(g_weather.high[i]));
        json_extract_str(p, "\"low\":\"",            g_weather.low[i],            sizeof(g_weather.low[i]));
        json_extract_str(p, "\"rainfall\":\"",       g_weather.rainfall[i],       sizeof(g_weather.rainfall[i]));
        json_extract_str(p, "\"precip\":\"",         g_weather.precip[i],         sizeof(g_weather.precip[i]));
        json_extract_str(p, "\"wind_direction\":\"", g_weather.wind_direction[i], sizeof(g_weather.wind_direction[i]));
        json_extract_str(p, "\"wind_speed\":\"",     g_weather.wind_speed[i],     sizeof(g_weather.wind_speed[i]));
        json_extract_str(p, "\"wind_scale\":\"",     g_weather.wind_scale[i],     sizeof(g_weather.wind_scale[i]));

        /* humidity 是每个 day 对象的最后一个字段，借它的位置推进到下一轮 */
        h = strstr(p, "\"humidity\":\"");
        if (h == NULL) break;
        json_extract_str(h, "\"humidity\":\"", g_weather.humidity[i], sizeof(g_weather.humidity[i]));
        p = h + strlen("\"humidity\":\"");
    }

    return 1;
}
/**
 * @brief 在边匹配目标字符串的同时，将读取到的所有字节存入输出数组（用于接收完整的 HTTP 数据）
 * @param target 匹配的目标标志字符串（如 "CLOSED"）
 * @param out 数据保存的目标缓冲区数组指针
 * @param out_size 目标缓冲区最大字节容量
 * @return uint8_t 1: 遇到目标字符串，接收结束；0: 未遇到
 */
uint8_t Match_Target_And_Capture(const char *target, char *out, uint16_t out_size) {
    uint8_t ch;
    uint8_t found = 0;
    
    // 循环从环形缓冲区弹出字符
    while (RingBuffer_ReadByte(&ch)) {
        // 只要没超出缓存容量，就将字符录入 out 数组
        if (rx_json_len < out_size - 1) {
            out[rx_json_len++] = (char)ch;
        }
        // 同步进行目标串 ("CLOSED") 的匹配
        if (ch == target[match_index]) {
            match_index++;
            if (target[match_index] == '\0') {
                match_index = 0;
                found = 1; // 匹配成功
            }
        } else {
            if (ch == target[0]) {
                match_index = 1;
            } else {
                match_index = 0;
            }
        }
    }
    
    // 字符串末尾及时补充 '\0' 结束符，使其保持合法字符串格式
    if (rx_json_len < out_size) {
        out[rx_json_len] = '\0';
    }
    return found;
}



static uint8_t Month_StrToNum(const char *str) {
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    for (uint8_t i = 0; i < 12; i++) {
        if (strncmp(str, months[i], 3) == 0) return i + 1;
    }
    return 1;
}

static uint8_t Week_StrToNum(const char *str) {
    const char *weeks[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
    for (uint8_t i = 0; i < 7; i++) {
        if (strncmp(str, weeks[i], 3) == 0) return i + 1;
    }
    return 1;
}

static uint8_t Parse_SNTP_Time(const char *buf) {//解析时间
    const char *p = strstr(buf, "+CIPSNTPTIME:");
    if (p == NULL) return 0;
    
    p += 13; // 跳过 "+CIPSNTPTIME:" 头部
    
    // 检查是否还没同步成功（未同步时通常返回 1970）
    if (strstr(p, "1970") != NULL) return 0; 
    
    char week_str[8] = {0};
    char month_str[8] = {0};
    int day = 0, hour = 0, min = 0, sec = 0, year = 0;
    
    // 从 "Wed Aug 26 16:00:00 2026" 提取各个独立字段
    if (sscanf(p, "%s %s %d %d:%d:%d %d", week_str, month_str, &day, &hour, &min, &sec, &year) == 7) {
        g_time.year  = (uint16_t)year;
        g_time.month = Month_StrToNum(month_str);
        g_time.date  = (uint8_t)day;
        g_time.hour  = (uint8_t)hour;
        g_time.min   = (uint8_t)min;
        g_time.sec   = (uint8_t)sec;
        g_time.week  = Week_StrToNum(week_str);
        
        return 1; // 解析成功
    }
    
    return 0; // 解析失败
}
/**
 * @brief 从 HTTP GET 请求中提取 WiFi 配置信息
 * @param http_req 接收到的 HTTP 请求字符串 (如 "GET /save?s=xxx&p=yyy&c=zzz HTTP/1.1")
 * @param config 用于保存提取结果的 WiFi 配置结构体指针
 * @return uint8_t 1: 提取成功; 0: 提取失败(参数不全)
 */
uint8_t Parse_Wifi_Config_From_HTTP(const char *http_req, Wifi_Config_t *config) {
    char *s_ptr = strstr(http_req, "s=");
    char *p_ptr = strstr(http_req, "&p=");
    char *c_ptr = strstr(http_req, "&c=");

    // 检查是否三个参数都存在
    if (!s_ptr || !p_ptr || !c_ptr) return 0; 

    s_ptr += 2; // 跳过 "s="
    p_ptr += 3; // 跳过 "&p="
    c_ptr += 3; // 跳过 "&c="

    uint16_t i;

    // 1. 提取 SSID (遇到 '&', 空格 或 结束符 停止)
    for (i = 0; i < sizeof(config->ssid) - 1; i++) {
        if (s_ptr[i] == '&' || s_ptr[i] == ' ' || s_ptr[i] == '\0') break;
        config->ssid[i] = s_ptr[i];
    }
    config->ssid[i] = '\0';

    // 2. 提取 Password
    for (i = 0; i < sizeof(config->pass) - 1; i++) {
        if (p_ptr[i] == '&' || p_ptr[i] == ' ' || p_ptr[i] == '\0') break;
        config->pass[i] = p_ptr[i];
    }
    config->pass[i] = '\0';

    // 3. 提取 City Code (解决原代码 strlen 会把 " HTTP/1.1" 录入的 Bug)
    for (i = 0; i < sizeof(config->city) - 1; i++) {
        if (c_ptr[i] == '&' || c_ptr[i] == ' ' || c_ptr[i] == '\0') break;
        config->city[i] = c_ptr[i];
    }
    config->city[i] = '\0';

    // 写入魔数标记
    config->magic = 0xABCD1234;

    return 1; // 解析成功
}

/* 统一进入错误态：重置5秒重试计时（否则长超时分支进入ERROR时会立即重试） */
static void Wifi_State_Error(void) {
    g_wifi_state = WIFI_STATE_ERROR;
    wifi_delay   = tick_now;
}

void wifi_handle(void){
		static uint8_t Error_count=0;
    now = tick_now;
    static char Request[300];


    switch(g_wifi_state){
        case WIFI_STATE_SEND_RST:
            match_index = 0;
            sprintf(AT_CMD, "AT+RST\r\n");
            Send_AT_Cmd(AT_CMD);
            wifi_delay = now;
            g_wifi_state = WIFI_STATE_WAIT_RST;
            break;

        case WIFI_STATE_WAIT_RST:
            if (now - wifi_delay >= 3000) {
                {   
                    uint8_t ch;
                    while (RingBuffer_ReadByte(&ch));
                }
                match_index = 0;
                g_wifi_state = WIFI_STATE_SEND_AT;
            }
            break;

        case WIFI_STATE_SEND_AT:
            match_index = 0;
            sprintf(AT_CMD, "AT\r\n");
            Send_AT_Cmd(AT_CMD);
            wifi_delay = now;
            g_wifi_state = WIFI_STATE_WAIT_AT;
            break;

        case WIFI_STATE_WAIT_AT:
            if(Match_Target_Stream("OK")){
                g_wifi_state = WIFI_STATE_SEND_CWMODE;
            } else if (now - wifi_delay >= 3000){ 
                Wifi_State_Error(); 
            }
            break;

        case WIFI_STATE_SEND_CWMODE:
            match_index = 0;
            sprintf(AT_CMD, "AT+CWMODE=1\r\n");
            Send_AT_Cmd(AT_CMD);
            wifi_delay = now;
            g_wifi_state = WIFI_STATE_WAIT_CWMODE;
            break;

        case WIFI_STATE_WAIT_CWMODE:
            if(Match_Target_Stream("OK"))
                g_wifi_state = WIFI_STATE_SEND_CIPMUX;
            else if (now - wifi_delay >= 3000)
                Wifi_State_Error();
            break;

        case WIFI_STATE_SEND_CIPMUX:
            match_index = 0;
            sprintf(AT_CMD, "AT+CIPMUX=0\r\n");
            Send_AT_Cmd(AT_CMD);
            wifi_delay = now;
            g_wifi_state = WIFI_STATE_WAIT_CIPMUX;
            break;

        case WIFI_STATE_WAIT_CIPMUX:
            if(Match_Target_Stream("OK"))
                g_wifi_state = WIFI_STATE_SEND_CWJAP;
            else if (now - wifi_delay >= 3000)
                Wifi_State_Error();
            break;

        case WIFI_STATE_SEND_CWJAP:
            match_index = 0;
            sprintf(AT_CMD, "AT+CWJAP=\"%s\",\"%s\"\r\n",wifi_flash_config.ssid,wifi_flash_config.pass);
            Send_AT_Cmd(AT_CMD);
            wifi_delay = now;
            g_wifi_state = WIFI_STATE_WAIT_CWJAP;
            break;

        case WIFI_STATE_WAIT_CWJAP:
            if(Match_Target_Stream("OK"))
                g_wifi_state = WIFI_STATE_SEND_CIPCLOSE;
            else if (now - wifi_delay >= 15000) 
                Wifi_State_Error(); 
            break;

        case WIFI_STATE_SEND_CIPCLOSE:
            match_index = 0;
            sprintf(AT_CMD, "AT+CIPCLOSE\r\n");
            Send_AT_Cmd(AT_CMD);
            wifi_delay = now;
            g_wifi_state = WIFI_STATE_WAIT_CIPCLOSE;
            break;

        case WIFI_STATE_WAIT_CIPCLOSE://断开旧连接->等 1 秒  →  发送 AT+CIPSTART（重连）
            if (now - wifi_delay >= 1000) {
                g_wifi_state = WIFI_STATE_SEND_CIPSTART;
            }
            break;

        case WIFI_STATE_SEND_CIPSTART:
            match_index = 0;
            {   
                uint8_t ch;
                while (RingBuffer_ReadByte(&ch));//把接收到的字节都丢掉
            }
            sprintf(AT_CMD, "AT+CIPSTART=\"TCP\",\"api.seniverse.com\",80\r\n");
            Send_AT_Cmd(AT_CMD);
            wifi_delay = now;
            g_wifi_state = WIFI_STATE_WAIT_CIPSTART;
            break;

        case WIFI_STATE_WAIT_CIPSTART:
            if(Match_Target_Stream("OK"))
                g_wifi_state = WIFI_STATE_SEND_CIPSEND;
            else if (now - wifi_delay >= 10000) 
                Wifi_State_Error();
            break;

        case WIFI_STATE_SEND_CIPSEND:
            match_index = 0;
            /* 心知天气 API Key：请到 https://www.seniverse.com 免费注册后替换 YOUR_API_KEY */
            sprintf(Request, "GET /v3/weather/daily.json?key=YOUR_API_KEY&location=%s&language=en&unit=c&start=0&days=3 HTTP/1.1\r\nHost: api.seniverse.com\r\nConnection: close\r\n\r\n", wifi_flash_config.city);
            sprintf(AT_CMD, "AT+CIPSEND=%d\r\n", (int)strlen(Request));//发送请求的长度
            Send_AT_Cmd(AT_CMD);
            wifi_delay = now;
            g_wifi_state = WIFI_STATE_WAIT_PROMPT;
            break;

        case WIFI_STATE_WAIT_PROMPT:
            if(Match_Target_Stream(">"))
                g_wifi_state = WIFI_STATE_SEND_REQUEST;
            else if (now - wifi_delay >= 15000) 
                Wifi_State_Error();
            break;

        case WIFI_STATE_SEND_REQUEST:
            match_index = 0;
            rx_json_len = 0;
            Send_AT_Cmd(Request);
            wifi_delay = now;
            g_wifi_state = WIFI_STATE_WAIT_RESPONSE;
            break;

        case WIFI_STATE_WAIT_RESPONSE:
            if(Match_Target_And_Capture("CLOSED", rx_json_buf, sizeof(rx_json_buf)))//把解析报文存进rx_json_buf
                g_wifi_state = WIFI_STATE_DONE;
            else if (now - wifi_delay >= 15000) {
                if (strstr(rx_json_buf, "HTTP/1.1") != NULL)
                    g_wifi_state = WIFI_STATE_DONE;
                else
                    Wifi_State_Error();
            }
            break;

        case WIFI_STATE_DONE:
            if (Parse_Weather_Data(rx_json_buf)){//解析获取天气
							g_weather.magic=0xA5A5A5A8;
							Flash_Write_Weather(&g_weather);//写进Flash
                EventQueue_Write(&ui_eventq, EV_WEATHER_UPDATED);//天气数据已更新，发事件通知UI
							g_wifi_state=WIFI_STATE_SEND_SNTP_CFG;
								wifi_delay = now;
            }
						else
							Wifi_State_Error();
            break;
						
						
						
						//请求时间
				case WIFI_STATE_SEND_SNTP_CFG:
						match_index = 0;
            sprintf(AT_CMD, "AT+CIPSNTPCFG=1,8,\"ntp1.aliyun.com\"\r\n");
            Send_AT_Cmd(AT_CMD);
            wifi_delay = now;
            g_wifi_state = WIFI_STATE_WAIT_SNTP_CFG;
					break;
				case WIFI_STATE_WAIT_SNTP_CFG:
					 if(Match_Target_Stream("OK"))
						 g_wifi_state=WIFI_STATE_SEND_SNTP_TIME;
					 else if(now-wifi_delay>=3000)
						 Wifi_State_Error();
					break;
				case WIFI_STATE_SEND_SNTP_TIME:
							rx_json_len=0;
							match_index = 0;
            sprintf(AT_CMD, "AT+CIPSNTPTIME?\r\n");
            Send_AT_Cmd(AT_CMD);
            wifi_delay = now;
            g_wifi_state = WIFI_STATE_WAIT_SNTP_TIME;
					break;
				case WIFI_STATE_WAIT_SNTP_TIME:
            if(Match_Target_And_Capture("OK", rx_json_buf, sizeof(rx_json_buf)))
						 g_wifi_state=WIFI_STATE_PARSE_SNTP_TIME;
					 else if(now-wifi_delay>=5000)
						 Wifi_State_Error();
					break;
				case WIFI_STATE_PARSE_SNTP_TIME:
					if (Parse_SNTP_Time(rx_json_buf)) {//解析获取时间
                Error_count = 0;   // 完整成功一轮，连续错误计数清零
                EventQueue_Write(&ui_eventq, EV_TIME_SYNCED);//时间已同步，发事件写RTC并通知UI
							g_wifi_state=WIFI_STATE_IDLE;
								wifi_delay = now;
            }
						else {
							if (now - wifi_delay >= 1000) {
										g_wifi_state = WIFI_STATE_SEND_SNTP_TIME; // 1秒后重新发起查询指令
										}
							}
					break;
				case WIFI_STATE_IDLE:
									current_wifi_mode=SYS_WIFI_MODE_IDLE;
					break;
        case WIFI_STATE_ERROR:
						if (now - wifi_delay >= 15000) {
                g_wifi_state = WIFI_STATE_SEND_RST;
							if(++Error_count==6){
									Error_count=0;
								current_wifi_mode=SYS_WIFI_MODE_IDLE;
							}
						}
					break;
			}


}



/* 配网流程统一进入错误态：重置5秒重试计时（否则进入ERROR时会立即重试） */
static void Wifiset_State_Error(void) {
    g_wifi_set    = WIFI_SET_ERROR;
    wifi_setdelay = tick_now;
}

void wifiset_handle(void)
	{
    now = tick_now;
    const char HTML_FORM[] =
    "<form action=\"/save\" method=\"GET\">"
    "SSID:<br><input type=\"text\" name=\"s\"><br>"
    "Pass:<br><input type=\"password\" name=\"p\"><br>"
    "City:<br><input type=\"text\" name=\"c\" value=\"\"><br><br>"
    "<input type=\"submit\" value=\"Save\">"
    "</form>";
    const char SUCCESS_HTML[] = "<h2>Saved! Reconnecting...</h2>";
    static uint8_t current_link_id = 0;
    static uint8_t is_save_req = 0; 
    static char http_buf[512];
    static uint8_t Error_count = 0;   // 配网流程连续失败计数

    switch(g_wifi_set) {
        case WIFI_SET_SEND_AT:
            match_index = 0;
            Send_AT_Cmd("AT\r\n");
            g_wifi_set = WIFI_SET_WAIT_AT;
            wifi_setdelay = now; 
            break;
            
        case WIFI_SET_WAIT_AT:
            if(Match_Target_Stream("OK")) {
                g_wifi_set = WIFI_SET_SEND_CWMODE;
            } else if(now - wifi_setdelay >= 2000) {
                Wifiset_State_Error();
            }
            break;
            
        case WIFI_SET_SEND_CWMODE:
            match_index = 0;
            Send_AT_Cmd("AT+CWMODE=2\r\n");
            g_wifi_set = WIFI_SET_WAIT_CWMODE;
            wifi_setdelay = now;
            break;
            
        case WIFI_SET_WAIT_CWMODE:
            if(Match_Target_Stream("OK")) {
                g_wifi_set = WIFI_SET_SEND_CWSAP;
            } else if(now - wifi_setdelay >= 2000) {
                Wifiset_State_Error();
            }
            break;
            
        case WIFI_SET_SEND_CWSAP:
            match_index = 0;
            Send_AT_Cmd("AT+CWSAP=\"Clock-Setup\",\"\",1,0\r\n");
            g_wifi_set = WIFI_SET_WAIT_CWSAP;
            wifi_setdelay = now;
            break;
            
        case WIFI_SET_WAIT_CWSAP:
            if(Match_Target_Stream("OK")) {
                g_wifi_set = WIFI_SET_SEND_CIPMUX;
            } else if(now - wifi_setdelay >= 2000) {
                Wifiset_State_Error();
            }
            break;
            
        case WIFI_SET_SEND_CIPMUX:
            match_index = 0;
            Send_AT_Cmd("AT+CIPMUX=1\r\n");
            g_wifi_set = WIFI_SET_WAIT_CIPMUX;
            wifi_setdelay = now;
            break;
            
        case WIFI_SET_WAIT_CIPMUX:
            if(Match_Target_Stream("OK")) {
                g_wifi_set = WIFI_SET_SEND_CIPSERVER;
            } else if(now - wifi_setdelay >= 2000) {
                Wifiset_State_Error();
            }
            break;
            
        case WIFI_SET_SEND_CIPSERVER:
            match_index = 0;
            Send_AT_Cmd("AT+CIPSERVER=1,80\r\n");
            g_wifi_set = WIFI_SET_WAIT_CIPSERVER;
            wifi_setdelay = now;
            break;
            
        case WIFI_SET_WAIT_CIPSERVER:
            if(Match_Target_Stream("OK")) {
                g_wifi_set = WIFI_SET_IDLE;
                Error_count = 0;   // AP热点+服务器就绪，连续失败计数清零
                match_index = 0;
            } else if(now - wifi_setdelay >= 2000) {
                Wifiset_State_Error();
            }
            break;

        case WIFI_SET_IDLE:
            if(Match_Target_Stream("+IPD,")) {
                rx_json_len = 0;
                match_index = 0;
                wifi_setdelay = now;
                g_wifi_set = WIFI_SET_CAPTURE_REQ;
            }
            break;

        case WIFI_SET_CAPTURE_REQ:
            if(Match_Target_And_Capture(" HTTP/1.1", rx_json_buf, sizeof(rx_json_buf))) {
                int link_id = 0;
                if (sscanf(rx_json_buf, "%d,", &link_id) == 1) {
                    current_link_id = (uint8_t)link_id;
                }

                if (strstr(rx_json_buf, "GET /save?")) {
                    is_save_req = 1; // 标记用户提交了配置
											if(Parse_Wifi_Config_From_HTTP(rx_json_buf,&wifi_flash_config)){
																wifi_save_failed = !Flash_Write_WifiConfig(&wifi_flash_config);
												           g_wifi_set = WIFI_SET_SEND_CIPSEND;
																
											}
											else 
														g_wifi_set=WIFI_SET_IDLE;
                } else if (strstr(rx_json_buf, "GET / ")) {
                    is_save_req = 0; // 标记用户请求主页
                    g_wifi_set = WIFI_SET_SEND_CIPSEND;
                } else {
                    g_wifi_set = WIFI_SET_IDLE;
                }
            } else if (now - wifi_setdelay >= 15000) {
                g_wifi_set = WIFI_SET_IDLE; // 接收 HTTP 头超时
            }
            break;

        case WIFI_SET_SEND_CIPSEND:
            // 根据请求类型打包对应的 HTML
            if (is_save_req) {
                if (wifi_save_failed) {
                    char diag_html[96];
                    snprintf(diag_html, sizeof(diag_html),
                             "<h2>FAILED stage=%u SR=0x%08lX OPTCR=0x%08lX</h2>",
                             flash_diag_stage, (unsigned long)flash_diag_sr, (unsigned long)flash_diag_optcr);
                    sprintf(http_buf, "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nConnection: close\r\n\r\n%s", diag_html);
                } else {
                    sprintf(http_buf, "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nConnection: close\r\n\r\n%s", SUCCESS_HTML);
                }
            } else {
                sprintf(http_buf, "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nConnection: close\r\n\r\n%s", HTML_FORM);
            }
            
            match_index = 0;
            sprintf(AT_CMD, "AT+CIPSEND=%d,%d\r\n", current_link_id, strlen(http_buf));
            Send_AT_Cmd(AT_CMD);
            wifi_setdelay = now;
            g_wifi_set = WIFI_SET_WAIT_PROMPT;
            break;

        case WIFI_SET_WAIT_PROMPT:
            if(Match_Target_Stream(">")) {
                g_wifi_set = WIFI_SET_SEND_HTML;
            } else if(now - wifi_setdelay >= 2000) {
                g_wifi_set = WIFI_SET_IDLE; 
            }
            break;

        case WIFI_SET_SEND_HTML:
            match_index = 0;
            Send_AT_Cmd(http_buf); // 发送 HTML 数据
            wifi_setdelay = now;
            g_wifi_set = WIFI_SET_WAIT_SEND_OK;
            break;

        case WIFI_SET_WAIT_SEND_OK:
            if(Match_Target_Stream("SEND OK")) {
                if (is_save_req) {
                    if (wifi_save_failed) {
                        wifi_save_failed = 0;
                        is_save_req = 0;
                        g_wifi_set = WIFI_SET_IDLE; // Flash 写失败：留在配网模式，等用户重新提交
                    } else {
                        g_wifi_set = WIFI_SET_CONFIG_DONE; // 如果发送的是保存成功页面，则配网彻底结束！
                    }
                } else {
                    g_wifi_set = WIFI_SET_IDLE; // 发送的是主页，退回等待手机再次提交
                }
            } else if(now - wifi_setdelay >= 3000) {
                g_wifi_set = WIFI_SET_IDLE;
            }
            break;

        case WIFI_SET_CONFIG_DONE:
						current_wifi_mode = SYS_WIFI_MODE_STA;
						g_wifi_set=WIFI_SET_IDLE;
           break;

       case WIFI_SET_ERROR:
            if (now - wifi_setdelay >= 15000) {
                if (++Error_count >= 6) {           // 连续6次配网初始化失败：放弃本次配网
                    Error_count = 0;
                    current_wifi_mode = SYS_WIFI_MODE_IDLE;
                } else {
                    g_wifi_set = WIFI_SET_SEND_AT;  // 未达次数：重新初始化配网流程
                }
            }
					break;
    }
}

void Wifi_Task_Manager(void) 
{
    static uint32_t wifi_mode_time = 0;
    static Sys_Wifi_Mode_t last_mode = (Sys_Wifi_Mode_t)0xFF; 
    uint8_t dump_ch;

    if (current_wifi_mode != last_mode) {//脏检测：模式换了才执行
        last_mode = current_wifi_mode;
        while (RingBuffer_ReadByte(&dump_ch));  //把之前残留的数据都丢了
						match_index = 0;
        if (current_wifi_mode == SYS_WIFI_MODE_STA)
					g_wifi_state = WIFI_STATE_SEND_RST;//配置sta状态机为最开始状态
        else if (current_wifi_mode == SYS_WIFI_MODE_AP_CFG)
					g_wifi_set = WIFI_SET_SEND_AT;//配置ap状态机为最开始状态
        else if (current_wifi_mode == SYS_WIFI_MODE_IDLE)
					wifi_mode_time = tick_now;//切换为IDLE模式时从此刻开始计时一天     
    }
    switch (current_wifi_mode) {
        case SYS_WIFI_MODE_IDLE:
            if (wifi_flash_config.magic == 0xABCD1234 && tick_now - wifi_mode_time >= 86400000)
                current_wifi_mode = SYS_WIFI_MODE_STA;
            break;
        case SYS_WIFI_MODE_STA:
            wifi_handle();
            break;
        case SYS_WIFI_MODE_AP_CFG:
            wifiset_handle();
            break;
    }
}

/* ---------- Flash 存储统一入口 ----------
 * 天气(0x08060000) 与 WiFi 配置(0x08070000) 同在 Sector 7，擦除粒度为整扇区，
 * 因此任一份数据写入都必须整扇区擦除后两份一起重写 */
static uint8_t flash_program_data(uint32_t addr, const void *data, uint32_t len)
{
    const uint16_t *p = (const uint16_t *)data;
    for (uint32_t i = 0; i < (len + 1) / 2; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr, p[i]) != HAL_OK) {
            HAL_FLASH_Lock();
            return 0;
        }
        addr += 2;
    }
    return 1;
}

static uint8_t Flash_Write_All(void)
{
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t SectorError = 0;

    flash_diag_stage = 0;
    flash_diag_sr = 0;

    HAL_FLASH_Unlock();
    /* 清除可能遗留的错误标志：SR 里挂着旧标志会让后续所有 Flash 操作直接失败 */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR |
                           FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    EraseInitStruct.TypeErase    = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.Sector       = FLASH_SECTOR_7;   /* 0x08060000 */
    EraseInitStruct.NbSectors    = 1;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK) {
        flash_diag_stage = 1;
        flash_diag_sr = FLASH->SR;
        flash_diag_optcr = FLASH->OPTCR;
        HAL_FLASH_Lock();
        return 0;
    }

    if (!flash_program_data(FLASH_WEATHER_ADDR, &g_weather, sizeof(Weather_Info_t))) {
        flash_diag_stage = 2;
        flash_diag_sr = FLASH->SR;
        flash_diag_optcr = FLASH->OPTCR;
        HAL_FLASH_Lock();
        return 0;
    }
    if (!flash_program_data(FLASH_WIFI_ADDR, &wifi_flash_config, sizeof(Wifi_Config_t))) {
        flash_diag_stage = 3;
        flash_diag_sr = FLASH->SR;
        flash_diag_optcr = FLASH->OPTCR;
        HAL_FLASH_Lock();
        return 0;
    }

    /* 写后读回校验：写入失败必须暴露出来，否则配置会在 STA 流程里悄悄丢失 */
    if (memcmp((const void *)FLASH_WEATHER_ADDR, &g_weather, sizeof(Weather_Info_t)) != 0) {
        flash_diag_stage = 4;
        flash_diag_sr = FLASH->SR;
        flash_diag_optcr = FLASH->OPTCR;
        HAL_FLASH_Lock();
        return 0;
    }
    if (memcmp((const void *)FLASH_WIFI_ADDR, &wifi_flash_config, sizeof(Wifi_Config_t)) != 0) {
        flash_diag_stage = 5;
        flash_diag_sr = FLASH->SR;
        flash_diag_optcr = FLASH->OPTCR;
        HAL_FLASH_Lock();
        return 0;
    }

    HAL_FLASH_Lock();
    return 1;
}

uint8_t Flash_Write_Weather(Weather_Info_t *weather)
{
    g_weather = *weather;
    return Flash_Write_All();
}

void Flash_Read_Weather(Weather_Info_t *weather)
{
    memcpy(weather, (void *)FLASH_WEATHER_ADDR, sizeof(Weather_Info_t));
}


uint8_t Flash_Write_WifiConfig(Wifi_Config_t *config)
{
    wifi_flash_config = *config;
    return Flash_Write_All();
}


void Flash_Read_WifiConfig(Wifi_Config_t *config)
{
    memcpy(config, (void *)FLASH_WIFI_ADDR, sizeof(Wifi_Config_t));
}

