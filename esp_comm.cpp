#include "esp_comm.h"

// יצירת אובייקט SoftwareSerial
SoftwareSerial espSerial(SOFT_RX_PIN, SOFT_TX_PIN);

// External references
extern bool pumpOn;
extern bool ForceneedStop;
void SaveSetting();

enum heat_ctrl_state_type {
  HC_STATE_SLEEP, HC_STATE_IDLE, HC_STATE_OPER_ON,
  HC_STATE_OPER_OFF, HC_STATE_OPER_FORCE_ON,
  HC_STATE_OPER_FORCE_OFF, HC_STATE_MENU, HC_STATE_SUBMENU
};

struct hc_oper_type {
  heat_ctrl_state_type hcSTATE;   
  heat_ctrl_state_type hc_prevSTATE;   
  uint16_t operIDX;
  uint32_t R1_ChangeTime[2];
  float tCOL; float tST; float tFLW;
};

struct hc_menu_items_span_type {
  char *str; float min; float max; float step; float curr;
};

struct hc_menu_type {
  uint16_t menuIDX;
  hc_menu_items_span_type sMAX; 
};

extern hc_oper_type HeatControlOper;
extern hc_menu_type HeatControlMenu;

void setupESP32Communication() {
  espSerial.begin(19200); // אתחול הערוץ הטורי מול ה-ESP32
}

void sendStatusToESP32() {
  espSerial.print("{\"tCOL\":"); espSerial.print(HeatControlOper.tCOL, 1);
  espSerial.print(",\"tST\":"); espSerial.print(HeatControlOper.tST, 1);
  espSerial.print(",\"tFLW\":"); espSerial.print(HeatControlOper.tFLW, 1);
  espSerial.print(",\"pump\":"); espSerial.print(pumpOn ? 1 : 0);
  espSerial.print(",\"state\":"); espSerial.print((int)HeatControlOper.hcSTATE);
  espSerial.println("}");
}

void handleESP32Communication() {
  static String inputBuffer = "";
  
  while (espSerial.available()) {
    char c = (char)espSerial.read();
    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        if (inputBuffer == "GET:STATUS") {
          sendStatusToESP32();
        } 
        else if (inputBuffer == "SET:MODE:AUTO") {
          HeatControlOper.hcSTATE = HC_STATE_IDLE;
          ForceneedStop = true;
        }
        else if (inputBuffer == "SET:MODE:MAN_ON") {
          HeatControlOper.hcSTATE = HC_STATE_OPER_FORCE_ON;
        }
        else if (inputBuffer == "SET:MODE:MAN_OFF") {
          HeatControlOper.hcSTATE = HC_STATE_OPER_FORCE_OFF;
        }
        else if (inputBuffer.startsWith("SET:CFG:sMAX:")) {
          HeatControlMenu.sMAX.curr = inputBuffer.substring(13).toFloat();
        }
        else if (inputBuffer == "CMD:SAVE") {
          SaveSetting();
        }
        inputBuffer = "";
      }
    } else {
      inputBuffer += c;
    }
  }
}