/**
 * ESP32-PN5180 
 */

// DEFINES
// #define DEBUG 

// INCLUDES
// Download from https://github.com/playfultechnology/PN5180-Library
#include <PN5180.h>
#include <PN5180ISO15693.h>

// What is the "correct" UID that should be detected by the reader
uint8_t correctUid[8] = {0xD1,0xD2,0x48,0x2A,0x50,0x1,0x4,0xE0};
// This pin will be driven LOW when the puzzle is solved
const byte relayPin = 27;

// GLOBALS
// Each PN5180 reader requires unique NSS, BUSY, and RESET pins,
// as defined in the constructor below
PN5180ISO15693 nfc = PN5180ISO15693(5, 16, 17);

// Array to record the value of the last UID read by each reader
uint8_t lastUid[8];

void setup() {
  // Configure the relay pin
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
  
  // Initialise serial connection
  Serial.begin(115200);
  delay(500);

  Serial.println(F("Initialising Reader..."));
  nfc.begin();
  Serial.println(F("Resetting..."));
  nfc.reset();
  Serial.println(F("Enabling RF field..."));
  nfc.setupRF();
  Serial.println(F("Setup Complete"));
}

void loop() {
    // Variable to store the ID of any tag read by this reader
    uint8_t thisUid[8];
    // Try to read a tag ID (or "get inventory" in ISO15693-speak) 
    ISO15693ErrorCode rc = nfc.getInventory(thisUid);
    // If the result code was that a card had been read
    if(rc == ISO15693_EC_OK) {
      // If different ID than we read last frame
      if(memcmp(thisUid, lastUid, 8) != 0) {
        Serial.print(F("New Card Detected..."));
        for (int j=0; j<sizeof(thisUid); j++) {
          Serial.print(thisUid[j], HEX);
          Serial.print(" ");
        }
        Serial.println();
        // Update the array that keeps track of most recent ID
        memcpy(lastUid, thisUid, sizeof(lastUid[0])*8);

        // Has placing this card solved the puzzle?
        if(memcmp(lastUid, correctUid, 8) == 0){
          // Activate the relay
          digitalWrite(relayPin, LOW);
          // Loop forever
          while(true) { delay(1000); }
        }
      }
    }
    // If a card cannot be read
    else {
      // Test if we previously knew about a card (in which case it's just been removed
      // The most significant (last) byte of a valid UID should always be 0xE0. e.g. E007C4A509C247A8
      if(lastUid[7] == 0xE0){
        Serial.print("Card ");
        for (int j=0; j<sizeof(lastUid); j++) {
          Serial.print(lastUid[j], HEX);
        }
        Serial.println(" removed from reader");
        // Update the array that keeps track of last known ID
        memset(lastUid, 0, sizeof(lastUid[0])*8);
      }
      
      #ifdef DEBUG
        Serial.print(F("Error in getInventory: "));
        Serial.println(nfc.strerror(rc));
      #endif
    }
 
    // Slight delay before checking the next reader
    delay(10);
}