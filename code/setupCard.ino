//cambio la chiave A
// disabilito la chiave B
//Scrivo il nome nella scheda

#include <MFRC522.h>
#include <MFRC522Extended.h>
#include <deprecated.h>
#include <require_cpp11.h>
#include <SPI.h>

#define RST_PIN 9   
#define SS_PIN 10

//mem blocks
#define UNIQUE_ID 4
#define CARD_NAME 5

MFRC522 mfrc522(SS_PIN, RST_PIN);

MFRC522::MIFARE_Key defaultKey = {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
MFRC522::MIFARE_Key newKey = {{0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6}};

void setup() {
  Serial.begin(9600);
  SPI.begin();          
  mfrc522.PCD_Init();  

  Serial.println("RFID...Do not remove the card until you are told!");
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println("Card detected!");

  if (changeKeysForAllSectors(defaultKey, newKey)) {
    Serial.println("All sectors done!");
  } else {
    Serial.println("changeKeysForAllSectors() error");
  }

    if (disableKeyBForAllSectors()) {
    Serial.println("Key B disabled for all sectors!");
  } else {
    Serial.println("Error in disabling key B.");
  }

  writeBlock(UNIQUE_ID, "dn3tj878qwddxoty"); //16 char random string 
  writeBlock(CARD_NAME, "User"); //Insert card's name here

  Serial.println("You can now remove the card");
  mfrc522.PICC_HaltA();     
  mfrc522.PCD_StopCrypto1(); 
}

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

/**
 * 
 * 
 * @param blockAddr memory block to write
 * @param message msg to save
 * @return returns true if the writing is ok, false otherwise
 */
bool writeBlock(byte blockAddr, const char* message) {
  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A,blockAddr,&newKey,&(mfrc522.uid));
  if (status == MFRC522::STATUS_OK) {
    //max 16 byte
    byte dataBlock[16];
    memset(dataBlock, 0, sizeof(dataBlock));
    strncpy((char*)dataBlock, message, 16);

    MFRC522::StatusCode writeStatus = mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
    if (writeStatus != MFRC522::STATUS_OK) {
      Serial.print("Write failed(): ");
      Serial.println(mfrc522.GetStatusCodeName(writeStatus));
      return false;
    }

    Serial.println("Write() ok");
    return true;
  }else{
    Serial.println("Write() failed");
  }
  return false;
}