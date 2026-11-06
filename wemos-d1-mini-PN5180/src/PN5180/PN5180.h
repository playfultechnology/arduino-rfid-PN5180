#ifndef PN5180_H
#define PN5180_H

#include <Arduino.h>
#include <SPI.h>

// PN5180 Commands
// PN5180 defines
// See https://www.nxp.com/docs/en/data-sheet/PN5180A0XX-C1-C2.pdf 
// 11.4.3.3 Host Interface Command List
#define PN5180_WRITE_REGISTER           (0x00)
#define PN5180_WRITE_REGISTER_OR_MASK   (0x01)
#define PN5180_WRITE_REGISTER_AND_MASK  (0x02)
#define PN5180_READ_REGISTER            (0x04)
#define PN5180_WRITE_EEPROM             (0x06)
#define PN5180_READ_EEPROM              (0x07)
#define PN5180_SEND_DATA                (0x09)
#define PN5180_READ_DATA                (0x0A)
#define PN5180_LOAD_RF_CONFIG           (0x11)
#define PN5180_RF_ON                    (0x16)
#define PN5180_RF_OFF                   (0x17)
// 11.9.1, Table 73 PN5180 Register Address Overview
#define SYSTEM_CONFIG       (0x00)
#define IRQ_ENABLE          (0x01)
#define IRQ_STATUS          (0x02)
#define IRQ_CLEAR           (0x03)
#define RX_STATUS           (0x13)
#define TX_WAIT_CONFIG      (0x17)
#define TX_CONFIG           (0x18)
// 11.9.1, Table 76 IRQ_STATUS Register
#define RX_IRQ_STAT         	(1<<0)  // End of RF reception IRQ
#define TX_IRQ_STAT         	(1<<1)  // End of RF transmission IRQ
#define IDLE_IRQ_STAT       	(1<<2)  // Idle IRQ
#define RFOFF_DET_IRQ_STAT  	(1<<6)  // RF Field OFF detection IRQ
#define RFON_DET_IRQ_STAT   	(1<<7)  // RF Field ON detection IRQ
#define TX_RFOFF_IRQ_STAT   	(1<<8)  // RF Field OFF in PCD IRQ
#define TX_RFON_IRQ_STAT    	(1<<9)  // RF Field ON in PCD IRQ
// 11.9.1 Table 92 RX_STATUS Register
#define RX_COLL_POS (1<<19) // These bits show the bit position of the first detected collision in a received frame (7 bits)
#define RX_COLLISION_DETECTED (1<<18) // This flag is set to 1, when a collision has occurred 
// 11.9.1 Table 97 TX_CONFIG Register
#define TX_DATA_ENABLE (1<<10) // If set to 1, transmission of data is enabled otherwise only symbols are transmitted.

// The PN5180 receive buffer can hold a max of 508 bytes
// But we only need to transfer 2 bytes + 8 bytes per tag = 10 bytes for a single card
#ifndef READ_BUFFER_SIZE
	#define READ_BUFFER_SIZE 10  
#endif
// Other constants and enums
enum ISO15693ErrorCode {
  EC_NO_CARD = -1,
  ISO15693_EC_OK = 0,
  ISO15693_EC_NOT_SUPPORTED = 0x01,
  ISO15693_EC_NOT_RECOGNIZED = 0x02,
  ISO15693_EC_OPTION_NOT_SUPPORTED = 0x03,
  ISO15693_EC_UNKNOWN_ERROR = 0x0F,
  ISO15693_EC_BLOCK_NOT_AVAILABLE = 0x10,
  ISO15693_EC_BLOCK_ALREADY_LOCKED = 0x11,
  ISO15693_EC_BLOCK_IS_LOCKED = 0x12,
  ISO15693_EC_BLOCK_NOT_PROGRAMMED = 0x13,
  ISO15693_EC_BLOCK_NOT_LOCKED = 0x14,
  ISO15693_EC_CUSTOM_CMD_ERROR = 0xA0
};

class PN5180 {
public:
  PN5180(uint8_t nssPin, uint8_t busyPin, uint8_t rstPin);
  void begin();
  void hardReset();
  bool getInventory(uint8_t *uid);
  void loadISO15693config();
  bool activateRF();
  bool disableRF();
  bool checkIdle();
  void clearIRQ();
  void setIdle();
  void activateTransceive();
  void sendInventoryCmd();
  void sendEndOfFrame();
  bool readReceptionBuffer(uint8_t *buffer, int16_t len);
  uint32_t readRegister(uint8_t regAddress);
  bool sendBytes(uint8_t *sendBuffer, size_t sendBufferLen);
  bool readBytes(uint8_t *recvBuffer, size_t recvBufferLen);

private:
  uint8_t _nss, _busy, _rst;
  SPISettings _spiSettings;
  uint16_t _commandTimeout = 800;
  bool _spiFailed = false;
  bool waitUntilAvailable();
  bool waitUntilBusy();
};

#endif
