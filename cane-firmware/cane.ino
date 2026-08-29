#include <WiFi.h>
#include <WiFiManager.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <time.h>

// ============================================================
// SMART CANE - SELF CONFIGURATION
//
// First boot / unconfigured:
//   ESP32 creates "Cane-Setup"
//   User connects phone to it
//   Open http://192.168.4.1
//   Configure:
//      - Wi-Fi SSID
//      - Wi-Fi Password
//      - Telegram Bot Token
//      - Telegram Chat ID
//      - Patient ID
//
// Configuration is saved permanently in ESP32 flash.
//
// To force reconfiguration:
//   Hold emergency button for 5 seconds while powering ON.
//
// Firebase project ID remains fixed because it is your shared
// Firebase backend.
// ============================================================


// ============================================================
// FIREBASE
// ============================================================

const char* FIRESTORE_PROJECT_ID = "cane-caregiver-app-bd501";


// ============================================================
// HARDWARE PINS
// ============================================================

const int trigPin = 5;
const int echoPin = 18;
const int buzzerPin = 23;

const int emergencyButtonPin = 27;


// ============================================================
// BUTTON TIMINGS
// ============================================================

const unsigned long EMERGENCY_HOLD_MS = 800;

// Hold emergency button for 5 seconds during power-on
// to enter configuration mode.
const unsigned long RESET_HOLD_MS = 5000;


// ============================================================
// GPS
// ============================================================

HardwareSerial gpsSerial(2);
TinyGPSPlus gps;

#define GPS_RX 16
#define GPS_TX 17


// ============================================================
// PREFERENCES
// ============================================================

Preferences preferences;


// ============================================================
// STORED CONFIGURATION
// ============================================================

char botTokenBuf[100] = "";
char chatIdBuf[40] = "";
char patientIdBuf[80] = "";

String BOT_TOKEN;
String CHAT_ID;
String PATIENT_ID;


// ============================================================
// SENSOR STATE
// ============================================================

long duration;
float distance;


// ============================================================
// TELEGRAM TIMING
// ============================================================

unsigned long lastTelegramTime = 0;


// ============================================================
// BUZZER STATE
// ============================================================

unsigned long lastBuzzTime = 0;
bool buzzerState = false;


// ============================================================
// PHONE LOCATION
// ============================================================

double phoneLat = 0.0;
double phoneLng = 0.0;

volatile bool phoneLocationValid = false;
volatile unsigned long phoneLocationMillis = 0;

long lastUpdateId = 0;

unsigned long lastPhoneLocationCheck = 0;

const unsigned long PHONE_LOCATION_CHECK_INTERVAL = 15000;
const unsigned long PHONE_LOCATION_MAX_AGE = 120000;


// ============================================================
// EMERGENCY BUTTON STATE
// ============================================================

unsigned long emergencyPressStart = 0;

bool emergencyButtonWasDown = false;
bool emergencyAlreadyFiredThisPress = false;


// ============================================================
// CHECK WHETHER REQUIRED CONFIGURATION EXISTS
// ============================================================

bool hasSavedConfiguration() {

  String savedBotToken =
      preferences.getString("botToken", "");

  String savedChatId =
      preferences.getString("chatId", "");

  String savedPatientId =
      preferences.getString("patientId", "");

  // All three must exist
  if (savedBotToken.length() == 0) return false;
  if (savedChatId.length() == 0) return false;
  if (savedPatientId.length() == 0) return false;

  return true;
}


// ============================================================
// LOAD CONFIGURATION FROM FLASH
// ============================================================

void loadSavedConfig() {

  String savedBotToken =
      preferences.getString("botToken", "");

  String savedChatId =
      preferences.getString("chatId", "");

  String savedPatientId =
      preferences.getString("patientId", "");


  savedBotToken.toCharArray(
      botTokenBuf,
      sizeof(botTokenBuf)
  );

  savedChatId.toCharArray(
      chatIdBuf,
      sizeof(chatIdBuf)
  );

  savedPatientId.toCharArray(
      patientIdBuf,
      sizeof(patientIdBuf)
  );


  Serial.println();
  Serial.println("========== SAVED CONFIG ==========");

  if (savedBotToken.length() > 0)
    Serial.println("Telegram Bot Token: SAVED");
  else
    Serial.println("Telegram Bot Token: NOT SET");

  if (savedChatId.length() > 0)
    Serial.println("Telegram Chat ID: SAVED");
  else
    Serial.println("Telegram Chat ID: NOT SET");

  if (savedPatientId.length() > 0)
    Serial.println("Patient ID: SAVED");
  else
    Serial.println("Patient ID: NOT SET");

  Serial.println("==================================");
}


// ============================================================
// WIFI MANAGER SETUP PORTAL
// ============================================================

void runSetupPortal(bool forceReset) {

  WiFiManager wm;


  // ----------------------------------------------------------
  // If force reset requested
  // ----------------------------------------------------------

  if (forceReset) {

    Serial.println();
    Serial.println("==================================");
    Serial.println("FORCING RECONFIGURATION");
    Serial.println("==================================");

    // Erase saved Wi-Fi credentials
    wm.resetSettings();

    // Also erase our custom configuration
    preferences.remove("botToken");
    preferences.remove("chatId");
    preferences.remove("patientId");

    // Clear RAM buffers
    botTokenBuf[0] = '\0';
    chatIdBuf[0] = '\0';
    patientIdBuf[0] = '\0';
  }


  // ----------------------------------------------------------
  // Custom fields
  // ----------------------------------------------------------

  WiFiManagerParameter customBotToken(
      "bottoken",
      "Telegram Bot Token",
      botTokenBuf,
      sizeof(botTokenBuf) - 1
  );

  WiFiManagerParameter customChatId(
      "chatid",
      "Telegram Chat ID",
      chatIdBuf,
      sizeof(chatIdBuf) - 1
  );

  WiFiManagerParameter customPatientId(
      "patientid",
      "Patient ID",
      patientIdBuf,
      sizeof(patientIdBuf) - 1
  );


  wm.addParameter(&customBotToken);
  wm.addParameter(&customChatId);
  wm.addParameter(&customPatientId);


  // ----------------------------------------------------------
  // Portal timeout
  // ----------------------------------------------------------

  wm.setConfigPortalTimeout(180);


  // ----------------------------------------------------------
  // Decide whether configuration portal is required
  // ----------------------------------------------------------

  bool configurationMissing = !hasSavedConfiguration();


  bool connected = false;


  // ----------------------------------------------------------
  // FIRST BOOT / MISSING CONFIGURATION
  // ----------------------------------------------------------

  if (forceReset || configurationMissing) {

    Serial.println();
    Serial.println("==================================");
    Serial.println("STARTING SMART CANE SETUP");
    Serial.println("==================================");

    Serial.println("Connect your phone to:");
    Serial.println("Wi-Fi: Cane-Setup");
    Serial.println("Open: http://192.168.4.1");


    // startConfigPortal forces the setup hotspot
    connected = wm.startConfigPortal("Cane-Setup");
  }


  // ----------------------------------------------------------
  // NORMAL BOOT
  // ----------------------------------------------------------

  else {

    Serial.println();
    Serial.println("Saved configuration found.");
    Serial.println("Connecting to saved Wi-Fi...");

    connected = wm.autoConnect("Cane-Setup");
  }


  // ----------------------------------------------------------
  // SETUP FAILED / TIMEOUT
  // ----------------------------------------------------------

  if (!connected) {

    Serial.println();
    Serial.println("Setup/connection failed.");
    Serial.println("Restarting...");

    delay(2000);

    ESP.restart();
  }


  // ----------------------------------------------------------
  // SAVE CUSTOM PARAMETERS
  // ----------------------------------------------------------

  String newBotToken =
      String(customBotToken.getValue());

  String newChatId =
      String(customChatId.getValue());

  String newPatientId =
      String(customPatientId.getValue());


  newBotToken.trim();
  newChatId.trim();
  newPatientId.trim();


  // Only overwrite stored values when values were supplied.
  if (newBotToken.length() > 0) {

    newBotToken.toCharArray(
        botTokenBuf,
        sizeof(botTokenBuf)
    );

    preferences.putString(
        "botToken",
        botTokenBuf
    );
  }


  if (newChatId.length() > 0) {

    newChatId.toCharArray(
        chatIdBuf,
        sizeof(chatIdBuf)
    );

    preferences.putString(
        "chatId",
        chatIdBuf
    );
  }


  if (newPatientId.length() > 0) {

    newPatientId.toCharArray(
        patientIdBuf,
        sizeof(patientIdBuf)
    );

    preferences.putString(
        "patientId",
        patientIdBuf
    );
  }


  // ----------------------------------------------------------
  // DISPLAY RESULT
  // ----------------------------------------------------------

  Serial.println();
  Serial.println("==================================");
  Serial.println("CONFIGURATION COMPLETE");
  Serial.println("==================================");

  Serial.print("Wi-Fi connected: ");
  Serial.println(WiFi.SSID());

  Serial.print("ESP32 IP: ");
  Serial.println(WiFi.localIP());

  Serial.println("Telegram Bot: SAVED");
  Serial.println("Telegram Chat ID: SAVED");
  Serial.println("Patient ID: SAVED");

  Serial.println("==================================");
}


// ============================================================
// GPS - GET BEST LOCATION
// ============================================================

bool getBestLocation(
    double &lat,
    double &lng,
    String &source
) {

  // ----------------------------------------------------------
  // 1. GPS MODULE
  // ----------------------------------------------------------

  if (
      gps.location.isValid() &&
      gps.location.age() < 5000
  ) {

    lat = gps.location.lat();
    lng = gps.location.lng();

    source = "GPS module";

    return true;
  }


  // ----------------------------------------------------------
  // 2. PHONE LOCATION FROM TELEGRAM
  // ----------------------------------------------------------

  if (
      phoneLocationValid &&
      (millis() - phoneLocationMillis) <
          PHONE_LOCATION_MAX_AGE
  ) {

    lat = phoneLat;
    lng = phoneLng;

    source = "Phone (Telegram live location)";

    return true;
  }


  return false;
}


// ============================================================
// LOCATION LINK
// ============================================================

String getLocationLink() {

  double lat;
  double lng;

  String source;


  if (getBestLocation(lat, lng, source)) {

    return
      "https://maps.google.com/?q=" +
      String(lat, 6) +
      "," +
      String(lng, 6) +
      " (" +
      source +
      ")";
  }


  return
    "Location not available "
    "(no GPS fix, no recent phone location)";
}


// ============================================================
// TIME
// ============================================================

String getIsoTimestamp() {

  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {

    return "1970-01-01T00:00:00Z";
  }


  char buf[25];

  strftime(
      buf,
      sizeof(buf),
      "%Y-%m-%dT%H:%M:%SZ",
      &timeinfo
  );


  return String(buf);
}


// ============================================================
// TELEGRAM - SEND MESSAGE
// ============================================================

void sendTelegramRaw(String message) {

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println(
        "WiFi not connected, "
        "skipping Telegram send."
    );

    return;
  }


  if (BOT_TOKEN.length() == 0 ||
      CHAT_ID.length() == 0) {

    Serial.println(
        "Telegram configuration missing."
    );

    return;
  }


  WiFiClientSecure secured_client;

  secured_client.setInsecure();


  HTTPClient http;

  http.setTimeout(3000);


  String url =
      "https://api.telegram.org/bot" +
      BOT_TOKEN +
      "/sendMessage";


  if (!http.begin(
          secured_client,
          url
      )) {

    Serial.println(
        "http.begin failed"
    );

    return;
  }


  http.addHeader(
      "Content-Type",
      "application/json"
  );


  DynamicJsonDocument doc(2048);

  doc["chat_id"] = CHAT_ID;
  doc["text"] = message;


  String payload;

  serializeJson(
      doc,
      payload
  );


  int httpCode =
      http.POST(payload);


  Serial.print(
      "Telegram HTTP Code: "
  );

  Serial.println(httpCode);


  http.end();
}


// ============================================================
// TELEGRAM - SEND LOCATION
// ============================================================

void sendTelegramLocationRaw(
    double lat,
    double lng
) {

  if (WiFi.status() != WL_CONNECTED)
    return;


  if (BOT_TOKEN.length() == 0 ||
      CHAT_ID.length() == 0)
    return;


  WiFiClientSecure secured_client;

  secured_client.setInsecure();


  HTTPClient http;

  http.setTimeout(3000);


  String url =
      "https://api.telegram.org/bot" +
      BOT_TOKEN +
      "/sendLocation";


  if (!http.begin(
          secured_client,
          url
      ))
    return;


  http.addHeader(
      "Content-Type",
      "application/json"
  );


  DynamicJsonDocument doc(1024);

  doc["chat_id"] = CHAT_ID;
  doc["latitude"] = lat;
  doc["longitude"] = lng;


  String payload;

  serializeJson(
      doc,
      payload
  );


  http.POST(payload);

  http.end();
}


// ============================================================
// FIRESTORE - EMERGENCY ONLY
// ============================================================

void sendAlertToFirestoreRaw(
    String message,
    bool hasLocation,
    double lat,
    double lng
) {

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println(
        "WiFi not connected, "
        "skipping Firestore send."
    );

    return;
  }


  WiFiClientSecure secured_client;

  secured_client.setInsecure();


  HTTPClient http;

  http.setTimeout(5000);


  String url =
      "https://firestore.googleapis.com/v1/projects/" +
      String(FIRESTORE_PROJECT_ID) +
      "/databases/(default)/documents/alerts";


  if (!http.begin(
          secured_client,
          url
      )) {

    Serial.println(
        "Firestore http.begin failed"
    );

    return;
  }


  http.addHeader(
      "Content-Type",
      "application/json"
  );


  DynamicJsonDocument doc(2048);


  JsonObject fields =
      doc.createNestedObject("fields");


  fields["patientId"]["stringValue"] =
      PATIENT_ID;


  fields["message"]["stringValue"] =
      message;


  fields["timestamp"]["stringValue"] =
      getIsoTimestamp();


  if (hasLocation) {

    JsonObject locationField =
        fields.createNestedObject("location");


    JsonObject geo =
        locationField.createNestedObject(
            "geoPointValue"
        );


    geo["latitude"] = lat;
    geo["longitude"] = lng;
  }


  String payload;

  serializeJson(
      doc,
      payload
  );


  int httpCode =
      http.POST(payload);


  Serial.print(
      "Firestore alert HTTP code: "
  );

  Serial.println(httpCode);


  if (httpCode != 200) {

    Serial.println(
        http.getString()
    );
  }


  http.end();
}


// ============================================================
// TELEGRAM TASK
// ============================================================

struct TelegramJob {

  String message;

  bool sendLocation;

  double lat;
  double lng;
};


// ============================================================
// TELEGRAM TASK
// ============================================================

void telegramTask(void *param) {

  TelegramJob *job =
      (TelegramJob *)param;


  sendTelegramRaw(
      job->message
  );


  if (job->sendLocation) {

    sendTelegramLocationRaw(
        job->lat,
        job->lng
    );
  }


  delete job;

  vTaskDelete(NULL);
}


// ============================================================
// QUEUE TELEGRAM ALERT
// ============================================================

void queueTelegramAlert(
    String message
) {

  TelegramJob *job =
      new TelegramJob();


  job->message = message;


  double lat;
  double lng;

  String source;


  if (
      getBestLocation(
          lat,
          lng,
          source
      )
  ) {

    job->sendLocation = true;

    job->lat = lat;
    job->lng = lng;

  }

  else {

    job->sendLocation = false;

    job->lat = 0;
    job->lng = 0;
  }


  xTaskCreatePinnedToCore(
      telegramTask,
      "telegramTask",
      8192,
      job,
      1,
      NULL,
      0
  );
}


// ============================================================
// EMERGENCY TASK
// ============================================================

struct EmergencyJob {

  bool hasLocation;

  double lat;
  double lng;
};


// ============================================================
// EMERGENCY TASK
// ============================================================

void emergencyTask(
    void *param
) {

  EmergencyJob *job =
      (EmergencyJob *)param;


  sendAlertToFirestoreRaw(
      "EMERGENCY - patient pressed the SOS button on the cane.",
      job->hasLocation,
      job->lat,
      job->lng
  );


  sendTelegramRaw(
      "EMERGENCY BUTTON PRESSED\n" +
      getLocationLink()
  );


  delete job;

  vTaskDelete(NULL);
}


// ============================================================
// TRIGGER EMERGENCY
// ============================================================

void triggerEmergencyAlert() {

  Serial.println(
      "!!! EMERGENCY BUTTON HELD - notifying caregiver !!!"
  );


  EmergencyJob *job =
      new EmergencyJob();


  double lat;
  double lng;

  String source;


  job->hasLocation =
      getBestLocation(
          lat,
          lng,
          source
      );


  job->lat = lat;
  job->lng = lng;


  xTaskCreatePinnedToCore(
      emergencyTask,
      "emergencyTask",
      8192,
      job,
      1,
      NULL,
      0
  );


  // Strong emergency buzzer pattern

  for (
      int i = 0;
      i < 3;
      i++
  ) {

    ledcWriteTone(
        0,
        2000
    );

    delay(150);

    ledcWriteTone(
        0,
        0
    );

    delay(150);
  }
}


// ============================================================
// CHECK EMERGENCY BUTTON
// ============================================================

void checkEmergencyButton() {

  bool isDown =
      (
        digitalRead(
            emergencyButtonPin
        ) == LOW
      );


  if (
      isDown &&
      !emergencyButtonWasDown
  ) {

    emergencyPressStart =
        millis();

    emergencyAlreadyFiredThisPress =
        false;
  }


  if (
      isDown &&
      !emergencyAlreadyFiredThisPress
  ) {

    if (
        millis() -
        emergencyPressStart >=
        EMERGENCY_HOLD_MS
    ) {

      triggerEmergencyAlert();

      emergencyAlreadyFiredThisPress =
          true;
    }
  }


  emergencyButtonWasDown =
      isDown;
}


// ============================================================
// TELEGRAM - CHECK PHONE LOCATION
// ============================================================

void checkPhoneLocationRaw() {

  if (WiFi.status() != WL_CONNECTED)
    return;


  if (BOT_TOKEN.length() == 0)
    return;


  WiFiClientSecure secured_client;

  secured_client.setInsecure();


  HTTPClient http;

  http.setTimeout(4000);


  String url =
      "https://api.telegram.org/bot" +
      BOT_TOKEN +
      "/getUpdates?offset=" +
      String(lastUpdateId + 1) +
      "&limit=10&timeout=0";


  if (!http.begin(
          secured_client,
          url
      ))
    return;


  int httpCode =
      http.GET();


  if (httpCode == 200) {

    String payload =
        http.getString();


    http.end();


    DynamicJsonDocument doc(6144);


    DeserializationError err =
        deserializeJson(
            doc,
            payload
        );


    if (err) {

      Serial.print(
          "getUpdates JSON parse failed: "
      );

      Serial.println(
          err.c_str()
      );

      return;
    }


    JsonArray results =
        doc["result"].as<JsonArray>();


    for (
        JsonObject update :
        results
    ) {

      long updateId =
          update["update_id"] | 0L;


      if (
          updateId >
          lastUpdateId
      ) {

        lastUpdateId =
            updateId;
      }


      JsonObject msg =
          update["message"];


      if (msg.isNull())
        msg =
            update["edited_message"];


      if (msg.isNull())
        continue;


      JsonObject loc =
          msg["location"];


      if (loc.isNull())
        continue;


      double lat =
          loc["latitude"] | 0.0;


      double lng =
          loc["longitude"] | 0.0;


      if (
          lat != 0.0 ||
          lng != 0.0
      ) {

        phoneLat = lat;
        phoneLng = lng;

        phoneLocationMillis =
            millis();

        phoneLocationValid =
            true;


        Serial.println(
            "Phone location updated from Telegram."
        );
      }
    }
  }

  else {

    Serial.print(
        "getUpdates HTTP code: "
    );

    Serial.println(
        httpCode
    );

    http.end();
  }
}


// ============================================================
// PHONE LOCATION TASK
// ============================================================

void phoneLocationTask(
    void *param
) {

  checkPhoneLocationRaw();

  vTaskDelete(NULL);
}


// ============================================================
// QUEUE PHONE LOCATION CHECK
// ============================================================

void queuePhoneLocationCheck() {

  xTaskCreatePinnedToCore(
      phoneLocationTask,
      "phoneLocTask",
      8192,
      NULL,
      1,
      NULL,
      0
  );
}


// ============================================================
// BUZZER
// ============================================================

void buzzPattern(
    int onTime,
    int offTime
) {

  unsigned long currentTime =
      millis();


  if (
      buzzerState == false &&
      currentTime -
      lastBuzzTime >=
      offTime
  ) {

    ledcWriteTone(
        0,
        1000
    );

    buzzerState = true;

    lastBuzzTime =
        currentTime;
  }


  else if (
      buzzerState == true &&
      currentTime -
      lastBuzzTime >=
      onTime
  ) {

    ledcWriteTone(
        0,
        0
    );

    buzzerState = false;

    lastBuzzTime =
        currentTime;
  }
}


// ============================================================
// BUZZER OFF
// ============================================================

void buzzerOff() {

  ledcWriteTone(
      0,
      0
  );

  buzzerState = false;

  lastBuzzTime = 0;
}


// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  delay(1000);


  // ----------------------------------------------------------
  // HARDWARE
  // ----------------------------------------------------------

  pinMode(
      trigPin,
      OUTPUT
  );

  pinMode(
      echoPin,
      INPUT
  );

  pinMode(
      emergencyButtonPin,
      INPUT_PULLUP
  );


  // ----------------------------------------------------------
  // BUZZER
  // ----------------------------------------------------------

  ledcSetup(
      0,
      1000,
      8
  );

  ledcAttachPin(
      buzzerPin,
      0
  );


  // ----------------------------------------------------------
  // GPS
  // ----------------------------------------------------------

  gpsSerial.begin(
      9600,
      SERIAL_8N1,
      GPS_RX,
      GPS_TX
  );


  // ----------------------------------------------------------
  // PREFERENCES
  // ----------------------------------------------------------

  preferences.begin(
      "cane-cfg",
      false
  );


  loadSavedConfig();


  // ----------------------------------------------------------
  // CHECK FOR FORCED RECONFIGURATION
  // ----------------------------------------------------------

  bool forceReset = false;


  if (
      digitalRead(
          emergencyButtonPin
      ) == LOW
  ) {

    Serial.println();
    Serial.println(
        "Button held at boot - checking for reset..."
    );


    unsigned long holdStart =
        millis();


    while (
        digitalRead(
            emergencyButtonPin
        ) == LOW &&
        millis() -
        holdStart <
        RESET_HOLD_MS
    ) {

      delay(50);
    }


    if (
        millis() -
        holdStart >=
        RESET_HOLD_MS
    ) {

      forceReset = true;

      Serial.println(
          "5-second hold detected."
      );

      Serial.println(
          "Entering configuration mode."
      );
    }
  }


  // ----------------------------------------------------------
  // WIFI + CUSTOM CONFIGURATION
  // ----------------------------------------------------------

  runSetupPortal(
      forceReset
  );


  // ----------------------------------------------------------
  // LOAD FINAL CONFIG INTO RAM
  // ----------------------------------------------------------

  loadSavedConfig();


  BOT_TOKEN =
      String(botTokenBuf);

  CHAT_ID =
      String(chatIdBuf);

  PATIENT_ID =
      String(patientIdBuf);


  // ----------------------------------------------------------
  // PRINT STATUS
  // ----------------------------------------------------------

  Serial.println();
  Serial.println(
      "=================================="
  );

  Serial.println(
      "SMART CANE READY"
  );

  Serial.println(
      "=================================="
  );

  Serial.print(
      "Wi-Fi: "
  );

  Serial.println(
      WiFi.SSID()
  );


  Serial.print(
      "IP Address: "
  );

  Serial.println(
      WiFi.localIP()
  );


  Serial.print(
      "Patient ID configured: "
  );

  Serial.println(
      PATIENT_ID.length() > 0
        ? "YES"
        : "NO"
  );


  Serial.print(
      "Telegram configured: "
  );

  Serial.println(
      (
        BOT_TOKEN.length() > 0 &&
        CHAT_ID.length() > 0
      )
        ? "YES"
        : "NO"
  );


  Serial.println(
      "=================================="
  );


  // ----------------------------------------------------------
  // TIME
  // ----------------------------------------------------------

  configTime(
      0,
      0,
      "pool.ntp.org"
  );


  // ----------------------------------------------------------
  // ONLINE MESSAGE
  // ----------------------------------------------------------

  queueTelegramAlert(
      "Smart Cane is online and monitoring."
  );
}


// ============================================================
// LOOP
// ============================================================

void loop() {

  // ----------------------------------------------------------
  // GPS
  // ----------------------------------------------------------

  while (
      gpsSerial.available() > 0
  ) {

    gps.encode(
        gpsSerial.read()
    );
  }


  // ----------------------------------------------------------
  // WIFI RECONNECTION
  // ----------------------------------------------------------

  static unsigned long lastWifiCheck = 0;


  if (
      WiFi.status() != WL_CONNECTED &&
      millis() -
      lastWifiCheck >
      10000
  ) {

    Serial.println(
        "WiFi dropped, attempting reconnect..."
    );


    WiFi.reconnect();


    lastWifiCheck =
        millis();
  }


  // ----------------------------------------------------------
  // PHONE LOCATION
  // ----------------------------------------------------------

  if (
      millis() -
      lastPhoneLocationCheck >
      PHONE_LOCATION_CHECK_INTERVAL
  ) {

    if (
        !(
          gps.location.isValid() &&
          gps.location.age() < 5000
        )
    ) {

      queuePhoneLocationCheck();
    }


    lastPhoneLocationCheck =
        millis();
  }


  // ----------------------------------------------------------
  // EMERGENCY BUTTON
  // ----------------------------------------------------------

  checkEmergencyButton();


  // ----------------------------------------------------------
  // ULTRASONIC SENSOR
  // ----------------------------------------------------------

  digitalWrite(
      trigPin,
      LOW
  );

  delayMicroseconds(2);


  digitalWrite(
      trigPin,
      HIGH
  );

  delayMicroseconds(10);


  digitalWrite(
      trigPin,
      LOW
  );


  duration =
      pulseIn(
          echoPin,
          HIGH,
          30000
      );


  // ----------------------------------------------------------
  // SENSOR TIMEOUT
  // ----------------------------------------------------------

  if (duration == 0) {

    Serial.println(
        "Sensor timeout / no echo"
    );

    buzzerOff();

    delay(50);

    return;
  }


  // ----------------------------------------------------------
  // CALCULATE DISTANCE
  // ----------------------------------------------------------

  distance =
      duration *
      0.0343 /
      2.0;


  Serial.print(
      "Distance: "
  );

  Serial.print(
      distance
  );

  Serial.print(
      " cm | GPS Sats: "
  );

  Serial.println(
      gps.satellites.value()
  );


  unsigned long currentTime =
      millis();


  // ==========================================================
  // VERY CLOSE
  // ==========================================================

  if (
      distance > 0 &&
      distance <= 10
  ) {

    Serial.println(
        "VERY CLOSE! Fast buzz!"
    );


    buzzPattern(
        150,
        300
    );


    if (
        currentTime -
        lastTelegramTime >
        5000
    ) {

      String msg =
          "DANGER! Obstacle very close! Distance: " +
          String(distance) +
          " cm\n" +
          getLocationLink();


      queueTelegramAlert(
          msg
      );


      lastTelegramTime =
          currentTime;
    }
  }


  // ==========================================================
  // MEDIUM
  // ==========================================================

  else if (
      distance > 10 &&
      distance <= 40
  ) {

    Serial.println(
        "MEDIUM! Medium buzz!"
    );


    buzzPattern(
        150,
        300
    );


    if (
        currentTime -
        lastTelegramTime >
        5000
    ) {

      String msg =
          "Warning! Obstacle nearby! Distance: " +
          String(distance) +
          " cm\n" +
          getLocationLink();


      queueTelegramAlert(
          msg
      );


      lastTelegramTime =
          currentTime;
    }
  }



  // ==========================================================
  // PATH CLEAR
  // ==========================================================

  else {

    Serial.println(
        "Path clear."
    );

    buzzerOff();
  }


  delay(50);
}