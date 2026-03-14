#define MODEM_EC800Z // select modem type

#include <ArduinoHttpClient.h>
#include <LPCell.h>

// ----------------- CONFIG ----------------
#define MODEM_TX 27
#define MODEM_RX 26
#define SerialAT Serial2

// --- SSL/TLS CERTIFICATES ---
// Amazon Root CA 1 (Used by JSONPlaceholder/AWS)
const char *root_ca =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
    "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDEwxBbWF6\n"
    "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
    "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
    "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
    "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
    "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
    "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
    "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
    "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
    "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ4EFgEFIQYz\n"
    "IU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUAA4IBAQCY8jdaQZChGsV2\n"
    "USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDIU5PMCCjjmCXPI6T53iHT\n"
    "fIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUsN+gDS63pYaACbvXy8MWy\n"
    "7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vvo/ufQJVtMVT8QtPHRh8j\n"
    "rdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU5MsI+yMRQ+hDKXJioald\n"
    "XgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpyrqXRfboQnoZsG4q5WTP4\n"
    "68SQvvG5\n"
    "-----END CERTIFICATE-----";

LPCell modem(SerialAT);
QuectelClientSecure tcSecureClient(modem);

// Forward declaration
void performHTTPSGet();

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- LPCell: HTTPS GET (JSONPlaceholder) ---");

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  // Enable AT logging
  modem.setDebug(&Serial);

  // Set the Root CA for verification
  tcSecureClient.setCACert(root_ca);

  Serial.println("System: Attaching network...");
  if (modem.attachNetwork()) {
    Serial.println("System: Activating PDP Context...");
    if (modem.activatePDP("internet")) {
      Serial.println("System: PDP Context active.");

      // Perform HTTPS GET
      performHTTPSGet();
    }
  }
}

void performHTTPSGet() {
  const char *host = "jsonplaceholder.typicode.com";
  const int port = 443;

  HttpClient https(tcSecureClient, host, port);

  Serial.println(
      "\nHTTPS: Requesting https://jsonplaceholder.typicode.com/todos/1 ...");

  int err = https.get("/todos/1");
  if (err == 0) {
    int status = https.responseStatusCode();
    Serial.print("HTTPS Status Code: ");
    Serial.println(status);

    if (status >= 0) {
      https.skipResponseHeaders();

      Serial.println("HTTPS Response Body:");
      while (https.available() || https.connected()) {
        if (https.available()) {
          char c = https.read();
          Serial.print(c);
        }
      }
      Serial.println("\n--- Request Finished ---");
    }
  } else {
    Serial.print("HTTPS GET failed error code: ");
    Serial.println(err);
  }

  https.stop();
}

void loop() {
  // Blink LED to show we are alive
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink >= 1000) {
    lastBlink = millis();
    digitalWrite(2, !digitalRead(2));
  }
}