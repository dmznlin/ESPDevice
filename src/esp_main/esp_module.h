/********************************************************************************
  作者: dmzn@163.com 2025-02-21
  描述: 功能模块
********************************************************************************/
#ifndef _esp_module__
#define _esp_module__
#include "esp_define.h"
#include "esp_znlib.h"

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

  charb* ret = sys_buf_lock(str.length() + 1, true);
  if (sys_buf_valid(ret)) {
    strcpy(ret->data, str.c_str());
  }
  return ret;
}
#endif

//-------------------------------------------------------------------------------
#ifdef com_enabled
/*
  date: 2025-03-09 10:49:05
  parm: 字符串
  desc: 返回str对应的硬件串口config值
*/
SerialConfig str_com_hard_cfg(const char* str) {
  if (strcmp(str, "5N1") == 0) return SERIAL_5N1;
  if (strcmp(str, "6N1") == 0) return SERIAL_6N1;
  if (strcmp(str, "7N1") == 0) return SERIAL_7N1;
  if (strcmp(str, "8N1") == 0) return SERIAL_8N1;
  if (strcmp(str, "5E1") == 0) return SERIAL_5E1;
  if (strcmp(str, "6E1") == 0) return SERIAL_6E1;
  if (strcmp(str, "7E1") == 0) return SERIAL_7E1;
  if (strcmp(str, "8E1") == 0) return SERIAL_8E1;
  if (strcmp(str, "5O1") == 0) return SERIAL_5O1;
  if (strcmp(str, "6O1") == 0) return SERIAL_6O1;
  if (strcmp(str, "7O1") == 0) return SERIAL_7O1;
  if (strcmp(str, "8O1") == 0) return SERIAL_8O1;
  if (strcmp(str, "5N2") == 0) return SERIAL_5N2;
  if (strcmp(str, "6N2") == 0) return SERIAL_6N2;
  if (strcmp(str, "7N2") == 0) return SERIAL_7N2;
  if (strcmp(str, "8N2") == 0) return SERIAL_8N2;
  if (strcmp(str, "5E2") == 0) return SERIAL_5E2;
  if (strcmp(str, "6E2") == 0) return SERIAL_6E2;
  if (strcmp(str, "7E2") == 0) return SERIAL_7E2;
  if (strcmp(str, "8E2") == 0) return SERIAL_8E2;
  if (strcmp(str, "5O2") == 0) return SERIAL_5O2;
  if (strcmp(str, "6O2") == 0) return SERIAL_6O2;
  if (strcmp(str, "7O2") == 0) return SERIAL_7O2;
  if (strcmp(str, "8O2") == 0) return SERIAL_8O2;

  //default
  return SERIAL_8N1;
}

/*
  date: 2025-03-12 15:13:05
  desc: 读取Serial串口中的数据
*/
void sys_serial_read() {
  #ifndef com_recv_overwrite
  //缓冲已满
  if (com_recv_buffer.isFull()) return;
  #endif

  int data_len = Serial.available(); //串口数据集
  if (data_len > 0) {
    //temp receive buffer
    charb* tmp = sys_buf_lock(com_recv_buf_size, true);

    do {
      if (data_len > com_recv_buf_size) {
        data_len = com_recv_buf_size;
      }

      //接收串口数据
      data_len = Serial.readBytes(tmp->data, data_len);
      for (int idx = 0; idx < data_len; idx++) {
        #ifdef com_recv_overwrite
        com_recv_buffer.pushOverwrite(tmp->data[idx]);
        #else
        com_recv_buffer.push(tmp->data[idx]);
        if (com_recv_buffer.isFull()) break; //写满即停
        #endif
      }

      //剩余数据
      data_len = Serial.available();
    } while (data_len > 0);

    //release buffer
    sys_buf_unlock(tmp);
  }
}
#endif

//NTP----------------------------------------------------------------------------
#ifdef ntp_enabled
/*
  date: 2025-03-12 15:15:05
  parm: 格式化
  desc: 使用 format 格式化ntp当前时间
*/
charb* str_now(const char* format = "%Y-%m-%d %H:%M:%S") {
  charb* ret = sys_buf_lock(32, true); //19=YYYY-mm-dd HH:MM:SS
  if (sys_buf_valid(ret)) {
    time_t et = ntp_client.getEpochTime();
    struct tm* now = gmtime(&et);
    strftime(ret->data, 32, format, now);
  }

  return ret;
}

void decode_now_date(int* year, int* mon, int* day) {
  time_t et = ntp_client.getEpochTime();
  struct tm* now = gmtime(&et);
  *year = now->tm_year + 1900;
  *mon = now->tm_mon + 1;
  *day = now->tm_mday;
}

void decode_now_time(int* hour, int* min, int* second) {
  time_t et = ntp_client.getEpochTime();
  struct tm* now = gmtime(&et);
  *hour = now->tm_hour;
  *min = now->tm_min;
  *second = now->tm_sec;
}

void decode_now(int* year, int* mon, int* day, int* hour, int* min, int* second) {
  time_t et = ntp_client.getEpochTime();
  struct tm* now = gmtime(&et);

  *year = now->tm_year + 1900;
  *mon = now->tm_mon + 1;
  *day = now->tm_mday;

  *hour = now->tm_hour;
  *min = now->tm_min;
  *second = now->tm_sec;
}
#endif

//INI----------------------------------------------------------------------------
#ifdef ini_enabled
/*
  date: 2025-02-26 15:06:05
  parm: 小节;键名;默认值
  desc: 读取ini配置文件中的值
*/
String ini_getval(const String& sec, const String& key, const String& def = "") {
  File file = LittleFS.open(ini_filename, "r");
  if (!file) {
    return def;
  }

  bool sec_match = false;
  String result = def;

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim(); //两端空格

    if (line.startsWith(";") || line.startsWith("[") || line.isEmpty()) { //注释行和空行
      if (line.startsWith("[")) {
        if (sec_match) { //new section, not found key
          return result;
        }

        line = line.substring(1, line.indexOf("]"));
        line.trim();
        sec_match = line.equalsIgnoreCase(sec);
      }
      continue;
    }

    if (sec_match) { //当前小节
      int idx = line.indexOf('=');
      if (idx > 0) {
        String k = line.substring(0, idx);
        k.trim();

        if (k.equalsIgnoreCase(key)) { //当前键
          result = line.substring(idx + 1);
          result.trim();
          break;
        }
      }
    }
  }

  file.close();
  return result;
}

/*
  date: 2025-03-04 10:57:05
  parm: 小节;键名;值
  desc: 写入ini配置文件中的值
*/
bool ini_setval(const String& sec, const String& key, const String& val) {
  if (!LittleFS.exists(ini_filename)) { //create on new
    File file = LittleFS.open(ini_filename, "w");
    if (!file) {
      showlog("ini_setval: create file failure");
      return false;
    }

    file.println("[" + sec + "]");
    file.println(key + "=" + val);

    file.flush();
    file.close();
    return true;
  }

  File src = LittleFS.open(ini_filename, "r");
  if (!src) {
    showlog("ini_setval: open file failure");
    return false;
  }

  String tmp = "/config.tmp"; //temp file
  File dst = LittleFS.open(tmp, "w");
  if (!dst) {
    src.close();
    showlog("ini_setval: write file failure");
    return false;
  }

  bool sec_match = false;
  bool key_match = false;

  while (src.available()) {
    String line = src.readStringUntil('\n');
    line.trim(); //两端空格

    if (sec_match && key_match) { //已修改完成
      dst.println(line); //write all
      continue;
    }

    if (line.startsWith(";") || line.startsWith("[") || line.isEmpty()) { //注释行和空行
      if (line.startsWith("[")) {
        if (sec_match) { //new section, add key first
          dst.println(key + "=" + val);
          dst.println();
          key_match = true;
        } else {
          String sub = line.substring(1, line.indexOf("]"));
          sub.trim();
          sec_match = sub.equalsIgnoreCase(sec);
        }
      }

      dst.println(line); //write section
      continue;
    }

    if (sec_match && !key_match) {  //find key
      int idx = line.indexOf('=');
      if (idx > 0) {
        String k = line.substring(0, idx);
        k.trim();
        key_match = k.equalsIgnoreCase(key);

        if (key_match) {
          line = key + "=" + val;
        }
      }
    }

    //write key-value
    dst.println(line);
  }

  if (!key_match) { //未找到key
    if (!sec_match) dst.println("[" + sec + "]");
    dst.println(key + "=" + val);
    dst.println();
  }

  dst.flush();
  dst.close();
  src.close(); //close all

  if (LittleFS.remove(ini_filename)) { //delete first
    return LittleFS.rename(tmp, ini_filename); //rename
  }
  return false;
}

/*
  date: 2025-03-07 09:17:05
  desc: 载入外部ini配置
*/
void ini_load_cfg() {
  String cfg = ini_getval("system", "dev_name");
  if (cfg.length() > 0) str2char(cfg, dev_name, false);

  cfg = ini_getval("system", "dev_id");
  if (cfg.length() > 0) str2char(cfg, dev_id, false);

  cfg = ini_getval("system", "dev_pwd");
  if (cfg.length() > 0) str2char(cfg, dev_pwd, false);

  cfg = ini_getval("performance", "sys_buffer_max");
  if (cfg.length() > 0) {
    long val = cfg.toInt();
    if (val > 0 && val < UINT8_MAX) sys_buffer_max = val;
  }

  #ifdef sys_auto_delay
    cfg = ini_getval("performance", "sys_loop_interval");
    if (cfg.length() > 0) {
      long val = cfg.toInt();
      if (val > 0 && val < UINT8_MAX) sys_loop_interval = val;
    }
  #endif

  #ifdef run_status
    cfg = ini_getval("performance", "run_status_update");
    if (cfg.length() > 0) {
      long val = cfg.toInt();
      if (val > 0 && val < UINT8_MAX) run_status_update = val;
    }
  #endif

  #ifdef com_enabled
    cfg = ini_getval("com_0", com_baud_rate);//UART0
    if (cfg.length() > 0) {
      uint32_t bd = cfg.toInt();
      cfg = ini_getval("com_0", com_config); //数据奇偶校验
      showlog("ini_load_cfg: hard_serial_0," + String(bd) + "|" + cfg);

      Serial.flush(); //no need call end()
      //start serial0
      Serial.begin(bd, str_com_hard_cfg(cfg.c_str()));
    }

    cfg = ini_getval("com_1", com_baud_rate); //UART1
    if (cfg.length() > 0) {
      uint32_t bd = cfg.toInt();
      cfg = ini_getval("com_1", com_config); //数据奇偶校验
      showlog("ini_load_cfg: hard_serial_1," + String(bd) + "|" + cfg);

      //start serial1
      Serial1.begin(bd, str_com_hard_cfg(cfg.c_str()));
    } else {
      Serial1.begin(115200);
    }
  #endif
}
#endif

//MQTT---------------------------------------------------------------------------
#ifdef mqtt_enabled
/*
  date: 2025-02-17 11:42:05
  desc: 连接mqtt服务器并订阅主题
*/
void mqtt_conn_broker() {
  showlog("MQTT connection...");
  bool is_conn = false;

  if (mqtt_server_user != NULL) {
    #ifdef debug_enabled
    showlog("mqtt_conn_broker: " + String(mqtt_client_id) + " - " +
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
    showlog("mqtt_conn_broker: " + String(mqtt_client_id));
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
    showlog("failed, rc=" + String(mqtt_client.state()) + " try again in 3 seconds");
    delay(3000); // Wait 3 seconds before retrying
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
  showlog("mqtt_conn_broker: subscribe " + String(mqtt_topic_cmd) + " " +
    String(mqtt_topic_cmd_qos));
  #endif
  mqtt_client.subscribe(mqtt_topic_cmd, mqtt_topic_cmd_qos);
}

/*
  date: 2025-02-18 19:07:15
  parm: 数据;broker保留数据
  desc: 将data写入mqtt发送缓冲
*/
void mqtt_send(const char* data, bool retained = false) {
  if (data == NULL) return;
  size_t data_len = strlen(data);
  if (data_len < 1 || !mqtt_client.connected())  return;

  charb* item;
  if (mqtt_send_buffer.isFull()) {
    mqtt_send_buffer.lockedPop(item);
    sys_buf_unlock(item);
  }

  item = sys_buf_lock(data_len + 1);
  if (!sys_buf_valid(item)) {
    showlog("mqtt_send: lock data failed");
    return;
  }

  item->val_bool = retained;
  strcpy(item->data, data);
  mqtt_send_buffer.lockedPushOverwrite(item);
}
#endif

//配置WiFi-----------------------------------------------------------------------
#ifdef wifi_fs_autoconfig
/*
  date: 2025-03-13 21:05:20
  desc: 管理员配置入口

  *.编辑器: /admin?pwd=xxx&do=editor
  *.格式化: /admin?pwd=xxx&do=format
  *.WiFi:   /admin?pwd=xxx&do=wifi
*/
void wifi_fs_server_admin(AsyncWebServerRequest* req) {
  String val = "";
  if (req->hasParam("pwd")) val = req->arg("pwd");

  if (!val.equals(dev_pwd)) {
    req->send(500, "text/plain", "Invalid admin password.");
    return;
  }

  if (req->hasParam("do")) {
    val = req->arg("do");
    if (val.equals("editor")) { //打开在线编辑器
      #ifdef ini_enabled
      ini_setval("system", "dev_enable_editor", "Yes");
      #else
      req->send(500, "text/plain", "Ini is disabled.");
      #endif

      return;
    }

    if (val.equals("format")) { //格式化文件系统
      LittleFS.format();
      ESP.restart();
      return;
    }

    if (val.equals("wifi")) { //重置 station 模式下的WiFi参数
      WiFiMode_t wm = WiFi.getMode();
      if (wm == WIFI_STA || wm == WIFI_AP_STA) {
        wifi_isok = false;
        delay(100);

        WiFi.persistent(true);
        WiFi.disconnect(true);
        WiFi.persistent(false);

        //restart
        ESP.restart();
      } else {
        req->send(500, "text/plain", "WiFi not on WIFI_STA modal.");
      }

      return;
    }
  }

  req->send(500, "text/plain", "Invalid admin action.");
}

#endif

#ifdef wifi_enabled
/*
  date: 2025-02-12 15:43:20
  desc: 提供用户配置wifi的入口
*/
bool wifi_config_by_web() {
  if (strcmp(dev_id, "000000") == 0) { //build id
    String id = WiFi.macAddress();
    id.replace(":", "");
    id.toLowerCase();
    str2char(id, dev_id, false);
  }

  if (wifi_on_serverInit != NULL) {
    wifi_on_serverInit(step_init_begin);
  }

  //手动---------------------------------------------------
  #ifdef wifi_manualconfig
    //设置为无线终端模式
    WiFi.mode(WIFI_STA);

    //连接wifi
    WiFi.begin(wifi_ssid, wifi_pwd);
    showlog("");
    byte i = 0;

    while (WiFi.status() != WL_CONNECTED) {
      showlog(".", false);
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
        str_clientID = "esp-" + String(dev_id);
      }

      if (str_topic_cmd.equals(str_null) || str_topic_cmd.length() == 0) {
        str_topic_cmd = "esp/cmd/" + String(dev_id);
      }

      if (str_topic_log.equals(str_null) || str_topic_log.length() == 0) {
        str_topic_log = "esp/log/" + String(dev_id);
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

    #ifdef ini_enabled
      String url = ini_getval("system", "dev_captive_url");
      if (url.length() > 0) { //认证主页
        wifi_fs_server.setCaptiveUrl(url);
      };
    #endif

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

    #ifdef ini_enabled
      String cfg = ini_getval("system", "dev_enable_editor");
      if (cfg.equals("Yes")) { //启用编辑器
        wifi_fs_server.enableFsCodeEditor();
      };
    #endif

    //管理入口
    wifi_fs_server.on("/admin", HTTP_GET, wifi_fs_server_admin);
    if (dev_pwd != NULL) {
      wifi_fs_server.setAuthentication("admin", dev_pwd);
    }

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
  showlog("\nESP Server On: " + local.toString());
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
    showlog(str.c_str());
  }

  return wifi_isok;
}
#endif

//业务---------------------------------------------------------------------------
#ifdef run_status
/*
  date: 2025-03-07 09:27:20
  desc: 打印系统运行状态日志
*/
void sys_run_status() {
  if (run_status_update > 0 && GetTickcountDiff(run_status_lastsend) >= run_status_update * 1000) {
    //update tickcount
    run_status_lastsend = sys_loop_start;

    //now free heap
    uint32_t size_heap_now = ESP.getFreeHeap();

    String info = "\nBufferSize: ";
    info.concat(sys_buffer_size);
    info.concat("\nBufferLocked: ");
    info.concat(sys_buffer_locked);
    info.concat("\nHeapFree: ");
    info.concat(size_heap_now);
    info.concat("\nHeapFreeChange: ");
    info.concat(int32_t(size_heap_now - size_heap_last));
    info.concat("\nHeapFragmentation: ");
    info.concat(ESP.getHeapFragmentation());
    info.concat("\nMaxFreeBlockSize: ");
    info.concat(ESP.getMaxFreeBlockSize());

    #ifdef ntp_enabled
    info.concat("\nTimeNow: ");
    info.concat(ntp_client.getFormattedTime());
    #endif

    info.concat("\nCoreVersion: ");
    info.concat(ESP.getCoreVersion());
    info.concat("\nSdkVersion: ");
    info.concat(ESP.getSdkVersion());
    showlog(info.c_str());

    //last
    size_heap_last = size_heap_now;
  }
}
#endif

#ifdef run_blinkled
/*
  date: 2025-03-07 09:35:10
  desc: 呼吸灯
*/
void sys_blink_led() {
  uint64_t diff = GetTickcountDiff(led_bright_start);
  if (diff <= 0 || diff >= led_bright_len) { //亮灯时间结束
    led_bright_start = 0;
    return;
  }

  //本次需达到的亮度表索引: diff/led_bright_len为时间进度
  byte idx = (diff * led_bright_tableSize / led_bright_len);

  if (idx != led_bright_tableIndex && idx >= 0 && idx < led_bright_tableSize) {
    if (idx >= led_bright_tableIndex + 3) {
      //计算与上次的索引差,若较大则补3个delay
      byte inc_idx = (idx - led_bright_tableIndex) / 3;

      for (; led_bright_tableIndex <= idx; led_bright_tableIndex += inc_idx) {
        analogWrite(LED_BUILTIN, led_bright_table[led_bright_tableIndex]);
        delay(1);
      }
    }
    else {
      analogWrite(LED_BUILTIN, led_bright_table[idx]);
    }

    //idx已写入
    led_bright_tableIndex = idx + 1;
  }
}
#endif

/*
  date: 2025-02-22 10:01:10
  desc: 在setup开始时的业务
*/
bool do_setup_begin() {
  #ifdef run_status
  size_heap_last = ESP.getFreeHeap();
  run_status_lastsend = GetTickCount(false);
  #endif

  #ifdef run_blinkled
  pinMode(LED_BUILTIN, OUTPUT);
  analogWriteFreq(500);   // 设置PWM频率为500Hz
  analogWriteRange(1023); // 10位分辨率(0-1023)
  #endif

  #ifdef lfs_enabled
  lfs_isok = LittleFS.begin();

  if (!lfs_isok) {
    showlog("format and restart...");
    LittleFS.format();
    ESP.restart();
    return false;
  }
  #endif

  #ifdef ini_enabled
  ini_load_cfg();
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
    showlog("mqtt_server: " + String(mqtt_server_ip) + ":" +
      String(mqtt_server_port));
    #endif

    mqtt_client.setServer(mqtt_server_ip, mqtt_server_port);
    mqtt_client.setCallback(mqtt_on_data);
  }

  //enable showlog callback
  mqtt_do_send = mqtt_send;
  #endif

  #ifdef com_swap_pin
  Serial.flush();
  Serial.swap(); //tx交接给外设
  while (Serial.read() > 0) {}  //clear rx
  #endif

  //setup结束,开始loop
  sys_run_step = step_run_loop;
}

/*
  date: 2025-02-15 15:45:10
  desc: 在loop开始时执行业务
*/
bool do_loop_begin() {
  #ifdef sys_loop_ticktime
  sys_loop_start = GetTickCount(false);
  #endif

  #ifdef buf_auto_unlock
  sys_buffer_stamp++;
  if (sys_buffer_stamp < 1) {
    sys_buffer_stamp = 1;
  }
  #endif

  #ifdef run_blinkled
  if (led_bright_start == 0) { //亮灯循环开始
    led_bright_start = sys_loop_start;
    analogWrite(LED_BUILTIN, led_bright_table[0]);
  }
  #endif

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

  #ifdef com_enabled
  //read data
  sys_serial_read();
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

  #ifdef run_blinkled
  sys_blink_led();
  #endif

  #ifdef run_status
  sys_run_status();
  #endif

  #ifdef sys_auto_delay
  if (sys_loop_interval > 0){
    uint64_t loop_delay = GetTickcountDiff(sys_loop_start);
    if (loop_delay < sys_loop_interval) {
      delay(sys_loop_interval - loop_delay);
    }
  }
  #endif
}

#endif