/**
 * Multi-PN5180 
 * An "escape room"-style puzzle in which a number of RFID tags must be placed in front 
 * of the correct PN5180 RFID readers in order to be detected and solve the puzzle.
 */

// DEFINES
// #define DEBUG 

// INCLUDES
// Download from https://github.com/playfultechnology/PN5180-Library
#include <PN5180.h>
#include <PN5180ISO15693.h>

// CONSTANTS
// The number of PN5180 readers connected
const byte numReaders = 2;
// What is the "correct" UID that should be detected by each reader
uint8_t correctUid[][8] = {
  {0xD1,0xD2,0x48,0x2A,0x50,0x1,0x4,0xE0},
  {0xB,0x8A,0xC6,0x6A,0x0,0x1,0x4,0xE0}
};
// This pin will be driven LOW when the puzzle is solved
const byte relayPin = A0;

// GLOBALS
// Each PN5180 reader requires unique NSS, BUSY, and RESET pins,
// as defined in the constructor below
PN5180ISO15693 nfc[] = {
  PN5180ISO15693(9,7,5),
  PN5180ISO15693(10,8,6),
};
// Array to record the value of the last UID read by each reader
uint8_t lastUid[numReaders][8];

void setup() {
  // Configure the relay pin
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);
  
  // Initialise serial connection
  Serial.begin(115200);

  for(int i=0; i<numReaders; i++){
    Serial.print("Reader #");
    Serial.println(i);
    Serial.println(F("Initialising..."));
    nfc[i].begin();
    Serial.println(F("Resetting..."));
    nfc[i].reset();
    Serial.println(F("Enabling RF field..."));
    nfc[i].setupRF();
  }
  Serial.println(F("Setup Complete"));
}

void loop() {

  for(int i=0; i<numReaders; i++) {
    // Variable to store the ID of any tag read by this reader
    uint8_t thisUid[8];
    // Try to read a tag ID (or "get inventory" in ISO15693-speak) 
    ISO15693ErrorCode rc = nfc[i].getInventory(thisUid);
    // If the result code was that a card had been read
    if(rc == ISO15693_EC_OK) {
      // If this is the same ID as we read last frame
      if(memcmp(thisUid, lastUid[i], 8) == 0) {
        // Nothing to do - move on to the next reader
        continue;
      }
      // If it's a different ID
      else {
        Serial.print(F("New Card Detected on Reader "));
        Serial.print(i);
        Serial.print(F("... "));
        for (int j=0; j<sizeof(thisUid); j++) {
          Serial.print(thisUid[j],HEX);
          Serial.print(" ");
        }
        Serial.println();
        // Update the array that keeps track of most recent ID
        memcpy(lastUid[i], thisUid, sizeof(lastUid[i][0])*8);

        // Has placing this card solved the puzzle?
        checkIfPuzzleSolved();
      }
    }
    // If a card cannot be read
    else {
      // Test if we previously knew about a card (in which case it's just been removed
      // The most significant (last) byte of a valid UID should always be 0xE0. e.g. E007C4A509C247A8
      if(lastUid[i][7] == 0xE0){
        Serial.print("Card ");
        for (int j=0; j<sizeof(lastUid[i]); j++) {
          Serial.print(lastUid[i][j], HEX);
        }
        Serial.print(" removed from Reader ");
        Serial.println(i);
        // Update the array that keeps track of last known ID
        memset(lastUid[i], 0, sizeof(lastUid[i][0])*8);
      }
      
      #ifdef DEBUG
        Serial.print(F("Error in getInventory: "));
        Serial.println(nfc[i].strerror(rc));
      #endif
    }
 
    // Slight delay before checking the next reader
    delay(100);
  }
}

// Action to take when the puzzle is solved
void onPuzzleSolved() {
  // Activate the relay
  digitalWrite(relayPin, LOW);
  // Loop forever
  while(true) { delay(1000); }
}

// Check whether all PN5180s have detected the correct tag
void checkIfPuzzleSolved() {
  // Test each reader in turn
  for(int i=0; i<numReaders; i++){
    // If this reader hasn't detected the correct tag
    if(memcmp(lastUid[i], correctUid[i], 8) != 0){
      // Exit
      return false;
    }
  }
  onPuzzleSolved();
}



