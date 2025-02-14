/********************************************************************************
  作者: dmzn@163.com 2025-02-12
  描述: 常用函数定义
********************************************************************************/
#include <Arduino.h>
#include "esp_define.h"
#include "esp_module.h"

/*
  date: 2025-02-12 15:43:20
  parm: 日志内容
  desc: 提供用户配置wifi的入口
*/
void showlog(const String& event){
  Serial.println(event);
} 

#ifdef wifi_enabled
  #include <ESP8266WiFi.h>
#endif

#ifdef wifi_autoconfig
  #include <WiFiManager.h>
#endif

#ifdef wifi_fs_autoconfig
  #include <LittleFS.h>
  #include <AsyncFsWebServer.h>

  //server for wifi config
  AsyncFsWebServer wifi_fs_server(80, LittleFS, dev_name);

  // 检查文件系统
  bool startFilesystem(){
    if (LittleFS.begin()){
        return true;
    }
    else {
      showlog("format and restart...");
      LittleFS.format();
      ESP.restart();
    }

    return false;
  }
#endif

/*
  date: 2025-02-12 15:43:20
  parm: 初始化server回调
  desc: 提供用户配置wifi的入口
*/
bool wifi_config_by_web(cb_wifi_serverInit doInit){
  #ifdef wifi_manualconfig //手动
    //设置为无线终端模式
    WiFi.mode(WIFI_STA);

    if (doInit != NULL) {
      doInit(NULL);
    }

    //连接wifi
    WiFi.begin(wifi_ssid, wifi_pwd);    
    Serial.println("");
    byte i = 0;

    while (WiFi.status() != WL_CONNECTED) {          
      Serial.print(".");
      delay(500);
      i++;

      if (i > 20){
        break;
      }
    }
  #endif

  #ifdef wifi_autoconfig //自动
    WiFiManager wifiManager;
    wifiManager.autoConnect(dev_name);
  #endif

  #ifdef wifi_fs_autoconfig //自动 + FS
    if (!startFilesystem()){
      showlog("LittleFS error!");
      return false;
    }
     
    //启动热点
    IPAddress local = wifi_fs_server.startWiFi(15000, dev_name, "");
    
    //禁用 Modem-sleep 模式
    //WiFi.setSleep(WIFI_PS_NONE);

    if (doInit != NULL) {
      doInit(&wifi_fs_server);
    }

    //启动服务
    wifi_fs_server.init();
    showlog("\nserver on http://" + local.toString());
  #endif

  #ifdef wifi_enabled
    if (WiFi.status() != WL_CONNECTED){
      showlog("WiFi is invalid!");      
      Serial.flush();

      WiFi.mode(WIFI_OFF);      
      ESP.deepSleep(10e6, RF_DISABLED);
      return false;
    }
    
    String str = "\nHost: ";
      str.concat(dev_name);
      str.concat("\nWiFi: ");
      str.concat(WiFi.SSID());
      str.concat("\nIP: ");
      str.concat(WiFi.localIP().toString());
    showlog(str);
  #endif

  return true;
}
