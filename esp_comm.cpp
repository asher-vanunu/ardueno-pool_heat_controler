#include "esp_comm.h"

// יצירת אובייקט SoftwareSerial
SoftwareSerial espSerial(SOFT_RX_PIN, SOFT_TX_PIN);

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
  hc_menu_items_span_type tDEL;
  hc_menu_items_span_type tON;
  hc_menu_items_span_type tOFF;
  hc_menu_items_span_type DT_O;
  hc_menu_items_span_type DT_F;
};

extern hc_oper_type HeatControlOper;
extern hc_menu_type HeatControlMenu;

void setupESP32Communication() {
  espSerial.begin(19200);
}

void sendStatusToESP32() {
  espSerial.print("{\"tCOL\":"); espSerial.print(HeatControlOper.tCOL, 1);
  espSerial.print(",\"tST\":"); espSerial.print(HeatControlOper.tST, 1);
  espSerial.print(",\"tFLW\":"); espSerial.print(HeatControlOper.tFLW, 1);
  espSerial.print(",\"pump\":"); espSerial.print(pumpOn ? 1 : 0);
  espSerial.print(",\"state\":"); espSerial.print((int)HeatControlOper.hcSTATE);
  
  // שליחת ערכי הקונפיגורציה הנוכחיים ב-JSON
  espSerial.print(",\"sMAX\":"); espSerial.print(HeatControlMenu.sMAX.curr, 1);
  espSerial.print(",\"tDEL\":"); espSerial.print(HeatControlMenu.tDEL.curr, 1);
  espSerial.print(",\"tON\":");  espSerial.print(HeatControlMenu.tON.curr, 1);
  espSerial.print(",\"tOFF\":"); espSerial.print(HeatControlMenu.tOFF.curr, 1);
  espSerial.print(",\"DT_O\":"); espSerial.print(HeatControlMenu.DT_O.curr, 1);
  espSerial.print(",\"DT_F\":"); espSerial.print(HeatControlMenu.DT_F.curr, 1);
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
        // עדכון הפרמטרים מתוך ה-ESP32
        else if (inputBuffer.startsWith("SET:CFG:sMAX:")) {
          HeatControlMenu.sMAX.curr = inputBuffer.substring(13).toFloat();
        }
        else if (inputBuffer.startsWith("SET:CFG:tDEL:")) {
          HeatControlMenu.tDEL.curr = inputBuffer.substring(13).toFloat();
        }
        else if (inputBuffer.startsWith("SET:CFG:tON:")) {
          HeatControlMenu.tON.curr = inputBuffer.substring(12).toFloat();
        }
        else if (inputBuffer.startsWith("SET:CFG:tOFF:")) {
          HeatControlMenu.tOFF.curr = inputBuffer.substring(13).toFloat();
        }
        else if (inputBuffer.startsWith("SET:CFG:DT_O:")) {
          HeatControlMenu.DT_O.curr = inputBuffer.substring(13).toFloat();
        }
        else if (inputBuffer.startsWith("SET:CFG:DT_F:")) {
          HeatControlMenu.DT_F.curr = inputBuffer.substring(13).toFloat();
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