#define MODEM_EC800Z // select modem type

#include <ArduinoHttpClient.h>
#include <TinyCell.h>

// ----------------- CONFIG ----------------
#define MODEM_TX 27
#define MODEM_RX 26
#define SerialAT Serial2

const char server[] = "postman-echo.com"; // Secure test server
const int port = 443;

// --- SSL/TLS CERTIFICATES (Optional) ---
// If your server uses a self-signed cert or you want strict validation,
// provide the Root CA in PEM format here.
const char *root_ca =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIFazCCA1OgAwIBAgIRAIIWz7S6bAh9ABAEEAD... (your cert here)\n"
    "-----END CERTIFICATE-----";

// If the server requires Client Authentication (mTLS):
const char *client_cert =
    "-----BEGIN CERTIFICATE-----\n...\n-----END CERTIFICATE-----";
const char *client_key =
    "-----BEGIN RSA PRIVATE KEY-----\n...\n-----END RSA PRIVATE KEY-----";

TinyCell modem(SerialAT);
QuectelClientSecure tcSecureClient(modem);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- TinyCell: HTTPS (GET & POST) Example ---");

  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  modem.setDebug(&Serial);

  // --- Handling Certificates ---
  // Option 1: No verification (Default)
  // If you don't call setCACert, the modem will skip server verification.

  // Option 2: Server Verification (Root CA)
  // tcSecureClient.setCACert(root_ca);

  // Option 3: Client Authentication (mTLS)
  // tcSecureClient.setCertificate(client_cert, client_key);

  Serial.println("System: Attaching network...");
  if (modem.attachNetwork()) {
    if (modem.activatePDP("internet")) {
      Serial.println("System: PDP Context active.");

      // Run HTTPS tests
      performSecureGet();
      delay(2000);
      performSecurePost();
    }
  }
}

void performSecureGet() {
  HttpClient https(tcSecureClient, server, port);
  Serial.println("\nHTTPS: Starting GET request...");

  int err = https.get("/get?test=123");
  if (err == 0) {
    int status = https.responseStatusCode();
    Serial.print("HTTPS Status: ");
    Serial.println(status);

    if (status >= 0) {
      https.skipResponseHeaders();
      Serial.println("HTTPS Response Body:");
      while (https.available() || https.connected()) {
        if (https.available()) {
          Serial.print((char)https.read());
        }
      }
      Serial.println("\nHTTPS: GET Finished.");
    }
  } else {
    Serial.print("HTTPS GET failed: ");
    Serial.println(err);
  }
}

void performSecurePost() {
  HttpClient https(tcSecureClient, server, port);
  Serial.println("\nHTTPS: Starting POST request...");

  String postData = "{\"message\":\"Hello from TinyCell HTTPS\"}";
  String contentType = "application/json";

  int err = https.post("/post", contentType, postData);
  if (err == 0) {
    int status = https.responseStatusCode();
    Serial.print("HTTPS Status: ");
    Serial.println(status);

    if (status >= 0) {
      https.skipResponseHeaders();
      Serial.println("HTTPS Response Body:");
      while (https.available() || https.connected()) {
        if (https.available()) {
          Serial.print((char)https.read());
        }
      }
      Serial.println("\nHTTPS: POST Finished.");
    }
  } else {
    Serial.print("HTTPS POST failed: ");
    Serial.println(err);
  }
}

void loop() {}
