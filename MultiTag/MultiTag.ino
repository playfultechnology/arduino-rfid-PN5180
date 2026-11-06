/**
 * Example of reading multiple simultaneous RFID cards using PN5180 reader
 */
 
// INCLUDES
#include <PN5180.h>
#include <PN5180ISO15693.h>
// OLED display. 
#include <U8g2lib.h>

// CONSTANTS
#if defined(ESP8266)
  const byte PN5180_NSS = D8;
  const byte PN5180_BUSY = D0;
  const byte PN5180_RST = D4;
#elif defined(ESP32)
  const byte PN5180_NSS = 5;
  const byte PN5180_BUSY = 16;
  const byte PN5180_RST = 17;
#endif
const uint8_t maxTags = 16;

// GLOBALS
PN5180ISO15693 nfc(PN5180_NSS, PN5180_BUSY, PN5180_RST);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ D1, /* data= */ D2);
uint8_t lastUid[8*maxTags];

void setup() {
  // Serial interface setup  
  Serial.begin(115200);
  Serial.println(__FILE__ __DATE__);

  // OLED display  
  u8g2.setI2CAddress(0x3C<<1);
  u8g2.begin();
  u8g2.clearBuffer();					// clear the internal memory
  u8g2.setFont(u8g2_font_ncenB08_tr);	// choose a suitable font
  u8g2.drawStr(0,10,"Multitag RFID Demo");	// write something to the internal memory
  u8g2.sendBuffer();					// transfer internal memory to the display

  // PN5180 setup
  nfc.begin();
  delay(100);
  nfc.reset();
  delay(100);
  nfc.setupRF();
}


void loop() {
  uint8_t numCardsInRange = 0;
  uint8_t currentUid[8*maxTags];

  // Try to read tags in range (or "get inventory" in ISO15693-speak) 
  ISO15693ErrorCode rc = nfc.getInventoryMultiple(currentUid, maxTags, &numCardsInRange);

  if (rc == ISO15693_EC_OK) {

    u8g2.clearBuffer();					// clear the internal memory
    u8g2.setFont(u8g2_font_ncenB08_tr);	// choose a suitable font
    char buffer[20];
    snprintf(buffer, 20, "%u cards in range", numCardsInRange);
    u8g2.drawStr(0,10,buffer);	// write something to the internal memory

    for(int i=0; i<numCardsInRange; i++){
		char Uid[16];
        memset(Uid, 0, sizeof Uid);
        snprintf(Uid, 16, "%02x%02x%02x%02x%02x%02x%02x%02x", currentUid[i*8+7], currentUid[i*8+6], currentUid[i*8+5], currentUid[i*8+4], currentUid[i*8+3], currentUid[i*8+2], currentUid[i*8+1], currentUid[i*8+0] );
        Serial.print(i);
        Serial.print(":");
        Serial.println(Uid);
        u8g2.drawStr(0, (10*i)+20, Uid);
      }
      // Update the stored UID array
      memcpy(lastUid, currentUid, 8*maxTags);
    u8g2.sendBuffer();
  }
}