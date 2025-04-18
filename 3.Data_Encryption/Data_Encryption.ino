#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <AESLib.h>  // AES encryption library

// PN532 SPI Pins
#define PN532_SCK  (2)
#define PN532_MOSI (3)
#define PN532_SS   (4)
#define PN532_MISO (5)

Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

// AES Key and IV
byte aesKey[16] = {
  0x2b, 0x7e, 0x15, 0x16,
  0x28, 0xae, 0xd2, 0xa6,
  0xab, 0xf7, 0x97, 0x75,
  0x46, 0x67, 0x2f, 0x85
};

byte aesIV[16] = {
  0x00, 0x01, 0x02, 0x03,
  0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0A, 0x0B,
  0x0C, 0x0D, 0x0E, 0x0F
};

AESLib aesLib;

// AES Encrypt helper
void aesEncrypt(byte* inputData, byte* outputData, int length) {
  aesLib.encrypt(inputData, length, outputData, aesKey, 128, aesIV);
}

// AES Decrypt helper
void aesDecrypt(byte* inputData, byte* outputData, int length) {
  aesLib.decrypt(inputData, length, outputData, aesKey, 128, aesIV);
}

void setup(void) {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println("Initializing NFC with SPI...");
  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Didn't find PN53x board");
    while (1); // Halt
  }

  Serial.print("Found chip PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware version: ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  nfc.setPassiveActivationRetries(0xFF);
  nfc.SAMConfig(); // Enable the NFC reader

  Serial.println("Waiting for an ISO14443A card...");
}

void loop(void) {
  uint8_t uid[7];     // Buffer to store the returned UID
  uint8_t uidLength;  // Length of the UID (4 or 7 bytes)

  boolean success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  if (success) {
    Serial.println("\nFound a card!");
    Serial.print("UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i = 0; i < uidLength; i++) {
      Serial.print("0x"); Serial.print(uid[i], HEX); Serial.print(" ");
    }
    Serial.println();

    // Data to be encrypted
    byte data[] = {0x01, 0x02, 0x03, 0x04}; // 4-byte data
    byte paddedData[16] = {0}; // AES block size
    memcpy(paddedData, data, sizeof(data)); // Pad to 16 bytes

    // Encrypt
    byte encryptedData[16];
    aesEncrypt(paddedData, encryptedData, 16);

    // Write to card (block 4 is safe to write)
    uint8_t blockNumber = 4;
    success = nfc.mifareclassic_WriteDataBlock(blockNumber, encryptedData);
    if (success) {
      Serial.println("Encrypted data written successfully to the card!");
    } else {
      Serial.println("Failed to write encrypted data to the card.");
    }

    delay(1000); // Short delay before read

    // Read back the data
    byte readData[16];
    success = nfc.mifareclassic_ReadDataBlock(blockNumber, readData);
    if (success) {
      Serial.println("Data read from the card:");
      Serial.print("Encrypted (HEX): ");
      for (uint8_t i = 0; i < 16; i++) {
        Serial.print("0x"); Serial.print(readData[i], HEX); Serial.print(" ");
      }
      Serial.println();

      // Decrypt
      byte decryptedData[16];
      aesDecrypt(readData, decryptedData, 16);

      Serial.print("Decrypted (HEX): ");
      for (uint8_t i = 0; i < 16; i++) {
        Serial.print("0x"); Serial.print(decryptedData[i], HEX); Serial.print(" ");
      }
      Serial.println();

      Serial.print("Decrypted (ASCII): ");
      for (uint8_t i = 0; i < 16; i++) {
        Serial.write(decryptedData[i]);
      }
      Serial.println();

    } else {
      Serial.println("Failed to read data from the card.");
    }
  } else {
    Serial.println("Timed out waiting for a card.");
  }

  delay(3000); // Wait before trying again
}
