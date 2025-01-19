#include <SHA256.h>
#include <AES.h>
#include <MFRC522.h>
#include <MFRC522Extended.h>
#include <deprecated.h>
#include <require_cpp11.h>
#include <SPI.h>

#define RST_PIN 9   // RST RC522
#define SS_PIN 10   // SDA
#define RELAY_PIN 8
#define BUZZER_PIN A5

MFRC522 rfid(SS_PIN, RST_PIN);

//KEY A
MFRC522::MIFARE_Key key = {{0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6}};

//AES KEY (you should change this for more security!)
const byte aesKey[16] = { 0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6, 0x01, 0x23,
                          0x34, 0x45, 0x56, 0x67, 0x78, 0x89, 0x9A, 0xAB };
AES128 aes;

//Auth hash storage (enter the hash you get by running the card setup)
String authorizedHashes[] = {
  "8642966f0d60111ffa9009f8f17b79b3f130636190cf0c432b65b4e18037160b"
};
int hashListSize = sizeof(authorizedHashes) / sizeof(String);

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  Serial.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
  Serial.println("Waiting...");
}

void loop() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return;
  }

  Serial.println("Card detected");

  //Read and decrypt uniqueId
  String uniqueCode = readBlock(4);
  byte decryptedUniqueId[16];
  decryptData(uniqueCode.c_str(), decryptedUniqueId);

  Serial.print("User: ");
  Serial.println(readBlock(5));

  //Read the UID of the card 
  String uid = getUID();

  String hashedUID = createHash(uid + (char*)decryptedUniqueId);
  Serial.println(hashedUID);
  
  if(checkHash(hashedUID)){
    Serial.print("Authorized");
    fireRele();
  }else{
    Serial.println("WARNING: Unknown card readed!");
    fireBuzzer(6000);
  }

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(5000);
}

/**
* @return a string containing the uid of the read card
*/
String getUID() {
  String uid = "";
  for (int i = 0; i < rfid.uid.size; i++) {
    uid += rfid.uid.uidByte[i] < 0x10 ? "0" : "";
    uid += String(rfid.uid.uidByte[i], HEX);
  }
  rfid.PICC_HaltA();
  return uid;
}

/**
* @param data string of which to create the hash
* @return the hash in string form of the string taken as input
*/
String createHash(String data) {
  SHA256 sha256;
  byte hash[32]; // SHA-256 output 32 byte
  sha256.update((const byte*)data.c_str(), data.length());
  sha256.finalize(hash, sizeof(hash));

  String hashString = "";
  for (int i = 0; i < 32; i++) {
    if (hash[i] < 0x10) {
      hashString += "0";
    }
    hashString += String(hash[i], HEX);
  }
  return hashString;
}

/**
* @param hash to compare with authorized hashes 
* @return true if the hash is authorized, false otherwise
*/
bool checkHash(String hash) {
  for (int i = 0; i < hashListSize; i++) {
    if (authorizedHashes[i] == hash) {
      return true;
    }
  }
  return false;
}

/**
* @param blockAddr memory block to read
* @return string containing the data read
*/
String readBlock(byte blockAddr) {
  if (authenticate(blockAddr)) {
    byte buffer[18];
    byte size = sizeof(buffer);

    MFRC522::StatusCode status = rfid.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
      Serial.print("Read() failed: ");
      Serial.println(rfid.GetStatusCodeName(status));
      return "";
    }

/* DEBUG ONLY
    Serial.print("Data ");
    Serial.print(blockAddr);
    Serial.print(": ");

    for (byte i = 0; i < 16; i++) {
      Serial.print(buffer[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
*/
    // Create the string
    char message[17];
    memcpy(message, buffer, 16);
    message[16] = '\0';

    return message;
  }
  return "";
}

/**
* @param blockAddr memory block to be authenticated in order to access it 
* @return true if authentication goes well, false otherwise
*/
bool authenticate(byte blockAddr) {
  MFRC522::StatusCode status = rfid.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A,
    blockAddr,
    &key,
    &(rfid.uid)
  );

  if (status != MFRC522::STATUS_OK) {
    Serial.print("Auth failed: ");
    Serial.println(rfid.GetStatusCodeName(status));
    return false;
  }
  return true;
}

void fireRele(){
  digitalWrite(RELAY_PIN, HIGH);
  delay(1000);
  digitalWrite(RELAY_PIN, LOW);
}

/**
 @param dur duration of activation
*/
void fireBuzzer(unsigned long dur) {
  unsigned long init = millis();
  while (millis() - init < dur) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}

/**
* @param cipherText text to decrypt 
* @param plainText plaintext restored to output
*/
void decryptData(const byte* cipherText, byte* plainText) {
  aes.setKey(aesKey, 16);
  aes.decryptBlock(plainText, cipherText);

  plainText[16] = '\0'; 
}
