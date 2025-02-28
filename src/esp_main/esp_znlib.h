/********************************************************************************
  作者: dmzn@163.com 2025-02-28
  描述: 常用函数库
********************************************************************************/
#ifndef _esp_funtion__
#define _esp_funtion__
#include "esp_define.h"

/*
  date: 2025-02-21 18:32:02
  parm: 精确值
  desc: 开机后经过的计时
*/
inline uint64_t GetTickCount(bool precise = true) {
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

#endif