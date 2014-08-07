#include <Wire.h>
#include "PyrobarSlaveConstants.h"
#define BASE_I2C_ADDRESS 0x10
#define NUM_LIGHT_ZONES 4

//const uint8_t lightPins[NUM_LIGHT_ZONES][COLOR_COUNT] = 
//  {{2, 3, 4}, {5, 6, 7}, {8, 9, 10}, {11, 12, 13}};
const uint8_t lightPins[] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
const int zonePinsCount = NUM_LIGHT_ZONES * COLOR_COUNT;

bool mainLightsOn = false;

void setup() {
  Serial.begin(9600);
  delay(100);
  for (int zone = 0; zone < NUM_LIGHT_ZONES; zone++) {
    for (int color = 0; color < COLOR_COUNT; color++) {
      pinMode(lightPins[zone * COLOR_COUNT + color], OUTPUT);        
    }
  }
  delay(100);  
  Serial.print("Setting up I2C at "); Serial.println(BASE_I2C_ADDRESS + 1);
  Wire.begin(BASE_I2C_ADDRESS + 1);
  delay(100);
  Wire.onReceive(writeLights);
  Serial.println("Hello");
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(10000);
}

void writeLights(int pktSz) {
//  Serial.print("Wire incoming, size: "); Serial.println(pktSz);
  if (pktSz == zonePinsCount) {
    for (int zone = 0; zone < 4; zone++) {
      for (int color = 0; color < 3; color++) {
        uint8_t value = Wire.read();
//        if (zone == 0 && color == 0) Serial.println(value);
        analogWrite(lightPins[zone * COLOR_COUNT + color], value);
      }
    }
  }
}

