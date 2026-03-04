#pragma once
#include <Arduino.h>

class QuectelAT {
public:
  QuectelAT(Stream &serial);

  // enable printing of AT traffic to a debug stream (e.g. Serial)
  void setDebug(Stream &dbg);

  // High-level AT command execution
  bool sendCommand(const String &cmd);
  bool sendCommand(const char *cmd);

  // Provide flexible response waiting
  int waitResponse(uint32_t timeout_ms, String &data, String r1 = "OK\r\n",
                   String r2 = "ERROR\r\n", String r3 = "", String r4 = "",
                   String r5 = "");
  int waitResponse(uint32_t timeout_ms, const char *r1 = "OK\r\n",
                   const char *r2 = "ERROR\r\n", const char *r3 = "",
                   const char *r4 = "", const char *r5 = "");
  int waitResponse(const char *r1 = "OK\r\n", const char *r2 = "ERROR\r\n",
                   const char *r3 = "", const char *r4 = "") {
    return waitResponse(1000, r1, r2, r3, r4);
  }

  // Stream interaction
  Stream &stream() { return *_serial; }
  void streamWrite(const char *s);
  void streamWrite(const uint8_t *buf, size_t size);
  int streamRead();
  void streamClear();

  // Process unsolicited result codes (pseudo-background task)
  void processURC();

  // URC state tracking
  bool isRecvUrcSeen() { return _recvUrcSeen; }
  void clearRecvUrc() { _recvUrcSeen = false; }

private:
  Stream *_serial;
  Stream *_debug = nullptr;
  bool _recvUrcSeen = false;

  // Internal helper to match strings progressively
  int8_t matchString(const String &str, const String &match, uint16_t offset);
  bool _debugAtStartOfLine = true;
};