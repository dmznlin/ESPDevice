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
  if (!sys_buf_timeout_valid(shop_goods, true)) {
    if (shop_goods != NULL) {
      sys_buf_timeout_unlock(shop_goods);
    }

    //load and lock
    shop_goods = sys_buf_timeout_lock(file_load_text(file_goods));
    if (sys_buf_timeout_invalid(shop_goods)) {
        showlog("load_goods: failure.");
    }
  }

  return shop_goods;
}

//desc: 将数据写入mesh发送缓冲
void mesh_send(chart* mesh_data, int32_t mesh_id, bool self = false) {
  if (sys_buf_timeout_valid(mesh_data)) {
    if (mesh_send_buffer.isFull()) {
      chart* old_data = NULL;
      mesh_send_buffer.lockedPop(old_data);
      sys_buf_timeout_unlock(old_data);
    }

    mesh_data->val_int = mesh_id;
    mesh_data->val_bool = self;
    if (!mesh_send_buffer.lockedPushOverwrite(mesh_data)) { //进入mesh发送队列
      sys_buf_timeout_unlock(mesh_data);
    }
  }
}

/*
  date: 2025-04-14 15:24:08
  parm: 检索内容
  desc: 在商品列表中检索search商品

  备注: 商品表结构
{
  type: search,
  shop: 蜜雪冰城,
  addr: 二七广场亚细亚负一楼,
  data: 2,

  a1: 1,
  b1: 鲜榨柠檬,
  c1: 4.00,
  d1: 夏日救星,

  a2: 2,
  b2: 新品甜筒,
  c2: 2.00,
  d2: 镇店之宝....
}
*/
charb* search_goods(const char* search) {
  load_goods();
  if (sys_buf_timeout_invalid(shop_goods)) {
    return NULL;
  }

  charb* ptr = json_get(shop_goods->buff->data, "data"); //商品数
  if (sys_buf_invalid(ptr)) {
    return NULL;
  }

  int num = atoi(ptr->data);
  sys_buf_unlock(ptr);
  if (num < 1 || num > 255) return NULL; //没有商品

  const char* init = "{\"type\": \"search\"}"; //数据类型
  charb* ret = sys_buf_lock(strlen(init) + 1, true);
  if (sys_buf_invalid(ptr)) {
    return NULL;
  }
  strcpy(ret->data, init); //init

  charb* tmp;
  char good_item[5];
  char good_name[5];
  uint8_t good_idx = 0;

  for (uint8_t i = 1;i <= num;i++) {
    good_item[0] = 'b';
    snprintf(&good_item[1], 4, "%d", i);

    ptr = json_get(shop_goods->buff->data, good_item); //商品名
    if (sys_buf_invalid(ptr)) continue;

    if (strstr(ptr->data, search) == NULL) { //匹配
      sys_buf_unlock(ptr);
      continue;
    }

    good_idx++;
    good_name[0] = 'b';
    snprintf(&good_name[1], 4, "%d", good_idx);

    tmp = json_set(ret->data, good_name, ptr->data); //b1
    sys_buf_unlock(ptr);
    if (sys_buf_valid(tmp)) {
      sys_buf_unlock(ret);
      ret = tmp;
    }

    good_name[0] = 'a';
    tmp = json_set(ret->data, good_name, &good_name[1]); //a1
    if (sys_buf_valid(tmp)) {
      sys_buf_unlock(ret);
      ret = tmp;
    }

    for (uint8_t j = 2;j < 4;j++) { //a1,b1,c1,d1
      good_item[0] = 'a' + j; //c,d
      good_name[0] = 'a' + j;

      ptr = json_get(shop_goods->buff->data, good_item);
      if (sys_buf_valid(ptr)) {
        tmp = json_set(ret->data, good_name, ptr->data); //c1,d1
        sys_buf_unlock(ptr);

        if (sys_buf_valid(tmp)) {
          sys_buf_unlock(ret);
          ret = tmp;
        }
      }
    }
  }

  if (good_idx == 0) { //未匹配任何商品
    sys_buf_unlock(ret);
    return NULL;
  }

  snprintf(good_item, 5, "%d", good_idx);
  tmp = json_set(ret->data, "data", good_item); //+data
  if (sys_buf_valid(tmp)) {
    sys_buf_unlock(ret);
    ret = tmp;
  }

  ptr = json_get(shop_goods->buff->data, "shop");
  if (sys_buf_valid(ptr)) {
    tmp = json_set(ret->data, "shop", ptr->data); //+shop
    sys_buf_unlock(ptr);

    if (sys_buf_valid(tmp)) {
      sys_buf_unlock(ret);
      ret = tmp;
    }
  }

  ptr = json_get(shop_goods->buff->data, "addr");
  if (sys_buf_valid(ptr)) {
    tmp = json_set(ret->data, "addr", ptr->data); //+addr
    sys_buf_unlock(ptr);

    if (sys_buf_valid(tmp)) {
      sys_buf_unlock(ret);
      ret = tmp;
    }
  }

  return ret;
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

      charb* msg_type = json_get(msg->data, "type");
      if (sys_buf_invalid(msg_type)) { //无效消息类型
        sys_buf_unlock(msg);
        return;
      }

      if (strcmp(msg_type->data, action_chat) == 0) { //聊天
        sys_buf_unlock(msg_type);
        server->textAll(msg->data); //发送给本服务器

        chart* mesh_data = sys_buf_timeout_lock(json_set(msg->data, "from", mesh_name)); //消息尾巴
        mesh_send(mesh_data, -1); //广播至mesh

        sys_buf_unlock(msg);
        return;
      }

      //--------------------------------------------------------------------------
      if (strcmp(msg_type->data, action_search) == 0) { //信息检索
        sys_buf_unlock(&msg_type);
        charb* search = json_get(msg->data, "data"); //待查找内容

        if (sys_buf_valid(search)) {
          bool is_empty = strlen(search->data) < 1;
          bool is_local = strstr(mesh_name, search->data) != NULL;

          if (is_empty || is_local) { //检索内容为空 或 包含店名
            load_goods();
            if (sys_buf_timeout_valid(shop_goods)) {
              server->text(client->id(), shop_goods->buff->data);
            }
          }

          if (!is_empty) { //发送至mesh网络
            if (!is_local) {
              charb* goods = search_goods(search->data);
              if (sys_buf_valid(goods)) {
                server->text(client->id(), goods->data);
                sys_buf_unlock(goods);
              }
            }

            charb* ptr = json_set(msg->data, "id", int2str(client->id())); //接收方id
            chart* mesh_data = sys_buf_timeout_lock(ptr);
            mesh_send(mesh_data, -1); //广播至mesh
          }

          sys_buf_unlock(search);
        }

        sys_buf_unlock(msg);
        return;
      }

      sys_buf_unlock(msg);
      sys_buf_unlock(msg_type);
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
  charb* msg_type = json_get(msg.c_str(), "type");
  if (sys_buf_invalid(msg_type)) { //无效消息类型
    return;
  }

  if (strcmp(msg_type->data, action_chat) == 0) { //聊天室数据
    sys_buf_unlock(msg_type);
    wifi_fs_server.wsBroadcast(msg.c_str());
    return;
  }

  //-----------------------------------------------------------------------------
  if (strcmp(msg_type->data, action_search) == 0) { //检索数据
    sys_buf_unlock(&msg_type);

    charb* ptr;
    ptr = json_get(msg.c_str(), "from");

    if (sys_buf_valid(ptr)) { //检索结果
      sys_buf_unlock(ptr);
      ptr = json_get(msg.c_str(), "id"); //ws.id

      if (sys_buf_valid(ptr)) {
        wifi_fs_server.getWebSocket()->text(atoi(ptr->data), msg.c_str()); //发给前端
        sys_buf_unlock(ptr);
      }

      return;
    }

    //---------------------------------------------------------------------------
    charb* search = json_get(msg.c_str(), "data");  //检索数据
    if (sys_buf_invalid(search)) return;

    if (strstr(mesh_name, search->data) != NULL) { //包含店名
      sys_buf_unlock(search);

      load_goods();
      if (sys_buf_timeout_valid(shop_goods)) {
        charb* id = json_get(msg.c_str(), "id"); //ws.id
        if (sys_buf_invalid(id)) return;

        charb* node = int2str(mesh.getNodeId());
        if (sys_buf_invalid(node)) {
          sys_buf_unlock(id);
          return;
        }

        sys_data_kv items[] = {{"from", node->data}, {"id", id->data}};
        charb* goods = json_multiset(shop_goods->buff->data, items, 2); //节点应答
        sys_buf_unlock(id);
        sys_buf_unlock(node);

        chart* mesh_data = sys_buf_timeout_lock(goods);
        mesh_send(mesh_data, from); //发送至mesh
      }

      return;
    }

    //生成检索结果
    charb* goods = search_goods(search->data);
    sys_buf_unlock(search);

    if (sys_buf_valid(goods)) {
      charb* id = json_get(msg.c_str(), "id"); //ws.id
      if (sys_buf_invalid(id)) {
        sys_buf_unlock(goods);
        return;
      }

      charb* node = int2str(mesh.getNodeId());
      if (sys_buf_invalid(node)) {
        sys_buf_unlock(id);
        sys_buf_unlock(goods);
        return;
      }

      sys_data_kv items[] = { {"from", node->data}, {"id", id->data} };
      ptr = json_multiset(goods->data, items, 2); //节点应答

      sys_buf_unlock(id);
      sys_buf_unlock(node);
      sys_buf_unlock(goods);

      chart* mesh_data = sys_buf_timeout_lock(ptr);
      mesh_send(mesh_data, from); //发送至mesh
    }
    return;
  }

  sys_buf_unlock(msg_type);
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
    if (sys_buf_timeout_valid(msg, true)) {
      if (msg->val_int < 0) { //广播
        mesh.sendBroadcast(msg->buff->data, msg->val_bool);
      } else {
        mesh.sendSingle(msg->val_int, msg->buff->data);
      }
    }
    sys_buf_timeout_unlock(msg);
  }

  /*external loop*/
  do_loop_end();
}