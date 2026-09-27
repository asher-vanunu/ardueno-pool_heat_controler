#include "esp_comm.h"

#ifndef USE_HARDWARE_SERIAL
// יצירת אובייקט SoftwareSerial רק אם לא נבחר Hardware Serial
SoftwareSerial espSerial(SOFT_RX_PIN, SOFT_TX_PIN);
#endif

extern bool pumpOn;
extern bool ForceneedStop;
void SaveSetting();
void ResetToDefaultSettings();

// משתנים חיצוניים מתוך heat_controller.ino עבור לוג הדיאגנוסטיקה
extern float LasttCOL;
extern float prevtCOL;
extern float difftCOL;
extern float maxdifftCOL;
extern float diff_tCOL_TH;
extern bool needHeat;
extern bool needStop;
extern int needHeatHist;
extern int needStopHist;

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
  COMM_PORT.println(F("Serial is ready/n/r"));
}

void sendStatusToESP32() {
  COMM_PORT.print(F("{\"tCOL\":")); COMM_PORT.print(HeatControlOper.tCOL, 1);
  COMM_PORT.print(F(",\"tST\":")); COMM_PORT.print(HeatControlOper.tST, 1);
  COMM_PORT.print(F(",\"tFLW\":")); COMM_PORT.print(HeatControlOper.tFLW, 1);
  COMM_PORT.print(F(",\"pump\":")); COMM_PORT.print(pumpOn ? 1 : 0);
  COMM_PORT.print(F(",\"state\":")); COMM_PORT.print((int)HeatControlOper.hcSTATE);
  
  // שליחת ערכי הקונפיגורציה הנוכחיים ב-JSON
  COMM_PORT.print(F(",\"sMAX\":")); COMM_PORT.print(HeatControlMenu.sMAX.curr, 1);
  COMM_PORT.print(F(",\"tDEL\":")); COMM_PORT.print(HeatControlMenu.tDEL.curr, 1);
  COMM_PORT.print(F(",\"tON\":"));  COMM_PORT.print(HeatControlMenu.tON.curr, 1);
  COMM_PORT.print(F(",\"tOFF\":")); COMM_PORT.print(HeatControlMenu.tOFF.curr, 1);
  COMM_PORT.print(F(",\"DT_O\":")); COMM_PORT.print(HeatControlMenu.DT_O.curr, 1);
  COMM_PORT.print(F(",\"DT_F\":")); COMM_PORT.print(HeatControlMenu.DT_F.curr, 1);
  COMM_PORT.println(F("}"));
}

void sendLogToESP32() {
  static int frame_time = 0;

  COMM_PORT.print(F("{\"type\":\"log\""));
  COMM_PORT.print(F(",\"frame\":"));        COMM_PORT.print(frame_time++);
  COMM_PORT.print(F(",\"tCOL\":"));         COMM_PORT.print(HeatControlOper.tCOL, 2);
  COMM_PORT.print(F(",\"tST\":"));          COMM_PORT.print(HeatControlOper.tST, 2);
  COMM_PORT.print(F(",\"tFLW\":"));         COMM_PORT.print(HeatControlOper.tFLW, 2);
  COMM_PORT.print(F(",\"tCOL_last\":"));    COMM_PORT.print(LasttCOL, 2);
  COMM_PORT.print(F(",\"state\":"));        COMM_PORT.print((int)HeatControlOper.hcSTATE);
  COMM_PORT.print(F(",\"pump\":"));         COMM_PORT.print(pumpOn ? 1 : 0);
  COMM_PORT.print(F(",\"on_time\":"));      COMM_PORT.print(HeatControlOper.R1_ChangeTime[0]);
  COMM_PORT.print(F(",\"off_time\":"));     COMM_PORT.print(HeatControlOper.R1_ChangeTime[1]);
  COMM_PORT.print(F(",\"needHeat\":"));     COMM_PORT.print(needHeat ? 1 : 0);
  COMM_PORT.print(F(",\"needHeatHist\":")); COMM_PORT.print(needHeatHist);
  COMM_PORT.print(F(",\"needStop\":"));     COMM_PORT.print(needStop ? 1 : 0);
  COMM_PORT.print(F(",\"needStopHist\":")); COMM_PORT.print(needStopHist);
  COMM_PORT.print(F(",\"prevtCOL\":"));     COMM_PORT.print(prevtCOL, 2);
  COMM_PORT.print(F(",\"difftCOL\":"));     COMM_PORT.print(difftCOL, 2);
  COMM_PORT.print(F(",\"maxdifftCOL\":"));  COMM_PORT.print(maxdifftCOL, 2);
  COMM_PORT.print(F(",\"diff_tCOL_TH\":")); COMM_PORT.print(diff_tCOL_TH, 2);
  COMM_PORT.print(F(",\"sMAX\":"));         COMM_PORT.print(HeatControlMenu.sMAX.curr, 1);
  COMM_PORT.print(F(",\"tDEL\":"));         COMM_PORT.print(HeatControlMenu.tDEL.curr, 1);
  COMM_PORT.print(F(",\"tON\":"));          COMM_PORT.print(HeatControlMenu.tON.curr, 1);
  COMM_PORT.print(F(",\"tOFF\":"));         COMM_PORT.print(HeatControlMenu.tOFF.curr, 1);
  COMM_PORT.print(F(",\"DT_O\":"));         COMM_PORT.print(HeatControlMenu.DT_O.curr, 1);
  COMM_PORT.print(F(",\"DT_F\":"));         COMM_PORT.print(HeatControlMenu.DT_F.curr, 1);
  COMM_PORT.println(F("}"));
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
        else if (inputBuffer == "GET:LOG") {
          sendLogToESP32();
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