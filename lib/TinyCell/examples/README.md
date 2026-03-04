# TinyCell Examples

Welcome to the **TinyCell** library examples. These sketches demonstrate how to use the high-performance features of this library for cellular connectivity.

## 📋 Example Guide

1.  **[ModemTest.ino](ModemTest.ino)**:
    *   Basic modem initialization and diagnostic info (IMEI, IMSI, CCID).
    *   Real-time signal quality monitoring.
    *   Network attachment and PDP activation tests.

2.  **[HTTP.ino](HTTP.ino)**:
    *   Demonstrates standard HTTP operations.
    *   Includes dedicated functions for **GET** and **POST** requests.
    *   Uses the `ArduinoHttpClient` library.

3.  **[HTTPS.ino](HTTPS.ino)**:
    *   Demonstrates secure encrypted HTTP operations.
    *   Includes dedicated functions for **Secure GET** and **Secure POST**.
    *   Utilizes the modem's built-in SSL engine via `QuectelClientSecure`.

4.  **[MQTT.ino](MQTT.ino)**:
    *   Integration with the `PubSubClient` library for standard MQTT.
    *   Automatic reconnection logic and telemetry publishing.

5.  **[MQTTS.ino](MQTTS.ino)**:
    *   Secure MQTT over SSL (MQTTS).
    *   Demonstrates connection to secure IoT platforms like ThingsBoard.

## ⚙️ Configuration Reminder

Before running any example, ensure you set the correct modem type in the `#define` at the top of the file:

```cpp
#define MODEM_EC800Z // or MODEM_EG800K
```

Ensure your antenna is connected and you are using a 5V/2A power supply for stable cellular performance.
