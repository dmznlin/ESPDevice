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
    Serial.println("mqtt_send: lock data failed");
    return;
  }

  item->val_bool = retained;
  strcpy(item->data, data);
  mqtt_send_buffer.lockedPushOverwrite(item);
}
#endif

//配置WiFi-----------------------------------------------------------------------
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
    showlog(str.c_str());
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
  #ifdef run_status
  size_heap_last = ESP.getFreeHeap();
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
    String cfg = ini_getval("system", "dev_name");
    if (cfg.length() > 0) str2char(cfg, dev_name, false);

    cfg = ini_getval("system", "dev_id");
    if (cfg.length() > 0) str2char(cfg, dev_id, false);

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

  //enable showlog callback
  mqtt_do_send = mqtt_send;
  #endif
}

/*
  date: 2025-02-15 15:45:10
  desc: 在loop开始时执行业务
*/
bool do_loop_begin() {
  #ifdef sys_auto_delay
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
    led_bright_start = GetTickCount(false);
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

  return true;
}

/*
  date: 2025-02-15 15:45:10
  desc: 在loop结束时执行业务
*/
void do_loop_end() {
  #ifdef run_status
  if (run_status_update > 0 && GetTickcountDiff(run_status_lastsend) >= run_status_update * 1000) {
    //update tickcount
    run_status_lastsend = GetTickCount(false);

    //now free heap
    size_heap_now = ESP.getFreeHeap();

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
  #endif

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
    } else {
      analogWrite(LED_BUILTIN, led_bright_table[idx]);
    }

    //idx已写入
    led_bright_tableIndex = idx + 1;
  }
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