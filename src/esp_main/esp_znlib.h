/********************************************************************************
  作者: dmzn@163.com 2025-02-28
  描述: 常用函数库
********************************************************************************/
#ifndef _esp_funtion__
#define _esp_funtion__
#include "esp_define.h"

void showlog(charb* event,bool ln = true);
void showlog(const char* event, bool ln = true);
void showlog(const String &event, bool ln = true);
void showlog(const char* event[], const uint8_t size, bool ln = true);

/*
  date: 2025-02-21 18:32:02
  parm: 精确值
  desc: 开机后经过的计时
*/
inline uint64_t GetTickCount(bool precise = false) {
  if (precise) {
    return micros();
  } else {
    return millis();
  }
}

/*
  date: 2025-02-21 18:32:02
  parm: 开始计时;精确值
  desc: 从from到当前的时间差
*/
uint64_t GetTickcountDiff(uint64_t from, bool precise = false) {
  uint64_t now = GetTickCount(precise);
  if (now >= from) {
    return now - from;
  }

  return now + (UINT64_MAX) - from;
}

// 缓冲区---------------------------------------------------------------------------
/*
  date: 2025-02-24 21:21:02
  desc: 新建缓冲数据项
*/
charb* sys_buf_new() {
  charb* item = (charb*)malloc(sizeof(charb));
  if (!item) {
    showlog("buf_new: malloc failed");
    return NULL;
  }

  item->data_type = 0;
  item->data = NULL;
  item->len = 0;
  item->next = NULL;
  item->used = false;
  item->val_bool = false;
  item->val_ptr = NULL;

  #ifdef buf_auto_unlock
  item->stamp = 0;
  #endif

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
  parm: 数据长度;自动释放;类型
  desc: 从缓冲区中锁定满足长度为len的项
*/
charb* sys_buf_lock(uint16_t len_data, bool auto_unlock = false, byte data_type = 0) {
  if (len_data < 1 || len_data > 2048) { //1-2k
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
    #ifdef buf_auto_unlock
    if (tmp->stamp != 0 && tmp->stamp != sys_buffer_stamp) { //自动解锁
      tmp->used = false;
      tmp->stamp = 0;
      sys_buffer_locked--;
    }
    #endif

    item_last = tmp;
    if (tmp->used || tmp->data_type != data_type) { //使用中,类型不匹配
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
      showlog("sys_buf_lock: re-used,size " + String(sys_buffer_size));
      #endif
    }
  }

  if (item != NULL) { //命中项
    if (item->len < 1) { //未申请空间
      if (data_type == 0) { //不带类型按大的申请,带类型的按需分配
        /*
          1.缓冲大小,正常会从小 -> 到大.
          2.项越多,检索越慢.
          3.防火墙: 每 10 个,会有一个 512;每 50 个,会有一个 1024
        */
        if (sys_buffer_size > 0) {
          if (sys_buffer_size % 100 == 0) {
            len_data = 2048;
          }
          else if (sys_buffer_size % 50 == 0) {
            len_data = 1024;
          }
          else if (sys_buffer_size % 10 == 0) {
            len_data = 512;
          }
        }

        if (len_data < 10) {
          len_data = len_data * 5;
        }
        else if (len_data < 50) {
          len_data = len_data * 3;
        }
        else if (len_data < 100) {
          len_data = len_data * 2;
        }
      }

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
      showlog("sys_buf_lock: re-malloc " + String(old_len) + "->" + String(len_data));
      #endif
    }

    if (item->data != NULL) {
      #ifdef buf_auto_unlock
      if (auto_unlock) { //自动释放标记
        item->stamp = sys_buffer_stamp;
      }

      item->used = true; //锁定
      item->data_type = data_type;
      sys_buffer_locked++;
      #endif
    }
  }

  interrupts(); //unlock
  if (sys_buf_invalid(item)) {
    showlog("sys_buf_lock: failure,size " + String(sys_buffer_size));
  }
  return item;
}

/*
  date: 2025-02-24 22:14:02
  parm: 数据;重置类型
  desc: 释放数据使其可用
*/
void sys_buf_unlock(charb* item, bool reset_type = false) {
  if (item != NULL) {
    noInterrupts();//sync-lock
    item->used = false;
    if (reset_type) item->data_type = 0;
    sys_buffer_locked--;

    #ifdef buf_auto_unlock
    item->stamp = 0;
    #endif
    interrupts(); //unlock
  }
}

/*
  date: 2025-03-01 10:12:20
  parm: 数据
  desc: 生成内容为str的buf项
*/
charb* sys_buf_fill(const char* str) {
  if (str == NULL) {
    return NULL;
  }

  charb* ret = sys_buf_lock(strlen(str) + 1, true);
  if (sys_buf_valid(ret)) {
    strcpy(ret->data, str);
  }
  return ret;
}

/*
  date: 2025-03-01 11:04:20
  parm: 数组;大小
  desc: 合并str为buf项
*/
charb* sys_buf_concat(const char* str[], const uint8_t size) {
  if (size == 0 || str == NULL) {
    return NULL;
  }

  size_t total_len = 0;
  for (uint8_t idx = 0; idx < size; idx++) {
    if (str[idx] != NULL) {
      total_len += strlen(str[idx]);
    }
  }

  if (total_len < 1) { //nothing
    return NULL;
  }

  charb* ret = sys_buf_lock(total_len + 1, true);  // len+1 to add \0
  if (sys_buf_invalid(ret)) {
    return NULL;
  }

  size_t offset = 0;
  for (uint8_t idx = 0; idx < size; idx++) {
    if (str[idx] != NULL) {
      size_t len = strlen(str[idx]);
      memcpy(ret->data + offset, str[idx], len);
      offset += len;
    }
  }

  ret->data[offset] = '\0';
  return ret;
}

/*
  date: 2025-03-02 11:43:20
  desc: 显示缓冲区内存申请状态
*/
charb* sys_buf_status() {
  noInterrupts();//sync-lock
  String status = "";
  charb* tmp = sys_data_buffer;

  while (tmp != NULL)
  {
    status.concat(tmp->len);
    status.concat(" ");
    tmp = tmp->next;
  }

  interrupts(); //unlock
  return sys_buf_fill(status.c_str());
}

// showlog-----------------------------------------------------------------------
/*
  date: 2025-02-12 15:43:20
  parm: 日志;自动换行
  desc: 向控制台和mqtt发送日志
*/
void showlog(charb* event, bool ln) {
  if (sys_buf_valid(event)) {
    #ifdef com_swap_pin
    if (ln) {
      if (sys_run_step == step_run_setup) {//setup,日志由Serial输出
        Serial.println(event->data);
      } else {
        #ifdef com_print_log
        Serial1.println(event->data);
        #endif
      }
    } else {
      if (sys_run_step == step_run_setup) {//setup,日志由Serial输出
        Serial.print(event->data);
      } else {
        #ifdef com_print_log
        Serial1.print(event->data);
        #endif
      }
    }
    #else
    if (ln) {
      Serial.println(event->data);
    } else {
      Serial.print(event->data);
    }
    #endif

    #ifdef mqtt_showlog
    if (mqtt_do_send != NULL) {
      mqtt_do_send(event->data, false);
    }
    #endif
  }
}

/*
  date: 2025-03-01 23:43:20
  parm: 日志
  desc: 向控制台和mqtt发送日志
*/
void showlog(const char* event, bool ln) {
  charb* buf = sys_buf_fill(event);
  showlog(buf, ln);
  sys_buf_unlock(buf);
}

/*
  date: 2025-03-10 21:09:20
  parm: 日志
  desc: 向控制台和mqtt发送日志
*/
void showlog(const String &event, bool ln) {
  charb* buf = sys_buf_fill(event.c_str());
  showlog(buf, ln);
  sys_buf_unlock(buf);
}

/*
  date: 2025-03-01 23:47:20
  parm: 日志数组;数组大小
  desc: 向控制台和mqtt发送日志
*/
void showlog(const char* event[], const uint8_t size, bool ln) {
  charb* buf = sys_buf_concat(event, size);
  showlog(buf, ln);
  sys_buf_unlock(buf);
}

// 字符串------------------------------------------------------------------------
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
  parm: 字符串;目标指针;检查目标
  desc: 将字符串转换为常量字符串
*/
void str2char(const String& str, const char*& dest, bool check = true) {
  if (check && dest != NULL) {
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

//编码&加密----------------------------------------------------------------------
#ifdef md5_enabled
/*
  date: 2025-03-05 09:04:10
  parm: 数据
  desc: 计算data的md5值
*/
charb* str_md5(const char* data) {
  if (data == NULL) return NULL;
  size_t len = strlen(data);
  if (len < 1) return NULL;

  uint8_t md5Bytes[br_md5_SIZE];
  br_md5_context md5Context;

  // 初始化 MD5 上下文
  br_md5_init(&md5Context);
  // 输入数据进行计算
  br_md5_update(&md5Context, (const void*)data, len);
  // 输出结果到 md5Bytes 数组
  br_md5_out(&md5Context, md5Bytes);

  charb* ret = sys_buf_lock(br_md5_SIZE * 2 + 1, true);
  if (sys_buf_invalid(ret)) return NULL;
  byte cur = 0;

  //hex char
  const char hex[] = "0123456789abcdef";

  for (int i = 0; i < br_md5_SIZE; i++) {
    byte b = md5Bytes[i];
    byte high = (b >> 4);
    byte low = (b & 0xF);

    ret->data[cur++] = hex[b >> 4]; //高4位
    ret->data[cur++] = hex[b & 0x0F]; //低4位
  }

  ret->data[cur] = '\0';
  return ret;
}
#endif

#ifdef crc_enabled
/*
  date: 2025-03-06 09:27:05
  parm: 数据
  desc: 计算data的crc8
*/
charb* str_crc8(const char* data) {
  if (data == NULL) return NULL;
  size_t len = strlen(data);
  if (len < 1) return NULL;

  charb* ret = sys_buf_lock(2 + 1, true); //2 + \0
  if (sys_buf_valid(ret)) {
    uint8_t crc = calcCRC8((const uint8_t*)data, len); //CRC-8
    snprintf(ret->data, 3, "%02x", crc);
  }

  return ret;
}

/*
  date: 2025-03-06 10:18:05
  parm: 数据
  desc: 计算data的crc16
*/
charb* str_crc16(const char* data) {
  if (data == NULL) return NULL;
  size_t len = strlen(data);
  if (len < 1) return NULL;

  charb* ret = sys_buf_lock(4 + 1, true); //4 + \0
  if (sys_buf_valid(ret)) {
    uint16_t crc = calcCRC16((const uint8_t*)data, len,
      CRC16_ARC_POLYNOME, CRC16_ARC_INITIAL, CRC16_ARC_XOR_OUT,
      CRC16_ARC_REV_IN, CRC16_ARC_REV_OUT); //CRC-16-IBM
    snprintf(ret->data, 5, "%04x", crc);
  }

  return ret;
}

/*
  date: 2025-03-06 10:18:05
  parm: 数据
  desc: 计算data的crc16
*/
charb* str_crc32(const char* data) {
  if (data == NULL) return NULL;
  size_t len = strlen(data);
  if (len < 1) return NULL;

  charb* ret = sys_buf_lock(8 + 1, true); //8 + \0
  if (sys_buf_valid(ret)) {
    uint32_t crc = calcCRC32((const uint8_t*)data, len); //CRC-32
    snprintf(ret->data, 9, "%08x", crc);
  }

  return ret;
}
#endif


#endif