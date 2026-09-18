#ifndef ESP_COMM_H
#define ESP_COMM_H

#include <Arduino.h>
#include <SoftwareSerial.h>

// הגדרת פיני התקשורת החדשים
#define SOFT_RX_PIN 2
#define SOFT_TX_PIN 3

// הצהרה על אובייקט התקשורת כדי שיהיה זמין לכל הקבצים
extern SoftwareSerial espSerial;

void setupESP32Communication();
void sendStatusToESP32();
void handleESP32Communication();

#endif