#define MODEM_EC800Z // select modem type (see TinyCell.h for options)

#include <PubSubClient.h>
#include <TinyCell.h>

// ----------------- PIN CONFIG ----------------
#define MODEM_TX 27
#define MODEM_RX 26

// Hardware Serial
#define SerialAT Serial2

// LED Pin
#define LED_PIN 17

// --- ThingsBoard Settings ---
const char *mqttServer = "mqtt.thingsboard.cloud";
const int mqttPort = 1883;
const char *mqttTopic = "v1/devices/me/telemetry";
const char *mqttToken = "tzNQG1X4iM9B3mvggBGC"; // Your Access Token

TinyCell modem(SerialAT);
QuectelClient mqttClient(modem);
PubSubClient mqtt(mqttClient);

String uniqueClientID;

// forward declarations
void mqttConnect();
void mqttCallback(char *topic, byte *payload, unsigned int len);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- TinyCell Startup (ThingsBoard) ---");

  modem.setDebug(Serial);
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  // Turn off local echo to reduce serial buffer noise
  modem.sendAT("ATE0");

  Serial.println("Reading Modem Info...");
  String imei = modem.getIMEI();
  if (imei.length() == 0) {
    Serial.println("System: Failed to read IMEI. Retrying logic...");
    // Try again or use a fallback
    imei = "860000000000000";
  }
  Serial.print("  IMEI: ");
  Serial.println(imei);
  Serial.print("  IMSI: ");
  Serial.println(modem.getIMSI());
  Serial.print("  ICCID: ");
  Serial.println(modem.getCCID());
  Serial.print("  Signal: ");
  Serial.println(modem.getSignalQuality());

  // Use IMEI to create a unique ClientID
  uniqueClientID = "TinyCell_" + imei;

  if (!modem.attachNetwork()) {
    Serial.println("System: Network attachment failed!");
    return;
  }
  Serial.println("System: Network attached.");

  if (!modem.activatePDP("internet")) {
    Serial.println("System: PDP Context Activation failed!");
    return;
  }
  Serial.println("System: PDP Context Activated.");

  // Setup MQTT
  mqtt.setServer(mqttServer, mqttPort);
  mqtt.setCallback(mqttCallback);

  pinMode(LED_PIN, OUTPUT);
  Serial.println("System: Setup Complete. Ready to connect MQTT.");
}

void loop() {
  if (!mqtt.connected()) {
    mqttConnect();
  }
  mqtt.loop();

  // Send telemetry every 15 seconds
  static unsigned long lastTelemetry = 0;
  if (millis() - lastTelemetry >= 15000) {
    lastTelemetry = millis();
    if (mqtt.connected()) {
      Serial.println(">>> Sending Telemetry to ThingsBoard...");
      // Strictly valid flat JSON for ThingsBoard
      bool ok = mqtt.publish(mqttTopic, "{\"temperature\":25}");
      if (ok) {
        Serial.println(">>> Publish SUCCESS");
      } else {
        Serial.println(">>> Publish FAILED (Check broker state)");
      }
    }
  }

  static unsigned long lastBlink = 0;
  if (millis() - lastBlink >= 1000) {
    lastBlink = millis();
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
}

void mqttConnect() {
  Serial.print("MQTT: Connecting to ");
  Serial.println(mqttServer);

  // For ThingsBoard:
  // Client ID = uniqueClientID (IMEI-based)
  // Username = Token
  // Password = NULL
  mqtt.setKeepAlive(60);

  if (mqtt.connect(uniqueClientID.c_str(), mqttToken, NULL)) {
    Serial.println("MQTT: Connected Successfully!");
    mqtt.subscribe(mqttTopic);
  } else {
    Serial.print("MQTT: Connect Failed, rc=");
    Serial.print(mqtt.state());
    Serial.println(". (Check Token). Retrying in 5s...");
    delay(5000);
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int len) {
  Serial.print("Message [");
  Serial.print(topic);
  Serial.print("]: ");
  for (unsigned int i = 0; i < len; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
