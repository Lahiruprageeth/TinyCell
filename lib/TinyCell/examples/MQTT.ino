#define MODEM_EC800Z // select modem type

#include <PubSubClient.h>
#include <TinyCell.h>

// ----------------- CONFIG ----------------
#define MODEM_TX 27
#define MODEM_RX 26
#define SerialAT Serial2

const char *mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char *topic = "tinycell/test";

TinyCell modem(SerialAT);
QuectelClient tcClient(modem);
PubSubClient mqtt(tcClient);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- TinyCell: MQTT Example ---");

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  modem.setDebug(&Serial);

  if (modem.attachNetwork()) {
    modem.activatePDP("internet");
  }

  mqtt.setServer(mqttServer, mqttPort);
  mqtt.setCallback(callback);
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  while (!mqtt.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "TinyCellClient-";
    clientId += String(modem.getIMEI());

    if (mqtt.connect(clientId.c_str())) {
      Serial.println("connected");
      mqtt.publish(topic, "TinyCell connected");
      mqtt.subscribe(topic);
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
  if (millis() - lastMsg > 10000) {
    lastMsg = millis();
    mqtt.publish(topic, "Hello from TinyCell!");
  }
}