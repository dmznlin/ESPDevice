/********************************************************************************
  作者: dmzn@163.com 2025-04-03
  描述: 基于基础框架的Mesh系统
********************************************************************************/
#include "esp_define.h"
#include "esp_module.h"
#include "esp_znlib.h"

const char* action_file = "file"; //文件传输
const char* action_chat = "chat"; //聊天
const char* action_search = "search"; //检索

const char* shop_all = "\xE9\x99\x84\xE8\xBF\x91"; //附近: 查询门店列表

const char* file_map = "/web/map.txt"; //导航信息
const char* file_goods = "/web/goods.txt"; //商品信息文件

chart* shop_goods = NULL; //商品信息列表

byte mesh_data_pack_serial = 0; //数据分包序列号

//-------------------------------------------------------------------------------
#ifdef mesh_enabled

//desc: 载入商品列表
chart* load_goods() {
  if (!sys_buf_timeout_valid(shop_goods, true)) {
    if (shop_goods != NULL) {
      sys_buf_timeout_unlock(shop_goods);
    }

    //load and lock
    shop_goods = sys_buf_timeout_lock((charb*)file_load_text(file_goods, true));
    if (sys_buf_timeout_invalid(shop_goods)) {
      showlog("load_goods: failure.");
    }
  }

  return shop_goods;
}

//desc: 载入地图数据
charb* load_map(const char* from, const char* ws_id) {
  const char* init = "{\"type\":\"file\",\"from\":\"%s\",\"id\":\"%s\",\"data\":\"";
  int size = snprintf(NULL, 0, init, from, ws_id);
  if (size < 1) return NULL;

  charb* pre = sys_buf_lock(size + 1, true);
  if (sys_buf_invalid(pre)) return NULL;
  snprintf(pre->data, size + 1, init, from, ws_id);

  charb* map = (charb*)file_load_text(file_map, true, pre->data, "\"}");
  sys_buf_unlock(pre);

  if (sys_buf_invalid(map)) {
    showlog("load_map: failure.");
  }
  return map;
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

  if (strcmp(search, shop_all) != 0) { //正常检索商品
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
        1.格式: {type: chat/search/control/file, data:xxx}
        2.类型: chat,聊天;search,检索;control,控制;file,文件传输
        3.数据: 具体数据
          *.chat:{type: chat, from: mesh_id, data: message}
          *.search:{type: search, id: client, from: mesh_id, data: goods or shop}
          *.file:{type: file, from/shop: mesh_id, id: client, data: file/filetype}
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
        mesh_send(mesh_data); //广播至mesh

        sys_buf_unlock(msg);
        return;
      }

      //--------------------------------------------------------------------------
      if (strcmp(msg_type->data, action_search) == 0) { //信息检索
        sys_buf_unlock(msg_type);
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
            mesh_send(mesh_data); //广播至mesh
          }

          sys_buf_unlock(search);
        }

        sys_buf_unlock(msg);
        return;
      }

      //--------------------------------------------------------------------------
      if (strcmp(msg_type->data, action_file) == 0) { //文件传输
        sys_buf_unlock(msg_type);

        uint32_t mesh_id = 0;
        charb* data = NULL;
        charb* shop = json_get(msg->data, "shop"); //mesh id

        if (sys_buf_valid(shop)) {
          mesh_id = strtoul(shop->data, NULL, 10);
          sys_buf_unlock(&shop);
          data = json_get(msg->data, "data"); //data
        }

        if (sys_buf_valid(data)) {
          if (mesh_id == 0) { //本节点
            if (strcmp(data->data, "map") == 0) { //读取地图
              sys_buf_unlock(&data);
              charb* map = load_map("0", "0");

              if (sys_buf_valid(map)) {
                server->text(client->id(), map->data);
                sys_buf_unlock(map);
              }
            }
          } else { //mesh节点
            charb* ptr = json_set(msg->data, "id", int2str(client->id())); //接收方id
            chart* mesh_data = sys_buf_timeout_lock(ptr);
            mesh_send(mesh_data, mesh_id); //广播至mesh
          }
        }

        sys_buf_unlock(data);
        sys_buf_unlock(shop);
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
    *.chat:{type: chat, from: mesh_id, data: message}
    *.search:{type: search, id: client, from: mesh_id, data: goods or shop}
    *.file:{type: file, from/shop: mesh_id, id: client, data: file/filetype}
*/
void mesh_do_receive(uint32_t from, TSTRING& msg) {
  charb* msg_type = sys_buf_lock(15, true);
  if (sys_buf_invalid(msg_type)) return;

  strncpy(msg_type->data, msg.c_str(), 15);
  msg_type->data[15] = '\0';

  //-----------------------------------------------------------------------------
  if (strstr(msg_type->data, "id=") != NULL) { //mesh分包数据: id=1;idx=2
    charb* id = NULL;
    charb* idx = NULL;
    char* tag = strchr(msg_type->data, ' '); //定位空格

    if (tag != NULL) id = get_kv_val(msg_type->data, "id");
    if (sys_buf_valid(id)) idx = get_kv_val(msg_type->data, "idx");

    if (sys_buf_valid(idx)) {
      charb* full = mesh_recv_peer(msg.c_str() + (tag - msg_type->data) + 1,
        from, atoi(id->data), atoi(idx->data));
      //缓存 或 组装

      sys_buf_unlock(&id);
      sys_buf_unlock(&idx);
      sys_buf_unlock(&msg_type);

      if (sys_buf_valid(full)) {
        id = json_get(full->data, "id"); //ws.id
        if (sys_buf_valid(id)) {
          wifi_fs_server.getWebSocket()->text(atoi(id->data), full->data);
        }
        sys_buf_unlock(full);
      }
    }

    sys_buf_unlock(id);
    sys_buf_unlock(idx);
    sys_buf_unlock(msg_type);
    return;
  }

  //-----------------------------------------------------------------------------
  sys_buf_unlock(msg_type);
  msg_type = json_get(msg.c_str(), "type");
  if (sys_buf_invalid(msg_type)) return; //无效消息类型

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

    charb* id = NULL;
    charb* node = NULL;
    if (strstr(mesh_name, search->data) != NULL) { //包含店名
      sys_buf_unlock(search);

      load_goods();
      if (sys_buf_timeout_valid(shop_goods)) id = json_get(msg.c_str(), "id"); //ws.id
      if (sys_buf_valid(id)) node = int2str(mesh.getNodeId()); //mesh.nodeId

      if (sys_buf_valid(node)) {
        sys_data_kv items[] = {{"from", node->data}, {"id", id->data}};
        charb* goods = json_multiset(shop_goods->buff->data, items, 2); //节点应答

        sys_buf_unlock(&id);
        sys_buf_unlock(&node);

        chart* mesh_data = sys_buf_timeout_lock(goods, sys_buffer_huge_timeout);
        mesh_send(mesh_data, from); //发送至mesh
      }

      sys_buf_unlock(id);
      sys_buf_unlock(node);
      return;
    }

    //生成检索结果
    charb* goods = search_goods(search->data);
    sys_buf_unlock(search);

    if (sys_buf_valid(goods)) id = json_get(msg.c_str(), "id"); //ws.id
    if (sys_buf_valid(id)) node = int2str(mesh.getNodeId()); //mesh.nodeId

    if (sys_buf_valid(node)) {
      sys_data_kv items[] = { {"from", node->data}, {"id", id->data} };
      ptr = json_multiset(goods->data, items, 2); //节点应答

      sys_buf_unlock(&id);
      sys_buf_unlock(&node);
      sys_buf_unlock(&goods);

      chart* mesh_data = sys_buf_timeout_lock(ptr, sys_buffer_huge_timeout);
      mesh_send(mesh_data, from); //发送至mesh
    }

    sys_buf_unlock(id);
    sys_buf_unlock(node);
    sys_buf_unlock(goods);
    return;
  }

  //-----------------------------------------------------------------------------
  if (strcmp(msg_type->data, action_file) == 0) { //传输文件
    sys_buf_unlock(msg_type);
    charb* ptr = json_get(msg.c_str(), "from");

    if (sys_buf_valid(ptr)) { //mesh返回数据
      sys_buf_unlock(ptr);
      ptr = json_get(msg.c_str(), "id"); //ws.id

      if (sys_buf_valid(ptr)) {
        wifi_fs_server.getWebSocket()->text(atoi(ptr->data), msg.c_str()); //发给前端
        sys_buf_unlock(ptr);
      }

      return;
    }

    charb* id = NULL;
    charb* node = NULL;
    charb* data = json_get(msg.c_str(), "data"); //data
    if (sys_buf_valid(data)) id = json_get(msg.c_str(), "id"); //ws.id
    if (sys_buf_valid(id)) node = int2str(mesh.getNodeId()); //mesh.nodeId

    if (sys_buf_valid(node)) {
      if (strcmp(data->data, "map") == 0) { //读取地图
        sys_buf_unlock(&data);

        chart* map = sys_buf_timeout_lock(load_map(node->data, id->data), sys_buffer_huge_timeout);
        mesh_send(map, from); //发送至mesh
      }
    }

    sys_buf_unlock(id);
    sys_buf_unlock(node);
    sys_buf_unlock(data);
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
      if (msg->val_uint < 1) { //广播
        if (msg->buff->len <= sys_buffer_huge) {
          mesh.sendBroadcast(msg->buff->data, msg->val_bool);
        } else {
          showlog("mesh.sendBroadcast: data too huge");
        }
      } else {
        if (msg->buff->len <= sys_buffer_huge) {
          mesh.sendSingle(msg->val_uint, msg->buff->data);
        } else {
          mesh_data_pack_serial++;
          if (mesh_data_pack_serial < 1) mesh_data_pack_serial = 1;
          mesh_send_peer(msg->buff, msg->val_uint, mesh_data_pack_serial);
        }
      }
    }
    sys_buf_timeout_unlock(msg);
  }

  /*external loop*/
  do_loop_end();
}