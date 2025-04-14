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
  if (sys_buf_timeout_invalid(shop_goods)) {
    if (shop_goods != NULL) {
      sys_buf_timeout_unlock(shop_goods);
    }

    //load and lock
    shop_goods = sys_buf_timeout_lock(file_load_text(file_goods));
  }

  return shop_goods;
}

//desc: ������д��mesh���ͻ���
void mesh_send(chart* mesh_data, int32_t mesh_id) {
  if (sys_buf_timeout_valid(mesh_data)) {
    if (mesh_send_buffer.isFull()) {
      chart* old_data = NULL;
      mesh_send_buffer.lockedPop(old_data);
      sys_buf_timeout_unlock(old_data);
    }

    mesh_data->val_int = mesh_id;
    if (!mesh_send_buffer.lockedPushOverwrite(mesh_data)) { //����mesh���Ͷ���
      sys_buf_timeout_unlock(mesh_data);
    }
  }
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

      charb* ptr = json_get(msg->data, "type");
      if (sys_buf_invalid(ptr)) { //��Ч��Ϣ����
        sys_buf_unlock(msg);
        return;
      }

      if (strcmp(ptr->data, action_chat) == 0) { //����
        sys_buf_unlock(ptr);
        server->textAll(msg->data); //���͸���������

        chart* mesh_data = sys_buf_timeout_lock(json_set(msg->data, "from", mesh_name)); //��Ϣβ��
        mesh_send(mesh_data, -1); //�㲥��mesh
      } else
      //--------------------------------------------------------------------------
      if (strcmp(ptr->data, action_search) == 0) { //��Ϣ����
        sys_buf_unlock(ptr);
        ptr = json_get(msg->data, "data");

        if (sys_buf_valid(ptr)) {
          bool is_empty = strlen(ptr->data) < 1;
          bool is_local = strstr(mesh_name, ptr->data) != NULL;
          sys_buf_unlock(ptr);

          if (is_empty || is_local) { //��������Ϊ�� �� ��������
            load_goods();
            if (sys_buf_timeout_valid(shop_goods)) {
              server->text(client->id(), shop_goods->buff->data);
            }
          }

          if (!is_empty) { //������mesh����
            ptr = json_set(msg->data, "id", int2str(client->id())); //���շ�id
            chart* mesh_data = sys_buf_timeout_lock(ptr);
            mesh_send(mesh_data, -1); //�㲥��mesh
          }
        }
      }

      //�ͷ���Ϣ
      sys_buf_unlock(msg);
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
  charb* ptr = json_get(msg.c_str(), "type");
  if (sys_buf_invalid(ptr)) { //��Ч��Ϣ����
    return;
  }

  if (strcmp(ptr->data, action_chat) == 0) { //����������
    sys_buf_unlock(ptr);
    wifi_fs_server.wsBroadcast(msg.c_str());
    return;
  }

  if (strcmp(ptr->data, action_search) == 0) { //��������
    sys_buf_unlock(ptr);
    ptr = json_get(msg.c_str(), "from");

    if (sys_buf_valid(ptr)) { //�������
      sys_buf_unlock(ptr);
      ptr = json_get(msg.c_str(), "id"); //ws.id

      if (sys_buf_valid(ptr)) {
        wifi_fs_server.getWebSocket()->text(atoi(ptr->data), msg.c_str()); //����ǰ��
        sys_buf_unlock(ptr);
      }
    } else { //��������
      if (strstr(mesh_name, ptr->data) != NULL) { //��������
        sys_buf_unlock(ptr);
        load_goods();

        if (sys_buf_timeout_valid(shop_goods)) {
          ptr = json_set(shop_goods->buff->data, "from", mesh_name); //Ӧ��ڵ�
          chart* mesh_data = sys_buf_timeout_lock(ptr);
          mesh_send(mesh_data, from); //������mesh
        }
      }
    }

    return;
  }
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
    if (sys_buf_timeout_valid(msg)) {
      if (msg->val_int < 0) { //�㲥
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