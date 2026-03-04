#define MODEM_EC800Z // select modem type

#include <ArduinoHttpClient.h>
#include <TinyCell.h>

// ----------------- CONFIG ----------------
#define MODEM_TX 27
#define MODEM_RX 26
#define SerialAT Serial2

const char server[] = "vsh.pp.ua"; // A simple HTTP test server
const int port = 80;

TinyCell modem(SerialAT);
QuectelClient tcClient(modem);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- TinyCell: HTTP (GET & POST) Example ---");

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  modem.setDebug(&Serial);

  Serial.println("System: Attaching network...");
  if (modem.attachNetwork()) {
    if (modem.activatePDP("internet")) {
      Serial.println("System: PDP Context active.");

      // Run HTTP tests
      performGetRequest();
      delay(2000);
      performPostRequest();
    }
  }
}

void performGetRequest() {
  HttpClient http(tcClient, server, port);
  Serial.println("\nHTTP: Starting GET request...");

  int err = http.get("/info.php");
  if (err == 0) {
    int status = http.responseStatusCode();
    Serial.print("HTTP Status: ");
    Serial.println(status);

    if (status >= 0) {
      http.skipResponseHeaders();
      Serial.println("HTTP Response Body:");
      while (http.available() || http.connected()) {
        if (http.available()) {
          Serial.print((char)http.read());
        }
      }
      Serial.println("\nHTTP: GET Finished.");
    }
  } else {
    Serial.print("HTTP GET failed: ");
    Serial.println(err);
  }
}

void performPostRequest() {
  HttpClient http(tcClient, server, port);
  Serial.println("\nHTTP: Starting POST request...");

  String postData = "{\"sensor\":\"temperature\",\"value\":25.5}";
  String contentType = "application/json";

  int err = http.post("/", contentType, postData);
  if (err == 0) {
    int status = http.responseStatusCode();
    Serial.print("HTTP Status: ");
    Serial.println(status);

    if (status >= 0) {
      http.skipResponseHeaders();
      Serial.println("HTTP Response Body:");
      while (http.available() || http.connected()) {
        if (http.available()) {
          Serial.print((char)http.read());
        }
      }
      Serial.println("\nHTTP: POST Finished.");
    }
  } else {
    Serial.print("HTTP POST failed: ");
    Serial.println(err);
  }
}

void loop() {}
