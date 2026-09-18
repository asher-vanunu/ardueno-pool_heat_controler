#include "esp_comm.h"

// External references to original global variables and functions
extern bool pumpOn;
extern bool ForceneedStop;
void SaveSetting();

// External definitions matching the original layout
enum heat_ctrl_state_type {
  HC_STATE_SLEEP,
  HC_STATE_IDLE,
  HC_STATE_OPER_ON,
  HC_STATE_OPER_OFF,
  HC_STATE_OPER_FORCE_ON,
  HC_STATE_OPER_FORCE_OFF,
  HC_STATE_MENU,
  HC_STATE_SUBMENU
};

struct hc_oper_type {
  heat_ctrl_state_type hcSTATE;   
  heat_ctrl_state_type hc_prevSTATE;   
  uint16_t operIDX;
  uint32_t R1_ChangeTime[2];
  float tCOL;
  float tST;
  float tFLW;
};

struct hc_menu_items_span_type {
  char *str;
  float min;
  float max;
  float step;
  float curr;
};

struct hc_menu_type {
  uint16_t menuIDX;
  hc_menu_items_span_type sMAX; 
};

extern hc_oper_type HeatControlOper;
extern hc_menu_type HeatControlMenu;

// Formats telemetry data as JSON and sends over serial interface
void sendStatusToESP32() {
  Serial.print("{\"tCOL\":"); Serial.print(HeatControlOper.tCOL, 1);
  Serial.print(",\"tST\":"); Serial.print(HeatControlOper.tST, 1);
  Serial.print(",\"tFLW\":"); Serial.print(HeatControlOper.tFLW, 1);
  Serial.print(",\"pump\":"); Serial.print(pumpOn ? 1 : 0);
  Serial.print(",\"state\":"); Serial.print((int)HeatControlOper.hcSTATE);
  Serial.println("}");
}

// Processes incoming command strings from ESP32 via Serial
void handleESP32Communication() {
  static String inputBuffer = "";
  
  while (Serial.available()) {
    char c = (char)Serial.read();
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