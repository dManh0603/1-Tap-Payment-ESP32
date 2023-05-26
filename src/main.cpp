#include <SPI.h>
#include <MFRC522.h>
#include <WiFiMulti.h>

#define BUILTIN_LED 2
#define LED 25
#define SS_PIN 5
#define RST_PIN 22
#define WIFI_SSID "121 dai la 2.4Ghz"
#define WIFI_PASSWORD "12345689"

WiFiMulti wifiMulti;
MFRC522 rfid(SS_PIN, RST_PIN); // Create an MFRC522 instance

void setup()
{
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(LED, OUTPUT);
  Serial.begin(921600); // Initialize serial communication
  SPI.begin();          // Initialize SPI bus
  rfid.PCD_Init();      // Initialize MFRC522

  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  while (wifiMulti.run() != WL_CONNECTED)
  {
    delay(100);
  }

  Serial.println("Wifi connected");
  Serial.println("Scan an RFID card");
}

void loop()
{
  digitalWrite(LED_BUILTIN, wifiMulti.run() == WL_CONNECTED ? HIGH : LOW);
  digitalWrite(LED, HIGH);
  // Check if a card is present
  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
  {
    // Read the card's UID
    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++)
    {
      uid += String(rfid.uid.uidByte[i], HEX);
    }

    Serial.print("Card UID: ");
    Serial.println(uid);

    // Halt PICC
    rfid.PICC_HaltA();
    // Stop encryption on PCD
    rfid.PCD_StopCrypto1();
  }
}
