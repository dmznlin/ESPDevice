/********************************************************************************
  作者: dmzn@163.com 2025-02-12
  描述: 编译条件,用于定义编译的功能模块
********************************************************************************/
#ifndef _esp_define__
#define _esp_define__
  //设备名称
  const char* dev_name = "esp_8266";
  
  //启用WiFi
  #define wifi_enabled
  #ifdef wifi_enabled
    //自动配置WiFi和文件系统
    //库: AsyncFsWebServer
    #define wifi_fs_autoconfig

    //自动配置WiFi
    //库: WiFiManager
    //#define wifi_autoconfig 

    //手动配置WiFi
    //#define wifi_manualconfig
  #endif

  #ifdef wifi_manualconfig
    //wifi param
    const char* wifi_ssid = "ssid";
    const char* wifi_pwd = "pwd";
  #endif
#endif