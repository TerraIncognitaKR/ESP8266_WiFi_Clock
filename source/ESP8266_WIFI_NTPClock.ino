/**
  ******************************************************************************
  * @file           : ESP8266_WIFI_NTPClock.ino
  * @brief          : A simple clock application
  ******************************************************************************
  * @attention
  *
  *  2023, 2024 TeIn
  *  https://blog.naver.com/bieemiho92
  *
  *  Target Device : ESP8266 0.96" OLED Board
  *
  *  IDE :
  *     Arduino IDE 2.3.2
  *
  *  Dependancy    :
  *    ESP8266 board support package
  *       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  *    U8g2-2.34.22
  *    NTPClient-3.2.1
  *    AM2302-Sensor-1.3.2
  *    LiquidCrystal_I2C-1.1.2
  *
  *
  * @note
  *   Ver.02 (2024/04) :
  *     - Supports I2C Character LCD
  *     - Supports AM2302 Sensor
  *   Ver.01 (2023/10) :
  *     - Initial Release
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <Arduino.h>              // basics
#include <Wire.h>                 // I2C communication for OLED
#include <U8g2lib.h>              // U8g2 graphic lib
#include <ESP8266WiFi.h>          // ESP8266 Wi-Fi
#include <WiFiUdp.h>              //
#include <NTPClient.h>            // NTP Time
#include <time.h>                 // for getting timestamp from date

#include <AM2302-Sensor.h>        // AM2302 Sensor
#include <LiquidCrystal_I2C.h>    // I2C CLCD


/* Defines -------------------------------------------------------------------*/

/* Macros --------------------------------------------------------------------*/

/*** Debug Msg ***/
// #define DBG_LOG_EN_LOOP            1
// #define DBG_LOG_EN                 1
// #define DBG_DISP_ALL_FOUND_AP      1


/*** LCD Control ***/
#define LCD_X_POS_INIT                0
#define LCD_Y_POS_INIT                10
#define LCD_Y_OFFSET_STARTBLUE        16

#define LCD_Y_INC_u8g2_font_tiny5_tr  6   // u8g2_font_tiny5_tr : 5 x 5 small
#define LCD_X_OFFSET_CLOCK            14
#define LCD_X_OFFSET_DATE             82
#define LCD_Y_OFFSET_CLOCK            30
#define LCD_Y_OFFSET_PROGRESSBAR      52


/*** Font ***/
#define ICO_WIFI_NOCARRIER            57879  // u8g2_font_siji_t_6x10 wifi no signal
#define ICO_WIFI_WEAK                 57880  // u8g2_font_siji_t_6x10 wifi weak
#define ICO_WIFI_MID                  57881  // u8g2_font_siji_t_6x10 wifi mid
#define ICO_WIFI_FULL                 57882  // u8g2_font_siji_t_6x10 wifi full
#define ICO_CLOCK                     57365  // u8g2_font_siji_t_6x10 black round clock
#define ICO_MAGNIFIER                 57838  // u8g2_font_siji_t_6x10 magnifier
#define ICO_INFO                      57797  // u8g2_font_siji_t_6x10 [i]
#define ICO_BLANK                     127    // u8g2_font_siji_t_6x10


/*** Internal Macro ***/
#define INTERVAL_WIFI_CONNECTION_CHK  10      // seconds
#define INTERVAL_WIFI_RECONNECTION    59      // (num+1)*(INTERVAL_WIFI_CONNECTION_CHK) seconds  (set 10 min @ release)
#define INTERVAL_GET_TIME_FROM_NET    1800    // seconds // (set 30 min @ release)
#define INTERVAL_READ_AM2302          10      // 10 seconds

/**
 * @brief TIMER
 *  prescaler : TIM_DIV16
 *  count value         timer callback interval
 *        1               200 ns
 *      500               100 us
 *   250000                50 ms
 *   500000               100 ms
 *   600000               120 ms (valid using oscilloscope)
 */
#define ESP8266_TIMER1_CNT_VAL        500000
#define ESP8266_FLASH_KEY             0       // FLASH key is connected with IO0. when pressed, it is LOW.
#define ESP8266_LED_PIN               2       // LOW ON / HIGH OFF
#define ESP8266_LED_ON                LOW
#define ESP8266_LED_OFF               HIGH

#define KEYPRESS_SHORT_TH             1       // (num+1)*(timer callback interval)  (0.2 sec)
#define KEYPRESS_LONG_TH              14      // (num+1)*(timer callback interval)  (1.5 sec)

#define FW_VER                        2       // 240118
// #define FW_VER                     1       // 231002

/**
 * @brief I2C Character LCD
 * @note  tested on 20x4 CLCD
 *        modify THIS section & some codes to fit your device.
 */
#define CLCD_I2C_ADDR                 0x27
#define CLCD_COL_NUM                  20
#define CLCD_ROW_NUM                  4

/* Types ---------------------------------------------------------------------*/

/**
  * @brief      convert unix-time to human-readable time
  * @note       reference : https://blog.naver.com/chandong83/222273392541
  */
typedef uint32_t timestamp_t; //seconds

typedef struct {
	uint16_t    year;
	uint8_t     month;
	uint8_t     day;
	uint8_t     hour;
	uint8_t     minute;
	uint8_t     second;
	uint8_t     week;
	uint8_t     weekday;
} datetime_t;

/* Variables -----------------------------------------------------------------*/

/*** Time ***/
char        daysOfTheWeek[7][12]  = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
// reserved
// char currHour[4];
// char currMinute[4];
// char currSecond[4];
const char  *strTimeSvrURL        = "time2.kriss.re.kr";
int         dwLocalTimeZoneOffset      = 32400;          // GMT+9
String      strCurrDate;
String      strTimeSynced         = "CLOCK SYNCHRONIZED :)";
String      strWiFiScanOngoing    = "SCANNING...";

/*** hard-coded progress bar ***/
#if 1
char dispBar[60][63] =
{
  "[|                                                           ]",     //  0
  "[||                                                          ]",     //  1
  "[|||                                                         ]",     //  2
  "[||||                                                        ]",     //  3
  "[|||||                                                       ]",     //  4
  "[||||||                                                      ]",     //  5
  "[|||||||                                                     ]",     //  6
  "[||||||||                                                    ]",     //  7
  "[|||||||||                                                   ]",     //  8
  "[||||||||||                                                  ]",     //  9
  "[|||||||||||                                                 ]",     //  10
  "[||||||||||||                                                ]",     //  11
  "[|||||||||||||                                               ]",     //  12
  "[||||||||||||||                                              ]",     //  13
  "[|||||||||||||||                                             ]",     //  14
  "[||||||||||||||||                                            ]",     //  15
  "[|||||||||||||||||                                           ]",     //  16
  "[||||||||||||||||||                                          ]",     //  17
  "[|||||||||||||||||||                                         ]",     //  18
  "[||||||||||||||||||||                                        ]",     //  19
  "[|||||||||||||||||||||                                       ]",     //  20
  "[||||||||||||||||||||||                                      ]",     //  21
  "[|||||||||||||||||||||||                                     ]",     //  22
  "[||||||||||||||||||||||||                                    ]",     //  23
  "[|||||||||||||||||||||||||                                   ]",     //  24
  "[||||||||||||||||||||||||||                                  ]",     //  25
  "[|||||||||||||||||||||||||||                                 ]",     //  26
  "[||||||||||||||||||||||||||||                                ]",     //  27
  "[|||||||||||||||||||||||||||||                               ]",     //  28
  "[||||||||||||||||||||||||||||||                              ]",     //  29
  "[|||||||||||||||||||||||||||||||                             ]",     //  30
  "[||||||||||||||||||||||||||||||||                            ]",     //  31
  "[|||||||||||||||||||||||||||||||||                           ]",     //  32
  "[||||||||||||||||||||||||||||||||||                          ]",     //  33
  "[|||||||||||||||||||||||||||||||||||                         ]",     //  34
  "[||||||||||||||||||||||||||||||||||||                        ]",     //  35
  "[|||||||||||||||||||||||||||||||||||||                       ]",     //  36
  "[||||||||||||||||||||||||||||||||||||||                      ]",     //  37
  "[|||||||||||||||||||||||||||||||||||||||                     ]",     //  38
  "[||||||||||||||||||||||||||||||||||||||||                    ]",     //  39
  "[|||||||||||||||||||||||||||||||||||||||||                   ]",     //  40
  "[||||||||||||||||||||||||||||||||||||||||||                  ]",     //  41
  "[|||||||||||||||||||||||||||||||||||||||||||                 ]",     //  42
  "[||||||||||||||||||||||||||||||||||||||||||||                ]",     //  43
  "[|||||||||||||||||||||||||||||||||||||||||||||               ]",     //  44
  "[||||||||||||||||||||||||||||||||||||||||||||||              ]",     //  45
  "[|||||||||||||||||||||||||||||||||||||||||||||||             ]",     //  46
  "[||||||||||||||||||||||||||||||||||||||||||||||||            ]",     //  47
  "[|||||||||||||||||||||||||||||||||||||||||||||||||           ]",     //  48
  "[||||||||||||||||||||||||||||||||||||||||||||||||||          ]",     //  49
  "[|||||||||||||||||||||||||||||||||||||||||||||||||||         ]",     //  50
  "[||||||||||||||||||||||||||||||||||||||||||||||||||||        ]",     //  51
  "[|||||||||||||||||||||||||||||||||||||||||||||||||||||       ]",     //  52
  "[||||||||||||||||||||||||||||||||||||||||||||||||||||||      ]",     //  53
  "[|||||||||||||||||||||||||||||||||||||||||||||||||||||||     ]",     //  54
  "[||||||||||||||||||||||||||||||||||||||||||||||||||||||||    ]",     //  55
  "[|||||||||||||||||||||||||||||||||||||||||||||||||||||||||   ]",     //  56
  "[||||||||||||||||||||||||||||||||||||||||||||||||||||||||||  ]",     //  57
  "[||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| ]",     //  58
  "[||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||]"      //  59
};
#else
char dispBar[60][63] =
{
  " .                                                            ",     //  0
  " ..                                                           ",     //  1
  " ...                                                          ",     //  2
  " ....                                                         ",     //  3
  " .....                                                        ",     //  4
  " ......                                                       ",     //  5
  " .......                                                      ",     //  6
  " ........                                                     ",     //  7
  " .........                                                    ",     //  8
  " ..........                                                   ",     //  9
  " ...........                                                  ",     //  10
  " ............                                                 ",     //  11
  " .............                                                ",     //  12
  " ..............                                               ",     //  13
  " ...............                                              ",     //  14
  " ................                                             ",     //  15
  " .................                                            ",     //  16
  " ..................                                           ",     //  17
  " ...................                                          ",     //  18
  " ....................                                         ",     //  19
  " .....................                                        ",     //  20
  " ......................                                       ",     //  21
  " .......................                                      ",     //  22
  " ........................                                     ",     //  23
  " .........................                                    ",     //  24
  " ..........................                                   ",     //  25
  " ...........................                                  ",     //  26
  " ............................                                 ",     //  27
  " .............................                                ",     //  28
  " ..............................                               ",     //  29
  " ...............................                              ",     //  30
  " ................................                             ",     //  31
  " .................................                            ",     //  32
  " ..................................                           ",     //  33
  " ...................................                          ",     //  34
  " ....................................                         ",     //  35
  " .....................................                        ",     //  36
  " ......................................                       ",     //  37
  " .......................................                      ",     //  38
  " ........................................                     ",     //  39
  " .........................................                    ",     //  40
  " ..........................................                   ",     //  41
  " ...........................................                  ",     //  42
  " ............................................                 ",     //  43
  " .............................................                ",     //  44
  " ..............................................               ",     //  45
  " ...............................................              ",     //  46
  " ................................................             ",     //  47
  " .................................................            ",     //  48
  " ..................................................           ",     //  49
  " ...................................................          ",     //  50
  " ....................................................         ",     //  51
  " .....................................................        ",     //  52
  " ......................................................       ",     //  53
  " .......................................................      ",     //  54
  " ........................................................     ",     //  55
  " .........................................................    ",     //  56
  " ..........................................................   ",     //  57
  " ...........................................................  ",     //  58
  " ............................................................ "      //  59
};
#endif

/*** Wi-Fi Access Point information ***/
String      foundmyAPssid;
const char  *my_own_ap_ssid      = "input your ssid";
const char  *my_own_ap_password  = "input your password of my_own_ap_ssid";
String      disp_no_connection   = "OFFLINE";


/*** NTP Time ***/
WiFiUDP ntpUDP;


/*** Internal ***/
char              currFwVer[2];                // for string
volatile uint32_t uptime_WiFiconnection = 0;
volatile uint32_t uptime_WiFiLost       = 0;
volatile uint32_t uptime_LastTimeSynced = 0;
volatile uint32_t uptime_LastSensorRead = 0;  // AM2302
volatile uint8_t  bLEDState             = 0;  // reserved
volatile uint32_t dwFLASHKEYpressedtime = 0;

uint8_t           bProgressBarStatus    = 0;  // 0: NOT display   / else: display
uint32_t          g_lcd_yPos            = 10;
const char        *my_board_name        = "[Wi-Fi Clock]";
const char        *my_board_name2       = "::: Wi-Fi  Clock :::";

/**
 * @brief stores current state
 *        bit         description
 *          0           Wi-Fi connection state  - 1:connected 0:disconnected
 *          1           NTP Client              - 1:open      0:closed
 *          2           update time display req - 1:update    0:pending
 *          3           ...
 *
 *       (else)         (reserved)
 *
 */
volatile uint32_t g_state               = 0;

/*** bit position of g_state ***/
//   bit mask   : (1 << bit_pos)
//   set bit    : g_state |=  (1 << bit_pos)
// clear bit    : g_state &= ~(1 << bit_pos)

#define G_STATE_IS_SET(__BIT__)                 ( (1<<__BIT__) == (g_state & (1<<__BIT__)) )
#define G_STATE_SET_BIT(__BIT__)                (g_state |=  (1 << __BIT__))
#define G_STATE_CLR_BIT(__BIT__)                (g_state &= ~(1 << __BIT__))

#define G_STATE_BIT_POS_WIFI_CONN_STATE         0
#define G_STATE_BIT_POS_NTP_CLIENT_STATE        1   // ?
#define G_STATE_BIT_POS_TIME_SYNC_STATE         2   // ?
#define G_STATE_BIT_POS_CLOCK_DISP_REDRAW_REQ   16
#define G_STATE_BIT_POS_WIFI_RECONNECT_REQ      17
#define G_STATE_BIT_POS_WIFI_STATE_CHK_REQ      18
#define G_STATE_BIT_POS_TIME_RESYNC_REQ         19
#define G_STATE_BIT_POS_KEYPRESS_SHORT_REQ      20
#define G_STATE_BIT_POS_KEYPRESS_LONG_REQ       21
#define G_STATE_BIT_POS_AM2302_READ_REQ         22

/*** AM2302 Sensor ***/
volatile uint8_t  isSensorPresent = 0;

/*** I2C Character LCD ***/
volatile uint8_t  isCLCDPresent = 0;




/* Function prototypes -------------------------------------------------------*/

uint8_t   timestamp_to_weekday(timestamp_t timestamp_sec);
int       is_leap_year(uint16_t year);
void      utc_timestamp_to_date(timestamp_t timestamp, datetime_t* datetime);

unsigned long GetTodayBaseTimeStamp(datetime_t *datetime);
void      disp_ssid(uint8_t MODE);
void      update_disp_clock(uint8_t MODE);
void      update_disp_clock_CLCD(uint8_t MODE);
void      blinkInternalLED_Polling(uint32_t dwRepeatCount, uint32_t dwDelay_ms);
uint8_t   WLAN_Connect(uint8_t MODE, uint8_t LCD_DISP_EN);

void ICACHE_RAM_ATTR onTimerISR();


/* Constructor---------------------------------------------------------------*/

/**
 * @brief
 * U8g2 constructor --- rotation, pinNumber_SCL, pinNumber_SDA, pinNumber_RESET
 */
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, 14, 12, U8X8_PIN_NONE);

/**
 * @brief
 * You can specify the time server pool and the offset (in seconds).
 *  to change timezone offset, use offset_sec.
 *  example) when you are in South Korea(GMT+9), offset_sec is 32400.
 *
 * You can specify the update interval (in milliseconds).
 *
 *  NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);
 *                        udp / poolservername / offset_sec / updateinterval_ms
 */
NTPClient timeClient(ntpUDP, strTimeSvrURL, dwLocalTimeZoneOffset, 60000);

/**
 * @brief
 * AM2302 Temperature & Humnidity Sensor
 */
#define AM2302_SENSOR_PIN                         13  // D7 OK...
auto AM2302_Status = 0;                               // Sensor Readout
AM2302::AM2302_Sensor am2302{AM2302_SENSOR_PIN};

/**
 * @brief
 * Character LCD
 */
LiquidCrystal_I2C lcd(CLCD_I2C_ADDR, CLCD_ROW_NUM, CLCD_COL_NUM);


/* User code -----------------------------------------------------------------*/

/**
  * @brief      convert unix-time to human-readable time
  * @note       reference : https://blog.naver.com/chandong83/222273392541
  */
#if 0   // typedef
typedef uint32_t timestamp_t; //seconds

typedef struct {
	uint16_t    year;
	uint8_t     month;
	uint8_t     day;
	uint8_t     hour;
	uint8_t     minute;
	uint8_t     second;
	uint8_t     week;
	uint8_t     weekday;
} datetime_t;
#endif

// 1일을 초로
#define ONE_DAY                  (1*60*60*24)
// UTC 시작 시간
#define UTC_TIME_WEEKDAY_OFFSET (4) /* 1970,1,1은 목요일이기때문에 */

//날짜                    x, 1월, 2월 ..... 11월, 12월
uint8_t month_days[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

//타임 스탬프를 기준으로 요일 얻기
uint8_t timestamp_to_weekday(timestamp_t timestamp_sec)
{
	uint8_t result = (timestamp_sec / ONE_DAY + UTC_TIME_WEEKDAY_OFFSET) % 7;
	if (result == 0) {
		result = 7;
	}
	return result;
}

//윤달 확인
int is_leap_year(uint16_t year)
{
	if (year % 4 == 0 && ((year % 100) != 0) || ((year % 400) == 0)) {
		return true;
	}
	else
		return false;
}

//utc 타임 스탬프를 날짜로 변환
void utc_timestamp_to_date(timestamp_t timestamp, datetime_t* datetime)
{
	uint8_t  month;
	uint32_t days;
	uint16_t days_in_year;
	uint16_t year;
	timestamp_t second_in_day;

    // 시/분/초 계산
	second_in_day = timestamp % ONE_DAY;

	//초
	datetime->second = second_in_day % 60;

	//분
	second_in_day /= 60;
	datetime->minute = second_in_day % 60;

	//시
	second_in_day /= 60;
	datetime->hour = second_in_day % 24;


	//1970-1-1 0:0:0부터 현재까지 총 일수
	days = timestamp / ONE_DAY;

	//days를 계속 차감하면서 해당 년도 계산
	for (year = 1970; year <= 2200; year++) {
		if (is_leap_year(year))
			days_in_year = 366;
		else
			days_in_year = 365;

		if (days >= days_in_year)
			days -= days_in_year;
		else
			break;
	}

	//년
	datetime->year = year;

	//요일
	datetime->weekday = timestamp_to_weekday(timestamp);

	//해당 년도 1월 1일을 기준으로 지금까지의 주(week) 계산
	datetime->week = (days + 11 - datetime->weekday) / 7;

	//월 계산하기
	if (is_leap_year(datetime->year)) //윤달의 경우 2월이 29일이다.
		month_days[2] = 29;
	else
		month_days[2] = 28;

	//년도와 마찬가지로 일에서 계속 차감해서 찾는다.
	for (month = 1; month <= 12; month++) {
		if (days >= month_days[month])
			days -= month_days[month];
		else
			break;
	}
	datetime->month = month;
	datetime->day = days + 1;


  /****    _m_ create final string for display    ****/
  String strTmpYear, strTmpMonth, strTmpDate, strTmpWeekDay;

  //    year
  strTmpYear     = String(datetime->year);

  //    month
  if(datetime->month > 9 )
    strTmpMonth = String(datetime->month);
  else
  {
      strTmpMonth = String(0);
      strTmpMonth += String(datetime->month);
  }

  //    day
  if(datetime->day > 9 )
    strTmpDate = String(datetime->day);
  else
  {
      strTmpDate = String(0);
      strTmpDate += String(datetime->day);
  }

  //    weekday
  // strTmpWeekDay  = String ( daysOfTheWeek[datetime->weekday] );  // sunday ?
  // strTmpWeekDay  = String ( daysOfTheWeek[timeClient.getDay()] );

  //    concatenate all
  // strCurrDate = String(strTmpYear+"-"+strTmpMonth+"-"+strTmpDate+" "+strTmpWeekDay);
  strCurrDate = String(strTmpYear+"-"+strTmpMonth+"-"+strTmpDate);

}


/**
  * @brief      return timestamp of today 0:00:00
  * @param      datetime  datetime_t
  * @return     timestamp of today 0:00:00
  * @note       requires <time.h>
  */
unsigned long GetTodayBaseTimeStamp(datetime_t *datetime)
{
  struct tm t = {0};

  t.tm_year = datetime->year-1900;
  t.tm_mon = datetime->month-1;
  t.tm_mday = datetime->day;
  t.tm_hour = 0;
  t.tm_min = 0;
  t.tm_sec = 0;

  return mktime(&t);
}



/**
  * @brief      update displayed text of wifi ssid
  * @param      MODE    second line text
  *                     0 : disp_no_connection
  *                     1 : display connected ssid
  *                     2 : time sync state
  * @return     none
  * @note       note
  */
void disp_ssid(uint8_t MODE)
{
  uint32_t old_yPos = g_lcd_yPos;

  // clear top area
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 0, 128, 16);
  u8g2.setDrawColor(1);

  // icon (line1 & line2)
  g_lcd_yPos = LCD_Y_POS_INIT;
  u8g2.setFont(u8g2_font_siji_t_6x10);
  if(0 == MODE)
    u8g2.drawGlyph(2, (g_lcd_yPos+2), ICO_WIFI_NOCARRIER);
  else if(1 == MODE)
    u8g2.drawGlyph(2, (g_lcd_yPos+2), ICO_WIFI_FULL);
  else if(2 == MODE)
    u8g2.drawGlyph(2, (g_lcd_yPos+2), ICO_CLOCK);
  else if(3 == MODE)
    u8g2.drawGlyph(2, (g_lcd_yPos+2), ICO_MAGNIFIER);

  // text
  // line1
  u8g2.setFont(u8g2_font_tiny5_tr);
  u8g2.drawStr(16, g_lcd_yPos, my_board_name);
//   u8g2.drawStr(16, g_lcd_yPos, String(WiFi.localIP()+" ("+WiFi.macAddress()+")").c_str() );
  g_lcd_yPos = g_lcd_yPos + LCD_Y_INC_u8g2_font_tiny5_tr;

  // line2
  if(0 == MODE)         // offline
    u8g2.drawStr(16, g_lcd_yPos, (String(disp_no_connection+" ("+foundmyAPssid+")")).c_str());
  else if(1 == MODE)    // online
    u8g2.drawStr(16, g_lcd_yPos, foundmyAPssid.c_str());
  else if(2 == MODE)    // time syncing
    u8g2.drawStr(16, g_lcd_yPos, strTimeSynced.c_str());
  else if(3 == MODE)    // conncting
    u8g2.drawStr(16, g_lcd_yPos, strWiFiScanOngoing.c_str());

  g_lcd_yPos = g_lcd_yPos + LCD_Y_INC_u8g2_font_tiny5_tr;

  // display!
  u8g2.sendBuffer();

}



/**
  * @brief      update clock display
  * @param      MODE    (reserved for further use)
  * @return     none
  * @note       note
  */
void update_disp_clock(uint8_t MODE)
{
  uint32_t    old_yPos = g_lcd_yPos;
  unsigned long currentEpochTime = timeClient.getEpochTime();
  datetime_t  datetime;

  // returns epoch time. uncomment for debug purpose.
  //Serial.println(timeClient.getEpochTime());

  g_lcd_yPos = LCD_Y_OFFSET_STARTBLUE;

  // clear pervious time display zone (blue area)
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, g_lcd_yPos, 128, 128);
  u8g2.setDrawColor(1);

  // check bar is display (1)
  if(isSensorPresent)
  {
    g_lcd_yPos = 28;
  }
  else
  {
    (bProgressBarStatus) ? (g_lcd_yPos = LCD_Y_OFFSET_CLOCK) : (g_lcd_yPos = LCD_Y_OFFSET_CLOCK+8);
  }
  // update date
  utc_timestamp_to_date(currentEpochTime, &datetime);
  u8g2.setFont(u8g2_font_tiny5_tr);   // font for date
  u8g2.drawStr(LCD_X_OFFSET_DATE, g_lcd_yPos, strCurrDate.c_str() );                // YYYY-MM-DD
  g_lcd_yPos = g_lcd_yPos + LCD_Y_INC_u8g2_font_tiny5_tr;
  u8g2.drawStr(LCD_X_OFFSET_DATE, g_lcd_yPos, daysOfTheWeek[timeClient.getDay()]);  // (weekday)

  // update time
  u8g2.setFont(u8g2_font_12x6LED_mn); // font for clock digit
  u8g2.drawStr(LCD_X_OFFSET_CLOCK, g_lcd_yPos, timeClient.getFormattedTime().c_str() );

  // check bar is display (2)
  if(bProgressBarStatus)
  {
    // update bar
    u8g2.setFont(u8g2_font_tiny5_tr);   // font for date
    g_lcd_yPos = LCD_Y_OFFSET_PROGRESSBAR;

    //  day - update every 1440 seconds from today 0:00
    u8g2.drawStr(0, g_lcd_yPos, dispBar[( (currentEpochTime - GetTodayBaseTimeStamp(&datetime))/1440 )] );
    g_lcd_yPos = g_lcd_yPos + LCD_Y_INC_u8g2_font_tiny5_tr;
    //  minute
    u8g2.drawStr(0, g_lcd_yPos, dispBar[( (currentEpochTime % 3600)/60 )] );
    g_lcd_yPos = g_lcd_yPos + LCD_Y_INC_u8g2_font_tiny5_tr;
    //  second
    u8g2.drawStr(0, g_lcd_yPos, dispBar[( currentEpochTime % 60 )] );
  }
  else
  {
    if(isSensorPresent)
    {
      g_lcd_yPos = LCD_Y_OFFSET_PROGRESSBAR;

      u8g2.setFont(u8g2_font_spleen5x8_me);   // font for text
      u8g2.drawStr(LCD_X_OFFSET_CLOCK, (g_lcd_yPos-3), "Temp('C) ");

      u8g2.setFont(u8g2_font_9x6LED_mn);      // font for value
      String tmpString = String(am2302.get_Temperature());  // returns previous readout value
      u8g2.drawStr(LCD_X_OFFSET_CLOCK+55, g_lcd_yPos, tmpString.c_str());
      g_lcd_yPos = g_lcd_yPos + 10;

      u8g2.setFont(u8g2_font_spleen5x8_me);   // font for text
      u8g2.drawStr(LCD_X_OFFSET_CLOCK, (g_lcd_yPos-3), "Humid(%) ");
      u8g2.setFont(u8g2_font_9x6LED_mn);      // font for value
      tmpString = String(am2302.get_Humidity());            // returns previous readout value
      u8g2.drawStr(LCD_X_OFFSET_CLOCK+55, g_lcd_yPos, tmpString.c_str());
    }
  }
  // display
  u8g2.sendBuffer();

  g_lcd_yPos = old_yPos;

  // blink LED on every minutes
  (0 == datetime.second) ? (digitalWrite(ESP8266_LED_PIN, ESP8266_LED_ON)) : (digitalWrite(ESP8266_LED_PIN, ESP8266_LED_OFF));

  return;

}




/**
  * @brief      update clock display (to CLCD)
  * @param      MODE    (reserved for further use)
  * @return     none
  * @note       display as...
  *                 20x4                        16x2
  *                 "::: Wi-Fi  Clock :::"      "    hh:mm:ss    "
  *                 "                    "      " YYYY-MM-DD MMM "
  *                 "      hh:mm:ss      "
  *                 "   YYYY-MM-DD MMM   "
  */
void update_disp_clock_CLCD(uint8_t MODE)
{
  if(!isCLCDPresent)
    return;

  if((CLCD_ROW_NUM > 2) && (CLCD_COL_NUM > 16))     // 20x4
  {
    lcd.setCursor(6,2);
    lcd.print(timeClient.getFormattedTime().c_str());
    lcd.setCursor(3,3);
    lcd.print(strCurrDate.c_str());
    lcd.setCursor(14,3);
    lcd.print(daysOfTheWeek[timeClient.getDay()]);
  }
  else                                              // 16x2
  {
    lcd.setCursor(5,0);
    lcd.print(timeClient.getFormattedTime().c_str());
    lcd.setCursor(1,1);
    lcd.print(strCurrDate.c_str());
    lcd.setCursor(13,1);
    lcd.print(daysOfTheWeek[timeClient.getDay()]);
  }

}



/**
  * @brief      blink LED of ESP8266
  * @param      dwRepeatCount   blink count. 0 means 1.
  * @param      dwDelay_ms      millisecond delay time of LED ON/OFF
  * @return     none
  * @note       note
  */
void      blinkInternalLED_Polling(uint32_t dwRepeatCount, uint32_t dwDelay_ms)
{

#ifdef DBG_LOG_EN
  Serial.printf("LED %d %d\r\n", dwRepeatCount, dwDelay_ms);
#endif

  for(uint32_t i=0; i<dwRepeatCount; i++)
  {
  digitalWrite(ESP8266_LED_PIN, ESP8266_LED_ON);
  delay(dwDelay_ms);
  digitalWrite(ESP8266_LED_PIN, ESP8266_LED_OFF);
  delay(dwDelay_ms);

#ifdef DBG_LOG_EN
    Serial.printf("%d\r\n", i);
#endif
  }

  digitalWrite(ESP8266_LED_PIN, ESP8266_LED_OFF); // LED off after blank

  return;

}



/**
  * @brief      Search my Wi-Fi Access Point & Connect
  * @param      MODE
  *               0       : search and connect
  *               (else)  : connect directly by hard-coded information
  * @param      LCD_DISP_EN
  *               0       : NOT display connection procedure on LCD
  *               (else)  : display connection procedure on LCD
  * @return     uint8_t
  *               0       : connection not established.
  *               (else)  : connection established.
  * @note       to direct connect to AP, use " WiFi.begin(ssid, password) "
  */
uint8_t   WLAN_Connect(uint8_t MODE, uint8_t LCD_DISP_EN)
{
  uint8_t bRet = 0;

  String  ssid;
  int32_t rssi;
  uint8_t encryptionType;
  uint8_t *bssid;
  int32_t channel;
  bool    hidden;
  int     scanResult;

  bool    isfindmyAP    = false;


  if(LCD_DISP_EN)
  {
    // update display
    // clear display
    u8g2.clearDisplay();
    // clear buffer
    u8g2.clearBuffer();
    // icon (line1 & line2)
    g_lcd_yPos = LCD_Y_POS_INIT;
    u8g2.setFont(u8g2_font_siji_t_6x10);
    u8g2.drawGlyph(2, (g_lcd_yPos+2), ICO_WIFI_NOCARRIER);
    // text
    u8g2.setFont(u8g2_font_tiny5_tr);
    // line1
    u8g2.drawStr(16, g_lcd_yPos, my_board_name);
    g_lcd_yPos = g_lcd_yPos + LCD_Y_INC_u8g2_font_tiny5_tr;
    // line2
    // u8g2.drawStr(16, g_lcd_yPos, "NO CARRIER");
    g_lcd_yPos += LCD_Y_INC_u8g2_font_tiny5_tr;
    // line3
    u8g2.drawStr(2, g_lcd_yPos, "Searching Wi-Fi...");
    g_lcd_yPos += LCD_Y_INC_u8g2_font_tiny5_tr;
    // display!
    u8g2.sendBuffer();
  }

  Serial.printf(">> [%s] Searching Wi-Fi... ", __FUNCTION__);

  scanResult = WiFi.scanNetworks(/*async=*/false, /*hidden=*/true);

  if (scanResult == 0)
  {
    Serial.println(F("No networks found"));
  }
  else if (scanResult > 0)
  {
    Serial.printf(PSTR("total %03d networks found :\n"), scanResult);

    // Print unsorted scan results
    // and find my Wi-Fi Access Point ("TERRA-****")
    isfindmyAP = false;

    for (int8_t i = 0; i < scanResult; i++)
    {
      WiFi.getNetworkInfo(i, ssid, encryptionType, rssi, bssid, channel, hidden);

      // get extra info
      const bss_info *bssInfo = WiFi.getScanInfoByIndex(i);
      String phyMode;
      const char *wps = "";

      if (bssInfo)
      {
        phyMode.reserve(12);
        phyMode = F("802.11");
        String slash;
        if (bssInfo->phy_11b)
        {
          phyMode += 'b';
          slash = '/';
        }
        if (bssInfo->phy_11g)
        {
          phyMode += slash + 'g';
          slash = '/';
        }
        if (bssInfo->phy_11n)
        {
          phyMode += slash + 'n';
        }
        if (bssInfo->wps)
        {
          wps = PSTR("WPS");
        }
      }
#ifdef DBG_DISP_ALL_FOUND_AP
      Serial.printf(PSTR("  %02d: [CH %03d] [%02X:%02X:%02X:%02X:%02X:%02X] %ddBm %c %c %-11s %3S %s\n"), \
                              i, channel, bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5], rssi, (encryptionType == ENC_TYPE_NONE) ? ' ' : '*', hidden ? 'H' : 'V', phyMode.c_str(), wps, ssid.c_str());
#endif /** DBG_DISP_ALL_FOUND_AP **/

      // check it is my Wi-Fi Acceses Point
      if( (false == isfindmyAP) && (0 == ssid.indexOf("TERRA-")) )
      {
        foundmyAPssid = ssid;
        isfindmyAP = true;
      }
      yield();
    }
  }
  else
  {
    Serial.printf(PSTR("WiFi scan error %d"), scanResult);
  }

  /////////////////////////////////////////////////////////////////////////////

  if(isfindmyAP)  // found my AP. proceed to connect
  {
    Serial.printf(">> [%s] Found! connecting to '%s'...\r\n", __FUNCTION__, foundmyAPssid.c_str());

    // connect to my AP
    if(MODE)    // connect directly by hard-coded information. previous scan routines are useless :)
      WiFi.begin(my_own_ap_ssid, my_own_ap_password);
    else        // connect to found AP
      WiFi.begin(foundmyAPssid, my_own_ap_password);

    if(LCD_DISP_EN)
    {
      // update display
      //line4
      u8g2.drawStr(2, g_lcd_yPos, "Connect to :");
      g_lcd_yPos += LCD_Y_INC_u8g2_font_tiny5_tr;
      u8g2.drawStr(2, g_lcd_yPos, foundmyAPssid.c_str());
      g_lcd_yPos += LCD_Y_INC_u8g2_font_tiny5_tr;
      // icon (line1 & line2)
      g_lcd_yPos = LCD_Y_POS_INIT;
      u8g2.setFont(u8g2_font_siji_t_6x10);
      u8g2.drawGlyph(2, (g_lcd_yPos+2), ICO_BLANK);      // clear previous icon
      u8g2.drawGlyph(2, (g_lcd_yPos+2), ICO_WIFI_FULL);

      // display!
      u8g2.sendBuffer();
    }

    // before exit, update state flag
    G_STATE_SET_BIT(G_STATE_BIT_POS_WIFI_CONN_STATE);

    return 1;
  }
  else            // can't found my AP.
  {
    Serial.printf(">> [%s] Not Found!\r\n", __FUNCTION__);

    if(LCD_DISP_EN)
    {
      // update display
      //line4
      u8g2.drawStr(2, g_lcd_yPos, "Error! Not Found my AP!");
      g_lcd_yPos += LCD_Y_INC_u8g2_font_tiny5_tr;
      u8g2.drawStr(2, g_lcd_yPos, foundmyAPssid.c_str());
      g_lcd_yPos += LCD_Y_INC_u8g2_font_tiny5_tr;
      // display!
      u8g2.sendBuffer();
    }

    // before exit, update state flag
    G_STATE_CLR_BIT(G_STATE_BIT_POS_WIFI_CONN_STATE);

    return 0;
  }

  // unreachable.
  G_STATE_CLR_BIT(G_STATE_BIT_POS_WIFI_CONN_STATE);
  return 0;
}



/*****************************************************************************/

/**
  * @brief      ESP8266 Timer1 ISR
  * @param      none
  * @return     none
  * @note       in this routine, just set flag and exit. next action is performed in loop().
  */
void ICACHE_RAM_ATTR onTimerISR()
{

  // update clock display
  G_STATE_SET_BIT(G_STATE_BIT_POS_CLOCK_DISP_REDRAW_REQ);

  // check Wi-Fi connection (every INTERVAL_WIFI_CONNECTION_CHK sec)
  if( (timeClient.getEpochTime() - uptime_WiFiconnection) > INTERVAL_WIFI_CONNECTION_CHK)
  {
    uptime_WiFiconnection = timeClient.getEpochTime();

    if(!(G_STATE_IS_SET(G_STATE_BIT_POS_WIFI_CONN_STATE)))
      uptime_WiFiLost++;

    G_STATE_SET_BIT(G_STATE_BIT_POS_WIFI_STATE_CHK_REQ);
  }

  // check Wi-Fi Reconnection request (every INTERVAL_WIFI_RECONNECTION sec)
  if( uptime_WiFiLost > INTERVAL_WIFI_RECONNECTION)
  {
    uptime_WiFiLost = 0;
    G_STATE_SET_BIT(G_STATE_BIT_POS_WIFI_RECONNECT_REQ);
  }

  // check time re-synchronize period is elapsed (every INTERVAL_GET_TIME_FROM_NET sec)
  if( (timeClient.getEpochTime() - uptime_LastTimeSynced) > INTERVAL_GET_TIME_FROM_NET)
  {
    // clear time sync state
    G_STATE_CLR_BIT(G_STATE_BIT_POS_TIME_SYNC_STATE);
    uptime_LastTimeSynced = timeClient.getEpochTime();

    // set flag to do update
    G_STATE_SET_BIT(G_STATE_BIT_POS_TIME_RESYNC_REQ);
  }

  // AM2302 Sensor Read
  if(isSensorPresent)
  {
    if( (timeClient.getEpochTime() - uptime_LastSensorRead) > INTERVAL_READ_AM2302)
    {
      uptime_LastSensorRead = timeClient.getEpochTime();

      // set flag to do update
      G_STATE_SET_BIT(G_STATE_BIT_POS_AM2302_READ_REQ);
    }
  }

  // check key pressed
  if( 0 == digitalRead(ESP8266_FLASH_KEY) )
  {
    // key is pressed. increase value for long press.
    dwFLASHKEYpressedtime += 1;
  }
  else
  {
    // key is released. set flag.
    if (dwFLASHKEYpressedtime >= KEYPRESS_LONG_TH)
    {
      G_STATE_SET_BIT(G_STATE_BIT_POS_KEYPRESS_LONG_REQ);
      G_STATE_CLR_BIT(G_STATE_BIT_POS_KEYPRESS_SHORT_REQ);
    }
    else if ( (KEYPRESS_SHORT_TH <= dwFLASHKEYpressedtime) && (dwFLASHKEYpressedtime < KEYPRESS_LONG_TH) )
    {
      G_STATE_SET_BIT(G_STATE_BIT_POS_KEYPRESS_SHORT_REQ);
      G_STATE_CLR_BIT(G_STATE_BIT_POS_KEYPRESS_LONG_REQ);
    }
    else
    {
      G_STATE_CLR_BIT(G_STATE_BIT_POS_KEYPRESS_SHORT_REQ);
      G_STATE_CLR_BIT(G_STATE_BIT_POS_KEYPRESS_LONG_REQ);
    }

    dwFLASHKEYpressedtime = 0;
  }

  // keypress test
  // digitalWrite(ESP8266_LED_PIN, digitalRead(ESP8266_FLASH_KEY));

  // reset counter for next time...
  timer1_write(ESP8266_TIMER1_CNT_VAL);
}



/**
  * @brief      arduino setup()
  * @param      none
  * @return     none
  * @note       put your setup code here, to run once
  */
void setup()
{
  /**
   *    Initialize hardware & libraries (1)
   */
  // Built-in LED
  pinMode(ESP8266_LED_PIN, OUTPUT);
  digitalWrite(ESP8266_LED_PIN, HIGH);

  // FLASH key
  pinMode(ESP8266_FLASH_KEY, INPUT_PULLUP);

  // Serial Monitor
  Serial.begin(115200);

  // LCD Graphic Library
  u8g2.begin();

  // ...
  delay(2500);
  blinkInternalLED_Polling(5, 100);

  // 1st splash - OLED
  // clear display
  u8g2.clearDisplay();
  // clear buffer
  u8g2.clearBuffer();
  // icon (line1 & line2)
  g_lcd_yPos = LCD_Y_POS_INIT;
  u8g2.setFont(u8g2_font_siji_t_6x10);
  u8g2.drawGlyph(2, (g_lcd_yPos+2), ICO_INFO);
  // text
  u8g2.setFont(u8g2_font_tiny5_tr);
  // line1
  u8g2.drawStr(16, g_lcd_yPos, my_board_name);
  g_lcd_yPos = g_lcd_yPos + LCD_Y_INC_u8g2_font_tiny5_tr;
  // line2
  u8g2.drawStr(16, g_lcd_yPos, "STARTING...");
  g_lcd_yPos = g_lcd_yPos + LCD_Y_INC_u8g2_font_tiny5_tr;
  // line3
  u8g2.drawStr(5,g_lcd_yPos, "FW Ver : ");
  itoa(FW_VER, currFwVer, 10);                // FW V02 : Fix radix to decimal
  u8g2.drawStr(35,g_lcd_yPos, currFwVer);
  u8g2.drawStr(48,g_lcd_yPos, __DATE__);
  u8g2.drawStr(92,g_lcd_yPos, __TIME__);
  g_lcd_yPos += LCD_Y_INC_u8g2_font_tiny5_tr;
  // line4
  g_lcd_yPos += LCD_Y_INC_u8g2_font_tiny5_tr;
  // line5
  u8g2.drawStr(0,g_lcd_yPos, "  made by Terra Incognita");
  g_lcd_yPos += LCD_Y_INC_u8g2_font_tiny5_tr;
  // line6
  u8g2.drawStr(0,g_lcd_yPos, "  https://blog.naver.com/bieemiho92");
  g_lcd_yPos += LCD_Y_INC_u8g2_font_tiny5_tr;
  //line7
  g_lcd_yPos += LCD_Y_INC_u8g2_font_tiny5_tr;
  // line8
  u8g2.drawStr(0,g_lcd_yPos, "  Device MAC Address : ");
  g_lcd_yPos += 10;
  // line9 - display own MAC address
  u8g2.setFont(u8g2_font_NokiaSmallBold_tr);
  u8g2.drawStr(10, g_lcd_yPos, WiFi.macAddress().c_str());

  // display!
  u8g2.sendBuffer();

  // 1st splash - serial teminal
  Serial.printf("\r\n\r\n\r\n");
  Serial.println("*******************************************************************************");
  Serial.printf("\t%s", my_board_name);
  Serial.println();
  Serial.printf("\tFW VER x%02X\t(Build @  %s %s)\r\n", FW_VER, __DATE__, __TIME__);
  Serial.printf("\tMAC Addr :\t");
  Serial.println(WiFi.macAddress());
  Serial.println();
  Serial.println("*******************************************************************************");

  delay(1000);

  /**
   *    Initialize hardware & libraries (2)
   */

  // I2C Character LCD --- check whether LCD is present
  Wire.begin();
  delay(100);
  if( (Wire.requestFrom(CLCD_I2C_ADDR, 1)))
  {
    Serial.println(">> CLCD is present.");
    isCLCDPresent = 1;
  }
  else
  {
    Serial.println(">> CLCD is NOT present.");
    isCLCDPresent = 0;
  }
  // Wire.end();         // disable I2C --- esp8266 doesn't support this.

  if(isCLCDPresent)
  {
    lcd.init();         // calls Wire.begin() internally.
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.printstr(my_board_name2);
    lcd.setCursor(1,1);
    lcd.print(WiFi.macAddress().c_str());
  }

  // AM2302 : Start
  if(am2302.begin())
  {
    Serial.println(">> AM2302 Sensor is present.");
    isSensorPresent = 1;
  }
  else
  {
    Serial.println(">> AM2302 Sensor is NOT present.");
    isSensorPresent = 0;
  }

  // Wi-Fi : station(client) mode, disconnect previous connection
  Serial.print(">> Init Wi-Fi...");
  WiFi.mode(WIFI_STA);  WiFi.disconnect();  delay(1000);
  Serial.println("Done!");

  Serial.println(">> Connect to Wi-Fi...");

  if(isCLCDPresent)
  {
    (CLCD_ROW_NUM > 2) ? (lcd.setCursor(0,2)) : (lcd.setCursor(0,0));
    lcd.print("Connect Network.. ");
  }

  while( !(WLAN_Connect(0, 1)) )   // return 1 when establish connect.
  {
    delay(1000);                // retry every 1 sec.
  }

  /**
   *    Refresh Display because connected Wi-Fi.
   */
  Serial.printf(">> Connected to %s\r\n", foundmyAPssid);

  if(isCLCDPresent)
  {
    lcd.print("OK");
  }

  // clear display
  u8g2.clearDisplay();
  // clear buffer
  u8g2.clearBuffer();
  // icon (line1 & line2)
  g_lcd_yPos = LCD_Y_POS_INIT;
  u8g2.setFont(u8g2_font_siji_t_6x10);
  u8g2.drawGlyph(2, (g_lcd_yPos+2), ICO_WIFI_FULL);
  // text
  u8g2.setFont(u8g2_font_tiny5_tr);
  // line1
  u8g2.drawStr(16, g_lcd_yPos, my_board_name);
  g_lcd_yPos = g_lcd_yPos + LCD_Y_INC_u8g2_font_tiny5_tr;
  // line2
  u8g2.drawStr(16, g_lcd_yPos, foundmyAPssid.c_str());
  g_lcd_yPos = g_lcd_yPos + LCD_Y_INC_u8g2_font_tiny5_tr;
  // display!
  u8g2.sendBuffer();

  /**
   *    Starting NTPClient...
   */
  timeClient.begin();
  Serial.printf(">> Update Time from %s ... ", strTimeSvrURL);

  if(isCLCDPresent)
  {
    (CLCD_ROW_NUM > 2) ? (lcd.setCursor(0,3)) : (lcd.setCursor(0,1));
    lcd.print("Update Time..     ");
  }

  // line3
  u8g2.drawStr(2,g_lcd_yPos, "Update Time from : ");
  g_lcd_yPos += LCD_Y_INC_u8g2_font_tiny5_tr;
  // line4
  u8g2.drawStr(10,g_lcd_yPos, strTimeSvrURL);
  g_lcd_yPos += LCD_Y_INC_u8g2_font_tiny5_tr;
  // line5
  u8g2.drawStr(2,g_lcd_yPos, "Please wait a few seconds...");
  g_lcd_yPos += LCD_Y_INC_u8g2_font_tiny5_tr;
  u8g2.sendBuffer();

  while(1)
  {
    if(true == timeClient.update())                     // check connection
    {
      G_STATE_SET_BIT(G_STATE_BIT_POS_TIME_SYNC_STATE); // update state
      break;
    }
    delay(1000);
  }

  if(isCLCDPresent)
  {
    lcd.print("OK");
  }

  Serial.println("Done!");

  // Display Own IP Address to console
  Serial.printf(">> Own IP : ");
  Serial.println(WiFi.localIP());

  // line6
  u8g2.drawStr(2,g_lcd_yPos, "Done!");
  g_lcd_yPos += LCD_Y_INC_u8g2_font_tiny5_tr;
  u8g2.drawStr(2,g_lcd_yPos, "Change to Clock Mode...");
  g_lcd_yPos += LCD_Y_INC_u8g2_font_tiny5_tr;
  // line7
  if(isSensorPresent)
  {
    u8g2.drawStr(2,g_lcd_yPos, "AM2302 Sensor is ON-LINE");
  }
  else
  {
    u8g2.drawStr(2,g_lcd_yPos, "AM2302 Sensor is OFFLINE");
  }
  g_lcd_yPos += LCD_Y_INC_u8g2_font_tiny5_tr;
  u8g2.sendBuffer();

  /**
   *    I2C Character LCD
   */
  if(isCLCDPresent)
  {
    u8g2.drawStr(2,g_lcd_yPos, "I2C CLCD is attached");
    u8g2.sendBuffer();
    lcd.clear();
  }

  delay(1500);


  /**
   *    before entering loop(),
   *    redraw all screen & set uptime
   */

  // set uptime
  uptime_WiFiconnection = timeClient.getEpochTime();
  uptime_LastTimeSynced = uptime_WiFiconnection;

  // clear display
  u8g2.clearDisplay();
  // clear buffer
  u8g2.clearBuffer();
  // icon (line1 & line2)
  g_lcd_yPos = LCD_Y_POS_INIT;
  u8g2.setFont(u8g2_font_siji_t_6x10);
  u8g2.drawGlyph(2, (g_lcd_yPos+2), ICO_WIFI_FULL);
  // text
  u8g2.setFont(u8g2_font_tiny5_tr);
  // line1
  u8g2.drawStr(16, g_lcd_yPos, my_board_name);
  g_lcd_yPos = g_lcd_yPos + LCD_Y_INC_u8g2_font_tiny5_tr;
  // line2
  u8g2.drawStr(16, g_lcd_yPos, foundmyAPssid.c_str());
  g_lcd_yPos = g_lcd_yPos + LCD_Y_INC_u8g2_font_tiny5_tr;
  // line3
  g_lcd_yPos = g_lcd_yPos + LCD_Y_INC_u8g2_font_tiny5_tr;
  // line4
  g_lcd_yPos = g_lcd_yPos + LCD_Y_INC_u8g2_font_tiny5_tr;
  // line5
  g_lcd_yPos = g_lcd_yPos + LCD_Y_INC_u8g2_font_tiny5_tr;
  // display!
  u8g2.sendBuffer();

  // for clock font
  u8g2.setFont(u8g2_font_12x6LED_mn); // perfect monospace font

  if(isCLCDPresent)
  {
    lcd.setCursor(0,0);
    lcd.printstr(my_board_name2);
  }

  // start timer
  timer1_attachInterrupt(onTimerISR);
  timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
  timer1_write(ESP8266_TIMER1_CNT_VAL);

  // refresh time from NTP server before entering loop()
  timeClient.update();

}



/**
  * @brief      arduino loop()
  * @param      none
  * @return     none
  * @note       put your main code here, to run repeatedly:
  */
void loop()
{
  // short keypress check
  if( G_STATE_IS_SET(G_STATE_BIT_POS_KEYPRESS_SHORT_REQ) )
  {
#ifdef DBG_LOG_EN_LOOP
    Serial.println(">>> key_s");
#endif
    // add short key press handler

    // toggle bar display
    (bProgressBarStatus) ? (bProgressBarStatus = 0) : (bProgressBarStatus = 1);

    G_STATE_CLR_BIT(G_STATE_BIT_POS_KEYPRESS_SHORT_REQ);

  }

  // long  keypress check
  if( G_STATE_IS_SET(G_STATE_BIT_POS_KEYPRESS_LONG_REQ) )
  {
#ifdef DBG_LOG_EN_LOOP
    Serial.println(">>> key_l");
#endif
    // add long  key press handler (force clock sync)

    // force Sync Time / Wi-Fi Connection
    if( G_STATE_IS_SET(G_STATE_BIT_POS_WIFI_CONN_STATE) )
    {
      // network is up. just do sync time from server.
      G_STATE_SET_BIT(G_STATE_BIT_POS_TIME_RESYNC_REQ);
    }
    else
    {
      // network is down. re-scan and connect new Wi-Fi.
      G_STATE_SET_BIT(G_STATE_BIT_POS_WIFI_RECONNECT_REQ);
    }

    G_STATE_CLR_BIT(G_STATE_BIT_POS_KEYPRESS_LONG_REQ);

  }

  // update clock display
  if( G_STATE_IS_SET(G_STATE_BIT_POS_CLOCK_DISP_REDRAW_REQ) )
  {
    update_disp_clock(0);
    update_disp_clock_CLCD(0);

    G_STATE_CLR_BIT(G_STATE_BIT_POS_CLOCK_DISP_REDRAW_REQ);

  }

  // check Wi-Fi connection
  if( G_STATE_IS_SET(G_STATE_BIT_POS_WIFI_STATE_CHK_REQ) )
  {
    uint8_t bTmpWiFiStatus = WiFi.status();

#ifdef DBG_LOG_EN_LOOP
    Serial.print(">>> net is ");
#endif

    G_STATE_CLR_BIT(G_STATE_BIT_POS_WIFI_STATE_CHK_REQ);

    if(WL_CONNECTED != bTmpWiFiStatus)
    {
#ifdef DBG_LOG_EN_LOOP
        Serial.printf("down (%d)\r\n", bTmpWiFiStatus);
#endif
        G_STATE_CLR_BIT(G_STATE_BIT_POS_WIFI_CONN_STATE);
    }
    else
    {
        // when AP is OFF -> ON, ESP8266 has reconnect automatically.
#ifdef DBG_LOG_EN_LOOP
        Serial.printf("up (%d)\r\n", bTmpWiFiStatus);
#endif
        G_STATE_SET_BIT(G_STATE_BIT_POS_WIFI_CONN_STATE);
    }

    disp_ssid( G_STATE_IS_SET(G_STATE_BIT_POS_WIFI_CONN_STATE) );

  }

  // re-scan and connect new Wi-Fi
  if( G_STATE_IS_SET(G_STATE_BIT_POS_WIFI_RECONNECT_REQ) )
  {
#ifdef DBG_LOG_EN_LOOP
    Serial.println(">>> scanning Wi-Fi...");
#endif
    disp_ssid(3);

    if(!(G_STATE_IS_SET(G_STATE_BIT_POS_WIFI_CONN_STATE)))
    {
      WLAN_Connect(0,0);
    }

    G_STATE_CLR_BIT(G_STATE_BIT_POS_WIFI_RECONNECT_REQ);

  }

  // do sync time
  if( G_STATE_IS_SET(G_STATE_BIT_POS_TIME_RESYNC_REQ) )
  {
#ifdef DBG_LOG_EN_LOOP
    Serial.print(">>> sync time...");
#endif

    if( G_STATE_IS_SET(G_STATE_BIT_POS_WIFI_CONN_STATE) )
    {

      if(true == timeClient.update() )  // done / fail
      {
        uptime_LastTimeSynced = timeClient.getEpochTime();
        G_STATE_SET_BIT(G_STATE_BIT_POS_TIME_SYNC_STATE);
#ifdef DBG_LOG_EN_LOOP
        Serial.println("done!");
#endif
        disp_ssid(2);
      }
      else
      {
#ifdef DBG_LOG_EN_LOOP
        Serial.println("failed!");
        // decrease INTERVAL_GET_TIME_FROM_NET?
#endif
      }
    }
    else
    {
#ifdef DBG_LOG_EN_LOOP
      Serial.println("net is down");
#endif
    }

    G_STATE_CLR_BIT(G_STATE_BIT_POS_TIME_RESYNC_REQ);

  }

  if( G_STATE_IS_SET(G_STATE_BIT_POS_AM2302_READ_REQ) )
  {
    AM2302_Status = am2302.read();
#ifdef DBG_LOG_EN_LOOP
    Serial.printf(">> AM2302 Readout => 0x%02X\r\n", AM2302_Status);
#endif

    G_STATE_CLR_BIT(G_STATE_BIT_POS_AM2302_READ_REQ);
  }
}
