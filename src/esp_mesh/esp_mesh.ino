/********************************************************************************
  ����: dmzn@163.com 2025-04-03
  ����: ���ڻ�����ܵ�Meshϵͳ
********************************************************************************/
#include "esp_define.h"
#include "esp_module.h"
#include "esp_znlib.h"

#ifdef mesh_enabled
void sendMessage(); // Prototype so PlatformIO doesn't complain
Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage);

void sendMessage() {
  String msg = "Hello from node ";
  msg += mesh.getNodeId();
  mesh.sendBroadcast(msg);
  taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5));
}

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
    if (info->opcode == WS_TEXT) {
      charb* msg = sys_buf_lock(len + 1, true);
      if (sys_buf_valid(msg)) {
        memcpy(msg->data, data, len);
        msg->data[len] = '\0';

        /*
          websocketЭ��:
          1.��ʽ: {type: chat/search/control/message, data:xxx}
          2.����: chat,����;search,����;control,����;message,��Ϣ
          3.����:
        */
        char* tag = strstr(msg->data, "data");
        server->textAll(msg->data);
      }
      sys_buf_unlock(msg);
    }
  }
  break;
  default:
    break;
  }
}

void mesh_do_receive(uint32_t from, TSTRING& msg) {
  showlog(msg);
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
    //task_scheduler.addTask(taskSendMessage);
    //taskSendMessage.enable();
  #endif

  /*external setup*/
  do_setup_end();
}

void loop() {
  /*external loop*/
  //if (!do_loop_begin()) return;
  do_loop_begin();

  /*�����￪ʼд��Ĵ���*/

  /*external loop*/
  do_loop_end();
}