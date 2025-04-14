/********************************************************************************
  作者: dmzn@163.com 2025-04-03
  描述: 基于基础框架的Mesh系统
********************************************************************************/
#include "esp_define.h"
#include "esp_module.h"
#include "esp_znlib.h"

const char* action_chat = "chat"; //聊天
const char* action_search = "search"; //检索
const char* file_goods = "/web/goods.txt"; //商品信息文件

chart* shop_goods = NULL; //商品信息列表

//-------------------------------------------------------------------------------
#ifdef mesh_enabled

//desc: 载入商品列表
chart* load_goods() {
  if (sys_buf_timeout_invalid(shop_goods)) {
    if (shop_goods != NULL) {
      sys_buf_timeout_unlock(shop_goods);
    }

    //load and lock
    shop_goods = sys_buf_timeout_lock(file_load_text(file_goods));
  }

  return shop_goods;
}

//desc: 将数据写入mesh发送缓冲
void mesh_send(chart* mesh_data, int32_t mesh_id) {
  if (sys_buf_timeout_valid(mesh_data)) {
    if (mesh_send_buffer.isFull()) {
      chart* old_data = NULL;
      mesh_send_buffer.lockedPop(old_data);
      sys_buf_timeout_unlock(old_data);
    }

    mesh_data->val_int = mesh_id;
    if (!mesh_send_buffer.lockedPushOverwrite(mesh_data)) { //进入mesh发送队列
      sys_buf_timeout_unlock(mesh_data);
    }
  }
}

//desc: 处理websocket消息
void wifi_doWebsocket(AsyncWebSocket* server, AsyncWebSocketClient* client,
  AwsEventType type, void* arg, uint8_t* data, size_t len) {
  switch (type) {
  case WS_EVT_CONNECT:
    {
      #ifdef debug_enabled
      charb* msg = sys_buf_lock(50, true);
      if (sys_buf_valid(msg)) {
        snprintf(msg->data, 50, "Websocket: client %u connected", client->id());
        showlog(msg);
      }
      sys_buf_unlock(msg);
      #endif
    }
    break;
  case WS_EVT_DISCONNECT:
    {
      #ifdef debug_enabled
      charb* msg = sys_buf_lock(50, true);
      if (sys_buf_valid(msg)) {
        snprintf(msg->data, 50, "Websocket: client %u disconnected", client->id());
        showlog(msg);
      }
      sys_buf_unlock(msg);
      #endif
    }
    break;
  case WS_EVT_DATA:
    {
      AwsFrameInfo* info = (AwsFrameInfo*)arg;
      if (info->opcode != WS_TEXT) return; //只处理文本消息

      charb* msg = sys_buf_lock(len + 1, true);
      if (sys_buf_invalid(msg)) return; //内存不足
      memcpy(msg->data, data, len);
      msg->data[len] = '\0';

      /*
        websocket协议:
        1.格式: {type: chat/search/control/message, data:xxx}
        2.类型: chat,聊天;search,检索;control,控制;message,消息
        3.数据: 具体数据
          *.chat:{type: chat, from: mesh_name, data: message}
          *.search:{type: search, id: client, data: goods or shop}
      */

      charb* ptr = json_get(msg->data, "type");
      if (sys_buf_invalid(ptr)) { //无效消息类型
        sys_buf_unlock(msg);
        return;
      }

      if (strcmp(ptr->data, action_chat) == 0) { //聊天
        sys_buf_unlock(ptr);
        server->textAll(msg->data); //发送给本服务器

        chart* mesh_data = sys_buf_timeout_lock(json_set(msg->data, "from", mesh_name)); //消息尾巴
        mesh_send(mesh_data, -1); //广播至mesh
      } else
      //--------------------------------------------------------------------------
      if (strcmp(ptr->data, action_search) == 0) { //信息检索
        sys_buf_unlock(ptr);
        ptr = json_get(msg->data, "data");

        if (sys_buf_valid(ptr)) {
          bool is_empty = strlen(ptr->data) < 1;
          bool is_local = strstr(mesh_name, ptr->data) != NULL;
          sys_buf_unlock(ptr);

          if (is_empty || is_local) { //检索内容为空 或 包含店名
            load_goods();
            if (sys_buf_timeout_valid(shop_goods)) {
              server->text(client->id(), shop_goods->buff->data);
            }
          }

          if (!is_empty) { //发送至mesh网络
            ptr = json_set(msg->data, "id", int2str(client->id())); //接收方id
            chart* mesh_data = sys_buf_timeout_lock(ptr);
            mesh_send(mesh_data, -1); //广播至mesh
          }
        }
      }

      //释放消息
      sys_buf_unlock(msg);
    }
    break;
  default:
    break;
  }
}

/*
  date: 2025-04-08 21:45:10
  desc: 处理mesh网络消息

  消息格式:
  1.格式: {type: chat/search/control/message, data:xxx}
  2.类型: chat,聊天;search,检索;control,控制;message,消息
  3.数据: 具体数据
    *.chat:{type: chat, from: mesh_name, data: message}
    *.search:{type: search, from: mesh_name, id: client, data: goods or shop}
*/
void mesh_do_receive(uint32_t from, TSTRING& msg) {
  charb* ptr = json_get(msg.c_str(), "type");
  if (sys_buf_invalid(ptr)) { //无效消息类型
    return;
  }

  if (strcmp(ptr->data, action_chat) == 0) { //聊天室数据
    sys_buf_unlock(ptr);
    wifi_fs_server.wsBroadcast(msg.c_str());
    return;
  }

  if (strcmp(ptr->data, action_search) == 0) { //检索数据
    sys_buf_unlock(ptr);
    ptr = json_get(msg.c_str(), "from");

    if (sys_buf_valid(ptr)) { //检索结果
      sys_buf_unlock(ptr);
      ptr = json_get(msg.c_str(), "id"); //ws.id

      if (sys_buf_valid(ptr)) {
        wifi_fs_server.getWebSocket()->text(atoi(ptr->data), msg.c_str()); //发给前端
        sys_buf_unlock(ptr);
      }
    } else { //检索请求
      if (strstr(mesh_name, ptr->data) != NULL) { //包含店名
        sys_buf_unlock(ptr);
        load_goods();

        if (sys_buf_timeout_valid(shop_goods)) {
          ptr = json_set(shop_goods->buff->data, "from", mesh_name); //应答节点
          chart* mesh_data = sys_buf_timeout_lock(ptr);
          mesh_send(mesh_data, from); //发送至mesh
        }
      }
    }

    return;
  }
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

  /*在这里开始写你的代码*/
  #ifdef mesh_enabled
    wifi_on_websocket = wifi_doWebsocket;
    mesh_on_receive = mesh_do_receive;
  #endif

  /*external setup*/
  do_setup_end();
}

void loop() {
  /*external loop*/
  //if (!do_loop_begin()) return;
  do_loop_begin();

  /*在这里开始写你的代码*/
  chart* msg;
  while (mesh_send_buffer.lockedPop(msg)) {
    if (sys_buf_timeout_valid(msg)) {
      if (msg->val_int < 0) { //广播
        mesh.sendBroadcast(msg->buff->data);
      } else {
        mesh.sendSingle(msg->val_int, msg->buff->data);
      }
    }
    sys_buf_timeout_unlock(msg);
  }

  /*external loop*/
  do_loop_end();
}