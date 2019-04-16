#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "time.h"

/**************************** DEBUG *******************************/
#define DEBUG true

#if DEBUG
#define DEBUG_PRINTLN(m) Serial.println(m)
#define DEBUG_PRINT(m) Serial.print(m)

#define DEBUG_PRINTLNC(m) Serial.println("[Core " + String(xPortGetCoreID()) + "]" + m)
#define DEBUG_PRINTC(m) Serial.print("[Core " + String(xPortGetCoreID()) + "]" + m)

#else
#define DEBUG_PRINTLN(m)
#define DEBUG_PRINT(m)

#define DEBUG_PRINTLNC(m)
#define DEBUG_PRINTC(m)
#endif

#define LED 2
const char* mqtt_server = "192.168.0.150";

const char* ntpServer = "a.st1.ntp.br";
const long  gmtOffset_sec = -3 * 3600;
const int   daylightOffset_sec = 0;
int maxLogSize = 20;

WiFiClient mqttClient;
PubSubClient mqtt(mqttClient);
Preferences preferences;

void setupNtp() {
  DEBUG_PRINTLNC("[NTP] Setup");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  DEBUG_PRINTC("[NTP] Now: ");
  DEBUG_PRINTLN(getTimestamp());
}

String getTimestamp() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    DEBUG_PRINTLNC("[NTP] Failed to obtain time");
    return "";
  }

  time_t now;
  time(&now);

  return String(now);
}

void reconnect() {
  while (!mqtt.connected()) {
    DEBUG_PRINTC("Attempting MQTT connection...");

    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    if (mqtt.connect(clientId.c_str())) {
      DEBUG_PRINTLN("connected");

      mqtt.subscribe("esp32/rpc/request");
    } else {
      DEBUG_PRINTC("failed, rc=");
      DEBUG_PRINT(mqtt.state());
      DEBUG_PRINTLN(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  DEBUG_PRINTC("Message arrived [");
  DEBUG_PRINT(topic);
  DEBUG_PRINT("] ");

  String msg;

  for (int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  DEBUG_PRINTLN(msg);

  String result = processRPC(msg);

  mqtt.publish("esp32/rpc/response", result.c_str());
}

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin();
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    DEBUG_PRINTLNC("WiFi Failed!");
    return;
  }

  setupNtp();

  preferences.begin("logs", false);
  preferences.clear();
  preferences.end();

  mqtt.setServer(mqtt_server, 1883);
  mqtt.setCallback(callback);
}

void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }
  mqtt.loop();
}
