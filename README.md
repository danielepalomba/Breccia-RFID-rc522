# Breccia-RFID-rc522

## Required Components
- Arduino board
- rc522 RFID module
- 1 or + MIFARE 1K Keys
- Relay module
- Active buzzer

*If you do not use a relay module, take care to include a diode and transistor in your circuit to prevent eddy currents from damaging the arduino board. In case you use a relay on a pre-fabricated board, then you will have no prob .*

## Connection diagram 


| RIFD | Arduino |
| ---- |---------|
| SDA  | D10     |
| SCK  | D13     |
| MOSI | D11     |
| MISO | D12     |
| GND  | GND     |
| RST  | 9       |
| VCC  | 3.3V    |

![Connection diagram](img/RC522-RFID-Module-Pin-Diagram-Pinout.png)

| Relay board | Arduino |
| ----------- |:------- |
| Signal      | D4      |
| VCC         | 5V      |
| GND         | GND     |



| A. Buzzer | Arduino |
|:--------- | ------- |
| VCC (+)   | D3      |
| GND       | GND     |

The rc522 module communicates with the arduino via SPI protocol. Make sure you use the correct pins for your board, in my case, a UNO R3, the pins set up for the SPI protocol are:

- D10(CS),
- D11(MOSI)
- D12(MISO)
- D13(SCK) 

**Searching online you might come across the terms, COPI & CIPO. Don't worry, they correspond to (deprecated) MOSI & MISO.**

![Connection diagram](img/connection.png)

The project also includes .stl files to print the boxes that will contain the Arduino and the RFID module. 

![Connection diagram](img/3dbox.jpg)

## About MIFARE Classic 1K
The rc522 module is usually used to read MIFARE Classic 1K cards, among the most common ones. We understand more about how they work and how we can write and read data from these cards to further customise our model.
These cards have a 1024-byte EEPROM memory, organised in 16 sectors (in the code we save the name of the key owner in sector 4). The last block in each sector is the *Sector trailer*, which contains the authentication keys and access permissions.
The operating frequency of these keys is 13.56 MHz, the standard. They use the ISO/IEC 14443-A protocol, which enables fast and secure communications.
These keys support a key authentication system (A and B). We will use only key A.

## Security-related aspects

Changing the default key “FFFFFFFFFF” is the critical first step in securing our system. Otherwise, anyone could access the data on the card.

It is important to point out that MIFARE 1K cards use Crypto-1 encryption technology, which is considered **weak**. Therefore, our system will be inherently weak. However, we can implement security measures to make it somewhat more reliable.

We specifically will implement:
- Custom security key A
- Disable security key B
- Adding a unique code in addition to just the uid of the RFID card.

Below is the function to be able to change the default key A

```cpp=
/**
 * 
 * 
 * @param oldKey Current key
 * @param newKey New key
 * @return true if all changes were successful, false otherwise
 */
bool changeKeysForAllSectors(MFRC522::MIFARE_Key &oldKey, MFRC522::MIFARE_Key &newKey) {
  for (byte sector = 0; sector < 16; sector++) {
    byte trailerBlock = sector * 4 + 3;
    byte buffer[18];

    if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &oldKey, &(mfrc522.uid)) != MFRC522::STATUS_OK) {
      Serial.print("Auth failed() - sector: ");
      Serial.println(sector);
      return false;
    }

    byte size = sizeof(buffer);
    if (mfrc522.MIFARE_Read(trailerBlock, buffer, &size) != MFRC522::STATUS_OK) {
      Serial.print("Trailer block reading failed by sector: ");
      Serial.println(sector);
      return false;
    }

    for (byte i = 0; i < 6; i++) {
      buffer[i] = newKey.keyByte[i];
    }

    if (mfrc522.MIFARE_Write(trailerBlock, buffer, 16) != MFRC522::STATUS_OK) {
      Serial.print("Writing the new failed key by sector: ");
      Serial.println(sector);
      return false;
    }

    Serial.print("Key changed by sector: ");
    Serial.println(sector);
  }

  return true;
}
```

Code to disable key B for each sector

```cpp
/**
 * Disables the B key for all sectors of the board.
 * 
 * @return true if disabling is successful, false otherwise.
 */
bool disableKeyBForAllSectors() {
  for (byte sector = 0; sector < 16; sector++) {
    byte trailerBlock = sector * 4 + 3; 
    byte buffer[18];

    if (mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &newKey, &(mfrc522.uid)) != MFRC522::STATUS_OK) {
      Serial.print("Authentication failed by sector: ");
      Serial.println(sector);
      return false;
    }

    for (byte i = 0; i < 6; i++) {
      buffer[i] = newKey.keyByte[i];     // key A
      buffer[i + 10] = 0x00; // disable key B
    }

    buffer[6] = 0xFF; // Full access to key A
    buffer[7] = 0x07; // Full access
    buffer[8] = 0x80; // Access mode for key A
    buffer[9] = 0x69; // Other access bits

    if (mfrc522.MIFARE_Write(trailerBlock, buffer, 16) != MFRC522::STATUS_OK) {
      Serial.print("Error in disabling key B by sector: ");
      Serial.println(sector);
      return false;
    }

    Serial.print("Key B disabled by sector: ");
    Serial.println(sector);
  }

  return true;
}
```

These functions are found in the *setupCard.ino* file. File that allows the initialization of a new card. All you have to do is provide a new unique string of 16 characters and enter the name of the card.

