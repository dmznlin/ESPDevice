/********************************************************************************
  ����: dmzn@163.com 2025-04-03
  ����: ���ڻ�����ܵ�Meshϵͳ
********************************************************************************/
#include "esp_define.h"
#include "esp_module.h"
#include "esp_znlib.h"

const char* action_chat = "chat"; //����
const char* action_search = "search"; //����
const char* file_goods = "/web/goods.txt"; //��Ʒ��Ϣ�ļ�

chart* shop_goods = NULL; //��Ʒ��Ϣ�б�

//-------------------------------------------------------------------------------
#ifdef mesh_enabled

//desc: ������Ʒ�б�
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

//desc: ������д��mesh���ͻ���
void mesh_send(chart* mesh_data, int32_t mesh_id, bool self = false) {
  if (sys_buf_timeout_valid(mesh_data)) {
    if (mesh_send_buffer.isFull()) {
      chart* old_data = NULL;
      mesh_send_buffer.lockedPop(old_data);
      sys_buf_timeout_unlock(old_data);
    }

    mesh_data->val_int = mesh_id;
    mesh_data->val_bool = self;
    if (!mesh_send_buffer.lockedPushOverwrite(mesh_data)) { //����mesh���Ͷ���
      sys_buf_timeout_unlock(mesh_data);
    }
  }
}

/*
  date: 2025-04-14 15:24:08
  parm: ��������
  desc: ����Ʒ�б��м���search��Ʒ

  ��ע: ��Ʒ��ṹ
{
  type: search,
  shop: ��ѩ����,
  addr: ���߹㳡��ϸ�Ǹ�һ¥,
  data: 2,

  a1: 1,
  b1: ��ե����,
  c1: 4.00,
  d1: ���վ���,

  a2: 2,
  b2: ��Ʒ��Ͳ,
  c2: 2.00,
  d2: ���֮��....
}
*/
charb* search_goods(const char* search) {
  load_goods();
  if (sys_buf_timeout_invalid(shop_goods)) {
    return NULL;
  }

  charb* ptr = json_get(shop_goods->buff->data, "data"); //��Ʒ��
  if (sys_buf_invalid(ptr)) {
    return NULL;
  }

  int num = atoi(ptr->data);
  sys_buf_unlock(ptr);
  if (num < 1 || num > 255) return NULL; //û����Ʒ

  const char* init = "{\"type\": \"search\"}"; //��������
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

    ptr = json_get(shop_goods->buff->data, good_item); //��Ʒ��
    if (sys_buf_invalid(ptr)) continue;

    if (strstr(ptr->data, search) == NULL) { //ƥ��
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

  if (good_idx == 0) { //δƥ���κ���Ʒ
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

//desc: ����websocket��Ϣ
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
      if (info->opcode != WS_TEXT) return; //ֻ�����ı���Ϣ

      charb* msg = sys_buf_lock(len + 1, true);
      if (sys_buf_invalid(msg)) return; //�ڴ治��

      memcpy(msg->data, data, len);
      msg->data[len] = '\0';

      /*
        websocketЭ��:
        1.��ʽ: {type: chat/search/control/message, data:xxx}
        2.����: chat,����;search,����;control,����;message,��Ϣ
        3.����: ��������
          *.chat:{type: chat, from: mesh_name, data: message}
          *.search:{type: search, id: client, data: goods or shop}
      */

      charb* msg_type = json_get(msg->data, "type");
      if (sys_buf_invalid(msg_type)) { //��Ч��Ϣ����
        sys_buf_unlock(msg);
        return;
      }

      if (strcmp(msg_type->data, action_chat) == 0) { //����
        sys_buf_unlock(msg_type);
        server->textAll(msg->data); //���͸���������

        chart* mesh_data = sys_buf_timeout_lock(json_set(msg->data, "from", mesh_name)); //��Ϣβ��
        mesh_send(mesh_data, -1); //�㲥��mesh

        sys_buf_unlock(msg);
        return;
      }

      //--------------------------------------------------------------------------
      if (strcmp(msg_type->data, action_search) == 0) { //��Ϣ����
        sys_buf_unlock(&msg_type);
        charb* search = json_get(msg->data, "data"); //����������

        if (sys_buf_valid(search)) {
          bool is_empty = strlen(search->data) < 1;
          bool is_local = strstr(mesh_name, search->data) != NULL;

          if (is_empty || is_local) { //��������Ϊ�� �� ��������
            load_goods();
            if (sys_buf_timeout_valid(shop_goods)) {
              server->text(client->id(), shop_goods->buff->data);
            }
          }

          if (!is_empty) { //������mesh����
            if (!is_local) {
              charb* goods = search_goods(search->data);
              if (sys_buf_valid(goods)) {
                server->text(client->id(), goods->data);
                sys_buf_unlock(goods);
              }
            }

            charb* ptr = json_set(msg->data, "id", int2str(client->id())); //���շ�id
            chart* mesh_data = sys_buf_timeout_lock(ptr);
            mesh_send(mesh_data, -1); //�㲥��mesh
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
  desc: ����mesh������Ϣ

  ��Ϣ��ʽ:
  1.��ʽ: {type: chat/search/control/message, data:xxx}
  2.����: chat,����;search,����;control,����;message,��Ϣ
  3.����: ��������
    *.chat:{type: chat, from: mesh_name, data: message}
    *.search:{type: search, from: mesh_name, id: client, data: goods or shop}
*/
void mesh_do_receive(uint32_t from, TSTRING& msg) {
  charb* msg_type = json_get(msg.c_str(), "type");
  if (sys_buf_invalid(msg_type)) { //��Ч��Ϣ����
    return;
  }

  if (strcmp(msg_type->data, action_chat) == 0) { //����������
    sys_buf_unlock(msg_type);
    wifi_fs_server.wsBroadcast(msg.c_str());
    return;
  }

  //-----------------------------------------------------------------------------
  if (strcmp(msg_type->data, action_search) == 0) { //��������
    sys_buf_unlock(&msg_type);

    charb* ptr;
    ptr = json_get(msg.c_str(), "from");

    if (sys_buf_valid(ptr)) { //�������
      sys_buf_unlock(ptr);
      ptr = json_get(msg.c_str(), "id"); //ws.id

      if (sys_buf_valid(ptr)) {
        wifi_fs_server.getWebSocket()->text(atoi(ptr->data), msg.c_str()); //����ǰ��
        sys_buf_unlock(ptr);
      }

      return;
    }

    //---------------------------------------------------------------------------
    charb* search = json_get(msg.c_str(), "data");  //��������
    if (sys_buf_invalid(search)) return;

    if (strstr(mesh_name, search->data) != NULL) { //��������
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
        charb* goods = json_multiset(shop_goods->buff->data, items, 2); //�ڵ�Ӧ��
        sys_buf_unlock(id);
        sys_buf_unlock(node);

        chart* mesh_data = sys_buf_timeout_lock(goods);
        mesh_send(mesh_data, from); //������mesh
      }

      return;
    }

    //���ɼ������
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
      ptr = json_multiset(goods->data, items, 2); //�ڵ�Ӧ��

      sys_buf_unlock(id);
      sys_buf_unlock(node);
      sys_buf_unlock(goods);

      chart* mesh_data = sys_buf_timeout_lock(ptr);
      mesh_send(mesh_data, from); //������mesh
    }
    return;
  }

  sys_buf_unlock(msg_type);
}
#endif

//������-------------------------------------------------------------------------
void setup() {
  /* Initialize serial and wait for port to open: */
  Serial.begin(115200);

  /* This delay gives the chance to wait for a Serial Monitor without blocking if none is found */
  delay(1500);

  /*external setup*/
  if (!do_setup_begin()) return;

  /*�����￪ʼд��Ĵ���*/
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

  /*�����￪ʼд��Ĵ���*/
  chart* msg;
  while (mesh_send_buffer.lockedPop(msg)) {
    if (sys_buf_timeout_valid(msg, true)) {
      if (msg->val_int < 0) { //�㲥
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