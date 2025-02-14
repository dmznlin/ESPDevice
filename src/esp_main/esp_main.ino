/********************************************************************************
  作者: dmzn@163.com 2025-02-12
  描述: 适用于esp8266的基础框架
********************************************************************************/
#include "esp_module.h"
#include <AsyncFsWebServer.h>

void doInit(void *srv){
  AsyncFsWebServer* local = (AsyncFsWebServer*) srv;
  local->addOptionBox("运行参数");
}

void setup() {
  /* Initialize serial and wait for port to open: */
  Serial.begin(115200);

  /* This delay gives the chance to wait for a Serial Monitor without blocking if none is found */
  delay(1500);
  
  /*user web server to config wifi*/
  wifi_config_by_web(doInit);
}

void loop() {
  // put your main code here, to run repeatedly:

}