#define MODEM_EC800Z // select modem type

#include <PubSubClient.h>
#include <TinyCell.h>

// ----------------- CONFIG ----------------
#define MODEM_TX 27
#define MODEM_RX 26
#define SerialAT Serial2

const char *mqttServer = "mqtt.thingsboard.cloud";
const int mqttPort = 8883; // Secure MQTT port
const char *mqttToken = "YOUR_ACCESS_TOKEN";

// --- SSL/TLS CERTIFICATES (Optional) ---
const char *root_ca =
    "-----BEGIN CERTIFICATE-----\n...\n-----END CERTIFICATE-----";

TinyCell modem(SerialAT);
QuectelClientSecure tcSecureClient(modem);
PubSubClient mqtt(tcSecureClient);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- TinyCell: MQTTS (Secure MQTT) Example ---");

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  modem.setDebug(&Serial);

  // --- Handling Certificates ---
  // If your MQTT broker uses a self-signed certificate, uncomment below:
  // tcSecureClient.setCACert(root_ca);

  if (modem.attachNetwork()) {
    modem.activatePDP("internet");
  }

  // Optional: Set Root CA if needed for specific servers
  // tcSecureClient.setRootCA(root_ca_pem);

  mqtt.setServer(mqttServer, mqttPort);
}

void reconnect() {
  while (!mqtt.connected()) {
    Serial.print("Attempting Secure MQTT connection...");
    String clientId = "TinyCellSecure-";
    clientId += String(modem.getIMEI());

    // Using token for authentication as common in MQTTS (e.g. ThingsBoard)
    if (mqtt.connect(clientId.c_str(), mqttToken, NULL)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void loop() {
  if (!mqtt.connected()) {
    reconnect();
  }
  mqtt.loop();

  static unsigned long lastMsg = 0;
  if (millis() - lastMsg > 15000) {
    lastMsg = millis();
    mqtt.publish("v1/devices/me/telemetry", "{\"temperature\":25}");
  }
}
