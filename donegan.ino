#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <Wire.h>
#include <SHT2x.h>
#include <LiquidCrystal_I2C.h>  // Include the LCD library

// Provide the token generation process info.
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "tester"
#define WIFI_PASSWORD "katasandi"

// Insert Firebase project API Key
#define API_KEY "AIzaSyB6OcY25hnUQNw4g9E4CVC_i4eQJiLNI7U"

// Insert Authorized Username and Corresponding Password
#define USER_EMAIL "admin@mail.id"
#define USER_PASSWORD "admin0"

// Insert RTDB URL
#define DATABASE_URL "https://database-walet-default-rtdb.asia-southeast1.firebasedatabase.app/"

// Define Firebase objects
FirebaseData stream;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Variable to save USER UID
String uid;

// Variables to save database paths
String databasePath;
String tempPath;
String humPath;
String output1Path;
String listenerPath;

// SHT20 sensor
SHT2x sht;  // Initialize SHT20
float temperature;
float humidity;

// Timer variables (send new readings every other minute)
unsigned long sendDataPrevMillis = 0;
unsigned long timerDelay = 1000;

// Declare outputs
const int output1 = 4;
const int output2 = 5;

// Initialize LCD
LiquidCrystal_I2C lcd(0x27, 20, 4);  // Set the I2C address (0x27) and dimensions (20x4)

// Variable to store the threshold value
int thresholdValue = 0;

// Initialize SHT20
void initSHT20() {
  if (!sht.begin()) {
    Serial.println("Could not find a valid SHT20 sensor, check wiring!");
    while (1)
      ;
  }
}

// Initialize WiFi
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
  delay(2000);  // Show "Connected!" for 2 seconds
  lcd.clear();
}

// Write float values to the database
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

// Callback function that runs on database changes
void streamCallback(FirebaseStream data) {
  Serial.printf("stream path, %s\nevent path, %s\ndata type, %s\nevent type, %s\n\n",
                data.streamPath().c_str(),
                data.dataPath().c_str(),
                data.dataType().c_str(),
                data.eventType().c_str());
  printResult(data);
  Serial.println();

  // Get the path that triggered the function
  String streamPath = String(data.dataPath());

  // When it first runs, it is triggered on the root (/) path
  if (data.dataTypeEnum() == fb_esp_rtdb_data_type_json) {
    FirebaseJson *json = data.to<FirebaseJson *>();
    FirebaseJsonData result;

    // Get the digital output1 value
    if (json->get(result, "/digital/" + String(output1), false)) {
      bool state = result.to<bool>();
      digitalWrite(output1, state);
    }

    // Get the digital output2 value
    if (json->get(result, "/digital/" + String(output2), false)) {
      bool state = result.to<bool>();
      digitalWrite(output2, state);
    }

    // Get the threshold value
    if (json->get(result, "/threshold", false)) {
      thresholdValue = result.to<int>();  // Store the threshold value
      Serial.print("Threshold: ");
      Serial.println(thresholdValue);
    }
  }

  // Check for changes in the digital output values
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
  }

  // Check for changes in the message
  else if (streamPath.indexOf("/threshold") >= 0) {
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

  // Initialize LCD
  lcd.init();
  lcd.backlight();  // Turn on the backlight

  // Init SHT20 sensor and WiFi
  Wire.begin();
  initSHT20();
  initWiFi();

  // Initialize Outputs
  pinMode(output1, OUTPUT);
  pinMode(output2, OUTPUT);

  // Assign the API key (required)
  config.api_key = API_KEY;

  // Assign the user sign-in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Assign the RTDB URL (required)
  config.database_url = DATABASE_URL;

  Firebase.reconnectWiFi(true);
  fbdo.setResponseSize(4096);

  // Assign the callback function for the long-running token generation task
  config.token_status_callback = tokenStatusCallback;

  // Assign the maximum retry of token generation
  config.max_token_generation_retry = 5;

  // Initialize the library with the Firebase authentication and config
  Firebase.begin(&config, &auth);

  // Getting the user UID might take a few seconds
  Serial.println("Getting User UID");
  while ((auth.token.uid) == "") {
    Serial.print('.');
    delay(1000);
  }

  // Print user UID
  uid = auth.token.uid.c_str();
  Serial.print("User UID: ");
  Serial.println(uid);

  // Update database path with user UID
  databasePath = "/UsersData/" + uid;

  // Define database path for sensor readings
  tempPath = databasePath + "/sensor/temperature";  // --> UsersData/<user_uid>/sensor/temperature
  humPath = databasePath + "/sensor/humidity";      // --> UsersData/<user_uid>/sensor/humidity
  output1Path = databasePath + "/outputs/digital/4";      // --> UsersData/<user_uid>/sensor/humidity

  // Update database path for listening
  listenerPath = databasePath + "/outputs/";

  // Streaming (whenever data changes on a path)
  if (!Firebase.RTDB.beginStream(&stream, listenerPath.c_str()))
    Serial.printf("stream begin error, %s\n\n", stream.errorReason().c_str());

  // Assign a callback function to run when it detects changes on the database
  Firebase.RTDB.setStreamCallback(&stream, streamCallback, streamTimeoutCallback);

  delay(2000);
}

void loop() {
  
  lcd.clear();
  sht.read();

  // Send new readings to database
  if (Firebase.ready() && (millis() - sendDataPrevMillis > timerDelay || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    // Get latest sensor readings
    temperature = sht.getTemperature();
    humidity = sht.getHumidity();

    // Send readings to database
    sendFloat(tempPath, temperature);
    sendFloat(humPath, humidity);
  }

  if (digitalRead(output2) == HIGH) {  // Check if output2 is HIGH for automatic mode
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


  // Update the LCD display
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

  delay(1000);  // Update every second
}
