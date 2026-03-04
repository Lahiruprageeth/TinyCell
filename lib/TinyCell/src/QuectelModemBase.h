#pragma once
#include <Arduino.h>

class QuectelModemBase {
public:
  virtual bool attachNetwork() = 0;
  virtual bool activatePDP(const char *apn) = 0;

  virtual bool openSocket(const char *host, uint16_t port) = 0;
  virtual bool openSSLSocket(const char *host, uint16_t port) = 0;
  virtual void restart() = 0;
  virtual int getSignalQuality() = 0;
  virtual String getIMEI() = 0;
  virtual String getIMSI() = 0;
  virtual String getCCID() = 0;

  // enable debug output from the underlying AT layer
  virtual void setDebug(Stream &dbg) {}

  virtual int send(const uint8_t *data, size_t len) = 0;
  virtual int available() = 0;
  virtual int receive(uint8_t *buffer, size_t len) = 0;
  virtual void closeSocket() = 0;
  virtual uint8_t connected() = 0;

  // Convenience AT/IO helpers used by higher-level clients (secure client)
  // Send a formatted AT command (printf-style). Implementations should
  // format and send the command via their internal AT helper.
  virtual bool sendAT(const char *fmt, ...) {
    (void)fmt;
    return false;
  }

  // Wait for modem response using simple pattern list (no timeout)
  virtual int waitResponse(const char *r1 = "OK\r\n",
                           const char *r2 = "ERROR\r\n", const char *r3 = "",
                           const char *r4 = "", const char *r5 = "") {
    (void)r1;
    (void)r2;
    (void)r3;
    (void)r4;
    (void)r5;
    return 0;
  }

  // Forwarding helpers for socket IO (default implementations delegate to
  // the required abstract socket methods).
  virtual size_t write(const uint8_t *buf, size_t size) {
    return send(buf, size);
  }
  virtual int read(uint8_t *buf, size_t size) { return receive(buf, size); }
  virtual int read() {
    uint8_t b;
    return (receive(&b, 1) == 1) ? b : -1;
  }
  virtual void flush() {}

  // Check whether a (logical) socket id is connected. Default forwards to
  // `connected()` which may be modem-specific.
  virtual bool isConnected(int /*socketId*/) { return connected() != 0; }
};