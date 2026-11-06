#include "PN5180.h"

// Based on Playful Technology PN5180 1.0.0. See LICENSE in this folder.
namespace {
// Set to false to restore the previous 16-slot inventory.
constexpr bool singleSlotInventory = true;
constexpr byte inventorySlotCount = singleSlotInventory ? 1 : 16;
constexpr byte inventoryFlags = singleSlotInventory ? 0x26 : 0x06;
constexpr unsigned int spiGuardUs = 100;
constexpr unsigned long tagPowerUpMs = 40;
constexpr unsigned long slotResponseMs = 10;
constexpr uint32_t rxErrorMask = (7UL << 16) | (7UL << 13);
}

PN5180::PN5180(uint8_t nssPin, uint8_t busyPin, uint8_t rstPin) 
  : _nss(nssPin), _busy(busyPin), _rst(rstPin), _spiSettings(125000, MSBFIRST, SPI_MODE0) {}

/***************************
// INITIALISATION AND CONFIGURATION
***************************/

// Startup procedure
void PN5180::begin() {
  pinMode(_nss, OUTPUT);
  digitalWrite(_nss, HIGH);
  pinMode(_rst, OUTPUT);
  digitalWrite(_rst, HIGH);
  pinMode(_busy, INPUT);
  SPI.begin();
  hardReset();
}

// Hard reset
void PN5180::hardReset() {
  digitalWrite(_rst, LOW);
  delay(10);
  digitalWrite(_rst, HIGH);
  delay(50);
}

// Load the ISO15693 Tx/Rx protocol into the RF registers
void PN5180::loadISO15693config(){
  uint8_t cmd[] = {PN5180_LOAD_RF_CONFIG, 0x0D, 0x8D};
  sendBytes(cmd, sizeof(cmd));
}

// Set the PN5180 into IDLE state
void PN5180::setIdle(){
	uint8_t cmd[] = {PN5180_WRITE_REGISTER_AND_MASK, SYSTEM_CONFIG, 0xF8, 0xFF, 0xFF, 0xFF};
  sendBytes(cmd, sizeof(cmd));
}

// Activate TRANSCEIVE routine
void PN5180::activateTransceive(){   
	uint8_t cmd[] = {PN5180_WRITE_REGISTER_OR_MASK, SYSTEM_CONFIG, 0x03, 0x00, 0x00, 0x00};
  sendBytes(cmd, sizeof(cmd));
}

/***************************
// SPI COMMUNICATION
***************************/

// Recommended SPI transceive routine with BUSY handling
// As described in 11.4.1 Physical Host Interface
// https://www.nxp.com/docs/en/data-sheet/PN5180A0XX-C1-C2.pdf
bool PN5180::sendBytes(uint8_t *sendBuffer, size_t sendBufferLen){
  if (_spiFailed) return false;
  // 0. Wait until PN5180 is not busy
  if(!waitUntilAvailable()){ 
    _spiFailed = true;
    Serial.print("SPI send error 0");
    digitalWrite(_nss, HIGH);
    return false;
  }
  // 1. Assert NSS LOW
  digitalWrite(_nss, LOW); delayMicroseconds(spiGuardUs);
  // 2. Perform data exchange
  SPI.beginTransaction(_spiSettings);
  SPI.transfer((uint8_t*)sendBuffer, sendBufferLen);
  // 3. Wait for BUSY to go HIGH
  if(!waitUntilBusy()){ 
    _spiFailed = true;
    Serial.print("SPI send error 3");
    SPI.endTransaction();
    digitalWrite(_nss, HIGH);
    return false;
  }
  // 4. Deassert NSS
  digitalWrite(_nss, HIGH); delayMicroseconds(spiGuardUs);
  // 5. Wait until BUSY goes LOW
  if(!waitUntilAvailable()){ 
    _spiFailed = true;
    Serial.print("SPI send error 5");
    SPI.endTransaction();
    digitalWrite(_nss, HIGH);
    return false;
  }
  // Cleanup
  SPI.endTransaction();
  digitalWrite(_nss, HIGH);
  return true;
}

bool PN5180::readBytes(uint8_t *recvBuffer, size_t recvBufferLen){
  // Clear buffer
  memset(recvBuffer, 0x00, recvBufferLen);
  if (_spiFailed) return false;
  // 0. Wait until PN5180 is not busy
  if(!waitUntilAvailable()){ 
    _spiFailed = true;
    Serial.print("SPI receive error 0");
    digitalWrite(_nss, HIGH);
    return false;
  }
  // 1. Assert NSS low
  digitalWrite(_nss, LOW); delayMicroseconds(spiGuardUs);
  // 2. Perform data exchange
  SPI.beginTransaction(_spiSettings);
  SPI.transfer(recvBuffer, recvBufferLen);
  // 3. Wait until BUSY is HIGH
  if(!waitUntilBusy()){ 
    _spiFailed = true;
    Serial.print("SPI receive error 3");
    SPI.endTransaction();
    digitalWrite(_nss, HIGH);
    return false;
  }
  // 4. Deassert NSS
  digitalWrite(_nss, HIGH); delayMicroseconds(spiGuardUs);
  // 5. Wait until BUSY is LOW
  if(!waitUntilAvailable()){ 
    _spiFailed = true;
    Serial.print("SPI receive error 5");
    SPI.endTransaction();
    return false;
  }
  SPI.endTransaction();
  return true;
}

/***************************
// REGISTER MANIPULATION
***************************/

// Read 4-byte value from specified register
uint32_t PN5180::readRegister(uint8_t regAddress){
  uint8_t cmd[] = {PN5180_READ_REGISTER, regAddress};
  sendBytes(cmd, sizeof(cmd));
  uint32_t regValue;
  if (!readBytes((uint8_t*)(&regValue), 4)) return 0;
  return regValue;
}

// Clear the IRQ_STATUS register
void PN5180::clearIRQ(){
  uint8_t cmd[] = {PN5180_WRITE_REGISTER, IRQ_CLEAR, 0xFF, 0xFF, 0x0F, 0x00};
  sendBytes(cmd, sizeof(cmd));
}

/***************************
// RF FIELD MANAGEMENT
***************************/
// Switch the RF field ON
bool PN5180::activateRF(){
  uint8_t cmd[] = {PN5180_RF_ON, 0x00};
  sendBytes(cmd, sizeof(cmd));
  // Check to ensure the RF_ON bit has been activated
  unsigned long startedWaiting = millis();
  while (!(readRegister(IRQ_STATUS) & TX_RFON_IRQ_STAT)) { 
    if (_spiFailed || millis() - startedWaiting > 500) {
      return false; 
    }
  };
  // And now we can clear the RF bit again
  uint32_t regValue = TX_RFON_IRQ_STAT;
  // Use bitshifting/masking to separate 32bit value into 4 separate byte values.
  uint8_t cmdClear[] = {PN5180_WRITE_REGISTER, IRQ_CLEAR, byte(regValue), byte(regValue >> 8), byte(regValue >> 16), byte(regValue >> 24)};
  sendBytes(cmdClear, sizeof(cmdClear));
  return true;
}
// Switch off RF field
bool PN5180::disableRF(){
  uint8_t cmd[] = {PN5180_RF_OFF, 0x00};
  sendBytes(cmd, sizeof(cmd));
  // Wait for RF field to shut down
  unsigned long startedWaiting = millis();
  while (!(readRegister(IRQ_STATUS) & TX_RFOFF_IRQ_STAT)) { 
    if (_spiFailed || millis() - startedWaiting > 500) {
      return false;
    }
  };
  // And now clear the RF IRQ bit again
  uint32_t regValue = TX_RFOFF_IRQ_STAT;
  // Register value is 32bit, which we need to send as 4 separate byte values.
  uint8_t *regBytes =  (uint8_t*)(&regValue);
  uint8_t cmdClear[] = {PN5180_WRITE_REGISTER, IRQ_CLEAR, regBytes[0], regBytes[1], regBytes[2], regBytes[3]};
  sendBytes(cmdClear, sizeof(cmdClear));
  return true;
}

/**************************
// ISO15693 Command Handling
***************************/

// Send ISO15693 Inventory with the selected slot mode and an empty mask.
void PN5180::sendInventoryCmd(){
  uint8_t cmd[] = {PN5180_SEND_DATA, 0x00, inventoryFlags, 0x01, 0x00};
  sendBytes(cmd, sizeof(cmd));
}

// As per 4.2.2. ISO/IEC 15693 - Inventory
// https://www.nxp.com/docs/en/application-note/AN12650.pdf
bool PN5180::getInventory(uint8_t *uid) {
  _spiFailed = false;
  loadISO15693config();
  if (!activateRF()) {
    hardReset();
    return false;
  }

  // Keep card power-up and over-the-air response time separate from SPI timing.
  delay(tagPowerUpMs);
  setIdle();
  activateTransceive();
  clearIRQ();
  sendInventoryCmd();

  bool tagDetected = false;
  uint8_t response[10];  // Flags, DSFID, and the eight UID bytes.
  for (byte slot = 0; slot < inventorySlotCount && !_spiFailed; ++slot) {
    delay(slotResponseMs);

    if (readRegister(IRQ_STATUS) & RX_IRQ_STAT) {
      const uint32_t rxStatus = readRegister(RX_STATUS);
      const uint16_t length = rxStatus & 0x1FF;
      if (length == sizeof(response) && !(rxStatus & rxErrorMask) &&
          readReceptionBuffer(response, length) && response[0] == 0) {
        tagDetected = true;
        break;
      }
    }

    if (slot + 1 < inventorySlotCount) {
      setIdle();
      activateTransceive();
      clearIRQ();
      sendEndOfFrame();
    }
  }

  if (!disableRF() || _spiFailed) {
    hardReset();
    return false;
  }
  if (tagDetected) memcpy(uid, &response[2], 8);
  return tagDetected;
}

// Send EOF symbol
void PN5180::sendEndOfFrame(){
  // Set config to only transmit end of frame symbol without any data at next transmission
  uint8_t cmdcfg[] = {PN5180_WRITE_REGISTER_AND_MASK, TX_CONFIG, 0b00111111, 0b11111011, 0xFF, 0xFF};
  sendBytes(cmdcfg, sizeof(cmdcfg));
  uint8_t cmd[] = {PN5180_SEND_DATA, 0x00};
  sendBytes(cmd, sizeof(cmd));
}

// Receive contents of the PN5180 reception buffer
bool PN5180::readReceptionBuffer(uint8_t* buffer, int16_t len) {
  uint8_t cmdRead[] = {PN5180_READ_DATA, 0x00};
  return sendBytes(cmdRead, sizeof(cmdRead)) && readBytes(buffer, len);
}

/*******************
// UTILITY FUNCTIONS
********************/

// Check if PN5180 is in IDLE state
bool PN5180::checkIdle(){
  unsigned long startedWaiting = millis();
  while ((readRegister(IRQ_STATUS) & IDLE_IRQ_STAT) == 0 ) {
    if (millis() - startedWaiting > _commandTimeout) {
      return false;
    }
  }
  return true;
}

// Wait until PN5180 is available to receive command
bool PN5180::waitUntilAvailable(){
  unsigned long startTime = millis();
  while (digitalRead(_busy) != LOW) {
    if (millis() - startTime > _commandTimeout) {
      return false;
    };
  };
  return true;
}

// Wait until PN5180 is processing
bool PN5180::waitUntilBusy(){
  unsigned long startTime = millis();
  while (digitalRead(_busy) != HIGH) {
    if (millis() - startTime > _commandTimeout) {
      return false;
    };
  };
  return true;
}
