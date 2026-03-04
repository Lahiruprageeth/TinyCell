#define MODEM_EC800Z // select modem type (see TinyCell.h for options)

#include <TinyCell.h>

// ----------------- PIN CONFIG ----------------
#define MODEM_TX 27
#define MODEM_RX 26

// Hardware Serial
#define SerialAT Serial2

// LED Pin
#define LED_PIN 2

TinyCell modem(SerialAT);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- TinyCell: Modem Test Example ---");

  // Initialize Modem Serial
  SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
  delay(3000);

  // Enable AT command logging to Serial
  modem.setDebug(&Serial);

  Serial.println("System: Initializing modem...");

  // Turn off local echo
  modem.sendAT("ATE0");

  Serial.println("\n--- Reading Modem Info ---");
  Serial.print("IMEI:  ");
  Serial.println(modem.getIMEI());
  Serial.print("IMSI:  ");
  Serial.println(modem.getIMSI());
  Serial.print("CCID:  ");
  Serial.println(modem.getCCID());

  Serial.println("\n--- Network Status ---");
  Serial.print("Signal Quality: ");
  Serial.print(modem.getSignalQuality());
  Serial.println(" dBm");

  if (modem.attachNetwork()) {
    Serial.println("Network: Attached successfully.");

    if (modem.activatePDP("internet")) {
      Serial.println("PDP Context: Activated.");
      Serial.print("Local IP: ");
      Serial.println(modem.getLocalIP());
    } else {
      Serial.println("PDP Context: Activation failed.");
    }
  } else {
    Serial.println("Network: Attachment failed.");
  }

  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  // Blink LED to show system is running
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink >= 1000) {
    lastBlink = millis();
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));

    // Periodically update signal quality
    Serial.print("Signal Quality: ");
    Serial.print(modem.getSignalQuality());
    Serial.println(" dBm");
  }
}
