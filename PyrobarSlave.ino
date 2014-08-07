#include <Wire.h>
#include "PyrobarSlaveConstants.h"
#include "PololuLedStrip.h"

typedef struct LightZoneInfo {
  uint8_t strip;
  uint16_t start,        // first index
           count;        // number of distinct addresses
  boolean isSymmetrical;
} LightZoneInfo;

PololuLedStrip<26> strip0;
PololuLedStrip<28> strip1;
PololuLedStrip<30> strip2;
PololuLedStrip<32> strip3;
PololuLedStrip<34> strip4;
PololuLedStrip<36> strip5;
PololuLedStripBase *ledStrips[6] = {&strip0, &strip1, &strip2, &strip3, &strip4, &strip5};

const int totalZoneCount = 10;
const int lightProgramPacketSize = 3 * 10;
LightZoneInfo lightZonesInfo[totalZoneCount] = {
  {0, 81, 18, false},  // Crane ring
  {0, 54, 27, false},  // Crane top
  {0, 27, 27, false},  // Crane middle
  {0, 0, 27, false},   // Crane base
  {1, 0, 50, true},    // Lounge
  {2, 0, 45, true},    // Bar ceiling
  {3, 0, 45, true},    // Bar surface
  {4, 0, 15, true},    // DJ booth
  {0, 0, 25, true},    // Steps
  {1, 100, 175, true}  // Undercarriage (offset will change for cartesian system)
};
                                                         // Value for Slave 1
const uint8_t lowZone = 0;                               // 8
const uint8_t highZone = 7;                              // 9
const uint8_t ledStripCount = 5;                         // 2
rgb_color tempColors[ledStripCount][100];                // [350] (longest strip)
const int stripAddressCounts[] = {100, 100, 90, 90, 30}; // {50, 350}

const int zoneCount = highZone - lowZone + 1;
rgb_color programValues[zoneCount];

boolean lightProgramIsOn = false;

void setup() {
  Serial.begin(9600);

  Serial.print("Setting up I2C at "); Serial.println(0);
  Wire.begin(0);

  Wire.onReceive(parseIncoming);
  Serial.println("Hello");
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(10000000);
}

void parseIncoming(int packetSize) {
  if (packetSize == 6) {   // lightBall(position, red, green, blue, decay)
    shutDownLightProgram();
  } else if (packetSize == lightProgramPacketSize) {
    igniteLightProgram();
    for (int disposalZoneIndex = 0; disposalZoneIndex < lowZone; disposalZoneIndex++) {
      for (int color = 0; color < 3; color++) {
        uint8_t _ = Wire.read();
      }
    }
    for (int zoneIndex = lowZone; zoneIndex <= highZone; zoneIndex++) {
      rgb_color color = { Wire.read(), Wire.read(), Wire.read()};
      writeEntireZoneBuffer(zoneIndex, color);
    }
    writeToStrips();
  }
}

void shutDownLightProgram() {
  if (lightProgramIsOn) {
    lightProgramIsOn = false;
    rgb_color color = {0, 0, 0};
    for (int zoneInd = lowZone; zoneInd <= highZone; zoneInd++) {
      writeEntireZoneBuffer(zoneInd, color);
    }
    writeToStrips();
  }
}

void writeToStrips() {
//  strip0.write(tempColors[0], stripAddressCounts[0]);
//  strip1.write(tempColors[1], stripAddressCounts[1]);
//  if (ledStripCount > 2) {
//    strip2.write(tempColors[2], stripAddressCounts[2]);
//    if (ledStripCount > 3) {
//      strip3.write(tempColors[3], stripAddressCounts[3]);
//      if (ledStripCount > 4) {
//        strip4.write(tempColors[4], stripAddressCounts[4]);
//        if (ledStripCount > 4) {
//          strip5.write(tempColors[5], stripAddressCounts[5]);
//        }
//      }
//    }
//  }
  for (int stripInd = 0; stripInd < ledStripCount; stripInd++) {
    ledStrips[stripInd]->write(tempColors[stripInd], stripAddressCounts[stripInd]);
  }
}

void igniteLightProgram() {
  lightProgramIsOn = true;
}

void writeEntireZoneBuffer(int zoneIndex, rgb_color color) {
  LightZoneInfo zoneInfo = lightZonesInfo[zoneIndex];
  int multiplier = zoneInfo.isSymmetrical ? 2 : 1;
  for (int address = zoneInfo.start; address < zoneInfo.count; address++) {
    tempColors[zoneInfo.strip][address] = color;
  }
}
