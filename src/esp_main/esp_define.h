/********************************************************************************
  作者: dmzn@163.com 2025-02-12
  描述: 编译条件,用于定义编译的功能模块

  备注:
  *.WiFi单选开关:
    1.wifi_fs_autoconfig: 使用AsyncFsWebServer库,自动配置WiFi和文件系统.
    2.wifi_autoconfig: 使用WiFiManager库,自动配置WiFi.
    3.wifi_manualconfig: 手动配置WiFi.

  *.启用mqtt_enabled时,需wifi_fs_autoconfig提供文件系统存取mqtt参数.
  *.MQTT默认主题:
    1.esp/cmd: 向esp发送指令.
    2.esp/log: esp发送日志或数据.
  *.mqtt_server: ip=x.x.x.x;port=1883;user=sa;pwd=123;wmsg=offline 按需填写.
  *.mqtt_topic: topic=x/x;qos=1

  *.启用ntp_enabled时,需wifi_fs_autoconfig提供文件系统存取ntp参数.
  *.ntp_server: ntp=x.x.x.x;zone=8;update=30 按需填写

  *.数据缓冲区: sys_buf_lock 与 sys_buf_unlock 需成对出现.
  *.所有 charb(char_in_buffer) 使用完毕后需要调用 sys_buf_unlock 释放.如:
    charb* ptr = sys_uuid();
    if (sys_buf_valid(ptr)) {
      showlog("UUID: " + String(ptr->data));
    }
    sys_buf_unlock(ptr);
  *.根据需要调整 sys_buffer_max,避免缓冲不够 或 内存爆满.

  *.缓冲区自动释放原理:
    1.开始 loop,设置 sys_buffer_stamp 为不等于 0 的数
    2.该 loop 内调用 sys_buf_lock 的数据都带有与 sys_buffer_stamp 相同标记
    3.新 loop 开始, sys_buffer_stamp++, 缓冲区内带有上一 loop 的数据都会自动释放
  *.依据原理: 只建议在局部变量使用自动释放,且只适用于基于 loop 的单任务模式.
  *.缓冲区支持带类型的指针,如:
    charb* ptr = sys_buf_lock(10, true, 1); //type=1
    if (sys_buf_valid(ptr)) {
      if (!ptr->val_ptr) {
        ptr->val_ptr = (void*)malloc(sizeof(uint16_t));
        *(uint16*)ptr->val_ptr = 1024;
        showlog(sys_buf_fill(String(*(uint16*)ptr->val_ptr).c_str()));
      } else {
        showlog(sys_buf_fill(String(*(uint16*)ptr->val_ptr).c_str()));
      }
    }
********************************************************************************/
#ifndef _esp_define__
#define _esp_define__

//功能开关-----------------------------------------------------------------------
//调试模式
#define debug_enabled

//启用随机数
#define random_enabled

//启用WiFi
#define wifi_enabled

//启用mqtt
#define mqtt_enabled

//启用ntp
#define ntp_enabled

//启用ini
#define ini_enabled

//启用crc
//#define crc_enabled

//启用md5
//#define md5_enabled

//启用串口
#define com_enabled

//启用modbus
#define modbus_enabled

//运行监控
#define run_status

//运行时呼吸灯
#define run_blinkled

//启用缓冲自动释放
#define buf_auto_unlock

//启用自动降速
#define sys_auto_delay

//global-------------------------------------------------------------------------
//分隔符
const char* split_tag = ";";

//空字符串
const char* str_null = "null";

//设备名称
const char* dev_name = "esp_8266";

//设备标识(MAC)
const char* dev_id = "000000";

//系统缓冲区: char-in-buffer
#define charb sys_buffer_item
struct sys_buffer_item {
  byte data_type; //类型
  uint16_t len;   //长度
  char* data;     //数据

  void* val_ptr;  //pointer
  bool val_bool;  //bool
  bool used;      //是否使用

  #ifdef buf_auto_unlock
  uint16_t stamp; //释放标记
  #endif
  charb* next;     //next-item
};

//全局系统缓冲区
charb* sys_data_buffer = NULL;

//当前缓冲区item个数
byte sys_buffer_size = 0;
//当前被锁定item个数
byte sys_buffer_locked = 0;
//全局缓冲区item个数上限
byte sys_buffer_max = 120;

#ifdef buf_auto_unlock
//当前有效的缓冲标记
uint16_t sys_buffer_stamp = 1;
#endif

//WiFi---------------------------------------------------------------------------
#ifdef wifi_enabled
  //base
  #include <ESP8266WiFi.h>

  //自动配置WiFi和文件系统
  //库: AsyncFsWebServer
  #define wifi_fs_autoconfig

  //自动配置WiFi
  //库: WiFiManager
  #define wifi_autoconfig

  //手动配置WiFi
  #define wifi_manualconfig

  //WiFi是否连接
  bool wifi_isok = false;

  //初始化step定义
  const byte step_init_begin = 0;
  const byte step_init_getoption = 1;
  const byte step_init_setoption = 2;

  //callback on init wifi
  typedef void (*cb_wifi_serverInit)(byte step);
  cb_wifi_serverInit wifi_on_serverInit = NULL;
#endif

#if defined(wifi_fs_autoconfig) || defined(ini_enabled)
  #include <LittleFS.h>
  #define lfs_enabled

  //文件系统(启动)正常
  bool lfs_isok = false;
#endif

#ifdef wifi_fs_autoconfig
  //单选
  #undef wifi_autoconfig
  #undef wifi_manualconfig
  #include <AsyncFsWebServer.h>

  //server for wifi config
  AsyncFsWebServer wifi_fs_server(80, LittleFS, dev_name);
#endif

#ifdef wifi_autoconfig
  //单选
  #undef wifi_manualconfig
  #include <WiFiManager.h>
#endif

#ifdef wifi_manualconfig
  //wifi param
  const char* wifi_ssid = "ssid";
  const char* wifi_pwd = "pwd";
#endif

//MQTT---------------------------------------------------------------------------
#ifndef wifi_fs_autoconfig
  //need wifi&fs
  #undef mqtt_enabled
#endif

#ifdef mqtt_enabled
  #include <PubSubClient.h>
  #include <RingBuf.h>

  //向mqtt打印日志
  #define mqtt_showlog

  //mqtt上线通知
  #define mqtt_online

  //发送缓冲
  const byte mqtt_data_buffer_size = 10;
  RingBuf<charb*, mqtt_data_buffer_size> mqtt_send_buffer;

  //mqtt param
  const char* mqtt_server_ip = NULL; //broker
  const char* mqtt_server_user = NULL; //user
  const char* mqtt_server_pwd = NULL; //pwd
  const char* mqtt_server_willmsg = NULL;//下线消息
  const char* mqtt_client_id = NULL; //id
  const char* mqtt_topic_cmd = NULL; //主题:接收指令
  const char* mqtt_topic_log = NULL; //主题:发送日志

  byte mqtt_topic_cmd_qos = 0;
  uint16_t mqtt_server_port = 1883;

  #define label_mqtt_server "MQTT服务器"
  #define label_mqtt_clientID "MQTT客户标识"
  #define label_mqtt_topic_cmd "MQTT接收主题"
  #define label_mqtt_topic_log "MQTT发送主题"

  WiFiClient mqtt_wifi_client;
  PubSubClient mqtt_client(mqtt_wifi_client);

  //callback on mqtt received data
  typedef void (*cb_mqtt_on_data)(char*, uint8_t*, unsigned int);
  cb_mqtt_on_data mqtt_on_data = NULL;

  //callback for send mqtt data
  typedef void (*cb_mqtt_send)(const char* data, bool retained);
  cb_mqtt_send mqtt_do_send = NULL;
#endif

//NTP----------------------------------------------------------------------------
#ifndef wifi_fs_autoconfig
  //need wifi&fs
  #undef ntp_enabled
#endif

#ifdef ntp_enabled
  #include <NTPClient.h>

  const char* ntp_server = NULL;
  #define label_ntp_server "NTP服务器"

  uint8_t ntp_timezone = 8; //时区
  uint8_t ntp_interval = 60;//更新评率(分钟)

  WiFiUDP ntp_wifi_client;
  NTPClient ntp_client(ntp_wifi_client);
#endif

//INI----------------------------------------------------------------------------
#ifdef ini_enabled
  //配置文件
  const char* ini_filename = "/config/config.csv";
#endif

#ifdef crc_enabled
  #include <CRC.h>
#endif

#ifdef md5_enabled
  #include <bearssl/bearssl.h>
#endif

//随机数-------------------------------------------------------------------------
#ifdef random_enabled
  #include <ESP8266TrueRandom.h>
#endif

//运行监控-----------------------------------------------------------------------
#ifdef  run_status
  //上次发送计时
  uint64_t run_status_lastsend = 0;

  //free head size
  uint32_t size_heap_last = 0;
  uint32_t size_heap_now = 0;

  //刷新频率(秒)
  byte run_status_update = 30;
#endif

//呼吸灯-------------------------------------------------------------------------
#ifdef run_blinkled
  //亮灯开始计时
  uint64_t led_bright_start = 0;

  //查表数组当前索引
  byte led_bright_tableIndex = 0;

  //亮灯时长(毫秒)
  const uint16_t led_bright_len = 1200;

  //查表数组大小，影响平滑度
  const byte led_bright_tableSize = 100;

  //呼吸值表
  const uint16_t led_bright_table[] = {621, 646, 671, 696, 721, 745, 769, 792, 814, 836,
    857, 877, 896, 914, 930, 946, 960, 973, 984, 994, 1003, 1010, 1015, 1019, 1022, 1023,
    1022, 1019, 1015, 1010, 1003, 994, 984, 973, 960, 946, 930, 914, 896, 877, 857,
    836, 814, 792, 769, 745, 721, 696, 671, 646, 621, 596, 571, 546, 521, 497, 473,
    450, 428, 406, 385, 365, 346, 328, 312, 296, 282, 269, 258, 248, 239, 232, 227,
    223, 220, 220, 220, 223, 227, 232, 239, 248, 258, 269, 282, 296, 312, 328, 346,
    365, 385, 406, 428, 450, 473, 497, 521, 546, 571, 596
  };

  /* 呼吸表算法:
import math

table_size = 100 #查表数组大小
pwm_max = 1023   #最大值
min_value = 220  #最小值

breathe_table = [
  int((math.sin(i * 2 * math.pi / table_size) + 1) * ((pwm_max - min_value) / 2) + min_value)
  for i in range(table_size)
]

# 输出C语言数组格式
print("const uint16_t led_bright_table[] = {")
print(", ".join(map(str, breathe_table)))
print("};")

  */
#endif

//自动降速---------------------------------------------------------------------------
#ifdef sys_auto_delay
  //本次loop开始计时
  uint64_t sys_loop_start = 0;

  //单次loop最小耗时(毫秒)
  byte sys_loop_interval = 50;
#endif

#endif
