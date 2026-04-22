#include <SPI.h>
#include <Ethernet.h>          // (kept because you had it; not used on ESP32)
#include <PubSubClient.h>
#include <LoRa.h>
#include <utilities.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <time.h>
#include "esp_sntp.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>

// New
#include <WiFiClientSecure.h>
#include <xxtea-iot-crypt.h>

// =====================
// ID define
// =====================
int receiverID = 1;

// =====================
// LoRa define
// =====================
#define frequency 915E6
#define signalBandwidth 125E3
int txPower = 17;
int spreadingFactor = 7;
int codingRateDenominator = 5;

// =====================
// Screen define
// =====================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RST -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// =====================
// WiFi define
// =====================
const char* ssid = "USER";
const char* password = "Fill_in_password";
IPAddress ip;


// =====================
// MQTT define
// =====================
const char *sub_topic = "Water_node_01";
const char *pub_water_topic = "LoRaGateway/TY01/water_status";
const char *pub_init_topic = "LoRaGateway/TY01/status";
const char *mqtt_ClientID = "LoRaGateway/TY01";

const char *mqtt_server = "192.168.1.101";
const char *mqtt_userName = "USER";
const char *mqtt_password = "Fill_in_password";
int mqtt_port = 14107;

// New
WiFiClientSecure espClient;

PubSubClient client(espClient);

// =====================
// Time define
// =====================
const char *time_zone = "HKT-8";
const char *ntpServer1 = "hk.pool.ntp.org";
const char *ntpServer2 = "stdtime.gov.hk";
const long gmtOffset_sec = 28800;
const int daylightOffset_sec = 0;

// New
// MUST be the same as the sender!
String loraKey = "Fill_in_password";

// New
const char* root_ca = R"EOF(
-----BEGIN CERTIFICATE-----
Fill_in_cert
-----END CERTIFICATE-----  
)EOF";

const char* client_cert = R"EOF(
-----BEGIN CERTIFICATE-----
Fill_in_cert
-----END CERTIFICATE-----
)EOF";

const char* client_key = R"EOF(
-----BEGIN PRIVATE KEY-----
Fill_in_cert
-----END PRIVATE KEY-----
)EOF";


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
struct tm timeinfo;
char getTime[25];

// =====================
// Traffic Light Pins + buzzer
// =====================
#define GREEN_LED_PIN 25
#define YELLOW_LED_PIN 13
#define RED_LED_PIN 14
#define BUZZER_PIN 2

// =====================
// Sender table (scalable)
// =====================
#define MAX_SENDERS 300
#define SENDER_ACTIVE_MS 30000

struct SenderData {
  int id;
  int packets;
  float waterLevel;
  String location;
  String status;
  unsigned long lastUpdate;
  bool needsUpdate;
  bool used;
};

SenderData senders[MAX_SENDERS];

// =====================
// JSON for MQTT
// =====================
StaticJsonDocument<256> json_doc;

// =====================
// Forward declarations
// =====================
void callback(char *topic, byte *payload, unsigned int length);
void reconnect();
void printLocalTime();
void timeavailable(struct timeval *t);
void getWaterlevel();
void publishPendingSenders();

void initSenders();
int findOrCreateSender(int id);

void updateTrafficLights();
int severityOf(const String &st);
int pickTopSender(int skipIdx);
void updateDisplay();

bool parseLoRaMessage(const String &loraMsg,
                      int &senderIDOut,
                      String &locationOut,
                      int &packetsOut,
                      float &waterLevelOut,
                      String &statusOut);

// =====================
// Setup
// =====================
void setup() {
  // IO setup
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  // Serial
  Serial.begin(115200);
  Serial.println("LoRa Receiver Starting");
  xxtea.setKey(loraKey);
  // I2C + OLED
  Wire.begin(I2C_SDA, I2C_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) {
    Serial.println("SSD1306 allocation failed!");
    for (;;);
  }
  Serial.println("SSD1306 allocation OK!");

  // reset OLED display via software (OLED_RST = -1 on many boards; keep as your original)
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(50);
  digitalWrite(OLED_RST, HIGH);

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("LoRa Receiver Ready");
  display.display();

  // init sender table
  initSenders();

  // LoRa SPI pins
  SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_CS_PIN);
  LoRa.setPins(RADIO_CS_PIN, RADIO_RST_PIN, RADIO_DIO0_PIN);

  if (!LoRa.begin(frequency)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  LoRa.setTxPower(txPower);
  LoRa.setSpreadingFactor(spreadingFactor);
  LoRa.setSignalBandwidth(signalBandwidth);
  LoRa.setCodingRate4(codingRateDenominator);
  LoRa.disableCrc();
  LoRa.enableInvertIQ();

  Serial.println("LoRa Initializing OK!");

  // Time
  Serial.println("Contacting Time Server");
  sntp_set_time_sync_notification_cb(timeavailable);
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
  configTzTime(time_zone, ntpServer1, ntpServer2);

  // Wifi SSL
  espClient.setCACert(root_ca);      // Verify the server
  espClient.setCertificate(client_cert); // Identify the ESP32
  espClient.setPrivateKey(client_key);   // Prove ownership of the ID
  // MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  updateTrafficLights();
  updateDisplay();

}

// =====================
// Main loop
// =====================
void loop() {
  // --- WiFi connect (non-blocking-ish with 10s timeout) ---
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to WiFi");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("LoRa Receiver");
    display.setCursor(0, 20);
    display.println("Connecting WiFi..");
    digitalWrite(BUZZER_PIN, HIGH);
    display.display();


    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startAttempt) < 10000) {
      delay(100);
      Serial.print(".");

      // still allow breaking out if a LoRa packet arrives
      if (LoRa.parsePacket()) break;
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to WiFi");
      ip = WiFi.localIP();
      Serial.print("IP address: ");
      Serial.println(ip);
      digitalWrite(BUZZER_PIN, LOW);
    } else {
      Serial.println("\nFailed to connect to WiFi");
      // keep buzzer as-is or turn off; your previous logic kept it on during connect screen
      digitalWrite(BUZZER_PIN, LOW);
    }
  }

  // --- Publish any pending sender updates ---
  publishPendingSenders();

  // --- LoRa receive ---
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String loraMsg;
    while (LoRa.available()) loraMsg += (char)LoRa.read();

    // New 
    loraMsg = xxtea.decrypt(loraMsg);
    Serial.println(loraMsg);

    Serial.println("Received packet");
    Serial.println("Msg: " + loraMsg);
    Serial.print("RSSI: ");
    Serial.print(LoRa.packetRssi());
    Serial.print("; SNR: ");
    Serial.println(LoRa.packetSnr());

    int senderIDInt = 0;
    String location;
    int packetsInt = 0;
    float waterLevelFloat = 0.0f;
    String status;

    if (!parseLoRaMessage(loraMsg, senderIDInt, location, packetsInt, waterLevelFloat, status)) {
      Serial.println("Invalid packet format");
      return; // keep your behavior: exit loop iteration
    }

    int idx = findOrCreateSender(senderIDInt);
    if (idx >= 0) {
      senders[idx].packets = packetsInt;
      senders[idx].waterLevel = waterLevelFloat;
      senders[idx].location = location;
      senders[idx].status = status;
      senders[idx].lastUpdate = millis();
      senders[idx].needsUpdate = true;

      Serial.printf("Updated sender %d, loc=%s, pkt=%d, lvl=%.1f, st=%s\n",
                    senderIDInt, location.c_str(), packetsInt, waterLevelFloat, status.c_str());
    } else {
      Serial.println("Sender table full; cannot store new sender!");
    }

    updateTrafficLights();
    updateDisplay();
  } else {
    // Periodic refresh
    static unsigned long lastDisplayTime = 0;
    if (millis() - lastDisplayTime >= 5000) {
      lastDisplayTime = millis();
      updateTrafficLights();
      updateDisplay();
    }
  }
}

// =====================
// Sender table helpers
// =====================
void initSenders() {
  for (int i = 0; i < MAX_SENDERS; i++) {
    senders[i].used = false;
    senders[i].needsUpdate = false;
    senders[i].lastUpdate = 0;
  }
}

int findOrCreateSender(int id) {
  for (int i = 0; i < MAX_SENDERS; i++) {
    if (senders[i].used && senders[i].id == id) return i;
  }
  for (int i = 0; i < MAX_SENDERS; i++) {
    if (!senders[i].used) {
      senders[i].used = true;
      senders[i].id = id;
      senders[i].packets = 0;
      senders[i].waterLevel = 0.0f;
      senders[i].location = "";
      senders[i].status = "green";
      senders[i].lastUpdate = 0;
      senders[i].needsUpdate = false;
      return i;
    }
  }
  return -1;
}

// =====================
// Traffic lights (aggregate severity)
// =====================
void updateTrafficLights() {
  bool anyRed = false;
  bool anyYellow = false;

  unsigned long now = millis();
  for (int i = 0; i < MAX_SENDERS; i++) {
    if (!senders[i].used) continue;
    if (now - senders[i].lastUpdate > SENDER_ACTIVE_MS) continue;

    if (senders[i].status == "red") anyRed = true;
    else if (senders[i].status == "yellow") anyYellow = true;
  }

  if (anyRed) {
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, HIGH);
  } else if (anyYellow) {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
  } else {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(YELLOW_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, LOW);
  }
}

// =====================
// OLED: show top-2 by severity then recency
// =====================
int severityOf(const String &st) {
  if (st == "red") return 2;
  if (st == "yellow") return 1;
  return 0;
}

int pickTopSender(int skipIdx) {
  int best = -1;
  int bestSev = -1;
  unsigned long bestTime = 0;
  unsigned long now = millis();

  for (int i = 0; i < MAX_SENDERS; i++) {
    if (!senders[i].used) continue;
    if (i == skipIdx) continue;
    if (now - senders[i].lastUpdate > SENDER_ACTIVE_MS) continue;

    int sev = severityOf(senders[i].status);

    if (best == -1 ||
        sev > bestSev ||
        (sev == bestSev && senders[i].lastUpdate > bestTime)) {
      best = i;
      bestSev = sev;
      bestTime = senders[i].lastUpdate;
    }
  }
  return best;
}

void updateDisplay() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("LoRa Receiver");

  int idx1 = pickTopSender(-1);
  int idx2 = pickTopSender(idx1);

  // 1st
  display.setCursor(0, 12);
  display.print("1:");
  if (idx1 >= 0) {
    display.print("ID");
    display.print(senders[idx1].id);
    display.print(" ");
    display.print(senders[idx1].location);

    display.setCursor(0, 22);
    display.print("Lvl:");
    display.print(senders[idx1].waterLevel, 1);

    display.setCursor(70, 22);
    display.print("St:");
    display.print(senders[idx1].status);
  } else {
    display.print("No active sender");
  }

  // 2nd
  display.setCursor(0, 34);
  display.print("2:");
  if (idx2 >= 0) {
    display.print("ID");
    display.print(senders[idx2].id);
    display.print(" ");
    display.print(senders[idx2].location);

    display.setCursor(0, 44);
    display.print("Lvl:");
    display.print(senders[idx2].waterLevel, 1);

    display.setCursor(70, 44);
    display.print("St:");
    display.print(senders[idx2].status);
  } else {
    display.print("-");
  }

  // WiFi
  display.setCursor(0, 56);
  display.print("WiFi:");
  display.print(WiFi.status() == WL_CONNECTED ? "OK" : "NO");

  display.display();
}

// =====================
// LoRa message parser
// =====================
bool parseLoRaMessage(const String &loraMsg,
                      int &senderIDOut,
                      String &locationOut,
                      int &packetsOut,
                      float &waterLevelOut,
                      String &statusOut) {
  // Expect 7 commas
  int delimiter[7];
  delimiter[0] = loraMsg.indexOf(',');
  for (int i = 1; i < 7; i++) {
    delimiter[i] = loraMsg.indexOf(',', delimiter[i - 1] + 1);
  }
  if (delimiter[0] == -1 || delimiter[6] == -1) return false;

  String senderID = loraMsg.substring(0, delimiter[0]);
  String location = loraMsg.substring(delimiter[0] + 1, delimiter[1]);
  String packets = loraMsg.substring(delimiter[1] + 1, delimiter[2]);
  // String txPowerStr = loraMsg.substring(delimiter[2] + 1, delimiter[3]);   // not used
  // String sfStr      = loraMsg.substring(delimiter[3] + 1, delimiter[4]);   // not used
  // String crStr      = loraMsg.substring(delimiter[4] + 1, delimiter[5]);   // not used
  String waterLevel = loraMsg.substring(delimiter[5] + 1, delimiter[6]);
  String status = loraMsg.substring(delimiter[6] + 1);

  senderID.trim();
  location.trim();
  packets.trim();
  waterLevel.trim();
  status.trim();

  senderIDOut = senderID.toInt();
  locationOut = location;
  packetsOut = packets.toInt();
  waterLevelOut = waterLevel.toFloat();
  statusOut = status;

  // basic sanity
  if (senderIDOut == 0 && senderID != "0") return false;
  if (locationOut.length() == 0) return false;
  if (statusOut.length() == 0) return false;

  return true;
}

// =====================
// Time helpers
// =====================
void printLocalTime() {
  if (!getLocalTime(&timeinfo)) {
    Serial.println("No time available (yet)");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void timeavailable(struct timeval *t) {
  Serial.println("Got time adjustment from NTP!");
  printLocalTime();
}

void getWaterlevel() {
  getLocalTime(&timeinfo);
  sprintf(getTime, "%04d-%02d-%02d,%02d:%02d:%02d",
          timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
          timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  Serial.println("Getting Time");
  Serial.println(getTime);
}

// =====================
// MQTT
// =====================
void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (unsigned int i = 0; i < length; i++) Serial.print((char)payload[i]);
  Serial.println();
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    if (client.connect(mqtt_ClientID, mqtt_userName, mqtt_password)) {
      Serial.print("Connected with Client ID: ");
      Serial.println(mqtt_ClientID);
      client.publish(pub_init_topic, "Hi, I'm online!");
      client.subscribe(sub_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
      time_t now = time(nullptr);
      Serial.print("ESP32 thinks the time is: ");
      Serial.println(ctime(&now)); // This will print the human-readable date
    }
  }
}

void publishPendingSenders() {
  if (WiFi.status() != WL_CONNECTED) return;

  for (int i = 0; i < MAX_SENDERS; i++) {
    if (!senders[i].used || !senders[i].needsUpdate) continue;

    if (!client.connected()) reconnect();
    client.loop();

    getWaterlevel();  // update getTime

    json_doc.clear();
    json_doc["receiverID"] = String(receiverID);
    json_doc["sendTime"] = String(getTime);
    json_doc["location"] = senders[i].location;
    json_doc["waterlevel"] = String(senders[i].waterLevel, 1);
    json_doc["id"] = String(senders[i].id);
    json_doc["waterlevel_status"] = senders[i].status;

    char mqttPayload[256];
    serializeJson(json_doc, mqttPayload);

    if (client.publish(pub_water_topic, mqttPayload)) {
      Serial.printf("MQTT published sender %d\n", senders[i].id);
      senders[i].needsUpdate = false;
    } else {
      Serial.printf("MQTT publish failed sender %d\n", senders[i].id);
      // keep needsUpdate = true so it retries next loop
    }
  }
}


