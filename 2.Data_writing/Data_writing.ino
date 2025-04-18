#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

#define PN532_SCK  (2)
#define PN532_MOSI (3)
#define PN532_SS   (4)
#define PN532_MISO (5)

Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

void setup(void) {
  Serial.begin(115200);
  while (!Serial) delay(10);
  Serial.println("Hello!");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Didn't find PN53x board");
    while (1);
  }

  Serial.print("Found chip PN5");
  Serial.println((versiondata >> 24) & 0xFF, HEX);
  Serial.print("Firmware ver. ");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);

  nfc.setPassiveActivationRetries(0xFF);
  nfc.SAMConfig();
  Serial.println("Waiting for an ISO14443A card to write data");
}

void loop(void) {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  if (!success) {
    Serial.println("No card detected.");
    delay(1000);
    return;
  }

  Serial.println("Card detected. Authenticating...");

  uint8_t blockNumber = 4;
  uint8_t keya[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }; // default key
  uint8_t data[] = { 0x01, 0x02, 0x03, 0x04, 0xAA, 0xBB, 0xCC, 0xDD,
                     0x10, 0x20, 0x30, 0x40, 0x55, 0x66, 0x77, 0x88 }; // must be 16 bytes

  if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, blockNumber, 0, keya)) {
    if (nfc.mifareclassic_WriteDataBlock(blockNumber, data)) {
      Serial.println("Data written successfully to the card!");
    } else {
      Serial.println("Failed to write data to the card.");
    }
  } else {
    Serial.println("Authentication failed.");
  }

  delay(3000);
}
