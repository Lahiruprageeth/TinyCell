#define MODEM_EC800Z  // select modem type (see TinyCell.h for options)

#include <PubSubClient.h>
#include <TinyCell.h>

// ----------------- PIN CONFIG ----------------
#define MODEM_TX 32
#define MODEM_RX 33

// Hardware Serial
#define SerialAT Serial2

// --- AWS IoT Core settings (replace with your own endpoint and certs) ---
const char* awsEndpoint = "your-iot-endpoint.amazonaws.com";
const int awsPort = 8883;
const char* mqttTopic = "gps/tracker/data";
const char* mqttClientID = "esp32-gps-tracker";

// Example certificates (PEM format). In a real sketch you'd store these in
// PROGMEM or load from filesystem; they're shortened here for brevity.
const char awsRootCA[] PROGMEM = "-----BEGIN CERTIFICATE-----\n...\n-----END CERTIFICATE-----";
const char awsClientCert[] PROGMEM = "-----BEGIN CERTIFICATE-----\n...\n-----END CERTIFICATE-----";
const char awsClientKey[] PROGMEM = "-----BEGIN PRIVATE KEY-----\n...\n-----END PRIVATE KEY-----";

TinyCell modem(SerialAT);
QuectelClientSecure secureClient(modem);
PubSubClient mqtt(secureClient);  // secure connection for MQTT
// forward declarations
void mqttConnect();
void mqttCallback(char* topic, byte* payload, unsigned int len);
void setup() {
  Serial.begin(115200);
  // optional: echo AT traffic to monitor for debugging
  // define SerialMon if you prefer a separate serial port, otherwise use
  // normal Serial.  e.g. `#define SerialMon Serial` earlier in sketch.
  modem.setDebug(Serial);  // or SerialMon
  // Initialize quctel eg800k
  SerialAT.begin(9600, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);
modem.sendAT("AT+csq");  // set response format to include line breaks for easier parsing
delay(3000);
modem.sendAT("AT+csq");  // set response format to include line breaks for easier parsing
modem.sendAT("AT+CREG?");  // set response format to include line breaks for easier parsing
  // quick library sanity check: attach network + PDP
  Serial.println("Checking modem...");
  if (!modem.attachNetwork()) {
    Serial.println("Failed to attach to network");
    delay(5000);
    return;
  }
  Serial.println("Attached to network");

  if (!modem.activatePDP("your_apn_here")) {
    Serial.println("Failed to activate PDP");
    delay(5000);
    return;
  }
  Serial.println("PDP activated");

  // configure SSL certificates and open secure socket
  modem.setRootCA(awsRootCA);
  modem.setClientCert(awsClientCert, awsClientKey);
  if (!modem.openSSLSocket(awsEndpoint, awsPort)) {
    Serial.println("SSL socket failed");
    delay(5000);
    return;
  }
  Serial.println("SSL socket open");

  mqtt.setServer(awsEndpoint, awsPort);
  mqtt.setCallback(mqttCallback);
}

void loop() {
  // Maintain MQTT connection
  if (!mqtt.connected()) {
    mqttConnect();
  }
  mqtt.loop();
}
void mqttConnect() {
  Serial.print("Connecting to MQTT: ");
  Serial.println(awsEndpoint);

  // Connect to AWS IoT Core (no username/password required)
  if (mqtt.connect(mqttClientID)) {
    Serial.println("MQTT connected");
    // mqtt.subscribe("gps/tracker/commands");
  } else {
    Serial.print("MQTT failed, rc=");
    Serial.print(mqtt.state());
    Serial.println(" retrying in 5 seconds");
    delay(5000);
  }
}
void mqttCallback(char* topic, byte* payload, unsigned int len) {
  // Handle incoming MQTT messages
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  String message;
  for (unsigned int i = 0; i < len; i++) {
    message += (char)payload[i];
  }

  Serial.print("Message: ");
  Serial.println(message);
}