//More information at: https://www.aeq-web.com/
//Version 1.0 | 03-DEC-2020
#include <Arduino.h> 
#include <Wire.h> 
//#include <LiquidCrystal_I2C.h>
#include <LiquidCrystal_AIP31068_I2C.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <EEPROM.h>

//LiquidCrystal_I2C lcd(0x27,16,2);
LiquidCrystal_AIP31068_I2C lcd(0x3E,16,2);  // set the LCD address to 0x3E for a 20 chars and 4 line display
static uint8_t InputSwAntiBounce(void);
static uint8_t Check4LongButtonPress( uint8_t in_sw );
void HC_Logic(void);
void SaveSetting(void);
void ReadSetting(void);
void TakeLog(void);

const int PT1000_FLW_PIN = A2;  // storage temp
const int PT1000_STR_PIN = A1;  // storage temp
const int PT1000_COL_PIN = A0;  // collector tesp
void HC_DisplayState(void);

#define BUTTON_UP 0
#define BUTTON_OK 1
#define BUTTON_DW 2

#define HEAT_PUMP_CTRL   11

#define NUMOF_INPUT_SW   3
#define ANTI_BOUNCE_TIME 2
#define LONG_ANTI_BOUNCE_TIME 100
#define LOOP_DELAY      20
#define SEC_TIME_CNT    (1000/LOOP_DELAY)
#define HC_LOGIC_TIME   (1*SEC_TIME_CNT)  // 1sec
#define MENU_TIMEOUT    (60*SEC_TIME_CNT) // 60sec

const float vt_factor = 1.92;//1.88;
const float offset = -31.7;//-46;
float LasttCOL = 0, prevtCOL, difftCOL = 0, maxdifftCOL = 0;//, tST, tFLW;
uint8_t InputSw = 0, Hi2Low = 0, Low2Hi = 0;
uint8_t InputSwLong = 0, Hi2LowLong = 0, Low2HiLong = 0;
uint16_t HC_logic_Time = 0;
bool pumpOn = false;
bool needHeat = false, needStop = false, ForceneedStop = false;
int  needHeatHist = 0, needStopHist = 0;
bool wakeup = true;
int wakeup_time = 15;
int takeLogTime = (5*SEC_TIME_CNT);
#define VEC_SIZE  12
float sensCOLvector[VEC_SIZE] = {0};
float sensSTvector[VEC_SIZE] = {0};
float sensFLWvector[VEC_SIZE] = {0};
float diff_tCOL_TH = 15;

typedef enum 
{
  HC_STATE_SLEEP,
  HC_STATE_IDLE,
  HC_STATE_OPER_ON,
  HC_STATE_OPER_OFF,
  HC_STATE_OPER_FORCE_ON,
  HC_STATE_OPER_FORCE_OFF,
  HC_STATE_MENU,
  HC_STATE_SUBMENU,

  HC_NUM_STATES
}  heat_ctrl_state;

typedef enum 
{
  HC_MENU_SMAX,
  HC_MENU_TDEL,
  HC_MENU_TON,
  HC_MENU_TOFF,
  HC_MENU_DT_O,
  HC_MENU_DT_F,

  HC_NUM_MENU_STATES,
}  heat_ctrl_menu_state;

typedef struct hc_menu_items_span {
  char *str;
  float min;
  float max;
  float step;
  float curr;
} hc_menu_items_span_t;

typedef struct hc_menu {
  uint16_t menuIDX;
  hc_menu_items_span_t sMAX; 
  hc_menu_items_span_t tDEL;
  hc_menu_items_span_t tON;
  hc_menu_items_span_t tOFF;
  hc_menu_items_span_t DT_O; // switch on temp diff
  hc_menu_items_span_t DT_F; // switch off temp diff
} hc_menu_t;

hc_menu_t HeatControlMenu = { 0,
                            { "sMAX:",0.0, 100.0, 0.5, 35.0},
                            { "tDEL:",0.0, 10.0 , 0.5, 1.0},
                            { "t_ON:",1.0, 100.0, 0.5, 8.0},
                            { "tOFF:",1.0, 100.0, 0.5, 3.0},
                            { "DT_O:",0.0, 100.0, 0.5, 8.0},
                            { "DT_F:",0.0, 100.0, 0.5, 2.0}};

//typedef enum 
//{
//  HC_OPER_TCOL,
//  HC_OPER_TST,
//  HC_OPER_HR1,
// 
//  HC_NUM_OPER_STATES,
//}  heat_ctrl_oper_state;

typedef struct hc_oper {
  heat_ctrl_state hcSTATE;   
  heat_ctrl_state hc_prevSTATE;   
  uint16_t operIDX;
  uint32_t R1_ChangeTime[2]; // idx0=time_on,idx1=time_off
  float tCOL;
  float tST;
  float tFLW;
  float cDEL;
  float cON;
  float cOFF;
  float hR1;
} hc_oper_t;
hc_oper_t HeatControlOper = {HC_STATE_OPER_OFF,0,0,{0,150},0,0,0,0,0,0,0};

void setup() {
  lcd.init();                      // initialize the lcd 
  lcd.init();
  // Print a message to the LCD.
//  lcd.backlight();
  Serial.begin(19200); // opens serial port, sets data rate to 9600 bps
  Serial.println("Serial is ready/n/r");

  // Inputs
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DW, INPUT_PULLUP);
  pinMode(BUTTON_OK, INPUT_PULLUP);

  // Outputs
  pinMode(HEAT_PUMP_CTRL, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

//	for (int j = 0; j < VEC_SIZE; j++)
//  {
//		sensCOLvector[j] = 500;
//		sensSTvector[j] = 500;
//		sensFLWvector[j] = 500;
//  }

  ReadSetting();

}
//uint16_t val = 0x55, rd_val = 0;
//uint16_t add = 0x0, rd_add = 0x0;

void loop() {
  InputSw = InputSwAntiBounce();
  InputSwLong = Check4LongButtonPress(InputSw);

  if(digitalRead(BUTTON_UP)==HIGH)
  {
      digitalWrite(LED_BUILTIN, HIGH);
  }
  else
  {
      digitalWrite(LED_BUILTIN, LOW);
  }

  if(wakeup)
  {
      HC_DisplayState();
      wakeup = false;
  }

  HC_Logic();  
  if(++HC_logic_Time == HC_LOGIC_TIME && HeatControlOper.hcSTATE != HC_STATE_MENU && HeatControlOper.hcSTATE != HC_STATE_SUBMENU)
  {
      HC_logic_Time = 0;
      controlPoolHeat();
      HC_DisplayState();
  }

  if(takeLogTime)   takeLogTime--;
  if(takeLogTime==0)
  {
      takeLogTime = (7*SEC_TIME_CNT);
      TakeLog();
  }

  delay (LOOP_DELAY);

}


hc_menu_items_span_t *pMenuValues = &HeatControlMenu.sMAX;
uint32_t menu_timeout = MENU_TIMEOUT;

void SaveSetting(void)
{
    uint8_t *param;
    int k = 0, m = 0;
    uint8_t wrBuff[6*sizeof(float)], *buff_ptr = wrBuff;
    uint8_t rdBuff[6*sizeof(float)];

//    lcd.clear();
//    lcd.setCursor(1,0);
//    lcd.print("SAVE 1");    

    for(k=0, m=0; k<6; k++) // number of parameters
    { 
        param = (uint8_t*)&pMenuValues[k].curr;
        for(m=0; m<sizeof(float); m++) // bytes per param
            *buff_ptr++ = *param++;
    }

    // erase magic word
    EEPROM.write(0x0, 0x0);
    EEPROM.write(0x1, 0x0);

    for(k=0; k<sizeof(wrBuff); k++) 
        EEPROM.write(0x4+k, wrBuff[k]);

    for(k=0; k<sizeof(wrBuff); k++) 
        rdBuff[k] = EEPROM.read(0x4+k);

    for(k=0; k<sizeof(wrBuff); k++) 
    {
        if(wrBuff[k] != rdBuff[k])
        {
            lcd.clear();
            lcd.setCursor(1,0);
            lcd.print("Fail to saved"); 
            delay(1000);   
            return;
        }
    }

    lcd.clear();
    lcd.setCursor(1,0);
    lcd.print("Saved Successfully");    

    delay (500);

    // write magic word
    EEPROM.write(0x0, 0xCA); //EEPROM.put(0x04, 0.04);
    EEPROM.write(0x1, 0xFE); //EEPROM.put(0x04, 0.04);

}

void ReadSetting(void)
{
    uint8_t *param;
    int k = 0, m = 0, magic;
    uint8_t rdBuff[6*sizeof(float)];

    lcd.setCursor(0,0);
    lcd.print(pMenuValues[0].curr);
    lcd.setCursor(6,0);
    lcd.print(pMenuValues[1].curr);
    lcd.setCursor(11,0);
    lcd.print(pMenuValues[2].curr);
    lcd.setCursor(0,1);
    lcd.print(pMenuValues[3].curr);
    lcd.setCursor(6,1);
    lcd.print(pMenuValues[4].curr);
    lcd.setCursor(11,1);
    lcd.print(pMenuValues[5].curr);
    delay (2000);

//   return;
    magic  = EEPROM.read(0x0)<<8;
    magic |= EEPROM.read(0x1);

    if(magic!=0xCAFE)
        return;

    for(k=0; k<sizeof(rdBuff); k++) 
      rdBuff[k] = EEPROM.read(0x4+k);

    for(k=0; k<6; k++) // number of parameters
        pMenuValues[k].curr = *((float*)&rdBuff[4*k]);
    
    lcd.clear();
    delay (500);
    lcd.setCursor(0,0);
    lcd.print(pMenuValues[0].curr);
    lcd.setCursor(6,0);
    lcd.print(pMenuValues[1].curr);
    lcd.setCursor(11,0);
    lcd.print(pMenuValues[2].curr);
    lcd.setCursor(0,1);
    lcd.print(pMenuValues[3].curr);
    lcd.setCursor(6,1);
    lcd.print(pMenuValues[4].curr);
    lcd.setCursor(11,1);
    lcd.print(pMenuValues[5].curr);
    delay (2000);
    lcd.clear();

}

void HC_Logic(void) 
{

  if((InputSwLong&(1<<BUTTON_OK)) && (InputSwLong&(1<<BUTTON_UP)))
  {
      HeatControlOper.hcSTATE = HC_STATE_OPER_FORCE_ON;
  }
  else if((InputSwLong&(1<<BUTTON_OK)) && (InputSwLong&(1<<BUTTON_DW)))
  {
      HeatControlOper.hcSTATE = HC_STATE_OPER_FORCE_OFF;    
  }
  else if((InputSwLong&(1<<BUTTON_UP)) && (InputSwLong&(1<<BUTTON_DW)))
  {
      HeatControlOper.hcSTATE = HC_STATE_IDLE;
      ForceneedStop = 1;
  }
//  else if((InputSwLong&(1<<BUTTON_OK)))
//  {
//      HeatControlOper.hc_prevSTATE = HeatControlOper.hcSTATE;
//      HeatControlOper.hcSTATE = HC_STATE_MENU;
//      HeatControlMenu.menuIDX = 0;
//  }


  switch(HeatControlOper.hcSTATE)
  {
      case  HC_STATE_SLEEP:
          HeatControlOper.operIDX = 0;
          HC_DisplayState();
      break;

      case  HC_STATE_OPER_ON:
      case  HC_STATE_OPER_OFF:
      case  HC_STATE_IDLE:
          
          if((Low2Hi&(1<<BUTTON_DW)) && (HeatControlMenu.menuIDX<=10-3))  
          {
              HeatControlMenu.menuIDX++;
              HC_DisplayState();
          }

          if((Low2Hi&(1<<BUTTON_UP)) && (HeatControlMenu.menuIDX>0))  
          {
              HeatControlMenu.menuIDX--;
              HC_DisplayState();
          }
                    
          if((Low2HiLong&(1<<BUTTON_OK)))
          {
              HeatControlOper.hc_prevSTATE = HeatControlOper.hcSTATE;
              HeatControlOper.hcSTATE = HC_STATE_MENU;
              HeatControlMenu.menuIDX = 0;
              menu_timeout = MENU_TIMEOUT;
              HC_DisplayState();
          }
      break;

      case  HC_STATE_MENU:
          HC_logic_Time = 0;

          if((Low2Hi&(1<<BUTTON_DW)) && (HeatControlMenu.menuIDX<=HC_NUM_MENU_STATES-2))  
          {
              HeatControlMenu.menuIDX++;
              menu_timeout = MENU_TIMEOUT;
              HC_DisplayState();
          }
          if((Low2Hi&(1<<BUTTON_UP)) && (HeatControlMenu.menuIDX>0))  
          {
              HeatControlMenu.menuIDX--;
              menu_timeout = MENU_TIMEOUT;
              HC_DisplayState();
          }
          if((Low2Hi&(1<<BUTTON_OK)) && menu_timeout!=MENU_TIMEOUT)//&& (InputSwLong&(1<<BUTTON_OK)))
          {
              HeatControlOper.hcSTATE = HC_STATE_SUBMENU; // move down menu level
              menu_timeout = MENU_TIMEOUT;
              HC_DisplayState();
          }
          if((Low2HiLong&(1<<BUTTON_OK)) || menu_timeout==0)
          {
              if(Low2HiLong&(1<<BUTTON_OK))
                  SaveSetting();

              HeatControlOper.hcSTATE = HC_STATE_IDLE;
              HeatControlOper.hc_prevSTATE = HC_STATE_MENU;        
              HC_DisplayState();
          }

          menu_timeout--;

      break;

      case  HC_STATE_SUBMENU:
          HC_logic_Time = 0;

          if((Low2Hi&(1<<BUTTON_UP)) && (pMenuValues[HeatControlMenu.menuIDX].curr<pMenuValues[HeatControlMenu.menuIDX].max))
          {
              pMenuValues[HeatControlMenu.menuIDX].curr += pMenuValues[HeatControlMenu.menuIDX].step;
              menu_timeout = MENU_TIMEOUT;
              HC_DisplayState();
          }
          if(Low2Hi&(1<<BUTTON_DW) && (pMenuValues[HeatControlMenu.menuIDX].curr>pMenuValues[HeatControlMenu.menuIDX].min))
          {
              pMenuValues[HeatControlMenu.menuIDX].curr -= pMenuValues[HeatControlMenu.menuIDX].step;
              menu_timeout = MENU_TIMEOUT;
              HC_DisplayState();
          }
          if((Low2Hi&(1<<BUTTON_OK)) )//&& (InputSwLong&(1<<BUTTON_OK)))
          {
//              if(Low2HiLong&(1<<BUTTON_OK))
//              SaveSetting();

              HeatControlOper.hcSTATE = HC_STATE_MENU; // move up menu level
              menu_timeout = MENU_TIMEOUT;
              HC_DisplayState();
          }
          if((Low2HiLong&(1<<BUTTON_OK)) || menu_timeout==0)
          {
//              if(Low2HiLong&(1<<BUTTON_OK))
//                  SaveSetting();

              HeatControlOper.hcSTATE = HC_STATE_IDLE;
              HeatControlOper.hc_prevSTATE = HC_STATE_MENU;        
              HC_DisplayState();
          }

          menu_timeout--;

      break;
 
      case  HC_STATE_OPER_FORCE_ON:
      case  HC_STATE_OPER_FORCE_OFF:
          if(HeatControlOper.hcSTATE != HeatControlOper.hc_prevSTATE )
              HC_DisplayState();

          if((Low2Hi&(1<<BUTTON_DW)) && (HeatControlMenu.menuIDX<=10-3))  
          {
              HeatControlMenu.menuIDX++;
              HC_DisplayState();
          }

          if((Low2Hi&(1<<BUTTON_UP)) && (HeatControlMenu.menuIDX>0))  
          {
              HeatControlMenu.menuIDX--;
              HC_DisplayState();
          }
      break;
   }

  if(Low2Hi&(1<<BUTTON_UP))
  {
      if(HeatControlOper.hcSTATE == HC_STATE_SLEEP )
      {

      }
  }
  else if(Low2Hi&(1<<BUTTON_DW))
  {

  }

  HeatControlOper.hc_prevSTATE = HeatControlOper.hcSTATE;
}

void HC_DisplayState(void)
{
  if( HeatControlOper.hcSTATE == HC_STATE_OPER_FORCE_ON )
  {
      lcd.clear();    
      lcd.setCursor(11,1);
      lcd.print("    ");
      lcd.setCursor(11,0);
      lcd.print("F-ON ");
  }

  if( HeatControlOper.hcSTATE == HC_STATE_OPER_FORCE_OFF )
  {
      lcd.clear();    
      lcd.setCursor(11,1);
      lcd.print("    ");
      lcd.setCursor(11,0);
      lcd.print("F-OFF ");
  }

  if(HeatControlOper.hcSTATE != HC_STATE_SLEEP && HeatControlOper.hcSTATE != HC_STATE_MENU  && HeatControlOper.hcSTATE != HC_STATE_SUBMENU)
  {
      char *strs[10] { "tCol:", "tStr:", "tFlw:", "tLst", pMenuValues[0].str, pMenuValues[1].str, pMenuValues[2].str, pMenuValues[3].str, pMenuValues[4].str, pMenuValues[5].str};
      float vals[10] = {HeatControlOper.tCOL, HeatControlOper.tST, HeatControlOper.tFLW, LasttCOL, pMenuValues[0].curr, pMenuValues[1].curr, pMenuValues[2].curr, pMenuValues[3].curr, pMenuValues[4].curr, pMenuValues[5].curr };
      int idx = HeatControlMenu.menuIDX;
      lcd.clear();    
      lcd.setCursor(0,0);
      lcd.print(strs[idx]);
      lcd.setCursor(5,0);
      lcd.print(vals[idx],1);

      lcd.setCursor(0,1);
      lcd.print(strs[idx+1]);
      lcd.setCursor(5,1);
      lcd.print(vals[idx+1],1);
      

      lcd.setCursor(11,0);
      if(HeatControlOper.hcSTATE == HC_STATE_OPER_FORCE_ON )
          lcd.print("F-ON ");
      else if(HeatControlOper.hcSTATE == HC_STATE_OPER_FORCE_OFF)
          lcd.print("F-OFF ");       
      else
          lcd.print((pumpOn ? "ON  " : "OFF "));

      lcd.setCursor(15,0);
      lcd.print((needHeat  ?  "H":" "));    
      lcd.setCursor(15,1);
      lcd.print((needStop  ?  "S":" "));

      lcd.setCursor(11,1);
      lcd.print((pumpOn ? HeatControlOper.R1_ChangeTime[0] : HeatControlOper.R1_ChangeTime[1]));
//      lcd.setCursor(14,0);
//      lcd.print(needStopHist);

  }

  if(HeatControlOper.hcSTATE == HC_STATE_MENU && HeatControlOper.hc_prevSTATE != HC_STATE_MENU)
  {
      lcd.clear();
      lcd.setCursor(6,0);
      lcd.print("Manu");
  }

  if(HeatControlOper.hcSTATE != HC_STATE_MENU && HeatControlOper.hcSTATE != HC_STATE_SUBMENU && HeatControlOper.hc_prevSTATE == HC_STATE_MENU)
  {
      lcd.clear();
  }

  if(HeatControlOper.hcSTATE == HC_STATE_MENU)
  {
      lcd.setCursor(0,1);
      lcd.print(pMenuValues[HeatControlMenu.menuIDX].str);
      lcd.setCursor(6,1);
      lcd.print(pMenuValues[HeatControlMenu.menuIDX].curr);
  }
  else if(HeatControlOper.hcSTATE == HC_STATE_SUBMENU)
  {
//    lcd.setCursor(0,1);
//    lcd.print(pMenuValues[HeatControlMenu.menuIDX].str);
    lcd.setCursor(12,1);
    lcd.print("set");
    lcd.setCursor(6,1);
    lcd.print(pMenuValues[HeatControlMenu.menuIDX].curr);
  }
}

#define SETPOINT_TEMPERATURE   HeatControlMenu.sMAX.curr//35 // Desired storage temperature (in degrees Celsius)
#define DELTA_T_ON_HYSTERESIS  HeatControlMenu.DT_O.curr//5.0    // Temperature difference hysteresis (in degrees Celsius)
#define DELTA_T_OFF_HYSTERESIS HeatControlMenu.DT_F.curr//1.0    // Temperature difference hysteresis (in degrees Celsius)

#define MIN_PUMP_ON_TIME  (uint32_t)(60*HeatControlMenu.tON.curr)//30    // Minimum time the pump should be ON (in seconds)
#define MIN_PUMP_OFF_TIME (uint32_t)(60*HeatControlMenu.tOFF.curr)//10   // Minimum time the pump should be OFF (in seconds)

int readTemperature(int sensorPin) {
    // Simulated function to read temperature from the specified sensor pin
    // Replace with actual code to read the sensors in your hardware
    return rand() % 50; // Simulated temperature reading (0-49°C)
}

float InsertNew(float *vec, float number)
{
		for (int j = 0; j < (VEC_SIZE-1); j++)
			vec[j] = vec[j + 1];

      vec[VEC_SIZE-1] = number;
}

void bubbleSort(float vector[], int n) {
	int i, j;
  float temp;
	for (i = 0; i < n - 1; i++) {
		for (j = 0; j < n - i - 1; j++) {
			if (vector[j] > vector[j + 1]) {
				// Swap elements
				temp = vector[j];
				vector[j] = vector[j + 1];
				vector[j + 1] = temp;
			}
		}
	}
}

float CalcAverage(float *vec, int len)
{
	float avg = 0;
  for (int j = 0 ; j < len; j++)
      avg += vec[j];
  
  return (avg/len);
}


float CalcMedinTemp(float *vec_in, float number, int avg_flag)
{
  float vec_sort[VEC_SIZE];

  InsertNew(vec_in, number);
  
	for (int j = 0; j < VEC_SIZE; j++)
      vec_sort[j] = vec_in[j];

  bubbleSort(vec_sort, VEC_SIZE);

  if(avg_flag)
    return CalcAverage(&vec_sort[4], 4); 

	return vec_sort[(VEC_SIZE / 2) - 1]; // return median value
}

void controlPoolHeat()
 {    
    int sensCOLvalue = analogRead ( PT1000_COL_PIN );
    int sensSTvalue  = analogRead ( PT1000_STR_PIN );
    int sensFLWvalue  = analogRead ( PT1000_FLW_PIN );
    float vCOL = sensCOLvalue * (5.0/ 1023.0);
    float vST = sensSTvalue * (5.0/ 1023.0);
    float vFLW = sensFLWvalue * (5.0/ 1023.0);

    if(wakeup_time)   wakeup_time--;

//    HeatControlOper.tCOL = ( ( ( vCOL * 100) / vt_factor ) + offset );
//    HeatControlOper.tST  = ( ( ( vST * 100) / vt_factor ) + offset );
//    HeatControlOper.tFLW  = ( ( ( vFLW * 100) / vt_factor ) + offset );
//-----------------------------------------------------------------------------    
    prevtCOL = HeatControlOper.tCOL;
    HeatControlOper.tCOL = CalcMedinTemp( sensCOLvector , (((vCOL * 100)/vt_factor) + offset) , 1);
    HeatControlOper.tST  = CalcMedinTemp( sensSTvector  , (((vST  * 100)/vt_factor) + offset) , 1);
    HeatControlOper.tFLW = CalcMedinTemp( sensFLWvector, (((vFLW  * 100)/vt_factor) + offset), 1);
//-------------------------------------------------------------------------------
//    if(HeatControlOper.R1_ChangeTime[1] < 1800)    diff_tCOL_TH = 15;
//    else if(HeatControlOper.R1_ChangeTime[1] > 1800 && HeatControlOper.R1_ChangeTime[1] < 2400)    diff_tCOL_TH = 12;
//    else    diff_tCOL_TH = 8;
    if(HeatControlOper.R1_ChangeTime[1] < 720)    diff_tCOL_TH = 13; // 12min
    else if(HeatControlOper.R1_ChangeTime[1] > 720 && HeatControlOper.R1_ChangeTime[1] < 900)    diff_tCOL_TH = 11; // 15min
    else if(HeatControlOper.R1_ChangeTime[1] > 900 && HeatControlOper.R1_ChangeTime[1] < 1080)    diff_tCOL_TH = 9; // 18min
//    else if(HeatControlOper.R1_ChangeTime[1] > 1080 && HeatControlOper.R1_ChangeTime[1] < 1080)    diff_tCOL_TH = 7;//
    else    diff_tCOL_TH = 7;

    if( HeatControlOper.hcSTATE == HC_STATE_OPER_FORCE_ON )//&& pumpOn == false)
    {
        if(pumpOn == false)
        {
            pumpOn = true;
            digitalWrite(HEAT_PUMP_CTRL, HIGH);
            HeatControlOper.R1_ChangeTime[0] = 0;
        }
    }
    else if( HeatControlOper.hcSTATE == HC_STATE_OPER_FORCE_OFF )//&& pumpOn == true )
    {
        if(pumpOn == true)
        {
            pumpOn = false;
            digitalWrite(HEAT_PUMP_CTRL, LOW);
            HeatControlOper.R1_ChangeTime[1] = 0;//PumpChangeToOffTime = 0;
        }
    }
    else
    {
        // Automatic
        if(wakeup_time)   
            return;

//        needHeat = (HeatControlOper.tCOL - HeatControlOper.tST > DELTA_T_ON_HYSTERESIS) && (HeatControlOper.tST < HeatControlMenu.sMAX.curr);
//        needStop = (HeatControlOper.tCOL - HeatControlOper.tST < DELTA_T_OFF_HYSTERESIS) || (HeatControlOper.tST >= HeatControlMenu.sMAX.curr);
//        needStop = (HeatControlOper.tST - HeatControlOper.tFLW < DELTA_T_OFF_HYSTERESIS) || (HeatControlOper.tST >= HeatControlMenu.sMAX.curr);
//        if(needHeat)   needHeatHist++;
//        else           needHeatHist = 0;
//        if(needStop)   needStopHist++;
//        else           needStopHist = 0;

        if (pumpOn) 
        {
//            needStopHist += (HeatControlOper.tST - HeatControlOper.tFLW < DELTA_T_OFF_HYSTERESIS) || (HeatControlOper.tST >= HeatControlMenu.sMAX.curr);
            if(HeatControlOper.R1_ChangeTime[0]>40) {
//                needStopHist += ((HeatControlOper.tST - HeatControlOper.tFLW < DELTA_T_OFF_HYSTERESIS) || (HeatControlOper.tST >= HeatControlMenu.sMAX.curr));
                if((HeatControlOper.tFLW - HeatControlOper.tST < DELTA_T_OFF_HYSTERESIS) || (HeatControlOper.tST >= HeatControlMenu.sMAX.curr))
                    needStopHist++;
                else
                    needStopHist = 0;
                if(needStopHist>20)
                {     
                    needStopHist = 0;
                    needStop = true;
                }
            }

            if ((needStop && (HeatControlOper.R1_ChangeTime[0] >= MIN_PUMP_ON_TIME)) || ForceneedStop)
            {
                // Turn off the pump if heat is not needed or if it has been on for at least MIN_PUMP_ON_TIME
                pumpOn = false;
                needStop = false;
                ForceneedStop = false;
                HeatControlOper.hcSTATE = HC_STATE_OPER_OFF;
                // Code to deactivate the pump relay (replace with your hardware control code)
                digitalWrite(HEAT_PUMP_CTRL, LOW);
                HeatControlOper.R1_ChangeTime[1] = 0;
                LasttCOL = HeatControlOper.tCOL;
                Serial.print("\n\r@@@@@@@@@@@@@@@@ PUMP OFF\n\n\r");
                maxdifftCOL = 0; difftCOL = 0;
                diff_tCOL_TH = 15;
            }
        } 
        else 
        {
            difftCOL = HeatControlOper.tCOL - prevtCOL;
            if(difftCOL>maxdifftCOL)    maxdifftCOL = difftCOL;

            needHeatHist += ((HeatControlOper.tCOL - HeatControlOper.tST > DELTA_T_ON_HYSTERESIS) && (HeatControlOper.tST < HeatControlMenu.sMAX.curr) && (HeatControlOper.tCOL - LasttCOL > diff_tCOL_TH));
            if(needHeatHist>20)
            {     
                needHeatHist = 0;
                needHeat = true;
            }

            if (needHeat && HeatControlOper.R1_ChangeTime[1] >= MIN_PUMP_OFF_TIME )//&& (HeatControlOper.tST < HeatControlMenu.sMAX.curr))
            {
                // Turn on the pump if heat is needed and it has been off for at least MIN_PUMP_OFF_TIME
                pumpOn = true;
                needHeat = false;
                HeatControlOper.hcSTATE = HC_STATE_OPER_ON;
                // Code to activate the pump relay (replace with your hardware control code)
                digitalWrite(HEAT_PUMP_CTRL, HIGH);
                HeatControlOper.R1_ChangeTime[0] = 0;
                Serial.print("\n\r@@@@@@@@@@@@@@@@ PUMP ON\n\n\r");
            }
        }
    }

    if (pumpOn) 
        HeatControlOper.R1_ChangeTime[0]++;
    else
        HeatControlOper.R1_ChangeTime[1]++;
       
//    HC_DisplayState();
    
}

typedef struct hc_log_data {
  byte *ptr;
  int  size;
} hc_log_data_t;

void TakeLog(void)
{
    static int frame_time = 0;
    char *strs[9] { "tCol:", "tStr:", "tFlw:", pMenuValues[0].str, pMenuValues[1].str, pMenuValues[2].str, pMenuValues[3].str, pMenuValues[4].str, pMenuValues[5].str};
     float vals[9] = {HeatControlOper.tCOL, HeatControlOper.tST, HeatControlOper.tFLW, pMenuValues[0].curr, pMenuValues[1].curr, pMenuValues[2].curr, pMenuValues[3].curr, pMenuValues[4].curr, pMenuValues[5].curr };

   if(frame_time==0)
    {
        Serial.println("######## Device Set_up ##########\n\r");
		    for (int k = 0; k < 6; k++)
        {
            Serial.print(strs[3+k]);
            Serial.println(vals[3+k]);
//            Serial.print("\r");
//            sprintf(first_str, "Smax=%f.\n\r", HeatControlOper);
        }
        Serial.print("\n\r####################################\n\n\r");

//        Serial.print("tCOL  |   tST   |  tFLW   |  LasttCOL |  State  |  Pump-State |  ON-Timer  |  OFF-Timer  |  needHeat  |  needStop | fram time");
    }

    Serial.print("Frame:        "); Serial.println(frame_time);                        //Serial.print("\n\r");
    Serial.print("tCOL:         "); Serial.println(HeatControlOper.tCOL);              //Serial.print(", ");
    Serial.print("tST:          "); Serial.println(HeatControlOper.tST) ;              //Serial.print(", ");
    Serial.print("tFLW:         "); Serial.println(HeatControlOper.tFLW);              //Serial.print(", ");
    Serial.print("tCOL_last:    "); Serial.println(LasttCOL);                          //Serial.print(", ");
    Serial.print("State:        "); Serial.println(HeatControlOper.hcSTATE);           //Serial.print(", ");
    Serial.print("Pamp State:   "); Serial.println(pumpOn);                            //Serial.print(", ");
    Serial.print("ON time:      "); Serial.println(HeatControlOper.R1_ChangeTime[0]);  //Serial.print(", ");
    Serial.print("OFF time:     "); Serial.println(HeatControlOper.R1_ChangeTime[1]);  //Serial.print(", ");
    Serial.print("Need Heat:    "); Serial.println(needHeat);                          //Serial.print(", ");
    Serial.print("NeedHeatHist: "); Serial.println(needHeatHist);                      //Serial.print(", ");
    Serial.print("Need Stop:    "); Serial.println(needStop);                          //Serial.print(", ");
    Serial.print("NeedStopHist: "); Serial.println(needStopHist);                      //Serial.print(", ");
    Serial.print("PrevtCOL:     "); Serial.println(prevtCOL);                          //Serial.print(", ");
    Serial.print("DifftCOL:     "); Serial.println(difftCOL);                          //Serial.print(", ");
    Serial.print("maxDifftCOL:  "); Serial.println(maxdifftCOL);                       //Serial.print(", ");
    Serial.print("diff_tCOL_TH:  "); Serial.println(diff_tCOL_TH);                       //Serial.print(", ");
   
#if 1
    // Send Raw data
    byte Magic[] = { 0xCA, 0xFE, 0xCA, 0xFE};
    hc_log_data_t Raw_data[12] = {
        // Convert float to a byte array
        (byte *)Magic                             , sizeof(Magic),
        (byte *)&frame_time                       ,  sizeof(frame_time),
        (byte *)&HeatControlOper.tCOL             ,  sizeof(HeatControlOper.tCOL),
        (byte *)&HeatControlOper.tST              ,  sizeof(HeatControlOper.tST),
        (byte *)&HeatControlOper.tFLW             ,  sizeof(HeatControlOper.tFLW),
        (byte *)&LasttCOL                         ,  sizeof(LasttCOL),
        (byte *)&HeatControlOper.hcSTATE          ,  sizeof(HeatControlOper.hcSTATE),
        (byte *)&pumpOn                           ,  sizeof(pumpOn),
        (byte *)&HeatControlOper.R1_ChangeTime[0] ,  sizeof(HeatControlOper.R1_ChangeTime[0]),
        (byte *)&HeatControlOper.R1_ChangeTime[1] ,  sizeof(HeatControlOper.R1_ChangeTime[1]),
        (byte *)&needHeat                         ,  sizeof(needHeat),
        (byte *)&needStop                         ,  sizeof(needStop),
    };
    Serial.println("\n\rRaw Data:");

  // Send each byte of the float value
  for (int i = 0; i < 12; i++) {
      for (int k = 0; k < Raw_data[i].size; k++) 
          Serial.write(Raw_data[i].ptr[k]);
  }
    
  Serial.println("\n\rEnd of Raw Data.");  
  Serial.print("\n\r");
#endif
  Serial.print("----------------------------------\r");

    frame_time++;   
}


static uint8_t InputSwAntiBounce(void)
{
	static uint8_t sw_cntr[NUMOF_INPUT_SW] = { 0,0,0 };
	uint8_t TempInputSw, sw;

	TempInputSw  = (digitalRead(BUTTON_UP)==HIGH) << 0;  // bit0
	TempInputSw |= (digitalRead(BUTTON_OK)==HIGH) << 1;  // bit1
	TempInputSw |= (digitalRead(BUTTON_DW)==HIGH) << 2;  // bit2
	TempInputSw ^= 0x7; // ^1 --> to change logic

	Hi2Low = Low2Hi = 0;

	for (int k = 0; k < NUMOF_INPUT_SW; k++)
	{
		sw = (1 << k);
		if ((InputSw^TempInputSw)&sw)
		{
			if (sw_cntr[k]++ >= ANTI_BOUNCE_TIME)
			{
				sw_cntr[k] = 0;
				InputSw &= ~sw;
				InputSw |= (TempInputSw&sw);
				if (InputSw&sw)    Low2Hi |= sw;
				else              Hi2Low |= sw;
			}
		}
		else
		{
			sw_cntr[k] = 0;
		}
	}

	return InputSw;
}

static uint8_t Check4LongButtonPress( uint8_t in_sw )
{
  static uint8_t long_press_counter[8] = {0,0,0,0,0,0,0,0};
  uint8_t long_button_press = InputSwLong, bit_mask, k;

	Hi2LowLong = Low2HiLong = 0;

  for(k=0; k<NUMOF_INPUT_SW; k++)
  {
      bit_mask = (1<<k);
      if(in_sw&bit_mask)
      {
          if(++long_press_counter[k] >= LONG_ANTI_BOUNCE_TIME)   { long_button_press |= bit_mask; long_press_counter[k] = 0; Low2HiLong |= bit_mask;}
      }
      else
      {
          if(long_button_press&bit_mask)   Hi2LowLong |= bit_mask;
          long_button_press &= ~bit_mask;
          long_press_counter[k] = 0;
      }
  }
  return long_button_press;
}


///////////// DUMP
#if 0
float CalcMedinTemp2(float *vec, float number)
{
	float temp;

	for (int j = 0; j < VEC_SIZE; j++)
	{
		if (number < vec[j]) {
			temp = vec[j];
			vec[j] = number;
			number = temp;
		}
	}

	return vec[(VEC_SIZE / 2) - 1]; // return median value
}

static int wakeup2[3] = { VEC_SIZE,VEC_SIZE,VEC_SIZE};
float VectorMovingSorting(float *vec, float number, int idx)
{
	float temp;

	if (wakeup2[idx] == 0)
	{
		for (int j = 0; j < (VEC_SIZE-1); j++)
			vec[j] = vec[j + 1];

      vec[VEC_SIZE-1] = 500;
	}
	if(wakeup2[idx])  wakeup2[idx]--;

	for (int j = 0 ; j < VEC_SIZE; j++)
	{
		if (number < vec[j]) {
			temp = vec[j];
			vec[j] = number;
			number = temp;
		}
	}
  
  return vec[(VEC_SIZE/2) - 1]; // return median value

}

#endif