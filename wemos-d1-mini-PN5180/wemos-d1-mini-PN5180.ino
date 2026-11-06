// Wemos D1 Mini + PN5180 RFID reader
// Uses a sketch-local copy of PN5180 by Alastair Aitchison / Playful Technology.

#include <EEPROM.h>
#define EEPROM_SIZE 64

#include <SPI.h>
#include "src/PN5180/PN5180.h"
#include <U8g2lib.h>

// ---------- PINS ----------
// PN5180 wiring from your schematic:
// SS/NSS = D8
// BUSY   = D0
// RST    = D3
// MOSI   = D7
// MISO   = D6
// CLK    = D5

constexpr uint8_t ssPin     = D8;
constexpr uint8_t busyPin   = D0;
constexpr uint8_t rstPin    = D3;

constexpr uint8_t clkPin    = D1;
constexpr uint8_t dataPin   = D2;

constexpr uint8_t outputPin = D4;
constexpr uint8_t buttonPin = A0;

constexpr int buttonThreshold = 600;

// Your optocoupler input is wired from +3.3V through the LED/resistor
// into the Wemos pin, so the output is active LOW.
constexpr bool outputActiveLow = true;

// ---------- GLOBALS ----------
PN5180 pn5180(ssPin, busyPin, rstPin);

U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(
  U8G2_R0, U8X8_PIN_NONE, clkPin, dataPin
);

// PN5180 ISO15693 UID is 8 bytes.
byte currentUID[8] = {0};
byte currentUIDSize = 0;

bool tagPresent = false;
bool outputActive = false;
unsigned long outputEnd = 0;

// Avoid flickering removal state on occasional missed reads.
byte missedReadCount = 0;
constexpr byte maxMissedReadsBeforeRemoved = 1;


// ---------- OUTPUT HELPERS ----------
void setOutput(bool active) {
  if (outputActiveLow) {
    digitalWrite(outputPin, active ? LOW : HIGH);
  } else {
    digitalWrite(outputPin, active ? HIGH : LOW);
  }
}

void pulseOutput(uint16_t durationMs = 10) {
  setOutput(true);
  outputEnd = millis() + durationMs;
  outputActive = true;
}


// ---------- UID HELPERS ----------
void uidToString(const byte *uid, byte size, char *out) {
  for (byte i = 0; i < size; i++) {
    sprintf(&out[i * 2], "%02X", uid[i]);
  }
  out[size * 2] = '\0';
}

void printUID(const byte *uid, byte size) {
  for (byte i = 0; i < size; i++) {
    if (uid[i] < 0x10) Serial.print("0");
    Serial.print(uid[i], HEX);
  }
  Serial.println();
}

bool uidEquals(const byte *a, byte aSize, const byte *b, byte bSize) {
  if (aSize != bSize) return false;
  return memcmp(a, b, aSize) == 0;
}


// ---------- OLED HELPERS ----------
void showUIDOnOLED(const byte *uid, byte size) {
  char tagStr[17];  // 8-byte UID => 16 hex chars + NUL
  uidToString(uid, size, tagStr);

  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 10, 128, 22);
  u8g2.setDrawColor(1);

  u8g2.setFont(u8g2_font_t0_15b_mr);
  u8g2.drawStr(0, 32, tagStr);
  // The tag occupies the bottom three 8-pixel tile rows.
  u8g2.updateDisplayArea(0, 1, 16, 3);
}

void clearOLEDTagArea() {
  u8g2.setDrawColor(0);
  u8g2.drawBox(0, 10, 128, 22);
  u8g2.setDrawColor(1);
  u8g2.updateDisplayArea(0, 1, 16, 3);
}


// ---------- PN5180 RFID ----------
bool readCurrentUID(byte *uidOut, byte &sizeOut) {
  uint8_t rawUID[8];

  if (!pn5180.getInventory(rawUID)) {
    return false;
  }

  // The PN5180 library stores ISO15693 UID bytes LSB-first.
  // Reverse them so display/EEPROM use the normal printed UID order.
  for (byte i = 0; i < 8; i++) {
    uidOut[i] = rawUID[7 - i];
  }

  sizeOut = 8;
  return true;
}


// ---------- EEPROM ----------
bool isSameAsStored() {
  byte storedSize = EEPROM.read(0);
  if (storedSize != currentUIDSize) return false;

  for (byte i = 0; i < storedSize; i++) {
    if (EEPROM.read(1 + i) != currentUID[i]) return false;
  }

  return true;
}

void saveUIDToEEPROM(const byte *uid, byte size) {
  EEPROM.write(0, size);

  for (byte i = 0; i < size; i++) {
    EEPROM.write(1 + i, uid[i]);
  }

  EEPROM.commit();
}

void printStoredUIDFromEEPROM() {
  byte storedSize = EEPROM.read(0);

  if (storedSize == 0 || storedSize > 8) {
    Serial.println("No UID stored");
    return;
  }

  byte storedUID[8];

  for (byte i = 0; i < storedSize; i++) {
    storedUID[i] = EEPROM.read(1 + i);
  }

  Serial.print("Stored UID: ");
  printUID(storedUID, storedSize);
}


// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);
  delay(200);

  Serial.println();
  Serial.println(__FILE__ " " __DATE__);
  Serial.println("PN5180 RFID reader");

  // Make sure the optocoupler output is inactive during startup.
  digitalWrite(outputPin, outputActiveLow ? HIGH : LOW);
  pinMode(outputPin, OUTPUT);
  setOutput(false);

  pinMode(buttonPin, INPUT);

  SPI.begin();
  pn5180.begin();

  u8g2.setI2CAddress(0x3C << 1);
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x7_mr);
  u8g2.drawStr(0, 8, "Playful Technology");
  u8g2.sendBuffer();

  EEPROM.begin(EEPROM_SIZE);
  printStoredUIDFromEEPROM();
}


// ---------- LOOP ----------
void loop() {
  byte newUID[8];
  byte newUIDSize = 0;

  bool readOK = readCurrentUID(newUID, newUIDSize);

  if (readOK) {
    missedReadCount = 0;

    bool changed = !tagPresent ||
                   !uidEquals(newUID, newUIDSize, currentUID, currentUIDSize);

    if (changed) {
      memcpy(currentUID, newUID, newUIDSize);
      currentUIDSize = newUIDSize;
      tagPresent = true;

      // Signal a match before serial logging and the OLED transfer.
      if (isSameAsStored()) {
        pulseOutput(10);
      }

      Serial.print("UID: ");
      printUID(currentUID, currentUIDSize);

      showUIDOnOLED(currentUID, currentUIDSize);
    } else {
      tagPresent = true;
    }
  } else {
    if (tagPresent) {
      missedReadCount++;

      if (missedReadCount >= maxMissedReadsBeforeRemoved) {
        Serial.println("Tag removed");
        clearOLEDTagArea();

        tagPresent = false;
        currentUIDSize = 0;
        missedReadCount = 0;
      }
    }
  }

  // Button on A0: save currently detected UID.
  static bool buttonHandled = false;
  int buttonValue = analogRead(buttonPin);

  if (buttonValue > buttonThreshold && !buttonHandled) {
    buttonHandled = true;

    Serial.print("Button pressed: ");
    Serial.println(buttonValue);

    if (tagPresent && currentUIDSize > 0) {
      if (!isSameAsStored()) {
        saveUIDToEEPROM(currentUID, currentUIDSize);
        Serial.println("UID saved to EEPROM");
      } else {
        Serial.println("UID already stored");
      }

      pulseOutput(50);
    } else {
      Serial.println("No UID to save");
    }
  }

  if (buttonValue < buttonThreshold) {
    buttonHandled = false;
  }

  // Stop output pulse.
  if (outputActive && millis() > outputEnd) {
    setOutput(false);
    outputActive = false;
  }

}
