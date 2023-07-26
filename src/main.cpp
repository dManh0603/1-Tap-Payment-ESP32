#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include "config.h"

int lastButtonState = HIGH; // the previous state from the input pin
int currentButtonState;     // the current reading from the input pin
bool isFirstNetwork = true;
String Endpoint = "";
MFRC522 rfid(SS_PIN, RST_PIN);      // Create an MFRC522 instance
LiquidCrystal_I2C lcd(0x27, 16, 2); // Initialize the LCD with the I2C address (0x27) and dimensions (16 columns, 2 rows)

String type = "motorbike";

void displayMessage(String message, bool itSelf = false)
{

  lcd.clear();         // Clear the LCD screen
  lcd.setCursor(0, 0); // Set the cursor to the first column, first row
  lcd.print(message);  // Print the message on the LCD
  if (itSelf)
    return;
  delay(1000);
  lcd.clear();
  lcd.print("Ready for");
  lcd.setCursor(0, 1); // Set the cursor to the first column, first row
  lcd.print(type);
}

bool connectToWiFi()
{
  const char *ssid;
  const char *password;

  if (isFirstNetwork)
  {
    ssid = WIFI_SSID;
    password = WIFI_PASSWORD;
    Endpoint = SERVER_URL;
  }
  else
  {
    ssid = WIFI_SSID_2;
    password = WIFI_PASSWORD_2;
    Endpoint = SERVER_URL_2;
  }

  WiFi.begin(ssid, password);
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < 3)
  {
    displayMessage("Connecting", true);
    delay(2000);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    return true; // Successfully connected to WiFi
  }
  else
  {
    isFirstNetwork = !isFirstNetwork; // Switch to the other network
    return false;                     // WiFi connection failed for this network
  }
}

void setup()
{
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(115200); // Initialize serial communication

  SPI.begin();     // Initialize SPI bus
  rfid.PCD_Init(); // Initialize MFRC522

  lcd.init();      // Initialize the LCD
  lcd.backlight(); // Turn on the backlight
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (!connectToWiFi())
  {
    // WiFi connection failed for both networks, retrying in 5 seconds
    displayMessage("WiFi Failed", true);
    delay(5000);
  }
  displayMessage("Success");
}

void loop()
{
  digitalWrite(LED_BUILTIN, WiFi.status() == WL_CONNECTED ? HIGH : LOW);
  digitalWrite(LED_PIN, HIGH);
  // read the state of the switch/button:
  currentButtonState = digitalRead(BUTTON_PIN);

  if (lastButtonState == LOW && currentButtonState == HIGH)
  {
    if (type == "motorbike")
    {
      type = "bicycle";
    }
    else
    {
      type = "motorbike";
    }
    displayMessage("Switched type");
    Serial.println(type);
  }

  // save the last state
  lastButtonState = currentButtonState;

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
      String url = Endpoint + "/api/balance/deduct/" + uid;

      http.begin(url);
      http.addHeader("Content-Type", "application/json"); // Set the content type header for JSON

      // Create a JSON object
      DynamicJsonDocument jsonBody(128);
      jsonBody["type"] = type; // Set the type property in the JSON object

      String requestBody;
      serializeJson(jsonBody, requestBody); // Serialize the JSON object to a string

      int httpResponseCode = http.PUT(requestBody);

      if (httpResponseCode == HTTP_CODE_OK)
      {
        String response = http.getString();
        Serial.println("Server response: " + response);
        displayMessage("Success");
      }
      else if (httpResponseCode == 404)
      {
        String response = http.getString();
        Serial.println("Server response: " + response);
        displayMessage("Unrecognized");
      }
      else if (httpResponseCode == 402)
      {
        String response = http.getString();
        Serial.println("Server response: " + response);
        displayMessage("Declined");
      }
      else if (httpResponseCode == 403)
      {
        String response = http.getString();
        Serial.println("Server response: " + response);
        displayMessage("Disabled card");
      }
      else
      {
        String response = http.getString();
        Serial.print("Error - HTTP response code: ");
        Serial.println(httpResponseCode);
        Serial.println(response);
        displayMessage("System failed");
      }

      http.end();
    }
  }
}
