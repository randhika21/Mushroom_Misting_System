#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <SHT2x.h>
#include <LiquidCrystal_I2C.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID "REPLACE"
#define WIFI_PASSWORD "REPLACE"
#define API_KEY "REPLACE"
#define USER_EMAIL "REPLACE"
#define USER_PASSWORD "REPLACE"
#define DATABASE_URL "REPLACE"

FirebaseData stream;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String uid;
String databasePath;
String tempPath;
String humPath;
String output1Path;
String listenerPath;

SHT2x sht;
float temperature;
float humidity;

unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 1000;

const int output1 = 4;
const int output2 = 5;

LiquidCrystal_I2C lcd(0x27, 20, 4);
int thresholdValue = 0;

void initSHT20() {
  if (!sht.begin()) {
    Serial.println("Could not find a valid SHT20 sensor, check wiring!");
    while (1);
  }
}

void initWiFi() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting to WiFi...");
  Serial.print("Connecting to WiFi ..");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connected!");
  delay(2000);
  lcd.clear();
}

void sendFloat(String path, float value) {
  if (Firebase.RTDB.setFloat(&fbdo, path.c_str(), value)) {
    Serial.print("Writing value: ");
    Serial.print(value);
    Serial.print(" on the following path: ");
    Serial.println(path);
    Serial.println("PASSED");
    Serial.println("PATH: " + fbdo.dataPath());
    Serial.println("TYPE: " + fbdo.dataType());
  } else {
    Serial.println("FAILED");
    Serial.println("REASON: " + fbdo.errorReason());
  }
}

void streamCallback(FirebaseStream data) {
  Serial.printf("stream path, %s\nevent path, %s\ndata type, %s\nevent type, %s\n\n",
                data.streamPath().c_str(),
                data.dataPath().c_str(),
                data.dataType().c_str(),
                data.eventType().c_str());
  printResult(data);
  Serial.println();
  String streamPath = String(data.dataPath());
  
  if (data.dataTypeEnum() == fb_esp_rtdb_data_type_json) {
    FirebaseJson *json = data.to<FirebaseJson *>();
    FirebaseJsonData result;
    
    if (json->get(result, "/digital/" + String(output1), false)) {
      bool state = result.to<bool>();
      digitalWrite(output1, state);
    }

    if (json->get(result, "/digital/" + String(output2), false)) {
      bool state = result.to<bool>();
      digitalWrite(output2, state);
    }

    if (json->get(result, "/threshold", false)) {
      thresholdValue = result.to<int>();
      Serial.print("Threshold: ");
      Serial.println(thresholdValue);
    }
  }

  if (streamPath.indexOf("/digital/") >= 0) {
    int stringLen = streamPath.length();
    int lastSlash = streamPath.lastIndexOf("/");
    String gpio = streamPath.substring(lastSlash + 1, stringLen);
    Serial.print("DIGITAL GPIO: ");
    Serial.println(gpio);
    if (data.dataType() == "int") {
      bool gpioState = data.intData();
      Serial.print("VALUE: ");
      Serial.println(gpioState);
      digitalWrite(gpio.toInt(), gpioState);
    }
    Serial.println();
  } else if (streamPath.indexOf("/threshold") >= 0) {
    int stringLen = streamPath.length();
    if (data.dataType() == "int") {
      thresholdValue = data.intData();
      Serial.print("threshold: ");
      Serial.println(thresholdValue);
    }
  }
  Serial.printf("Received stream payload size: %d (Max. %d)\n\n", data.payloadLength(), data.maxPayloadLength());
}

void streamTimeoutCallback(bool timeout) {
  if (timeout)
    Serial.println("stream timeout, resuming...\n");
  if (!stream.httpConnected())
    Serial.printf("error code: %d, reason: %s\n\n", stream.httpCode(), stream.errorReason().c_str());
}

void setup() {
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  Wire.begin();
  initSHT20();
  initWiFi();
  
  pinMode(output1, OUTPUT);
  pinMode(output2, OUTPUT);

  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);
  config.token_status_callback = tokenStatusCallback;
  config.max_token_generation_retry = 5;
  
  Firebase.begin(&config, &auth);

  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }

  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  databasePath = "/UsersData/" + uid;
  tempPath = databasePath + "/sensor/temperature";
  humPath = databasePath + "/sensor/humidity";
  output1Path = databasePath + "/outputs/digital/4";
  listenerPath = databasePath + "/outputs/";

  if (!Firebase.RTDB.beginStream(&stream, listenerPath.c_str()))
    Serial.printf("stream begin error, %s\n\n", stream.errorReason().c_str());

  Firebase.RTDB.setStreamCallback(&stream, streamCallback, streamTimeoutCallback);

  delay(2000);
}

void loop() {
  lcd.clear();
  sht.read();

  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
    temperature = sht.getTemperature();
    humidity = sht.getHumidity();
    sendFloat(tempPath, temperature);
    sendFloat(humPath, humidity);
  }

  if (digitalRead(output2) == HIGH) {
    lcd.setCursor(0, 0);
    lcd.print("Automatic Mode");
    if (temperature < thresholdValue) {
      digitalWrite(output1, LOW);
      sendFloat(output1Path, 0);
    } else {
      digitalWrite(output1, HIGH);
      sendFloat(output1Path, 1);
    }
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Manual Mode");
  }

  lcd.setCursor(0, 1);
  lcd.print("T: ");
  lcd.print(temperature);
  lcd.print("C  H: ");
  lcd.print(humidity);
  lcd.print("%");
  lcd.setCursor(0, 2);
  lcd.print("Batas Suhu: ");
  lcd.print(thresholdValue);
  lcd.print("C");
  if (digitalRead(output1) == LOW) {
    lcd.setCursor(0, 3);
    lcd.print("Pompa OFF");
  } else {
    lcd.setCursor(0, 3);
    lcd.print("Pompa ON");
  }

  delay(1000);
}
