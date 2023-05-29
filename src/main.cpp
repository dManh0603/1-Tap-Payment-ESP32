#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h> // Include the ArduinoJSON library

#define LED 25
#define SS_PIN 5
#define RST_PIN 22
#define WIFI_SSID "121 dai la 2.4Ghz"
#define WIFI_PASSWORD "12345689"

MFRC522 rfid(SS_PIN, RST_PIN); // Create an MFRC522 instance

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED, OUTPUT);
  Serial.begin(115200); // Initialize serial communication
  SPI.begin();          // Initialize SPI bus
  rfid.PCD_Init();      // Initialize MFRC522

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
  }

  Serial.println("WiFi connected");
  Serial.println("Scan an RFID card");
}

void loop()
{
  digitalWrite(LED_BUILTIN, WiFi.status() == WL_CONNECTED ? HIGH : LOW);
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

    // Make a PUT request with JSON body
    if (WiFi.status() == WL_CONNECTED)
    {
      HTTPClient http;
      String url = "http://192.168.1.10:5050/api/balance/deduct/" + uid;
      
      http.begin(url);
      http.addHeader("Content-Type", "application/json"); // Set the content type header for JSON

      // Create a JSON object
      DynamicJsonDocument jsonBody(128);
      jsonBody["amount"] = 3; // Set the amount property in the JSON object

      String requestBody;
      serializeJson(jsonBody, requestBody); // Serialize the JSON object to a string

      int httpResponseCode = http.PUT(requestBody);

      if (httpResponseCode == HTTP_CODE_OK)
      {
        String response = http.getString();
        Serial.println("Server response: " + response);
      }
      else
      {
        Serial.print("Error - HTTP response code: ");
        Serial.println(httpResponseCode);
      }

      http.end();
    }
  }
}
