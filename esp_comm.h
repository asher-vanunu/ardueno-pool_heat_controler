#ifndef ESP_COMM_H
#define ESP_COMM_H

#include <Arduino.h>

// =========================================================================
// בורר מצב תקשורת מול ה-ESP32:
// בטל את ההערה (uncomment) כדי לעבוד דרך החומרה (פינים D0 ו-D1) - חובה עבור OTA!
// שים בהערה (//) כדי לעבוד דרך SoftwareSerial (פינים D2 ו-D3)
// =========================================================================
#define USE_HARDWARE_SERIAL

#ifdef USE_HARDWARE_SERIAL
  // מיפוי ישיר ל-Hardware Serial של הנאנו (D0/D1)
  #define COMM_PORT Serial
#else
  #include <SoftwareSerial.h>
  #define SOFT_RX_PIN 2
  #define SOFT_TX_PIN 3
  extern SoftwareSerial espSerial;
  #define COMM_PORT espSerial
#endif

void setupESP32Communication();
void sendStatusToESP32();
void handleESP32Communication();
void ResetToDefaultSettings();

#endif