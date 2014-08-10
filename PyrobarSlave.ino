#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>
#include "PyrobarSlaveConstants.h"
#include "PyrobarSlaveOnlyConstants.h"
#include "PyrobarLightStrip.h"

const int totalZoneCount = 10;

#ifdef SLAVE1

const uint8_t lowZone = 8;
const uint8_t highZone = 9;
const uint8_t ledStripCount = 2;
rgb_color tempColors[ledStripCount][350];
const int stripAddressCounts[] = {50, 350};

#else

const uint8_t lowZone = 0;
const uint8_t highZone = 7;
const uint8_t ledStripCount = 5;
rgb_color tempColors[ledStripCount][100];
const int stripAddressCounts[] = {100, 100, 90, 90, 30};

#endif

PololuLedStrip<29> strip0;
PololuLedStrip<31> strip1;

#ifdef SLAVE1

LightZoneInfo lightZonesInfo[8] = {
  {0, 0, 25, true},    // Steps
  {1, 0, 175, true}  // Undercarriage (offset will change for cartesian system)
};
PololuLedStripBase *ledStrips[2] = {&strip0, &strip1};

#else

PololuLedStrip<33> strip2;
PololuLedStrip<35> strip3;
PololuLedStrip<37> strip4;

LightZoneInfo lightZonesInfo[8] = {
  {0, 0, 18, false},  // Crane ring
  {0, 18, 27, false},  // Crane top
  {0, 45, 27, false},  // Crane middle
  {0, 72, 27, false},   // Crane base
  {1, 0, 50, true},    // Lounge
  {2, 0, 45, true},    // Bar ceiling
  {3, 0, 45, true},    // Bar surface
  {4, 0, 15, true},    // DJ booth
};
PololuLedStripBase *ledStrips[5] = {&strip0, &strip1, &strip2, &strip3, &strip4};

#endif

#ifdef SLAVE1
// Only need coordinates; color will be 0, 0, 0 by default
Pixel pixels0[175] = {{1000, 500}, {999, 499}}; // Crane and ring
Pixel pixels1[50] =  {{1000, 500}}; // Lounge
PyrobarLightStrip lightStrips2d[2] = {
  {pixels0, 175, true, 100, tempColors[0]},
  {pixels1, 50, true, 0, tempColors[1]}
};

#else

Pixel pixels0[100] = {{1000, 500}, {999, 499}}; // Crane and ring
Pixel pixels1[50] =  {{1000, 500}}; // Lounge
Pixel pixels2[45] =  {{1000, 500}}; // Bar ceiling
Pixel pixels3[45] =  {{1000, 500}}; // Bar surface
Pixel pixels4[15] =  {{1000, 500}}; // DJ booth
PyrobarLightStrip lightStrips2d[5] = {
  {pixels0, 100, false, 0, tempColors[0]},
  {pixels1, 50, true, 0, tempColors[1]},
  {pixels1, 45, true, 0, tempColors[2]},
  {pixels1, 45, true, 0, tempColors[3]},
  {pixels1, 15, true, 0, tempColors[4]},
};

#endif

const int lightProgramPacketSize = 3 * 10;

const int zoneCount = highZone - lowZone + 1;
float halfLife;
unsigned long lastLoopTime;

LightMode lightMode = BALL_DRAG;

void setup() {
  Serial.begin(9600);

  Serial.print("Setting up I2C at "); Serial.println(BASE_I2C_ADDRESS);
  Wire.begin(BASE_I2C_ADDRESS);

  Wire.onReceive(parseIncoming);
  Serial.println("Hello");

  lastLoopTime = millis();
}

void loop() {
  lastLoopTime = millis();
  if (lightMode == BALL_DRAG) {
    float elapsedTime = (millis() - lastLoopTime) / 1000.0;
    float decayFactor = pow(0.5, elapsedTime / halfLife);
    for (int stripInd = 0; stripInd < ledStripCount; stripInd++) {
      lightStrips2d[stripInd].update(decayFactor);
    }
    writeToStrips();
  }
  delay(25);
}

void writeEntireZoneBuffer(int zoneIndex, rgb_color color) {
  LightZoneInfo zoneInfo = lightZonesInfo[zoneIndex];
  int multiplier = zoneInfo.isSymmetrical ? 2 : 1;
  for (int address = zoneInfo.start; address < zoneInfo.count + zoneInfo.start; address++) {
    tempColors[zoneInfo.strip][address] = color;
  }
}

void parseIncoming(int packetSize) {
  if (packetSize == 7) {   // light ball information (x, y, radius, red, green, blue, halfLifeInt)
    Serial.println("Ball");
    lightMode = BALL_DRAG;
    Location location = {Wire.read(), Wire.read()};
    uint8_t radius = Wire.read();
    rgb_color color = {Wire.read(), Wire.read(), Wire.read()};
    halfLife = Wire.read() / 128.0; // highest = 2.0
    for (int stripInd = 0; stripInd < ledStripCount; stripInd++) {
      lightStrips2d[stripInd].setBall(location, radius, color);
    }
  } else if (packetSize == lightProgramPacketSize) {
    lightMode = PROGRAM;

    for (int disposalZoneIndex = 0; disposalZoneIndex < lowZone; disposalZoneIndex++) {
      for (int color = 0; color < 3; color++) {
        uint8_t _ = Wire.read();
      }
    }
    for (int zoneIndex = lowZone; zoneIndex <= highZone; zoneIndex++) {
      // Can't simply do the following because of reordering of colors
      // rgb_color color = {Wire.read(), Wire.read(), Wire.read()};
      rgb_color color;
      color.red = Wire.read();
      color.green = Wire.read();
      color.blue = Wire.read();
      writeEntireZoneBuffer(zoneIndex, color);
    }
    while(Wire.available()) Wire.read();

    writeToStrips();
  } else { Serial.print("Incoming: "); Serial.println(packetSize); }
}

void writeToStrips() {
  for (int stripInd = 0; stripInd < ledStripCount; stripInd++) {
    ledStrips[stripInd]->write(tempColors[stripInd], stripAddressCounts[stripInd]);
  }
}

void igniteLightProgram() {
  lightMode = PROGRAM;
}

