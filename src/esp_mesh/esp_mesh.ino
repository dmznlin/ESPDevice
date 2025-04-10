/********************************************************************************
  ����: dmzn@163.com 2025-04-03
  ����: ���ڻ�����ܵ�Meshϵͳ
********************************************************************************/
#include "esp_define.h"
#include "esp_module.h"
#include "esp_znlib.h"

#ifdef mesh_enabled
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
      */

      charb* ptr = json_get(msg->data, "type");
      if (sys_buf_invalid(ptr)) { //��Ч��Ϣ����
        sys_buf_unlock(msg);
        return;
      }

      if (strcmp(ptr->data, "chat") == 0) { //����
        sys_buf_unlock(ptr);
        server->textAll(msg->data); //���͸���������

        ptr = json_set(msg->data, "from", mesh_name); //��Ϣβ��
        if (sys_buf_valid(ptr)) {
          chart* mesh_data = sys_buf_timeout_lock(strlen(ptr->data) + 1);
          if (sys_buf_timeout_valid(mesh_data)) {
            if (mesh_send_buffer.isFull()) {
              chart* old_data = NULL;
              mesh_send_buffer.lockedPop(old_data);
              sys_buf_timeout_unlock(old_data);
            }

            strcpy(mesh_data->buff->data, ptr->data);
            if (!mesh_send_buffer.lockedPushOverwrite(mesh_data)) { //���뷢�Ͷ���
              sys_buf_timeout_unlock(mesh_data);
            }
          }

          sys_buf_unlock(ptr);
        }
      } else if (strcmp(ptr->data, "search") == 0) { //��Ϣ����
        sys_buf_unlock(ptr);
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
*/
void mesh_do_receive(uint32_t from, TSTRING& msg) {
  charb* ptr = json_get(msg.c_str(), "type");
  if (sys_buf_invalid(ptr)) { //��Ч��Ϣ����
    return;
  }

  if (strcmp(ptr->data, "chat") == 0) { //����������
    wifi_fs_server.wsBroadcast(msg.c_str());
  }

  sys_buf_unlock(ptr);
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
      mesh.sendBroadcast(msg->buff->data);
    }
    sys_buf_timeout_unlock(msg);
  }

  /*external loop*/
  do_loop_end();
}