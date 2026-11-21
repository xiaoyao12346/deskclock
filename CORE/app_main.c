#include "app_main.h"
static lv_obj_t *img_home_weather;
LV_IMG_DECLARE(sun);
LV_IMG_DECLARE(cloud);
LV_IMG_DECLARE(overcast);
LV_IMG_DECLARE(rain);
LV_IMG_DECLARE(thundershower);
LV_IMG_DECLARE(snow);
LV_IMG_DECLARE(comfort);
LV_IMG_DECLARE(Warning);
LV_IMG_DECLARE(cold);
LV_IMG_DECLARE(hot);
LV_IMG_DECLARE(dry);
LV_IMG_DECLARE(tide);
LV_IMG_DECLARE(clock);
LV_IMG_DECLARE(countdown);
LV_IMG_DECLARE(tomato);


LV_FONT_DECLARE(font_china_14);
PageId_t 			 current_page           = PAGE_HOME;
TrendSubPage_t current_trend_sub_page = INDOOR_SUB_OVERVIEW;       // 趋势图子视图
EnvSubPage_t   current_env_sub_page   = ENV_SUB_OVERVIEW;    // 温湿度页子视图
PomoMode_t  current_pomo_sub_page  = POMO_BROWSE;			// 番茄时钟/倒计时子视图
SysSubPage_t   current_sys_sub_page   = SYS_SUB_MAIN;        // 设置页子视图
static uint8_t pomo_cursor = 1;   // 选卡：0倒计时 1番茄钟 2闹钟
static int8_t  pomo_hl = -1;

/* ===== 设置列表页（SYS_SUB_MAIN，手表式列表，值先写死）===== */
static lv_obj_t *label_sys_arrow;           // 左侧箭头 ">"，跟随选中行移动
static lv_obj_t *label_sys_item_name[3];    // 可见 3 行：项目名
static lv_obj_t *label_sys_item_val[3];     // 可见 3 行：当前值
static lv_obj_t *sys_row_bg[3];             // 可见 3 行的反相衬底条（选中行黑底亮块）
/* 时间项编辑时的拆分字段组：在整行反相(黑底白字)上，当前编辑字段再二次反相(白底黑字小块) */
static lv_obj_t *sys_t_hour;                // 时 数字
static lv_obj_t *sys_t_colon;               // ":"
static lv_obj_t *sys_t_min;                 // 分 数字
static lv_obj_t *sys_t_focus_bg[2];         // 时/分 的二次反相白底块(0时 1分)
static uint8_t   sys_cursor   = 0;          // 当前选中项 0~7
static uint8_t   sys_list_top = 0;          // 列表首行可见项索引（滚动偏移 0~5）
/* 设置页最外层交互态（NAV/EDIT 只存在于 MAIN 列表；跳到 WiFi 等独立子屏时回到 BROWSE）*/
typedef enum {
    SYS_MODE_BROWSE = 0,   // 浏览态：K1/K3翻顶层页/横翻子屏
    SYS_MODE_NAV,          // 导航态：K1/K3上下移光标
    SYS_MODE_EDIT          // 编辑态：K1/K3改值、K2短切字段
} Sys_Mode_t;
static Sys_Mode_t sys_mode = SYS_MODE_BROWSE;
static uint8_t   sys_field = 0;             // 编辑态下当前改该项的第几个字段（多字段项用：时间 时/分、番茄钟 工作/休息）
static const char *const sys_item_names[8] = {
    "时间",       "番茄钟时长", "振动",     "WiFi配网",
    "LCD亮度",    "息屏时间",   "蜂鸣器",   "敲击唤醒灵敏度"
};

/* 各项设置的真实值*/
static uint8_t set_hour = 12;          // idx0 时间：时
static uint8_t set_min  = 30;          // idx0 时间：分
static uint8_t *set_list[2]={&set_hour,&set_min};
static uint8_t set_idx=0;
static uint8_t pomo_break_min = 5;     // idx1 番茄钟：休息分钟（工作分钟用已有的 pomo_work_min）
static uint8_t set_vib    = 1;         // idx2 振动：     0关 1开
static uint8_t set_bright = 1;         // idx4 LCD亮度：  0低 1中 2高
static uint8_t set_sleep  = 1;         // idx5 息屏时间： 0=30s 1=1min 2=5min 3=永不
static uint8_t set_buzzer = 1;         // idx6 蜂鸣器：   0关 1开
static uint8_t set_tap    = 1;         // idx7 敲击灵敏度：0低 1中 2高
static const char *const SET_ONOFF_MAP[2] = {"关闭", "开启"};
static const char *const SET_LEVEL_MAP[3] = {"低", "中", "高"};
static const char *const SET_SLEEP_MAP[4] = {"30s", "1min", "5min", "永不"};

static void sys_list_refresh(void);

/* ===== 时光工坊三个卡片的数据（UI 文本从这些变量生成，后续编辑态改的就是它们）===== */
/* 倒计时 */
static uint8_t cd_status     = 0;   // 状态数字：0 = 已暂停, 1 = 进行中
static uint8_t cd_remain_min = 21;         // 剩余分钟 → "21m"
static uint8_t* cd_vars[2]   = { &cd_status, &cd_remain_min };
static uint8_t  cd_active_idx = 0;  // 当前选中索引：0 或 1
static const char* const STATUS_TEXT_MAP[] = { "已暂停", "进行中" };
static uint8_t cd_firing=0;
static uint32_t cd_last_time=0;   // 倒计时计时基准，启动/继续时赋值为当前 tick

//按键按中键切换 改变cd_active_idx修改的变量
//按上下键调整变量大小


/* 番茄钟 */
static uint8_t pomo_status    = 0;         // 运行状态：0=已暂停, 1=进行中（兼开关）
static uint8_t pomo_today_cnt = 0;         // 今日完成次数 → 顶部徽章"今日4次"（只读统计）
//static uint8_t* pomo_vars[2]   = { &pomo_status, &pomo_work_min };
static uint8_t  pomo_active_idx = 0;  // 番茄钟目前只有一个可编辑字段（状态开关），索引恒为 0；将来加"时长"字段时再改成真正的索引
static const char *const POMO_STATUS_MAP[] = {"已暂停", "进行中"};
static uint8_t pomo_firing=0;
static uint32_t pomo_last_time=0;   // 倒计时计时基准，启动/继续时赋值为当前 tick
static uint8_t pomo_count[2]={5,25};
//static uint8_t pomo_count[2]={1,2};

static uint8_t pomo_count_idx=1;
static uint8_t pomo_work_min  = 25;        // 单个番茄时长(分钟) → "25m"

static uint8_t pomo_run_status=0;

/* 番茄钟"休息段"整卡变暗的不透明度 —— 越小越淡（灰阶屏上直接决定灰到什么程度） */
#define POMO_SEG_DIM_OPA   LV_OPA_50

/* 闹钟 */
static int8_t alarm_hour = 11;            // 时
static int8_t alarm_min  = 50;            // 分 → "11:50"
static int8_t alarm_on   = 0;             // 1=开启 0=关闭 → "开启/关闭"
static int8_t* alarm_vars[3]   = { &alarm_hour, &alarm_min ,&alarm_on};
static uint8_t  alarm_active_idx = 0;  // 当前选中索引：0 或 1 或 2
static const char* const ALARM_ON_TEXT_MAP[] = { "关闭", "开启" };
static uint8_t alarm_firing=0;

uint8_t App_Is_Timing(void)
{
    if (cd_firing || alarm_firing ||pomo_firing) return 1;   
    if (cd_status == 1)            return 1;
    if (pomo_status == 1)          return 1;
    if (alarm_on == 1)             return 1;
    return 0;
}
   RTC_TimeTypeDef sTime = {0};
   RTC_DateTypeDef sDate = {0};

void RTC_Set_From_GTime(void)
{
    sTime.Hours   = g_time.hour;
    sTime.Minutes = g_time.min;
    sTime.Seconds = g_time.sec;
    
    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

	  sDate.WeekDay = g_time.week; 
    sDate.Year    = (uint8_t)(g_time.year % 100); 
    sDate.Month   = g_time.month;	
    sDate.Date    = g_time.date;
  
    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0xA5A5);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR2, sDate.Year);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR3, sDate.Month);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR4, sDate.Date);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR5, sDate.WeekDay);
}

/* 时间编辑即时生效：每按一次加减，立刻把 set_hour/set_min 写入 RTC（保留当前秒）。
 * 不能延迟到退出设置页再写——期间若 WiFi 配网触发 NTP 校时，退出时陈旧的编辑值
 * 会反过来覆盖刚校准好的网络时间。 */
static void sys_time_commit(void)
{
    /* F4：GetTime 后必须 GetDate 解锁影子寄存器；读回是为了保留当前秒 */
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    sTime.Hours   = set_hour;
    sTime.Minutes = set_min;
    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);   // 只改时分，秒沿用读回值

    g_time.hour = set_hour;                           // 同步网络时间结构，显示不回退
    g_time.min  = set_min;
}

void RTC_Restore_Date(void)
{
    uint8_t month, day;

    if (HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR1) != 0xA5A5) {
        return;	
    }

    month = (uint8_t)HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR3);
    day   = (uint8_t)HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR4);

    if (month < 1 || month > 12 || day < 1 || day > 31) {
        return;
    }
			sDate.Year    = (uint8_t)HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR2);
			sDate.Month   = month;
			sDate.Date    = day;
			sDate.WeekDay = (uint8_t)HAL_RTCEx_BKUPRead(&hrtc, RTC_BKP_DR5);
			HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
}

void alert_stop(void){
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_RESET);
}
void alert_start(void){
	if(!set_vib) return;                  // 振动总开关关闭（设置→振动）时不起振
	HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_SET);
}

/* 退出某项编辑（EDIT→NAV）时，把该项配置立即应用到硬件。
 * 番茄钟时长是内存变量，改完即被计时逻辑读取，无需处理；时间在离开设置页时单独写 RTC。 */
static void sys_apply_item(uint8_t idx)
{
	static const uint8_t  oled_master_contrast[3] = {0x04, 0x09, 0x0F};  // SSD1322 0xC7：低/中/高
	static const uint16_t sleep_sec_table[4]       = {30, 60, 300, 0};      // 30s/1min/5min/永不(0=不计空闲秒、不进睡眠)

	switch(idx){
		case 1:   /* 番茄钟时长：pomo_work_min / pomo_break_min 为内存变量，计时直接读取，已生效 */
			break;

		case 2:   /* 振动开关：关闭时若正在振，立即停 */
			if(!set_vib) alert_stop();
			break;

		case 4:   /* LCD亮度：写 SSD1322 主对比度电流寄存器 0xC7 */
			Oled_Write_cmd(0xC7);
			Oled_Write_data(oled_master_contrast[set_bright]);
			break;

		case 5:   /* 息屏时间：更新低功耗模块的空闲阈值（秒） */
			sleep_threshold_seconds = sleep_sec_table[set_sleep];
			System_Reset_TimerCount();   /* 换档后从当前时刻重新计时，避免旧空闲秒直接触发更短的阈值 */
			break;

		case 6:   /* 蜂鸣器开关：蜂鸣器硬件未接，驱动就绪后在此使能/静音 */
			/* TODO: buzzer enable/disable */
			break;

		case 7: { /* 敲击唤醒灵敏度：写 MPU6050 MOT_THR，阈值越大越不灵敏。低=10 中=5 高=2 */
			static const uint8_t tap_threshold[3] = {10, 5, 2};
			MPU6050_Config_Motion_Interrupt(tap_threshold[set_tap]);
			break;
		}
	}
}
static uint8_t ui_data_dirty = 0;   /* UI私有脏标志：UI事件队列置位，UI_Show消费清零 */

void main_run(void){
    EventType_t ev;
    EventType_t uiev;

    /* UI数据事件队列（与按键事件分开）：时间同步→写RTC；数据更新→通知UI刷新 */
    while(EventQueue_Read(&ui_eventq, &uiev)){
        switch(uiev){
            case EV_TIME_SYNCED:
                RTC_Set_From_GTime();      /* 网络时间写入RTC（含日期/BKP） */
                ui_data_dirty = 1;
                break;
            case EV_WEATHER_UPDATED:
                ui_data_dirty = 1;
                break;
            default:
                break;
        }
    }

    while(EventBuffer_ReadByte(&ev)){
        switch(current_page){
            case PAGE_HOME:
										 if(ev==EV_KEY1_SHORT) current_page=PAGE_SYS;
                else if(ev==EV_KEY3_SHORT) current_page=PAGE_ENV;
                else if(ev==EV_KEY2_LONG)  current_page=PAGE_SYS;
            break;
						
						
            case PAGE_ENV:
									switch(current_env_sub_page){
										case ENV_SUB_OVERVIEW:
											  if(ev==EV_KEY1_SHORT)      current_page=PAGE_HOME;
												else if(ev==EV_KEY3_SHORT) current_page=PAGE_TREND;
												else if(ev==EV_KEY2_LONG)  current_page=PAGE_SYS;
												else if(ev==EV_KEY2_SHORT) current_env_sub_page=ENV_SUB_TODAY_DETAIL;
										break;
										case ENV_SUB_TODAY_DETAIL:
												if(ev==EV_KEY3_SHORT)		current_env_sub_page=ENV_SUB_TOMORROW_DETAIL;
												else if(ev==EV_KEY1_SHORT) current_env_sub_page=ENV_SUB_WIND_HUMIDITY;
												else if(ev==EV_KEY2_LONG) current_env_sub_page=ENV_SUB_OVERVIEW;	
										break;
										case ENV_SUB_TOMORROW_DETAIL:
												 if(ev==EV_KEY3_SHORT) current_env_sub_page=ENV_SUB_AFTER_TOM_DETAIL;
													else if(ev==EV_KEY1_SHORT) current_env_sub_page=ENV_SUB_TODAY_DETAIL;
													else if(ev==EV_KEY2_LONG) current_env_sub_page=ENV_SUB_OVERVIEW;	
										break;
										case ENV_SUB_AFTER_TOM_DETAIL:
												if(ev==EV_KEY3_SHORT) current_env_sub_page=ENV_SUB_WIND_HUMIDITY;
												else if(ev==EV_KEY1_SHORT) current_env_sub_page=ENV_SUB_TOMORROW_DETAIL;
												else if(ev==EV_KEY2_LONG) current_env_sub_page=ENV_SUB_OVERVIEW;	
										break;
										case ENV_SUB_WIND_HUMIDITY:
												 if(ev==EV_KEY3_SHORT) current_env_sub_page=ENV_SUB_TODAY_DETAIL;
													else if(ev==EV_KEY1_SHORT)  current_env_sub_page=ENV_SUB_AFTER_TOM_DETAIL;
													else if(ev==EV_KEY2_LONG) current_env_sub_page=ENV_SUB_OVERVIEW;	
										break;
									}
						break;
									
									
            case PAGE_TREND:
									switch(current_trend_sub_page){
										case INDOOR_SUB_OVERVIEW:
											if(ev==EV_KEY1_SHORT)      current_page=PAGE_ENV;
											else if(ev==EV_KEY3_SHORT) current_page=PAGE_POMODORO;
											else if(ev==EV_KEY2_LONG)  current_page=PAGE_SYS;
											else if(ev==EV_KEY2_SHORT) current_trend_sub_page=INDOOR_SUB_TEMP;
										break;
										case INDOOR_SUB_TEMP:
											if(ev==EV_KEY1_SHORT) current_trend_sub_page=INDOOR_SUB_EVAL;
											else if(ev==EV_KEY2_LONG) current_trend_sub_page=INDOOR_SUB_OVERVIEW;
											else if(ev==EV_KEY3_SHORT) current_trend_sub_page=INDOOR_SUB_HUMI;
										break;
										case INDOOR_SUB_HUMI:
											if(ev==EV_KEY1_SHORT) current_trend_sub_page=INDOOR_SUB_TEMP;
											else if(ev==EV_KEY2_LONG) current_trend_sub_page=INDOOR_SUB_OVERVIEW;
											else if(ev==EV_KEY3_SHORT) current_trend_sub_page=INDOOR_SUB_PRESS;
										break;
										case INDOOR_SUB_PRESS:
											if(ev==EV_KEY1_SHORT) current_trend_sub_page=INDOOR_SUB_HUMI;
											else if(ev==EV_KEY2_LONG) current_trend_sub_page=INDOOR_SUB_OVERVIEW;
											else if(ev==EV_KEY3_SHORT) current_trend_sub_page=INDOOR_SUB_FORECAST;
										break;
										case INDOOR_SUB_FORECAST:
											if(ev==EV_KEY1_SHORT) current_trend_sub_page=INDOOR_SUB_PRESS;
											else if(ev==EV_KEY2_LONG) current_trend_sub_page=INDOOR_SUB_OVERVIEW;
											else if(ev==EV_KEY3_SHORT) current_trend_sub_page=INDOOR_SUB_EVAL;
										break;
										case INDOOR_SUB_EVAL:
											if(ev==EV_KEY1_SHORT) current_trend_sub_page=INDOOR_SUB_FORECAST;
											else if(ev==EV_KEY2_LONG) current_trend_sub_page=INDOOR_SUB_OVERVIEW;
											else if(ev==EV_KEY3_SHORT) current_trend_sub_page=INDOOR_SUB_TEMP;
										break;
									}	
           break;
						
						
            case PAGE_POMODORO:
							switch(current_pomo_sub_page){
								case POMO_BROWSE:
								if(ev==EV_KEY1_SHORT)      current_page=PAGE_TREND;
								else if(ev==EV_KEY3_SHORT) current_page=PAGE_SYS;
								else if(ev==EV_KEY2_LONG)  current_page=PAGE_SYS;
								else if(ev==EV_KEY2_SHORT){ pomo_cursor=1; current_pomo_sub_page=POMO_SELECT; }
							break;
							case POMO_SELECT:
								if(ev==EV_KEY1_SHORT)      pomo_cursor = (pomo_cursor == 0) ? 2 : pomo_cursor - 1;
								else if(ev==EV_KEY3_SHORT) pomo_cursor = (pomo_cursor == 2) ? 0 : pomo_cursor + 1;
							  else if(ev==EV_KEY2_LONG)  current_pomo_sub_page = POMO_BROWSE;
								else if(ev==EV_KEY2_SHORT) current_pomo_sub_page = POMO_EDIT;
							break;
							case POMO_EDIT:
						  if(ev==EV_KEY2_LONG) current_pomo_sub_page = POMO_SELECT;   // 长按退回选择
							
							if(ev==EV_KEY2_SHORT){//-----------------换选中的变量
								//倒计时cd_active_idx==0是"已暂停", "进行中"，==1是选中分钟+-
								//pomo_cursor=0倒计时 1番茄钟 2闹钟
								if(pomo_cursor==0){//倒计时
									cd_active_idx^=1;//只有已暂停已开启
								}
								else if(pomo_cursor==2){//闹钟
									if(++alarm_active_idx==3)
										alarm_active_idx=0;//3个变量
								}
							}
							
							
							if(ev==EV_KEY3_SHORT){//-----------------增加
								if (pomo_cursor== 0){//倒计时
									if(cd_active_idx==0){//"已暂停", "进行中"
									*cd_vars[cd_active_idx]^=1;
									if(cd_status==1) cd_last_time=tick_now;//启动：计时基准对准现在
								}else if(cd_active_idx==1){//分钟++
										if (++(*cd_vars[ 1 ]) > 59 ) *cd_vars[ 1 ] = 1 ;
									}
							}else if(pomo_cursor== 1){//番茄钟
										pomo_status^=1;//*pomo_vars[0]就是pomo_status运行状态：0=已暂停, 1=进行中（兼开关）
									if(pomo_status==1) pomo_last_time=tick_now;//启动：计时基准对准现在
							}else if(pomo_cursor==2){//闹钟
								if(alarm_active_idx==0){
										if (++(*alarm_vars[0])>23) *alarm_vars[0]=0;
								}else if(alarm_active_idx==1){
										if (++(*alarm_vars[1])>59) *alarm_vars[1]=0;
								}else if(alarm_active_idx==2){
											alarm_on^=1;
								}
							}
						}
							if(ev==EV_KEY1_SHORT){//------------------减少
								if (pomo_cursor== 0 ){//倒计时
									if(cd_active_idx==0){//"已暂停", "进行中"
									*cd_vars[cd_active_idx]^=1;
									if(cd_status==1) cd_last_time=tick_now;//继续：重新对准现在
								}else if(cd_active_idx==1){//分钟--
									if(--(*cd_vars[1]) < 1)  *cd_vars[1] = 59;
									}
							}else if(pomo_cursor== 1){//番茄钟
									pomo_status^=1;
									if(pomo_status==1) pomo_last_time=tick_now;//启动：计时基准对准现在
							}else if(pomo_cursor==2){//闹钟
								if(alarm_active_idx==0){
									if (--(*alarm_vars[0])<0) *alarm_vars[0]=23;
								}else if(alarm_active_idx==1){
										if (--(*alarm_vars[1])<0) *alarm_vars[1]=59;
								}else if(alarm_active_idx==2){
											alarm_on^=1;
								}
							}
						}
						break;
					}
          break;
						
						
            case PAGE_SYS:
            	/* ===== 最外层：设置页三态状态机 ===== */
            	switch(sys_mode){
            		case SYS_MODE_EDIT: {                    /* 编辑态：K1减 K3加，K2短切字段，K2长退回导航态 */
            			if(ev==EV_KEY1_SHORT){                      // ----------减
            				switch(sys_cursor){
            					case 0:                            // 时间
            						if(sys_field==0) set_hour = (set_hour==0) ? 23 : set_hour-1;
            						else            set_min  = (set_min==0)  ? 59 : set_min-1;
            						sys_time_commit();              // 立刻写RTC，不等退出
            						break;
            					case 1:                            // 番茄钟时长（范围1~59循环）
            						if(sys_field==0) pomo_work_min  = (pomo_work_min==1)  ? 59 : pomo_work_min-1;
            						else            pomo_break_min = (pomo_break_min==1) ? 59 : pomo_break_min-1;
            						break;
            					case 2: set_vib ^= 1; break;       // 振动开关
            					case 4: if(set_bright>0) set_bright--; else set_bright=2; break;  // 低中高循环
            					case 5: if(set_sleep>0)  set_sleep--;  else set_sleep=3;  break;  // 30s/1min/5min/永不循环
            					case 6: set_buzzer ^= 1; break;    // 蜂鸣器开关
            					case 7: if(set_tap>0)    set_tap--;    else set_tap=2;    break;  // 灵敏度低中高循环
            				}
            				sys_list_refresh();
            			}
            			else if(ev==EV_KEY3_SHORT){                 // --------------加
            				switch(sys_cursor){
            					case 0:
            						if(sys_field==0) set_hour = (set_hour==23) ? 0 : set_hour+1;
            						else            set_min  = (set_min==59)  ? 0 : set_min+1;
            						sys_time_commit();              // 立刻写RTC，不等退出
            						break;
            					case 1:
            						if(sys_field==0) pomo_work_min  = (pomo_work_min==59)  ? 1 : pomo_work_min+1;
            						else            pomo_break_min = (pomo_break_min==59) ? 1 : pomo_break_min+1;
            						break;
            					case 2: set_vib ^= 1; break;
            					case 4: if(set_bright<2) set_bright++; else set_bright=0; break;
            					case 5: if(set_sleep<3)  set_sleep++;  else set_sleep=0;  break;
            					case 6: set_buzzer ^= 1; break;
            					case 7: if(set_tap<2)    set_tap++;    else set_tap=0;    break;
            				}
            				sys_list_refresh();
            			}
            			else if(ev==EV_KEY2_SHORT){                 // 切字段：仅时间/番茄钟两项有2个字段
            				if(sys_cursor==0 || sys_cursor==1) sys_field ^= 1;
            				sys_list_refresh();                      // 时间项：驱动时/分二次反相互换
            				/* 单字段项（开关/档位）按中键无动作（刷新无副作用）*/
            			}
            			else if(ev==EV_KEY2_LONG){                  // 退出编辑回导航态：应用刚改的这一项
            				sys_apply_item(sys_cursor);
            				sys_mode = SYS_MODE_NAV;
            				sys_field = 0;
            				sys_list_refresh();
            			}
            			break;
            		}

            		case SYS_MODE_NAV: {                      /* 导航态（必在MAIN）：K1/K3移光标，K2短进编辑(WiFi除外)，K2长退回浏览态 */
            			if(ev==EV_KEY1_SHORT){                          // 光标上移
            				if(sys_cursor>0) sys_cursor--;
            				if(sys_cursor<sys_list_top) sys_list_top=sys_cursor;
            				sys_list_refresh();
            			}
            			else if(ev==EV_KEY3_SHORT){                     // 光标下移
            				if(sys_cursor<7) sys_cursor++;
            				if(sys_cursor>sys_list_top+2) sys_list_top=sys_cursor-2;
            				sys_list_refresh();
            			}
            			else if(ev==EV_KEY2_SHORT){
            				if(sys_cursor==3){                          // WiFi配网特殊 → 跳配网子屏（离开列表回浏览态）
            					current_sys_sub_page = SYS_SUB_WIFI;
            					current_wifi_mode = SYS_WIFI_MODE_AP_CFG;
            					sys_mode = SYS_MODE_BROWSE;
            				}
            				else{                                       // 其余项 → 进入编辑态
            					if(sys_cursor==0){                      // 时间：进编辑再读一次RTC，防止停留导航期间NTP刚校时
            						HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
            						HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
            						set_hour = sTime.Hours;
            						set_min  = sTime.Minutes;
            					}
            					sys_mode = SYS_MODE_EDIT;
            					sys_field = 0;
            					sys_list_refresh();
            				}
            			}
            			else if(ev==EV_KEY2_LONG){                      // 退回浏览态
            				sys_mode = SYS_MODE_BROWSE;
            				sys_list_refresh();
            			}
            			break;
            		}

            		default: {                                /* 浏览态：再按"在哪个子屏"细分 */
            			switch(current_sys_sub_page){
            				case SYS_SUB_MAIN:                      // 设置列表浏览：K1/K3翻顶层页，K2短进入导航
            					if(ev==EV_KEY1_SHORT){ current_page=PAGE_POMODORO; }
            					else if(ev==EV_KEY3_SHORT){ current_page=PAGE_HOME; }
            					else if(ev==EV_KEY2_SHORT){
            						/* 进列表前以当前 RTC 时分作为编辑基线（F4：GetTime 后必须 GetDate 解锁）*/
            						HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
            						HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
            						set_hour = sTime.Hours;
            						set_min  = sTime.Minutes;
            						sys_mode=SYS_MODE_NAV;
            						sys_list_refresh();
            					}
            					break;
            				case SYS_SUB_WIFI:                       // WiFi配网页：上下键屏蔽，仅K2长退出
            					/* K1/K3 短按/长按：无动作，防止误切、打断配网 */
            					if(ev==EV_KEY2_LONG){ current_sys_sub_page=SYS_SUB_MAIN; sys_mode=SYS_MODE_NAV; current_wifi_mode=SYS_WIFI_MODE_STA; }
            					break;
            			}
            			break;
            		}
            	}
           break;
        }
    }
}

void Ringing_handling(void){
			 EventType_t ev;
	//闹钟
	//闹钟开&&闹钟没响
		if (alarm_on == 1 && alarm_firing == 0 ) {
				if(alarm_hour==sTime.Hours && alarm_min==sTime.Minutes){//闹钟时间到了
					alarm_firing=1;//闹钟响
					alert_start();
				}
		}
		if(alarm_firing){//闹钟响后的处理
					if (alarm_on == 0) {//改成关闭->取消闹钟
							alarm_firing = 0;         
							alert_stop();
					}else if (sTime.Minutes != alarm_min) {//时间过了一分钟->取消闹钟
							alarm_firing = 0;         
							alert_stop();
							alarm_on=0;
					}
			}
			//倒计时
			uint32_t cd_time=tick_now;
			if(cd_status==1 && cd_firing==0){//倒计时开&&时间还没到
				if(cd_time-cd_last_time>=60*1000){
					cd_last_time=cd_time;
					if(cd_remain_min>0) cd_remain_min--;
				}
				if(cd_remain_min==0){//到点了
					cd_firing=1;
					cd_status=0;
					cd_remain_min=25;
					alert_start();
				}
			}
			//番茄钟
			if(pomo_count_idx==1)pomo_run_status=0;//工作中
			else if(pomo_count_idx==0)pomo_run_status=1;//休息中
			uint32_t pomo_time=tick_now;
			if(pomo_status==1 && pomo_firing==0){
					if(pomo_time-pomo_last_time>=60*1000){
					pomo_last_time=pomo_time;
					if(pomo_work_min>0) pomo_work_min--;
				}
				if(pomo_work_min==0){//到点了
					pomo_count_idx^=1;
					pomo_firing=1;
					pomo_work_min=pomo_count[pomo_count_idx];
					alert_start();
					if(!pomo_run_status)pomo_today_cnt++;
				}
			}
			
			
			
			
			//按键取消
		if (alarm_firing || cd_firing ||	pomo_firing) { //按键取消闹钟或者倒计时
			while (EventBuffer_ReadByte(&ev)) {
				if(alarm_firing){
            alarm_firing = 0 ;
            alarm_on = 0 ;
				}else if(cd_firing){
					  cd_firing = 0 ;
            cd_status = 0 ;
				}else if(pomo_firing){
						pomo_firing=0;
					pomo_last_time=tick_now;//按完按键后开始计时下一个轮回时间
				}
				 alert_stop();
      }
    }
}
/* ==================== LVGL UI (256x64 SSD1322) ==================== */
/* ==================== LVGL UI (256x64) ==================== */
// 屏幕容器二维数组：[顶层页][子页]，用 lv_scr_load_anim(ui_scr[current_page][cur_sub_page]) 切换
static lv_obj_t *ui_scr[5][6];

// --- 主页 (PAGE_HOME) 专属控件 ---
static lv_obj_t *scr_home;							//容器
static lv_obj_t *label_home_time;      // 中间大时间 (12:45)
static lv_obj_t *label_home_temp;      // 左侧温度 (26°C)
static lv_obj_t *label_home_weather;   // 左侧天气 (晴)
static lv_obj_t *label_home_date;      // 右侧日期 (09月10日)
static lv_obj_t *obj_home_week_bg;     // 右下星期白底胶囊框
static lv_obj_t *label_home_week;      // 胶囊框里的文字 (星期四)
static lv_obj_t *sep_left;							//左侧分割线
static lv_obj_t *sep_right;							//右侧分割线


// 环境页主页
static lv_obj_t *scr_env;															//容器
static lv_obj_t* obj_env_city_bg;											//城市背景
static lv_obj_t *label_env_city;                     // 城市名
static lv_obj_t * label_env_date_today;
static lv_obj_t *label_env_date_tomorrow;            // 明天日期
static lv_obj_t *label_env_date_after_tomorrow;      // 后天日期
static lv_obj_t *label_env_temp_tomorrow;            // 明天温度范围
static lv_obj_t *label_env_temp_after_tomorrow;      // 后天温度范围
static lv_obj_t *img_env_weather_tomorrow;        		// 明天天气图标
static lv_obj_t *img_env_weather_after_tomorrow;  		// 后天天气图标
static lv_obj_t *label_env_weather_tomorrow;         // 明天天气状况
static lv_obj_t *label_env_weather_after_tomorrow;   // 后天天气状况
static lv_obj_t *label_env_humidity;                 // 相对湿度
static lv_obj_t *label_env_wind;                     // 风向风力
static lv_obj_t *obj_env_sep;                        // 界面分割线
//环境子页123
static lv_obj_t *scr_env_sub1[3];                                  // 容器
static lv_obj_t *label_env_sub1_title[3];                          // 顶部标题 (📍 佛山 · 05-18 今天)
static lv_obj_t *label_env_sub1_head_temp[3];                     // 顶部温度 (24°~32°C)
static lv_obj_t *obj_env_sub1_badge_bg[3];                        // 页码角标背景框
static lv_obj_t *label_env_sub1_badge[3];                          // 页码角标文本 (1/4 >)
static lv_obj_t *obj_env_sub1_head_sep[3];                         // 顶部横向分割线
static lv_obj_t *label_env_sub1_day_title[3];                      // 白天天气标签 ("白天天气:")
static lv_obj_t *img_env_sub1_day_icon[3];                         // 白天天气图标
static lv_obj_t *label_env_sub1_day_weather[3];                    // 白天天气文本 ("小雨")
static lv_obj_t *label_env_sub1_night_title[3];                    // 夜间天气标签 ("夜间天气:")
static lv_obj_t *img_env_sub1_night_icon[3];                       // 夜间天气图标
static lv_obj_t *label_env_sub1_night_weather[3];                  // 夜间天气文本 ("雷阵雨")
static lv_obj_t *label_env_sub1_rain_title[3];                     // 降水量标签 ("降水量:")
static lv_obj_t *label_env_sub1_rain_val[3];                       // 降水量数值 ("5.40mm")
static lv_obj_t *label_env_sub1_pop_title[3];                      // 降水概率标签 ("降水概率:")
static lv_obj_t *label_env_sub1_pop_val[3];                        // 降水概率数值 ("79%")
static lv_obj_t *bar_env_sub1_pop[3];                              // 降水概率进度条
static lv_obj_t *obj_env_sub1_ver_sep[3];                          // 中间垂直分割线
//环境子页4
static lv_obj_t *label_sub4_title;                 // 顶部标题
static lv_obj_t *obj_env_sub4_badge_bg;           // 页码角标背景框
static lv_obj_t *label_env_sub4_badge;            // 页码角标文本
static lv_obj_t *obj_env_sub4_head_sep;           // 顶部分割线
static lv_obj_t *label_env_sub4_today_title;      // "今天" 标签
static lv_obj_t *label_env_sub4_today_wind;       // 今天风向文本
static lv_obj_t *label_env_sub4_today_hum_title;  // 今天 "湿" 标签
static lv_obj_t *label_env_sub4_today_hum_val;    // 今天湿度数值
static lv_obj_t *obj_env_sub4_row_sep;            // 行间隔虚线/分割线
static lv_obj_t *label_env_sub4_tomorrow_title;   // "明天" 标签
static lv_obj_t *label_env_sub4_tomorrow_wind;    // 明天风向文本
static lv_obj_t *label_env_sub4_tomorrow_hum_title; // 明天 "湿" 标签
static lv_obj_t *label_env_sub4_tomorrow_hum_val; // 明天湿度数值
static lv_obj_t *lobj_env_sub4_bottom_sep;// 底部分割线
//室内环境主页
static lv_obj_t *label_env_main_title;         // 左上角标题 ("环境监控 >")
static lv_obj_t *label_env_3h_title;           // 右侧顶部标题 ("本地 3h 预测")
static lv_obj_t *label_env_indoor_temp_title;  // 室内温度标签
static lv_obj_t *label_env_hum_title;          // 内/外湿度标签
static lv_obj_t *label_env_pressure_title;     // 室内气压标签

static lv_obj_t *label_env_indoor_temp_val;    // 室内温度数值
static lv_obj_t *label_env_hum_val;            // 内/外湿度数值
static lv_obj_t *label_env_pressure_val;       // 室内气压数值
 static lv_obj_t *img_env_3h_icon;              // 3h预测天气图标
static lv_obj_t *label_env_3h_weather;         // 3h预测天气文本
static lv_obj_t *label_env_3h_pressure_trend;  // 3h气压趋势文本
//室内环境子页123
lv_obj_t *obj_trend_card[3];           // 主卡片外框
lv_obj_t *label_trend_title[3];        // 标题文本
lv_obj_t *label_trend_badge[3];        // 右上角页码
lv_obj_t *obj_trend_div[3];            // 顶部横向分割线
lv_obj_t *chart_trend[3];              // 60 点折线图控件
lv_chart_series_t *series_trend[3];    // 数据系列
const char *str_trend_unit[3];         // 数值单位后缀 (例如 "℃" / "%")
lv_obj_t *label_trend_time[3][4];      // 底部时间标签 "-3h" / "-2h" / "-1h" / "现在"
lv_obj_t *label_trend_val[3][4];       // 折线上 4 个关键点数值
/* 折线上 4 个关键点的下标：-3h / -2h / -1h / 现在 */
static const uint8_t trend_key_idx[4] = { 0, 19, 39, 59 };

//室内环境子环境4
lv_obj_t *label_fc3h_title;        // 顶部标题
lv_obj_t *label_fc3h_badge;        // 右上角页码
lv_obj_t *obj_fc3h_hdiv;           // 横向分割线
lv_obj_t *obj_fc3h_vdiv;           // 左右竖向分割线
lv_obj_t *img_fc3h_icon;           // 左侧天气图标
lv_obj_t *label_fc3h_weather;      // 左侧天气状态 ("多云")
// 右侧 3 个项目名称标签 (不用数组)
lv_obj_t *label_fc3h_item_pop;     // "降水概率"
lv_obj_t *label_fc3h_item_temp;    // "Δ 温度变化"
lv_obj_t *label_fc3h_item_press;   // "Δ 气压变化"
// 右侧 3 个项目数值标签 (不用数组)
lv_obj_t *label_fc3h_val_pop;      // 降水概率数值
lv_obj_t *label_fc3h_val_temp;     // 温度变化数值
lv_obj_t *label_fc3h_val_press;    // 气压变化数值
//室内环境子环境5
/* ==================== 内外环境对比与衍生评估 UI 变量声明 ==================== */
lv_obj_t *label_env_title;         // 顶部标题
lv_obj_t *label_env_badge;         // 右上角页码 (5/5)
lv_obj_t *obj_env_hdiv;            // 横向分割线
lv_obj_t *obj_env_vdiv;            // 左右竖向分割线

lv_obj_t *img_env_icon;            // 左侧水滴图标
lv_obj_t *label_env_status;        // 左侧主要状态 ("室外偏潮")
lv_obj_t *label_env_advice;        // 左侧建议标签 ("建议关窗")

// 右侧 3 个项目名称标签
lv_obj_t *label_env_item_dew;      // "露点温度"
lv_obj_t *label_env_item_outhum;   // "室外湿度"
lv_obj_t *label_env_item_diffhum;  // "内外湿差"

// 右侧 3 个项目数值标签
lv_obj_t *label_env_val_dew;       // 露点温度数值
lv_obj_t *label_env_val_outhum;    // 室外湿度数值
lv_obj_t *label_env_val_diffhum;   // 内外湿差数值
/* ==================== 闹钟时钟主页变量声明 ==================== */
// 顶部标题栏
static lv_obj_t *label_pomo_header;       // "< 时光工坊 >"
static lv_obj_t *label_pomo_badge;        // 右上徽章 "4次 (100m)"
// 三个卡片容器
static lv_obj_t *card_countdown;          // 左：倒计时卡片
static lv_obj_t *card_pomodoro;           // 中：番茄钟卡片
static lv_obj_t *card_alarm;              // 右：闹钟卡片
// 倒计时卡片内部
static lv_obj_t *img_countdown_icon;      // 占位图片，用户后补
static lv_obj_t *label_countdown_name;    // "倒计时"
static lv_obj_t *label_countdown_status;   // "已暂停"（后续可变）
static lv_obj_t *label_countdown_time;     // "21m"（后续可变）
// 番茄钟卡片内部
static lv_obj_t *img_pomodoro_icon;       // 占位图片，用户后补
static lv_obj_t *label_pomodoro_name;     // "番茄钟"
static lv_obj_t *label_pomodoro_status;   // "已暂停"/"进行中"（兼开关，可变）
static lv_obj_t *label_pomodoro_dur;      // "25m"（可变）
// 闹钟卡片内部
static lv_obj_t *img_alarm_icon;          // 占位图片，用户后补
static lv_obj_t *label_alarm_name;         // "闹钟"
static lv_obj_t *label_alarm_hour;          // "11"（闹钟时，可变）
static lv_obj_t *label_alarm_min;           // "50"（闹钟分，可变）
static lv_obj_t *label_alarm_colon;         // ":" 固定分隔符
static lv_obj_t *label_alarm_state;         // "开启"（后续可变）

// 其他页面的变量保留
static lv_obj_t *label_pomo_title[4];   // WORKING/COUNTDOWN/ALARM_LIST 三个子页占位标签
static lv_obj_t *label_sys_title[2];   // [SYS_SUB_MAIN] 未使用；[SYS_SUB_WIFI] 配网标题
static lv_obj_t *label_wifi_ap;     // 配网页：提示要连接的 AP 名(Clock-Setup)
static lv_obj_t *label_wifi_ip;     // 配网页：提示浏览器访问的 IP(192.168.4.1)
static lv_obj_t *label_wifi_exit;   // 配网页：退出提示

// 时光工坊选择态：0=倒计时 1=番茄钟 2=闹钟；pomo_hl 记录当前高亮的卡(-1=无高亮)



/* ==================== 2. 内部功能函数 ==================== */


/* 量一段文本在当前字体下的像素宽度 */
static lv_coord_t trend_text_w(const char *txt)
{
    lv_point_t sz;
    lv_txt_get_size(&sz, txt, &font_china_14, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    return sz.x;
}

/* 图表内 60 个点中第 idx 个点在 X 方向的像素位置 */
static lv_coord_t trend_point_x(lv_coord_t chart_w, uint8_t idx)
{
    return (lv_coord_t)((int32_t)(chart_w - 1) * idx / (60 - 1));
}


/* ==================== 3. UI 创建函数 ==================== */
/**
 * @brief 创建一张趋势折线卡片 (256x64 单色屏适配)
 * @param parent 父容器句柄
 * @param idx    0 = 温度卡片, 1 = 湿度卡片
 * @param title  标题文本
 * @param badge  右上角页码文本
 * @param unit   数值单位后缀 (例如 "℃" / "%")
 */
void ui_trend_card_create(lv_obj_t *parent, uint8_t idx, const char *title, const char *badge, const char *unit)
{
						if (parent == NULL || idx >= 3) return;

						// 去掉边框和 padding 后的内容宽度 = 254 - 2*1 - 2*2 = 248
						lv_coord_t card_w = 254 - 2 * 1 - 2 * 2;
						str_trend_unit[idx] = (unit != NULL) ? unit : "";

						// ---------------- 1. 卡片外框 (透明背景 + 黑色边框) ----------------
						obj_trend_card[idx] = lv_obj_create(parent);
						lv_obj_set_size(obj_trend_card[idx], 254, 62);
						lv_obj_align(obj_trend_card[idx], LV_ALIGN_CENTER, 0, 0);
						lv_obj_set_style_bg_opa(obj_trend_card[idx], LV_OPA_0, 0);
						lv_obj_set_style_border_color(obj_trend_card[idx], lv_color_black(), 0);
						lv_obj_set_style_border_width(obj_trend_card[idx], 1, 0);
						lv_obj_set_style_radius(obj_trend_card[idx], 6, 0);
						lv_obj_set_style_pad_all(obj_trend_card[idx], 2, 0);
						lv_obj_clear_flag(obj_trend_card[idx], LV_OBJ_FLAG_SCROLLABLE);

						// ---------------- 2. 标题 ----------------
						label_trend_title[idx] = lv_label_create(obj_trend_card[idx]);
						lv_obj_set_style_text_font(label_trend_title[idx], &font_china_14, 0);
						lv_obj_set_style_text_color(label_trend_title[idx], lv_color_black(), 0);
						lv_label_set_text(label_trend_title[idx], title);
						lv_obj_align(label_trend_title[idx], LV_ALIGN_TOP_LEFT, 0, 0);

						// ---------------- 3. 右上角 Badge ----------------
						label_trend_badge[idx] = lv_label_create(obj_trend_card[idx]);
						lv_obj_set_style_text_font(label_trend_badge[idx], &font_china_14, 0);
						lv_obj_set_style_text_color(label_trend_badge[idx], lv_color_white(), 0);
						lv_obj_set_style_bg_color(label_trend_badge[idx], lv_color_black(), 0);
						lv_obj_set_style_bg_opa(label_trend_badge[idx], LV_OPA_COVER, 0);
						lv_obj_set_style_radius(label_trend_badge[idx], 2, 0);
						lv_obj_set_style_pad_hor(label_trend_badge[idx], 3, 0);
						lv_obj_set_style_pad_ver(label_trend_badge[idx], 0, 0);
						lv_label_set_text(label_trend_badge[idx], badge);
						lv_obj_align(label_trend_badge[idx], LV_ALIGN_TOP_RIGHT, 0, 0);

						// ---------------- 4. 横向分割线 ----------------
						obj_trend_div[idx] = lv_obj_create(obj_trend_card[idx]);
						lv_obj_set_size(obj_trend_div[idx], LV_PCT(100), 1);
						lv_obj_set_style_bg_color(obj_trend_div[idx], lv_color_black(), 0);
						lv_obj_set_style_bg_opa(obj_trend_div[idx], LV_OPA_40, 0);
						lv_obj_set_style_border_width(obj_trend_div[idx], 0, 0);
						lv_obj_set_style_pad_all(obj_trend_div[idx], 0, 0);
						lv_obj_align(obj_trend_div[idx], LV_ALIGN_TOP_MID, 0, 15);

						// ---------------- 5. 折线图 ----------------
						chart_trend[idx] = lv_chart_create(obj_trend_card[idx]);
						lv_obj_set_size(chart_trend[idx], LV_PCT(100), 25);
						lv_obj_align(chart_trend[idx], LV_ALIGN_TOP_LEFT, 0, 16); // 折线和数值标签一起往下挪，改这个 19
						lv_obj_clear_flag(chart_trend[idx], LV_OBJ_FLAG_SCROLLABLE);
						lv_chart_set_type(chart_trend[idx], LV_CHART_TYPE_LINE);
						lv_chart_set_point_count(chart_trend[idx], 60);

						lv_obj_set_style_bg_opa(chart_trend[idx], LV_OPA_0, 0);
						lv_obj_set_style_border_width(chart_trend[idx], 0, 0);
						lv_obj_set_style_pad_all(chart_trend[idx], 0, 0);
						lv_obj_set_style_size(chart_trend[idx], 0, LV_PART_INDICATOR);                      // 隐藏数据圆点
						lv_obj_set_style_line_color(chart_trend[idx], lv_color_black(), LV_PART_ITEMS);     // 细线颜色
						lv_obj_set_style_line_width(chart_trend[idx], 1, LV_PART_ITEMS);                    // 细线宽度 1px
						lv_obj_set_style_bg_opa(chart_trend[idx], LV_OPA_0, LV_PART_ITEMS);                 // 隐藏下方填充
						lv_chart_set_div_line_count(chart_trend[idx], 2, 0);
						lv_obj_set_style_line_color(chart_trend[idx], lv_color_black(), LV_PART_MAIN);
						lv_obj_set_style_line_opa(chart_trend[idx], LV_OPA_30, LV_PART_MAIN);

						series_trend[idx] = lv_chart_add_series(chart_trend[idx], lv_color_black(), LV_CHART_AXIS_PRIMARY_Y);

						// ---------------- 6. 底部 X 轴时间标签 ----------------
						static const char *time_txt[4] = { "-3h", "-2h", "-1h", "现在" };
						for (int i = 0; i < 4; i++) {
								lv_coord_t w = trend_text_w(time_txt[i]);
								lv_coord_t x = trend_point_x(card_w, trend_key_idx[i]) - w / 2;
								if (x < 0) x = 0;
								if (x + w > card_w) x = card_w - w;

								label_trend_time[idx][i] = lv_label_create(obj_trend_card[idx]);
								lv_obj_set_style_text_font(label_trend_time[idx][i], &font_china_14, 0);
								lv_obj_set_style_text_color(label_trend_time[idx][i], lv_color_black(), 0);
								lv_label_set_text(label_trend_time[idx][i], time_txt[i]);
								lv_obj_set_pos(label_trend_time[idx][i], x, 41); // 跟着折线图一起往下挪，别再往下推(卡片底边只剩 2px)
						}

						// ---------------- 7. 折线上的 4 个数值标签 ----------------
						for (int i = 0; i < 4; i++) {
								label_trend_val[idx][i] = lv_label_create(chart_trend[idx]);
								lv_obj_set_style_text_font(label_trend_val[idx][i], &lv_font_montserrat_10, 0); // 数值字体：10 号(行高 11px, 数字墨迹 7px)，改字号改这里
								lv_obj_set_style_text_color(label_trend_val[idx][i], lv_color_black(), 0);
								lv_obj_set_style_bg_color(label_trend_val[idx][i], lv_color_white(), 0); // 白底：屏幕上是熄灭，需要遮挡折线时用
								lv_obj_set_style_bg_opa(label_trend_val[idx][i], LV_OPA_0, 0);           // 平时透明，放不下时才在 set_data 里打开
								lv_obj_set_style_pad_all(label_trend_val[idx][i], 0, 0);
								lv_label_set_long_mode(label_trend_val[idx][i], LV_LABEL_LONG_CLIP);
								lv_label_set_text(label_trend_val[idx][i], "--");
								lv_obj_set_pos(label_trend_val[idx][i], 0, 0);
						}
}
void create_env_sub_page(uint8_t  idx)
{
							// 详情页从 ui_scr[1][1] 开始：idx 0=今天, 1=明天, 2=后天；ui_scr[1][0] 是概览页，不能占用
							scr_env_sub1[idx] = ui_scr[1][idx + 1];                          // 容器
							label_env_sub1_title[idx] = lv_label_create(scr_env_sub1[idx]);
							lv_obj_set_style_text_font(label_env_sub1_title[idx], &font_china_14, 0);
							lv_obj_set_style_text_color(label_env_sub1_title[idx], lv_color_black(), 0);
							lv_obj_align(label_env_sub1_title[idx], LV_ALIGN_TOP_LEFT, 4, 0);
							// 顶部温度范围 (24°~32°C)
							label_env_sub1_head_temp[idx] = lv_label_create(scr_env_sub1[idx]);
							lv_obj_set_style_text_font(label_env_sub1_head_temp[idx], &font_china_14, 0);
							lv_obj_set_style_text_color(label_env_sub1_head_temp[idx], lv_color_black(), 0);
							lv_obj_align(label_env_sub1_head_temp[idx], LV_ALIGN_TOP_LEFT, 132, 0);
							// 页码角标背景 (1/4 )
							obj_env_sub1_badge_bg[idx] = lv_obj_create(scr_env_sub1[idx]);
							lv_obj_set_size(obj_env_sub1_badge_bg[idx], LV_SIZE_CONTENT, 14);
							lv_obj_align(obj_env_sub1_badge_bg[idx], LV_ALIGN_TOP_RIGHT, -4, 1);
							lv_obj_set_style_bg_color(obj_env_sub1_badge_bg[idx], lv_color_black(), 0);
							lv_obj_set_style_bg_opa(obj_env_sub1_badge_bg[idx], LV_OPA_COVER, 0);
							lv_obj_set_style_radius(obj_env_sub1_badge_bg[idx], 2, 0);
							lv_obj_set_style_pad_left(obj_env_sub1_badge_bg[idx], 3, 0);
							lv_obj_set_style_pad_right(obj_env_sub1_badge_bg[idx], 3, 0);
							lv_obj_set_style_pad_top(obj_env_sub1_badge_bg[idx], 0, 0);
							lv_obj_set_style_pad_bottom(obj_env_sub1_badge_bg[idx], 0, 0);
							lv_obj_set_style_border_width(obj_env_sub1_badge_bg[idx], 0, 0);
							lv_obj_clear_flag(obj_env_sub1_badge_bg[idx], LV_OBJ_FLAG_SCROLLABLE);
							// 页码角标文本
							label_env_sub1_badge[idx] = lv_label_create(obj_env_sub1_badge_bg[idx]);
							lv_obj_set_style_text_font(label_env_sub1_badge[idx], &font_china_14, 0);
							lv_obj_set_style_text_color(label_env_sub1_badge[idx], lv_color_white(), 0);
							lv_obj_align(label_env_sub1_badge[idx], LV_ALIGN_CENTER, 0, 0);
							// 顶部横向分割线
							obj_env_sub1_head_sep [idx]= lv_obj_create(scr_env_sub1[idx]);
							lv_obj_set_size(obj_env_sub1_head_sep[idx], 248, 1);
							lv_obj_align(obj_env_sub1_head_sep[idx], LV_ALIGN_TOP_LEFT, 4, 15);
							lv_obj_set_style_bg_color(obj_env_sub1_head_sep[idx], lv_color_black(), 0);
							lv_obj_set_style_bg_opa(obj_env_sub1_head_sep[idx], LV_OPA_40, 0);
							lv_obj_set_style_pad_all(obj_env_sub1_head_sep[idx], 0, 0);
							lv_obj_set_style_border_width(obj_env_sub1_head_sep[idx], 0, 0);
							// --- 中间垂直分割线 ---
							obj_env_sub1_ver_sep[idx] = lv_obj_create(scr_env_sub1[idx]);
							lv_obj_set_size(obj_env_sub1_ver_sep[idx], 1, 44);
							lv_obj_align(obj_env_sub1_ver_sep[idx], LV_ALIGN_TOP_LEFT, 128, 18);
							lv_obj_set_style_bg_color(obj_env_sub1_ver_sep[idx], lv_color_black(), 0);
							lv_obj_set_style_bg_opa(obj_env_sub1_ver_sep[idx], LV_OPA_40, 0);
							lv_obj_set_style_pad_all(obj_env_sub1_ver_sep[idx], 0, 0);
							lv_obj_set_style_border_width(obj_env_sub1_ver_sep[idx], 0, 0);
							// --- 左侧：白天与夜间天气 ---
							// 白天天气标签
							label_env_sub1_day_title[idx] = lv_label_create(scr_env_sub1[idx]);
							lv_obj_set_style_text_font(label_env_sub1_day_title[idx], &font_china_14, 0);
							lv_obj_set_style_text_color(label_env_sub1_day_title[idx], lv_color_black(), 0);
							lv_obj_align(label_env_sub1_day_title[idx], LV_ALIGN_TOP_LEFT, 4, 20);
							// 白天天气图标
							img_env_sub1_day_icon[idx] = lv_img_create(scr_env_sub1[idx]);
							lv_img_set_src(img_env_sub1_day_icon[idx], &sun);
							lv_obj_set_style_img_recolor(img_env_sub1_day_icon[idx], lv_color_black(), 0);
							lv_obj_set_style_img_recolor_opa(img_env_sub1_day_icon[idx], LV_OPA_COVER, 0);
							lv_obj_align(img_env_sub1_day_icon[idx], LV_ALIGN_TOP_LEFT, 66, 19);
							// 白天天气状况
							label_env_sub1_day_weather[idx] = lv_label_create(scr_env_sub1[idx]);
							lv_obj_set_style_text_font(label_env_sub1_day_weather[idx], &font_china_14, 0);
							lv_obj_set_style_text_color(label_env_sub1_day_weather[idx], lv_color_black(), 0);
							lv_obj_align_to(label_env_sub1_day_weather[idx], img_env_sub1_day_icon[idx], LV_ALIGN_OUT_RIGHT_MID, 2, 0);
							// 夜间天气标签
							label_env_sub1_night_title[idx] = lv_label_create(scr_env_sub1[idx]);
							lv_obj_set_style_text_font(label_env_sub1_night_title[idx], &font_china_14, 0);
							lv_obj_set_style_text_color(label_env_sub1_night_title[idx], lv_color_black(), 0);
							lv_obj_align(label_env_sub1_night_title[idx], LV_ALIGN_TOP_LEFT, 4, 42);
							// 夜间天气图标
							img_env_sub1_night_icon[idx] = lv_img_create(scr_env_sub1[idx]);
							lv_img_set_src(img_env_sub1_night_icon[idx], &sun);
							lv_obj_set_style_img_recolor(img_env_sub1_night_icon[idx], lv_color_black(), 0);
							lv_obj_set_style_img_recolor_opa(img_env_sub1_night_icon[idx], LV_OPA_COVER, 0);
							lv_obj_align(img_env_sub1_night_icon[idx], LV_ALIGN_TOP_LEFT, 66, 41);
							// 夜间天气状况
							label_env_sub1_night_weather[idx] = lv_label_create(scr_env_sub1[idx]);
							lv_obj_set_style_text_font(label_env_sub1_night_weather[idx], &font_china_14, 0);
							lv_obj_set_style_text_color(label_env_sub1_night_weather[idx], lv_color_black(), 0);
							lv_obj_align_to(label_env_sub1_night_weather[idx], img_env_sub1_night_icon[idx], LV_ALIGN_OUT_RIGHT_MID, 2, 0);
							// --- 右侧：降水量与降水概率 ---
							// 降水量标签
							label_env_sub1_rain_title[idx] = lv_label_create(scr_env_sub1[idx]);
							lv_obj_set_style_text_font(label_env_sub1_rain_title[idx], &font_china_14, 0);
							lv_obj_set_style_text_color(label_env_sub1_rain_title[idx], lv_color_black(), 0);
							lv_obj_align(label_env_sub1_rain_title[idx], LV_ALIGN_TOP_LEFT, 134, 20);

							// 降水量数值
							label_env_sub1_rain_val[idx] = lv_label_create(scr_env_sub1[idx]);
							lv_obj_set_style_text_font(label_env_sub1_rain_val[idx], &font_china_14, 0);
							lv_obj_set_style_text_color(label_env_sub1_rain_val[idx], lv_color_black(), 0);
							lv_obj_align(label_env_sub1_rain_val[idx], LV_ALIGN_TOP_LEFT, 192, 20);

							// 降水概率标签
							label_env_sub1_pop_title[idx] = lv_label_create(scr_env_sub1[idx]);
							lv_obj_set_style_text_font(label_env_sub1_pop_title[idx], &font_china_14, 0);
							lv_obj_set_style_text_color(label_env_sub1_pop_title[idx], lv_color_black(), 0);
							lv_obj_align(label_env_sub1_pop_title[idx], LV_ALIGN_TOP_LEFT, 134, 42);

							// 降水概率数值
							label_env_sub1_pop_val[idx] = lv_label_create(scr_env_sub1[idx]);
							lv_obj_set_style_text_font(label_env_sub1_pop_val[idx], &font_china_14, 0);
							lv_obj_set_style_text_color(label_env_sub1_pop_val[idx], lv_color_black(), 0);
							lv_obj_align(label_env_sub1_pop_val[idx], LV_ALIGN_TOP_LEFT, 200, 42);

							// 降水概率微型进度条
							bar_env_sub1_pop[idx] = lv_bar_create(scr_env_sub1[idx]);
							lv_obj_set_size(bar_env_sub1_pop[idx], 26, 6);
							lv_obj_align(bar_env_sub1_pop[idx], LV_ALIGN_TOP_LEFT, 226, 46);
							lv_obj_set_style_bg_opa(bar_env_sub1_pop[idx], LV_OPA_0, LV_PART_MAIN);               // 内部透明（全暗）
							lv_obj_set_style_border_width(bar_env_sub1_pop[idx], 1, LV_PART_MAIN);                // 加一个框线
							lv_obj_set_style_border_color(bar_env_sub1_pop[idx], lv_color_black(), LV_PART_MAIN); 
							lv_obj_set_style_border_opa(bar_env_sub1_pop[idx], LV_OPA_40, LV_PART_MAIN);          // 框线亮度压低
							lv_obj_set_style_bg_color(bar_env_sub1_pop[idx], lv_color_black(), LV_PART_INDICATOR);
							lv_obj_set_style_bg_opa(bar_env_sub1_pop[idx], LV_OPA_COVER, LV_PART_INDICATOR);
							lv_obj_set_style_radius(bar_env_sub1_pop[idx], 2, 0);
}

/* 设置列表刷新：按 sys_cursor/sys_list_top 重画 3 行文本 + 箭头位置 + 选中行反相 */
static void sys_list_refresh(void)
{
    for (int r = 0; r < 3; r++) {
        uint8_t idx = sys_list_top + r;
        /* 选中反相：仅导航态/编辑态有光标，且本行即光标所在行 */
        uint8_t selected = (sys_mode != SYS_MODE_BROWSE) && (idx < 8) && (idx == sys_cursor);

        if (idx < 8) {
            lv_label_set_text(label_sys_item_name[r], sys_item_names[idx]);
            /* 每行的值由各自独立的变量生成，参照时光工坊标签的写法 */
            switch (idx) {
                case 0: lv_label_set_text_fmt(label_sys_item_val[r], "%02d:%02d", set_hour, set_min); break;
                case 1: lv_label_set_text_fmt(label_sys_item_val[r], "%dm/%dm", pomo_work_min, pomo_break_min); break;
                case 2: lv_label_set_text(label_sys_item_val[r], SET_ONOFF_MAP[set_vib]);    break;
                case 3: lv_label_set_text(label_sys_item_val[r], "去配网");                  break;
                case 4: lv_label_set_text(label_sys_item_val[r], SET_LEVEL_MAP[set_bright]);break;
                case 5: lv_label_set_text(label_sys_item_val[r], SET_SLEEP_MAP[set_sleep]); break;
                case 6: lv_label_set_text(label_sys_item_val[r], SET_ONOFF_MAP[set_buzzer]);break;
                case 7: lv_label_set_text(label_sys_item_val[r], SET_LEVEL_MAP[set_tap]);   break;
            }
        } else {
            lv_label_set_text(label_sys_item_name[r], "");
            lv_label_set_text(label_sys_item_val[r],  "");
        }

        /* 衬底 + 文字反相（OLED 反相屏：黑底=亮块，白字=灭，整行呈选中反白效果）*/
        if (selected) {
            lv_obj_set_style_bg_color(sys_row_bg[r], lv_color_black(), 0);
            lv_obj_set_style_text_color(label_sys_item_name[r], lv_color_white(), 0);
            lv_obj_set_style_text_color(label_sys_item_val[r],  lv_color_white(), 0);
        } else {
            lv_obj_set_style_bg_color(sys_row_bg[r], lv_color_white(), 0);
            lv_obj_set_style_text_color(label_sys_item_name[r], lv_color_black(), 0);
            lv_obj_set_style_text_color(label_sys_item_val[r],  lv_color_black(), 0);
        }
    }
    if (sys_mode != SYS_MODE_BROWSE) {                   // 导航态/编辑态都显示箭头（编辑时箭头仍指向正在改的项）
        lv_obj_set_pos(label_sys_arrow, 2, 17 + (sys_cursor - sys_list_top) * 15);
        lv_label_set_text(label_sys_arrow, ">");
        lv_obj_set_style_text_color(label_sys_arrow, lv_color_white(), 0);  // 落在黑底亮块上，箭头用白(灭)才可见
    } else {
        lv_label_set_text(label_sys_arrow, "");         // 浏览态不显示箭头
    }

    /* ---- 时间项编辑：拆成 时/":" /分，当前字段在整行反相上再二次反相（白底黑字小块）---- */
    uint8_t time_editing = (sys_mode == SYS_MODE_EDIT) && (sys_cursor == 0);
    for (int r = 0; r < 3; r++)                          // 默认 3 行值标签都可见
        lv_obj_clear_flag(label_sys_item_val[r], LV_OBJ_FLAG_HIDDEN);

    if (time_editing) {
        uint8_t r0 = sys_cursor - sys_list_top;         // 时间项当前所在物理行(0~2)
        lv_coord_t yt = 17 + (lv_coord_t)r0 * 15;      // 文字行 y
        lv_coord_t yb = 17 + (lv_coord_t)r0 * 15;      // 底块  y：与文字同顶，高14罩全字

        lv_obj_add_flag(label_sys_item_val[r0], LV_OBJ_FLAG_HIDDEN);   // 合并值让位给拆分字段

        lv_label_set_text_fmt(sys_t_hour, "%02d", set_hour);
        lv_label_set_text_fmt(sys_t_min,  "%02d", set_min);

        lv_obj_set_pos(sys_t_focus_bg[0], 187, yb);
        lv_obj_set_pos(sys_t_hour,        190, yt);
        lv_obj_set_pos(sys_t_colon,       206, yt);
        lv_obj_set_pos(sys_t_focus_bg[1], 211, yb);
        lv_obj_set_pos(sys_t_min,         214, yt);

        /* 当前字段=黑字(亮)+白底(灭块)二次反相；另一字段=白字(灭)融入整行亮块，无底块 */
        if (sys_field == 0) {                           // 编辑"时"
            lv_obj_clear_flag(sys_t_focus_bg[0], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(sys_t_focus_bg[0], lv_color_white(), 0);
            lv_obj_set_style_bg_opa(sys_t_focus_bg[0], LV_OPA_COVER, 0);
            lv_obj_add_flag(sys_t_focus_bg[1], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_text_color(sys_t_hour, lv_color_black(), 0);
            lv_obj_set_style_text_color(sys_t_min,  lv_color_white(), 0);
        } else {                                        // 编辑"分"
            lv_obj_add_flag(sys_t_focus_bg[0], LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(sys_t_focus_bg[1], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(sys_t_focus_bg[1], lv_color_white(), 0);
            lv_obj_set_style_bg_opa(sys_t_focus_bg[1], LV_OPA_COVER, 0);
            lv_obj_set_style_text_color(sys_t_hour, lv_color_white(), 0);
            lv_obj_set_style_text_color(sys_t_min,  lv_color_black(), 0);
        }
        lv_obj_set_style_text_color(sys_t_colon, lv_color_white(), 0);

        lv_obj_clear_flag(sys_t_hour,  LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(sys_t_colon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(sys_t_min,   LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(sys_t_focus_bg[0], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(sys_t_focus_bg[1], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(sys_t_hour,  LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(sys_t_colon, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(sys_t_min,   LV_OBJ_FLAG_HIDDEN);
    }
}

void ui_create(void)   /* 由 main.c 初始化阶段调用（须在 lv_init/lv_port_disp_init 之后） */
{
//------------------------------------------------------------------------创建屏幕------------------------------------------------------------------------

					// 30 个基础的空白屏幕 ---
					 for (int p = 0; p < 5; p++) {
						for (int s = 0; s < 6; s++) {
								ui_scr[p][s] = lv_obj_create(NULL);
								lv_obj_clear_flag(ui_scr[p][s], LV_OBJ_FLAG_SCROLLABLE);
								lv_obj_set_style_bg_color(ui_scr[p][s], lv_color_white(), 0);
								lv_obj_set_style_bg_opa(ui_scr[p][s], LV_OPA_COVER, 0);
						}
					}
					 
//------------------------------------------------------------------------时钟页主页------------------------------------------------------------------------
					
					scr_home = ui_scr[0][0];//容器
					// 【1.1 中间大时间】
					label_home_time = lv_label_create(scr_home);
					lv_obj_set_style_text_font(label_home_time, &lv_font_montserrat_32, 0);
					lv_obj_align(label_home_time, LV_ALIGN_CENTER, 0, 0);
					// 【1.2 左侧分割线】
					sep_left = lv_obj_create(scr_home);
					lv_obj_set_size(sep_left, 1, 32);
					lv_obj_align(sep_left, LV_ALIGN_LEFT_MID, 70, 0);
					lv_obj_set_style_bg_color(sep_left, lv_color_black(), 0);
					lv_obj_set_style_bg_opa(sep_left, LV_OPA_COVER, 0);
					lv_obj_set_style_pad_all(sep_left, 0, 0);
					lv_obj_set_style_radius(sep_left, 0, 0);
					lv_obj_set_style_border_width(sep_left, 0, 0);
					// 【1.3 左侧：温度与天气】
					label_home_temp = lv_label_create(scr_home);
					lv_obj_align(label_home_temp, LV_ALIGN_LEFT_MID, 10, -10);
					label_home_weather = lv_label_create(scr_home);
					lv_obj_set_style_text_font(label_home_weather, &font_china_14, 0);
					lv_obj_align(label_home_weather, LV_ALIGN_LEFT_MID, 10, 10);
					// 【1.4 天气图标】
					img_home_weather = lv_img_create(scr_home);
					lv_img_set_src(img_home_weather, &sun); 
					lv_obj_set_style_img_recolor(img_home_weather, lv_color_black(), 0);
					lv_obj_set_style_img_recolor_opa(img_home_weather, LV_OPA_COVER, 0);
					lv_obj_align_to(img_home_weather, label_home_weather, LV_ALIGN_OUT_RIGHT_MID, 4, 0);
					// 【1.5 右侧分割线】
					sep_right = lv_obj_create(scr_home);
					lv_obj_set_size(sep_right, 1, 32);
					lv_obj_align(sep_right, LV_ALIGN_RIGHT_MID, -70, 0);
					lv_obj_set_style_bg_color(sep_right, lv_color_black(), 0);
					lv_obj_set_style_bg_opa(sep_right, LV_OPA_COVER, 0);
					lv_obj_set_style_pad_all(sep_right, 0, 0);
					lv_obj_set_style_radius(sep_right, 0, 0);
					lv_obj_set_style_border_width(sep_right, 0, 0);
					// 【1.5 右侧：日期】
					label_home_date = lv_label_create(scr_home);
					lv_obj_set_style_text_font(label_home_date, &font_china_14, 0);
					lv_obj_align(label_home_date, LV_ALIGN_RIGHT_MID, -4, -10);
					// 【1.6 右侧：星期
					obj_home_week_bg = lv_obj_create(scr_home);
					lv_obj_set_size(obj_home_week_bg, 48, 18);
					lv_obj_align(obj_home_week_bg, LV_ALIGN_RIGHT_MID, -8, 11);
					lv_obj_set_style_bg_color(obj_home_week_bg, lv_color_black(), 0);
					lv_obj_set_style_bg_opa(obj_home_week_bg, LV_OPA_COVER, 0);
					lv_obj_set_style_radius(obj_home_week_bg, 4, 0);
					lv_obj_set_style_border_width(obj_home_week_bg, 0, 0);
					lv_obj_set_style_pad_all(obj_home_week_bg, 0, 0);
					lv_obj_clear_flag(obj_home_week_bg, LV_OBJ_FLAG_SCROLLABLE);
					label_home_week = lv_label_create(obj_home_week_bg);
					lv_obj_set_style_text_font(label_home_week, &font_china_14, 0);
					lv_obj_set_style_text_color(label_home_week, lv_color_white(), 0);
					lv_obj_align(label_home_week, LV_ALIGN_CENTER, 0, 0);
					
//------------------------------------------------------------------------环境主页------------------------------------------------------------------------
				
					scr_env = ui_scr[1][0];			//容器
					// 城市
					obj_env_city_bg = lv_obj_create(scr_env);
					lv_obj_set_size(obj_env_city_bg, LV_SIZE_CONTENT, 15);
					lv_obj_align(obj_env_city_bg, LV_ALIGN_TOP_LEFT, 8, 1);
					lv_obj_set_style_bg_color(obj_env_city_bg, lv_color_black(), 0);
					lv_obj_set_style_bg_opa(obj_env_city_bg, LV_OPA_COVER, 0);
					lv_obj_set_style_radius(obj_env_city_bg, 3, 0);
					lv_obj_set_style_pad_left(obj_env_city_bg, 3, 0);
					lv_obj_set_style_pad_right(obj_env_city_bg, 3, 0);
					lv_obj_set_style_pad_top(obj_env_city_bg, 0, 0);
					lv_obj_set_style_pad_bottom(obj_env_city_bg, 0, 0);
					lv_obj_set_style_border_width(obj_env_city_bg, 0, 0);
					lv_obj_clear_flag(obj_env_city_bg, LV_OBJ_FLAG_SCROLLABLE);
					label_env_city = lv_label_create(obj_env_city_bg);
					lv_obj_set_style_text_font(label_env_city, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_city, lv_color_white(), 0);
					lv_obj_align(label_env_city, LV_ALIGN_CENTER, 0, 0);
					//今天日期
					label_env_date_today = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_date_today, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_date_today, lv_color_black(), 0);
					lv_obj_align(label_env_date_today, LV_ALIGN_TOP_LEFT, 8, 17);
					//湿度
					label_env_humidity = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_humidity, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_humidity, lv_color_black(), 0);
					lv_obj_align(label_env_humidity, LV_ALIGN_TOP_LEFT, 8, 33);
					//风力
					label_env_wind = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_wind, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_wind, lv_color_black(), 0);
					lv_obj_align(label_env_wind, LV_ALIGN_TOP_LEFT, 8, 48);
					//分割线
					obj_env_sep = lv_obj_create(scr_env);
					lv_obj_set_size(obj_env_sep, 1, 44);
					lv_obj_align(obj_env_sep, LV_ALIGN_LEFT_MID, 96, 0);
					lv_obj_set_style_bg_color(obj_env_sep, lv_color_black(), 0);
					lv_obj_set_style_bg_opa(obj_env_sep, LV_OPA_40, 0); 
					lv_obj_set_style_pad_all(obj_env_sep, 0, 0);
					lv_obj_set_style_border_width(obj_env_sep, 0, 0);
					//明天
					label_env_date_tomorrow = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_date_tomorrow, &font_china_14, 0);
					lv_obj_align(label_env_date_tomorrow, LV_ALIGN_TOP_LEFT, 102, 5);
					//明天的天气图标
					img_env_weather_tomorrow = lv_img_create(scr_env);
					lv_img_set_src(img_env_weather_tomorrow, &sun);  // 默认初始图标
					lv_obj_set_style_img_recolor(img_env_weather_tomorrow, lv_color_black(), 0);
					lv_obj_set_style_img_recolor_opa(img_env_weather_tomorrow, LV_OPA_COVER, 0);
					lv_obj_align(img_env_weather_tomorrow, LV_ALIGN_LEFT_MID, 105, 2);
					//明天天气
					label_env_weather_tomorrow = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_weather_tomorrow, &font_china_14, 0);
					lv_obj_align_to(label_env_weather_tomorrow, img_env_weather_tomorrow, LV_ALIGN_OUT_RIGHT_MID, 4, 0);
					//明天温度范围
					label_env_temp_tomorrow = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_temp_tomorrow, &font_china_14, 0);
					lv_obj_align(label_env_temp_tomorrow, LV_ALIGN_BOTTOM_LEFT, 111, -5);
					//后天
					label_env_date_after_tomorrow = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_date_after_tomorrow, &font_china_14, 0);
					lv_obj_align(label_env_date_after_tomorrow, LV_ALIGN_TOP_LEFT, 182, 5);
					//后天的天气图标
					img_env_weather_after_tomorrow = lv_img_create(scr_env);
					lv_img_set_src(img_env_weather_after_tomorrow, &sun);  // 默认初始图标
					lv_obj_set_style_img_recolor(img_env_weather_after_tomorrow, lv_color_black(), 0);
					lv_obj_set_style_img_recolor_opa(img_env_weather_after_tomorrow, LV_OPA_COVER, 0);
					lv_obj_align(img_env_weather_after_tomorrow, LV_ALIGN_LEFT_MID, 185, 2);
					//后天天气状况
					label_env_weather_after_tomorrow = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_weather_after_tomorrow, &font_china_14, 0);
					lv_obj_align_to(label_env_weather_after_tomorrow, img_env_weather_after_tomorrow, LV_ALIGN_OUT_RIGHT_MID, 4, 0);
					//后天温度范围
					label_env_temp_after_tomorrow = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_temp_after_tomorrow, &font_china_14, 0);
					lv_obj_align(label_env_temp_after_tomorrow, LV_ALIGN_BOTTOM_LEFT, 191, -5);
					
//------------------------------------------------------------------------环境子页123------------------------------------------------------------------------
					
					for(uint8_t i=0;i<3;i++)
					create_env_sub_page(i);
					
//------------------------------------------------------------------------环境子页4--------------------------------------------------------------------------

					scr_env = ui_scr[1][4];
					// 顶部标题（无图标，直接左上角对齐）
					label_sub4_title = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_sub4_title, &font_china_14, 0);
					lv_obj_set_style_text_color(label_sub4_title, lv_color_black(), 0);
					lv_obj_align(label_sub4_title, LV_ALIGN_TOP_LEFT, 4, 1);
					// 页码角标背景 (4/4 >)
					obj_env_sub4_badge_bg = lv_obj_create(scr_env);
					lv_obj_set_size(obj_env_sub4_badge_bg, LV_SIZE_CONTENT, 14);
					lv_obj_align(obj_env_sub4_badge_bg, LV_ALIGN_TOP_RIGHT, -4, 1);
					lv_obj_set_style_bg_color(obj_env_sub4_badge_bg, lv_color_hex(0x2C2C2C), 0);
					lv_obj_set_style_bg_opa(obj_env_sub4_badge_bg, LV_OPA_COVER, 0);
					lv_obj_set_style_radius(obj_env_sub4_badge_bg, 2, 0);
					lv_obj_set_style_pad_all(obj_env_sub4_badge_bg, 0, 0);
					lv_obj_set_style_pad_left(obj_env_sub4_badge_bg, 4, 0);
					lv_obj_set_style_pad_right(obj_env_sub4_badge_bg, 4, 0);
					lv_obj_set_style_border_width(obj_env_sub4_badge_bg, 0, 0);
					lv_obj_clear_flag(obj_env_sub4_badge_bg, LV_OBJ_FLAG_SCROLLABLE);
					// 页码角标文字
					label_env_sub4_badge = lv_label_create(obj_env_sub4_badge_bg);
					lv_obj_set_style_text_font(label_env_sub4_badge, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_sub4_badge, lv_color_hex(0xCCCCCC), 0);
					lv_obj_align(label_env_sub4_badge, LV_ALIGN_CENTER, 0, 0);
					// 顶部分割线
					obj_env_sub4_head_sep = lv_obj_create(scr_env);
					lv_obj_set_size(obj_env_sub4_head_sep, 248, 1);
					lv_obj_align(obj_env_sub4_head_sep, LV_ALIGN_TOP_LEFT, 4, 17);
					lv_obj_set_style_bg_color(obj_env_sub4_head_sep, lv_color_black(), 0);
					lv_obj_set_style_bg_opa(obj_env_sub4_head_sep, LV_OPA_20, 0);
					lv_obj_set_style_pad_all(obj_env_sub4_head_sep, 0, 0);
					lv_obj_set_style_border_width(obj_env_sub4_head_sep, 0, 0);
					// "今天" 标签
					label_env_sub4_today_title = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_sub4_today_title, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_sub4_today_title, lv_color_black(), 0);
					lv_obj_align(label_env_sub4_today_title, LV_ALIGN_TOP_LEFT, 4, 22);
					// 今天风向
					label_env_sub4_today_wind = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_sub4_today_wind, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_sub4_today_wind, lv_color_black(), 0);
					lv_obj_align(label_env_sub4_today_wind, LV_ALIGN_TOP_LEFT, 75, 22);
					// 今天 "湿" 标签
					label_env_sub4_today_hum_title = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_sub4_today_hum_title, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_sub4_today_hum_title, lv_color_black(), 0);
					lv_obj_align(label_env_sub4_today_hum_title, LV_ALIGN_TOP_LEFT, 210, 22);
					// 今天湿度数值
					label_env_sub4_today_hum_val = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_sub4_today_hum_val, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_sub4_today_hum_val, lv_color_black(), 0);
					lv_obj_align(label_env_sub4_today_hum_val, LV_ALIGN_TOP_LEFT, 230, 22);
					// 行中间分隔线
					obj_env_sub4_row_sep = lv_obj_create(scr_env);
					lv_obj_set_size(obj_env_sub4_row_sep, 248, 1);
					lv_obj_align(obj_env_sub4_row_sep, LV_ALIGN_TOP_LEFT, 4, 40);
					lv_obj_set_style_bg_color(obj_env_sub4_row_sep, lv_color_black(), 0);
					lv_obj_set_style_bg_opa(obj_env_sub4_row_sep, LV_OPA_10, 0);
					lv_obj_set_style_pad_all(obj_env_sub4_row_sep, 0, 0);
					lv_obj_set_style_border_width(obj_env_sub4_row_sep, 0, 0);
					// "明天" 标签
					label_env_sub4_tomorrow_title = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_sub4_tomorrow_title, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_sub4_tomorrow_title, lv_color_black(), 0);
					lv_obj_align(label_env_sub4_tomorrow_title, LV_ALIGN_TOP_LEFT, 4, 44);
					// 明天风向
					label_env_sub4_tomorrow_wind = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_sub4_tomorrow_wind, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_sub4_tomorrow_wind, lv_color_black(), 0);
					lv_obj_align(label_env_sub4_tomorrow_wind, LV_ALIGN_TOP_LEFT, 75, 44);
					// 明天 "湿" 标签
					label_env_sub4_tomorrow_hum_title = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_sub4_tomorrow_hum_title, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_sub4_tomorrow_hum_title, lv_color_black(), 0);
					lv_obj_align(label_env_sub4_tomorrow_hum_title, LV_ALIGN_TOP_LEFT, 210, 44);
					// 明天湿度数值
					label_env_sub4_tomorrow_hum_val = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_sub4_tomorrow_hum_val, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_sub4_tomorrow_hum_val, lv_color_black(), 0);
					lv_obj_align(label_env_sub4_tomorrow_hum_val, LV_ALIGN_TOP_LEFT, 230, 44);
					// 行中间分隔线
					lobj_env_sub4_bottom_sep = lv_obj_create(scr_env);
					lv_obj_set_size(lobj_env_sub4_bottom_sep, 248, 1);
					lv_obj_align(lobj_env_sub4_bottom_sep, LV_ALIGN_TOP_LEFT, 4, 63);
					lv_obj_set_style_bg_color(lobj_env_sub4_bottom_sep, lv_color_black(), 0);
					lv_obj_set_style_bg_opa(lobj_env_sub4_bottom_sep, LV_OPA_10, 0);
					lv_obj_set_style_pad_all(lobj_env_sub4_bottom_sep, 0, 0);
					lv_obj_set_style_border_width(lobj_env_sub4_bottom_sep, 0, 0);

//------------------------------------------------------------------------室内环境页主页------------------------------------------------------------------------
					// 1. 左上角标题框背景
					scr_env = ui_scr[2][0];
					lv_obj_t *title_bg = lv_obj_create(scr_env);
					lv_obj_set_size(title_bg, LV_SIZE_CONTENT, 16);
					lv_obj_align(title_bg, LV_ALIGN_TOP_LEFT, 4, 2);
					lv_obj_set_style_bg_color(title_bg, lv_color_white(), 0);
					lv_obj_set_style_bg_opa(title_bg, LV_OPA_COVER, 0);
					lv_obj_set_style_radius(title_bg, 2, 0);
					lv_obj_set_style_pad_all(title_bg, 2, 0);
					lv_obj_set_style_border_width(title_bg, 0, 0);
					lv_obj_clear_flag(title_bg, LV_OBJ_FLAG_SCROLLABLE);
					label_env_main_title = lv_label_create(title_bg);
					lv_obj_set_style_text_font(label_env_main_title, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_main_title, lv_color_black(), 0);
					lv_obj_align(label_env_main_title, LV_ALIGN_CENTER, 0, 0);
					// 2. 左右垂直分割线
					lv_obj_t *ver_sep = lv_obj_create(scr_env);
					lv_obj_set_size(ver_sep, 1, 64);
					lv_obj_align(ver_sep, LV_ALIGN_TOP_LEFT, 140, 0);
					lv_obj_set_style_bg_color(ver_sep, lv_color_black(), 0);
					lv_obj_set_style_bg_opa(ver_sep, LV_OPA_40, 0);
					lv_obj_set_style_pad_all(ver_sep, 0, 0);
					lv_obj_set_style_border_width(ver_sep, 0, 0);
					// 4. 右侧标题（本地 3h 预测）
					label_env_3h_title = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_3h_title, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_3h_title, lv_color_black(), 0);
					lv_obj_align(label_env_3h_title, LV_ALIGN_TOP_LEFT, 162, 2);
					// 5. 顶栏全宽分割线
					lv_obj_t *head_sep = lv_obj_create(scr_env);
					lv_obj_set_size(head_sep, 256, 1);
					lv_obj_align(head_sep, LV_ALIGN_TOP_LEFT, 0, 17);
					lv_obj_set_style_bg_color(head_sep, lv_color_black(), 0);
					lv_obj_set_style_bg_opa(head_sep, LV_OPA_40, 0);
					lv_obj_set_style_pad_all(head_sep, 0, 0);
					lv_obj_set_style_border_width(head_sep, 0, 0);
					label_env_indoor_temp_title = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_indoor_temp_title, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_indoor_temp_title, lv_color_black(), 0);
					lv_obj_align(label_env_indoor_temp_title, LV_ALIGN_TOP_LEFT, 4, 20);
					label_env_indoor_temp_val = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_indoor_temp_val, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_indoor_temp_val, lv_color_black(), 0);
					lv_obj_align(label_env_indoor_temp_val, LV_ALIGN_TOP_LEFT, 75, 20);
					label_env_hum_title = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_hum_title, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_hum_title, lv_color_black(), 0);
					lv_obj_align(label_env_hum_title, LV_ALIGN_TOP_LEFT, 4, 35);
					label_env_hum_val = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_hum_val, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_hum_val, lv_color_black(), 0);
					lv_obj_align(label_env_hum_val, LV_ALIGN_TOP_LEFT, 75, 35);
					label_env_pressure_title = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_pressure_title, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_pressure_title, lv_color_black(), 0);
					lv_obj_align(label_env_pressure_title, LV_ALIGN_TOP_LEFT, 4, 50);
					label_env_pressure_val = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_pressure_val, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_pressure_val, lv_color_black(), 0);
					lv_obj_align(label_env_pressure_val, LV_ALIGN_TOP_LEFT, 75, 50);
					img_env_3h_icon = lv_img_create(scr_env);
					lv_img_set_src(img_env_3h_icon, &cloud);  // 1. 创建时设置默认图标
					// 2. 强行着色为黑色（解决白底隐形问题）
					lv_obj_set_style_img_recolor(img_env_3h_icon, lv_color_black(), 0);
					lv_obj_set_style_img_recolor_opa(img_env_3h_icon, LV_OPA_COVER, 0);
					// 3. 使用对齐方式或安全坐标
					lv_obj_align(img_env_3h_icon, LV_ALIGN_TOP_LEFT, 144, 24);
					//预测天气情况
					label_env_3h_weather = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_3h_weather, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_3h_weather, lv_color_black(), 0);
					lv_obj_align(label_env_3h_weather, LV_ALIGN_TOP_LEFT, 164, 25);
					label_env_3h_pressure_trend = lv_label_create(scr_env);
					lv_obj_set_style_text_font(label_env_3h_pressure_trend, &font_china_14, 0);
					lv_obj_set_style_text_color(label_env_3h_pressure_trend, lv_color_black(), 0);
					lv_obj_align(label_env_3h_pressure_trend, LV_ALIGN_TOP_LEFT, 144, 46);

//------------------------------------------------------------------------室内环境123------------------------------------------------------------------------
					
					scr_env = ui_scr[2][1];
					ui_trend_card_create(scr_env, 0, "< 温度3小时趋势", "1/5", "°C"); // 数值用 montserrat 字体，没有 ℃ 字形，只能写 °C
					scr_env = ui_scr[2][2];
					ui_trend_card_create(scr_env, 1, "< 湿度3小时趋势", "2/5", "%");
					scr_env = ui_scr[2][3];
					ui_trend_card_create(scr_env, 2, "< 气压3小时趋势", "3/5", "Pa");
					
//------------------------------------------------------------------------室内环境4------------------------------------------------------------------------

		// ---------------- 1. 主卡片容器 ----------------
scr_env = ui_scr[2][4];
lv_obj_set_size(scr_env, 254, 62);
lv_obj_align(scr_env, LV_ALIGN_CENTER, 0, 0);
// 卡片背景：用 white 让像素点熄灭 (呈现暗色背景)
lv_obj_set_style_bg_color(scr_env, lv_color_white(), 0);
lv_obj_set_style_bg_opa(scr_env, LV_OPA_COVER, 0);
// 边框：用 black 点亮
lv_obj_set_style_border_color(scr_env, lv_color_black(), 0);
lv_obj_set_style_border_width(scr_env, 1, 0);
lv_obj_set_style_radius(scr_env, 6, 0);
lv_obj_set_style_pad_all(scr_env, 0, 0);
lv_obj_clear_flag(scr_env, LV_OBJ_FLAG_SCROLLABLE);

// ---------------- 2. 标题 ----------------
label_fc3h_title = lv_label_create(scr_env);
lv_obj_set_style_text_font(label_fc3h_title, &font_china_14, 0);
// 标题文字：用 black 点亮
lv_obj_set_style_text_color(label_fc3h_title, lv_color_black(), 0);
lv_obj_align(label_fc3h_title, LV_ALIGN_TOP_LEFT, 0, 0);

// ---------------- 3. 右上角 Badge ----------------
label_fc3h_badge = lv_label_create(scr_env);
lv_obj_set_style_text_font(label_fc3h_badge, &font_china_14, 0);
// Badge 文字：用 white 熄灭 (呈现暗色字)[cite: 3]
lv_obj_set_style_text_color(label_fc3h_badge, lv_color_white(), 0);
// Badge 背景：用 black 点亮 (呈现亮色背景)[cite: 3]
lv_obj_set_style_bg_color(label_fc3h_badge, lv_color_black(), 0);
lv_obj_set_style_bg_opa(label_fc3h_badge, LV_OPA_COVER, 0);
lv_obj_set_style_radius(label_fc3h_badge, 2, 0);
lv_obj_set_style_pad_hor(label_fc3h_badge, 3, 0);
lv_obj_set_style_pad_ver(label_fc3h_badge, 0, 0);
lv_obj_align(label_fc3h_badge, LV_ALIGN_TOP_RIGHT, 0, 0);

// ---------------- 4. 分割线 ----------------
obj_fc3h_hdiv = lv_obj_create(scr_env);
lv_obj_set_size(obj_fc3h_hdiv, 246, 1);
// 分割线：用 black 点亮
lv_obj_set_style_bg_color(obj_fc3h_hdiv, lv_color_black(), 0);
lv_obj_set_style_bg_opa(obj_fc3h_hdiv, LV_OPA_40, 0);
lv_obj_set_style_border_width(obj_fc3h_hdiv, 0, 0);
lv_obj_set_pos(obj_fc3h_hdiv, 4, 17);

obj_fc3h_vdiv = lv_obj_create(scr_env);
lv_obj_set_size(obj_fc3h_vdiv, 1, 40);
lv_obj_set_style_bg_color(obj_fc3h_vdiv, lv_color_black(), 0);
lv_obj_set_style_bg_opa(obj_fc3h_vdiv, LV_OPA_30, 0);
lv_obj_set_style_border_width(obj_fc3h_vdiv, 0, 0);
lv_obj_set_pos(obj_fc3h_vdiv, 80, 18);

// ---------------- 5. 左侧天气状态 ----------------
img_fc3h_icon = lv_img_create(scr_env);
// 天气图标笔画为白色，本屏 black 点亮，强制染黑否则白卡上隐形
lv_obj_set_style_img_recolor(img_fc3h_icon, lv_color_black(), 0);
lv_obj_set_style_img_recolor_opa(img_fc3h_icon, LV_OPA_COVER, 0);
lv_obj_set_pos(img_fc3h_icon, 24, 20);

label_fc3h_weather = lv_label_create(scr_env);
lv_obj_set_style_text_font(label_fc3h_weather, &font_china_14, 0);
// 左侧文字：用 black 点亮
lv_obj_set_style_text_color(label_fc3h_weather, lv_color_black(), 0);
lv_obj_set_pos(label_fc3h_weather, 20, 44);

// ---------------- 6. 右侧 3 行项目名称标签 ----------------
label_fc3h_item_pop = lv_label_create(scr_env);
lv_obj_set_style_text_font(label_fc3h_item_pop, &font_china_14, 0);
lv_obj_set_style_text_color(label_fc3h_item_pop, lv_color_black(), 0); // 点亮
lv_label_set_text(label_fc3h_item_pop, "降水概率");
lv_obj_set_pos(label_fc3h_item_pop, 88, 20);

label_fc3h_item_temp = lv_label_create(scr_env);
lv_obj_set_style_text_font(label_fc3h_item_temp, &font_china_14, 0);
lv_obj_set_style_text_color(label_fc3h_item_temp, lv_color_black(), 0); // 点亮
lv_label_set_text(label_fc3h_item_temp, "Δ 温度变化");
lv_obj_set_pos(label_fc3h_item_temp, 88, 33);

label_fc3h_item_press = lv_label_create(scr_env);
lv_obj_set_style_text_font(label_fc3h_item_press, &font_china_14, 0);
lv_obj_set_style_text_color(label_fc3h_item_press, lv_color_black(), 0); // 点亮
lv_label_set_text(label_fc3h_item_press, "Δ 气压变化");
lv_obj_set_pos(label_fc3h_item_press, 88, 46);

// ---------------- 7. 右侧 3 行项目数值标签 ----------------
label_fc3h_val_pop = lv_label_create(scr_env);
lv_obj_set_style_text_font(label_fc3h_val_pop, &font_china_14, 0);
lv_obj_set_style_text_color(label_fc3h_val_pop, lv_color_black(), 0); // 点亮
lv_obj_align(label_fc3h_val_pop, LV_ALIGN_TOP_RIGHT, -8, 20);

label_fc3h_val_temp = lv_label_create(scr_env);
lv_obj_set_style_text_font(label_fc3h_val_temp, &font_china_14, 0);
lv_obj_set_style_text_color(label_fc3h_val_temp, lv_color_black(), 0); // 点亮
lv_obj_align(label_fc3h_val_temp, LV_ALIGN_TOP_RIGHT, -8, 33);

label_fc3h_val_press = lv_label_create(scr_env);
lv_obj_set_style_text_font(label_fc3h_val_press, &font_china_14, 0);
lv_obj_set_style_text_color(label_fc3h_val_press, lv_color_black(), 0); // 点亮
lv_obj_align(label_fc3h_val_press, LV_ALIGN_TOP_RIGHT, -8, 46);

//------------------------------------------------------------------------室内环境5------------------------------------------------------------------------


// ---------------- 1. 主卡片容器 ----------------
scr_env = ui_scr[2][5];
lv_obj_set_size(scr_env, 254, 62);
lv_obj_align(scr_env, LV_ALIGN_CENTER, 0, 0);
// 卡片背景：用 white 让像素点熄灭 (呈现暗色底)
lv_obj_set_style_bg_color(scr_env, lv_color_white(), 0);
lv_obj_set_style_bg_opa(scr_env, LV_OPA_COVER, 0);
// 卡片边框：用 black 点亮
lv_obj_set_style_border_color(scr_env, lv_color_black(), 0);
lv_obj_set_style_border_width(scr_env, 1, 0);
lv_obj_set_style_radius(scr_env, 6, 0);
lv_obj_set_style_pad_all(scr_env, 0, 0);
lv_obj_clear_flag(scr_env, LV_OBJ_FLAG_SCROLLABLE);

// ---------------- 2. 标题 ----------------
label_env_title = lv_label_create(scr_env);
lv_obj_set_style_text_font(label_env_title, &font_china_14, 0);
// 标题文字：用 black 点亮
lv_obj_set_style_text_color(label_env_title, lv_color_black(), 0);
lv_obj_align(label_env_title, LV_ALIGN_TOP_LEFT, 0, 0);

// ---------------- 3. 右上角 Badge (5/5) ----------------
label_env_badge = lv_label_create(scr_env);
lv_obj_set_style_text_font(label_env_badge, &font_china_14, 0);
// Badge文字：用 white 熄灭 (呈现暗色字)
lv_obj_set_style_text_color(label_env_badge, lv_color_white(), 0);
// Badge背景：用 black 点亮 (呈现亮色背景)
lv_obj_set_style_bg_color(label_env_badge, lv_color_black(), 0);
lv_obj_set_style_bg_opa(label_env_badge, LV_OPA_COVER, 0);
lv_obj_set_style_radius(label_env_badge, 2, 0);
lv_obj_set_style_pad_hor(label_env_badge, 3, 0);
lv_obj_set_style_pad_ver(label_env_badge, 0, 0);
lv_obj_align(label_env_badge, LV_ALIGN_TOP_RIGHT, 0, 0);

// ---------------- 4. 分割线 ----------------
obj_env_hdiv = lv_obj_create(scr_env);
lv_obj_set_size(obj_env_hdiv, 246, 1);
lv_obj_set_style_bg_color(obj_env_hdiv, lv_color_black(), 0); // 点亮
lv_obj_set_style_bg_opa(obj_env_hdiv, LV_OPA_40, 0);
lv_obj_set_style_border_width(obj_env_hdiv, 0, 0);
lv_obj_set_pos(obj_env_hdiv, 4, 17);

obj_env_vdiv = lv_obj_create(scr_env);
lv_obj_set_size(obj_env_vdiv, 1, 40);
lv_obj_set_style_bg_color(obj_env_vdiv, lv_color_black(), 0); // 点亮
lv_obj_set_style_bg_opa(obj_env_vdiv, LV_OPA_30, 0);
lv_obj_set_style_border_width(obj_env_vdiv, 0, 0);
lv_obj_set_pos(obj_env_vdiv, 95, 18);  // 由于左边文字较多，竖线设在 x=95 处		

// ---------------- 5. 左侧内容 (图标、主状态、建议标签) ----------------
img_env_icon = lv_img_create(scr_env);
// 图标资源笔画为白色，本屏 black 点亮/white 熄灭，用 recolor 强制染黑否则不可见
lv_obj_set_style_img_recolor(img_env_icon, lv_color_black(), 0);
lv_obj_set_style_img_recolor_opa(img_env_icon, LV_OPA_COVER, 0);
lv_obj_set_pos(img_env_icon, 8, 25);

label_env_status = lv_label_create(scr_env);
lv_obj_set_style_text_font(label_env_status, &font_china_14, 0); 
lv_obj_set_style_text_color(label_env_status, lv_color_black(), 0); // 点亮
lv_obj_set_pos(label_env_status, 30, 22);

label_env_advice = lv_label_create(scr_env);
lv_obj_set_style_text_font(label_env_advice, &font_china_14, 0); 
lv_obj_set_style_text_color(label_env_advice, lv_color_black(), 0); // 文字点亮
lv_obj_set_style_bg_color(label_env_advice, lv_color_white(), 0);   // 背景熄灭 (暗色)
lv_obj_set_style_bg_opa(label_env_advice, LV_OPA_COVER, 0);
lv_obj_set_style_border_color(label_env_advice, lv_color_black(), 0); // 边框点亮
lv_obj_set_style_border_width(label_env_advice, 1, 0);
lv_obj_set_style_radius(label_env_advice, 3, 0);
lv_obj_set_style_pad_hor(label_env_advice, 4, 0);
lv_obj_set_style_pad_ver(label_env_advice, 0, 0);
lv_obj_set_pos(label_env_advice, 28, 39);

// ---------------- 6. 右侧 3 行项目名称 ----------------
label_env_item_dew = lv_label_create(scr_env);
lv_obj_set_style_text_font(label_env_item_dew, &font_china_14, 0);
lv_obj_set_style_text_color(label_env_item_dew, lv_color_black(), 0); // 点亮
lv_obj_set_pos(label_env_item_dew, 104, 19);

label_env_item_outhum = lv_label_create(scr_env);
lv_obj_set_style_text_font(label_env_item_outhum, &font_china_14, 0);
lv_obj_set_style_text_color(label_env_item_outhum, lv_color_black(), 0); // 点亮
lv_obj_set_pos(label_env_item_outhum, 104, 33);

label_env_item_diffhum = lv_label_create(scr_env);
lv_obj_set_style_text_font(label_env_item_diffhum, &font_china_14, 0);
lv_obj_set_style_text_color(label_env_item_diffhum, lv_color_black(), 0); // 点亮
lv_obj_set_pos(label_env_item_diffhum, 104, 46);

// ---------------- 7. 右侧 3 行项目数值 ----------------
label_env_val_dew = lv_label_create(scr_env);
lv_obj_set_style_text_font(label_env_val_dew, &font_china_14, 0);
lv_obj_set_style_text_color(label_env_val_dew, lv_color_black(), 0); // 点亮
lv_obj_align(label_env_val_dew, LV_ALIGN_TOP_RIGHT, -8, 19);

label_env_val_outhum = lv_label_create(scr_env);
lv_obj_set_style_text_font(label_env_val_outhum, &font_china_14, 0);
lv_obj_set_style_text_color(label_env_val_outhum, lv_color_black(), 0); // 点亮
lv_obj_align(label_env_val_outhum, LV_ALIGN_TOP_RIGHT, -8, 33);

label_env_val_diffhum = lv_label_create(scr_env);
lv_obj_set_style_text_font(label_env_val_diffhum, &font_china_14, 0);
lv_obj_set_style_text_color(label_env_val_diffhum, lv_color_black(), 0); // 点亮
lv_obj_align(label_env_val_diffhum, LV_ALIGN_TOP_RIGHT, -8, 46);

//------------------------------------------------------------------------闹钟时钟番茄钟主页------------------------------------------------------------------------
	{
		scr_env = ui_scr[3][0];
		// 关键：屏幕容器必须明确尺寸+居中，否则子元素坐标会飘
		lv_obj_set_size(scr_env, 254, 62);
		lv_obj_align(scr_env, LV_ALIGN_CENTER, 0, 0);

		// --- 1. 顶部标题 "< 时光工坊 >" ---
		label_pomo_header = lv_label_create(scr_env);
		lv_obj_set_style_text_font(label_pomo_header, &font_china_14, 0);
		lv_obj_set_style_text_color(label_pomo_header, lv_color_black(), 0);
		lv_obj_set_pos(label_pomo_header, 4, 1);
		lv_label_set_text(label_pomo_header, "< 时光工坊 >");

		// --- 2. 顶部分割线 ---
		lv_obj_t *pomo_head_sep = lv_obj_create(scr_env);
		lv_obj_set_size(pomo_head_sep, 246, 1);
		lv_obj_set_pos(pomo_head_sep, 4, 16);
		lv_obj_set_style_bg_color(pomo_head_sep, lv_color_black(), 0);
		lv_obj_set_style_bg_opa(pomo_head_sep, LV_OPA_40, 0);
		lv_obj_set_style_border_width(pomo_head_sep, 0, 0);
		lv_obj_clear_flag(pomo_head_sep, LV_OBJ_FLAG_SCROLLABLE);

		// --- 2b. 右上角徽章：今日完成次数（只读统计） ---
		label_pomo_badge = lv_label_create(scr_env);
		lv_obj_set_style_text_font(label_pomo_badge, &font_china_14, 0);
		lv_obj_set_style_text_color(label_pomo_badge, lv_color_black(), 0);
		// 右上角对齐：文本从右边界(留4px)向左延伸，数字位数变化也不会超出屏宽切掉"次"
		lv_obj_align(label_pomo_badge, LV_ALIGN_TOP_RIGHT, -4, 1);
		lv_label_set_text_fmt(label_pomo_badge, "今日专注%d次", pomo_today_cnt);

		// --- 3. 三个卡片：图标左、文字右，横向布局 ---
		// 卡片统一 y=19, h=42（底到 y=61，距容器底 1px）
		// 左卡片 x=4, w=78；中卡片 x=87, w=82；右卡片 x=174, w=78

		card_countdown = lv_obj_create(scr_env);
		lv_obj_set_size(card_countdown, 78, 42);
		lv_obj_set_pos(card_countdown, 4, 19);
		lv_obj_set_style_bg_color(card_countdown, lv_color_white(), 0);
		lv_obj_set_style_bg_opa(card_countdown, LV_OPA_COVER, 0);
		lv_obj_set_style_border_color(card_countdown, lv_color_black(), 0);
		lv_obj_set_style_border_width(card_countdown, 1, 0);
		lv_obj_set_style_radius(card_countdown, 3, 0);
		lv_obj_set_style_pad_all(card_countdown, 0, 0);
		lv_obj_clear_flag(card_countdown, LV_OBJ_FLAG_SCROLLABLE);

		img_countdown_icon = lv_img_create(card_countdown);
		lv_obj_set_size(img_countdown_icon, 16, 16);
		lv_obj_set_pos(img_countdown_icon, 4, 6);   // 左侧居中 (16+16+10)/2=22→(42-16)/2=13
		lv_obj_set_style_img_recolor(img_countdown_icon, lv_color_black(), 0);
		lv_obj_set_style_img_recolor_opa(img_countdown_icon, LV_OPA_COVER, 0);

		label_countdown_name = lv_label_create(card_countdown);
		lv_obj_set_style_text_font(label_countdown_name, &font_china_14, 0);
		lv_obj_set_style_text_color(label_countdown_name, lv_color_black(), 0);
		lv_obj_set_pos(label_countdown_name, 24, 6);   // 图标右侧，上方
		lv_label_set_text(label_countdown_name, "倒计时");

		label_countdown_status = lv_label_create(card_countdown);
		lv_obj_set_style_text_font(label_countdown_status, &font_china_14, 0);
		lv_obj_set_style_text_color(label_countdown_status, lv_color_black(), 0);
		lv_obj_set_pos(label_countdown_status, 2, 22);
		lv_label_set_text(label_countdown_status,STATUS_TEXT_MAP[cd_status]);//已暂停已开启

		label_countdown_time = lv_label_create(card_countdown);
		lv_obj_set_style_text_font(label_countdown_time, &font_china_14, 0);
		lv_obj_set_style_text_color(label_countdown_time, lv_color_black(), 0);
		lv_obj_set_pos(label_countdown_time, 46, 22);   // "已暂停"3字≈42px + 2间距
		lv_label_set_text_fmt(label_countdown_time, "%dm",cd_remain_min);

		// --- 中卡片：番茄钟 ---
		card_pomodoro = lv_obj_create(scr_env);
		lv_obj_set_size(card_pomodoro, 82, 42);
		lv_obj_set_pos(card_pomodoro, 87, 19);
		lv_obj_set_style_bg_color(card_pomodoro, lv_color_white(), 0);
		lv_obj_set_style_bg_opa(card_pomodoro, LV_OPA_COVER, 0);
		lv_obj_set_style_border_color(card_pomodoro, lv_color_black(), 0);
		lv_obj_set_style_border_width(card_pomodoro, 1, 0);
		lv_obj_set_style_radius(card_pomodoro, 3, 0);
		lv_obj_set_style_pad_all(card_pomodoro, 0, 0);
		lv_obj_clear_flag(card_pomodoro, LV_OBJ_FLAG_SCROLLABLE);

		img_pomodoro_icon = lv_img_create(card_pomodoro);
		lv_obj_set_size(img_pomodoro_icon, 16, 16);
		lv_obj_set_pos(img_pomodoro_icon, 4, 6);
		lv_obj_set_style_img_recolor(img_pomodoro_icon, lv_color_black(), 0);
		lv_obj_set_style_img_recolor_opa(img_pomodoro_icon, LV_OPA_COVER, 0);

		label_pomodoro_name = lv_label_create(card_pomodoro);
		lv_obj_set_style_text_font(label_pomodoro_name, &font_china_14, 0);
		lv_obj_set_style_text_color(label_pomodoro_name, lv_color_black(), 0);
		lv_obj_set_pos(label_pomodoro_name, 24, 6);
		lv_label_set_text(label_pomodoro_name, "番茄钟");

		label_pomodoro_status = lv_label_create(card_pomodoro);
		lv_obj_set_style_text_font(label_pomodoro_status, &font_china_14, 0);
		lv_obj_set_style_text_color(label_pomodoro_status, lv_color_black(), 0);
		lv_obj_set_pos(label_pomodoro_status, 2, 22);
		lv_label_set_text(label_pomodoro_status, POMO_STATUS_MAP[pomo_status]);

		label_pomodoro_dur = lv_label_create(card_pomodoro);
		lv_obj_set_style_text_font(label_pomodoro_dur, &font_china_14, 0);
		lv_obj_set_style_text_color(label_pomodoro_dur, lv_color_black(), 0);
		lv_obj_set_pos(label_pomodoro_dur, 46, 22);    // "已暂停"3字≈42px + 2间距
		lv_label_set_text_fmt(label_pomodoro_dur, "%dm", pomo_work_min);

		// --- 右卡片：闹钟 ---
		card_alarm = lv_obj_create(scr_env);
		lv_obj_set_size(card_alarm, 78, 42);
		lv_obj_set_pos(card_alarm, 174, 19);
		lv_obj_set_style_bg_color(card_alarm, lv_color_white(), 0);
		lv_obj_set_style_bg_opa(card_alarm, LV_OPA_COVER, 0);
		lv_obj_set_style_border_color(card_alarm, lv_color_black(), 0);
		lv_obj_set_style_border_width(card_alarm, 1, 0);
		lv_obj_set_style_radius(card_alarm, 3, 0);
		lv_obj_set_style_pad_all(card_alarm, 0, 0);
		lv_obj_clear_flag(card_alarm, LV_OBJ_FLAG_SCROLLABLE);

		img_alarm_icon = lv_img_create(card_alarm);
		lv_obj_set_size(img_alarm_icon, 16, 16);
		lv_obj_set_pos(img_alarm_icon, 4, 6);
		lv_obj_set_style_img_recolor(img_alarm_icon, lv_color_black(), 0);
		lv_obj_set_style_img_recolor_opa(img_alarm_icon, LV_OPA_COVER, 0);

		label_alarm_name = lv_label_create(card_alarm);
		lv_obj_set_style_text_font(label_alarm_name, &font_china_14, 0);
		lv_obj_set_style_text_color(label_alarm_name, lv_color_black(), 0);
		lv_obj_set_pos(label_alarm_name, 24, 6);
		lv_label_set_text(label_alarm_name, "闹钟");

		label_alarm_hour = lv_label_create(card_alarm);
		lv_obj_set_style_text_font(label_alarm_hour, &font_china_14, 0);
		lv_obj_set_style_text_color(label_alarm_hour, lv_color_black(), 0);
		lv_obj_set_pos(label_alarm_hour, 2, 22);
		lv_label_set_text_fmt(label_alarm_hour, "%02d", alarm_hour);

		label_alarm_colon = lv_label_create(card_alarm);
		lv_obj_set_style_text_font(label_alarm_colon, &font_china_14, 0);
		lv_obj_set_style_text_color(label_alarm_colon, lv_color_black(), 0);
		lv_obj_set_pos(label_alarm_colon, 17, 22);   // "11"≈14px + 1
		lv_label_set_text(label_alarm_colon, ":");

		label_alarm_min = lv_label_create(card_alarm);
		lv_obj_set_style_text_font(label_alarm_min, &font_china_14, 0);
		lv_obj_set_style_text_color(label_alarm_min, lv_color_black(), 0);
		lv_obj_set_pos(label_alarm_min, 23, 22);     // ":"≈4px + 2
		lv_label_set_text_fmt(label_alarm_min, "%02d", alarm_min);

		label_alarm_state = lv_label_create(card_alarm);
		lv_obj_set_style_text_font(label_alarm_state, &font_china_14, 0);
		lv_obj_set_style_text_color(label_alarm_state, lv_color_black(), 0);
		lv_obj_set_pos(label_alarm_state, 39, 22);    // "50"≈14px + 2
		lv_label_set_text(label_alarm_state, ALARM_ON_TEXT_MAP[alarm_on]);




		// 第五页 ui_scr[4][SYS_SUB_WIFI] 配网子屏标题（列表页标题在下方单独创建）
		label_sys_title[SYS_SUB_WIFI] = lv_label_create(ui_scr[4][SYS_SUB_WIFI]);
		lv_obj_set_style_text_font(label_sys_title[SYS_SUB_WIFI], &font_china_14, 0);
		lv_obj_set_style_text_color(label_sys_title[SYS_SUB_WIFI], lv_color_black(), 0);

		// WiFi 配网子屏：4行居中，行距15，屏幕中心y=32（末行中心54，字底61不超屏64）
		lv_label_set_text(label_sys_title[SYS_SUB_WIFI], "WiFi配网");
		lv_obj_align(label_sys_title[SYS_SUB_WIFI], LV_ALIGN_CENTER, 0, -23);  // 中心y=9

		label_wifi_ap = lv_label_create(ui_scr[4][SYS_SUB_WIFI]);
		lv_obj_set_style_text_font(label_wifi_ap, &font_china_14, 0);
		lv_obj_set_style_text_color(label_wifi_ap, lv_color_black(), 0);
		lv_obj_align(label_wifi_ap, LV_ALIGN_CENTER, 0, -8);                   // 中心y=24
		lv_label_set_text(label_wifi_ap, "连接Clock-Setup网络");

		label_wifi_ip = lv_label_create(ui_scr[4][SYS_SUB_WIFI]);
		lv_obj_set_style_text_font(label_wifi_ip, &font_china_14, 0);
		lv_obj_set_style_text_color(label_wifi_ip, lv_color_black(), 0);
		lv_obj_align(label_wifi_ip, LV_ALIGN_CENTER, 0, 7);                    // 中心y=39
		lv_label_set_text(label_wifi_ip, "浏览器搜索网址192.168.4.1进行配置");

		label_wifi_exit = lv_label_create(ui_scr[4][SYS_SUB_WIFI]);
		lv_obj_set_style_text_font(label_wifi_exit, &font_china_14, 0);
		lv_obj_set_style_text_color(label_wifi_exit, lv_color_black(), 0);
		lv_obj_align(label_wifi_exit, LV_ALIGN_CENTER, 0, 22);                  // 中心y=54
		lv_label_set_text(label_wifi_exit, "K2 long exit");

		// ui_scr[4][0] = 设置列表页（手表式列表：header + 箭头 + 3 行可见项）
		lv_obj_t *label_set_header = lv_label_create(ui_scr[4][0]);
		lv_obj_set_style_text_font(label_set_header, &font_china_14, 0);
		lv_obj_set_style_text_color(label_set_header, lv_color_black(), 0);
		lv_obj_set_pos(label_set_header, 4, 1);
		lv_label_set_text(label_set_header, "< 设置 >");

		// 3 行反相衬底条：先创建垫在最底层，普通态=白底(屏上熄灭不可见)，选中行由 sys_list_refresh 置黑底
		// 满宽(x=0,w=256)，让左侧 x=2 的箭头也完整压在亮块上，白箭头才可见
		// y=17 与文字同顶、高14(17~30)，完整罩住14px字底；行距15行间留1px缝
		for (int r = 0; r < 3; r++) {
			sys_row_bg[r] = lv_obj_create(ui_scr[4][0]);
			lv_obj_set_size(sys_row_bg[r], 256, 14);
			lv_obj_set_pos(sys_row_bg[r], 0, 17 + r * 15);
			lv_obj_set_style_bg_color(sys_row_bg[r], lv_color_white(), 0);  // 普通：与屏底同白=不亮
			lv_obj_set_style_bg_opa(sys_row_bg[r], LV_OPA_COVER, 0);
			lv_obj_set_style_border_width(sys_row_bg[r], 0, 0);
			lv_obj_set_style_radius(sys_row_bg[r], 0, 0);
			lv_obj_set_style_shadow_width(sys_row_bg[r], 0, 0);
			lv_obj_set_style_pad_all(sys_row_bg[r], 0, 0);
			lv_obj_clear_flag(sys_row_bg[r], LV_OBJ_FLAG_SCROLLABLE);
		}

		// 左侧箭头 ">"：位置由 sys_list_refresh() 按选中行计算
		label_sys_arrow = lv_label_create(ui_scr[4][0]);
		lv_obj_set_style_text_font(label_sys_arrow, &font_china_14, 0);
		lv_obj_set_style_text_color(label_sys_arrow, lv_color_black(), 0);

		// 可见 3 行：项目名 x=16，当前值 x=190，行距 15px
		for (int r = 0; r < 3; r++) {
			label_sys_item_name[r] = lv_label_create(ui_scr[4][0]);
			lv_obj_set_style_text_font(label_sys_item_name[r], &font_china_14, 0);
			lv_obj_set_style_text_color(label_sys_item_name[r], lv_color_black(), 0);
			lv_obj_set_pos(label_sys_item_name[r], 16, 17 + r * 15);

			label_sys_item_val[r] = lv_label_create(ui_scr[4][0]);
			lv_obj_set_style_text_font(label_sys_item_val[r], &font_china_14, 0);
			lv_obj_set_style_text_color(label_sys_item_val[r], lv_color_black(), 0);
			lv_obj_set_pos(label_sys_item_val[r], 190, 17 + r * 15);
		}

		// 时间字段二次反相组（默认隐藏，仅 EDIT+时间项 时由 sys_list_refresh 定位到当前物理行）
		// 先建白底块(垫在数字下)，再建数字 label；x 对齐右侧数值区
		sys_t_focus_bg[0] = lv_obj_create(ui_scr[4][0]);           // 时 底块
		lv_obj_set_size(sys_t_focus_bg[0], 17, 14);
		lv_obj_set_style_radius(sys_t_focus_bg[0], 2, 0);
		lv_obj_set_style_border_width(sys_t_focus_bg[0], 0, 0);
		lv_obj_set_style_pad_all(sys_t_focus_bg[0], 0, 0);
		lv_obj_set_style_shadow_width(sys_t_focus_bg[0], 0, 0);
		lv_obj_clear_flag(sys_t_focus_bg[0], LV_OBJ_FLAG_SCROLLABLE);

		sys_t_focus_bg[1] = lv_obj_create(ui_scr[4][0]);           // 分 底块
		lv_obj_set_size(sys_t_focus_bg[1], 17, 14);
		lv_obj_set_style_radius(sys_t_focus_bg[1], 2, 0);
		lv_obj_set_style_border_width(sys_t_focus_bg[1], 0, 0);
		lv_obj_set_style_pad_all(sys_t_focus_bg[1], 0, 0);
		lv_obj_set_style_shadow_width(sys_t_focus_bg[1], 0, 0);
		lv_obj_clear_flag(sys_t_focus_bg[1], LV_OBJ_FLAG_SCROLLABLE);

		sys_t_hour = lv_label_create(ui_scr[4][0]);
		lv_obj_set_style_text_font(sys_t_hour, &font_china_14, 0);
		sys_t_colon = lv_label_create(ui_scr[4][0]);
		lv_obj_set_style_text_font(sys_t_colon, &font_china_14, 0);
		lv_label_set_text(sys_t_colon, ":");
		sys_t_min = lv_label_create(ui_scr[4][0]);
		lv_obj_set_style_text_font(sys_t_min, &font_china_14, 0);

		lv_obj_add_flag(sys_t_focus_bg[0], LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(sys_t_focus_bg[1], LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(sys_t_hour, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(sys_t_colon, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(sys_t_min, LV_OBJ_FLAG_HIDDEN);
		sys_list_refresh();
	}

					lv_scr_load(scr_home);
}
static const lv_img_dsc_t* get_weather_icon(const char *zh_text) {
    if (strstr(zh_text, "晴")) return &sun; 
		if (strstr(zh_text, "雷")) return &thundershower;
    if (strstr(zh_text, "雨")) return &rain;
    if (strstr(zh_text, "雪")) return &snow;
		if (strstr(zh_text, "阴")) return &overcast;
		if (strstr(zh_text, "云")) return &cloud;
    return &sun; 
}
/**
 * @brief 传入 60 个 float 数据，更新指定卡片上的折线，并按高低比例摆放 4 个数值标签
 * @param slot       TREND_SLOT_TEMP 温度卡片 / TREND_SLOT_HUMI 湿度卡片
 * @param data_array 长度为 60 的数据数组
 */
/**
 * @brief 填充趋势图数据并刷新图表
 * @param idx        0 = 温度卡片, 1 = 湿度卡片
 * @param data_array 包含 60 个浮点数的数据数组
 */
void ui_trend_card_set_data(uint8_t idx, float *data_array)
{
    if (data_array == NULL || idx >= 3) return;
    if (chart_trend[idx] == NULL || series_trend[idx] == NULL) return;

    // 1. 填入 60 个数据，并寻找最大最小值
    float min_f = data_array[0];
    float max_f = data_array[0];
    for (int i = 0; i < 60; i++) {
        series_trend[idx]->y_points[i] = (lv_coord_t)data_array[i];
        if (data_array[i] < min_f) min_f = data_array[i];
        if (data_array[i] > max_f) max_f = data_array[i];
    }

    // 2. 动态计算 Y 轴上下限
    int min_range = (int)min_f - 2;
    int max_range = (int)max_f + 2;
    int range_diff = max_range - min_range;
    if (range_diff <= 0) range_diff = 1;

    lv_chart_set_range(chart_trend[idx], LV_CHART_AXIS_PRIMARY_Y, min_range, max_range);

    // 3. 更新 4 个关键节点的文本数值，并按数值高度摆放 Y 坐标
    lv_coord_t chart_h = 25;

    // 重新计算内容宽度 (原本在结构体里的 t->chart_w)
    lv_coord_t chart_w = 254 - 4; // 254 - 2*1 - 2*2 = 248

    for (int i = 0; i < 4; i++) {
        float v = data_array[trend_key_idx[i]];

        // 先拼字符串再量尺寸：这里的字体必须和 ui_trend_card_create 里给数值标签设的字体一致，
        // 否则宽度算错、居中会偏，高度算错、和折线的空隙就不准
        char buf[16];
        lv_snprintf(buf, sizeof(buf), "%.0f%s", v, str_trend_unit[idx]);
        lv_point_t sz;
        lv_txt_get_size(&sz, buf, &lv_font_montserrat_10, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);

        lv_coord_t x = trend_point_x(chart_w, trend_key_idx[i]) - sz.x / 2;
        if (x < 0) x = 0;
        if (x + sz.x > chart_w) x = chart_w - sz.x;

        // 数据点在图表内的像素行 (0 = 图表顶部, chart_h = 图表底部)
        lv_coord_t pt_y = chart_h - (lv_coord_t)((v - min_range) * chart_h / range_diff);

        // Y：数字永远摆在数据点上方，不再往线下面翻。
        // sz.y 是字体行高 (10 号 = 11px)，但数字墨迹只有 7px，行高底部自带 2px 空白，
        // 所以盒底对齐到数据点行时墨迹已经离线 2px 了，再往上抬 1px → 实际空隙约 3px。
        // 觉得还不够开就把这个 1 加大；数字再贴上去就说明字号换了，行高跟着变。
        lv_coord_t y = pt_y - sz.y - 1;
        if (y < -1) {
            // 数据点太靠图表顶部，上方塞不下 → 顶到图表上沿(-1 是行高顶部的空白，不会切到字)，
            // 同时开白底把底下的折线擦掉一段，看起来仍然是不贴线的
            y = -1;
            lv_obj_set_style_bg_opa(label_trend_val[idx][i], LV_OPA_COVER, 0);
        } else {
            lv_obj_set_style_bg_opa(label_trend_val[idx][i], LV_OPA_0, 0);
        }

        lv_label_set_text(label_trend_val[idx][i], buf);
        lv_obj_set_pos(label_trend_val[idx][i], x, y);
    }

    // 4. 刷新图表
    lv_chart_refresh(chart_trend[idx]);
}
static void parse_date_split(const char *date_in, char *month_out, char *day_out) {
    // 校验字符串长度 (YYYY-MM-DD 至少 10 字节)
    if (!date_in || strlen(date_in) < 10) return;

    // "YYYY-MM-DD" 中，月从 index 5 开始，日从 index 8 开始
    strncpy(month_out, date_in + 5, 2);
    month_out[2] = '\0';

    strncpy(day_out, date_in + 8, 2);
    day_out[2] = '\0';
}
// 1. 改变 Y 轴位移回调
static void anim_y_cb(void * var, int32_t v) {
    lv_obj_set_style_translate_y((lv_obj_t *)var, v, 0);
}

// 2. 改变透明度回调
static void anim_opa_cb(void * var, int32_t v) {
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)v, 0);
}

// 3. 动画结束后自动销毁旧控件
static void anim_del_old_cb(lv_anim_t * a) {
    lv_obj_del((lv_obj_t *)a->var);
}

// 4. 【核心】真正的数字向上翻滚动画函数
static void lv_label_roll_up(lv_obj_t * main_label, const char * new_text)
{
    // 如果文字内容没有变，不播放动画直接返回
    if (strcmp(lv_label_get_text(main_label), new_text) == 0) return;

    // 清理主 Label 上未完成的动画，重置状态
    lv_anim_del(main_label, anim_y_cb);
    lv_anim_del(main_label, anim_opa_cb);
    lv_obj_set_style_translate_y(main_label, 0, 0);
    lv_obj_set_style_opa(main_label, LV_OPA_COVER, 0);

    // 【步骤 A】：创建一个临时的旧 Label，把“旧文字”重叠贴在主 Label 的位置上
    lv_obj_t * parent = lv_obj_get_parent(main_label);
    lv_obj_t * old_label = lv_label_create(parent);
    lv_label_set_text(old_label, lv_label_get_text(main_label));
    
    // 让旧 Label 与主 Label 完全重合
    lv_obj_align_to(old_label, main_label, LV_ALIGN_TOP_LEFT, 0, 0);

    // 【步骤 B】：旧文字向上飞出屏幕 (-20 像素) 并渐变消失，动画结束后销毁自身
    lv_anim_t a_old_y, a_old_opa;
    lv_anim_init(&a_old_y);
    lv_anim_set_var(&a_old_y, old_label);
    lv_anim_set_exec_cb(&a_old_y, anim_y_cb);
    lv_anim_set_values(&a_old_y, 0, -20);            // 从 0 滚到 -20px
    lv_anim_set_time(&a_old_y, 220);                 // 220 毫秒
    lv_anim_start(&a_old_y);

    lv_anim_init(&a_old_opa);
    lv_anim_set_var(&a_old_opa, old_label);
    lv_anim_set_exec_cb(&a_old_opa, anim_opa_cb);
    lv_anim_set_values(&a_old_opa, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a_old_opa, 220);
    lv_anim_set_ready_cb(&a_old_opa, anim_del_old_cb); // 旧 Label 的最后一个动画播完后再销毁，避免 use-after-free
    lv_anim_start(&a_old_opa);

    lv_label_set_text(main_label, new_text);

    lv_anim_t a_new_y, a_new_opa;
    lv_anim_init(&a_new_y);
    lv_anim_set_var(&a_new_y, main_label);
    lv_anim_set_exec_cb(&a_new_y, anim_y_cb);
    lv_anim_set_values(&a_new_y, 20, 0);             // 从 +20px 滚到 0 (原位)
    lv_anim_set_time(&a_new_y, 220);
    lv_anim_set_path_cb(&a_new_y, lv_anim_path_ease_out);
    lv_anim_start(&a_new_y);

    lv_anim_init(&a_new_opa);
    lv_anim_set_var(&a_new_opa, main_label);
    lv_anim_set_exec_cb(&a_new_opa, anim_opa_cb);
    lv_anim_set_values(&a_new_opa, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&a_new_opa, 220);
    lv_anim_start(&a_new_opa);
}

/* ===== 番茄卡片明暗：休息段整卡变淡 =====
 * 判定只看分段号 pomo_count_idx：0=休息段 → 变灰，1=工作段 → 还原。
 * 跟 pomo_status 无关，所以"休息中暂停"也是灰的，符合预期。
 *
 * 用对象级 opa 而不是改颜色：LVGL 的 lv_obj_get_style_opa_recursive() 会把它
 * 递归乘到背景/边框(lv_obj_draw.c:41)、文字(:196)、图片(:228)上，一次调用整卡生效，
 * 而且跟 pomo_set_highlight / pomo_set_field_focus 改颜色是正交的两个维度，不会打架。
 *
 * 高亮和焦点切换会重设颜色，虽然不影响 opa，但为了不给以后留坑，
 * 这几个函数末尾都再调一次，保证任何路径走完状态都是对的。
 */
static void pomo_apply_seg_opa(void)
{
    if (card_pomodoro == NULL) return;
    lv_obj_set_style_opa(card_pomodoro,
        (pomo_count_idx == 0) ? POMO_SEG_DIM_OPA : LV_OPA_COVER, 0);
}

/* ===== 时光工坊：卡片高亮（只在 UI_Show 检测到模式/光标变化时调用）===== */
/* 高亮第 idx 张卡（0倒计时 1番茄钟 2闹钟），旧高亮自动恢复普通态。
   pomo_hl 记住上一张，只刷新旧两张卡。OLED 反相：选中=黑底白字(屏上白块黑字) */
static void pomo_set_highlight(uint8_t idx)
{
    lv_obj_t *card[3] = {card_countdown, card_pomodoro, card_alarm};
    lv_obj_t *name[3] = {label_countdown_name, label_pomodoro_name, label_alarm_name};
    lv_obj_t *sub[3][3] = {{label_countdown_status, label_countdown_time, NULL},
                           {label_pomodoro_status,  label_pomodoro_dur,  NULL},
                           {label_alarm_hour,        label_alarm_min,     label_alarm_state}};
    lv_obj_t *icon[3] = {img_countdown_icon,   img_pomodoro_icon,   img_alarm_icon};

    if (pomo_hl >= 0) {   // 旧高亮卡恢复普通态：白底黑字细框，图标黑(OLED 白轮廓)
        lv_obj_set_style_bg_color(card[pomo_hl], lv_color_white(), 0);
        lv_obj_set_style_border_width(card[pomo_hl], 1, 0);
        lv_obj_set_style_text_color(name[pomo_hl], lv_color_black(), 0);
        lv_obj_set_style_text_color(sub[pomo_hl][0], lv_color_black(), 0);
        lv_obj_set_style_text_color(sub[pomo_hl][1], lv_color_black(), 0);
        if (sub[pomo_hl][2]) lv_obj_set_style_text_color(sub[pomo_hl][2], lv_color_black(), 0);
        if (pomo_hl == 2) lv_obj_set_style_text_color(label_alarm_colon, lv_color_black(), 0);
        lv_obj_set_style_img_recolor(icon[pomo_hl], lv_color_black(), 0);
        lv_obj_set_style_img_recolor_opa(icon[pomo_hl], LV_OPA_COVER, 0);
    }

    // 新卡高亮：黑底白字粗框，图标白(OLED 黑轮廓)
    lv_obj_set_style_bg_color(card[idx], lv_color_black(), 0);
    lv_obj_set_style_border_width(card[idx], 2, 0);
    lv_obj_set_style_text_color(name[idx], lv_color_white(), 0);
    lv_obj_set_style_text_color(sub[idx][0], lv_color_white(), 0);
    lv_obj_set_style_text_color(sub[idx][1], lv_color_white(), 0);
    if (sub[idx][2]) lv_obj_set_style_text_color(sub[idx][2], lv_color_white(), 0);
    if (idx == 2) lv_obj_set_style_text_color(label_alarm_colon, lv_color_white(), 0);
    lv_obj_set_style_img_recolor(icon[idx], lv_color_white(), 0);
    lv_obj_set_style_img_recolor_opa(icon[idx], LV_OPA_COVER, 0);

    pomo_hl = idx;
    pomo_apply_seg_opa();
}

/* 清除高亮（退出 SELECT 回 BROWSE/EDIT 时调一次） */
static void pomo_clear_highlight(void)
{
    if (pomo_hl < 0) return;
    lv_obj_t *card[3] = {card_countdown, card_pomodoro, card_alarm};
    lv_obj_t *name[3] = {label_countdown_name, label_pomodoro_name, label_alarm_name};
    lv_obj_t *sub[3][3] = {{label_countdown_status, label_countdown_time, NULL},
                           {label_pomodoro_status,  label_pomodoro_dur,  NULL},
                           {label_alarm_hour,        label_alarm_min,     label_alarm_state}};
    lv_obj_t *icon[3] = {img_countdown_icon,   img_pomodoro_icon,   img_alarm_icon};

    lv_obj_set_style_bg_color(card[pomo_hl], lv_color_white(), 0);
    lv_obj_set_style_border_width(card[pomo_hl], 1, 0);
    lv_obj_set_style_text_color(name[pomo_hl], lv_color_black(), 0);
    lv_obj_set_style_text_color(sub[pomo_hl][0], lv_color_black(), 0);
    lv_obj_set_style_text_color(sub[pomo_hl][1], lv_color_black(), 0);
    if (sub[pomo_hl][2]) lv_obj_set_style_text_color(sub[pomo_hl][2], lv_color_black(), 0);
    if (pomo_hl == 2) lv_obj_set_style_text_color(label_alarm_colon, lv_color_black(), 0);
    lv_obj_set_style_img_recolor(icon[pomo_hl], lv_color_black(), 0);
    lv_obj_set_style_img_recolor_opa(icon[pomo_hl], LV_OPA_COVER, 0);
    pomo_hl = -1;
    pomo_apply_seg_opa();
}

/* ===== 时光工坊：字段二次反相（EDIT 态选中的字段）===== */
/* 在已高亮（黑底白字）的卡片内，把当前编辑字段再反相成白底黑字小方块。
   focus_lbl 记住上一次反相的标签，切换时先恢复旧的再反相新的。 */
static lv_obj_t *pomo_focus_lbl = NULL;

/* 根据当前卡片(pomo_cursor)和字段索引，取出对应标签 */
static lv_obj_t *pomo_get_field_label(uint8_t cursor, uint8_t field)
{
    lv_obj_t *lbl = NULL;
    if (cursor == 0) {           // 倒计时：0=状态, 1=时长
        lbl = (field == 0) ? label_countdown_status : label_countdown_time;
    } else if (cursor == 1) {    // 番茄钟：0=状态, 1=时长
        lbl = (field == 0) ? label_pomodoro_status : label_pomodoro_dur;
    } else {                     // 闹钟：0=时, 1=分, 2=开关
        if (field == 0)      lbl = label_alarm_hour;
        else if (field == 1) lbl = label_alarm_min;
        else                 lbl = label_alarm_state;
    }
    return lbl;
}

static void pomo_clear_field_focus(void)
{
    if (pomo_focus_lbl == NULL) return;
    // 恢复成卡片高亮态：透明背景 + 白字
    lv_obj_set_style_bg_opa(pomo_focus_lbl, LV_OPA_TRANSP, 0);
    lv_obj_set_style_text_color(pomo_focus_lbl, lv_color_white(), 0);
    pomo_focus_lbl = NULL;
    pomo_apply_seg_opa();
}

static void pomo_set_field_focus(uint8_t cursor, uint8_t field)
{
    lv_obj_t *lbl = pomo_get_field_label(cursor, field);
    if (lbl == pomo_focus_lbl) return;       // 已经是它，不用重设

    pomo_clear_field_focus();                 // 先恢复旧标签

    // 二次反相：白底黑字（在黑卡片里就是白字小方块）
    lv_obj_set_style_bg_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(lbl, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(lbl, lv_color_black(), 0);
    pomo_focus_lbl = lbl;
    pomo_apply_seg_opa();
}

void UI_Show(void)
{	
  static uint8_t last_second = 0xFF;
  static uint32_t last_rtc_read = 0;
		static uint8_t last_page = 0xFF;
		static uint8_t last_sub_page = 0xFF;
	
		static Weather_State_t Prediction_Weather;
		char month[4] = {0};
		char day[4] = {0};
		uint8_t second_changed = 0;
    uint32_t tick = tick_now;   /* 主循环帧时间戳（main.c 每轮开头采样） */
		float precip_float;     // 转成浮点数 0.99
		int precip_num ; 
    // 2. 获取 RTC 时间
    if ((tick - last_rtc_read) >= 200) {
        last_rtc_read = tick;
        HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    }

    if (sTime.Seconds != last_second) {
        last_second = sTime.Seconds;
        second_changed = 1;
    }

    uint8_t date_changed = 0;
    static uint8_t last_day = 0xFF;
    if (sDate.Date != last_day) {
        last_day = sDate.Date;
        date_changed = 1;
    }
		//页面变化动画
    uint8_t cur_sub_page = 0;
    switch (current_page) {
        case PAGE_ENV:      cur_sub_page = (uint8_t)current_env_sub_page; break;
        case PAGE_TREND:    cur_sub_page = (uint8_t)current_trend_sub_page; break;
        case PAGE_POMODORO:
            cur_sub_page = 0;
            break;
        case PAGE_SYS:      cur_sub_page = (uint8_t)current_sys_sub_page; break;
        default:            cur_sub_page = 0; break;
    }
    uint8_t page_changed = (current_page != last_page);
    uint8_t sub_changed  = (cur_sub_page != last_sub_page);

    //主页时间滚动
    if (second_changed && current_page == PAGE_HOME && !page_changed) {
        char time_buf[16];
        snprintf(time_buf, sizeof(time_buf), "%02d:%02d", sTime.Hours, sTime.Minutes);
        lv_label_roll_up(label_home_time, time_buf);
    }
		
    //室内传感器数值实时刷新
    {
        static int16_t  last_temp = 0x7FFF;   // 0.1°C 刻度
        static uint8_t  last_hum  = 0xFF;     // %
        static uint16_t last_pres = 0xFFFF;   // hPa
        if (g_current_env.Pressure >= 800.0f) {   // 有有效气压读数才认为数据就绪
						Prediction_Weather=Predict_Weather(_3hours_press);//预测天气		
            int16_t  cur_temp = (int16_t)(g_current_env.Temperature * 10.0f + 0.5f);
            uint8_t  cur_hum  = (uint8_t)(g_current_env.Humidity + 0.5f);
            uint16_t cur_pres = (uint16_t)(g_current_env.Pressure + 0.5f);

            if (cur_temp != last_temp) {
                last_temp = cur_temp;
                lv_label_set_text_fmt(label_env_indoor_temp_val, "%d.%d°C", cur_temp / 10, cur_temp % 10);
            }
            if (cur_hum != last_hum) {
                last_hum = cur_hum;
                lv_label_set_text_fmt(label_env_hum_val, "%d%℃/%s℃", cur_hum,g_weather.humidity);
            }
            if (cur_pres != last_pres) {
                last_pres = cur_pres;
                lv_label_set_text_fmt(label_env_pressure_val, "%dhPa", cur_pres);
            }
        }
    }

    // 4a-1. 时光工坊：数据变量 → 6 个卡片标签。
    // EDIT 态按键改变量时 page/sub 都不变，门控不开，所以放门控外 + last 值对比
    if (current_page == PAGE_POMODORO) {
        static uint8_t     last_cd_status = 2;
        static uint8_t last_cd_remain = 0xFF;
        static uint8_t last_pomo_status = 2;
        static uint8_t last_pomo_cnt  = 0xFF;
        static uint8_t last_pomo_work = 0xFF;
        static uint8_t last_pomo_seg  = 0xFF;   // 分段：0=休息段 1=工作段（控制番茄图标明暗）
        static int8_t last_alarm_h   = 0xFF;
        static int8_t last_alarm_m   = 0xFF;
        static int8_t  last_alarm_on  = -1;
				//倒计时
				if(cd_status != last_cd_status){
		            lv_label_set_text(label_countdown_status, STATUS_TEXT_MAP[cd_status]);
								last_cd_status=cd_status;
				}
        if (cd_remain_min != last_cd_remain) {
            lv_label_set_text_fmt(label_countdown_time, "%dm", cd_remain_min);
            last_cd_remain = cd_remain_min;
        }
				// 番茄钟状态：0=已暂停 1=进行中
        if (pomo_status != last_pomo_status) {
            lv_label_set_text(label_pomodoro_status, POMO_STATUS_MAP[pomo_status]);
            last_pomo_status = pomo_status;
        }
        if (pomo_work_min != last_pomo_work) {
            lv_label_set_text_fmt(label_pomodoro_dur, "%dm", pomo_work_min);
            last_pomo_work = pomo_work_min;
        }
        // 番茄卡片分段明暗：休息段整卡变淡，工作段还原（详见 pomo_apply_seg_opa）
        if (pomo_count_idx != last_pomo_seg) {
            pomo_apply_seg_opa();
            last_pomo_seg = pomo_count_idx;
        }
        // 今日次数 → 顶部徽章（只读统计）
        if (pomo_today_cnt != last_pomo_cnt) {
            lv_label_set_text_fmt(label_pomo_badge, "今日专注%d次", pomo_today_cnt);
            last_pomo_cnt = pomo_today_cnt;
        }
				//闹钟
        if (alarm_hour != last_alarm_h) {
            lv_label_set_text_fmt(label_alarm_hour, "%02d", alarm_hour);
            last_alarm_h = alarm_hour;
        }
        if (alarm_min != last_alarm_m) {
            lv_label_set_text_fmt(label_alarm_min, "%02d", alarm_min);
            last_alarm_m = alarm_min;
        }
        if ((int8_t)alarm_on != last_alarm_on) {
            lv_label_set_text(label_alarm_state, alarm_on ? "开启" : "关闭");
            last_alarm_on = alarm_on;
        }
    }

    // 卡片高亮 + EDIT 字段二次反相
    {
			static PomoMode_t pomo_last_mode   = POMO_BROWSE;//上次什么模式
			static uint8_t    pomo_last_cursor = 1;//上次选中哪个功能
			static uint8_t    pomo_last_field   = 0;//上次选中哪个字段
        if (current_pomo_sub_page != pomo_last_mode || pomo_cursor != pomo_last_cursor
            || pomo_last_field != ((pomo_cursor==0)?cd_active_idx:(pomo_cursor==1?pomo_active_idx:alarm_active_idx))) {
            if (current_page == PAGE_POMODORO) {
                if (current_pomo_sub_page == POMO_BROWSE) {
                    pomo_clear_field_focus();
                    pomo_clear_highlight();
                } else if (current_pomo_sub_page == POMO_SELECT) {
                    pomo_clear_field_focus();   // 从 EDIT 退回时清掉字段反相
                    pomo_set_highlight(pomo_cursor);
                } else { /* POMO_EDIT：保持卡片高亮 + 字段二次反相 */
                    pomo_set_highlight(pomo_cursor);
                    uint8_t field = (pomo_cursor==0) ? cd_active_idx
                                  : (pomo_cursor==1) ? pomo_active_idx
                                  :                    alarm_active_idx;
                    pomo_set_field_focus(pomo_cursor, field);
                }
            }
            pomo_last_mode   = current_pomo_sub_page;
            pomo_last_cursor = pomo_cursor;
            pomo_last_field  = (pomo_cursor==0) ? cd_active_idx
                            : (pomo_cursor==1) ? pomo_active_idx
                            :                     alarm_active_idx;
        }
    }

    // 切出第四页时清掉字段焦点（防止白底反相残留）
    if (current_page != PAGE_POMODORO) pomo_clear_field_focus();

    // 4b. 数据/页面变化：刷新标签内容（不再每秒刷新，避免频繁分配导致堆碎片化）
    if (page_changed || sub_changed || ui_data_dirty || date_changed) {

        ui_data_dirty = 0;

        switch (current_page) {
					case PAGE_HOME: {
									//主页时钟
									char time_buf[16];
									char date_buf[16];
									char temp_buf[16];
									const char *week_str[] = {"", "星期一", "星期二", "星期三", "星期四", "星期五", "星期六", "星期日"};
									snprintf(time_buf, sizeof(time_buf), "%02d:%02d", sTime.Hours, sTime.Minutes); // HH:MM
									snprintf(date_buf, sizeof(date_buf), "%02d月%02d日", sDate.Month, sDate.Date);
									snprintf(temp_buf, sizeof(temp_buf), "%s°C", g_weather.low[0]);
									if (second_changed && !page_changed)
											lv_label_roll_up(label_home_time, time_buf);
									else 
											lv_label_set_text(label_home_time, time_buf);
											lv_label_set_text(label_home_temp, temp_buf);
									const char *zh_weather = weather_en_to_zh(g_weather.text_day[0]);						
									lv_label_set_text(label_home_weather, zh_weather); 
									lv_obj_update_layout(label_home_weather);
									lv_img_set_src(img_home_weather, get_weather_icon(zh_weather));
									lv_obj_align_to(img_home_weather, label_home_weather, LV_ALIGN_OUT_RIGHT_MID, 4, 0);
									lv_label_set_text(label_home_date, date_buf);
									lv_label_set_text(label_home_week, week_str[sDate.WeekDay]);
									break;
					}

					case PAGE_ENV: {
						switch(current_env_sub_page){
											case ENV_SUB_OVERVIEW:
													//室外天气API
													lv_label_set_text(label_env_city, g_weather.city);
													lv_label_set_text_fmt(label_env_humidity, "湿度:%s%%",g_weather.humidity);
													parse_date_split(g_weather.date[0],month,day);
													lv_label_set_text_fmt(label_env_date_today, "%s-%s 今天",month,day);
													lv_label_set_text_fmt(label_env_wind, "风力:%s%s级",
																					get_wind_direction_cn(g_weather.wind_direction[0]),g_weather.wind_scale[0]);
													parse_date_split(g_weather.date[1],month,day);
													lv_label_set_text_fmt(label_env_date_tomorrow,"%s-%s 明天",month,day);
													lv_img_set_src(img_env_weather_tomorrow, get_weather_icon(weather_en_to_zh(g_weather.text_day[1])));
													lv_label_set_text(label_env_weather_tomorrow,weather_en_to_zh(g_weather.text_day[1]));
													lv_label_set_text_fmt(label_env_temp_tomorrow, "%s~%s℃",
																								g_weather.low[1], g_weather.high[1]);
													parse_date_split(g_weather.date[2],month,day);
													lv_label_set_text_fmt(label_env_date_after_tomorrow,"%s-%s 后天",month,day);
													lv_img_set_src(img_env_weather_after_tomorrow, get_weather_icon(weather_en_to_zh(g_weather.text_day[2])));
													lv_label_set_text(label_env_weather_after_tomorrow,weather_en_to_zh(g_weather.text_day[2]));
													lv_label_set_text_fmt(label_env_temp_after_tomorrow, "%s~%s℃",
																								g_weather.low[2], g_weather.high[2]);
											break;
											case ENV_SUB_TODAY_DETAIL:
													//今天的天气详情
															parse_date_split(g_weather.date[0],month,day);
													   lv_label_set_text_fmt(label_env_sub1_title[0],"%s-%s 今天 ",month,day);
													   lv_label_set_text_fmt(label_env_sub1_head_temp[0],"%s~%s℃",g_weather.low[0],g_weather.high[0]);
														lv_label_set_text(label_env_sub1_badge[0], "1/4 >>");
															lv_label_set_text(label_env_sub1_day_title[0], "白天天气:");
														lv_label_set_text(label_env_sub1_day_weather[0],weather_en_to_zh(g_weather.text_day[0]));
															lv_img_set_src(img_env_sub1_day_icon[0],get_weather_icon(weather_en_to_zh(g_weather.text_day[0])));
															lv_label_set_text(label_env_sub1_night_title[0], "夜间天气:");
														lv_label_set_text(label_env_sub1_night_weather[0],weather_en_to_zh(g_weather.text_night[0]));
															lv_img_set_src(img_env_sub1_night_icon[0],get_weather_icon(weather_en_to_zh(g_weather.text_night[0])));
														lv_label_set_text(label_env_sub1_rain_title[0], "降水量:");
														lv_label_set_text_fmt(label_env_sub1_rain_val[0],"%smm",g_weather.rainfall[0]);
														precip_float = atof(g_weather.precip[0]);     // 转成浮点数 0.99
														precip_num = (int)(precip_float * 100.0f);         // 乘以100并强制转为整数 99
														if (precip_num < 0) precip_num = 0;
														if (precip_num > 100) precip_num = 100;														
													lv_label_set_text(label_env_sub1_pop_title[0], "降水概率:");
														lv_label_set_text_fmt(label_env_sub1_pop_val[0],"%d%%",precip_num);
														lv_bar_set_value(bar_env_sub1_pop[0], precip_num, LV_ANIM_ON);
												break;
										case ENV_SUB_TOMORROW_DETAIL:
											//明天的天气详情
												  		parse_date_split(g_weather.date[1],month,day);
													   lv_label_set_text_fmt(label_env_sub1_title[1],"%s-%s 明天 ",month,day);
													   lv_label_set_text_fmt(label_env_sub1_head_temp[1],"%s~%s℃",g_weather.low[1],g_weather.high[1]);
														lv_label_set_text(label_env_sub1_badge[1], "2/4 >>");
															lv_label_set_text(label_env_sub1_day_title[1], "白天天气:");
														lv_label_set_text(label_env_sub1_day_weather[1],weather_en_to_zh(g_weather.text_day[1]));
															lv_img_set_src(img_env_sub1_day_icon[1],get_weather_icon(weather_en_to_zh(g_weather.text_day[1])));
															lv_label_set_text(label_env_sub1_night_title[1], "夜间天气:");
														lv_label_set_text(label_env_sub1_night_weather[1],weather_en_to_zh(g_weather.text_night[1]));
															lv_img_set_src(img_env_sub1_night_icon[1],get_weather_icon(weather_en_to_zh(g_weather.text_night[1])));
														lv_label_set_text(label_env_sub1_rain_title[1], "降水量:");
														lv_label_set_text_fmt(label_env_sub1_rain_val[1],"%smm",g_weather.rainfall[1]);
														precip_float = atof(g_weather.precip[1]);    			 // 转成浮点数 0.99
														precip_num = (int)(precip_float * 100.0f);         // 乘以100并强制转为整数 99
														if (precip_num < 0) precip_num = 0;
														if (precip_num > 100) precip_num = 100;														
													lv_label_set_text(label_env_sub1_pop_title[1], "降水概率:");
														lv_label_set_text_fmt(label_env_sub1_pop_val[1],"%d%%",precip_num);
														lv_bar_set_value(bar_env_sub1_pop[1], precip_num, LV_ANIM_ON);
											break;
										case ENV_SUB_AFTER_TOM_DETAIL:
														//后天的天气详情
												  			parse_date_split(g_weather.date[2],month,day);
													   lv_label_set_text_fmt(label_env_sub1_title[2],"%s-%s 后天 ",month,day);
													   lv_label_set_text_fmt(label_env_sub1_head_temp[2],"%s~%s℃",g_weather.low[2],g_weather.high[2]);
														lv_label_set_text(label_env_sub1_badge[2], "3/4 >>");
															lv_label_set_text(label_env_sub1_day_title[2], "白天天气:");
														lv_label_set_text(label_env_sub1_day_weather[2],weather_en_to_zh(g_weather.text_day[2]));
															lv_img_set_src(img_env_sub1_day_icon[2],get_weather_icon(weather_en_to_zh(g_weather.text_day[2])));
															lv_label_set_text(label_env_sub1_night_title[2], "夜间天气:");
														lv_label_set_text(label_env_sub1_night_weather[2],weather_en_to_zh(g_weather.text_night[2]));
															lv_img_set_src(img_env_sub1_night_icon[2],get_weather_icon(weather_en_to_zh(g_weather.text_night[2])));
														lv_label_set_text(label_env_sub1_rain_title[2], "降水量:");
														lv_label_set_text_fmt(label_env_sub1_rain_val[2],"%smm",g_weather.rainfall[2]);
														precip_float = atof(g_weather.precip[2]);    
														precip_num = (int)(precip_float * 100.0f);         
														if (precip_num < 0) precip_num = 0;
														if (precip_num > 100) precip_num = 100;														
														lv_label_set_text(label_env_sub1_pop_title[2], "降水概率:");
														lv_label_set_text_fmt(label_env_sub1_pop_val[2],"%d%%",precip_num);
														lv_bar_set_value(bar_env_sub1_pop[2], precip_num, LV_ANIM_ON);
											break;
										case ENV_SUB_WIND_HUMIDITY:
											//3日风向与湿度对比
													lv_label_set_text(label_sub4_title, "3日风向与湿度对比");
													lv_label_set_text(label_env_sub4_badge, "4/4 >>");
													lv_label_set_text(label_env_sub4_today_title, "今天");
													lv_label_set_text_fmt(label_env_sub4_today_wind, "%s %s级(%skm/h)",
																											get_wind_direction_cn(g_weather.wind_direction[0]),g_weather.wind_scale[0],g_weather.wind_speed[0]);
													lv_label_set_text(label_env_sub4_today_hum_title, "湿");
													lv_label_set_text_fmt(label_env_sub4_today_hum_val, "%s%%",g_weather.humidity[0]);
													
													lv_label_set_text(label_env_sub4_tomorrow_title, "明天");
													lv_label_set_text_fmt(label_env_sub4_tomorrow_wind, "%s %s级(%skm/h)",
																											get_wind_direction_cn(g_weather.wind_direction[1]),g_weather.wind_scale[1],g_weather.wind_speed[1]);
													lv_label_set_text(label_env_sub4_tomorrow_hum_title, "湿");	
													lv_label_set_text_fmt(label_env_sub4_tomorrow_hum_val, "%s%%",g_weather.humidity[1]);
											break;
							break;
						}
					}
        case PAGE_TREND:
            switch (current_trend_sub_page) {	
            case INDOOR_SUB_OVERVIEW:
							//室内环境主页
								lv_img_set_src(img_env_3h_icon,get_weather_icon(weather_state_to_zh(Prediction_Weather)));
                lv_label_set_text(label_env_main_title, "环境监控 >");
								lv_label_set_text(label_env_3h_title, "本地 3h 预测");
								lv_label_set_text(label_env_indoor_temp_title, "室内温度:");
								lv_label_set_text(label_env_hum_title, "内/外湿度:");
								lv_label_set_text(label_env_pressure_title, "室内气压:");
								lv_label_set_text_fmt(label_env_3h_weather, "%s",weather_state_to_zh(Prediction_Weather));
								lv_label_set_text_fmt(label_env_3h_pressure_trend, "气压:%d->%d ",(uint16_t)(_3hours_press[0]+0.5f),(uint16_t)(_3hours_press[59]+0.5f));
                break;
            case INDOOR_SUB_TEMP:
							//温度折线图
                ui_trend_card_set_data(0, _3hours_temp);
                break;
            case INDOOR_SUB_HUMI:
							//湿度折线图
                ui_trend_card_set_data(1, _3hours_humi);
                break;
					  case INDOOR_SUB_PRESS:
							//气压折线图
							  ui_trend_card_set_data(2, _3hours_press);
							break;
						case INDOOR_SUB_FORECAST:
							//3h预测变化
							lv_label_set_text(label_fc3h_title, "< 3h预测变化数据");
							lv_label_set_text(label_fc3h_badge, "4/5");
							lv_img_set_src(img_fc3h_icon,get_weather_icon(weather_state_to_zh(Prediction_Weather)));
							lv_label_set_text_fmt(label_fc3h_weather, "%s",weather_state_to_zh(Prediction_Weather));
						lv_label_set_text_fmt(label_fc3h_val_pop,"%.0f%%",Calculate_Rain_Probability(_3hours_press));
						lv_label_set_text_fmt(label_fc3h_val_temp,"%+.1f℃",_3hours_temp[59]-_3hours_temp[0]);
						lv_label_set_text_fmt(label_fc3h_val_press,"%+.1fhPa",_3hours_press[59]-_3hours_press[0]);
                break;
						case INDOOR_SUB_EVAL:
					{
						//内外环境
						lv_label_set_text(label_env_title, "< 内外环境对比与衍生评估"); //[cite: 4]
						lv_label_set_text(label_env_badge, "5/5"); //[cite: 4]
						float temp_out = (atof(g_weather.high[0]) + atof(g_weather.low[0])) / 2.0f;
						float hum_out  = atof(g_weather.humidity[0]);
						Env_Brief_Result_t env_status = Evaluate_Brief_Environment(_3hours_temp[59], _3hours_humi[59], temp_out, hum_out);
						lv_label_set_text(label_env_status,env_status.condition); //[cite: 4]
						lv_label_set_text(label_env_advice,env_status.advice); //[cite: 4]
						lv_label_set_text(label_env_item_dew, "露点温度"); //[cite: 4]
						lv_label_set_text(label_env_item_outhum, "室外湿度"); //[cite: 4]
						lv_label_set_text(label_env_item_diffhum, "内外湿差"); //[cite: 4]
						// 室内露点：必须用同一点位的室内温湿度配对计算
						lv_label_set_text_fmt(label_env_val_dew, "%.1f℃", Calculate_Dew_Point(_3hours_temp[59], _3hours_humi[59])); //[cite: 4]
						// 室外湿度：天气 API 今日湿度
						lv_label_set_text_fmt(label_env_val_outhum, "%.0f%%", hum_out); //[cite: 4]
						// 内外湿差：室内-室外，正值=室内更潮，负值=室外更潮
						lv_label_set_text_fmt(label_env_val_diffhum, "%+.1f%%", _3hours_humi[59] - hum_out); //[cite: 4]
						// 按评估词条选择对应图标，默认舒适
						const lv_img_dsc_t *env_icon = &comfort;
						if (strcmp(env_status.condition, "室外偏热") == 0 ||
							strcmp(env_status.condition, "室内偏热") == 0) {
							env_icon = &hot;
						} else if (strcmp(env_status.condition, "室外偏冷") == 0 ||
								   strcmp(env_status.condition, "室内偏冷") == 0) {
							env_icon = &cold;
						} else if (strcmp(env_status.condition, "易结露") == 0 ||
								   strcmp(env_status.condition, "温差过大") == 0) {
							env_icon = &Warning;
						} else if (strcmp(env_status.condition, "室外偏潮") == 0 ||
								   strcmp(env_status.condition, "室内偏潮") == 0) {
							env_icon = &tide;
						} else if (strcmp(env_status.condition, "室内偏干") == 0 ||
								   strcmp(env_status.condition, "室外偏干") == 0) {
							env_icon = &dry;
						}
						lv_img_set_src(img_env_icon,env_icon);
						break;
					}
            }
            break;
        case PAGE_POMODORO:
            switch (current_pomo_sub_page) {
            case POMO_BROWSE:
                lv_label_set_text(label_pomo_header, "时光工坊 >");
				lv_img_set_src(img_countdown_icon,&countdown);
				lv_img_set_src(img_pomodoro_icon,&tomato);
				lv_img_set_src(img_alarm_icon,&clock);
                break;   // 6 个数值标签由门控外的变量同步块负责（编辑态也要实时更新）
									case POMO_SELECT:
                break;
									case POMO_EDIT:
                break;
            }
            break;

        case PAGE_SYS:
            switch (current_sys_sub_page) {
            case SYS_SUB_MAIN:
                sys_list_refresh();   // 列表内容 + 箭头位置（值以后从写死改成动态）
                break;
            case SYS_SUB_WIFI:
                /* 标题固定 "WiFi配网"，AP名/IP 提示在建屏时设置，无需每帧刷新 */
				wifiset_handle();
                break;
            }
            break;
        }
			if (page_changed || sub_changed) {
				if (last_page == 0xFF) {
					// 首次显示：直接加载，不播动画
					last_page = current_page;
					last_sub_page = cur_sub_page;
					lv_scr_load(ui_scr[current_page][cur_sub_page]);
				} else {
					lv_scr_load_anim_t anim_dir = LV_SCR_LOAD_ANIM_OVER_LEFT; // 默认向左滑入

					// 子页索引减小，或顶层页往回翻 → 向右滑
					if ((!page_changed && cur_sub_page < last_sub_page)
						|| (page_changed && current_page < last_page)) {
						anim_dir = LV_SCR_LOAD_ANIM_OVER_RIGHT;
					}

					last_page = current_page;
					last_sub_page = cur_sub_page;

					lv_scr_load_anim(
						ui_scr[current_page][cur_sub_page],
						anim_dir,
						250, 0, false);
				}
			}
    }
}