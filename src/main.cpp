#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>

#include "config.h"

Servo myServo;

int lastButtonState = HIGH;
int currentButtonState;   
bool isFirstNetwork = true;
String Endpoint = "";
MFRC522 rfid(SS_PIN, RST_PIN);     
LiquidCrystal_I2C lcd(0x27, 16, 2); 

String type = "motorbike";

void moveServo()
{
  myServo.write(0);
  delay(200);
  myServo.write(90);
  delay(2000);
  myServo.write(180);
  delay(220);
  myServo.write(90);

}

void displayMessage(String message, bool stay = false)
{

  lcd.clear(); 
  lcd.setCursor(0, 0);
  lcd.print(message); 
  if (stay)
    return;
  delay(1000);
  lcd.clear();
  lcd.print("Ready for");
  lcd.setCursor(0, 1); 
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
    return true; 
  }
  else
  {
    isFirstNetwork = !isFirstNetwork; 
    return false;                    
  }
}

void setup()
{
  Serial.begin(115200); 

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  myServo.setPeriodHertz(50);
  myServo.attach(SERVO_PIN, 500, 2400);
  myServo.write(90);

  SPI.begin();   
  rfid.PCD_Init(); 

  lcd.init();     
  lcd.backlight(); 

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (!connectToWiFi())
  {
    displayMessage("WiFi Failed", true);
  }
  displayMessage("Success");
}

void loop()
{
  digitalWrite(LED_BUILTIN, WiFi.status() == WL_CONNECTED ? HIGH : LOW);
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

  lastButtonState = currentButtonState;

  if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial())
  {
    displayMessage("Processing", true);
    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++)
    {
      uid += String(rfid.uid.uidByte[i], HEX);
    }

    Serial.print("Card UID: ");
    Serial.println(uid);
    moveServo();

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    if (WiFi.status() == WL_CONNECTED)
    {
      HTTPClient http;
      String url = Endpoint + "/api/balance/deduct/" + uid;

      http.begin(url);
      http.addHeader("Content-Type", "application/json"); 

      DynamicJsonDocument jsonBody(128);
      jsonBody["type"] = type;

      String requestBody;
      serializeJson(jsonBody, requestBody);

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
