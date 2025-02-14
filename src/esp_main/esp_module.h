#include "c_types.h"
/********************************************************************************
  作者: dmzn@163.com 2025-02-12
  描述: 常用函数定义- 头文件定义
********************************************************************************/
#ifndef _esp_module__
#define _esp_module__
  //callback on init wifi
  typedef void(*cb_wifi_serverInit)(void *server);
  
  //打印日志
  void showlog(const String& event);

  //配置WiFi
  bool wifi_config_by_web(cb_wifi_serverInit doInit = NULL);
#endif