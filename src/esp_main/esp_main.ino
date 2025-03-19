/********************************************************************************
  作者: dmzn@163.com 2025-02-12
  描述: 适用于esp8266/esp32的基础框架
********************************************************************************/
#include "esp_define.h"
#include "esp_module.h"
#include "esp_znlib.h"

/*
  date: 2025-02-17 11:00:12
  parm: 初始化阶段
  desc: 提供用户配置wifi的入口
*/
void do_wifi_onInit(byte step) {
  #ifdef wifi_fs_autoconfig
    if (step == step_init_setoption) {
      //do something
    }
  #endif
}

#ifdef mqtt_enabled
/*
  date: 2025-02-17 11:07:30
  parm: 主题;内容;内容长度
  desc: 接收并处理mqtt数据
*/
void do_mqtt_onData(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
#endif

//主程序-------------------------------------------------------------------------
void setup() {
  /* Initialize serial and wait for port to open: */
  Serial.begin(115200);

  /* This delay gives the chance to wait for a Serial Monitor without blocking if none is found */
  delay(1500);

  /*external setup*/
  if (!do_setup_begin()) return;

  #ifdef wifi_enabled
    /*use web server to config wifi*/
    wifi_on_serverInit = do_wifi_onInit;
  #endif

  #ifdef mqtt_enabled
    /*parse mqtt data*/
    mqtt_on_data = do_mqtt_onData;
  #endif

  /*在这里开始写你的代码*/

  /*external setup*/
  do_setup_end();
}

void loop() {
  /*external loop*/
  if (!do_loop_begin()) return;
  //do_loop_begin();

  /*在这里开始写你的代码*/

  /*external loop*/
  do_loop_end();
}