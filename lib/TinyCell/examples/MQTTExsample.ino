#include <PubSubClient.h>
#include <TinyCell.h> // your TinyCell library


QuectelAT atSerial(Serial1);
EG800KModem modem(atSerial);
QuectelClient quectelClient(modem); // TCP client for TinyCell
PubSubClient client(quectelClient); // Use it in place of WiFiClient

void setup() {
  Serial.begin(115200);

  atSerial.sendCommand("AT");
  modem.attachNetwork();
  modem.activatePDP("your_apn");

  client.setServer("broker.hivemq.com", 1883);
}

void loop() {
  if (!client.connected()) {
    while (!client.connect("TinyCellClient")) {
      Serial.println("Connecting MQTT...");
      delay(1000);
    }
  }

  client.loop();
}