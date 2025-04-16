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

  *.数据缓冲区: sys_buf_lock 与 sys_buf_unlock 需成对使用.
  *.所有 charb(char_in_buffer) 使用完毕后需要调用 sys_buf_unlock 释放.如:
    charb* ptr = sys_uuid();
    if (sys_buf_valid(ptr)) {
      showlog("UUID: " + String(ptr->data));
    }
    sys_buf_unlock(ptr);
  *.根据需要调整 sys_buffer_max,避免缓冲不够 或 内存爆满.

  *.缓冲区超时检查:
    1.调用 sys_buf_lock 时 auto_unlock = true,会自动设置超时标记.
    2.超过 sys_buffer_timeout 会自动解除锁定.
  *.相关函数: sys_buf_timeout_lock, sys_buf_timeout_unlock需成对使用.
  *.使用原则:
    1.同函数调用 sys_buf_lock;
    2.跨函数调用 sys_buf_timeout_lock,避免申请和释放不及时导致的访问异常;
  *.超时缓冲区支持指针,如:
    chart* ptr = sys_buf_timeout_lock(10);
    if (sys_buf_timeout_valid(ptr)) {
      if (!ptr->val_ptr) {
        ptr->val_ptr = (void*)malloc(sizeof(uint16_t));
        *(uint16_t*)ptr->val_ptr = 1024;
        showlog(sys_buf_fill(String(*(uint16_t*)ptr->val_ptr).c_str()));
      }
    }

  *.当 Serial 作为硬件串口连接设备时,需打开 com_swap_pin,避免日志干扰通讯.
  *.当 step_run_setup 日志输出 Serial.print,step_run_loop 使用 Serial1.print.
  *.当 Serial1 用作通讯口时,需禁用 com_print_log 关闭日志.
  *.串口数据解析示例:
    uint8_t size = com_recv_buffer.size();
    if (size > 0) { //has data
      int16_t st = -1; //start
      int16_t ed = -1; //end
      int16_t idx = size - 1; //last distance

      char dt; //data
      while (idx >= 0) {
        //com_recv_buffer.peek(dt, idx);
        dt = com_recv_buffer[idx];
        if (dt == 0x03 && ed < 0) ed = idx; //帧尾

        if (dt == 0x02 && ed > 0) { //帧头,需先有帧尾
          st = idx;
          break;
        }

        idx--; //倒序查找最新帧
      }

      if (st >= 0 && st < ed) {
        for (idx = st + 1; idx < ed; idx++) { //read start-end
          //com_recv_buffer.peek(dt, idx);
          dt = com_recv_buffer[idx];
          Serial1.print(dt);
        }

        //clear buffer
        com_recv_buffer.clear();
      }
    }
  *.默认开启 com_recv_overwrite,在接收缓冲区写满后自动覆盖旧数据,这可能会导致丢
    数据;若关闭 com_recv_overwrite,需手动 clear 或 使用 pop 读数据以释放空间.
********************************************************************************/
#ifndef _esp_define__
#define _esp_define__

#ifdef ESP32
  #define sys_esp32
#endif

#ifdef  ESP8266
  #define sys_esp8266
#endif

//功能开关-----------------------------------------------------------------------
//调试模式
#define debug_enabled

//启用随机数
#define random_enabled

//LittleFS文件系统
#define lfs_enabled

//启用WiFi
#define wifi_enabled

//启用wifi-mesh
#define mesh_enabled

//启用mqtt
#define mqtt_enabled

//启用ntp
#define ntp_enabled

//启用ini
#define ini_enabled

//启用crc
#define crc_enabled

//启用md5
#define md5_enabled

//启用串口
#define com_enabled

//启用modbus
#define modbus_enabled

//运行监控
#define run_status

//运行时呼吸灯
#define run_blinkled

//启用缓冲区超时检查
#define buf_timeout_check

//启用自动降速
#define sys_auto_delay

#ifdef sys_esp32 //not support
  #undef com_enabled
#endif

//编译正式版本
#include "sys_define.h"

//global-------------------------------------------------------------------------
//分隔符
const char* split_tag = ";";

//空字符串
const char* str_null = "null";

//hex char
const char str_hex[] = "0123456789abcdef";

//设备名称
const char* dev_name = "esp_8266";

//设备标识(MAC)
const char* dev_id = "000000";

//默认管理密码
const char* dev_pwd = NULL;

//系统缓冲区: char-in-buffer
#define charb sys_buffer_item
struct sys_buffer_item {
  bool used;      //是否使用
  #ifdef buf_timeout_check
  uint32_t time;  //超时计时
  #endif

  byte data_type; //类型
  uint16_t size;  //缓冲大小
  uint16_t len;   //数据大小
  char* data;     //数据
  charb* next;    //next-item
};

//全局系统缓冲区
charb* sys_data_buffer = NULL;

//当前缓冲区item个数
byte sys_buffer_size = 0;
//当前被锁定item个数
byte sys_buffer_locked = 0;
//全局缓冲区item个数上限
byte sys_buffer_max = 120;

#ifdef buf_timeout_check
  //缓冲区item最长锁定时间
  uint32_t sys_buffer_timeout = 1000 * 60; //60s

  //带超时保护的缓冲区item
  #define chart sys_buffer_timeout_item
  struct sys_buffer_timeout_item {
    uint32_t time; //超时计时
    charb* buff;   //缓冲数据

    void* val_ptr;    //pointer
    bool val_bool;    //bool
    int32_t val_int;  //int
  };
#endif

//当前运行阶段
const byte step_run_setup = 7;
const byte step_run_loop = 19;
byte sys_run_step = step_run_setup;

#ifdef sys_esp32
  //全局互斥信号量
  SemaphoreHandle_t sys_sync_lock = NULL;
#endif

//系统数据结构: key-value
struct sys_data_kv {
  const char* key;
  const char* value;
};

//WiFi---------------------------------------------------------------------------
#ifdef wifi_enabled
  //base
  #ifdef sys_esp32
    #include <WiFi.h>
  #else
    #include <ESP8266WiFi.h>
  #endif

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

#if defined(wifi_enabled) && defined(mesh_enabled)
  #ifndef wifi_fs_autoconfig
    //wifi和mesh同时开启时,需要AsyncFsWebServer对象配合
    #define wifi_fs_autoconfig
  #endif
#endif

#if defined(wifi_fs_autoconfig) || defined(ini_enabled)
  #ifndef lfs_enabled
    //wifi_fs ini 需要文件系统支持
    #define lfs_enabled
  #endif
#endif

#ifdef lfs_enabled
  #include <LittleFS.h>

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
  //callback on init websocket
  AwsEventHandler wifi_on_websocket = nullptr;
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

//MESH---------------------------------------------------------------------------
#if defined(mesh_enabled) || defined(mqtt_enabled)
  //环形缓冲(FIFO)
  #include <RingBuf.h>
#endif

#ifdef mesh_enabled
  #include <painlessMesh.h>

  //mesh对象
  painlessMesh mesh;
  //任务调度器
  Scheduler task_scheduler;

  //mesh名称(dev_name去掉mesh前缀)
  const char* mesh_name = NULL;

  //服务端口
  const uint16_t mesh_port = 5555;

  //发送缓冲
  const byte mesh_data_buffer_size = 20;
  RingBuf<chart*, mesh_data_buffer_size> mesh_send_buffer;

  //回调事件
  painlessmesh::receivedCallback_t mesh_on_receive = nullptr;
  painlessmesh::newConnectionCallback_t mesh_on_newConn = nullptr;
  painlessmesh::changedConnectionsCallback_t mesh_on_connChanged = nullptr;
  painlessmesh::nodeTimeAdjustedCallback_t mesh_on_timeAdjust = nullptr;
  painlessmesh::nodeDelayCallback_t mesh_on_delayReceive = nullptr;
#endif

//MQTT---------------------------------------------------------------------------
#ifndef wifi_fs_autoconfig
  //need wifi&fs
  #undef mqtt_enabled
#endif

#ifdef mqtt_enabled
  #include <PubSubClient.h>

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
  #ifdef sys_esp32
    #include "mbedtls/md5.h"
  #endif

  #ifdef sys_esp8266
    #include <bearssl/bearssl.h>
  #endif
#endif

//串口通讯-----------------------------------------------------------------------
#ifdef com_enabled
  //屏蔽Serial上电打印
  #define com_swap_pin
  /*
    1.启动时esp会通过 Serial 打印日志
    2.若使用 Serial 作为通讯口,日志会影响设备通讯
    3.稳妥的方法是: 将引脚 swap,1->15(tx->d8),3->13(rx->d7)
    4.若启用 swap,则日志会自动通过 Serial1 打印
    5.若 Serial1 用于通讯,则禁用 com_print_log
  */

  #ifdef com_swap_pin
    //esp8266的 led 在 UART1 的 tx 的引脚上,呼吸灯会干扰通讯
    #undef run_blinkled

    //使用 Serial1 打印日志
    //#define com_print_log
  #endif

  //配置文件key
  #define com_tx "pin_tx"
  #define com_rx "pin_rx"
  #define com_config "config"
  #define com_baud_rate "baud_rate"

  //接收时自动覆盖旧数据
  #define com_recv_overwrite

  //接收缓冲,一般为协议包的2倍
  const byte com_recv_buf_size = 30;
  RingBuf<char, com_recv_buf_size> com_recv_buffer;
#endif

//随机数-------------------------------------------------------------------------
#ifdef random_enabled
  #ifdef sys_esp32

  #endif

  #ifdef sys_esp8266
    #include <ESP8266TrueRandom.h>
  #endif
#endif

//运行监控-----------------------------------------------------------------------
#ifdef  run_status
  //上次发送计时
  uint64_t run_status_lastsend = 0;

  //free head size
  uint32_t size_heap_last = 0;

  //刷新频率(秒)
  byte run_status_update = 30;
#endif

//呼吸灯-------------------------------------------------------------------------
#ifdef run_blinkled
  #ifdef sys_esp32
  #include <Adafruit_NeoPixel.h>

  //当前颜色索引
  uint8_t led_color_current = 0;

  //esp32-c3 mini使用1颗三色LED,在gpio8引脚
  Adafruit_NeoPixel led_strip = Adafruit_NeoPixel(1, 8);
  #endif

  #ifdef sys_esp8266
  //亮灯开始计时
  uint64_t led_bright_start = 0;

  //亮灯时长(毫秒)
  const uint16_t led_bright_len = 1200;

  //查表数组当前索引
  byte led_bright_tableIndex = 0;

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
  #endif

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
  //单次loop最小耗时(毫秒)
  byte sys_loop_interval = 50;
#endif

#if defined(sys_auto_delay) || defined(run_blinkled) || defined(run_status)
  //使用loop计时
  #define sys_loop_ticktime
#endif

#ifdef sys_loop_ticktime
  //本次loop开始计时
  uint64_t sys_loop_start = 0;
#endif

#endif
