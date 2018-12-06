# arduino-rfid-PN5180
Interfacing Arduino with 13.56MHz ISO15693 RFID tags using PN5180 modules
![PN5180 module](https://github.com/playfultechnology/arduino-rfid-RDM6300/raw/master/documentation/RDM6300.jpg)

The PN5180 reader uses the ISO15693 "vicinity card" protocol, which offers a significantly longer read range than the ISO14443 "proximity card" protocol seen in other common RFID readers such as MFRC522 or PN532. It uses SPI interface, which can be shared between a number of readers. Each reader also requires their own slave select, reset, and busy lines.
For a comparison of alternative readers, see https://www.patreon.com/posts/rfid-roundup-23115452

# Components
- [PN5180 Module](https://www.aliexpress.com/item/PN5180-NFC-IC-ISO15693-RFID-SLIX-ISO-IEC-18092-14443-A-B-Read-Write-module/32840851498.html) ~£8
- [Arduino](https://www.banggood.com/ATmega328P-Nano-V3-Controller-Board-Compatible-Arduino-p-940937.html) ~£3

# Wiring
There are 13 pins on the 5180 board, as follows:
- +5V : +5V DC supply
- +3.3V : +3.3V DC supply 
- RST : Reset line from Arduino *
- NSS : (Not) Slave Select is pulled LOW when communicating with Arduino *  
- MOSI : Master-Out, Slave-In used for transmitting data from Arduino to the reader *
- MISO : Master-In, Slave-Out used for transmitting data from the reader to the Arduino
- SCK : System Clock signal sent from Arduino *
- BUSY : Pulled high when the reader is in a busy state
- GND : Ground
- GPIO : Can be used as general-purpose output pin from the reader (e.g. to power an LED)
- IRQ : Not required
- AUX : Not required
- REQ : Not required


![PN5180 to Arduino using SPI](https://raw.githubusercontent.com/playfultechnology/arduino-rfid-PN5180/master/documentation/PN5180_bb.jpg)
