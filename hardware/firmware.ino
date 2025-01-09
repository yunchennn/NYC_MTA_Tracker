#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

const char* ssid = "DG1670AD2";
const char* password = "oolala4ever";
const char* flaskServer = "http://3.19.32.161/";

LiquidCrystal_I2C lcd(0x27, 16, 2);

int btnGPIO = 0;
int btnState = HIGH;  // HIGH when the button is not pressed
int lastBtnState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

void setup() {
    Serial.begin(115200);
    delay(10);

    pinMode(btnGPIO, INPUT);

    Serial.println();
    Serial.print("[WiFi] Connecting to " + String(ssid));
    lcd.begin(16, 2);
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("Initializing...");
    WiFi.begin(ssid, password);


    int tryDelay = 500;
    int numberOfTries = 20;
    int connectDelay = 5000;

    // Wait for the WiFi event
    while (true) {
        switch(WiFi.status()) {
          case WL_NO_SSID_AVAIL:
            lcd.init(); //
            Serial.println("[WiFi] SSID not found");
            break;
          case WL_CONNECT_FAILED:
            lcd.init(); //
            Serial.print("[WiFi] Failed - WiFi not connected! Reason: ");
            return;
            break;
          case WL_CONNECTION_LOST:
            lcd.init(); //
            Serial.println("[WiFi] Connection was lost");
            break;
          case WL_SCAN_COMPLETED:
            lcd.init(); //
            Serial.println("[WiFi] Scan is completed");
            break;
          case WL_DISCONNECTED:
            lcd.init(); //
            Serial.println("[WiFi] WiFi is disconnected");
            break;
          case WL_CONNECTED:
            Serial.println("[WiFi] WiFi is connected!");
            Serial.print("[WiFi] IP address: ");
            Serial.println(WiFi.localIP());
            lcd.init(); // initialize the lcd 
            lcd.backlight();
            lcd.setCursor(0,0);
            lcd.print("WiFi Connected!");
            lcd.setCursor(0,1);
            lcd.print("SSID: ");
            lcd.print(ssid);
            delay(connectDelay);
            return;
            break;
          default:
            lcd.init(); //
            Serial.print("[WiFi] WiFi Status: ");
            Serial.println(WiFi.status());
            break;
        }
        delay(tryDelay);
        
        if(numberOfTries <= 0){
          Serial.print("[WiFi] Failed to connect to WiFi!");
          lcd.init(); // initialize the lcd 
          lcd.backlight();
          lcd.setCursor(0,0);
          lcd.print(" WiFi Failed :(");
          lcd.setCursor(0,1);
          lcd.print("   Try Again");
          // Use disconnect function to force stop trying to connect
          WiFi.disconnect();
          return;
        } else {
          numberOfTries--;
        }
    }
}

void loop() {
    // Read the button state with debouncing
    int reading = digitalRead(btnGPIO);

    if (reading != lastBtnState) {
        lastDebounceTime = millis();
    }

    if ((millis() - lastDebounceTime) > debounceDelay) {
        if (reading != btnState) {
            btnState = reading;

            if (btnState == LOW) {
                // Disconnect from WiFi
                Serial.println("[WiFi] Disconnecting from WiFi!");
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Disconnecting WiFi");
                // This function will disconnect and turn off the WiFi (NVS WiFi data is kept)
                if (WiFi.disconnect(true, false)) {
                    Serial.println("[WiFi] Disconnected from WiFi!");
                }
                delay(1000);
            }
        }
    }

    lastBtnState = reading;

    // Make an HTTP request to the Flask server to get data
    HTTPClient http;

    if (WiFi.status() == WL_CONNECTED) {
        lcd.clear();
        lcd.setCursor(0, 0);

        Serial.println("[HTTP] GET request to Flask server");
        http.begin(flaskServer);
        int httpCode = http.GET();
        Serial.println(httpCode);

        if (httpCode > 0) {
            String payload = http.getString();
            Serial.println("[HTTP] Response received");
            Serial.println(payload);

            // Find the position of the newline character
            int newlinePos = payload.indexOf('\n');

            // If a newline character is found, display the substrings on separate lines
            if (newlinePos != -1) {
                lcd.print(payload.substring(0, newlinePos));
                lcd.setCursor(0, 1);
                lcd.print(payload.substring(newlinePos + 1));
            } else {
                // If no newline character is found, display the entire payload on the first line
                lcd.print(payload);
            }
        } else {
            Serial.printf("[HTTP] GET request failed, error: %s\n", http.errorToString(httpCode).c_str());
            lcd.setCursor(0, 0);
            lcd.print("MTA ETA Tracker");
            lcd.setCursor(0, 1);
            lcd.print("   -No Data-");
        }

        http.end();
    }
    
    delay(5000);  // Adjust the delay as needed
}