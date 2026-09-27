#include "esp_comm.h"

#ifndef USE_HARDWARE_SERIAL
// יצירת אובייקט SoftwareSerial רק אם לא נבחר Hardware Serial
SoftwareSerial espSerial(SOFT_RX_PIN, SOFT_TX_PIN);
#endif

extern bool pumpOn;
extern bool ForceneedStop;
void SaveSetting();
void ResetToDefaultSettings();

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
  COMM_PORT.begin(19200);
  COMM_PORT.println("Serial is ready/n/r");
}

void sendStatusToESP32() {
  COMM_PORT.print("{\"tCOL\":"); COMM_PORT.print(HeatControlOper.tCOL, 1);
  COMM_PORT.print(",\"tST\":"); COMM_PORT.print(HeatControlOper.tST, 1);
  COMM_PORT.print(",\"tFLW\":"); COMM_PORT.print(HeatControlOper.tFLW, 1);
  COMM_PORT.print(",\"pump\":"); COMM_PORT.print(pumpOn ? 1 : 0);
  COMM_PORT.print(",\"state\":"); COMM_PORT.print((int)HeatControlOper.hcSTATE);
  
  // שליחת ערכי הקונפיגורציה הנוכחיים ב-JSON
  COMM_PORT.print(",\"sMAX\":"); COMM_PORT.print(HeatControlMenu.sMAX.curr, 1);
  COMM_PORT.print(",\"tDEL\":"); COMM_PORT.print(HeatControlMenu.tDEL.curr, 1);
  COMM_PORT.print(",\"tON\":");  COMM_PORT.print(HeatControlMenu.tON.curr, 1);
  COMM_PORT.print(",\"tOFF\":"); COMM_PORT.print(HeatControlMenu.tOFF.curr, 1);
  COMM_PORT.print(",\"DT_O\":"); COMM_PORT.print(HeatControlMenu.DT_O.curr, 1);
  COMM_PORT.print(",\"DT_F\":"); COMM_PORT.print(HeatControlMenu.DT_F.curr, 1);
  COMM_PORT.println("}");
}

void handleESP32Communication() {
  static String inputBuffer = "";
  
  while (COMM_PORT.available()) {
    char c = (char)COMM_PORT.read();
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
        else if (inputBuffer == "CMD:DEFAULT") {
          ResetToDefaultSettings();
        }
        inputBuffer = "";
      }
    } else {
      inputBuffer += c;
    }
  }
}