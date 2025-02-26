/********************************************************************************
  作者: dmzn@163.com 2025-02-21
  描述: 功能模块
********************************************************************************/
#ifndef _esp_module__
#define _esp_module__
#include "esp_define.h"

//global-------------------------------------------------------------------------
//打印日志
void showlog(const String& event);

/*
  date: 2025-02-21 18:32:02
  parm: 精确值
  desc: 开机后经过的计时
*/
uint64_t GetTickCount(bool precise = true) {
  if (precise) {
    return micros();
  } else {
    return millis();
  }
}

/*
  date: 2025-02-24 21:21:02
  desc: 新建缓冲数据项
*/
charb* sys_buf_new() {
  charb* item = (charb*)malloc(sizeof(charb));
  if (!item) {
    showlog("buf_new: malloc failed");
    return item;
  }

  item->data = NULL;
  item->len = 0;
  item->next = NULL;
  item->used = false;
  item->val_bool = false;
  item->val_ptr = NULL;

  sys_buffer_size++;
  return item;
}

/*
  date: 2025-02-25 21:26:02
  parm: 数据项
  desc: 验证item是否有效(true,有效)
*/
inline bool sys_buf_valid(charb* item) {
  return item != NULL && item->data != NULL;
}

/*
  date: 2025-02-25 21:26:02
  parm: 数据项
  desc: 验证item是否无效(true,无效)
*/
inline bool sys_buf_invalid(charb* item) {
  return item == NULL || item->data == NULL;
}

/*
  date: 2025-02-24 22:11:02
  parm: 数据长度
  desc: 从缓冲区中锁定满足长度为len的项
*/
charb* sys_buf_lock(byte len_data) {
  if (len_data < 1 || len_data > UINT8_MAX) {
    return NULL;
  }

  charb* item = NULL;  //待返回项
  charb* item_first = NULL; //首个可用项
  charb* item_last = NULL; //链表结束项

  noInterrupts();
  //sync-lock

  charb* tmp = sys_data_buffer;
  while (tmp != NULL)
  {
    item_last = tmp;
    if (tmp->used) { //使用中
      tmp = tmp->next;
      continue;
    }

    if (tmp->len >= len_data) { //长度满足
      item = tmp;
      break;
    }

    if (item_first == NULL || item_first->len < tmp->len) { //首个可用项,或长度优先
      item_first = tmp;
    }

    //next item
    tmp = tmp->next;
  }

  if (item == NULL) { //缓冲区未命中
    if (sys_buffer_size == 0) { //缓冲区为空
      sys_data_buffer = sys_buf_new();
      item = sys_data_buffer;
    } else if (sys_buffer_size < sys_buffer_max) { //缓冲区未满
      item = sys_buf_new();
      item_last->next = item;
    } else if (item_first != NULL) { //有可选项
      item = item_first;
      #ifdef debug_enabled
      Serial.println("sys_buf_lock: re-used,size " + String(sys_buffer_size));
      #endif
    }
  }
  
  if (item != NULL) { //命中项
    if (item->len < 1) { //未申请空间
      item->data = (char*)malloc(len_data);
      if (item->data != NULL) {
        item->len = len_data;
      }
    } else if (item->len < len_data) { //长度不够重新分配      
      #ifdef debug_enabled
      byte old_len = item->len;
      #endif

      char* data_ptr = item->data;
      item->len = 0;
      item->data = (char*)realloc(data_ptr, len_data);

      if (item->data != NULL) {
        //appy new memory
        item->len = len_data;
      } else {
        //free old memory
        free(data_ptr);
      }

      #ifdef debug_enabled
      Serial.println("sys_buf_lock: re-malloc " + String(old_len) + "->" + String(len_data));
      #endif
    }

    if (item->data != NULL) {
      item->used = true; //锁定
    }
  }

  interrupts(); //unlock
  if (sys_buf_invalid(item)) {
    Serial.println("sys_buf_lock: failure,size " + String(sys_buffer_size));
  }
  return item;
}

/*
  date: 2025-02-24 22:14:02
  parm: 数据
  desc: 释放数据使其可用
*/
void sys_buf_unlock(charb* item) {
  if (item != NULL) {
    noInterrupts();//sync-lock
    item->used = false;
    interrupts(); //unlock
  }
}

/*
  date: 2025-02-19 10:10:20
  parm: 键值字符串;键名;默认值;分隔符
  desc: 从str中提取keyname的值
*/
String split_val(const String& str, const String& keyname, const char* defval = NULL,
  const char* split_tag = split_tag) {
  String key = keyname + "=";
  int idx_key = str.indexOf(key);

  if (idx_key < 0) {
    if (defval != NULL) {
      return defval;
    }
    return "";
  }

  int idx_tag = str.indexOf(split_tag, idx_key + key.length());
  if (idx_tag < 0) {
    return str.substring(idx_key + key.length());
  }
  else {
    return str.substring(idx_key + key.length(), idx_tag);
  }
}

/*
  date: 2025-02-20 21:13:10
  parm: 字符串;目标指针
  desc: 将字符串转换为常量字符串
*/
void str2char(const String& str, const char*& dest) {
  if (dest != NULL) {
    free((void*)dest);
    dest = NULL;
  }

  if (str.length() > 0) {
    dest = (const char*)malloc(str.length() + 1);
    if (dest == NULL) {
      showlog("str2char: malloc failed");
      return;
    }

    //copy to new memory
    strcpy((char*)dest, str.c_str());
  }
}

//随机数-------------------------------------------------------------------------
#ifdef random_enabled
/*
  date: 2025-02-25 16:14:05
  parm: 最大;最小
  desc: 生成随机数
*/
long sys_random(long big = 0, long small = 0) {
  if (big < 1) {
    return ESP8266TrueRandom.random();
  }

  if (small < 1) {
    return ESP8266TrueRandom.random(big);
  }

  return ESP8266TrueRandom.random(small, big);
}

/*
  date: 2025-02-26 11:43:05
  desc: 生成随机的UUID
*/
charb* sys_random_uuid() {
  byte num[16];
  ESP8266TrueRandom.uuid(num);
  String str = ESP8266TrueRandom.uuidToString(num);

  charb* ret = sys_buf_lock(str.length() + 1);
  if (sys_buf_valid(ret)) {
    strcpy(ret->data, str.c_str());
  }
  return ret;
}
#endif

//INI----------------------------------------------------------------------------
#ifdef ini_enabled
charb* ini_errmsg(uint8_t code){
  charb* ret = sys_buf_lock(30);
  if (sys_buf_invalid(ret)) {
    return NULL;
  }

  switch (code){
  case IniFile::errorNoError:
    strcpy(ret->data, "no error");
    break;
  case IniFile::errorFileNotFound:
    strcpy(ret->data, "file not found");
    break;
  case IniFile::errorFileNotOpen:
    strcpy(ret->data, "file not open");
    break;
  case IniFile::errorBufferTooSmall:
    strcpy(ret->data, "buffer too small");
    break;
  case IniFile::errorSeekError:
    strcpy(ret->data, "seek error");
    break;
  case IniFile::errorSectionNotFound:
    strcpy(ret->data, "section not found");
    break;
  case IniFile::errorKeyNotFound:
    strcpy(ret->data, "key not found");
    break;
  case IniFile::errorEndOfFile:
    strcpy(ret->data, "end of file");
    break;
  case IniFile::errorUnknownError:
    strcpy(ret->data, "unknown error");
    break;
  default:
    strcpy(ret->data, "unknown error value");
    break;
  }

  return ret;
}

/*
  date: 2025-02-26 15:06:05
  parm: 小节;键名;默认值
  desc: 读取ini配置文件中的值
*/
charb* ini_getval(const char* sec, const char* key, const char* def = "") {
  charb* ret = sys_buf_lock(ini_val_len);
  if (sys_buf_invalid(ret)) {
    showlog("ini_getval: lock buffer failure");
    return NULL;
  }

  IniFile ini_file(ini_filename, FILE_READ);
  if (!ini_file.open()) {
    showlog("ini_getval: file " + String(ini_filename) + "  does not exist");
    strcpy(ret->data, def); //default value
    return ret;
  }  

  charb* msg;
  if (!ini_file.validate(ret->data, ini_val_len)) { //键值超长
    msg = ini_errmsg(ini_file.getError());
    if (sys_buf_valid(msg)) {
      showlog("ini_getval: file " + String(ini_filename) + "  not valid," + String(msg->data));
      sys_buf_unlock(msg);
    }

    strcpy(ret->data, def); //default value
    ini_file.close();
    return ret;
  }

  if (!ini_file.getValue(sec, key, ret->data, ini_val_len)) { //读取失败
    msg = ini_errmsg(ini_file.getError());
    if (sys_buf_valid(msg)) {
      showlog("ini_getval: read " + String(ini_filename) + "  failure," + String(msg->data));
      sys_buf_unlock(msg);
    }

    //default value
    strcpy(ret->data, def);
  }

  ini_file.close();
  return ret;
}
#endif

//MQTT---------------------------------------------------------------------------
#ifdef mqtt_enabled
/*
  date: 2025-02-17 11:42:05
  desc: 连接mqtt服务器并订阅主题
*/
void mqtt_conn_broker() {
  Serial.print("MQTT connection...");
  bool is_conn = false;

  if (mqtt_server_user != NULL) {
    #ifdef debug_enabled
    Serial.println("mqtt_conn_broker: " + String(mqtt_client_id) + " - " + 
      String(mqtt_server_user) + String(mqtt_server_pwd));
    #endif 

    #ifdef mqtt_online
    //do conn
    is_conn = mqtt_client.connect(mqtt_client_id, mqtt_server_user, mqtt_server_pwd, 
      mqtt_topic_log, 2, true, mqtt_server_willmsg);
    #else
    //do conn
    is_conn = mqtt_client.connect(mqtt_client_id, mqtt_server_user, mqtt_server_pwd);
    #endif
  } else {
    #ifdef debug_enabled
    Serial.println("mqtt_conn_broker: " + String(mqtt_client_id));
    #endif 

    #ifdef mqtt_online
    //do conn
    is_conn = mqtt_client.connect(mqtt_client_id, mqtt_topic_log, 2, true, mqtt_server_willmsg);
    #else 
    //do conn
    is_conn = mqtt_client.connect(mqtt_client_id);
    #endif
  }

  if (!is_conn) {
    Serial.print("failed, rc=");
    Serial.print(mqtt_client.state());
    Serial.println(" try again in 3 seconds");

    // Wait 3 seconds before retrying
    delay(3000);
    return;
  }

  #ifdef mqtt_online
  String str_online = "online: ";
  str_online.concat(mqtt_client_id);
  str_online.concat(",");
  str_online.concat(WiFi.localIP().toString());
  mqtt_client.publish(mqtt_topic_log, str_online.c_str(), true);
  #endif

  #ifdef debug_enabled
  Serial.println("mqtt_conn_broker: subscribe " + String(mqtt_topic_cmd) + " " + 
    String(mqtt_topic_cmd_qos));
  #endif
  mqtt_client.subscribe(mqtt_topic_cmd, mqtt_topic_cmd_qos);
}

/*
  date: 2025-02-18 19:07:15
  parm: 数据;broker保留数据
  desc: 将data写入mqtt发送缓冲
*/
void mqtt_send(const String& data, bool retained = false) {
  if (!mqtt_client.connected() || data.length() < 1) {
    return;
  }
    
  charb* item;
  if (mqtt_send_buffer.isFull()) {
    mqtt_send_buffer.lockedPop(item);
    sys_buf_unlock(item);
  }

  item = sys_buf_lock(data.length() + 1);
  if (!sys_buf_valid(item)) {
    Serial.println("mqtt_send: lock data failed");
    return;
  }

  item->val_bool = retained;
  strcpy(item->data, data.c_str());
  mqtt_send_buffer.lockedPushOverwrite(item);
}
#endif

/*
  date: 2025-02-12 15:43:20
  parm: 日志
  desc: 向控制台和mqtt发送日志
*/
void showlog(const String& event) {
  Serial.println(event);

  #ifdef mqtt_showlog
  mqtt_send(event);
  #endif
}

//配置WiFi-----------------------------------------------------------------------
#ifdef wifi_enabled
/*
  date: 2025-02-12 15:43:20
  desc: 提供用户配置wifi的入口
*/
bool wifi_config_by_web() {
  //build id
  dev_id = WiFi.macAddress();
  dev_id.replace(":", "");
  dev_id.toLowerCase();

  if (wifi_on_serverInit != NULL) {
    wifi_on_serverInit(step_init_begin);
  }

  //手动---------------------------------------------------
  #ifdef wifi_manualconfig  
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

      if (i > 20) {
        break;
      }
    }
  #endif

  //自动---------------------------------------------------
  #ifdef wifi_autoconfig
    WiFiManager wifiManager;
    wifiManager.autoConnect(dev_name);
  #endif

  //自动 + FS----------------------------------------------
  #ifdef wifi_fs_autoconfig  
    if (!lfs_isok) {
      showlog("LittleFS error!");
      return false;
    }

    //加载参数
    #ifdef mqtt_enabled
      //mqtt param
      String str_server = "";
      String str_clientID = "";
      String str_topic_cmd = "";
      String str_topic_log = "";

      if (LittleFS.exists(wifi_fs_server.getConfiFileName())) {
        wifi_fs_server.getOptionValue(label_mqtt_server, str_server);
        wifi_fs_server.getOptionValue(label_mqtt_clientID, str_clientID);
        wifi_fs_server.getOptionValue(label_mqtt_topic_cmd, str_topic_cmd);
        wifi_fs_server.getOptionValue(label_mqtt_topic_log, str_topic_log);
      };

      if (str_clientID.equals(str_null) || str_clientID.length() == 0) {
        str_clientID = "esp-" + dev_id;
      }

      if (str_topic_cmd.equals(str_null) || str_topic_cmd.length() == 0) {
        str_topic_cmd = "esp/cmd/" + dev_id;
      }

      if (str_topic_log.equals(str_null) || str_topic_log.length() == 0) {
        str_topic_log = "esp/log/" + dev_id;
      }
              
      String mqtt_str;
      //server ip
      str2char(split_val(str_server, "ip"), mqtt_server_ip);
      if (mqtt_server_ip == NULL) {        
        //only ip
        str2char(str_server, mqtt_server_ip);
      } else {
        //port
        mqtt_str = split_val(str_server, "port");
        if (mqtt_str.length() > 0) {
          mqtt_server_port = mqtt_str.toInt();
        }

        //user
        str2char(split_val(str_server, "user"), mqtt_server_user);
        //pwd
        str2char(split_val(str_server, "pwd"), mqtt_server_pwd);
      }

      //topic cmd
      str2char(split_val(str_topic_cmd, "topic"), mqtt_topic_cmd);
      if (mqtt_topic_cmd == NULL) {
        //only topic
        str2char(str_topic_cmd, mqtt_topic_cmd);
      } else {
        //topic qos
        mqtt_str = split_val(str_topic_cmd, "qos");
        if (mqtt_str.length() > 0) {
          mqtt_topic_cmd_qos = mqtt_str.toInt();
          if (mqtt_topic_cmd_qos > 2) {
            mqtt_topic_cmd_qos = 2;
          }
        }
      }

      //topic log
      str2char(str_topic_log, mqtt_topic_log);
      //client id
      str2char(str_clientID, mqtt_client_id);   
      //will message
      str2char(split_val(str_server, "wmsg", "offline"), mqtt_server_willmsg);
    #endif

    #ifdef ntp_enabled
      String str_ntp_server = "";

      if (LittleFS.exists(wifi_fs_server.getConfiFileName())) {
        wifi_fs_server.getOptionValue(label_ntp_server, str_ntp_server);
      };

      if (str_ntp_server.equals(str_null) || str_ntp_server.length() == 0) {
        str_ntp_server = "ntp1.aliyun.com";
      }

      //ntp server
      str2char(split_val(str_ntp_server, "ntp"), ntp_server);
      if (ntp_server == NULL) {
        //only server
        str2char(str_ntp_server, ntp_server);
      } else {
        String ntp_str = split_val(str_ntp_server, "zone");
        if (ntp_str.length() > 0) {
          ntp_timezone = ntp_str.toInt();
        }

        ntp_str = split_val(str_ntp_server, "update");
        if (ntp_str.length() > 0) {
          ntp_interval = ntp_str.toInt();
        }
      }

      ntp_client.setPoolServerName(ntp_server); //NTP服务器
      ntp_client.setTimeOffset(ntp_timezone * 60 * 60); //60*60为东一区时间，北京时间为东八区
      ntp_client.setUpdateInterval(ntp_interval * 60 * 1000); //设置NTP更新最小时间间隔(毫秒)
    #endif

    if (wifi_on_serverInit != NULL) {
      wifi_on_serverInit(step_init_getoption);
    }

    //连接WiFi
    IPAddress local = wifi_fs_server.startWiFi(10000, dev_name, "");

    //禁用 Modem-sleep 模式
    //WiFi.setSleep(WIFI_PS_NONE);

    //添加参数配置
    #ifdef mqtt_enabled
      wifi_fs_server.addOptionBox("MQTT");
      wifi_fs_server.addOption(label_mqtt_server, str_server);
      wifi_fs_server.addOption(label_mqtt_clientID, str_clientID);
      wifi_fs_server.addOption(label_mqtt_topic_cmd, str_topic_cmd);
      wifi_fs_server.addOption(label_mqtt_topic_log, str_topic_log);
    #endif

    #ifdef ntp_enabled
      wifi_fs_server.addOptionBox("参数");
      wifi_fs_server.addOption(label_ntp_server, str_ntp_server);
    #endif

    if (wifi_on_serverInit != NULL) {
      wifi_on_serverInit(step_init_setoption);
    }

    //启动服务
    wifi_fs_server.init();
  #endif

  //结果---------------------------------------------------
  //check status
  wifi_isok = WiFi.status() == WL_CONNECTED;
  
  #ifdef wifi_fs_autoconfig
  Serial.print("\nESP Server On: ");
  Serial.println(local);
  #else
  if (!wifi_isok) {
    showlog("WiFi is invalid!");
    Serial.flush();

    WiFi.mode(WIFI_OFF);
    ESP.deepSleep(10e6, RF_DISABLED);
    return false;
  }
  #endif

  if (wifi_isok){
    String str = "\nHost: ";
      str.concat(dev_name);
      str.concat("\nWiFi: ");
      str.concat(WiFi.SSID());
      str.concat("\nIP: ");
      str.concat(WiFi.localIP().toString());
      str.concat("\nID: ");
      str.concat(dev_id);
    showlog(str);
  }

  return wifi_isok;
}
#endif

//业务---------------------------------------------------------------------------
/*
  date: 2025-02-22 10:01:10
  desc: 在setup开始时的业务
*/
bool do_setup_begin() {
  #ifdef lfs_enabled
  lfs_isok = LittleFS.begin();

  if (!lfs_isok) {
    showlog("format and restart...");
    LittleFS.format();
    ESP.restart();
    return false;
  }
  #endif

  return true;
}

/*
  date: 2025-02-15 19:45:10
  desc: 在setup结束时的业务
*/
void do_setup_end() {
  #ifdef wifi_enabled
  //connect wifi
  if (!wifi_config_by_web()) return;
  #endif

  #ifdef mqtt_enabled
  if (mqtt_server_ip != NULL) {
    #ifdef debug_enabled
    Serial.println("mqtt_server: " + String(mqtt_server_ip) + ":" +
      String(mqtt_server_port));
    #endif

    mqtt_client.setServer(mqtt_server_ip, mqtt_server_port);
    mqtt_client.setCallback(mqtt_on_data);
  }
#endif
}

/*
  date: 2025-02-15 15:45:10
  desc: 在loop开始时执行业务
*/
bool do_loop_begin() {
  #ifdef wifi_fs_autoconfig  
  if (wifi_fs_server.getCaptivePortal()) {
    wifi_fs_server.updateDNS();
    return false;
  }
  #endif

  #ifdef ntp_enabled
  //update time
  ntp_client.update();
  #endif

  return true;
}

/*
  date: 2025-02-15 15:45:10
  desc: 在loop结束时执行业务
*/
void do_loop_end() {
  #ifdef mqtt_enabled
  if (!mqtt_client.connected()) {
    mqtt_conn_broker();
  }
  else {
    mqtt_client.loop();
    charb* item;

    while (mqtt_send_buffer.lockedPop(item)) {
      mqtt_client.publish(mqtt_topic_log, item->data, item->val_bool);
      sys_buf_unlock(item);
    }
  }
  #endif   
}

#endif