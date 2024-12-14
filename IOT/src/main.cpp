#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>

LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C address 0x27, 16 column and 2 rows

// KEYPAD CODE
#include <Keypad.h>

#define ROW_NUM     4  // 4 rows
#define COLUMN_NUM  4  // 4 columns
#define SDD2  9
#define SDD3  10
#define BUZZER D8
#define GREEN_LED 1
# define RED_LED 3

ESP8266WiFiMulti WiFiMulti;

char key_layout[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3', 'A'},
  {'4','5','6', 'B'},
  {'7','8','9', 'C'},
  {'*','0','#', 'D'}
};

byte pin_rows[ROW_NUM] = {D0, D3, D4, D5};  // The ESP8266 pins connect to the row pins
byte pin_column[COLUMN_NUM] = {D6, D7, SDD2, SDD3}; // The ESP8266 pins connect to the column pins

Keypad keypad = Keypad(makeKeymap(key_layout), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

const String password = "222"; // change your password here
String input_password;

void setup() {
  Serial.begin(115200);
  // Aggressive serial initialization
  delay(2000);  // Give more time to establish serial connection

  Serial.println("Serial Monitor Initialized");
  Serial.println("Keypad System Ready");
  
  input_password.reserve(32); // maximum input characters is 33, change if needed
  pinMode(SDD2, INPUT);
  pinMode(SDD3, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);

  lcd.init(); // Initialize the LCD I2C display
  lcd.backlight();
  lcd.print("Enter passcode:");
  lcd.setCursor(0, 1);

  // setup WiFi
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("IshaiPhone", "momogurl666");
  // seed for random bldg number using time
  srand(time(0));
}

void playBuzzer(int frequency, int duration) {
  tone(BUZZER, frequency); // Generate sound at the specified frequency
  delay(duration);         // Keep the buzzer on for the specified duration
  noTone(BUZZER);          // Stop the sound
}

void wifiConnect(boolean isIncorrect) {
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    WiFiClient client;
    HTTPClient http;

    Serial.print("[HTTP] begin...\n");
    if (http.begin(client, "http://18.191.122.250:5000/receive_data")) { 
      http.addHeader("Content-Type", "application/json");
      
      int bldg_no = (rand() % 4) + 1;
      String payload = "{\"password\": " + String(isIncorrect ? "false" : "true") + 
                       ", \"bldg_number\": \"" + bldg_no + "\"}";

      Serial.print("[HTTP] POST Payload: ");
      Serial.println(payload);

      int httpCode = http.POST(payload);

      if (httpCode > 0) {
        if (httpCode == HTTP_CODE_OK) {
          String response = http.getString();
          Serial.print("Response: ");
          Serial.println(response);
        } else {
          Serial.printf("[HTTP] POST failed, error: %s\n", http.errorToString(httpCode).c_str());
        }
      } else {
        Serial.printf("[HTTP] Connection failed, error: %s\n", http.errorToString(httpCode).c_str());
      }

      http.end();
    } else {
      Serial.println("[HTTP] Unable to connect");
    }
  } else {
    Serial.println("[WiFi] Not connected!");
  }
}

void loop() {
  char key = keypad.getKey();

  if (key) {
    Serial.println(key);

    if (key == '*') {
      input_password = ""; // clear input password
      lcd.clear();
      lcd.print("Enter passcode:");
      lcd.setCursor(0, 1);
    } else if (key == '#') {
      lcd.print(key);
      if (password == input_password) {
        Serial.println("password is correct");
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Correct Passcode!");

        digitalWrite(GREEN_LED, HIGH);
        // Play a short tone for correct passcode
        playBuzzer(2000, 200); // 2000 Hz for 200 ms
        playBuzzer(2000, 200); // 2000 Hz for 200 ms
        delay(2000);
        digitalWrite(GREEN_LED, LOW);

        wifiConnect(false); // Post `isIncorrect: false`

      } else {
        Serial.println("password is incorrect, try again");
        lcd.clear();
        lcd.print("Wrong Passcode!");

        digitalWrite(RED_LED, HIGH);
        // Play lower frequency tones for incorrect passcode
        for (int i = 0; i < 5; i++) { // Repeat 5 short bursts
          playBuzzer(1000, 400);  // 1000 Hz for 400 ms
          delay(100);             // 100 ms off between bursts
        }
        digitalWrite(RED_LED, LOW);
        delay(2000);

        wifiConnect(true); // Post `isIncorrect: true`
      }

      input_password = ""; // clear input password
      lcd.clear();
      lcd.print("Enter passcode:");
      lcd.setCursor(0, 1);
    } else {
      input_password += key; // append new character to input password string
      lcd.setCursor(input_password.length() - 1, 1); // Move cursor to display each key on the second row
      lcd.print('*');
    }
  }
}